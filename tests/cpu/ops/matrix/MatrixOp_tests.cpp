// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/log/LogOp.h"

#include "ops/matrix/MatrixOp.cpp"

#include "ops/noop/NoOps.h"
#include "testutils/UnitTest.h"
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

    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops.finalize());

    std::string cacheID;
    OCIO_CHECK_NO_THROW(cacheID = ops[0]->getCacheID());
    OCIO_REQUIRE_ASSERT(!cacheID.empty());

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

    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops.finalize());

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

    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops.finalize());

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

    OCIO_CHECK_NO_THROW(
        OCIO::CreateMatrixOffsetOp(ops, matrix, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops.finalize());

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
    std::string cacheID;
    OCIO_CHECK_NO_THROW(cacheID = ops[1]->getCacheID());
    std::string cacheIDCloned;
    OCIO_CHECK_NO_THROW(cacheIDCloned = clonedOp->getCacheID());

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

    OCIO_CHECK_NO_THROW(OCIO::CreateFitOp(ops,
                                          oldmin4, oldmax4,
                                          newmin4, newmax4, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops.finalize());

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

    OCIO_CHECK_NO_THROW(
        OCIO::CreateSaturationOp(ops, sat, lumaCoef3, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops.finalize());

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
    OCIO_CHECK_NO_THROW(ops.finalize());

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

        OCIO_CHECK_NO_THROW(ops.finalize());

        OCIO::OpRcPtrVec combined;
        OCIO::ConstOpRcPtr opc1 = ops[1];
        OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, opc1));
        OCIO_REQUIRE_EQUAL(combined.size(), 1);
        OCIO_CHECK_NO_THROW(combined.finalize());

        auto combinedData = OCIO::DynamicPtrCast<const OCIO::Op>(combined[0])->data();

        // Check metadata of combined op.
        OCIO_CHECK_EQUAL(combinedData->getName(), "mat1");
        OCIO_CHECK_EQUAL(combinedData->getID(), "ID2");
        // 3 attributes: name, id, Attrib.
        OCIO_CHECK_EQUAL(combinedData->getFormatMetadata().getNumAttributes(), 3);
        auto & attribs = combinedData->getFormatMetadata().getAttributes();
        OCIO_CHECK_EQUAL(attribs[1].first, "Attrib");
        OCIO_CHECK_EQUAL(attribs[1].second, "1 + 2");

        std::string cacheIDCombined;
        OCIO_CHECK_NO_THROW(cacheIDCombined = combined[0]->getCacheID());
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

        OCIO_CHECK_NO_THROW(ops.finalize());
        OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
        OCIO_REQUIRE_EQUAL(ops.size(), 1);

        std::string cacheIDOptimized;
        OCIO_CHECK_NO_THROW(cacheIDOptimized = ops[0]->getCacheID());
        OCIO_CHECK_ASSERT(!cacheIDOptimized.empty());

        OCIO_CHECK_EQUAL(cacheIDCombined, cacheIDOptimized);

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
        OCIO_CHECK_NO_THROW(ops.finalize());

        OCIO::OpRcPtrVec combined;
        OCIO::ConstOpRcPtr opc1 = ops[1];
        OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, opc1));
        OCIO_REQUIRE_EQUAL(combined.size(), 1);
        OCIO_CHECK_NO_THROW(combined.validate());

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
        OCIO_CHECK_NO_THROW(ops.finalize());

        OCIO::OpRcPtrVec combined;
        OCIO::ConstOpRcPtr opc1 = ops[1];
        OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, opc1));
        OCIO_REQUIRE_EQUAL(combined.size(), 1);
        OCIO_CHECK_NO_THROW(combined.validate());

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
        OCIO_CHECK_NO_THROW(ops.finalize());

        OCIO::OpRcPtrVec combined;
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, op1));
        OCIO_REQUIRE_EQUAL(combined.size(), 1);
        OCIO_CHECK_NO_THROW(combined.validate());

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
        const double offset[] = { 1.1, -1.3, 0.3, 0.0 };
        const double offsetInv[] = { -1.1, 1.3, -0.3, 0.0 };
        OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_CHECK_EQUAL(ops.size(), 1);
        OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_INVERSE));
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offsetInv, OCIO::TRANSFORM_DIR_FORWARD));
        OCIO_REQUIRE_EQUAL(ops.size(), 3);

        OCIO_CHECK_NO_THROW(ops.finalize());

        // Combining offset (FWD) and offset (INV) becomes an identity and is optimized out.

        OCIO::OpRcPtrVec combined;
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, op1));
        OCIO_CHECK_EQUAL(combined.size(), 0);
        combined.clear();

        // Combining offset (FWD) and offsetInv (FWD) becomes an identity and is optimized out.

        OCIO::ConstOpRcPtr op2 = ops[2];
        OCIO_CHECK_NO_THROW(ops[0]->combineWith(combined, op2));
        OCIO_CHECK_EQUAL(combined.size(), 0);
    }
}

