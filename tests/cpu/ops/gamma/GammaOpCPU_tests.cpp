// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gamma/GammaOpCPU.cpp"

#include "MathUtils.h"
#include "ParseUtils.h"
#include "ops/gamma/GammaOp.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
void ApplyGamma(const OCIO::OpRcPtr & op, 
                float * image, const float * result,
                long numPixels, unsigned line,
                float errorThreshold)
{
    const auto cpu = op->getCPUOp(true);

    OCIO_CHECK_NO_THROW_FROM(cpu->apply(image, image, numPixels), line);

    for(long idx=0; idx<(numPixels*4); ++idx)
    {
        if (OCIO::IsNan(result[idx]))
        {
            OCIO_CHECK_ASSERT_FROM(OCIO::IsNan(image[idx]), line);
            continue;
        }
        // Using rel error with a large minExpected value of 1 will transition
        // from absolute error for expected values < 1 and
        // relative error for values > 1.
        const bool equalRel = OCIO::EqualWithSafeRelError(image[idx], result[idx],
                                                          errorThreshold, 1.0f);
        if (!equalRel)
        {
            // As most of the error thresholds are 1e-7f, the output 
            // value precision should then be bigger than 7 digits 
            // to highlight small differences. 
            std::ostringstream message;
            message.precision(9);
            message << "Index: " << idx
                    << " - Values: " << image[idx]
                    << " and: " << result[idx]
                    << " - Threshold: " << errorThreshold;
            OCIO_CHECK_ASSERT_MESSAGE_FROM(0, message.str(), line);
        }
    }
}

constexpr float qnan = std::numeric_limits<float>::quiet_NaN();
constexpr float inf = std::numeric_limits<float>::infinity();

};

OCIO_ADD_TEST(GammaOpCPU, apply_basic_style_fwd)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 7;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f,
        -inf,      inf,      qnan,       0.0f}; 

    // Including a gamma of 1.0 because OCIO v1 did not clamp negatives in that case.
    // In OCIO v2, the behavior  does *not* depend on the gamma.
    const std::vector<double> gammaVals = { 1.2, 2.12, 1., 1.05 };

#if OCIO_USE_SSE2
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,       0.0f,
        0.0f,        0.0f,        0.00005f,   0.48297336f,
        0.00010933f, 0.00001323f, 0.0499999f, 0.73928129f,
        0.18946611f, 0.23005184f, 0.7499921f, 1.00001204f,
        0.76507961f, 0.89695119f, 1.0000116f, 1.53070319f,
        1.00601125f, 1.10895324f, 1.4999843f, 0.0f,
        0.0f,        inf,         0.0f,       0.0f};
#else
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,     0.0f,
        0.0f,        0.0f,        0.00005f, 0.48296818f,
        0.00010933f, 0.00001323f, 0.05f,    0.73928916f,
        powf(input_32f[12], (float)gammaVals[0]),
        powf(input_32f[13], (float)gammaVals[1]),
        powf(input_32f[14], (float)gammaVals[2]),
        powf(input_32f[15], (float)gammaVals[3]),
        powf(input_32f[16], (float)gammaVals[0]),
        powf(input_32f[17], (float)gammaVals[1]),
        powf(input_32f[18], (float)gammaVals[2]),
        powf(input_32f[19], (float)gammaVals[3]),
        1.00600302f, 1.10897374f, 1.5f,  0.0f,
        0.0f,        inf,         0.0f,  0.0f};
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams   = { gammaVals[0] };
    const OCIO::GammaOpData::Params greenParams = { gammaVals[1] };
    const OCIO::GammaOpData::Params blueParams  = { gammaVals[2] };
    const OCIO::GammaOpData::Params alphaParams = { gammaVals[3] };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_FWD,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, __LINE__, errorThreshold);
}

