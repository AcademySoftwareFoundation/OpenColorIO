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
#include "ops/Matrix/MatrixOps.h"
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
            
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;
        
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
            cacheIDStream << BitDepthToString(getInputBitDepth()) << " ";
            cacheIDStream << BitDepthToString(getOutputBitDepth()) << " ";
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
        
        void MatrixOffsetOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if(getInputBitDepth()!=BIT_DEPTH_F32 
                || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception("Only 32F bit depth is supported for the GPU shader");
            }

            // TODO: This should not act upon alpha,
            // since we dont apply it on the CPU?
            
            GpuShaderText ss(shaderDesc->getLanguage());
            ss.indent();

            if(m_direction == TRANSFORM_DIR_FORWARD)
            {
                if(!m_m44IsIdentity)
                {
                    if(m_m44IsDiagonal)
                    {
                        float scale[4];
                        GetM44Diagonal(scale, m_m44);

                        ss.newLine() << shaderDesc->getPixelName() << " = "
                                     << ss.vec4fConst(scale[0], scale[1], scale[2], scale[3])
                                     << " * " << shaderDesc->getPixelName()
                                     << ";";
                    }
                    else
                    {
                        ss.newLine() << shaderDesc->getPixelName() << " = "
                                     << ss.mat4fMul(m_m44, shaderDesc->getPixelName())
                                     << ";";
                    }
                }
                
                if(!m_offset4IsIdentity)
                {
                    ss.newLine() << shaderDesc->getPixelName() << " = "
                                 << ss.vec4fConst(m_offset4[0], m_offset4[1], m_offset4[2], m_offset4[3])
                                 << " + " << shaderDesc->getPixelName()
                                 << ";";
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
                    
                    ss.newLine() << shaderDesc->getPixelName() << " = "
                                 << ss.vec4fConst(offset_inv[0], offset_inv[1], offset_inv[2], offset_inv[3])
                                 << " + " << shaderDesc->getPixelName()
                                 << ";";
                }
                
                if(!m_m44IsIdentity)
                {
                    if(m_m44IsDiagonal)
                    {
                        float scale[4];
                        GetM44Diagonal(scale, m_m44_inv);

                        ss.newLine() << shaderDesc->getPixelName() << " = "
                                     << ss.vec4fConst(scale[0], scale[1], scale[2], scale[3])
                                     << " * " << shaderDesc->getPixelName()
                                     << ";";
                    }
                    else
                    {
                        ss.newLine() << shaderDesc->getPixelName() << " = "
                                     << ss.mat4fMul(m_m44_inv, shaderDesc->getPixelName())
                                     << ";";
                    }
                }
            }

            shaderDesc->addToFunctionShaderCode(ss.string().c_str());
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

    void CreateIdentityMatrixOp(OpRcPtrVec & ops,
                                TransformDirection direction)
    {
        float matrix[16];
        memset(matrix, 0, 16 * sizeof(float));
        matrix[0] = 1.0f;
        matrix[5] = 1.0f;
        matrix[10] = 1.0f;
        matrix[15] = 1.0f;
        float offset[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        ops.push_back(MatrixOffsetOpRcPtr(new MatrixOffsetOp(matrix,
            offset, direction)));
    }

}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"
#include "ops/NoOp/NoOps.h"
#include "ops/Log/LogOps.h"

OCIO_NAMESPACE_USING

OIIO_ADD_TEST(MatrixOps, Scale)
{
    const float error = 1e-8f;

    OpRcPtrVec ops;
    const float scale[] = { 1.1f, 1.3f, 0.3f, 1.0f };
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);

    std::string cacheID = ops[0]->getCacheID();
    OIIO_REQUIRE_ASSERT(cacheID.empty());

    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    cacheID = ops[0]->getCacheID();
    OIIO_REQUIRE_ASSERT(!cacheID.empty());

    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

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
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }
}

