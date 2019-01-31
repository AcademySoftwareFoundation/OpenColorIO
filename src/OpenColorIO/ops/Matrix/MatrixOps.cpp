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


#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/Matrix/MatrixOpCPU.h"
#include "ops/Matrix/MatrixOps.h"

OCIO_NAMESPACE_ENTER
{

    namespace
    {

        // The class represents the Matrix Op
        //
        // The class specifies a matrix transformation to be applied to
        // the input values. The input and output of a matrix are always
        // 4-component values.
        // An offset vector is also applied to the result.
        // The output values are calculated using the row-major order convention:
        // 
        // Rout = a[0][0]*Rin + a[0][1]*Gin + a[0][2]*Bin + a[0][3]*Ain + o[0];
        // Gout = a[1][0]*Rin + a[1][1]*Gin + a[1][2]*Bin + a[1][3]*Ain + o[1];
        // Bout = a[2][0]*Rin + a[2][1]*Gin + a[2][2]*Bin + a[2][3]*Ain + o[2];
        // Aout = a[3][0]*Rin + a[3][1]*Gin + a[3][2]*Bin + a[3][3]*Ain + o[3];

        class MatrixOffsetOp : public Op
        {
        public:
            MatrixOffsetOp(const float * m44,
                           const float * offset4,
                           TransformDirection direction);

            MatrixOffsetOp(MatrixOpDataRcPtr & matrix,
                           TransformDirection direction);

            virtual ~MatrixOffsetOp();
            
            OpRcPtr clone() const override;
            
            std::string getInfo() const override;
            
            bool isSameType(ConstOpRcPtr & op) const override;
            bool isInverse(ConstOpRcPtr & op) const override;
            bool canCombineWith(ConstOpRcPtr & op) const override;
            void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;
            
            void finalize() override;
            void apply(float* rgbaBuffer, long numPixels) const override;
            
            void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;
        
        protected:
            ConstMatrixOpDataRcPtr matrixData() const { return DynamicPtrCast<const MatrixOpData>(data()); }
            MatrixOpDataRcPtr matrixData() { return DynamicPtrCast<MatrixOpData>(data()); }

        private:
            MatrixOffsetOp() = delete;

            TransformDirection m_direction;
            OpCPURcPtr m_cpu;
        };
        
        
        typedef OCIO_SHARED_PTR<MatrixOffsetOp> MatrixOffsetOpRcPtr;
        typedef OCIO_SHARED_PTR<const MatrixOffsetOp> ConstMatrixOffsetOpRcPtr;

        
        MatrixOffsetOp::MatrixOffsetOp(const float * m44,
                                       const float * offset4,
                                       TransformDirection direction)
            : Op()
            , m_direction(direction)
        {
            if(m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception("Cannot apply MatrixOffsetOp op, unspecified transform direction.");
            }

            MatrixOpDataRcPtr mat = std::make_shared<MatrixOpData>();
            mat->setRGBA(m44);
            mat->setRGBAOffsets(offset4);
            data() = mat;
        }
        
        MatrixOffsetOp::MatrixOffsetOp(MatrixOpDataRcPtr & matrix,
                                       TransformDirection direction)
            : Op()
            , m_cpu(std::make_shared<NoOpCPU>())
            , m_direction(direction)
        {
            if (m_direction == TRANSFORM_DIR_UNKNOWN)
            {
                throw Exception(
                    "Cannot create MatrixOffsetOp with unspecified transform direction.");
            }
            data() = matrix;
        }

        OpRcPtr MatrixOffsetOp::clone() const
        {
            MatrixOpDataRcPtr clonedData = matrixData()->clone();
            return std::make_shared<MatrixOffsetOp>(clonedData, m_direction);
        }
        
        MatrixOffsetOp::~MatrixOffsetOp()
        { }
        
        std::string MatrixOffsetOp::getInfo() const
        {
            return "<MatrixOffsetOp>";
        }
        
        bool MatrixOffsetOp::isSameType(ConstOpRcPtr & op) const
        {
            ConstMatrixOffsetOpRcPtr typedRcPtr = DynamicPtrCast<const MatrixOffsetOp>(op);
            if(!typedRcPtr) return false;
            return true;
        }
        
        bool MatrixOffsetOp::isInverse(ConstOpRcPtr & op) const
        {
            if (canCombineWith(op))
            {
                OpRcPtrVec ops;
                combineWith(ops, op);

                // If combined matrix isNoOp nothing is added.
                if (ops.size() == 0)
                {
                    return true;
                }
            }
            return false;
        }
        
        bool MatrixOffsetOp::canCombineWith(ConstOpRcPtr & op) const
        {
            return isSameType(op);
        }
        
        void MatrixOffsetOp::combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const
        {
            ConstMatrixOffsetOpRcPtr typedRcPtr = DynamicPtrCast<const MatrixOffsetOp>(secondOp);
            if(!typedRcPtr)
            {
                std::ostringstream os;
                os << "MatrixOffsetOp can only be combined with other ";
                os << "MatrixOffsetOps.  secondOp:" << secondOp->getInfo();
                throw Exception(os.str().c_str());
            }
            
            ConstMatrixOpDataRcPtr firstMat = matrixData();
            unsigned long indexNew = 0;
            if (m_direction == TRANSFORM_DIR_INVERSE)
            {
                // Could throw.
                firstMat = firstMat->inverse();
            }

            ConstMatrixOpDataRcPtr secondMat = typedRcPtr->matrixData();
            if (typedRcPtr->m_direction == TRANSFORM_DIR_INVERSE)
            {
                // Could throw.
                secondMat = secondMat->inverse();
            }

            MatrixOpDataRcPtr composedMat = firstMat->compose(secondMat);

            if (composedMat->isNoOp()) return;

            CreateMatrixOp(ops, composedMat, TRANSFORM_DIR_FORWARD);
        }
        
        void MatrixOffsetOp::finalize()
        {
            if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                MatrixOpDataRcPtr thisMat = matrixData();
                thisMat = thisMat->inverse();
                data() = thisMat;
                m_direction = TRANSFORM_DIR_FORWARD;
            }

            // TODO: Only the 32f processing is natively supported
            matrixData()->setInputBitDepth(BIT_DEPTH_F32);
            matrixData()->setOutputBitDepth(BIT_DEPTH_F32);

            matrixData()->validate();
            matrixData()->finalize();

            m_cpu = GetMatrixRenderer(matrixData());

            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<MatrixOffsetOp ";
            cacheIDStream << matrixData()->getCacheID() << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";
            
            m_cacheID = cacheIDStream.str();
        }
        
        void MatrixOffsetOp::apply(float* rgbaBuffer, long numPixels) const
        {
            m_cpu->apply(rgbaBuffer, numPixels);
        }

        void MatrixOffsetOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
        {
            if (m_direction == TRANSFORM_DIR_INVERSE)
            {
                throw Exception("MatrixOp direction should have been"
                    " set to forward by finalize");
            }

            if (getInputBitDepth() != BIT_DEPTH_F32
                || getOutputBitDepth() != BIT_DEPTH_F32)
            {
                throw Exception(
                    "Only 32F bit depth is supported for the GPU shader");
            }

            // TODO: Review implementation to handle bitdepth

            GpuShaderText ss(shaderDesc->getLanguage());
            ss.indent();

            ss.newLine() << "";
            ss.newLine() << "// Add a Matrix processing";
            ss.newLine() << "";

            ConstMatrixOpDataRcPtr matData = matrixData();
            ArrayDouble::Values values = matData->getArray().getValues();
            MatrixOpData::Offsets offs(matData->getOffsets());

            if (!matData->isUnityDiagonal())
            {
                if (matData->isDiagonal())
                {
                    ss.newLine() << shaderDesc->getPixelName() << " = "
                                 << ss.vec4fConst((float)values[0],
                                                  (float)values[5],
                                                  (float)values[10],
                                                  (float)values[15])
                                 << " * " << shaderDesc->getPixelName() << ";";
                }
                else
                {
                    ss.newLine() << shaderDesc->getPixelName() << " = "
                                 << ss.mat4fMul(&values[0], shaderDesc->getPixelName())
                                 << ";";
                }
            }

            if (matData->hasOffsets())
            {
                ss.newLine() << shaderDesc->getPixelName() << " = "
                             << ss.vec4fConst((float)offs[0], (float)offs[1], (float)offs[2], (float)offs[3])
                             << " + " << shaderDesc->getPixelName() << ";";
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
        
        ops.push_back(std::make_shared<MatrixOffsetOp>(m44,
                                                       offset4,
                                                       direction));
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

        ops.push_back(std::make_shared<MatrixOffsetOp>(matrix,
                                                       offset,
                                                       direction));
    }

    void CreateMinMaxOp(OpRcPtrVec & ops,
                        const float * from_min3,
                        const float * from_max3,
                        TransformDirection direction)
    {
        float scale4[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float offset4[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        bool somethingToDo = false;
        for (int i = 0; i < 3; ++i)
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

    void CreateMatrixOp(OpRcPtrVec & ops,
                        MatrixOpDataRcPtr & matrix,
                        TransformDirection direction)
    {
        if (matrix->isNoOp())
        {
            return;
        }
        ops.push_back(std::make_shared<MatrixOffsetOp>(matrix, direction));
    }
}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"
#include "ops/Log/LogOps.h"
#include "ops/NoOp/NoOps.h"

OCIO_NAMESPACE_USING

// TODO: syncolor also tests various bit-depths and pixel formats.
// synColorCheckApply_test.cpp - CheckMatrixRemovingGreen
// synColorCheckApply_test.cpp - CheckMatrixWithInt16Scaling
// synColorCheckApply_test.cpp - CheckMatrixWithFloatScaling
// synColorCheckApply_test.cpp - CheckMatrixWithHalfScaling
// synColorCheckApply_test.cpp - CheckIdentityWith16iRGBAImage
// synColorCheckApply_test.cpp - CheckIdentityWith16iBGRAImage
// synColorCheckApply_test.cpp - CheckMatrixWith16iRGBAImage


OIIO_ADD_TEST(MatrixOffsetOp, scale)
{
    const float error = 1e-8f;

    OpRcPtrVec ops;
    const float scale[] = { 1.1f, 1.3f, 0.3f, -1.0f };
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");

    std::string cacheID = ops[0]->getCacheID();
    OIIO_REQUIRE_ASSERT(cacheID.empty());
    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    cacheID = ops[0]->getCacheID();
    OIIO_REQUIRE_ASSERT(!cacheID.empty());

    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    const unsigned long NB_PIXELS = 3;
    const float src[NB_PIXELS*4] = {  0.1004f,  0.2f, 0.3f,   0.4f,
                                     -0.1008f, -0.2f, 5.001f, 0.1234f,
                                      1.0090f,  1.0f, 1.0f,   1.0f };

    const float dst[NB_PIXELS*4] = {  0.11044f,  0.26f, 0.090f,  -0.4f,
                                     -0.11088f, -0.26f, 1.5003f, -0.1234f,
                                      1.10990f,  1.30f, 0.300f,  -1.0f };

    float tmp[NB_PIXELS*4];
    memcpy(tmp, &src[0], 4*NB_PIXELS*sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }
}

OIIO_ADD_TEST(MatrixOffsetOp, offset)
{
    const float error = 1e-6f;

    OpRcPtrVec ops;
    const float offset[] = { 1.1f, -1.3f, 0.3f, -1.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    const unsigned long NB_PIXELS = 3;
    const float src[NB_PIXELS*4] = {  0.1004f,  0.2f, 0.3f,  0.4f,
                                     -0.1008f, -0.2f, 5.01f, 0.1234f,
                                      1.0090f,  1.0f, 1.0f,  1.0f };

    const float dst[NB_PIXELS*4] = {  1.2004f, -1.1f, 0.60f, -0.6f,
                                      0.9992f, -1.5f, 5.31f, -0.8766f,
                                      2.1090f, -0.3f, 1.30f,  0.0f };

    float tmp[NB_PIXELS*4];
    memcpy(tmp, &src[0], 4*NB_PIXELS*sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }
}

OIIO_ADD_TEST(MatrixOffsetOp, matrix)
{
    const float error = 1e-6f;

    const float matrix[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                               0.5f, 1.6f, 0.7f, 0.8f,
                               0.2f, 0.1f, 1.1f, 0.2f,
                               0.3f, 0.4f, 0.5f, 1.6f };
                   
    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateMatrixOp(ops, matrix, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    OIIO_CHECK_NO_THROW(CreateMatrixOp(ops, matrix, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    const unsigned long NB_PIXELS = 3;
    const float src[NB_PIXELS*4] = {  0.1004f,  0.201f, 0.303f, 0.408f,
                                     -0.1008f, -0.207f, 5.002f, 0.123422f,
                                      1.0090f,  1.009f, 1.044f, 1.001f };

    const double dst[NB_PIXELS*4] = {
        0.40474f,   0.91030f,   0.45508f,   0.914820f,
        1.3976888f, 3.2185376f, 5.4860244f, 2.5854352f,
        2.02530f,   3.65050f,   1.65130f,   2.829900f };

    float tmp[NB_PIXELS*4];
    memcpy(tmp, &src[0], 4*NB_PIXELS*sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError((float)dst[idx],
                                                      (float)tmp[idx],
                                                      error, 1.0f));
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError((float)src[idx],
                                                      (float)tmp[idx],
                                                      error, 1.0f));
    }
}

OIIO_ADD_TEST(MatrixOffsetOp, arbitrary)
{
    const float error = 1e-6f;

    const float matrix[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                               0.5f, 1.6f, 0.7f, 0.8f,
                               0.2f, 0.1f, 1.1f, 0.2f,
                               0.3f, 0.4f, 0.5f, 1.6f };
                   
    const float offset[4] = { -0.5f, -0.25f, 0.25f, 0.1f };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(
        CreateMatrixOffsetOp(ops, matrix, offset,TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    OIIO_CHECK_NO_THROW(
        CreateMatrixOffsetOp(ops, matrix, offset, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    const unsigned long NB_PIXELS = 3;
    const float src[NB_PIXELS*4] = {
         0.1004f,  0.201f, 0.303f, 0.408f,
        -0.1008f, -0.207f, 5.02f,  0.123422f,
         1.0090f,  1.009f, 1.044f, 1.001f };

    const float dst[NB_PIXELS*4] = {
        -0.09526f,   0.660300f,  0.70508f,   1.014820f,
        0.9030888f, 2.9811376f, 5.7558244f,  2.6944352f,
         1.52530f,   3.400500f,  1.90130f,   2.929900f };

    float tmp[NB_PIXELS*4];
    memcpy(tmp, &src[0], 4*NB_PIXELS*sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError((float)dst[idx],
                                                      (float)tmp[idx],
                                                      error, 1.0f));
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OIIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError((float)src[idx],
                                                      (float)tmp[idx],
                                                      error, 1.0f));
    }

    std::string opInfo0 = ops[0]->getInfo();
    OIIO_CHECK_ASSERT(!opInfo0.empty());

    std::string opInfo1 = ops[1]->getInfo();
    OIIO_CHECK_EQUAL(opInfo0, opInfo1);

    OpRcPtr clonedOp = ops[1]->clone();
    std::string cacheID = ops[1]->getCacheID();
    std::string cacheIDCloned = clonedOp->getCacheID();
    
    OIIO_CHECK_ASSERT(cacheIDCloned.empty());
    OIIO_CHECK_NO_THROW(clonedOp->finalize());

    cacheIDCloned = clonedOp->getCacheID();

    OIIO_CHECK_ASSERT(!cacheIDCloned.empty());
    OIIO_CHECK_EQUAL(cacheIDCloned, cacheID);
}

OIIO_ADD_TEST(MatrixOffsetOp, create_fit_op)
{
    const float error = 1e-6f;

    const float oldmin4[4] = { 0.0f, 1.0f, 1.0f, 4.0f };
    const float oldmax4[4] = { 1.0f, 3.0f, 4.0f, 8.0f };
    const float newmin4[4] = { 0.0f, 2.0f, 0.0f, 4.0f };
    const float newmax4[4] = { 1.0f, 6.0f, 9.0f, 20.0f };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateFitOp(ops,
                                    oldmin4, oldmax4,
                                    newmin4, newmax4, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    OIIO_CHECK_NO_THROW(CreateFitOp(ops,
                                    oldmin4, oldmax4,
                                    newmin4, newmax4, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    const unsigned long NB_PIXELS = 3;
    const float src[NB_PIXELS * 4] = {  0.1004f, 0.201f, 0.303f, 0.408f,
                                       -0.10f,  -2.10f,  0.5f,   1.0f,
                                       42.0f,    1.0f,  -1.11f, -0.001f };

    const double dst[NB_PIXELS * 4] = {  0.1004f, 0.402f, -2.091f, -10.368f,
                                        -0.10f,  -4.20f,  -1.50f,   -8.0f,
                                        42.0f,    2.0f,   -6.33f,  -12.004f };

    float tmp[NB_PIXELS * 4];
    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for (unsigned long idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for (unsigned long idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }

}

OIIO_ADD_TEST(MatrixOffsetOp, create_saturation_op)
{
    const float error = 1e-6f;
    const float sat = 0.9f;
    const float lumaCoef3[3] = { 1.0f, 0.5f, 0.1f };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(
        CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD));

    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    OIIO_CHECK_NO_THROW(
        CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    const unsigned long NB_PIXELS = 3;
    const float src[NB_PIXELS * 4] = { 0.1004f, 0.201f, 0.303f, 0.408f,
                                      -0.10f,  -2.1f,   0.5f,   1.0f,
                                      42.0f,    1.0f,  -1.11f, -0.001f };

    const double dst[NB_PIXELS * 4] = {
         0.11348f, 0.20402f, 0.29582f, 0.408f,
        -0.2f,    -2.0f,     0.34f,    1.0f,
        42.0389f,  5.1389f,  3.2399f, -0.001f };

    float tmp[NB_PIXELS * 4];
    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for (unsigned long idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for (unsigned long idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }
}

OIIO_ADD_TEST(MatrixOffsetOp, create_min_max_op)
{
    const float error = 1e-6f;

    const float min3[4] = { 1.0f, 2.0f, 3.0f };
    const float max3[4] = { 2.0f, 4.0f, 6.0f };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateMinMaxOp(ops, min3, max3, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    const unsigned long NB_PIXELS = 5;
    const float src[NB_PIXELS * 4] = { 1.0f, 2.0f, 3.0f,  1.0f,
                                       1.5f, 2.5f, 3.15f, 1.0f,
                                       0.0f, 0.0f, 0.0f,  1.0f,
                                       3.0f, 5.0f, 6.3f,  1.0f,
                                       2.0f, 4.0f, 6.0f,  1.0f };

    const double dst[NB_PIXELS * 4] = { 0.0f,  0.0f,  0.0f,  1.0f,
                                        0.5f,  0.25f, 0.05f, 1.0f,
                                       -1.0f, -1.0f, -1.0f,  1.0f,
                                        2.0f,  1.5f,  1.1f,  1.0f,
                                        1.0f,  1.0f,  1.0f,  1.0f };

    float tmp[NB_PIXELS * 4];
    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for (unsigned long idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OIIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

}

OIIO_ADD_TEST(MatrixOffsetOp, combining)
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
        OIIO_REQUIRE_EQUAL(ops.size(), 2);
        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[1]->finalize());
        
        OpRcPtrVec combined;
        ConstOpRcPtr opc1 = ops[1];
        OIIO_CHECK_NO_THROW(ops[0]->combineWith(combined, opc1));
        OIIO_REQUIRE_EQUAL(combined.size(), 1);
        OIIO_CHECK_NO_THROW(combined[0]->finalize());

        const std::string cacheIDCombined = combined[0]->getCacheID();
        OIIO_CHECK_ASSERT(!cacheIDCombined.empty());

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

        // Now try the same thing but use FinalizeOpVec to call combineWith. 
        ops.clear();
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m1, v1, TRANSFORM_DIR_FORWARD));
        OIIO_CHECK_NO_THROW(
            CreateMatrixOffsetOp(ops, m2, v2, TRANSFORM_DIR_FORWARD));
        OIIO_REQUIRE_EQUAL(ops.size(), 2);
        OpRcPtr op0 = ops[0];
        OpRcPtr op1 = ops[1];

        OIIO_CHECK_NO_THROW(FinalizeOpVec(ops));
        OIIO_REQUIRE_EQUAL(ops.size(), 1);

        const std::string cacheIDOptimized = ops[0]->getCacheID();
        OIIO_CHECK_ASSERT(!cacheIDOptimized.empty());

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
        OIIO_REQUIRE_EQUAL(ops.size(), 2);
        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[1]->finalize());
        
        OpRcPtrVec combined;
        ConstOpRcPtr opc1 = ops[1];
        OIIO_CHECK_NO_THROW(ops[0]->combineWith(combined, opc1));
        OIIO_REQUIRE_EQUAL(combined.size(), 1);
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
        OIIO_REQUIRE_EQUAL(ops.size(), 2);
        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[1]->finalize());
        
        OpRcPtrVec combined;
        ConstOpRcPtr opc1 = ops[1];
        OIIO_CHECK_NO_THROW(ops[0]->combineWith(combined, opc1));
        OIIO_REQUIRE_EQUAL(combined.size(), 1);
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
        OIIO_REQUIRE_EQUAL(ops.size(), 2);
        OIIO_CHECK_NO_THROW(ops[0]->finalize());
        OIIO_CHECK_NO_THROW(ops[1]->finalize());
        
        OpRcPtrVec combined;
        OCIO::ConstOpRcPtr op1 = ops[1];
        OIIO_CHECK_NO_THROW(ops[0]->combineWith(combined, op1));
        OIIO_REQUIRE_EQUAL(combined.size(), 1);
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

OIIO_ADD_TEST(MatrixOffsetOp, throw_create)
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

    // FitOp can't be created when old min and max are equal.
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

OIIO_ADD_TEST(MatrixOffsetOp, throw_finalize)
{
    // Matrix that can't be inverted can't be used in inverse direction.
    OpRcPtrVec ops;
    const float scale[] = { 0.0f, 1.3f, 0.3f, 1.0f };
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_INVERSE));

    OIIO_CHECK_THROW_WHAT(ops[0]->finalize(),
        OCIO::Exception, "Singular Matrix can't be inverted");
}

OIIO_ADD_TEST(MatrixOffsetOp, throw_combine)
{
    OpRcPtrVec ops;
    
    // Combining with different op.
    const float offset[] = { 1.1f, -1.3f, 0.3f, 0.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateFileNoOp(ops, "NoOp"));

    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];

    OpRcPtrVec combinedOps;
    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "can only be combined with other MatrixOffsetOps");

    // Combining forward with inverse that can't be inverted.
    ops.clear();
    const float scaleNoInv[] = { 1.1f, 0.0f, 0.3f, 0.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "Singular Matrix can't be inverted");

    // Combining inverse that can't be inverted with forward.
    ops.clear();
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "Singular Matrix can't be inverted");

    // Combining inverse with inverse that can't be inverted.
    ops.clear();
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "Singular Matrix can't be inverted");
    
    // Combining inverse that can't be inverted with inverse.
    ops.clear();
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scaleNoInv, TRANSFORM_DIR_INVERSE));
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OIIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "Singular Matrix can't be inverted");

}

OIIO_ADD_TEST(MatrixOffsetOp, no_op)
{
    OpRcPtrVec ops;
    const float offset[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    OIIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, TRANSFORM_DIR_FORWARD));

    // No ops are not created.
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

    // Except if CreateIdentityMatrixOp is used.
    OIIO_CHECK_NO_THROW(CreateIdentityMatrixOp(ops, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_ASSERT(ops[0]->isNoOp());
    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    OIIO_CHECK_ASSERT(ops[0]->isNoOp());

    OIIO_CHECK_NO_THROW(CreateIdentityMatrixOp(ops, TRANSFORM_DIR_INVERSE));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_ASSERT(ops[1]->isNoOp());
    OIIO_CHECK_NO_THROW(ops[1]->finalize());
    OIIO_CHECK_ASSERT(ops[1]->isNoOp());

}

OIIO_ADD_TEST(MatrixOffsetOp, is_same_type)
{
    const float sat = 0.9f;
    const float lumaCoef3[3] = { 1.0f, 0.5f, 0.1f };
    const float scale[] = { 1.1f, 1.3f, 0.3f, 1.0f };
    const float k[3] = { 0.18f, 0.5f, 0.3f };
    const float m[3] = { 2.0f, 4.0f, 8.0f };
    const float b[3] = { 0.1f, 0.1f, 0.1f };
    const float base[3] = { 10.0f, 5.0f, 2.0f };
    const float kb[3] = { 1.0f, 1.0f, 1.0f };

    // Create saturation, scale and log.
    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(
        CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 1);
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_FORWARD));
    OIIO_CHECK_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(
        CreateLogOp(ops, k, m, b, base, kb, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];

    // saturation and scale are MatrixOffset operators, log is not.
    OIIO_CHECK_ASSERT(ops[0]->isSameType(op1));
    OIIO_CHECK_ASSERT(ops[1]->isSameType(op0));
    OIIO_CHECK_ASSERT(!ops[0]->isSameType(op2));
    OIIO_CHECK_ASSERT(!ops[2]->isSameType(op0));
    OIIO_CHECK_ASSERT(!ops[1]->isSameType(op2));
    OIIO_CHECK_ASSERT(!ops[2]->isSameType(op1));
}

OIIO_ADD_TEST(MatrixOffsetOp, is_inverse)
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
    OIIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];
    OCIO::ConstOpRcPtr op3 = ops[3];

    OIIO_CHECK_ASSERT(ops[0]->isInverse(op1));
    OIIO_CHECK_ASSERT(ops[1]->isInverse(op0));
    
    OIIO_CHECK_ASSERT(ops[0]->isInverse(op2));
    OIIO_CHECK_ASSERT(ops[2]->isInverse(op0));
    OIIO_CHECK_ASSERT(!ops[2]->isInverse(op3));
    OIIO_CHECK_ASSERT(!ops[3]->isInverse(op2));
}