OCIO_ADD_TEST(GammaOpCPU, apply_basic_style_rev)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 7;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f,
        -inf,      inf,      qnan,       0.0f };

    const std::vector<double> gammaVals = { 1.2, 2.12, 1.123, 1.05 };

#if OCIO_USE_SSE2
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,        0.0f,
        0.0f,        0.0f,        0.00014792f, 0.51678240f,
        0.00177476f, 0.08215060f, 0.06941742f, 0.76033723f, 
        0.31498342f, 0.72111737f, 0.77400052f, 1.00001109f, 
        0.83031141f, 0.97609287f, 1.00001061f, 1.47130167f, 
        1.00417137f, 1.02327621f, 1.43483067f, 0.0f,
        0.0f,        1.49761057e+18f, 0.0f,    0.0f };
#else
    const float expected_32f[numPixels*4] = {
        0.0f,        0.0f,        0.0f,        0.0f,
        0.0f,        0.0f,        0.00014792f, 0.51677888f,
        0.00177476f, 0.08215017f, 0.06941755f, 0.76034504f,
        powf(input_32f[12], (float)(1./gammaVals[0])),
        powf(input_32f[13], (float)(1./gammaVals[1])),
        powf(input_32f[14], (float)(1./gammaVals[2])),
        powf(input_32f[15], (float)(1./gammaVals[3])),
        powf(input_32f[16], (float)(1./gammaVals[0])),
        powf(input_32f[17], (float)(1./gammaVals[1])),
        powf(input_32f[18], (float)(1./gammaVals[2])),
        powf(input_32f[19], (float)(1./gammaVals[3])),
        1.00416493f, 1.02328109f, 1.43484282f, 0.0f,
        0.0f,        inf,         0.0f,        0.0f};
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams   = { gammaVals[0] };
    const OCIO::GammaOpData::Params greenParams = { gammaVals[1] };
    const OCIO::GammaOpData::Params blueParams  = { gammaVals[2] };
    const OCIO::GammaOpData::Params alphaParams = { gammaVals[3] };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_REV,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, __LINE__, errorThreshold);
}

OCIO_ADD_TEST(GammaOpCPU, apply_basic_mirror_style_fwd)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 9;

    float input_32f[numPixels * 4] = {
         0.0005f,  0.005f,  0.05f,  0.75f,
        -0.0005f, -0.005f, -0.05f, -0.75f,
         0.25f,    0.5f,    0.75f,  1.0f,
        -0.25f,   -0.5f,   -0.75f, -1.0f,
         0.80f,    0.95f,   1.0f,   1.5f,
        -0.80f,   -0.95f,  -1.0f,  -1.5f,
         1.005f,   1.05f,   1.5f,   0.25f,
        -1.005f,  -1.05f,  -1.5f,  -0.25f,
        -inf,      inf,     qnan,   0.0f };

    const std::vector<double> gammaVals = { 1.2, 2.12, 1.123, 1.05 };

#if OCIO_USE_SSE2
    const float expected_32f[numPixels * 4] = {
         0.00010933f,  0.00001323f,  0.03458935f,  0.73928129f,
        -0.00010933f, -0.00001323f, -0.03458935f, -0.73928129f,
         0.18946611f,  0.23005184f,  0.72391760f,  1.00001204f,
        -0.18946611f, -0.23005184f, -0.72391760f, -1.00001204f,
         0.76507961f,  0.89695119f,  1.00001264f,  1.53070319f,
        -0.76507961f, -0.89695119f, -1.00001264f, -1.53070319f,
         1.00601125f,  1.10895324f,  1.57668686f,  0.23326106f,
        -1.00601125f, -1.10895324f, -1.57668686f, -0.23326106f,
        -inf,          inf,          0.0f,         0.0f };

