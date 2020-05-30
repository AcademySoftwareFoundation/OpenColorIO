// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>

#include "ops/lut1d/Lut1DOp.cpp"

#include "OpBuilders.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Lut1DOp, extrapolation_errors)
{
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(3);
    auto & lutArray = lut->getArray();

    // Simple y=x+0.1 LUT.
    for (int i = 0; i < 3; ++i)
    {
        for (int c = 0; c < 3; ++c)
        {
            lutArray[c + i * 3] += 0.1f;
        }
    }

    bool isIdentity = true;
    OCIO_CHECK_NO_THROW(isIdentity = lut->isNoOp());
    OCIO_CHECK_ASSERT(!isIdentity);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_FORWARD));

    const int PIXELS = 5;
    float inputBuffer_linearforward[PIXELS * 4] = {
        -0.1f,   -0.2f, -10.0f, 0.0f,
         0.5f,    1.0f,   1.1f, 0.0f,
        10.1f,   55.0f,   2.3f, 0.0f,
         9.1f,  1.0e6f, 1.0e9f, 0.0f,
        4.0e9f, 9.5e7f,   0.5f, 0.0f };
    const float outputBuffer_linearforward[PIXELS * 4] = {
        0.1f, 0.1f, 0.1f, 0.0f,
        0.6f, 1.1f, 1.1f, 0.0f,
        1.1f, 1.1f, 1.1f, 0.0f,
        1.1f, 1.1f, 1.1f, 0.0f,
        1.1f, 1.1f, 0.6f, 0.0f };

    ops[0]->apply(inputBuffer_linearforward, PIXELS);
    for (size_t i = 0; i < sizeof(inputBuffer_linearforward) / sizeof(inputBuffer_linearforward[0]); ++i)
    {
        OCIO_CHECK_CLOSE(inputBuffer_linearforward[i], outputBuffer_linearforward[i], 1e-5f);
    }
}

OCIO_ADD_TEST(Lut1DOp, inverse)
{
    OCIO::Lut1DOpDataRcPtr luta = std::make_shared<OCIO::Lut1DOpData>(3);
    auto & lutaArray = luta->getArray();
    lutaArray[0] = 0.1f;

    OCIO::Lut1DOpDataRcPtr lutb = luta->clone();
    OCIO::Lut1DOpDataRcPtr lutc = luta->clone();
    auto & lutcArray = lutc->getArray();
    lutcArray[0] = 0.2f;

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, luta, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, luta, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lutb, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lutb, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lutc, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lutc, OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_REQUIRE_EQUAL(ops.size(), 6);
    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];
    OCIO::ConstOpRcPtr op3 = ops[3];
    OCIO::ConstOpRcPtr op4 = ops[4];
    OCIO::ConstOpRcPtr op5 = ops[5];

    OCIO_CHECK_EQUAL(op0->isInverse(op1), true);
    OCIO_CHECK_EQUAL(op2->isInverse(op3), true);
    OCIO_CHECK_EQUAL(op4->isInverse(op5), true);

    OCIO_CHECK_EQUAL(op0->isInverse(op2), false);
    OCIO_CHECK_EQUAL(op0->isInverse(op3), true);
    OCIO_CHECK_EQUAL(op1->isInverse(op2), true);
    OCIO_CHECK_EQUAL(op1->isInverse(op3), false);

    OCIO_CHECK_EQUAL(op0->isInverse(op4), false);
    OCIO_CHECK_EQUAL(op0->isInverse(op5), false);
    OCIO_CHECK_EQUAL(op1->isInverse(op4), false);
    OCIO_CHECK_EQUAL(op1->isInverse(op5), false);

    std::string cacheID0, cacheID1, cacheID2, cacheID3, cacheID4, cacheID5;
    OCIO_CHECK_NO_THROW(cacheID0 = ops[0]->getCacheID());
    OCIO_CHECK_NO_THROW(cacheID1 = ops[1]->getCacheID());
    OCIO_CHECK_NO_THROW(cacheID2 = ops[2]->getCacheID());
    OCIO_CHECK_NO_THROW(cacheID3 = ops[3]->getCacheID());
    OCIO_CHECK_NO_THROW(cacheID4 = ops[4]->getCacheID());
    OCIO_CHECK_NO_THROW(cacheID5 = ops[5]->getCacheID());
    OCIO_CHECK_EQUAL(cacheID0, cacheID2);
    OCIO_CHECK_EQUAL(cacheID1, cacheID3);

    OCIO_CHECK_NE(cacheID0, cacheID4);
    OCIO_CHECK_NE(cacheID0, cacheID5);
    OCIO_CHECK_NE(cacheID1, cacheID4);
    OCIO_CHECK_NE(cacheID1, cacheID5);

    // Optimize will remove LUT forward and inverse (0+1, 2+3 and 4+5)
    // and replace them by a clamping range.
    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<RangeOp>");
}

