// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>
#include <cstdlib>
#ifndef _WIN32
#include <sys/time.h>
#endif

#include "BitDepthUtils.h"
#include "OpBuilders.h"
#include "ops/lut3d/Lut3DOp.cpp"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Lut3DOp, inverse_comparison_check)
{
    OCIO::Lut3DOpDataRcPtr lut_a = std::make_shared<OCIO::Lut3DOpData>(32);
    OCIO::Lut3DOpDataRcPtr lut_b = std::make_shared<OCIO::Lut3DOpData>(16);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut_a, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut_a, OCIO::TRANSFORM_DIR_INVERSE));
    // Add Matrix and LUT.
    OCIO_CHECK_NO_THROW(CreateMinMaxOp(ops, 0.5f, 1.0f, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut_b, OCIO::TRANSFORM_DIR_FORWARD));
    // Add LUT and Matrix.
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut_b, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(CreateMinMaxOp(ops, 0.5f, 1.0f, OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_REQUIRE_EQUAL(ops.size(), 6);

    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];
    OCIO::ConstOpRcPtr op3 = ops[3];
    OCIO::ConstOpRcPtr op4 = ops[4];
    OCIO::ConstOpRcPtr op4Cloned = op4->clone();

    OCIO_CHECK_ASSERT(ops[0]->isSameType(op1));
    OCIO_CHECK_ASSERT(ops[0]->isSameType(op3));
    OCIO_CHECK_ASSERT(ops[0]->isSameType(op4Cloned));

    OCIO_CHECK_ASSERT(ops[0]->isInverse(op1));
    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op3));
    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op4));
    OCIO_CHECK_ASSERT(ops[3]->isInverse(op4));
}

/*
OCIO_ADD_TEST(Lut3DOp, performance_check)
{
    OCIO::Lut3D lut;

    lut.size[0] = 32;
    lut.size[1] = 32;
    lut.size[2] = 32;

    lut.lut.resize(lut.size[0]*lut.size[1]*?lut.size[2]*3);
    GenerateIdentityLut3D(&lut.lut[0], lut.size[0], 3, OCIO::LUT3DORDER_FAST_RED);

    std::vector<float> img;
    int xres = 2048;
    int yres = 1;
    int channels = 4;
    img.resize(xres*yres*channels);

    srand48(0);

    // create random values from -0.05 to 1.05
    // (To simulate clipping performance)

    for(unsigned int i=0; i<img.size(); ++i)
    {
    float uniform = (float)drand48();
    img[i] = uniform*1.1f - 0.05f;
    }

    timeval t;
    gettimeofday(&t, 0);
    double starttime = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;

    int numloops = 1024;
    for(int i=0; i<numloops; ++i)
    {
    ?//OCIO::Lut3D_Nearest(&img[0], xres*yres, lut);
    OCIO::Lut3D_Linear(&img[0], xres*yres, lut);
    }

    gettimeofday(&t, 0);
    double endtime = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
    double totaltime_a = (endtime-starttime)/numloops;

    printf("Linear: ?%0.1f ms  - ?%0.1f fps\n", totaltime_a*1000.0, 1.0/totaltime_a);


    // Tetrahedral
    gettimeofday(&t, 0);
    starttime = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;

    for(int i=0; i<numloops; ++i)
    {
    OCIO::Lut3D_Tetrahedral(&img[0], xres*yres, lut);
    }

    gettimeofday(&t, 0);
    endtime = (double) t.tv_sec + (double) t.tv_usec / 1000000.0;
    double totaltime_b = (endtime-starttime)/numloops;

    printf("Tetra: ?%0.1f ms  - ?%0.1f fps\n", totaltime_b*1000.0, 1.0/totaltime_b);

    double speed_diff = totaltime_a/totaltime_b;
    printf("Tetra is ?%.04f speed of Linear\n", speed_diff);
}
*/