#else
    const float expected_32f[numPixels * 4] = {
         powf(input_32f[0], (float)gammaVals[0]),
         powf(input_32f[1], (float)gammaVals[1]),
         powf(input_32f[2], (float)gammaVals[2]),
         powf(input_32f[3], (float)gammaVals[3]),
        -powf(input_32f[0], (float)gammaVals[0]),
        -powf(input_32f[1], (float)gammaVals[1]),
        -powf(input_32f[2], (float)gammaVals[2]),
        -powf(input_32f[3], (float)gammaVals[3]),

         powf(input_32f[8], (float)gammaVals[0]),
         powf(input_32f[9], (float)gammaVals[1]),
         powf(input_32f[10], (float)gammaVals[2]),
         powf(input_32f[11], (float)gammaVals[3]),
        -powf(input_32f[8], (float)gammaVals[0]),
        -powf(input_32f[9], (float)gammaVals[1]),
        -powf(input_32f[10], (float)gammaVals[2]),
        -powf(input_32f[11], (float)gammaVals[3]),

         powf(input_32f[16], (float)gammaVals[0]),
         powf(input_32f[17], (float)gammaVals[1]),
         powf(input_32f[18], (float)gammaVals[2]),
         powf(input_32f[19], (float)gammaVals[3]),
        -powf(input_32f[16], (float)gammaVals[0]),
        -powf(input_32f[17], (float)gammaVals[1]),
        -powf(input_32f[18], (float)gammaVals[2]),
        -powf(input_32f[19], (float)gammaVals[3]),

         powf(input_32f[24], (float)gammaVals[0]),
         powf(input_32f[25], (float)gammaVals[1]),
         powf(input_32f[26], (float)gammaVals[2]),
         powf(input_32f[27], (float)gammaVals[3]),
        -powf(input_32f[24], (float)gammaVals[0]),
        -powf(input_32f[25], (float)gammaVals[1]),
        -powf(input_32f[26], (float)gammaVals[2]),
        -powf(input_32f[27], (float)gammaVals[3]),
        -inf, inf, qnan, 0.0f};
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams = { gammaVals[0] };
    const OCIO::GammaOpData::Params greenParams = { gammaVals[1] };
    const OCIO::GammaOpData::Params blueParams = { gammaVals[2] };
    const OCIO::GammaOpData::Params alphaParams = { gammaVals[3] };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_MIRROR_FWD,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, __LINE__, errorThreshold);
}

OCIO_ADD_TEST(GammaOpCPU, apply_basic_mirror_style_rev)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 9;

    float input_32f[numPixels * 4] = {
         0.0005f,  0.005f,  0.05f,  0.75f,
        -0.0005f, -0.005f, -0.05f, -0.75f,
         0.25f,    0.5f,    0.75f,  1.0f,
        -0.25f,   -0.5f,   -0.75f, -1.0f,
         0.80f,    0.95f,   1.0f,   1.5f,
        -0.80f,   -0.95f,  -1.0f,  -1.5f,
         1.005f,   1.05f,   1.5f,   0.25f,
        -1.005f,  -1.05f,  -1.5f,  -0.25f,
        -inf,      inf,     qnan,   0.0f };

    const std::vector<double> gammaVals = { 1.2, 2.12, 1.123, 1.05 };

#if OCIO_USE_SSE2
    const float expected_32f[numPixels * 4] = {
         0.00177476f,  0.08215060f,  0.06941742f,  0.76033723f,
        -0.00177476f, -0.08215060f, -0.06941742f, -0.76033723f,
         0.31498342f,  0.72111737f,  0.77400052f,  1.00001109f,
        -0.31498342f, -0.72111737f, -0.77400052f, -1.00001109f,
         0.83031141f,  0.97609287f,  1.00001061f,  1.47130167f,
        -0.83031141f, -0.97609287f, -1.00001061f, -1.47130167f,
         1.00417137f,  1.02327621f,  1.43483067f,  0.26706201f,
        -1.00417137f, -1.02327621f, -1.43483067f, -0.26706201f,
        -1.28786104e+32f, 1.49761057e+18f, 0.0f,   0.0f };
