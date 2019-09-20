// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


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
            MatrixOffsetOp() = delete;
            MatrixOffsetOp(const MatrixOffsetOp &) = delete;

            MatrixOffsetOp(const double * m44,
                           const double * offset4,
                           TransformDirection direction);

            MatrixOffsetOp(MatrixOpDataRcPtr & matrix,
                           TransformDirection direction);

            virtual ~MatrixOffsetOp();

            TransformDirection getDirection() const noexcept override { return m_direction; }

            OpRcPtr clone() const override;

            std::string getInfo() const override;

            bool isSameType(ConstOpRcPtr & op) const override;
            bool isInverse(ConstOpRcPtr & op) const override;
            bool canCombineWith(ConstOpRcPtr & op) const override;
            void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

            void finalize(FinalizationFlags fFlags) override;

            ConstOpCPURcPtr getCPUOp() const override;

            void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

        protected:
            ConstMatrixOpDataRcPtr matrixData() const { return DynamicPtrCast<const MatrixOpData>(data()); }
            MatrixOpDataRcPtr matrixData() { return DynamicPtrCast<MatrixOpData>(data()); }

        private:
            TransformDirection m_direction;
        };


        typedef OCIO_SHARED_PTR<MatrixOffsetOp> MatrixOffsetOpRcPtr;
        typedef OCIO_SHARED_PTR<const MatrixOffsetOp> ConstMatrixOffsetOpRcPtr;

        
        MatrixOffsetOp::MatrixOffsetOp(const double * m44,
                                       const double * offset4,
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

            if (!composedMat->isNoOp())
            {
                CreateMatrixOp(ops, composedMat, TRANSFORM_DIR_FORWARD);
            }
        }

        void MatrixOffsetOp::finalize(FinalizationFlags /*fFlags*/)
        {
            if(m_direction == TRANSFORM_DIR_INVERSE)
            {
                MatrixOpDataRcPtr thisMat = matrixData();
                thisMat = thisMat->inverse();
                data() = thisMat;
                m_direction = TRANSFORM_DIR_FORWARD;
            }

            matrixData()->finalize();

            // Create the cacheID
            std::ostringstream cacheIDStream;
            cacheIDStream << "<MatrixOffsetOp ";
            cacheIDStream << matrixData()->getCacheID() << " ";
            cacheIDStream << TransformDirectionToString(m_direction) << " ";
            cacheIDStream << ">";

            m_cacheID = cacheIDStream.str();
        }

        ConstOpCPURcPtr MatrixOffsetOp::getCPUOp() const
        {
            ConstMatrixOpDataRcPtr data = matrixData();
            return GetMatrixRenderer(data);
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
                       const double * scale4,
                       TransformDirection direction)
    {
        const double offset4[4] = { 0, 0, 0, 0 };
        CreateScaleOffsetOp(ops, scale4, offset4, direction);
    }

    void CreateMatrixOp(OpRcPtrVec & ops,
                        const double * m44,
                        TransformDirection direction)
    {
        const double offset4[4] = { 0.0, 0.0, 0.0, 0.0 };
        CreateMatrixOffsetOp(ops, m44, offset4, direction);
    }

    void CreateOffsetOp(OpRcPtrVec & ops,
                        const double * offset4,
                        TransformDirection direction)
    {
        const double scale4[4] = { 1.0, 1.0, 1.0, 1.0 };
        CreateScaleOffsetOp(ops, scale4, offset4, direction);
    }

    void CreateScaleOffsetOp(OpRcPtrVec & ops,
                             const double * scale4, const double * offset4,
                             TransformDirection direction)
    {
        double m44[16]{ 0.0 };
        
        m44[0] = scale4[0];
        m44[5] = scale4[1];
        m44[10] = scale4[2];
        m44[15] = scale4[3];

        CreateMatrixOffsetOp(ops,
                             m44, offset4,
                             direction);
    }

    void CreateSaturationOp(OpRcPtrVec & ops,
                            double sat,
                            const double * lumaCoef3,
                            TransformDirection direction)
    {
        double matrix[16];
        double offset[4];
        MatrixTransform::Sat(matrix, offset,
                             sat, lumaCoef3);

        CreateMatrixOffsetOp(ops, matrix, offset, direction);
    }

    void CreateMatrixOffsetOp(OpRcPtrVec & ops,
                              const double * m44, const double * offset4,
                              TransformDirection direction)
    {
        auto mat = std::make_shared<MatrixOpData>();
        mat->setRGBA(m44);
        mat->setRGBAOffsets(offset4);

        CreateMatrixOp(ops, mat, direction);
    }

    void CreateFitOp(OpRcPtrVec & ops,
                     const double * oldmin4, const double * oldmax4,
                     const double * newmin4, const double * newmax4,
                     TransformDirection direction)
    {
        double matrix[16];
        double offset[4];
        MatrixTransform::Fit(matrix, offset,
                             oldmin4, oldmax4,
                             newmin4, newmax4);

        CreateMatrixOffsetOp(ops, matrix, offset, direction);
    }

    void CreateIdentityMatrixOp(OpRcPtrVec & ops,
                                TransformDirection direction)
    {
        double matrix[16]{ 0.0 };
        matrix[0] = 1.0;
        matrix[5] = 1.0;
        matrix[10] = 1.0;
        matrix[15] = 1.0;
        const double offset[4] = { 0.0, 0.0, 0.0, 0.0 };

        ops.push_back(std::make_shared<MatrixOffsetOp>(matrix,
                                                       offset,
                                                       direction));
    }

    void CreateMinMaxOp(OpRcPtrVec & ops,
                        const double * from_min3,
                        const double * from_max3,
                        TransformDirection direction)
    {
        double scale4[4] = { 1.0, 1.0, 1.0, 1.0 };
        double offset4[4] = { 0.0, 0.0, 0.0, 0.0 };

        bool somethingToDo = false;
        for (int i = 0; i < 3; ++i)
        {
            scale4[i] = 1.0 / (from_max3[i] - from_min3[i]);
            offset4[i] = -from_min3[i] * scale4[i];
            somethingToDo |= (scale4[i] != 1.0 || offset4[i] != 0.0);
        }

        if (somethingToDo)
        {
            CreateScaleOffsetOp(ops, scale4, offset4, direction);
        }
    }

    void CreateMinMaxOp(OpRcPtrVec & ops,
                        float from_min,
                        float from_max,
                        TransformDirection direction)
    {
        const double min[3] = { from_min, from_min, from_min };
        const double max[3] = { from_max, from_max, from_max };
        CreateMinMaxOp(ops, min, max, direction);
    }

    void CreateMatrixOp(OpRcPtrVec & ops,
                        MatrixOpDataRcPtr & matrix,
                        TransformDirection direction)
    {
        ops.push_back(std::make_shared<MatrixOffsetOp>(matrix, direction));
    }

    void CreateIdentityMatrixOp(OpRcPtrVec & ops)
    {
        MatrixOpDataRcPtr mat
            = MatrixOpData::CreateDiagonalMatrix(BIT_DEPTH_F32, BIT_DEPTH_F32, 1.0);

        ops.push_back(std::make_shared<MatrixOffsetOp>(mat, TRANSFORM_DIR_FORWARD));
    }

    ///////////////////////////////////////////////////////////////////////////

    void CreateMatrixTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
    {
        auto mat = DynamicPtrCast<const MatrixOffsetOp>(op);
        if (!mat)
        {
            throw Exception("CreateMatrixTransform: op has to be a MatrixOffsetOp");
        }
        auto matTransform = MatrixTransform::Create();
        matTransform->setDirection(mat->getDirection());

        auto matDataSrc = DynamicPtrCast<const MatrixOpData>(op->data());

        // Clone to make 32F.
        auto matData = matDataSrc->clone();
        matData->setInputBitDepth(BIT_DEPTH_F32);
        matData->setOutputBitDepth(BIT_DEPTH_F32);

        matTransform->setFileInputBitDepth(matData->getFileInputBitDepth());
        matTransform->setFileOutputBitDepth(matData->getFileOutputBitDepth());
        auto & formatMetadata = matTransform->getFormatMetadata();
        auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
        metadata = matData->getFormatMetadata();

        if (matData->getArray().getLength() != 4)
        {
            // Note: By design, only 4x4 matrices are instantiated.
            // The CLF 3x3 (and 3x4) matrices are automatically converted
            // to 4x4 matrices, and a Matrix Transform only expects 4x4 matrices.

            std::ostringstream os;
            os << "CreateMatrixTransform: The matrix dimension is always ";
            os << "expected to be 4. Found: ";
            os << matData->getArray().getLength() << ".";
            throw Exception(os.str().c_str());
        }

        matTransform->setMatrix(matData->getArray().getValues().data());
        matTransform->setOffset(matData->getOffsets().getValues());

        group->push_back(matTransform);
    }

    void BuildMatrixOps(OpRcPtrVec & ops,
                        const Config & /*config*/,
                        const MatrixTransform & transform,
                        TransformDirection dir)
    {
        TransformDirection combinedDir =
            CombineTransformDirections(dir,
                                       transform.getDirection());

        double matrix[16];
        double offset[4];
        transform.getMatrix(matrix);
        transform.getOffset(offset);

        const FormatMetadataImpl metadata(transform.getFormatMetadata());
        MatrixOpDataRcPtr mat = std::make_shared<MatrixOpData>(BIT_DEPTH_F32,
                                                               BIT_DEPTH_F32,
                                                               metadata);
        mat->setFileInputBitDepth(transform.getFileInputBitDepth());
        mat->setFileOutputBitDepth(transform.getFileOutputBitDepth());
        mat->setRGBA(matrix);
        mat->setRGBAOffsets(offset);

        CreateMatrixOp(ops, mat, combinedDir);
    }

}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

