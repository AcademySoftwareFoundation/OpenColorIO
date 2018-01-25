/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/*
    Made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
*/


#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "MatrixOps.h"
#include "MathUtils.h"

#include <cstring>
#include <sstream>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        void ApplyScale(float* rgbaBuffer, long numPixels,
                        const float* scale4)
        {
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] *= scale4[0];
                rgbaBuffer[1] *= scale4[1];
                rgbaBuffer[2] *= scale4[2];
                rgbaBuffer[3] *= scale4[3];
                
                rgbaBuffer += 4;
            }
        }
        
        void ApplyOffset(float* rgbaBuffer, long numPixels,
                         const float* offset4)
        {
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                rgbaBuffer[0] += offset4[0];
                rgbaBuffer[1] += offset4[1];
                rgbaBuffer[2] += offset4[2];
                rgbaBuffer[3] += offset4[3];
                
                rgbaBuffer += 4;
            }
        }
        
        void ApplyMatrix(float* rgbaBuffer, long numPixels,
                         const float* mat44)
        {
            float r,g,b,a;
            
            for(long pixelIndex=0; pixelIndex<numPixels; ++pixelIndex)
            {
                r = rgbaBuffer[0];
                g = rgbaBuffer[1];
                b = rgbaBuffer[2];
                a = rgbaBuffer[3];
                
                rgbaBuffer[0] = r*mat44[0] + g*mat44[1] + b*mat44[2] + a*mat44[3];
                rgbaBuffer[1] = r*mat44[4] + g*mat44[5] + b*mat44[6] + a*mat44[7];
                rgbaBuffer[2] = r*mat44[8] + g*mat44[9] + b*mat44[10] + a*mat44[11];
                rgbaBuffer[3] = r*mat44[12] + g*mat44[13] + b*mat44[14] + a*mat44[15];
                
                rgbaBuffer += 4;
            }
        }
    }
    
    
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    
    
    
    
    namespace
    {
        class MatrixOffsetOp : public Op
        {
        public:
            MatrixOffsetOp(const float * m44,
                           const float * offset4,
                           TransformDirection direction);
            virtual ~MatrixOffsetOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual bool isNoOp() const;
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;
            virtual bool canCombineWith(const OpRcPtr & op) const;
            virtual void combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const;
            
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual bool supportsGpuShader() const;
            virtual void writeGpuShader(std::ostream & shader,
                                        const std::string & pixelName,
                                        const GpuShaderDesc & shaderDesc) const;
        
        private:
            bool m_isNoOp;
            float m_m44[16];
            float m_offset4[4];
            TransformDirection m_direction;
            
            // Set in finalize
            bool m_m44IsIdentity;
            bool m_m44IsDiagonal;
            bool m_offset4IsIdentity;
            float m_m44_inv[16];
            std::string m_cacheID;
        };
        
        
        typedef OCIO_SHARED_PTR<MatrixOffsetOp> MatrixOffsetOpRcPtr;
        
        
        MatrixOffsetOp::MatrixOffsetOp(const float * m44,
                                       const float * offset4,
                                       TransformDirection direction):
                                       Op(),
                                       m_isNoOp(false),
                                       m_direction(direction),
                                       m_m44IsIdentity(false),
                                       m_offset4IsIdentity(false)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot apply MatrixOffsetOp op, unspecified transform direction.");
            }
            
            memcpy(m_m44, m44, 16*sizeof(float));
            memcpy(m_offset4, offset4, 4*sizeof(float));
            
            memset(m_m44_inv, 0, 16*sizeof(float));
            
            // This Op will be a NoOp if and old if both the offset and matrix
            // are identity. This hold true no matter what the direction is,
            // so we can compute this ahead of time.
            m_isNoOp = (IsVecEqualToZero(m_offset4, 4) && IsM44Identity(m_m44));
        }
        
        OpRcPtr MatrixOffsetOp::clone() const
        {
            OpRcPtr op = OpRcPtr(new MatrixOffsetOp(m_m44, m_offset4, m_direction));
            return op;
        }
        
        MatrixOffsetOp::~MatrixOffsetOp()
        { }
        
        std::string MatrixOffsetOp::getInfo() const
        {
            return "<MatrixOffsetOp>";
        }
        
        std::string MatrixOffsetOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        bool MatrixOffsetOp::isNoOp() const
        {
            return m_isNoOp;
        }
        
        bool MatrixOffsetOp::isSameType(const OpRcPtr & op) const
        {
            MatrixOffsetOpRcPtr typedRcPtr = DynamicPtrCast<MatrixOffsetOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool MatrixOffsetOp::isInverse(const OpRcPtr & op) const
        {
            MatrixOffsetOpRcPtr typedRcPtr = DynamicPtrCast<MatrixOffsetOp>(op);
            if(!typedRcPtr) return false;
            
            if(GetInverseTransformDirection(m_direction) != typedRcPtr->m_direction)
                return false;
            
            float error = std::numeric_limits<float>::min();
            if(!VecsEqualWithRelError(m_m44, 16, typedRcPtr->m_m44, 16, error))
                return false;
            if(!VecsEqualWithRelError(m_offset4, 4,typedRcPtr->m_offset4, 4, error))
                return false;
            
            return true;
        }
        
        bool MatrixOffsetOp::canCombineWith(const OpRcPtr & op) const
        {
            return isSameType(op);
        }
        
        void MatrixOffsetOp::combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const
        {
            MatrixOffsetOpRcPtr typedRcPtr = DynamicPtrCast<MatrixOffsetOp>(secondOp);
            if(!typedRcPtr)
            {
                std::ostringstream os;
                os << "MatrixOffsetOp can only be combined with other ";
                os << "MatrixOffsetOps.  secondOp:" << secondOp->getInfo();
                throw Exception(os.str().c_str());
            }
            
            float mout[16];
            float vout[4];
            
            if(m_direction == TRANSFORM_DIR_FORWARD &&
                typedRcPtr->m_direction == TRANSFORM_DIR_FORWARD)
            {
                GetMxbCombine(mout, vout,
                              m_m44, m_offset4,
                              typedRcPtr->m_m44, typedRcPtr->m_offset4);
            }
            else if(m_direction == TRANSFORM_DIR_FORWARD &&
                    typedRcPtr->m_direction == TRANSFORM_DIR_INVERSE)
            {
                float minv2[16];
                float vinv2[4];
                
                if(!GetMxbInverse(minv2, vinv2, typedRcPtr->m_m44, typedRcPtr->m_offset4))
                {
                    std::ostringstream os;
                    os << "Cannot invert second MatrixOffsetOp op. ";
                    os << "Matrix inverse does not exist for (";
                    for(int i=0; i<16; ++i)
                    {
                        os << typedRcPtr->m_m44[i] << " ";
                    }
                    os << ").";
                    throw Exception(os.str().c_str());
                }
                
                GetMxbCombine(mout, vout,
                              m_m44, m_offset4,
                              minv2, vinv2);
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE &&
                    typedRcPtr->m_direction == TRANSFORM_DIR_FORWARD)
            {
                float minv1[16];
                float vinv1[4];
                
                if(!GetMxbInverse(minv1, vinv1, m_m44, m_offset4))
                {
                    std::ostringstream os;
                    os << "Cannot invert primary MatrixOffsetOp op. ";
                    os << "Matrix inverse does not exist for (";
                    for(int i=0; i<16; ++i)
                    {
                        os << m_m44[i] << " ";
                    }
                    os << ").";
                    throw Exception(os.str().c_str());
                }
                
                GetMxbCombine(mout, vout,
                              minv1, vinv1,
                              typedRcPtr->m_m44, typedRcPtr->m_offset4);
                
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE &&
                    typedRcPtr->m_direction == TRANSFORM_DIR_INVERSE)
            {
                float minv1[16];
                float vinv1[4];
                float minv2[16];
                float vinv2[4];
                
                if(!GetMxbInverse(minv1, vinv1, m_m44, m_offset4))
                {
                    std::ostringstream os;
                    os << "Cannot invert primary MatrixOffsetOp op. ";
                    os << "Matrix inverse does not exist for (";
                    for(int i=0; i<16; ++i)
                    {
                        os << m_m44[i] << " ";
                    }
                    os << ").";
                    throw Exception(os.str().c_str());
                }
                
                if(!GetMxbInverse(minv2, vinv2, typedRcPtr->m_m44, typedRcPtr->m_offset4))
                {
                    std::ostringstream os;
                    os << "Cannot invert second MatrixOffsetOp op. ";
                    os << "Matrix inverse does not exist for (";
                    for(int i=0; i<16; ++i)
                    {
                        os << typedRcPtr->m_m44[i] << " ";
                    }
                    os << ").";
                    throw Exception(os.str().c_str());
                }
                
                GetMxbCombine(mout, vout,
                              minv1, vinv1,
                              minv2, vinv2);
            }
            else
            {
                std::ostringstream os;
                os << "MatrixOffsetOp cannot combine ops with unspecified ";
                os << "directions. First op: " << m_direction << " ";
                os << "secondOp:" << typedRcPtr->m_direction;
                throw Exception(os.str().c_str());
            }
            
            CreateMatrixOffsetOp(ops,
                                 mout, vout,
                                 TRANSFORM_DIR_FORWARD);
        }
        
        bool MatrixOffsetOp::hasChannelCrosstalk() const
        {
            return (!m_m44IsDiagonal);
        }
        
        void MatrixOffsetOp::finalize()
        {
            m_offset4IsIdentity = IsVecEqualToZero(m_offset4, 4);
            m_m44IsIdentity = IsM44Identity(m_m44);
            m_m44IsDiagonal = IsM44Diagonal(m_m44);
            
            if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                if(!GetM44Inverse(m_m44_inv, m_m44))
                {
                    std::ostringstream os;
                    os << "Cannot apply MatrixOffsetOp op. ";
                    os << "Matrix inverse does not exist for m44 (";
                    for(int i=0; i<16; ++i) os << m_m44[i] << " ";
                    os << ").";
                    throw Exception(os.str().c_str());
                }
            }
            
            // Create the cacheID
            md5_state_t state;
            md5_byte_t digest[16];
            md5_init(&state);
            md5_append(&state, (const md5_byte_t *)m_m44,     (int)(16*sizeof(float)));
            md5_append(&state, (const md5_byte_t *)m_offset4, (int)(4*sizeof(float)));
            md5_finish(&state, digest);
            
            std::ostringstream cacheIDStream;
            cacheIDStream << "<MatrixOffsetOp ";
            cacheIDStream << GetPrintableHash(digest) << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }
        
        void MatrixOffsetOp::apply(float* rgbaBuffer, long numPixels) const
        {
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                if(!m_m44IsIdentity)
                {
                    if(m_m44IsDiagonal)
                    {
                        float scale[4];
                        GetM44Diagonal(scale, m_m44);
                        ApplyScale(rgbaBuffer, numPixels, scale);
                    }
                    else
                    {
                        ApplyMatrix(rgbaBuffer, numPixels, m_m44);
                    }
                }
                
                if(!m_offset4IsIdentity)
                {
                    ApplyOffset(rgbaBuffer, numPixels, m_offset4);
                }
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                if(!m_offset4IsIdentity)
                {
                    float offset_inv[] = { -m_offset4[0],
                                           -m_offset4[1],
                                           -m_offset4[2],
                                           -m_offset4[3] };
                    
                    ApplyOffset(rgbaBuffer, numPixels, offset_inv);
                }
                
                if(!m_m44IsIdentity)
                {
                    if(m_m44IsDiagonal)
                    {
                        float scale[4];
                        GetM44Diagonal(scale, m_m44_inv);
                        ApplyScale(rgbaBuffer, numPixels, scale);
                    }
                    else
                    {
                        ApplyMatrix(rgbaBuffer, numPixels, m_m44_inv);
                    }
                }
            }
        } // Op::process
        
        bool MatrixOffsetOp::supportsGpuShader() const
        {
            return true;
        }
        
        void MatrixOffsetOp::writeGpuShader(std::ostream & shader,
                                            const std::string & pixelName,
                                            const GpuShaderDesc & shaderDesc) const
        {
            GpuLanguage lang = shaderDesc.getLanguage();
            
            // TODO: This should not act upon alpha,
            // since we dont apply it on the CPU?
            
            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                if(!m_m44IsIdentity)
                {
                    if(m_m44IsDiagonal)
                    {
                        shader << pixelName << " = ";
                        float scale[4];
                        GetM44Diagonal(scale, m_m44);
                        Write_half4(shader, scale, lang);
                        shader << " * " << pixelName << ";\n";
                    }
                    else
                    {
                        shader << pixelName << " = ";
                        Write_mtx_x_vec(shader,
                                        GpuTextHalf4x4(m_m44, lang), pixelName,
                                        lang);
                        shader << ";\n";
                    }
                }
                
                if(!m_offset4IsIdentity)
                {
                    shader << pixelName << " = ";
                    Write_half4(shader, m_offset4, lang);
                    shader << " + " << pixelName << ";\n";
                }
            }
            else if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                if(!m_offset4IsIdentity)
                {
                    float offset_inv[] = { -m_offset4[0],
                                           -m_offset4[1],
                                           -m_offset4[2],
                                           -m_offset4[3] };
                    
                    shader << pixelName << " = ";
                    Write_half4(shader, offset_inv, lang);
                    shader << " + " << pixelName << ";\n";
                }
                
                if(!m_m44IsIdentity)
                {
                    if(m_m44IsDiagonal)
                    {
                        shader << pixelName << " = ";
                        float scale[4];
                        GetM44Diagonal(scale, m_m44_inv);
                        Write_half4(shader, scale, lang);
                        shader << " * " << pixelName << ";\n";
                    }
                    else
                    {
                        shader << pixelName << " = ";
                        Write_mtx_x_vec(shader,
                                        GpuTextHalf4x4(m_m44_inv, lang), pixelName,
                                        lang);
                        shader << ";\n";
                    }
                }
            }
        }
        
    }  // Anon namespace
    
    
    
    
    
    
    
    
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    
    
    
    
    
    
    void CreateScaleOp(OpRcPtrVec & ops,
                       const float * scale4,
                       TransformDirection direction)
    {
        float offset4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        CreateScaleOffsetOp(ops, scale4, offset4, direction);
    }
    
    void CreateMatrixOp(OpRcPtrVec & ops,
                        const float * m44,
                        TransformDirection direction)
    {
        float offset4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        CreateMatrixOffsetOp(ops, m44, offset4, direction);
    }
    
    void CreateOffsetOp(OpRcPtrVec & ops,
                        const float * offset4,
                        TransformDirection direction)
    {
        float scale4[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        CreateScaleOffsetOp(ops, scale4, offset4, direction);
    }
    
    void CreateScaleOffsetOp(OpRcPtrVec & ops,
                             const float * scale4, const float * offset4,
                             TransformDirection direction)
    {
        float m44[16];
        memset(m44, 0, 16*sizeof(float));
        
        m44[0] = scale4[0];
        m44[5] = scale4[1];
        m44[10] = scale4[2];
        m44[15] = scale4[3];
        
        CreateMatrixOffsetOp(ops,
                             m44, offset4,
                             direction);
    }
    
    void CreateSaturationOp(OpRcPtrVec & ops,
                            float sat,
                            const float * lumaCoef3,
                            TransformDirection direction)
    {
        float matrix[16];
        float offset[4];
        MatrixTransform::Sat(matrix, offset,
                             sat, lumaCoef3);
        
        CreateMatrixOffsetOp(ops, matrix, offset, direction);
    }
    
    void CreateMatrixOffsetOp(OpRcPtrVec & ops,
                              const float * m44, const float * offset4,
                              TransformDirection direction)
    {
        bool mtxIsIdentity = IsM44Identity(m44);
        bool offsetIsIdentity = IsVecEqualToZero(offset4, 4);
        if(mtxIsIdentity && offsetIsIdentity) return;
        
        ops.push_back( MatrixOffsetOpRcPtr(new MatrixOffsetOp(m44,
            offset4, direction)) );
    }
    
    void CreateFitOp(OpRcPtrVec & ops,
                     const float * oldmin4, const float * oldmax4,
                     const float * newmin4, const float * newmax4,
                     TransformDirection direction)
    {
        float matrix[16];
        float offset[4];
        MatrixTransform::Fit(matrix, offset,
                             oldmin4, oldmax4,
                             newmin4, newmax4);
        
        CreateMatrixOffsetOp(ops, matrix, offset, direction);
    }
    
}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OCIO_NAMESPACE_USING