OCIO_ADD_TEST(MatrixOffsetOp, throw_create)
{
    OCIO::OpRcPtrVec ops;

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

OCIO_ADD_TEST(MatrixOffsetOp, throw_validate)
{
    // Matrix that can't be inverted can't be used in inverse direction.
    OCIO::OpRcPtrVec ops;
    const double scale[] = { 0.0, 1.3, 0.3, 1.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_CHECK_THROW_WHAT(ops[0]->validate(), OCIO::Exception, "Singular Matrix can't be inverted");
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

    OCIO_CHECK_ASSERT(!ops[0]->canCombineWith(op1));
    OCIO::OpRcPtrVec combinedOps;
    OCIO_CHECK_THROW_WHAT(
        ops[0]->combineWith(combinedOps, op1),
        OCIO::Exception, "MatrixOffsetOp: canCombineWith must be checked before calling combineWith");

    // Combining forward with inverse that can't be inverted.
    ops.clear();
    const double scaleNoInv[] = { 1.1, 0.0, 0.3, 0.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scaleNoInv, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OCIO_CHECK_THROW_WHAT(ops[0]->canCombineWith(op1),
        OCIO::Exception, "Op::finalize has to be called");

    // Combining inverse that can't be inverted with forward.
    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scaleNoInv, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OCIO_CHECK_THROW_WHAT(ops[0]->canCombineWith(op1),
        OCIO::Exception, "Op::finalize has to be called");

    // Combining inverse with inverse that can't be inverted.
    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scaleNoInv, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OCIO_CHECK_THROW_WHAT(ops[0]->canCombineWith(op1),
        OCIO::Exception, "Op::finalize has to be called");

    // Combining inverse that can't be inverted with inverse.
    ops.clear();
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scaleNoInv, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    op1 = ops[1];

    OCIO_CHECK_THROW_WHAT(ops[0]->canCombineWith(op1),
        OCIO::Exception, "Op::finalize has to be called");

}

OCIO_ADD_TEST(MatrixOffsetOp, no_op)
{
    OCIO::OpRcPtrVec ops;
    const double offset[] = { 0.0, 0.0, 0.0, 0.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));

    // No ops are created.
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    const double scale[] = { 1.0, 1.0, 1.0, 1.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    const double matrix[16] = { 1.0, 0.0, 0.0, 0.0,
                                0.0, 1.0, 0.0, 0.0,
                                0.0, 0.0, 1.0, 0.0,
                                0.0, 0.0, 0.0, 1.0 };

    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, matrix, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(
        OCIO::CreateMatrixOffsetOp(ops, matrix, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);
    OCIO_CHECK_NO_THROW(
        OCIO::CreateMatrixOffsetOp(ops, matrix, offset, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    const double oldmin4[4] = { 0.0, 0.0, 0.0, 0.0 };
    const double oldmax4[4] = { 1.0, 2.0, 3.0, 4.0 };

    OCIO_CHECK_NO_THROW(OCIO::CreateFitOp(ops,
                                          oldmin4, oldmax4,
                                          oldmin4, oldmax4,
                                          OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    OCIO_CHECK_NO_THROW(OCIO::CreateFitOp(ops,
                                          oldmin4, oldmax4,
                                          oldmin4, oldmax4, 
                                          OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    const double sat = 1.0;
    const double lumaCoef3[3] = { 1.0, 1.0, 1.0 };

    OCIO_CHECK_NO_THROW(
        OCIO::CreateSaturationOp(ops, sat, lumaCoef3, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    OCIO_CHECK_NO_THROW(
        OCIO::CreateSaturationOp(ops, sat, lumaCoef3, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_CHECK_EQUAL(ops.size(), 0);

    OCIO_CHECK_NO_THROW(CreateIdentityMatrixOp(ops));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_ASSERT(ops[0]->isNoOp());
    OCIO_CHECK_NO_THROW(ops[0]->validate());
    OCIO_CHECK_NO_THROW(ops[0]->finalize());
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

OCIO_ADD_TEST(MatrixOffsetOp, has_channel_crosstalk)
{
    const double scale[] = { 1.1, 1.3, 0.3, 1.0 };
    const double sat = 0.9;
    const double lumaCoef3[3] = { 1.0, 0.5, 0.1 };

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateScaleOp(ops, scale, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops[0]->validate());
    OCIO_CHECK_NO_THROW(
        OCIO::CreateSaturationOp(ops, sat, lumaCoef3, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->validate());

    OCIO_CHECK_ASSERT(!ops[0]->hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(ops[1]->hasChannelCrosstalk());
}

OCIO_ADD_TEST(MatrixOffsetOp, removing_red_green)
{
    OCIO::MatrixOpDataRcPtr mat = std::make_shared<OCIO::MatrixOpData>();
    mat->setArrayValue(0, 0.0);
    mat->setArrayValue(5, 0.0);
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, mat, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops[0]->validate());
    OCIO_CHECK_NO_THROW(ops[0]->finalize());

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
    OCIO::MatrixOpDataRcPtr mat = std::make_shared<OCIO::MatrixOpData>();
    mat->getFormatMetadata().addAttribute("name", "test");

    const double offset[4] { 1., 2., 3., 4 };
    mat->getOffsets().setRGBA(offset);
    mat->getArray().getValues() = { 1.1, 0.2, 0.3, 0.4,
                                    0.5, 1.6, 0.7, 0.8,
                                    0.2, 0.1, 1.1, 0.2,
                                    0.3, 0.4, 0.5, 1.6 };

    OCIO::OpRcPtrVec ops;
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;
    OCIO_CHECK_NO_THROW(OCIO::CreateMatrixOp(ops, mat, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateMatrixTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
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

    OCIO::OpRcPtrVec opsBack;
    OCIO::BuildMatrixOp(opsBack, *mTransform, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::BuildMatrixOp(opsBack, *mTransform, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_REQUIRE_EQUAL(opsBack.size(), 2);

    auto o0 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOffsetOp>(opsBack[0]);
    auto o1 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOffsetOp>(opsBack[1]);
    OCIO_REQUIRE_ASSERT(o0);
    OCIO_REQUIRE_ASSERT(o1);
    auto m0 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(o0->data());
    auto m1 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::MatrixOpData>(o1->data());
    OCIO_REQUIRE_ASSERT(m0);
    OCIO_REQUIRE_ASSERT(m1);
    OCIO_CHECK_EQUAL(m0->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(m1->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(m0->getArray() == mat->getArray());
    OCIO_CHECK_ASSERT(m1->getArray() == mat->getArray());
    OCIO_CHECK_ASSERT(m0->getOffsets() == mat->getOffsets());
    OCIO_CHECK_ASSERT(m1->getOffsets() == mat->getOffsets());
}