#include "ops/Log/LogOps.h"
#include "ops/NoOp/NoOps.h"
#include "UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


// TODO: syncolor also tests various bit-depths and pixel formats.
// synColorCheckApply_test.cpp - CheckMatrixRemovingGreen
// synColorCheckApply_test.cpp - CheckMatrixWithInt16Scaling
// synColorCheckApply_test.cpp - CheckMatrixWithFloatScaling
// synColorCheckApply_test.cpp - CheckMatrixWithHalfScaling
// synColorCheckApply_test.cpp - CheckIdentityWith16iRGBAImage
// synColorCheckApply_test.cpp - CheckIdentityWith16iBGRAImage
// synColorCheckApply_test.cpp - CheckMatrixWith16iRGBAImage


OCIO_ADD_TEST(MatrixOffsetOp, scale)
{
    const float error = 1e-6f;

    OCIO::OpRcPtrVec ops;
    const double scale[] = { 1.1, 1.3, 0.3, -1.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");

    std::string cacheID = ops[0]->getCacheID();
    OCIO_REQUIRE_ASSERT(cacheID.empty());
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));

    cacheID = ops[0]->getCacheID();
    OCIO_REQUIRE_ASSERT(!cacheID.empty());

    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

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
        OCIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OCIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }
}

OCIO_ADD_TEST(MatrixOffsetOp, offset)
{
    const float error = 1e-6f;

    OCIO::OpRcPtrVec ops;
    const double offset[] = { 1.1, -1.3, 0.3, -1.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));

    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

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
        OCIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OCIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }
}