OCIO_ADD_TEST(GenerateIdentityLut3D, throw_lut)
{
    const int lutSize = 3;
    std::vector<float> lut(lutSize * lutSize * lutSize * 3);

    OCIO_CHECK_THROW_WHAT(GenerateIdentityLut3D(
        lut.data(), lutSize, 2, OCIO::LUT3DORDER_FAST_RED),
        OCIO::Exception, "less than 3 channels");

    // GenerateIdentityLut3D with unknown order.
    OCIO_CHECK_THROW_WHAT(GenerateIdentityLut3D(
        lut.data(), lutSize, 3, (OCIO::Lut3DOrder)42),
        OCIO::Exception, "Unknown Lut3DOrder");

    // Get3DLutEdgeLenFromNumPixels with not cubic size.
    OCIO_CHECK_THROW_WHAT(OCIO::Get3DLutEdgeLenFromNumPixels(10),
        OCIO::Exception, "Cannot infer 3D LUT size");
}

OCIO_ADD_TEST(Lut3DOpData, create_op)
{
    OCIO::Lut3DOpDataRcPtr lut = std::make_shared<OCIO::Lut3DOpData>(3);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    // Inverse is fine.
    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));
    ops.clear();

    // Only valid direction.
    OCIO_CHECK_THROW_WHAT(CreateLut3DOp(ops, lut, OCIO::TRANSFORM_DIR_UNKNOWN),
                          OCIO::Exception,
                          "unspecified transform direction");
}

OCIO_ADD_TEST(Lut3DOp, cache_id)
{
    OCIO::OpRcPtrVec ops;
    for (int i = 0; i < 2; ++i)
    {
        OCIO::Lut3DOpDataRcPtr lut = std::make_shared<OCIO::Lut3DOpData>(3);
        OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::TRANSFORM_DIR_FORWARD));
    }

    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO_CHECK_NO_THROW(ops.validate());

    std::string cacheID0, cacheID1;
    OCIO_CHECK_NO_THROW(cacheID0 = ops[0]->getCacheID());
    OCIO_CHECK_NO_THROW(cacheID1 = ops[1]->getCacheID());
    OCIO_CHECK_ASSERT(!cacheID0.empty());
    // Identical LUTs have the same cacheID.
    OCIO_CHECK_EQUAL(cacheID0, cacheID1);
}

OCIO_ADD_TEST(Lut3DOp, edge_len_from_num_pixels)
{
    OCIO_CHECK_THROW_WHAT(OCIO::Get3DLutEdgeLenFromNumPixels(10),
                          OCIO::Exception, "Cannot infer 3D LUT size");
    int expectedRes = 33;
    int res = 0;
    OCIO_CHECK_NO_THROW(res = OCIO::Get3DLutEdgeLenFromNumPixels(
        expectedRes*expectedRes*expectedRes));
    OCIO_CHECK_EQUAL(res, expectedRes);

    expectedRes = 1290; // Maximum value such that v^3 is still an int.
    OCIO_CHECK_NO_THROW(res = OCIO::Get3DLutEdgeLenFromNumPixels(
        expectedRes*expectedRes*expectedRes));
    OCIO_CHECK_EQUAL(res, expectedRes);
}