namespace
{
OCIO::Lut1DOpDataRcPtr CreateSquareLut()
{
    // Make a LUT that squares the input.
    static constexpr int size = 256;
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(size);
    auto & lutArray = lut->getArray();

    for (int i = 0; i < size; ++i)
    {
        float x = (float)i / (float)(size - 1);
        float x2 = x*x;

        for (int c = 0; c < 3; ++c)
        {
            lutArray[c + i * 3] = x2;
        }
    }
    return lut;
}
}

OCIO_ADD_TEST(Lut1DOp, finite_value)
{
    OCIO::Lut1DOpDataRcPtr lut = CreateSquareLut();

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));

    float inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    const float outputBuffer_linearforward[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    ops[0]->apply(inputBuffer_linearforward, 1);
    for(int i=0; i <4; ++i)
    {
        OCIO_CHECK_CLOSE(inputBuffer_linearforward[i], outputBuffer_linearforward[i], 1e-5f);
    }

    const float inputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float outputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    ops[1]->apply(outputBuffer_linearinverse, 1);
    for(int i=0; i <4; ++i)
    {
        OCIO_CHECK_CLOSE(inputBuffer_linearinverse[i], outputBuffer_linearinverse[i], 1e-5f);
    }
}

OCIO_ADD_TEST(Lut1DOp, gpu)
{
    OCIO::Lut1DOpDataRcPtr lut = CreateSquareLut();
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->supportedByLegacyShader(), false);
}