OCIO_ADD_TEST(MatrixOffsetOp, matrix)
{
    const float error = 1e-6f;

    const double matrix[16] = { 1.1, 0.2, 0.3, 0.4,
                                0.5, 1.6, 0.7, 0.8,
                                0.2, 0.1, 1.1, 0.2,
                                0.3, 0.4, 0.5, 1.6 };
                   
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));

    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

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
        OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError((float)dst[idx],
                                                      (float)tmp[idx],
                                                      error, 1.0f));
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError((float)src[idx],
                                                      (float)tmp[idx],
                                                      error, 1.0f));
    }
}

OCIO_ADD_TEST(MatrixOffsetOp, arbitrary)
{
    const float error = 1e-6f;

    const double matrix[16] = { 1.1, 0.2, 0.3, 0.4,
                                0.5, 1.6, 0.7, 0.8,
                                0.2, 0.1, 1.1, 0.2,
                                0.3, 0.4, 0.5, 1.6 };
                   
    const double offset[4] = { -0.5, -0.25, 0.25, 0.1 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(
        OCIO::CreateMatrixOffsetOp(ops, matrix, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));

    OCIO_CHECK_NO_THROW(
        OCIO::CreateMatrixOffsetOp(ops, matrix, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

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
        OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError((float)dst[idx],
                                                      (float)tmp[idx],
                                                      error, 1.0f));
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for(unsigned long idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError((float)src[idx],
                                                      (float)tmp[idx],
                                                      error, 1.0f));
    }

    std::string opInfo0 = ops[0]->getInfo();
    OCIO_CHECK_ASSERT(!opInfo0.empty());

    std::string opInfo1 = ops[1]->getInfo();
    OCIO_CHECK_EQUAL(opInfo0, opInfo1);

    OCIO::OpRcPtr clonedOp = ops[1]->clone();
    std::string cacheID = ops[1]->getCacheID();
    std::string cacheIDCloned = clonedOp->getCacheID();

    OCIO_CHECK_ASSERT(cacheIDCloned.empty());
    OCIO_CHECK_NO_THROW(clonedOp->finalize(OCIO::FINALIZATION_EXACT));

    cacheIDCloned = clonedOp->getCacheID();

    OCIO_CHECK_ASSERT(!cacheIDCloned.empty());
    OCIO_CHECK_EQUAL(cacheIDCloned, cacheID);
}

OCIO_ADD_TEST(MatrixOffsetOp, create_fit_op)
{
    const float error = 1e-6f;

    const double oldmin4[4] = { 0.0, 1.0, 1.0,  4.0 };
    const double oldmax4[4] = { 1.0, 3.0, 4.0,  8.0 };
    const double newmin4[4] = { 0.0, 2.0, 0.0,  4.0 };
    const double newmax4[4] = { 1.0, 6.0, 9.0, 20.0 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateFitOp(ops,
                                          oldmin4, oldmax4,
                                          newmin4, newmax4, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));

    OCIO_CHECK_NO_THROW(OCIO::CreateFitOp(ops,
                                          oldmin4, oldmax4,
                                          newmin4, newmax4, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

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
        OCIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for (unsigned long idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OCIO_CHECK_CLOSE(src[idx], tmp[idx], error);
    }

}

OCIO_ADD_TEST(MatrixOffsetOp, create_saturation_op)
{
    const float error = 1e-6f;
    const double sat = 0.9;
    const double lumaCoef3[3] = { 1.0, 0.5, 0.1 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(
        OCIO::CreateSaturationOp(ops, sat, lumaCoef3, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));

    OCIO_CHECK_NO_THROW(
        OCIO::CreateSaturationOp(ops, sat, lumaCoef3, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

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
        OCIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

    ops[1]->apply(tmp, NB_PIXELS);

    for (unsigned long idx = 0; idx<(NB_PIXELS * 4); ++idx)
    {
        OCIO_CHECK_CLOSE(src[idx], tmp[idx], 10.0f*error);
    }
}

OCIO_ADD_TEST(MatrixOffsetOp, create_min_max_op)
{
    const float error = 1e-6f;

    const double min3[4] = { 1.0, 2.0, 3.0 };
    const double max3[4] = { 2.0, 4.0, 6.0 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateMinMaxOp(ops, min3, max3, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<MatrixOffsetOp>");
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));

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
        OCIO_CHECK_CLOSE(dst[idx], tmp[idx], error);
    }

}

OCIO_ADD_TEST(MatrixOffsetOp, combining)
{
    const float error = 1e-4f;
    const double m1[16] = { 1.1, 0.2, 0.3, 0.4,
                            0.5, 1.6, 0.7, 0.8,
                            0.2, 0.1, 1.1, 0.2,
                            0.3, 0.4, 0.5, 1.6 };
    const double v1[4] = { -0.5, -0.25, 0.25, 0.0 };
    const double m2[16] = { 1.1, -0.1, -0.1, 0.0,
                            0.1,  0.9, -0.2, 0.0,
                            0.05, 0.0,  1.1, 0.0,
                            0.0,  0.0,  0.0, 1.0 };
    const double v2[4] = { -0.2, -0.1, -0.1, -0.2 };
    const float source[] = { 0.1f, 0.2f, 0.3f, 0.4f,
                             -0.1f, -0.2f, 50.0f, 123.4f,
                             1.0f, 1.0f, 1.0f, 1.0f };

    {
        OCIO::OpRcPtrVec ops;

        auto mat1 = std::make_shared<OCIO::MatrixOpData>();
        mat1->setRGBA(m1);
        mat1->setRGBAOffsets(v1);
        mat1->getFormatMetadata().addAttribute(OCIO::METADATA_NAME, "mat1");
        mat1->getFormatMetadata().addAttribute("Attrib", "1");
        OCIO_CHECK_NO_THROW(CreateMatrixOp(ops, mat1, OCIO::TRANSFORM_DIR_FORWARD));

        auto mat2 = std::make_shared<OCIO::MatrixOpData>();
        mat2->setRGBA(m2);
        mat2->setRGBAOffsets(v2);
        mat2->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "ID2");
        mat2->getFormatMetadata().addAttribute("Attrib", "2");
        OCIO_CHECK_NO_THROW(CreateMatrixOp(ops, mat2, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);

        OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
        OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

        OCIO::OpRcPtrVec combined;
        OCIO::ConstOpRcPtr opc1 = ops[1];
        OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, opc1));
        OCIO_REQUIRE_EQUAL(combined.size(), 1);
        OCIO_CHECK_NO_THROW(combined[0]->finalize(OCIO::FINALIZATION_EXACT));

        auto combinedData = OCIO::DynamicPtrCast<const OCIO::Op>(combined[0])->data();

        // Check metadata of combined op.
        OCIO_CHECK_EQUAL(combinedData->getName(), "mat1");
        OCIO_CHECK_EQUAL(combinedData->getID(), "ID2");
        // 3 attributes: name, id, Attrib.
        OCIO_CHECK_EQUAL(combinedData->getFormatMetadata().getNumAttributes(), 3);
        auto & attribs = combinedData->getFormatMetadata().getAttributes();
        OCIO_CHECK_EQUAL(attribs[1].first, "Attrib");
        OCIO_CHECK_EQUAL(attribs[1].second, "1 + 2");

        const std::string cacheIDCombined = combined[0]->getCacheID();
        OCIO_CHECK_ASSERT(!cacheIDCombined.empty());

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
                OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
            }
        }

        // Now try the same thing but use FinalizeOpVec to call combineWith.
        ops.clear();
        OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, mat1, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, mat2, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        OCIO::OpRcPtr op0 = ops[0];
        OCIO::OpRcPtr op1 = ops[1];

        OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
        OCIO_CHECK_NO_THROW(OCIO::FinalizeOpVec(ops, OCIO::FINALIZATION_EXACT));
        OCIO_REQUIRE_EQUAL(ops.size(), 1);

        const std::string cacheIDOptimized = ops[0]->getCacheID();
        OCIO_CHECK_ASSERT(!cacheIDOptimized.empty());

        OCIO_CHECK_EQUAL(cacheIDCombined, cacheIDOptimized);

        op0->finalize(OCIO::FINALIZATION_EXACT);
        op1->finalize(OCIO::FINALIZATION_EXACT);

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
                OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
            }
        }
    }


    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(
            OCIO::CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_NO_THROW(
            OCIO::CreateMatrixOffsetOp(ops, m2, v2, OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
        OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

        OCIO::OpRcPtrVec combined;
        OCIO::ConstOpRcPtr opc1 = ops[1];
        OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, opc1));
        OCIO_REQUIRE_EQUAL(combined.size(), 1);
        OCIO_CHECK_NO_THROW(combined[0]->finalize(OCIO::FINALIZATION_EXACT));

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
                OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
            }
        }
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(
            OCIO::CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_CHECK_NO_THROW(
            OCIO::CreateMatrixOffsetOp(ops, m2, v2, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
        OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

        OCIO::OpRcPtrVec combined;
        OCIO::ConstOpRcPtr opc1 = ops[1];
        OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, opc1));
        OCIO_REQUIRE_EQUAL(combined.size(), 1);
        OCIO_CHECK_NO_THROW(combined[0]->finalize(OCIO::FINALIZATION_EXACT));

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
                OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
            }
        }
    }

    {
        OCIO::OpRcPtrVec ops;
        OCIO_CHECK_NO_THROW(
            OCIO::CreateMatrixOffsetOp(ops, m1, v1, OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_CHECK_NO_THROW(
            OCIO::CreateMatrixOffsetOp(ops, m2, v2, OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
        OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

        OCIO::OpRcPtrVec combined;
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, op1));
        OCIO_REQUIRE_EQUAL(combined.size(), 1);
        OCIO_CHECK_NO_THROW(combined[0]->finalize(OCIO::FINALIZATION_EXACT));

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
                OCIO_CHECK_CLOSE(tmp2[i], tmp[i], error);
            }
        }
    }
}