OIIO_ADD_TEST(MatrixOps, Scale)
{
    float error = 1e-8f;

    OpRcPtrVec ops;
    const float scale[] = { 1.1f, 1.3f, 0.3f, 1.0f };
    CreateScaleOp(ops, scale, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    ops[0]->finalize();

    CreateScaleOp(ops, scale, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    ops[1]->finalize();

    const unsigned NB_PIXELS = 3;
    const float src[NB_PIXELS*4] = {  0.1004f,  0.2f,  0.3f,    0.4f,
                                     -0.1008f, -0.2f, 50.01f, 123.4f,
                                      1.0090f,  1.0f,  1.0f,    1.0f };

    const float dst[NB_PIXELS*4] = {  0.11044f,  0.26f,  0.090f,   0.4f,
                                      -0.11088f, -0.26f, 15.003f, 123.4f,
                                       1.10990f,  1.30f,  0.300f,   1.0f };

    float tmp[NB_PIXELS*4];
    memcpy(tmp, &src[0], 4*NB_PIXELS*sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(src[idx], tmp[idx],error);
    }
}

OIIO_ADD_TEST(MatrixOps, Offset)
{
    float error = 1e-5f;

    OpRcPtrVec ops;
    const float offset[] = { 1.1f, -1.3f, 0.3f, 0.0f };
    CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    ops[0]->finalize();

    CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    ops[1]->finalize();

    const unsigned NB_PIXELS = 3;
    const float src[NB_PIXELS*4] = {  0.1004f,  0.2f,  0.3f,    0.4f,
                                     -0.1008f, -0.2f, 50.01f, 123.4f,
                                      1.0090f,  1.0f,  1.0f,    1.0f };

    const float dst[NB_PIXELS*4] = {  1.2004f, -1.100f,  0.600f,   0.4f,
                                      0.9992f, -1.500f, 50.310f, 123.4f,
                                      2.1090f, -0.300f,  1.300f,   1.0f };

    float tmp[NB_PIXELS*4];
    memcpy(tmp, &src[0], 4*NB_PIXELS*sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }
}

OIIO_ADD_TEST(MatrixOps, Matrix)
{
    float error = 1e-5f;

    const float matrix[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                               0.5f, 1.6f, 0.7f, 0.8f,
                               0.2f, 0.1f, 1.1f, 0.2f,
                               0.3f, 0.4f, 0.5f, 1.6f };
                   
    OpRcPtrVec ops;
    CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    ops[0]->finalize();

    CreateMatrixOp(ops, matrix, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    ops[1]->finalize();

    const unsigned NB_PIXELS = 3;
    const float src[NB_PIXELS*4] = {  0.1004f,  0.201f,  0.303f,    0.408f,
                                     -0.1008f, -0.207f, 50.019f,  123.422f,
                                      1.0090f,  1.009f,  1.044f,    1.001f };

    const double dst[NB_PIXELS*4] = {   0.40474f,   0.91030f,    0.45508f,   0.914820f,
                                       64.22222f, 133.369308f,  79.66444f, 222.371658f,
                                        2.02530,    3.65050f,    1.65130f,   2.829900f };

    float tmp[NB_PIXELS*4];
    memcpy(tmp, &src[0], 4*NB_PIXELS*sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }
}

OIIO_ADD_TEST(MatrixOps, Arbitrary)
{
    float error = 1e-5f;

    const float matrix[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                               0.5f, 1.6f, 0.7f, 0.8f,
                               0.2f, 0.1f, 1.1f, 0.2f,
                               0.3f, 0.4f, 0.5f, 1.6f };
                   
    const float offset[4] = { -0.5f, -0.25f, 0.25f, 0.0f };

    OpRcPtrVec ops;
    CreateMatrixOffsetOp(ops, matrix, offset, TRANSFORM_DIR_FORWARD);
    OIIO_CHECK_EQUAL(ops.size(), 1);
    ops[0]->finalize();

    CreateMatrixOffsetOp(ops, matrix, offset, TRANSFORM_DIR_INVERSE);
    OIIO_CHECK_EQUAL(ops.size(), 2);
    ops[1]->finalize();

    const unsigned NB_PIXELS = 3;
    const float src[NB_PIXELS*4] = {  0.1004f,  0.201f,  0.303f,    0.408f,
                                     -0.1008f, -0.207f, 50.019f,  123.422f,
                                      1.0090f,  1.009f,  1.044f,    1.001f };

    const float dst[NB_PIXELS*4] = {  -0.09526f,   0.660300f,  0.70508f,   0.914820f,
                                      63.72222f, 133.119308f, 79.91444f, 222.371658f,
                                       1.52530f,   3.400500f,  1.90130f,   2.829900f };

    float tmp[NB_PIXELS*4];
    memcpy(tmp, &src[0], 4*NB_PIXELS*sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }
}

OIIO_ADD_TEST(MatrixOps, Combining)
{
    float m1[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                     0.5f, 1.6f, 0.7f, 0.8f,
                     0.2f, 0.1f, 1.1f, 0.2f,
                     0.3f, 0.4f, 0.5f, 1.6f };
                   
    float v1[4] = { -0.5f, -0.25f, 0.25f, 0.0f };
    
    float m2[16] = { 1.1f, -0.1f, -0.1f, 0.0f,
                     0.1f, 0.9f, -0.2f, 0.0f,
                     0.05f, 0.0f, 1.1f, 0.0f,
                     0.0f, 0.0f, 0.0f, 1.0f };
    float v2[4] = { -0.2f, -0.1f, -0.1f, -0.2f };
    
    const float source[] = { 0.1f, 0.2f, 0.3f, 0.4f,
                             -0.1f, -0.2f, 50.0f, 123.4f,
                             1.0f, 1.0f, 1.0f, 1.0f };
    float error = 1e-4f;
    
    {
        OpRcPtrVec ops;
        CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD);
        CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_FORWARD);
        OIIO_CHECK_EQUAL(ops.size(), 2);
        ops[0]->finalize();
        ops[1]->finalize();
        
        OpRcPtrVec combined;
        ops[0]->combineWith(combined, ops[1]);
        OIIO_CHECK_EQUAL(combined.size(), 1);
        combined[0]->finalize();
        
        for(int test=0; test<3; ++test)
        {
            float tmp[4];
            memcpy(tmp, &source[4*test], 4*sizeof(float));
            ops[0]->apply(tmp, 1);
            ops[1]->apply(tmp, 1);
            
            float tmp2[4];
            memcpy(tmp2, &source[4*test], 4*sizeof(float));
            combined[0]->apply(tmp2, 1);
            
            for(unsigned int i=0; i<4; ++i)
            {
                OIIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
            }
        }
    }
    
    
    {
        OpRcPtrVec ops;
        CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD);
        CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_INVERSE);
        OIIO_CHECK_EQUAL(ops.size(), 2);
        ops[0]->finalize();
        ops[1]->finalize();
        
        OpRcPtrVec combined;
        ops[0]->combineWith(combined, ops[1]);
        OIIO_CHECK_EQUAL(combined.size(), 1);
        combined[0]->finalize();
        
        
        for(int test=0; test<3; ++test)
        {
            float tmp[4];
            memcpy(tmp, &source[4*test], 4*sizeof(float));
            ops[0]->apply(tmp, 1);
            ops[1]->apply(tmp, 1);
            
            float tmp2[4];
            memcpy(tmp2, &source[4*test], 4*sizeof(float));
            combined[0]->apply(tmp2, 1);
            
            for(unsigned int i=0; i<4; ++i)
            {
                OIIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
            }
        }
    }
    
    {
        OpRcPtrVec ops;
        CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_INVERSE);
        CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_FORWARD);
        OIIO_CHECK_EQUAL(ops.size(), 2);
        ops[0]->finalize();
        ops[1]->finalize();
        
        OpRcPtrVec combined;
        ops[0]->combineWith(combined, ops[1]);
        OIIO_CHECK_EQUAL(combined.size(), 1);
        combined[0]->finalize();
        
        for(int test=0; test<3; ++test)
        {
            float tmp[4];
            memcpy(tmp, &source[4*test], 4*sizeof(float));
            ops[0]->apply(tmp, 1);
            ops[1]->apply(tmp, 1);
            
            float tmp2[4];
            memcpy(tmp2, &source[4*test], 4*sizeof(float));
            combined[0]->apply(tmp2, 1);
            
            for(unsigned int i=0; i<4; ++i)
            {
                OIIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
            }
        }
    }
    
    {
        OpRcPtrVec ops;
        CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_INVERSE);
        CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_INVERSE);
        OIIO_CHECK_EQUAL(ops.size(), 2);
        ops[0]->finalize();
        ops[1]->finalize();
        
        OpRcPtrVec combined;
        ops[0]->combineWith(combined, ops[1]);
        OIIO_CHECK_EQUAL(combined.size(), 1);
        combined[0]->finalize();
        
        for(int test=0; test<3; ++test)
        {
            float tmp[4];
            memcpy(tmp, &source[4*test], 4*sizeof(float));
            ops[0]->apply(tmp, 1);
            ops[1]->apply(tmp, 1);
            
            float tmp2[4];
            memcpy(tmp2, &source[4*test], 4*sizeof(float));
            combined[0]->apply(tmp2, 1);
            
            for(unsigned int i=0; i<4; ++i)
            {
                OIIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
            }
        }
    }
}

#endif