OIIO_ADD_TEST(MatrixOps, Offset)
{
    const float error = 1e-5f;

    OpRcPtrVec ops;
    const float offset[] = { 1.1f, -1.3f, 0.3f, 0.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);

    std::string cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), true);

    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);

    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

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
    const float error = 1e-5f;

    const float matrix[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                               0.5f, 1.6f, 0.7f, 0.8f,
                               0.2f, 0.1f, 1.1f, 0.2f,
                               0.3f, 0.4f, 0.5f, 1.6f };
                   
    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);

    std::string cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), true);

    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);

    OIIO_CHECK_NO_THROW(CreateMatrixOp(ops, matrix, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    cacheID = ops[1]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), true);

    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    cacheID = ops[1]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);

    const unsigned NB_PIXELS = 3;
    const float src[NB_PIXELS*4] = {  0.1004f,  0.201f,  0.303f,    0.408f,
                                     -0.1008f, -0.207f, 50.019f,  123.422f,
                                      1.0090f,  1.009f,  1.044f,    1.001f };

    const double dst[NB_PIXELS*4] = {
         0.40474f,    0.91030f,  0.45508f,   0.914820f,
        64.22222f, 133.369308f, 79.66444f, 222.371658f,
         2.02530f,    3.65050f,  1.65130f,   2.829900f };

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
    const float error = 1e-5f;

    const float matrix[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                               0.5f, 1.6f, 0.7f, 0.8f,
                               0.2f, 0.1f, 1.1f, 0.2f,
                               0.3f, 0.4f, 0.5f, 1.6f };
                   
    const float offset[4] = { -0.5f, -0.25f, 0.25f, 0.0f };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(
        CreateMatrixOffsetOp(ops, matrix, offset,TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);

    std::string cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), true);

    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);

    OIIO_CHECK_NO_THROW(
        CreateMatrixOffsetOp(ops, matrix, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    cacheID = ops[1]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), true);

    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    cacheID = ops[1]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);

    const unsigned NB_PIXELS = 3;
    const float src[NB_PIXELS*4] = {
         0.1004f,  0.201f,  0.303f,    0.408f,
        -0.1008f, -0.207f, 50.019f,  123.422f,
         1.0090f,  1.009f,  1.044f,    1.001f };

    const float dst[NB_PIXELS*4] = {
        -0.09526f,   0.660300f,  0.70508f,   0.914820f,
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

    std::string opInfo0 = ops[0]->getInfo();
    OIIO_CHECK_EQUAL(opInfo0.empty(), false);

    std::string opInfo1 = ops[1]->getInfo();
    OIIO_CHECK_EQUAL(opInfo0, opInfo1);

    OpRcPtr clonedOp = ops[1]->clone();
    
    std::string cacheIDCloned = clonedOp->getCacheID();
    
    OIIO_CHECK_EQUAL(cacheIDCloned.empty(), true);

    OIIO_CHECK_NO_THROW(clonedOp->finalize());

    cacheIDCloned = clonedOp->getCacheID();

    OIIO_CHECK_EQUAL(cacheIDCloned.empty(), false);
    OIIO_CHECK_EQUAL(cacheIDCloned, cacheID);
}

OIIO_ADD_TEST(MatrixOps, CreateFitOp)
{
    const float error = 1e-7f;

    const float oldmin4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float oldmax4[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float newmin4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float newmax4[4] = { 1.0f, 4.0f, 9.0f, 16.0f };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateFitOp(ops,
        oldmin4, oldmax4,
        newmin4, newmax4, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);

    std::string cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), true);

    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);

    OIIO_CHECK_NO_THROW(CreateFitOp(ops,
        oldmin4, oldmax4,
        newmin4, newmax4, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    cacheID = ops[1]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), true);

    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    cacheID = ops[1]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);

    const unsigned NB_PIXELS = 3;
    const float src[NB_PIXELS * 4] = { 0.1004f, 0.201f, 0.303f,  0.408f,
                                        -0.10f, -21.0f,  50.0f,    1.0f,
                                         42.0f,   1.0f, -1.11f, -0.001f };

    const double dst[NB_PIXELS * 4] = { 0.1004f, 0.402f, 0.909f,  1.632f,
                                         -0.10f, -42.0f, 150.0f,    4.0f,
                                          42.0f,   2.0f, -3.33f, -0.004f };

    float tmp[NB_PIXELS * 4];
    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for (unsigned idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for (unsigned idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }

}