OCIO_ADD_TEST(MatrixOffsetOp, throw_create)
{
    OCIO::OpRcPtrVec ops;
    const double scale[] = { 1.1, 1.3, 0.3, 1.0 };
    OCIO_CHECK_THROW_WHAT(
        OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");

    const double offset[] = { 1.1, -1.3, 0.3, 0.0 };
    OCIO_CHECK_THROW_WHAT(
        OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");

    const double matrix[16] = { 1.1, 0.2, 0.3, 0.4,
                                0.5, 1.6, 0.7, 0.8,
                                0.2, 0.1, 1.1, 0.2,
                                0.3, 0.4, 0.5, 1.6 };

    OCIO_CHECK_THROW_WHAT(
        OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_UNKNOWN),
        OCIO::Exception, "unspecified transform direction");

    // FitOp can't be created when old min and max are equal.
    const double oldmin4[4] = { 1.0, 0.0, 0.0,  0.0 };
    const double oldmax4[4] = { 1.0, 2.0, 3.0,  4.0 };
    const double newmin4[4] = { 0.0, 0.0, 0.0,  0.0 };
    const double newmax4[4] = { 1.0, 4.0, 9.0, 16.0 };

    OCIO_CHECK_THROW_WHAT(CreateFitOp(ops,
                                      oldmin4, oldmax4,
                                      newmin4, newmax4, OCIO::TRANSFORM_DIR_FORWARD),
                          OCIO::Exception,
                          "Cannot create Fit operator. Max value equals min value");
}

OCIO_ADD_TEST(MatrixOffsetOp, throw_finalize)
{
    // Matrix that can't be inverted can't be used in inverse direction.
    OCIO::OpRcPtrVec ops;
    const double scale[] = { 0.0, 1.3, 0.3, 1.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_CHECK_THROW_WHAT(ops[0]->finalize(OCIO::FINALIZATION_EXACT),
        OCIO::Exception, "Singular Matrix can't be inverted");
}

OCIO_ADD_TEST(MatrixOffsetOp, throw_combine)
{
    OCIO::OpRcPtrVec ops;

    // Combining with different op.
    const double offset[] = { 1.1, -1.3, 0.3, 0.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateFileNoOp(ops, "NoOp"));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO::OpRcPtrVec combinedOps;
    OCIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "can only be combined with other MatrixOffsetOps");

    // Combining forward with inverse that can't be inverted.
    ops.clear();
    const double scaleNoInv[] = { 1.1, 0.0, 0.3, 0.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scaleNoInv, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OCIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "Singular Matrix can't be inverted");

    // Combining inverse that can't be inverted with forward.
    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scaleNoInv, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OCIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "Singular Matrix can't be inverted");

    // Combining inverse with inverse that can't be inverted.
    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scaleNoInv, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OCIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "Singular Matrix can't be inverted");

    // Combining inverse that can't be inverted with inverse.
    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scaleNoInv, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OCIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "Singular Matrix can't be inverted");

}