#else
    const float expected_32f[numPixels * 4] = {
         powf(input_32f[0], (float)(1./ gammaVals[0])),
         powf(input_32f[1], (float)(1./ gammaVals[1])),
         powf(input_32f[2], (float)(1./ gammaVals[2])),
         powf(input_32f[3], (float)(1./ gammaVals[3])),
        -powf(input_32f[0], (float)(1./ gammaVals[0])),
        -powf(input_32f[1], (float)(1./ gammaVals[1])),
        -powf(input_32f[2], (float)(1./ gammaVals[2])),
        -powf(input_32f[3], (float)(1./ gammaVals[3])),

         powf(input_32f[8],  (float)(1. / gammaVals[0])),
         powf(input_32f[9],  (float)(1. / gammaVals[1])),
         powf(input_32f[10], (float)(1. / gammaVals[2])),
         powf(input_32f[11], (float)(1. / gammaVals[3])),
        -powf(input_32f[8],  (float)(1. / gammaVals[0])),
        -powf(input_32f[9],  (float)(1. / gammaVals[1])),
        -powf(input_32f[10], (float)(1. / gammaVals[2])),
        -powf(input_32f[11], (float)(1. / gammaVals[3])),

         powf(input_32f[16], (float)(1. / gammaVals[0])),
         powf(input_32f[17], (float)(1. / gammaVals[1])),
         powf(input_32f[18], (float)(1. / gammaVals[2])),
         powf(input_32f[19], (float)(1. / gammaVals[3])),
        -powf(input_32f[16], (float)(1. / gammaVals[0])),
        -powf(input_32f[17], (float)(1. / gammaVals[1])),
        -powf(input_32f[18], (float)(1. / gammaVals[2])),
        -powf(input_32f[19], (float)(1. / gammaVals[3])),

         powf(input_32f[24], (float)(1. / gammaVals[0])),
         powf(input_32f[25], (float)(1. / gammaVals[1])),
         powf(input_32f[26], (float)(1. / gammaVals[2])),
         powf(input_32f[27], (float)(1. / gammaVals[3])),
        -powf(input_32f[24], (float)(1. / gammaVals[0])),
        -powf(input_32f[25], (float)(1. / gammaVals[1])),
        -powf(input_32f[26], (float)(1. / gammaVals[2])),
        -powf(input_32f[27], (float)(1. / gammaVals[3])),
        -inf, inf, qnan, 0.0f };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams = { gammaVals[0] };
    const OCIO::GammaOpData::Params greenParams = { gammaVals[1] };
    const OCIO::GammaOpData::Params blueParams = { gammaVals[2] };
    const OCIO::GammaOpData::Params alphaParams = { gammaVals[3] };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_MIRROR_REV,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, __LINE__, errorThreshold);
}

OCIO_ADD_TEST(GammaOpCPU, apply_basic_pass_thru_style_fwd)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 9;

    float input_32f[numPixels * 4] = {
         0.0005f,  0.005f,  0.05f,  0.75f,
        -0.0005f, -0.005f, -0.05f, -0.75f,
         0.25f,    0.5f,    0.75f,  1.0f,
        -0.25f,   -0.5f,   -0.75f, -1.0f,
         0.80f,    0.95f,   1.0f,   1.5f,
        -0.80f,   -0.95f,  -1.0f,  -1.5f,
         1.005f,   1.05f,   1.5f,   0.25f,
        -1.005f,  -1.05f,  -1.5f,  -0.25f,
        -inf,      inf,     qnan,   0.0f };

    const std::vector<double> gammaVals = { 1.2, 2.12, 1.123, 1.05 };