OIIO_ADD_TEST(MatrixOps, CreateSaturationOp)
{
    const float error = 1e-5f;
    const float sat = 0.9f;
    const float lumaCoef3[3] = { 1.0f, 0.5f, 0.1f };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(
        CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD));

    OIIO_CHECK_EQUAL(ops.size(), 1);

    std::string cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), true);

    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);

    OIIO_CHECK_NO_THROW(
        CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    cacheID = ops[1]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), true);

    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    cacheID = ops[1]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);

    const unsigned NB_PIXELS = 3;
    const float src[NB_PIXELS * 4] = { 0.1004f, 0.201f, 0.303f,  0.408f,
                                        -0.10f, -21.0f,  50.0f,    1.0f,
                                         42.0f,   1.0f, -1.11f, -0.001f };

    const double dst[NB_PIXELS * 4] = {
        0.11348f, 0.20402f, 0.29582f,  0.408f,
          -0.65f,  -19.46f,   44.44f,    1.0f,
        42.0389f,  5.1389f,  3.2399f, -0.001f };

    float tmp[NB_PIXELS * 4];
    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for (unsigned idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for (unsigned idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }
}

OIIO_ADD_TEST(MatrixOps, Combining)
{
    const float error = 1e-4f;
    const float m1[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                     0.5f, 1.6f, 0.7f, 0.8f,
                     0.2f, 0.1f, 1.1f, 0.2f,
                     0.3f, 0.4f, 0.5f, 1.6f };
                   
    const float v1[4] = { -0.5f, -0.25f, 0.25f, 0.0f };
    
    const float m2[16] = { 1.1f, -0.1f, -0.1f, 0.0f,
                     0.1f, 0.9f, -0.2f, 0.0f,
                     0.05f, 0.0f, 1.1f, 0.0f,
                     0.0f, 0.0f, 0.0f, 1.0f };
    const float v2[4] = { -0.2f, -0.1f, -0.1f, -0.2f };
    
    const float source[] = { 0.1f, 0.2f, 0.3f, 0.4f,
                             -0.1f, -0.2f, 50.0f, 123.4f,
                             1.0f, 1.0f, 1.0f, 1.0f };
    
    {
        OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_EQUAL(ops.size(), 2);
        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[1]->finalize());
        
        OpRcPtrVec combined;
        OIIO_CHECK_NO_THROW(ops[0]->combineWith(combined, ops[1]));
        OIIO_CHECK_EQUAL(combined.size(), 1);
        OIIO_CHECK_NO_THROW(combined[0]->finalize());

        const std::string cacheIDCombined = combined[0]->getCacheID();
        OIIO_CHECK_EQUAL(cacheIDCombined.empty(), false);

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

        ops.clear();
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_EQUAL(ops.size(), 2);
        OpRcPtr op0 = ops[0];
        OpRcPtr op1 = ops[1];

        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));
        OIIO_CHECK_EQUAL(ops.size(), 1);

        const std::string cacheIDOptimized = ops[0]->getCacheID();
        OIIO_CHECK_EQUAL(cacheIDOptimized.empty(), false);

        OIIO_CHECK_EQUAL(cacheIDCombined, cacheIDOptimized);

        op0->finalize();
        op1->finalize();

        for (int test = 0; test<3; ++test)
        {
            float tmp[4];
            memcpy(tmp, &source[4 * test], 4 * sizeof(float));
            op0->apply(tmp, 1);
            op1->apply(tmp, 1);

            float tmp2[4];
            memcpy(tmp2, &source[4 * test], 4 * sizeof(float));
            ops[0]->apply(tmp2, 1);

            for (unsigned int i = 0; i<4; ++i)
            {
                OIIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
            }
        }
    }
    
    
    {
        OpRcPtrVec ops;
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_INVERSE));
        OIIO_CHECK_EQUAL(ops.size(), 2);
        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[1]->finalize());
        
        OpRcPtrVec combined;
        OIIO_CHECK_NO_THROW(ops[0]->combineWith(combined, ops[1]));
        OIIO_CHECK_EQUAL(combined.size(), 1);
        OIIO_CHECK_NO_THROW(combined[0]->finalize());
        
        
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
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_INVERSE));
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_EQUAL(ops.size(), 2);
        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[1]->finalize());
        
        OpRcPtrVec combined;
        OIIO_CHECK_NO_THROW(ops[0]->combineWith(combined, ops[1]));
        OIIO_CHECK_EQUAL(combined.size(), 1);
        OIIO_CHECK_NO_THROW(combined[0]->finalize());
        
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
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_INVERSE));
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_INVERSE));
        OIIO_CHECK_EQUAL(ops.size(), 2);
        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[1]->finalize());
        
        OpRcPtrVec combined;
        OIIO_CHECK_NO_THROW(ops[0]->combineWith(combined, ops[1]));
        OIIO_CHECK_EQUAL(combined.size(), 1);
        OIIO_CHECK_NO_THROW(combined[0]->finalize());
        
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