OCIO_ADD_TEST(Lut3DOpStruct, lut3d_order)
{
    const int lutSize = 3;
    std::vector<float> lut(lutSize * lutSize * lutSize * 3);

    OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        lut.data(), lutSize, 3, OCIO::LUT3DORDER_FAST_RED));

    // First 3 values have red changing.
    OCIO_CHECK_EQUAL(lut[0], 0.0f);
    OCIO_CHECK_EQUAL(lut[3], 0.5f);
    OCIO_CHECK_EQUAL(lut[6], 1.0f);
    // Blue is all 0.
    OCIO_CHECK_EQUAL(lut[2], 0.0f);
    OCIO_CHECK_EQUAL(lut[5], 0.0f);
    OCIO_CHECK_EQUAL(lut[8], 0.0f);
    // Last 3 values have red changing.
    OCIO_CHECK_EQUAL(lut[72], 0.0f);
    OCIO_CHECK_EQUAL(lut[75], 0.5f);
    OCIO_CHECK_EQUAL(lut[78], 1.0f);
    // Blue is all 1.
    OCIO_CHECK_EQUAL(lut[74], 1.0f);
    OCIO_CHECK_EQUAL(lut[77], 1.0f);
    OCIO_CHECK_EQUAL(lut[80], 1.0f);

    OCIO_CHECK_NO_THROW(GenerateIdentityLut3D(
        lut.data(), lutSize, 3, OCIO::LUT3DORDER_FAST_BLUE));

    // First 3 values have blue changing.
    OCIO_CHECK_EQUAL(lut[2], 0.0f);
    OCIO_CHECK_EQUAL(lut[5], 0.5f);
    OCIO_CHECK_EQUAL(lut[8], 1.0f);
    // Red is all 0.
    OCIO_CHECK_EQUAL(lut[0], 0.0f);
    OCIO_CHECK_EQUAL(lut[3], 0.0f);
    OCIO_CHECK_EQUAL(lut[6], 0.0f);
    // Last 3 values have blue changing.
    OCIO_CHECK_EQUAL(lut[74], 0.0f);
    OCIO_CHECK_EQUAL(lut[77], 0.5f);
    OCIO_CHECK_EQUAL(lut[80], 1.0f);
    // Red is all 1.
    OCIO_CHECK_EQUAL(lut[72], 1.0f);
    OCIO_CHECK_EQUAL(lut[75], 1.0f);
    OCIO_CHECK_EQUAL(lut[78], 1.0f);

}

OCIO_ADD_TEST(Lut3DOpData, lut_order)
{
    OCIO::Lut3DOpDataRcPtr pLB = std::make_shared<OCIO::Lut3DOpData>(3);

    // First 3 values have blue changing.
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[2], 0.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[5], 0.5f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[8], 1.0f);
    // Red is all 0.
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[0], 0.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[3], 0.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[6], 0.0f);
    // Last 3 values have blue changing.
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[74], 0.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[77], 0.5f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[80], 1.0f);
    // Red is all 1.
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[72], 1.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[75], 1.0f);
    OCIO_CHECK_EQUAL(pLB->getArray().getValues()[78], 1.0f);
}