#if OCIO_USE_SSE2
    const float expected_32f[numPixels * 4] = {
        0.00010933f, 0.00001323f, 0.03458935f, 0.73928129f,
        input_32f[04], input_32f[05], input_32f[06], input_32f[07],
        0.18946611f, 0.23005184f, 0.72391760f, 1.00001204f,
        input_32f[12], input_32f[13], input_32f[14], input_32f[15],
        0.76507961f, 0.89695119f, 1.00001264f, 1.53070319f,
        input_32f[20], input_32f[21], input_32f[22], input_32f[23],
        1.00601125f, 1.10895324f, 1.57668686f, 0.23326106f,
        input_32f[28], input_32f[29], input_32f[30], input_32f[31],
       -inf, inf, qnan, 0.0f };
#else
    const float expected_32f[numPixels * 4] = {
        powf(input_32f[0], (float)gammaVals[0]),
        powf(input_32f[1], (float)gammaVals[1]),
        powf(input_32f[2], (float)gammaVals[2]),
        powf(input_32f[3], (float)gammaVals[3]),
        input_32f[04], input_32f[05], input_32f[06], input_32f[07],

        powf(input_32f[8], (float)gammaVals[0]),
        powf(input_32f[9], (float)gammaVals[1]),
        powf(input_32f[10], (float)gammaVals[2]),
        powf(input_32f[11], (float)gammaVals[3]),
        input_32f[12], input_32f[13], input_32f[14], input_32f[15],

        powf(input_32f[16], (float)gammaVals[0]),
        powf(input_32f[17], (float)gammaVals[1]),
        powf(input_32f[18], (float)gammaVals[2]),
        powf(input_32f[19], (float)gammaVals[3]),
        input_32f[20], input_32f[21], input_32f[22], input_32f[23],

        powf(input_32f[24], (float)gammaVals[0]),
        powf(input_32f[25], (float)gammaVals[1]),
        powf(input_32f[26], (float)gammaVals[2]),
        powf(input_32f[27], (float)gammaVals[3]),
        input_32f[28], input_32f[29], input_32f[30], input_32f[31],
       -inf, inf, qnan, 0.0f };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams = { gammaVals[0] };
    const OCIO::GammaOpData::Params greenParams = { gammaVals[1] };
    const OCIO::GammaOpData::Params blueParams = { gammaVals[2] };
    const OCIO::GammaOpData::Params alphaParams = { gammaVals[3] };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_PASS_THRU_FWD,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, __LINE__, errorThreshold);
}

OCIO_ADD_TEST(GammaOpCPU, apply_basic_pass_thru_style_rev)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 9;

    float input_32f[numPixels * 4] = {
         0.0005f,  0.005f,  0.05f,  0.75f,
        -0.0005f, -0.005f, -0.05f, -0.75f,
         0.25f,    0.5f,    0.75f,  1.0f,
        -0.25f,   -0.5f,   -0.75f, -1.0f,
         0.80f,    0.95f,   1.0f,   1.5f,
        -0.80f,   -0.95f,  -1.0f,  -1.5f,
         1.005f,   1.05f,   1.5f,   0.25f,
        -1.005f,  -1.05f,  -1.5f,  -0.25f,
        -inf,      inf,     qnan,   0.0f };

    const std::vector<double> gammaVals = { 1.2, 2.12, 1.123, 1.05 };

#if OCIO_USE_SSE2
    const float expected_32f[numPixels * 4] = {
        0.00177476f, 0.08215060f, 0.06941742f, 0.76033723f,
        input_32f[04], input_32f[05], input_32f[06], input_32f[07],
        0.31498342f, 0.72111737f, 0.77400052f, 1.00001109f,
        input_32f[12], input_32f[13], input_32f[14], input_32f[15],
        0.83031141f, 0.97609287f, 1.00001061f, 1.47130167f,
        input_32f[20], input_32f[21], input_32f[22], input_32f[23],
        1.00417137f, 1.02327621f, 1.43483067f, 0.26706201f,
        input_32f[28], input_32f[29], input_32f[30], input_32f[31],
       -inf,         1.49761057e+18f, qnan,    0.0f };