OIIO_ADD_TEST(MatrixOps, ThrowCreate)
{
    OpRcPtrVec ops;
    const float scale[] = { 1.1f, 1.3f, 0.3f, 1.0f };
    OIIO_CHECK_THROW_WHAT(
        CreateScaleOp(ops, scale, TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");

    const float offset[] = { 1.1f, -1.3f, 0.3f, 0.0f };
    OIIO_CHECK_THROW_WHAT(
        CreateOffsetOp(ops, offset, TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");

    const float matrix[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
        0.5f, 1.6f, 0.7f, 0.8f,
        0.2f, 0.1f, 1.1f, 0.2f,
        0.3f, 0.4f, 0.5f, 1.6f };

    OIIO_CHECK_THROW_WHAT(
        CreateMatrixOp(ops, matrix, TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");

    // Firt can't be created when old min and max are equal
    const float oldmin4[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    const float oldmax4[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float newmin4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float newmax4[4] = { 1.0f, 4.0f, 9.0f, 16.0f };

    OIIO_CHECK_THROW_WHAT(CreateFitOp(ops,
        oldmin4, oldmax4,
        newmin4, newmax4, TRANSFORM_DIR_FORWARD),
        OCIO::Exception,
        "Cannot create Fit operator. Max value equals min value");
}

OIIO_ADD_TEST(MatrixOps, ThrowFinalize)
{
    // Matrix that can't be inverted can't be used in inverse direction
    OpRcPtrVec ops;
    const float scale[] = { 0.0f, 1.3f, 0.3f, 1.0f };
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_INVERSE));

    OIIO_CHECK_THROW_WHAT(ops[0]->finalize(),
        OCIO::Exception, "Matrix inverse does not exist");
}

OIIO_ADD_TEST(MatrixOps, ThrowCombine)
{
    OpRcPtrVec ops;
    
    // combining with different op
    const float offset[] = { 1.1f, -1.3f, 0.3f, 0.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));
    
    OpRcPtrVec combinedOps;
    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, ops[1]),
        OCIO::Exception, "can only be combined with other MatrixOffsetOps");

    // combining forward with inverse that can't be inverted
    ops.clear();
    const float scaleNoInv[] = { 1.1f, 0.0f, 0.3f, 0.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, ops[1]),
        OCIO::Exception, "Cannot invert second MatrixOffsetOp op");

    // combining inverse that can't be inverted with forward
    ops.clear();
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, ops[1]),
        OCIO::Exception, "Cannot invert primary MatrixOffsetOp op");

    // combining inverse with inverse that can't be inverted 
    ops.clear();
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, ops[1]),
        OCIO::Exception, "Cannot invert second MatrixOffsetOp op");
    
    // combining inverse that can't be inverted with inverse
    ops.clear();
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, ops[1]),
        OCIO::Exception, "Cannot invert primary MatrixOffsetOp op");

}