OCIO_ADD_TEST(Lut3DOpData, lut_combine)
{
    OCIO::Lut3DOpDataRcPtr lutData1 = std::make_shared<OCIO::Lut3DOpData>(3);
    OCIO::Lut3DOpDataRcPtr lutData2 = std::make_shared<OCIO::Lut3DOpData>(5);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateLut3DOp(ops, lutData1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateLut3DOp(ops, lutData2, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(OCIO::CreateLut3DOp(ops, lutData1, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(OCIO::CreateLut3DOp(ops, lutData2, OCIO::TRANSFORM_DIR_INVERSE));
    const double offset[] = { 1.1, -1.3, 0.3, -1.0 };
    OCIO_CHECK_NO_THROW(OCIO::CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 5);

    OCIO::ConstOpRcPtr lutFwd1 = ops[0];
    OCIO::ConstOpRcPtr lutFwd2 = ops[1];
    OCIO::ConstOpRcPtr lutInv1 = ops[2];
    OCIO::ConstOpRcPtr lutInv2 = ops[3];
    OCIO::ConstOpRcPtr mat = ops[4];

    // LUT 3D can combine with other LUT 3D.
    OCIO_CHECK_ASSERT(lutFwd1->canCombineWith(lutFwd2));
    OCIO_CHECK_ASSERT(lutFwd1->canCombineWith(lutInv1));
    OCIO_CHECK_ASSERT(lutInv1->canCombineWith(lutInv2));
    OCIO_CHECK_ASSERT(lutInv1->canCombineWith(lutFwd1));

    // LUT 3D can't combine with other ops (like matrix).
    OCIO_CHECK_ASSERT(!lutFwd1->canCombineWith(mat));
    OCIO_CHECK_ASSERT(!lutInv1->canCombineWith(mat));
    OCIO_CHECK_ASSERT(!mat->canCombineWith(lutFwd1));
    OCIO_CHECK_ASSERT(!mat->canCombineWith(lutInv1));
}

OCIO_ADD_TEST(Lut3DOp, cpu_renderer_lut3d)
{
    // By default, this constructor creates an 'identity LUT'.
    OCIO::Lut3DOpDataRcPtr
        lutData = std::make_shared<OCIO::Lut3DOpData>(
            OCIO::INTERP_LINEAR, 33);

    OCIO::Lut3DOp lut(lutData);

    OCIO_CHECK_NO_THROW(lut.validate());
    OCIO_CHECK_NO_THROW(lut.finalize());
    OCIO_CHECK_ASSERT(!lutData->isIdentity());
    OCIO_CHECK_ASSERT(!lut.isNoOp());

    // Use an input value exactly at a grid point so the output value is 
    // just the grid value, regardless of interpolation.
    const float step = 1.0f / ((float)lutData->getArray().getLength() - 1.0f);

    float myImage[8] = { 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, step, 1.0f };

    {
        OCIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

        OCIO_CHECK_EQUAL(myImage[0], 0.0f);
        OCIO_CHECK_EQUAL(myImage[1], 0.0f);
        OCIO_CHECK_EQUAL(myImage[2], 0.0f);
        OCIO_CHECK_EQUAL(myImage[3], 0.0f);

        OCIO_CHECK_EQUAL(myImage[4], 0.0f);
        OCIO_CHECK_EQUAL(myImage[5], 0.0f);
        OCIO_CHECK_EQUAL(myImage[6], step);
        OCIO_CHECK_EQUAL(myImage[7], 1.0f);
    }

    // No more an 'identity LUT 3D'.
    const float arbitraryVal = 0.123456f;
    lutData->getArray()[5] = arbitraryVal;

    OCIO_CHECK_NO_THROW(lut.validate());
    OCIO_CHECK_NO_THROW(lut.finalize());
    OCIO_CHECK_ASSERT(!lutData->isIdentity());
    OCIO_CHECK_ASSERT(!lut.isNoOp());

    {
        OCIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

        OCIO_CHECK_EQUAL(myImage[0], 0.0f);
        OCIO_CHECK_EQUAL(myImage[1], 0.0f);
        OCIO_CHECK_EQUAL(myImage[2], 0.0f);
        OCIO_CHECK_EQUAL(myImage[3], 0.0f);

        OCIO_CHECK_EQUAL(myImage[4], 0.0f);
        OCIO_CHECK_EQUAL(myImage[5], 0.0f);
        OCIO_CHECK_EQUAL(myImage[6], arbitraryVal);
        OCIO_CHECK_EQUAL(myImage[7], 1.0f);
    }

    // Change interpolation.
    lutData->setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_NO_THROW(lut.validate());
    OCIO_CHECK_NO_THROW(lut.finalize());
    OCIO_CHECK_ASSERT(!lutData->isIdentity());
    OCIO_CHECK_ASSERT(!lut.isNoOp());
    myImage[6] = step;
    {
        OCIO_CHECK_NO_THROW(lut.apply(myImage, myImage, 2));

        OCIO_CHECK_EQUAL(myImage[0], 0.0f);
        OCIO_CHECK_EQUAL(myImage[1], 0.0f);
        OCIO_CHECK_EQUAL(myImage[2], 0.0f);
        OCIO_CHECK_EQUAL(myImage[3], 0.0f);

        OCIO_CHECK_EQUAL(myImage[4], 0.0f);
        OCIO_CHECK_EQUAL(myImage[5], 0.0f);
        OCIO_CHECK_EQUAL(myImage[6], arbitraryVal);
        OCIO_CHECK_EQUAL(myImage[7], 1.0f);
    }
}

OCIO_ADD_TEST(Lut3DOp, cpu_renderer_cloned)
{
    // The unit test validates the processing of cloned ops.

    const std::string fileName("clf/lut3d_17x17x17_10i_12i.clf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(2, ops.size());
    OCIO::ConstOpRcPtr op1 = ops[1];
    auto lutData = OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(op1->data());
    OCIO_CHECK_EQUAL(lutData->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_EQUAL(1, ops.size());

    auto op0 = OCIO::DynamicPtrCast<const OCIO::Lut3DOp>(ops[0]);
    OCIO_REQUIRE_ASSERT(op0);
    auto fwdLutData = OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(op0->data());
    OCIO_CHECK_EQUAL(fwdLutData->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OCIO::Lut3DOp * fwdLut = const_cast<OCIO::Lut3DOp *>(op0.get());

    auto fwdLutDataCloned = fwdLutData->clone();
    OCIO::Lut3DOp fwdLutCloned(fwdLutDataCloned);

    OCIO::OpRcPtr fwdLutClonedCloned = fwdLutCloned.clone();

    const float inImage[] = {
        0.1f, 0.25f, 0.7f, 0.f,
        0.66f, 0.25f, 0.81f, 0.5f,
        //0.18f, 0.80f, 0.45f, 1.f }; // This one is easier to get correct.
        0.18f, 0.99f, 0.45f, 1.f };   // Setting G way up on the s-curve is harder.

    float bufferImage[12];
    memcpy(bufferImage, inImage, 12 * sizeof(float));

    float bufferImageClone[12];
    memcpy(bufferImageClone, inImage, 12 * sizeof(float));

    float bufferImageClone2[12];
    memcpy(bufferImageClone2, inImage, 12 * sizeof(float));

    // Apply the forward LUT.
    OCIO_CHECK_NO_THROW(fwdLut->finalize());
    OCIO_CHECK_NO_THROW(fwdLut->apply(bufferImage, 3));

    // Apply the cloned forward LUT.
    OCIO_CHECK_NO_THROW(fwdLutCloned.finalize());
    OCIO_CHECK_NO_THROW(fwdLutCloned.apply(bufferImageClone, 3));

    // Apply the cloned cloned forward LUT.
    OCIO_CHECK_NO_THROW(fwdLutClonedCloned->finalize());
    OCIO_CHECK_NO_THROW(fwdLutClonedCloned->apply(bufferImageClone2, 3));

    // Validate that the cloned op produces the exact same results.
    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_EQUAL(bufferImageClone[i], bufferImage[i]);
        OCIO_CHECK_EQUAL(bufferImageClone2[i], bufferImage[i]);
    }
}

OCIO_ADD_TEST(Lut3DOp, cpu_renderer_inverse)
{
    // The unit test validates the processing of inversed ops.

    const std::string fileName("clf/lut3d_17x17x17_10i_12i.clf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_DEFAULT));
    // Exact LUT inversion.
    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));

    auto op0 = OCIO::DynamicPtrCast<const OCIO::Lut3DOp>(ops[0]);
    OCIO_REQUIRE_ASSERT(op0);
    auto fwdLutData = OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(op0->data());
    auto fwdLutDataCloned = fwdLutData->clone();
    // Inversion is based on tetrahedral interpolation, so need to make sure 
    // the forward evals are also tetrahedral.
    fwdLutDataCloned->setInterpolation(OCIO::INTERP_TETRAHEDRAL);

    OCIO::Lut3DOp fwdLut(fwdLutDataCloned);

    const float inImage[] = {
        0.1f, 0.25f, 0.7f, 0.f,
        0.66f, 0.25f, 0.81f, 0.5f,
        //0.18f, 0.80f, 0.45f, 1.f }; // This one is easier to get correct.
        0.18f, 0.99f, 0.45f, 1.f };   // Setting G way up on the s-curve is harder.

    float bufferImage[12];
    memcpy(bufferImage, inImage, 12 * sizeof(float));

    // Apply forward LUT.
    OCIO_CHECK_NO_THROW(fwdLut.validate());
    OCIO_CHECK_NO_THROW(fwdLut.finalize());
    OCIO_CHECK_NO_THROW(fwdLut.apply(bufferImage, 3));

    //
    // Step 1: Test that forward and inverse ops are producing
    //         the right results in EXACT mode.
    //

    float outImage1[12];
    memcpy(outImage1, bufferImage, 12 * sizeof(float));

    OCIO::Lut3DOpDataRcPtr invLutData = fwdLutDataCloned->inverse();
    OCIO::Lut3DOp invLut(invLutData);

    // Apply inverse LUT.
    OCIO_CHECK_NO_THROW(invLut.validate());
    OCIO_CHECK_NO_THROW(invLut.finalize());
    OCIO_CHECK_NO_THROW(invLut.apply(bufferImage, 3));

    // Need to do another forward apply.  This is due to precision issues.
    // Also, some LUTs have flat or virtually flat areas so the inverse is not unique.
    // The first inverse does not match the source, but the fact that it winds up
    // in the same place after another cycle verifies that it is as good an inverse
    // for this particular LUT as the original input.  In other words, when the
    // forward LUT has a small derivative, precision issues imply that there will
    // be a range of inverses which should be considered valid.
    OCIO_CHECK_NO_THROW(fwdLut.apply(bufferImage, 3));

    const float errorThreshold = 1e-6f;
    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_CLOSE(outImage1[i], bufferImage[i], errorThreshold);
    }

    //
    // Step 2: Repeat with inversion quality FAST, apply inverse LUT.
    //

    memcpy(bufferImage, outImage1, 12 * sizeof(float));

    OCIO::ConstLut3DOpDataRcPtr invLutDataConst = invLutData;
    OCIO::Lut3DOpDataRcPtr invLutDataFast = OCIO::MakeFastLut3DFromInverse(invLutDataConst);
    OCIO::Lut3DOp invLutFast(invLutDataFast);
    OCIO_CHECK_NO_THROW(invLutFast.validate());
    OCIO_CHECK_NO_THROW(invLutFast.finalize());
    OCIO_CHECK_NO_THROW(invLutFast.apply(bufferImage, 3));

    OCIO_CHECK_NO_THROW(fwdLut.apply(bufferImage, 3));

    // Note that, even more than with Lut1D, the FAST inv Lut3D renderer is not exact.
    // It is expected that a fairly loose tolerance must be used here.
    const float errorLoose = 0.015f;
    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_CLOSE(outImage1[i], bufferImage[i], errorLoose);
    }

    //
    // Step 3: Test clamping of large values in EXACT mode.
    // 
    // Note: No need to test FAST mode since the forward LUT eval clamps inputs
    //       to the input domain.
    //

    bufferImage[0] = 100.f;

    OCIO_CHECK_NO_THROW(invLut.validate());
    OCIO_CHECK_NO_THROW(invLut.finalize());
    OCIO_CHECK_NO_THROW(invLut.apply(bufferImage, 1));

    // This tests that extreme large values get inverted.
    // (If no inverse is found, apply() currently returns zeros.)
    OCIO_CHECK_ASSERT(bufferImage[0] > 0.5f);
}

OCIO_ADD_TEST(Lut3DOp, cpu_renderer_lut3d_with_nan)
{
    const std::string fileName("clf/lut3d_identity_12i_16f.clf");
    OCIO::OpRcPtrVec ops;

    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));
    OCIO_REQUIRE_EQUAL(1, ops.size());
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO_CHECK_EQUAL(op0->data()->getType(), OCIO::OpData::Lut3DType);

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    float myImage[]
        = { qnan, 0.25f, 0.25f, 0.0f,
            0.25f, qnan, 0.25f, 0.0f,
            0.25f, 0.25f, qnan, 0.0f,
            0.25f, 0.25f, 0.0f, qnan,
            0.5f, 0.5f,  0.5f,  0.0f };

    OCIO_CHECK_NO_THROW(op0->apply(myImage, 5));

    OCIO_CHECK_EQUAL(myImage[0], 0.0f);
    OCIO_CHECK_EQUAL(myImage[1], 0.25f);
    OCIO_CHECK_EQUAL(myImage[2], 0.25f);
    OCIO_CHECK_EQUAL(myImage[3], 0.0f);

    OCIO_CHECK_EQUAL(myImage[5], 0.0f);
    OCIO_CHECK_EQUAL(myImage[10], 0.0f);

    OCIO_CHECK_ASSERT(OCIO::IsNan(myImage[15]));

    OCIO_CHECK_EQUAL(myImage[16], 0.5f);
    OCIO_CHECK_EQUAL(myImage[17], 0.5f);
    OCIO_CHECK_EQUAL(myImage[18], 0.5f);
    OCIO_CHECK_EQUAL(myImage[19], 0.0f);
}

OCIO_ADD_TEST(Lut3D, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;

    OCIO::Lut3DOpDataRcPtr lut = std::make_shared<OCIO::Lut3DOpData>(3);

    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    lut->getArray()[39] = 0.61f;
    lut->getArray()[40] = 0.52f;
    lut->getArray()[41] = 0.74f;

    auto & metadataSource = lut->getFormatMetadata();
    metadataSource.addAttribute(OCIO::METADATA_NAME, "test");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateLut3DOp(ops, lut, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateLut3DTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto lTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut3DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lTransform);

    const auto & metadata = lTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), OCIO::METADATA_NAME);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(lTransform->getDirection(), direction);
    OCIO_REQUIRE_EQUAL(lTransform->getGridSize(), 3);

    OCIO_CHECK_EQUAL(lTransform->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    lTransform->getValue(1, 1, 1, r, g, b);

    OCIO_CHECK_EQUAL(r, 0.61f);
    OCIO_CHECK_EQUAL(g, 0.52f);
    OCIO_CHECK_EQUAL(b, 0.74f);
}