#else
    const float expected_32f[numPixels * 4] = {
        powf(input_32f[0], (float)(1. / gammaVals[0])),
        powf(input_32f[1], (float)(1. / gammaVals[1])),
        powf(input_32f[2], (float)(1. / gammaVals[2])),
        powf(input_32f[3], (float)(1. / gammaVals[3])),
        input_32f[04], input_32f[05], input_32f[06], input_32f[07],

        powf(input_32f[8],  (float)(1. / gammaVals[0])),
        powf(input_32f[9],  (float)(1. / gammaVals[1])),
        powf(input_32f[10], (float)(1. / gammaVals[2])),
        powf(input_32f[11], (float)(1. / gammaVals[3])),
        input_32f[12], input_32f[13], input_32f[14], input_32f[15],

        powf(input_32f[16], (float)(1. / gammaVals[0])),
        powf(input_32f[17], (float)(1. / gammaVals[1])),
        powf(input_32f[18], (float)(1. / gammaVals[2])),
        powf(input_32f[19], (float)(1. / gammaVals[3])),
        input_32f[20], input_32f[21], input_32f[22], input_32f[23],

        powf(input_32f[24], (float)(1. / gammaVals[0])),
        powf(input_32f[25], (float)(1. / gammaVals[1])),
        powf(input_32f[26], (float)(1. / gammaVals[2])),
        powf(input_32f[27], (float)(1. / gammaVals[3])),
        input_32f[28], input_32f[29], input_32f[30], input_32f[31],
       -inf, inf, qnan, 0.0f };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams = { gammaVals[0] };
    const OCIO::GammaOpData::Params greenParams = { gammaVals[1] };
    const OCIO::GammaOpData::Params blueParams = { gammaVals[2] };
    const OCIO::GammaOpData::Params alphaParams = { gammaVals[3] };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::BASIC_PASS_THRU_REV,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);

    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, __LINE__, errorThreshold);
}

OCIO_ADD_TEST(GammaOpCPU, apply_moncurve_style_fwd)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 7;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f,
        -inf,      inf,      qnan,       0.0f };

#if OCIO_USE_SSE2
    const float expected_32f[numPixels*4] = {
        -0.07738016f, -0.33144456f, -0.25f,      0.0f,
        -0.00019345f,  0.0f,         0.00005f,   0.49101364f, 
         0.00003869f,  0.00220963f,  0.05f,      0.73652046f,
         0.05087645f,  0.30550804f,  0.75f,      1.00001871f,
         0.60383129f,  0.91060406f,  1.0f,       1.63147723f,
         1.01142657f,  1.09394502f,  1.499984f, -0.24550682f,
        -inf,          inf,          qnan,       0.0f };
#else
    const float expected_32f[numPixels*4] = {
        -0.07738015f, -0.33144456f,  -0.25f,     0.0f,
        -0.00019345f,  0.0f,          0.00005f,  0.49101364f, 
         0.00003869f,  0.00220963f,   0.05f,     0.73652046f,
         0.05087607f,  0.30550399f,   0.75f,     1.0f,
         0.60382729f,  0.91061854f,   1.0f,      1.63146877f,
         1.01141202f,  1.09396457f,   1.5f,     -0.24550682f,
        -inf,          inf,           qnan,      0.0f };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams   = { 2.4, 0.055 };
    const OCIO::GammaOpData::Params greenParams = { 2.2, 0.2 };
    const OCIO::GammaOpData::Params blueParams  = { 1.0, 0.0 };
    const OCIO::GammaOpData::Params alphaParams = { 1.8, 0.6 };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_FWD,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, __LINE__, errorThreshold);
}