OIIO_ADD_TEST(MatrixOps, NoOp)
{
    OpRcPtrVec ops;
    const float offset[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));

    // no ops are not created
    OIIO_CHECK_EQUAL(ops.size(), 0);
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 0);

    const float scale[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 0);
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 0);

    const float matrix[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };

    OIIO_CHECK_NO_THROW(CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 0);
    OIIO_CHECK_NO_THROW(CreateMatrixOp(ops, matrix, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 0);
    OIIO_CHECK_NO_THROW(
        CreateMatrixOffsetOp(ops, matrix, offset, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 0);
    OIIO_CHECK_NO_THROW(
        CreateMatrixOffsetOp(ops, matrix, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 0);

    const float oldmin4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float oldmax4[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    OIIO_CHECK_NO_THROW(CreateFitOp(ops,
        oldmin4, oldmax4,
        oldmin4, oldmax4, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 0);

    OIIO_CHECK_NO_THROW(CreateFitOp(ops,
        oldmin4, oldmax4,
        oldmin4, oldmax4, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 0);

    const float sat = 1.0f;
    const float lumaCoef3[3] = { 1.0f, 1.0f, 1.0f };

    OIIO_CHECK_NO_THROW(
        CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 0);

    OIIO_CHECK_NO_THROW(
        CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 0);

    // except if CreateIdentityMatrixOp is used
    OIIO_CHECK_NO_THROW(CreateIdentityMatrixOp(ops, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->isNoOp(), true);
    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    OIIO_CHECK_EQUAL(ops[0]->isNoOp(), true);

    OIIO_CHECK_NO_THROW(CreateIdentityMatrixOp(ops, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_EQUAL(ops[1]->isNoOp(), true);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());
    OIIO_CHECK_EQUAL(ops[1]->isNoOp(), true);

}

OIIO_ADD_TEST(MatrixOps, isSameType)
{
    const float sat = 0.9f;
    const float lumaCoef3[3] = { 1.0f, 0.5f, 0.1f };
    const float scale[] = { 1.1f, 1.3f, 0.3f, 1.0f };
    const float k[3] = { 0.18f, 0.5f, 0.3f };
    const float m[3] = { 2.0f, 4.0f, 8.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base[3] = { 10.0f, 5.0f, 2.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };

    // Create saturation, scale and log
    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(
        CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(
        CreateLogOp(ops, k, m, b, base, kb, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 3);
    // saturation and scale are MatrixOffset operators, log is not.
    OIIO_CHECK_EQUAL(ops[0]->isSameType(ops[1]), true);
    OIIO_CHECK_EQUAL(ops[1]->isSameType(ops[0]), true);
    OIIO_CHECK_EQUAL(ops[0]->isSameType(ops[2]), false);
    OIIO_CHECK_EQUAL(ops[2]->isSameType(ops[0]), false);
    OIIO_CHECK_EQUAL(ops[1]->isSameType(ops[2]), false);
    OIIO_CHECK_EQUAL(ops[2]->isSameType(ops[1]), false);
}

OIIO_ADD_TEST(MatrixOps, isInverse)
{
    OpRcPtrVec ops;
    const float offset[] = { 1.1f, -1.3f, 0.3f, 0.0f };
    const float offsetInv[] = { -1.1f, 1.3f, -0.3f, 0.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offsetInv, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 3);

    const float k[3] = { 0.18f, 0.5f, 0.3f };
    const float m[3] = { 2.0f, 4.0f, 8.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base[3] = { 10.0f, 5.0f, 2.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };
    OIIO_CHECK_NO_THROW(
        CreateLogOp(ops, k, m, b, base, kb, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 4);

    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[1]), true);
    OIIO_CHECK_EQUAL(ops[1]->isInverse(ops[0]), true);
    
    // isInverse does not try to invert the transform
    OIIO_CHECK_EQUAL(ops[0]->isInverse(ops[2]), false);
    OIIO_CHECK_EQUAL(ops[2]->isInverse(ops[0]), false);
    
    OIIO_CHECK_EQUAL(ops[2]->isInverse(ops[3]), false);
    OIIO_CHECK_EQUAL(ops[3]->isInverse(ops[2]), false);
}

OIIO_ADD_TEST(MatrixOps, hasChannelCrosstalk)
{
    const float scale[] = { 1.1f, 1.3f, 0.3f, 1.0f };
    const float sat = 0.9f;
    const float lumaCoef3[3] = { 1.0f, 0.5f, 0.1f };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    OIIO_CHECK_NO_THROW(
        CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    OIIO_CHECK_EQUAL(ops[0]->hasChannelCrosstalk(), false);
    OIIO_CHECK_EQUAL(ops[1]->hasChannelCrosstalk(), true);
}

#endif
