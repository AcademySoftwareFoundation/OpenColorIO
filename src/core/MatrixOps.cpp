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


#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "MatrixOps.h"
#include "MathUtils.h"

#include "cpu/CPUMatrixOp.h"

#include <cstring>
#include <sstream>

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        class MatrixOffsetOp : public Op
        {
        public:
            MatrixOffsetOp(const float * m44,
                           const float * offset4,
                           TransformDirection direction);

            MatrixOffsetOp(OpData::OpDataMatrixRcPtr & matrix,
                           TransformDirection direction);

            virtual ~MatrixOffsetOp();
            
            virtual OpRcPtr clone() const;
            
            virtual std::string getInfo() const;
            virtual std::string getCacheID() const;
            
            virtual BitDepth getInputBitDepth() const;
            virtual BitDepth getOutputBitDepth() const;
            virtual void setInputBitDepth(BitDepth bitdepth);
            virtual void setOutputBitDepth(BitDepth bitdepth);

            virtual bool isNoOp() const;
            virtual bool isIdentity() const;
            virtual bool isSameType(const OpRcPtr & op) const;
            virtual bool isInverse(const OpRcPtr & op) const;
            virtual bool canCombineWith(const OpRcPtr & op) const;
            virtual void combineWith(OpRcPtrVec & ops, const OpRcPtr & secondOp) const;
            
            virtual bool hasChannelCrosstalk() const;
            virtual void finalize();
            virtual void apply(float* rgbaBuffer, long numPixels) const;
            
            virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;
        
        private:
            // No default constructor
            MatrixOffsetOp();

            // The Matrix Data
            OpData::OpDataMatrixRcPtr m_data;
            // The CPU processor
            CPUOpRcPtr m_cpu;

            TransformDirection m_direction;
            
            // Set in finalize
            std::string m_cacheID;
        };
        
        typedef OCIO_SHARED_PTR<MatrixOffsetOp> MatrixOffsetOpRcPtr;
        
        MatrixOffsetOp::MatrixOffsetOp(const float * m44,
                                       const float * offset4,
                                       TransformDirection direction)
            : Op()
            , m_data(new OpData::Matrix())
            , m_cpu(new CPUNoOp)
            , m_direction(direction)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception(
                    "Cannot create MatrixOffsetOp with unspecified transform direction.");
            }

            m_data->setRGBAValues(m44);
            m_data->setRGBAOffsets(offset4);
        }

        MatrixOffsetOp::MatrixOffsetOp(OpData::OpDataMatrixRcPtr & matrix,
                                       TransformDirection direction)
            : Op()
            , m_data(matrix)
            , m_cpu(new CPUNoOp)
            , m_direction(direction)
        {
            if (m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception(
                    "Cannot create MatrixOffsetOp with unspecified transform direction.");
            }
        }

        OpRcPtr MatrixOffsetOp::clone() const
        {
            OpData::OpDataMatrixRcPtr
                matrix(dynamic_cast<OpData::Matrix*>(m_data->clone(
                    OpData::OpData::DO_DEEP_COPY)));

            return OpRcPtr(new MatrixOffsetOp(matrix, m_direction));
        }
        
        MatrixOffsetOp::~MatrixOffsetOp()
        {
        }
        
        std::string MatrixOffsetOp::getInfo() const
        {
            return "<MatrixOffsetOp>";
        }
        
        std::string MatrixOffsetOp::getCacheID() const
        {
            return m_cacheID;
        }
        
        BitDepth MatrixOffsetOp::getInputBitDepth() const
        {
            return m_data->getInputBitDepth();
        }

        BitDepth MatrixOffsetOp::getOutputBitDepth() const
        {
            return m_data->getOutputBitDepth();
        }

        void MatrixOffsetOp::setInputBitDepth(BitDepth bitdepth)
        {
            m_data->setInputBitDepth(bitdepth);
        }

        void MatrixOffsetOp::setOutputBitDepth(BitDepth bitdepth)
        {
            m_data->setOutputBitDepth(bitdepth);
        }

        // This Op will be a NoOp if and only if the matrix is an identity and
        // the offsets are zero. This holds true no matter what the
        // direction is, so we can compute this ahead of time.
        bool MatrixOffsetOp::isNoOp() const
        {
            return m_data->isNoOp();
        }

        bool MatrixOffsetOp::isIdentity() const
        {
            return m_data->isIdentity();
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
            if (!VecsEqualWithRelError(
                &(m_data->getArray().getValues()[0]), 16,
                &(typedRcPtr->m_data->getArray().getValues()[0]), 16, error))
            {
                return false;
            }
            if (!VecsEqualWithRelError(
                m_data->getOffsets().getValues(), 4,
                typedRcPtr->m_data->getOffsets().getValues(), 4, error))
            {
                return false;
            }

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
            
            OpData::OpDataVec v;
            OpData::Matrix * firstMat = m_data.get();
            unsigned indexNew = 0;
            if (m_direction == TRANSFORM_DIR_INVERSE)
            {
                // can throw
                m_data->inverse(v);
                firstMat = dynamic_cast<OpData::Matrix*>(v[indexNew]);
                indexNew += 1;
            }

            OpData::Matrix * secondMat = typedRcPtr->m_data.get();
            if (typedRcPtr->m_direction == TRANSFORM_DIR_INVERSE)
            {
                // might throw
                typedRcPtr->m_data->inverse(v);
                secondMat = dynamic_cast<OpData::Matrix*>(v[indexNew]);
            }


            OpData::OpDataMatrixRcPtr composedMat(firstMat->compose(*secondMat));

            if(composedMat->isNoOp()) return;

            CreateMatrixOp(ops, composedMat, TRANSFORM_DIR_FORWARD);
        }
        
        bool MatrixOffsetOp::hasChannelCrosstalk() const
        {
            return !m_data->isDiagonal();
        }
        
        void MatrixOffsetOp::finalize()
        {
            if (m_direction == TRANSFORM_DIR_INVERSE)
            {
                OpData::OpDataVec v;
                // can throw
                m_data->inverse(v);
                m_data.reset(dynamic_cast<OpData::Matrix*>(v.remove(0)));
                m_direction = TRANSFORM_DIR_FORWARD;
            }

            // TODO: Currently, only the 32f processing is natively supported
            m_data->setInputBitDepth(BIT_DEPTH_F32);
            m_data->setOutputBitDepth(BIT_DEPTH_F32);

            m_data->validate();

            m_cpu = CPUMatrixOp::GetRenderer(m_data);

            // Create the cacheID
            md5_state_t state;
            md5_byte_t digest[16];
            md5_init(&state);
            md5_append(&state,
                       (const md5_byte_t *)&(m_data->getArray().getValues()[0]),
                       (int)(16*sizeof(float)));
            md5_append(&state,
                       (const md5_byte_t *)m_data->getOffsets().getValues(),
                       (int)(4*sizeof(float)));
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
#ifndef NDEBUG
            // Skip validation in release
            if (m_direction == TRANSFORM_DIR_INVERSE)
            {
                throw Exception("MatrixOp direction should have been"
                    " set to forward by finalize");
            }
#endif
            m_cpu->apply(rgbaBuffer, (unsigned int)numPixels);
        }
        
        void MatrixOffsetOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if (m_direction == TRANSFORM_DIR_INVERSE)
            {
                throw Exception("MatrixOp direction should have been"
                    " set to forward by finalize");
            }

            if(getInputBitDepth()!=BIT_DEPTH_F32 
                || getOutputBitDepth()!=BIT_DEPTH_F32)
            {
                throw Exception(
                    "Only 32F bit depth is supported for the GPU shader");
            }

            // TODO: This should not act upon alpha,
            // since we dont apply it on the CPU?
            
            // TODO: Review implementation to handle bitdepth

            GpuShaderText ss(shaderDesc->getLanguage());
            ss.indent();

            ss.newLine() << "";
            ss.newLine() << "// Add a Matrix processing";
            ss.newLine() << "";

            if(!m_data->isMatrixIdentity())
            {
                if(m_data->isDiagonal())
                {
                    const OpData::ArrayDouble & vals = m_data->getArray();

                    ss.newLine() << shaderDesc->getPixelName() << " = "
                                 << ss.vec4fConst((float)vals[0],
                                                  (float)vals[5],
                                                  (float)vals[10],
                                                  (float)vals[15])
                                 << " * " << shaderDesc->getPixelName()
                                 << ";";
                }
                else
                {
                    const OpData::ArrayDouble::Values & vals = m_data->getArray().getValues();

                    ss.newLine() << shaderDesc->getPixelName() << " = "
                                 << ss.mat4fMul(&vals[0], shaderDesc->getPixelName())
                                 << ";";
                }
            }
            
            if(m_data->hasOffsets())
            {
                const OpData::Matrix::Offsets & offs = m_data->getOffsets();

                ss.newLine() << shaderDesc->getPixelName() << " = "
                             << ss.vec4fConst((float)offs[0], (float)offs[1], (float)offs[2], (float)offs[3])
                             << " + " << shaderDesc->getPixelName()
                             << ";";
            }

            shaderDesc->addToFunctionShaderCode(ss.string().c_str());
        }
        
    }  // Anon namespace
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    void CreateMatrixOp(OpRcPtrVec & ops,
                        const float * from_min3,
                        const float * from_max3,
                        TransformDirection direction)
    {
        float scale4[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float offset4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        bool somethingToDo = false;
        for (unsigned i = 0; i < 3; ++i)
        {
            scale4[i] = 1.f / (from_max3[i] - from_min3[i]);
            offset4[i] = -from_min3[i] * scale4[i];
            somethingToDo |= (scale4[i] != 1.f || offset4[i] != 0.f);
        }

        if (somethingToDo)
        {
            CreateScaleOffsetOp(ops, scale4, offset4, direction);
        }
    }

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
        OpData::Matrix mat;
        mat.setRGBAValues(m44);
        mat.setRGBAOffsets(offset4);
        if (mat.isIdentity()) return;
        
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

    void CreateMatrixOp(OpRcPtrVec & ops,
                        OpData::OpDataMatrixRcPtr & matrix,
                        TransformDirection direction)
    {
        if (matrix->isIdentity())
        {
            return;
        }
        ops.push_back(MatrixOffsetOpRcPtr(new MatrixOffsetOp(matrix, direction)));
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
#include "UnitTest.h"
#include "NoOps.h"
#include "LogOps.h"
#include "FileFormatCTF.h"

OCIO_NAMESPACE_USING

OIIO_ADD_TEST(MatrixOps, Scale)
{
    const float error = 1e-8f;

    OpRcPtrVec ops;
    const float scale[] = { 1.1f, 1.3f, 0.3f, 1.0f };
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);

    std::string cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), true);

    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    cacheID = ops[0]->getCacheID();
    OIIO_CHECK_EQUAL(cacheID.empty(), false);

    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);
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
        OIIO_CHECK_ASSERT(OCIO::equalWithSafeRelError((float)dst[idx],
                                                      (float)tmp[idx],
                                                      error, 1.0f));
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_ASSERT(OCIO::equalWithSafeRelError((float)src[idx],
                                                      (float)tmp[idx],
                                                      error, 1.0f));
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
        // Do not use equalWithSafeRelError so that values would get displayed
        // in case of an error.
        float error_rel = tmp[idx];
        if (error_rel < 0.f) error_rel = -error_rel;
        if (error_rel < 1.f) error_rel = error;
        else error_rel *= error;
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error_rel);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        // Do not use equalWithSafeRelError so that values would get displayed
        // in case of an error.
        float error_rel = tmp[idx];
        if (error_rel < 0.f) error_rel = -error_rel;
        if (error_rel < 1.f) error_rel = error;
        else error_rel *= error;
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error_rel);
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
    
    const float m2[16] = { 1.1f, -0.1f, -0.1f,  0.0f,
                           0.1f,  0.9f, -0.2f,  0.0f,
                           0.05f, 0.0f,  1.1f,  0.0f,
                           0.0f,  0.0f,  0.0f,  1.0f };
    const float v2[4] = { -0.2f, -0.1f, -0.1f, -0.2f };
    
    const float source[] = { 0.1f,  0.2f,  0.3f,   0.4f,
                            -0.1f, -0.2f, 50.0f, 123.4f,
                             1.0f,  1.0f,  1.0f,   1.0f };
    
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
        OCIO::Exception, "Singular Matrix can't be inverted");
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
        OCIO::Exception, "Singular Matrix can't be inverted");

    // combining inverse that can't be inverted with forward
    ops.clear();
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, ops[1]),
        OCIO::Exception, "Singular Matrix can't be inverted");

    // combining inverse with inverse that can't be inverted 
    ops.clear();
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, ops[1]),
        OCIO::Exception, "Singular Matrix can't be inverted");
    
    // combining inverse that can't be inverted with inverse
    ops.clear();
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, ops[1]),
        OCIO::Exception, "Singular Matrix can't be inverted");

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

OIIO_ADD_TEST(MatrixOps, Matrix_double)
{
    // This file contains some double precision values
    const std::string ctfFile("matrix_double.ctf");

    OCIO::CTF::Reader::TransformPtr transform;
    OIIO_CHECK_NO_THROW(transform = OCIO::LoadCTFTestFile(ctfFile));

    const OCIO::OpData::OpDataVec & opList = transform->getOps();
    OIIO_CHECK_EQUAL(opList.size(), 1);
    OIIO_CHECK_EQUAL(opList[0]->getOpType(), OCIO::OpData::OpData::MatrixType);

    const OCIO::OpData::Matrix * mat = static_cast<const OCIO::OpData::Matrix*>(opList[0]);
    OIIO_CHECK_ASSERT(mat);

    // Check the values that are double precision
    OIIO_CHECK_EQUAL(mat->getOffsets()[0], 1.23456789012345);
    OIIO_CHECK_EQUAL(mat->getOffsets()[1], 2.34567890123456);
    OIIO_CHECK_EQUAL(mat->getArray()[0],   3.45678901234567);
}



#endif