OCIO_ADD_TEST(Lut3DTransform, build_op)
{
    const OCIO::Lut3DTransformRcPtr lut = OCIO::Lut3DTransform::Create();
    const unsigned long gs = 4;
    lut->setGridSize(gs);

    const float r = 0.51f;
    const float g = 0.52f;
    const float b = 0.53f;

    const unsigned long ri = 1;
    const unsigned long gi = 2;
    const unsigned long bi = 3;
    lut->setValue(ri, gi, bi, r, g, b);

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::OpRcPtrVec ops;
    OCIO::BuildOps(ops, *(config.get()), config->getCurrentContext(), lut, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    auto constop = std::const_pointer_cast<const OCIO::Op>(ops[0]);
    OCIO_REQUIRE_ASSERT(constop);
    auto data = constop->data();
    auto lutdata = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Lut3DOpData>(data);
    OCIO_REQUIRE_ASSERT(lutdata);

    // Blue fast.
    const unsigned long i = 3 * ((ri*gs + gi)*gs + bi);
    OCIO_CHECK_EQUAL(lutdata->getArray().getLength(), gs);
    OCIO_CHECK_EQUAL(lutdata->getArray()[i], r);
    OCIO_CHECK_EQUAL(lutdata->getArray()[i + 1], g);
    OCIO_CHECK_EQUAL(lutdata->getArray()[i + 2], b);
}

// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Blue
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Green
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Red
// TODO: Port syncolor test: renderer\test\CPURenderer_cases.cpp_inc - CPURendererLUT3D_Example