OCIO_ADD_TEST(GammaOpCPU, apply_moncurve_style_rev)
{
    const float errorThreshold = 1e-6f;
    const long numPixels = 7;

    float input_32f[numPixels*4] = {
        -1.0f,    -0.75f,   -0.25f,      0.0f,
        -0.0025f,  0.0f,     0.00005f,   0.5f,
         0.0005f,  0.005f,   0.05f,      0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
         1.005f,   1.05f,    1.5f,      -0.25f,
        -inf,      inf,      qnan,       0.0f };

#if OCIO_USE_SSE2
    const float expected_32f[numPixels*4] = {
        -6.18606853f, -1.69711625f, -0.25f,      0.0f,
        -0.01546517f,  0.0f,         0.00005f,   0.50915080f,
         0.00309303f,  0.01131410f,  0.05f,      0.76366448f,
         0.51735591f,  0.67569005f,  0.75f,      1.00001215f,
         0.90233862f,  0.97234255f,  1.0f,       1.40423023f,
         1.00229334f,  1.02690458f,  1.499984f, -0.25457540f,
        -inf,          3.92334474e+17f, qnan,    0.0f };
#else
    const float expected_32f[numPixels*4] = {
        -6.18606853f, -1.69711625f, -0.25f,     0.0f,
        -0.01546517f,  0.0f,         0.00005f,  0.50915080f,
         0.00309303f,  0.01131410f,  0.05f,     0.76367092f,
         0.51735413f,  0.67568808f,  0.75f,     1.0f,
         0.90233647f,  0.97234553f,  1.0f,      1.40423429f,
         1.00228834f,  1.02691006f,  1.5f,     -0.25457540f,
        -inf,          inf,          qnan,      0.0f };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams   = { 2.4, 0.1 };
    const OCIO::GammaOpData::Params greenParams = { 2.2, 0.2 };
    const OCIO::GammaOpData::Params blueParams  = { 1.0, 0.0 };
    const OCIO::GammaOpData::Params alphaParams = { 1.8, 0.6 };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_REV,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, __LINE__, errorThreshold);
}

OCIO_ADD_TEST(GammaOpCPU, apply_moncurve_mirror_style_fwd)
{
    const float errorThreshold = 1e-7f;
    const long numPixels = 9;

    float input_32f[numPixels * 4] = {
         0.0005f,  0.005f,   0.05f,      0.75f,
        -0.0005f, -0.005f,  -0.05f,     -0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
        -0.25f,   -0.5f,    -0.75f,     -1.0f,
         0.80f,    0.95f,    1.0f,       1.5f,
        -0.80f,   -0.95f,   -1.0f,      -1.5f,
         1.005f,   1.05f,    1.5f,       1.0f,
        -1.005f,  -1.05f,   -1.5f,      -1.0f,
        -inf,      inf,      qnan,       0.0f };

#if OCIO_USE_SSE2
    const float expected_32f[numPixels * 4] = {
         0.00003869f,  0.00220963f,  0.04081632f,  0.73652046f,
        -0.00003869f, -0.00220963f, -0.04081632f, -0.73652046f,
         0.05087645f,  0.30550804f,  0.67475068f,  1.00001871f,
        -0.05087645f, -0.30550804f, -0.67475068f, -1.00001871f,
         0.60383129f,  0.91060406f,  1.00002050f,  1.63147723f,
        -0.60383129f, -0.91060406f, -1.00002050f, -1.63147723f,
         1.01142657f,  1.09394502f,  1.84183871f,  1.00001871f,
        -1.01142657f, -1.09394502f, -1.84183871f, -1.00001871f,
        -inf,          inf,          qnan,         0.0f };
#else
    const float expected_32f[numPixels * 4] = {
         0.00003869f,  0.00220963f,  0.04081632f,  0.73652046f,
        -0.00003869f, -0.00220963f, -0.04081632f, -0.73652046f,
         0.05087607f,  0.30550399f,  0.67474484f,  1.0f,
        -0.05087607f, -0.30550399f, -0.67474484f, -1.0f,
         0.60382729f,  0.91061854f,  1.0f,         1.63146877f,
        -0.60382729f, -0.91061854f, -1.0f,        -1.63146877f,
         1.01141202f,  1.09396457f,  1.84183657f,  1.0f,
        -1.01141202f, -1.09396457f, -1.84183657f, -1.0f,
        -inf,          inf,          qnan,         0.0f };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams = { 2.4, 0.055 };
    const OCIO::GammaOpData::Params greenParams = { 2.2, 0.2 };
    const OCIO::GammaOpData::Params blueParams = { 2.0, 0.4 };
    const OCIO::GammaOpData::Params alphaParams = { 1.8, 0.6 };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_MIRROR_FWD,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, __LINE__, errorThreshold);
}