OCIO_ADD_TEST(Lut1DOp, identity_lut_1d)
{
    int size = 3;
    int channels = 2;
    std::vector<float> data(size*channels);
    OCIO::GenerateIdentityLut1D(&data[0], size, channels);
    OCIO_CHECK_EQUAL(data[0], 0.0f);
    OCIO_CHECK_EQUAL(data[1], 0.0f);
    OCIO_CHECK_EQUAL(data[2], 0.5f);
    OCIO_CHECK_EQUAL(data[3], 0.5f);
    OCIO_CHECK_EQUAL(data[4], 1.0f);
    OCIO_CHECK_EQUAL(data[5], 1.0f);

    size = 4;
    channels = 3;
    data.resize(size*channels);
    OCIO::GenerateIdentityLut1D(&data[0], size, channels);
    for (int c = 0; c < channels; ++c)
    {
        OCIO_CHECK_EQUAL(data[0+c], 0.0f);
        OCIO_CHECK_EQUAL(data[channels+c], 0.33333333f);
        OCIO_CHECK_EQUAL(data[2*channels+c], 0.66666667f);
        OCIO_CHECK_EQUAL(data[3*channels+c], 1.0f);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, finite_value_hue_adjust)
{
    // Make a LUT that squares the input.
    OCIO::Lut1DOpDataRcPtr lutData = CreateSquareLut();
    lutData->setHueAdjust(OCIO::HUE_DW3);
    OCIO::Lut1DOp lut(lutData);

    OCIO_CHECK_NO_THROW(lutData->finalize());
    OCIO_CHECK_ASSERT(!lut.isIdentity());

    const float outputBuffer_linearforward[4] = { 0.25f,
                                                  0.37000f, // (Hue adj modifies green here.)
                                                  0.49f,
                                                  0.5f };
    float lut1d_inputBuffer_linearforward[4] = { 0.5f, 0.6f, 0.7f, 0.5f };

    OCIO_CHECK_NO_THROW(lut.apply(lut1d_inputBuffer_linearforward, 1));
    for (int i = 0; i <4; ++i)
    {
        OCIO_CHECK_CLOSE(lut1d_inputBuffer_linearforward[i],
                         outputBuffer_linearforward[i], 1e-5f);
    }

    OCIO::Lut1DOpDataRcPtr invData = lutData->inverse();
    OCIO::Lut1DOpDataRcPtr invDataExact = invData->clone();

    OCIO::OpRcPtrVec opsFast, opsExact;
    OCIO_CHECK_NO_THROW(
        CreateLut1DOp(opsFast, invData, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(
        CreateLut1DOp(opsExact, invDataExact, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(opsFast.size(), 1);
    OCIO_REQUIRE_EQUAL(opsExact.size(), 1);

    const float inputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };
    float lut1d_outputBuffer_linearinverse[4] = { 0.25f, 0.37f, 0.49f, 0.5f };
    float lut1d_outputBuffer_linearinverseEx[4] = { 0.25f, 0.37f, 0.49f, 0.5f };

    OCIO_CHECK_NO_THROW(opsFast.validate());
    OCIO_CHECK_NO_THROW(opsExact.validate());

    OCIO_CHECK_NO_THROW(opsFast.finalize(OCIO::OPTIMIZATION_LUT_INV_FAST));
    OCIO_CHECK_NO_THROW(opsExact.finalize(OCIO::OPTIMIZATION_NONE));

    OCIO_REQUIRE_EQUAL(opsFast.size(), 1);
    OCIO_REQUIRE_EQUAL(opsExact.size(), 1);

    auto op = std::const_pointer_cast<const OCIO::Op>(opsFast[0]);
    auto opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lutFast = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OCIO_REQUIRE_ASSERT(lutFast);
    OCIO_CHECK_EQUAL(lutFast->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    op = std::const_pointer_cast<const OCIO::Op>(opsExact[0]);
    opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lutExact = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OCIO_REQUIRE_ASSERT(lutExact);
    OCIO_CHECK_EQUAL(lutExact->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(opsFast[0]->apply(lut1d_outputBuffer_linearinverse, 1)); // fast
    OCIO_CHECK_NO_THROW(opsExact[0]->apply(lut1d_outputBuffer_linearinverseEx, 1)); // exact
    for (int i = 0; i <4; ++i)
    {
        OCIO_CHECK_CLOSE(lut1d_outputBuffer_linearinverse[i],
                         inputBuffer_linearinverse[i], 1e-5f);
        OCIO_CHECK_CLOSE(lut1d_outputBuffer_linearinverseEx[i],
                         inputBuffer_linearinverse[i], 1e-5f);
    }
}

OCIO_ADD_TEST(Lut1D, lut_1d_compose_with_bit_depth)
{
    const std::string ctfFile("clf/lut1d_comp.clf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, ctfFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut1 = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OCIO_REQUIRE_ASSERT(lut1);
    op = std::const_pointer_cast<const OCIO::Op>(ops[2]);
    opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut2 = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);

    {
        OCIO::Lut1DOpDataRcPtr lutComposed;
        OCIO_CHECK_NO_THROW(lutComposed =
            OCIO::Lut1DOpData::Compose(lut1, lut2, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO));

        const float error = 1e-5f;
        OCIO_CHECK_EQUAL(lutComposed->getArray().getLength(), 2);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[0], 0.00744791f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[1], 0.03172233f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[2], 0.07058375f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[3], 0.3513808f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[4], 0.51819527f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[5], 0.67463773f, error);
    }
    {
        OCIO::Lut1DOpDataRcPtr lutComposed;
        OCIO_CHECK_NO_THROW(lutComposed =
            OCIO::Lut1DOpData::Compose(lut1, lut2, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_BIG));

        const float error = 1e-5f;
        OCIO_CHECK_EQUAL(lutComposed->getArray().getLength(), 65536);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[0], 0.00744791f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[1], 0.03172233f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[2], 0.07058375f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[98688], 0.09914176f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[98689], 0.1866852f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[98690], 0.2830042f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[196605], 0.3513808f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[196606], 0.51819527f, error);
        OCIO_CHECK_CLOSE(lutComposed->getArray().getValues()[196607], 0.67463773f, error);
    }
}

OCIO_ADD_TEST(Lut1DOpData, compose_only_forward)
{
    OCIO::Lut1DOpDataRcPtr l1 = CreateSquareLut();

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, l1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, l1, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, l1, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, l1, OCIO::TRANSFORM_DIR_INVERSE));

    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO::ConstOpRcPtr l1f = ops[1];
    OCIO::ConstOpRcPtr l1b = ops[3];

    // Forward + forward.
    OCIO_CHECK_ASSERT(ops[0]->canCombineWith(l1f));
    // Inverse + inverse.
    OCIO_CHECK_ASSERT(ops[2]->canCombineWith(l1b));
    // Forward + Inverse.
    OCIO_CHECK_ASSERT(ops[0]->canCombineWith(l1b));
    // Inverse + forward.
    OCIO_CHECK_ASSERT(ops[2]->canCombineWith(l1f));
}

OCIO_ADD_TEST(Lut1D, compose_big_domain)
{
    auto lut1 = std::make_shared<OCIO::Lut1DOpData>(10);
    auto lut2 = std::make_shared<OCIO::Lut1DOpData>(10);
    lut1->getArray()[9 * 3] = 1.0001f;

    OCIO::OpRcPtrVec ops;
    OCIO::CreateLut1DOp(ops, lut1, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateLut1DOp(ops, lut2, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    op0->combineWith(ops, op1);
    OCIO_REQUIRE_EQUAL(ops.size(), 3);

    OCIO::ConstOpRcPtr op2 = ops[2];
    auto lut = OCIO::DynamicPtrCast<const OCIO::Lut1DOpData>(op2->data());
    OCIO_REQUIRE_ASSERT(lut);
    OCIO_CHECK_EQUAL(lut->getArray().getLength(), 65536);
    OCIO_CHECK_ASSERT(!lut->isInputHalfDomain());
}

OCIO_ADD_TEST(Lut1D, inverse_twice)
{
    // Make a LUT that squares the input.
    OCIO::Lut1DOpDataRcPtr lut = CreateSquareLut();

    const float outputBuffer_linearinverse[4] = { 0.5f, 0.6f, 0.7f, 0.5f };

    // Create inverse lut.
    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut1DOp(ops, lut, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    const float lut1d_inputBuffer_reference[4] = { 0.25f, 0.36f, 0.49f, 0.5f };
    float lut1d_inputBuffer_linearinverse[4] = { 0.25f, 0.36f, 0.49f, 0.5f };

    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));
    OCIO_CHECK_NO_THROW(ops[0]->apply(lut1d_inputBuffer_linearinverse, 1));
    for (int i = 0; i < 4; ++i)
    {
        OCIO_CHECK_CLOSE(lut1d_inputBuffer_linearinverse[i],
                         outputBuffer_linearinverse[i], 1e-5f);
    }

    // Inverse the inverse.
    OCIO::Lut1DOp * pLut = dynamic_cast<OCIO::Lut1DOp*>(ops[0].get());
    OCIO_CHECK_ASSERT(pLut);
    OCIO::Lut1DOpDataRcPtr lutData = pLut->lut1DData()->inverse();
    OCIO_CHECK_NO_THROW(OCIO::CreateLut1DOp(ops, lutData,
                                            OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    // Apply the inverse.
    OCIO_CHECK_NO_THROW(ops.validate());
    OCIO_CHECK_NO_THROW(ops.finalize(OCIO::OPTIMIZATION_NONE));
    OCIO_CHECK_NO_THROW(ops[1]->apply(lut1d_inputBuffer_linearinverse, 1));

    // Verify we are back on the input.
    for (int i = 0; i < 4; ++i)
    {
        OCIO_CHECK_CLOSE(lut1d_inputBuffer_linearinverse[i],
                         lut1d_inputBuffer_reference[i], 1e-5f);
    }
}

OCIO_ADD_TEST(Lut1D, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;

    auto lut = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 3);
    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    lut->getArray()[3] = 0.51f;
    lut->getArray()[4] = 0.52f;
    lut->getArray()[5] = 0.53f;

    auto & metadataSource = lut->getFormatMetadata();
    metadataSource.addAttribute(OCIO::METADATA_NAME, "test");

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateLut1DOp(ops, lut, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateLut1DTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto lTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut1DTransform>(transform);
    OCIO_REQUIRE_ASSERT(lTransform);

    const auto & metadata = lTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), OCIO::METADATA_NAME);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(lTransform->getDirection(), direction);
    OCIO_REQUIRE_EQUAL(lTransform->getLength(), 3);

    OCIO_CHECK_EQUAL(lTransform->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    lTransform->getValue(1, r, g, b);

    OCIO_CHECK_EQUAL(r , 0.51f);
    OCIO_CHECK_EQUAL(g , 0.52f);
    OCIO_CHECK_EQUAL(b , 0.53f);
}

OCIO_ADD_TEST(Lut1DTransform, build_op)
{
    const OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();
    lut->setLength(3);

    const float r = 0.51f;
    const float g = 0.52f;
    const float b = 0.53f;
    lut->setValue(1, r, g, b);

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::OpRcPtrVec ops;
    OCIO::BuildOps(ops, *(config.get()), config->getCurrentContext(), lut, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    auto constop = std::const_pointer_cast<const OCIO::Op>(ops[0]);
    OCIO_REQUIRE_ASSERT(constop);
    auto data = constop->data();
    auto lutdata = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Lut1DOpData>(data);
    OCIO_REQUIRE_ASSERT(lutdata);

    OCIO_CHECK_EQUAL(lutdata->getArray().getLength(), 3);
    OCIO_CHECK_EQUAL(lutdata->getArray()[3], r);
    OCIO_CHECK_EQUAL(lutdata->getArray()[4], g);
    OCIO_CHECK_EQUAL(lutdata->getArray()[5], b);
}