OIIO_ADD_TEST(MatrixOffsetOp, has_channel_crosstalk)
{
    const float scale[] = { 1.1f, 1.3f, 0.3f, 1.0f };
    const float sat = 0.9f;
    const float lumaCoef3[3] = { 1.0f, 0.5f, 0.1f };

    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(CreateScaleOp(ops, scale, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_NO_THROW(ops[0]->finalize());
    OIIO_CHECK_NO_THROW(
        CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 2);
    OIIO_CHECK_NO_THROW(ops[1]->finalize());

    OIIO_CHECK_ASSERT(!ops[0]->hasChannelCrosstalk());
    OIIO_CHECK_ASSERT(ops[1]->hasChannelCrosstalk());
}

OIIO_ADD_TEST(MatrixOffsetOp, removing_red_green)
{
    MatrixOpDataRcPtr mat = std::make_shared<MatrixOpData>();
    mat->setArrayValue(0, 0.0);
    mat->setArrayValue(5, 0.0);
    OpRcPtrVec ops;
    OIIO_CHECK_NO_THROW(
        CreateMatrixOp(ops, mat, TRANSFORM_DIR_FORWARD));
    OIIO_REQUIRE_EQUAL(ops.size(), 1);
    OIIO_CHECK_NO_THROW(ops[0]->finalize());

    const unsigned long NB_PIXELS = 6;
    const float src[NB_PIXELS * 4] = {
        0.1004f,  0.201f,  0.303f, 0.408f,
        -0.1008f, -0.207f, 0.502f, 0.123422f,
        1.0090f,  1.009f,  1.044f, 1.001f,
        1.1f,     1.2f,    1.3f,   1.0f,
        1.4f,     1.5f,    1.6f,   0.0f,
        1.7f,     1.8f,    1.9f,   1.0f };

    float tmp[NB_PIXELS * 4];
    memcpy(tmp, &src[0], 4 * NB_PIXELS * sizeof(float));

    ops[0]->apply(tmp, NB_PIXELS);

    for (unsigned long idx = 0; idx<NB_PIXELS; idx+=4)
    {
        OIIO_CHECK_EQUAL(0.0f, tmp[idx]);
        OIIO_CHECK_EQUAL(0.0f, tmp[idx+1]);
        OIIO_CHECK_EQUAL(src[idx+2], tmp[idx+2]);
        OIIO_CHECK_EQUAL(src[idx+3], tmp[idx+3]);
    }
}

#endif