OCIO_ADD_TEST(MatrixOffsetOp, no_op)
{
    OCIO::OpRcPtrVec ops;
    const double offset[] = { 0.0, 0.0, 0.0, 0.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));

    // No ops are created.
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    const double scale[] = { 1.0, 1.0, 1.0, 1.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    const double matrix[16] = { 1.0, 0.0, 0.0, 0.0,
                                0.0, 1.0, 0.0, 0.0,
                                0.0, 0.0, 1.0, 0.0,
                                0.0, 0.0, 0.0, 1.0 };

    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(
        OCIO::CreateMatrixOffsetOp(ops, matrix, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(
        OCIO::CreateMatrixOffsetOp(ops, matrix, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    const double oldmin4[4] = { 0.0, 0.0, 0.0, 0.0 };
    const double oldmax4[4] = { 1.0, 2.0, 3.0, 4.0 };

    OCIO_CHECK_NO_THROW(OCIO::CreateFitOp(ops,
                                          oldmin4, oldmax4,
                                          oldmin4, oldmax4,
                                          OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    OCIO_CHECK_NO_THROW(OCIO::CreateFitOp(ops,
                                          oldmin4, oldmax4,
                                          oldmin4, oldmax4, 
                                          OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    const double sat = 1.0;
    const double lumaCoef3[3] = { 1.0, 1.0, 1.0 };

    OCIO_CHECK_NO_THROW(
        OCIO::CreateSaturationOp(ops, sat, lumaCoef3, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    OCIO_CHECK_NO_THROW(
        OCIO::CreateSaturationOp(ops, sat, lumaCoef3, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::OptimizeOpVec(ops, OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    OCIO_CHECK_NO_THROW(CreateIdentityMatrixOp(ops));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());
}

OCIO_ADD_TEST(MatrixOffsetOp, is_same_type)
{
    const double sat = 0.9;
    const double lumaCoef3[3] = { 1.0, 0.5, 0.1 };
    const double scale[] = { 1.1, 1.3, 0.3, 1.0 };
    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.5, 0.3 };
    const double linSlope[3] = { 2.0, 4.0, 8.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };

    // Create saturation, scale and log.
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(
        OCIO::CreateSaturationOp(ops, sat, lumaCoef3, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(
        OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(
        OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];

    // saturation and scale are MatrixOffset operators, log is not.
    OCIO_CHECK_ASSERT(ops[0]->isSameType(op1));
    OCIO_CHECK_ASSERT(ops[1]->isSameType(op0));
    OCIO_CHECK_ASSERT(!ops[0]->isSameType(op2));
    OCIO_CHECK_ASSERT(!ops[2]->isSameType(op0));
    OCIO_CHECK_ASSERT(!ops[1]->isSameType(op2));
    OCIO_CHECK_ASSERT(!ops[2]->isSameType(op1));
}

OCIO_ADD_TEST(MatrixOffsetOp, is_inverse)
{
    OCIO::OpRcPtrVec ops;
    const double offset[] = { 1.1, -1.3, 0.3, 0.0 };
    const double offsetInv[] = { -1.1, 1.3, -0.3, 0.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offsetInv, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 3);

    const double base = 10.0;
    const double logSlope[3] = { 0.18, 0.5, 0.3 };
    const double linSlope[3] = { 2.0, 4.0, 8.0 };
    const double linOffset[3] = { 0.1, 0.1, 0.1 };
    const double logOffset[3] = { 1.0, 1.0, 1.0 };
    OCIO_CHECK_NO_THROW(
        OCIO::CreateLogOp(ops, base, logSlope, logOffset, linSlope, linOffset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];
    OCIO::ConstOpRcPtr op3 = ops[3];

    OCIO_CHECK_ASSERT(ops[0]->isInverse(op1));
    OCIO_CHECK_ASSERT(ops[1]->isInverse(op0));

    OCIO_CHECK_ASSERT(ops[0]->isInverse(op2));
    OCIO_CHECK_ASSERT(ops[2]->isInverse(op0));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op3));
    OCIO_CHECK_ASSERT(!ops[3]->isInverse(op2));
}

OCIO_ADD_TEST(MatrixOffsetOp, has_channel_crosstalk)
{
    const double scale[] = { 1.1, 1.3, 0.3, 1.0 };
    const double sat = 0.9;
    const double lumaCoef3[3] = { 1.0, 0.5, 0.1 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO_CHECK_NO_THROW(
        OCIO::CreateSaturationOp(ops, sat, lumaCoef3, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

    OCIO_CHECK_ASSERT(!ops[0]->hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(ops[1]->hasChannelCrosstalk());
}

OCIO_ADD_TEST(MatrixOffsetOp, removing_red_green)
{
    OCIO::MatrixOpDataRcPtr mat = std::make_shared<OCIO::MatrixOpData>();
    mat->setArrayValue(0, 0.0);
    mat->setArrayValue(5, 0.0);
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(
        OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));

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
        OCIO_CHECK_EQUAL(0.0f, tmp[idx]);
        OCIO_CHECK_EQUAL(0.0f, tmp[idx+1]);
        OCIO_CHECK_EQUAL(src[idx+2], tmp[idx+2]);
        OCIO_CHECK_EQUAL(src[idx+3], tmp[idx+3]);
    }
}

OCIO_ADD_TEST(MatrixOffsetOp, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;

    OCIO::FormatMetadataImpl metadataSource(OCIO::METADATA_ROOT);
    metadataSource.addAttribute("name", "test");

    OCIO::MatrixOpDataRcPtr mat
        = std::make_shared<OCIO::MatrixOpData>(OCIO::BIT_DEPTH_F32,
                                               OCIO::BIT_DEPTH_F32,
                                               metadataSource);

    const double offset[4] { 1., 2., 3., 4 };
    mat->getOffsets().setRGBA(offset);
    mat->getArray().getValues() = { 1.1, 0.2, 0.3, 0.4,
                                    0.5, 1.6, 0.7, 0.8,
                                    0.2, 0.1, 1.1, 0.2,
                                    0.3, 0.4, 0.5, 1.6 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, mat, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateMatrixTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->size(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto mTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::MatrixTransform>(transform);
    OCIO_REQUIRE_ASSERT(mTransform);

    const auto & metadata = mTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), "name");
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(mTransform->getDirection(), direction);
    double oval[4];
    mTransform->getOffset(oval);
    OCIO_CHECK_EQUAL(oval[0], offset[0]);
    OCIO_CHECK_EQUAL(oval[1], offset[1]);
    OCIO_CHECK_EQUAL(oval[2], offset[2]);
    OCIO_CHECK_EQUAL(oval[3], offset[3]);
    double mval[16];
    mTransform->getMatrix(mval);
    OCIO_CHECK_EQUAL(mval[0], mat->getArray()[0]);
    OCIO_CHECK_EQUAL(mval[1], mat->getArray()[1]);
    OCIO_CHECK_EQUAL(mval[2], mat->getArray()[2]);
    OCIO_CHECK_EQUAL(mval[3], mat->getArray()[3]);
    OCIO_CHECK_EQUAL(mval[4], mat->getArray()[4]);
    OCIO_CHECK_EQUAL(mval[5], mat->getArray()[5]);
    OCIO_CHECK_EQUAL(mval[6], mat->getArray()[6]);
    OCIO_CHECK_EQUAL(mval[7], mat->getArray()[7]);
    OCIO_CHECK_EQUAL(mval[8], mat->getArray()[8]);
    OCIO_CHECK_EQUAL(mval[9], mat->getArray()[9]);
    OCIO_CHECK_EQUAL(mval[10], mat->getArray()[10]);
    OCIO_CHECK_EQUAL(mval[11], mat->getArray()[11]);
    OCIO_CHECK_EQUAL(mval[12], mat->getArray()[12]);
    OCIO_CHECK_EQUAL(mval[13], mat->getArray()[13]);
    OCIO_CHECK_EQUAL(mval[14], mat->getArray()[14]);
    OCIO_CHECK_EQUAL(mval[15], mat->getArray()[15]);
}

#endif