OCIO_ADD_TEST(GammaOpCPU, apply_moncurve_mirror_style_rev)
{
    const float errorThreshold = 1e-6f;
    const long numPixels = 9;

    float input_32f[numPixels * 4] = {
         0.0005f,  0.005f,   0.05f,      0.75f,
        -0.0005f, -0.005f,  -0.05f,     -0.75f,
         0.25f,    0.5f,     0.75f,      1.0f,
        -0.25f,   -0.5f,    -0.75f,     -1.0f,
         0.80f,    0.95f,    1.0f,       0.75f,
        -0.80f,   -0.95f,   -1.0f,      -0.75f,
         1.005f,   1.05f,    1.5f,       1.0f,
        -1.005f,  -1.05f,   -1.5f,      -1.0f,
        -inf,      inf,      qnan,       0.0f };

#if OCIO_USE_SSE2
    const float expected_32f[numPixels * 4] = {
         0.00309303f,  0.01131410f,  0.06125000f,  0.76366448f,
        -0.00309303f, -0.01131410f, -0.06125000f, -0.76366448f,
         0.51735591f,  0.67569005f,  0.81243133f,  1.00001215f,
        -0.51735591f, -0.67569005f, -0.81243133f, -1.00001215f,
         0.90233862f,  0.97234255f,  1.00000989f,  0.76366448f,
        -0.90233862f, -0.97234255f, -1.00000989f, -0.76366448f,
         1.00229334f,  1.02690458f,  1.31464004f,  1.00001215f,
        -1.00229334f, -1.02690458f, -1.31464004f, -1.00001215f,
        -1.24832838e+16f, 3.92334474e+17f, qnan,   0.0f };
#else
    const float expected_32f[numPixels * 4] = {
         0.00309303f,  0.01131410f,  0.06125000f,  0.76367092f,
        -0.00309303f, -0.01131410f, -0.06125000f, -0.76367092f,
         0.51735413f,  0.67568808f,  0.81243550f,  1.0f,
        -0.51735413f, -0.67568808f, -0.81243550f, -1.0f,
         0.90233647f,  0.97234553f,  1.0f,         0.76367092f,
        -0.90233647f, -0.97234553f, -1.0f,        -0.76367092f,
         1.00228834f,  1.02691006f,  1.31464290f,  1.0f,
        -1.00228834f, -1.02691006f, -1.31464290f, -1.0f,
        -inf,          inf,          qnan,         0.0f };
#endif

    OCIO::OpRcPtrVec ops;

    const OCIO::GammaOpData::Params redParams = { 2.4, 0.1 };
    const OCIO::GammaOpData::Params greenParams = { 2.2, 0.2 };
    const OCIO::GammaOpData::Params blueParams = { 2.0, 0.4 };
    const OCIO::GammaOpData::Params alphaParams = { 1.8, 0.6 };

    auto gammaData = std::make_shared<OCIO::GammaOpData>(OCIO::GammaOpData::MONCURVE_MIRROR_REV,
                                                         redParams,
                                                         greenParams,
                                                         blueParams,
                                                         alphaParams);
    OCIO_CHECK_NO_THROW(OCIO::CreateGammaOp(ops, gammaData, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_CHECK_NO_THROW(ops.optimize(OCIO::OPTIMIZATION_DEFAULT));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    ApplyGamma(ops[0], input_32f, expected_32f, numPixels, __LINE__, errorThreshold);
}

