// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/log/LogOpCPU.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;

constexpr float qnan = std::numeric_limits<float>::quiet_NaN();
constexpr float inf = std::numeric_limits<float>::infinity();

void TestLog(float logBase)
{
    const float rgbaImage[32] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                  0.2f,       0.f,   .99f, 128.f,
                                  qnan,       qnan, qnan,    0.f,
                                  0.f,        0.f,  0.f,     qnan,
                                  inf,        inf,  inf,     0.f,
                                  0.f,        0.f,  0.f,     inf,
                                 -inf,       -inf, -inf,     0.f,
                                  0.f,        0.f,  0.f,    -inf };
                                  
    float rgba[32] = {};

    OCIO::ConstLogOpDataRcPtr logOp = std::make_shared<OCIO::LogOpData>(
        logBase, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::ConstOpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp, true);
    pRenderer->apply(rgbaImage, rgba, 8);

    const float minValue = std::numeric_limits<float>::min();

    // LogOpCPU implementation uses optimized logarithm approximation
    // cannot use strict comparison.
#if OCIO_USE_SSE2
    const float error = 5e-5f;
#else
    const float error = 1e-5f;
#endif // OCIO_USE_SSE2

    for (unsigned i = 0; i < 8; ++i)
    {
        const bool isAlpha = (i % 4 == 3);

        const float result = rgba[i];
        float expected = rgbaImage[i];
        if (!isAlpha)
        {
            expected = logf(std::max(minValue, (float)expected)) / logf(logBase);
        }

        // Evaluating output for input rgbaImage[0-7] = { 0.0367126f, 0.5f, 1.f,     0.f,
        //                                                0.2f,       0.f,   .99f, 128.f,
        //                                                ... }
        OCIO_CHECK_CLOSE(result, expected, error);
    }

    const float resMin = logf(minValue) / logf(logBase);

    // Evaluating output for input rgbaImage[8-11] = {qnan, qnan, qnan, 0.}.
    OCIO_CHECK_CLOSE(rgba[8], resMin, error);
    OCIO_CHECK_EQUAL(rgba[11], 0.0f);

    // Evaluating output for input rgbaImage[12-15] = {0., 0., 0., qnan.}.
    OCIO_CHECK_CLOSE(rgba[12], resMin, error);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[15]));

    // SSE implementation of sseLog2 & sseExp2 do not behave like CPU.
    // TODO: Address issues with Inf/NaN handling demonstrated by many of the test results below.
    // Evaluating output for input rgbaImage[16-19] = {inf, inf, inf, 0.}.
#if OCIO_USE_SSE2
    if (logBase == 10.0f)
    {
        OCIO_CHECK_CLOSE(rgba[16], 38.53184509f, error);
    }
    else if (logBase == 2.0f)
    {
        OCIO_CHECK_CLOSE(rgba[16], 128.0000153f, error);
    }
#else
    OCIO_CHECK_EQUAL(rgba[16], inf);
#endif
    OCIO_CHECK_EQUAL(rgba[19], 0.0f);

    // Evaluating output for input rgbaImage[20-23] =  {0., 0., 0., inf}.
    OCIO_CHECK_CLOSE(rgba[20], resMin, error);
    OCIO_CHECK_EQUAL(rgba[23], inf);

    // Evaluating output for input rgbaImage[24-27] = {-inf, -inf, -inf, 0.}.
    OCIO_CHECK_CLOSE(rgba[24], resMin, error);
    OCIO_CHECK_EQUAL(rgba[27], 0.0f);

    // Evaluating output for input rgbaImage[28-31] = {0., 0.,  0., -inf}.
    OCIO_CHECK_CLOSE(rgba[28], resMin, error);
    OCIO_CHECK_EQUAL(rgba[31], -inf);
}

OCIO_ADD_TEST(LogOpCPU, log_test)
{
    // Log base 10 case, no scaling.
    TestLog(10.0f);

    // Log base 2 case, no scaling.
    TestLog(2.0f);
}

void TestAntiLog(float logBase)
{
    const float rgbaImage[32] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                 0.2f,       0.f,   .99f, 128.f,
                                 qnan,       qnan, qnan,    0.f,
                                 0.f,        0.f,  0.f,     qnan,
                                 inf,        inf,  inf,     0.f,
                                 0.f,        0.f,  0.f,     inf,
                                -inf,       -inf, -inf,     0.f,
                                 0.f,        0.f,  0.f,    -inf };

    float rgba[32] = {};

    OCIO::ConstLogOpDataRcPtr logOp = std::make_shared<OCIO::LogOpData>(
        logBase, OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::ConstOpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp, true);
    pRenderer->apply(rgbaImage, rgba, 8);

    // Relative error tolerance for the log2 approximation.
    const float rtol = powf(2.f, -14.f);

    for (unsigned i = 0; i < 8; ++i)
    {
        const bool isAlpha = (i % 4 == 3);

        const float result = rgba[i];
        float expected = rgbaImage[i];
        if (!isAlpha)
        {
            expected = powf(logBase, expected);
        }

        // LogOpCPU implementation uses optimized logarithm approximation
        // cannot use strict comparison.
        // Evaluating output for input rgbaImage[0-7] = { 0.0367126f, 0.5f, 1.f,     0.f,
        //                                                0.2f,       0.f,   .99f, 128.f,
        //                                                ... }
        OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(result, expected, rtol, 1.0f));
    }

    // Evaluating output for input rgbaImage[8-11] = {qnan, qnan, qnan, 0.}.
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[8]));
    OCIO_CHECK_EQUAL(rgba[11], 0.0f);

    // Evaluating output for input rgbaImage[12-15] = {0., 0., 0., qnan.}.
    OCIO_CHECK_CLOSE(rgba[12], 1.0f, rtol);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[15]));

    // Evaluating output for input rgbaImage[16-19] = {inf, inf, inf, 0.}.
    OCIO_CHECK_EQUAL(rgba[16], inf);
    OCIO_CHECK_EQUAL(rgba[19], 0.0f);

    // Evaluating output for input rgbaImage[20-23] =  {0., 0., 0., inf}.
    OCIO_CHECK_CLOSE(rgba[20], 1.0f, rtol);
    OCIO_CHECK_EQUAL(rgba[23], inf);

    // Evaluating output for input rgbaImage[24-27] = {-inf, -inf, -inf, 0.}.
    OCIO_CHECK_EQUAL(rgba[24], 0.0f);
    OCIO_CHECK_EQUAL(rgba[27], 0.0f);

    // Evaluating output for input rgbaImage[28-31] = {0., 0.,  0., -inf}.
    OCIO_CHECK_CLOSE(rgba[28], 1.0f, rtol);
    OCIO_CHECK_EQUAL(rgba[31], -inf);
}

OCIO_ADD_TEST(LogOpCPU, anti_log_test)
{
    // Anti-Log base 10 case, no scaling.
    TestAntiLog(10.0f);

    // Anti-Log base 2 case, no scaling.
    TestAntiLog(2.0f);
}

float ComputeLog2LinEval(float in, const OCIO::LogUtil::CTFParams::Params & params)
{
    const float range = 0.002f * 1023.0f;

    const float gamma = (float)params[0];
    const float refWhite = (float)params[1] / 1023.0f;
    const float refBlack = (float)params[2] / 1023.0f;
    const float highlight = (float)params[3];
    const float shadow = (float)params[4];

    const float mult_factor = range / gamma;

    float tmp_value = (refBlack - refWhite) * mult_factor;
    tmp_value = std::min(tmp_value, -0.0001f);

    const float gain = (highlight - shadow)
                       / (1.0f - powf(10.0f, tmp_value));
    const float offset = gain - (highlight - shadow);

    return powf(10.f, (in - refWhite) * mult_factor) * gain
           - offset + shadow;
}

OCIO_ADD_TEST(LogOpCPU, log2lin_test)
{
    const float rgbaImage[32] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                  0.2f,       0.f,   .99f, 128.f,
                                  qnan,       qnan, qnan,    0.f,
                                  0.f,        0.f,  0.f,     qnan,
                                  inf,        inf,  inf,     0.f,
                                  0.f,        0.f,  0.f,     inf,
                                 -inf,       -inf, -inf,     0.f,
                                  0.f,        0.f,  0.f,    -inf };

    float rgba[32] = {};

    OCIO::LogUtil::CTFParams params;
    auto & redP = params.get(OCIO::LogUtil::CTFParams::red);
    redP[OCIO::LogUtil::CTFParams::gamma]     = 0.5;
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 685.;
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 93.;
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.8;
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.0004;

    auto & greenP = params.get(OCIO::LogUtil::CTFParams::green);
    greenP[OCIO::LogUtil::CTFParams::gamma]     = 0.6;
    greenP[OCIO::LogUtil::CTFParams::refWhite]  = 684.;
    greenP[OCIO::LogUtil::CTFParams::refBlack]  = 94.;
    greenP[OCIO::LogUtil::CTFParams::highlight] = 0.9;
    greenP[OCIO::LogUtil::CTFParams::shadow]    = 0.0005;

    auto & blueP = params.get(OCIO::LogUtil::CTFParams::blue);
    blueP[OCIO::LogUtil::CTFParams::gamma]     = 0.65;
    blueP[OCIO::LogUtil::CTFParams::refWhite]  = 683.;
    blueP[OCIO::LogUtil::CTFParams::refBlack]  = 95.;
    blueP[OCIO::LogUtil::CTFParams::highlight] = 1.0;
    blueP[OCIO::LogUtil::CTFParams::shadow]    = 0.0003;

    params.m_style = OCIO::LogUtil::LOG_TO_LIN;

    OCIO::LogOpData::Params paramsR, paramsG, paramsB;
    double base = 1.0;
    OCIO::TransformDirection dir = OCIO::LogUtil::GetLogDirection(params.m_style);
    OCIO::LogUtil::ConvertLogParameters(params, base, paramsR, paramsG, paramsB);

    OCIO::ConstLogOpDataRcPtr logOp
        = std::make_shared<OCIO::LogOpData>(base, paramsR, paramsG, paramsB, dir);

    OCIO::ConstOpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp, true);
    pRenderer->apply(rgbaImage, rgba, 8);

    const OCIO::LogUtil::CTFParams::Params noParam;

    // Relative error tolerance for the log2 approximation.
    const float rtol = powf(2.f, -14.f);

    for (unsigned i = 0; i < 8; ++i)
    {
        const bool isRed = (i % 4 == 0);
        const bool isGreen = (i % 4 == 1);
        const bool isBlue = (i % 4 == 2);
        const bool isAlpha = (i % 4 == 3);

        const OCIO::LogUtil::CTFParams::Params & params
            = isGreen ? greenP
            : isBlue ? blueP
            : isRed ? redP
            : noParam;

        const float result = rgba[i];
        float expected = rgbaImage[i];

        if (!isAlpha)
        {
            expected = ComputeLog2LinEval(expected, params);
        }

        // LogOpCPU implementation uses optimized logarithm approximation
        // cannot use strict comparison.
        // Evaluating output for input rgbaImage[0-7] = { 0.0367126f, 0.5f, 1.f,     0.f,
        //                                                0.2f,       0.f,   .99f, 128.f,
        //                                                ... }
        OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(result, expected, rtol, 1.0f));
    }

    const float res0 = ComputeLog2LinEval(0.0f, redP);

    // Evaluating output for input rgbaImage[8-11] = {qnan, qnan, qnan, 0.}.
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[8]));
    OCIO_CHECK_EQUAL(rgba[11], 0.0f);

    // Evaluating output for input rgbaImage[12-15] = {0., 0., 0., qnan.}.
    OCIO_CHECK_CLOSE(rgba[12], res0, rtol);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[15]));

    // Evaluating output for input rgbaImage[16-19] = {inf, inf, inf, 0.}.
    OCIO_CHECK_EQUAL(rgba[16], inf);
    OCIO_CHECK_EQUAL(rgba[19], 0.0f);

    // Evaluating output for input rgbaImage[20-23] =  {0., 0., 0., inf}.
    OCIO_CHECK_CLOSE(rgba[20], res0, rtol);
    OCIO_CHECK_EQUAL(rgba[23], inf);

    // Evaluating output for input rgbaImage[24-27] = {-inf, -inf, -inf, 0.}.
    OCIO_CHECK_CLOSE(rgba[24], ComputeLog2LinEval(-inf, redP), rtol);
    OCIO_CHECK_EQUAL(rgba[27], 0.0f);

    // Evaluating output for input rgbaImage[28-31] = {0., 0.,  0., -inf}.
    OCIO_CHECK_CLOSE(rgba[28], res0, rtol);
    OCIO_CHECK_EQUAL(rgba[31], -inf);
}

float ComputeLin2LogEval(float in, const OCIO::LogUtil::CTFParams::Params & params)
{
    const float minValue = std::numeric_limits<float>::min();

    const float gamma = (float)params[0];
    const float refWhite = (float)params[1] / 1023.0f;
    const float refBlack = (float)params[2] / 1023.0f;
    const float highlight = (float)params[3];
    const float shadow = (float)params[4];

    const float range = 0.002f * 1023.0f;
    const float mult_factor = range / gamma;

    float tmp_value = (refBlack - refWhite) * mult_factor;
    tmp_value = std::min(tmp_value, -0.0001f);

    const float gain = (highlight - shadow)
                       / (1.0f - powf(10.0f, tmp_value));
    const float offset = gain - (highlight - shadow);

    in = (in - shadow + offset) / gain;
    return log10(std::max(minValue, (float)in))
           / mult_factor + refWhite;
}

OCIO_ADD_TEST(LogOpCPU, lin2log_test)
{
    const float rgbaImage[32] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                  0.2f,       0.f,   .99f, 128.f,
                                  qnan,       qnan, qnan,    0.f,
                                  0.f,        0.f,  0.f,     qnan,
                                  inf,        inf,  inf,     0.f,
                                  0.f,        0.f,  0.f,     inf,
                                 -inf,       -inf, -inf,     0.f,
                                  0.f,        0.f,  0.f,    -inf };

    float rgba[32] = {};

    OCIO::LogUtil::CTFParams params;
    auto & redP = params.get(OCIO::LogUtil::CTFParams::red);
    redP[OCIO::LogUtil::CTFParams::gamma]     = 0.5;
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 685.;
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 93.;
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.8;
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.0004;

    auto & greenP = params.get(OCIO::LogUtil::CTFParams::green);
    greenP[OCIO::LogUtil::CTFParams::gamma]     = 0.6;
    greenP[OCIO::LogUtil::CTFParams::refWhite]  = 684.;
    greenP[OCIO::LogUtil::CTFParams::refBlack]  = 94.;
    greenP[OCIO::LogUtil::CTFParams::highlight] = 0.9;
    greenP[OCIO::LogUtil::CTFParams::shadow]    = 0.0005;

    auto & blueP = params.get(OCIO::LogUtil::CTFParams::blue);
    blueP[OCIO::LogUtil::CTFParams::gamma]     = 0.65;
    blueP[OCIO::LogUtil::CTFParams::refWhite]  = 683.;
    blueP[OCIO::LogUtil::CTFParams::refBlack]  = 95.;
    blueP[OCIO::LogUtil::CTFParams::highlight] = 1.0;
    blueP[OCIO::LogUtil::CTFParams::shadow]    = 0.0003;

    params.m_style = OCIO::LogUtil::LIN_TO_LOG;

    OCIO::LogOpData::Params paramsR, paramsG, paramsB;
    double base = 1.0;
    OCIO::TransformDirection dir = OCIO::LogUtil::GetLogDirection(params.m_style);
    OCIO::LogUtil::ConvertLogParameters(params, base, paramsR, paramsG, paramsB);

    OCIO::ConstLogOpDataRcPtr logOp 
        = std::make_shared<OCIO::LogOpData>(base, paramsR, paramsG, paramsB, dir);

    OCIO::ConstOpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp, true);
    pRenderer->apply(rgbaImage, rgba, 8);

    const OCIO::LogUtil::CTFParams::Params noParam;

    const float error = 1e-4f;
    for (unsigned i = 0; i < 8; ++i)
    {
        const bool isRed = (i % 4 == 0);
        const bool isGreen = (i % 4 == 1);
        const bool isBlue = (i % 4 == 2);
        const bool isAlpha = (i % 4 == 3);

        const OCIO::LogUtil::CTFParams::Params & params
            = isGreen ? greenP
            : isBlue ? blueP
            : isRed ? redP
            : noParam;

        const float result = rgba[i];
        float expected = rgbaImage[i];

        if (!isAlpha)
        {
            expected = ComputeLin2LogEval(expected, params);
        }

        // LogOpCPU implementation uses optimized logarithm approximation
        // cannot use strict comparison
        // Evaluating output for input rgbaImage[0-7] = { 0.0367126f, 0.5f, 1.f,     0.f,
        //                                                0.2f,       0.f,   .99f, 128.f,
        //                                                ... }
        OCIO_CHECK_CLOSE(result, expected, error);
    }

    const float res0 = ComputeLin2LogEval(0.0f, redP);
    const float resMin = ComputeLin2LogEval(-100.0f, redP);

    // Evaluating output for input rgbaImage[8-11] = {qnan, qnan, qnan, 0.}.
    OCIO_CHECK_CLOSE(rgba[8], resMin, error);
    OCIO_CHECK_EQUAL(rgba[11], 0.0f);

    // Evaluating output for input rgbaImage[12-15] = {0., 0., 0., qnan.}.
    OCIO_CHECK_CLOSE(rgba[12], res0, error);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[15]));

    // Evaluating output for input rgbaImage[16-19] = {inf, inf, inf, 0.}.
#if OCIO_USE_SSE2
    OCIO_CHECK_CLOSE(rgba[16], 10.08598328f, error);
#else
    OCIO_CHECK_EQUAL(rgba[16], inf);
#endif
    OCIO_CHECK_EQUAL(rgba[19], 0.0f);

    // Evaluating output for input rgbaImage[20-23] =  {0., 0., 0., inf}.
    OCIO_CHECK_CLOSE(rgba[20], res0, error);
    OCIO_CHECK_EQUAL(rgba[23], inf);

    // Evaluating output for input rgbaImage[24-27] = {-inf, -inf, -inf, 0.}.
    OCIO_CHECK_CLOSE(rgba[24], resMin, error);
    OCIO_CHECK_EQUAL(rgba[27], 0.0f);

    // Evaluating output for input rgbaImage[28-31] = {0., 0.,  0., -inf}.
    OCIO_CHECK_CLOSE(rgba[28], res0, error);
    OCIO_CHECK_EQUAL(rgba[31], -inf);
}

OCIO_ADD_TEST(LogOpCPU, cameralin2log_test)
{
    constexpr int numPixels = 3;
    constexpr int numValues = 4 * numPixels;
    const float rgbaImage[numValues] = { -0.1f,  0.f,   0.01f, 0.0f,
                                          0.08f, 0.16f, 1.16f, 0.0f,
                                         -inf,   inf,   qnan,  0.0f};

    float rgba[numValues] = {};

    // logSideSlope = 0.2
    // logSideOffset = 0.6
    // linSideSlope = 1.1
    // linSideOffset = 0.05
    // linSideBreak = 0.1
    // linearSlope = 1.2

    OCIO::LogOpData::Params params{ 0.2, 0.6, 1.1, 0.05, 0.1, 1.2 };
    const double base = 2.0;
    OCIO::TransformDirection dir{ OCIO::TRANSFORM_DIR_FORWARD };
    OCIO::ConstLogOpDataRcPtr logOp
        = std::make_shared<OCIO::LogOpData>(base, params, params, params, dir);

    OCIO::ConstOpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp, true);
    pRenderer->apply(rgbaImage, rgba, numPixels);

#if OCIO_USE_SSE2
    const float error = 1e-6f;
#else
    const float error = 1e-7f;
#endif // OCIO_USE_SSE2

    // Evaluating output for input rgbaImage[0-2] = { -0.1f, 0.f, 0.01f, ... }.
    OCIO_CHECK_CLOSE(rgba[0], -0.168771237955f, error);
    OCIO_CHECK_CLOSE(rgba[1], -0.048771237955f, error);
    OCIO_CHECK_CLOSE(rgba[2], -0.036771237955f, error);

    // Evaluating output for input rgbaImage[4-6] = { 0.08f, 0.16f, 1.16f, ... }.
    OCIO_CHECK_CLOSE(rgba[4], 0.047228762045f, error);
#if OCIO_USE_SSE2
    OCIO_CHECK_CLOSE(rgba[5], 0.170878935551f, 10.0f * error);
#else
    OCIO_CHECK_CLOSE(rgba[5], 0.170878935551f, error);
#endif // OCIO_USE_SSE2
    OCIO_CHECK_CLOSE(rgba[6], 0.68141615509f, error);

   // Evaluating output for input rgbaImage[8-10] = { -inf, inf, qnan, ... }.
#if OCIO_USE_SSE2
    OCIO_CHECK_EQUAL(rgba[8], -inf);
    OCIO_CHECK_CLOSE(rgba[9], 26.2f, 10.0f * error);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[10]));
#else
    OCIO_CHECK_EQUAL(rgba[8], -inf);
    OCIO_CHECK_EQUAL(rgba[9], inf);
    OCIO_CHECK_CLOSE(rgba[10], -24.6f, error);
#endif

    float rgba_nols[numValues] = {};

    // Set linearSlope to default.
    params.pop_back();
    OCIO::ConstLogOpDataRcPtr lognols
        = std::make_shared<OCIO::LogOpData>(base, params, params, params, dir);

    OCIO::ConstOpCPURcPtr pRendererNoLS = OCIO::GetLogRenderer(lognols, true);
    pRendererNoLS->apply(rgbaImage, rgba_nols, numPixels);

    // Evaluating output for input rgbaImage[0-2] = { -0.1f, 0.f, 0.01f, ... }.
    OCIO_CHECK_CLOSE(rgba_nols[0], -0.325512374199f, error);
    OCIO_CHECK_CLOSE(rgba_nols[1], -0.127141806077f, error);
    OCIO_CHECK_CLOSE(rgba_nols[2], -0.107304749265f, error);
    
    // Evaluating output for input rgbaImage[4-6] = { 0.08f, 0.16f, 1.16f, ... }.
    OCIO_CHECK_CLOSE(rgba_nols[4], 0.031554648421f, error);
#if OCIO_USE_SSE2
    OCIO_CHECK_CLOSE(rgba_nols[5], 0.170878935551f, 10.0f * error);
#else
    OCIO_CHECK_CLOSE(rgba_nols[5], 0.170878935551f, error);
#endif // OCIO_USE_SSE2
    OCIO_CHECK_CLOSE(rgba_nols[6], 0.68141615509f, error);

    // Evaluating output for input rgbaImage[8-10] = { -inf, inf, qnan, ... }.
    OCIO_CHECK_EQUAL(rgba_nols[8], -inf);
#if OCIO_USE_SSE2
    OCIO_CHECK_CLOSE(rgba_nols[9], 26.2f, 10.0f * error);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba_nols[10]));
#else
    OCIO_CHECK_EQUAL(rgba_nols[9], inf);
    OCIO_CHECK_CLOSE(rgba_nols[10], -24.6f, error);
#endif // OCIO_USE_SSE2

    float rgba_nobreak[numValues] = {};

    // Don't use break.
    params.pop_back();
    OCIO::ConstLogOpDataRcPtr lognobreak
        = std::make_shared<OCIO::LogOpData>(base, params, params, params, dir);

    OCIO::ConstOpCPURcPtr pRendererNoBreak = OCIO::GetLogRenderer(lognobreak, true);
    pRendererNoBreak->apply(rgbaImage, rgba_nobreak, numPixels);

#if OCIO_USE_SSE2
    const float error2 = 1e-5f;
#else
    const float error2 = 1e-7f;
#endif // OCIO_USE_SSE2

    // Evaluating output for input rgbaImage[0-2] = { -0.1f, 0.f, 0.01f, ... }.
    OCIO_CHECK_CLOSE(rgba_nobreak[0], -24.6f, error2);
    OCIO_CHECK_CLOSE(rgba_nobreak[1], -0.264385618977f, error2);
    OCIO_CHECK_CLOSE(rgba_nobreak[2], -0.20700938942f, error2);

    // Evaluating output for input rgbaImage[4-6] = { 0.08f, 0.16f, 1.16f, ... }.
    OCIO_CHECK_CLOSE(rgba_nobreak[4], 0.028548034423f, error2);
    OCIO_CHECK_CLOSE(rgba_nobreak[5], 0.170878935551f, error2);
    OCIO_CHECK_CLOSE(rgba_nobreak[6], 0.68141615509, error2);

    // Evaluating output for input rgbaImage[8-10] = { -inf, inf, qnan, ... }.
    OCIO_CHECK_CLOSE(rgba_nobreak[8], -24.6f, error2);
#if OCIO_USE_SSE2
    OCIO_CHECK_CLOSE(rgba_nobreak[9], 26.2f, error2);
#else
    OCIO_CHECK_EQUAL(rgba_nobreak[9], inf);
#endif
    OCIO_CHECK_CLOSE(rgba_nobreak[10], -24.6f, error2);
}

OCIO_ADD_TEST(LogOpCPU, cameralog2lin_test)
{
    // Inverse of previous test.
    const float rgbaImage[12] = { -0.168771237955f, -0.048771237955f, -0.036771237955f, 0.f,
                                   0.047228762045f, 0.170878935551f, 0.68141615509f, 0.f,
                                  -inf,   inf,   qnan,  0.0f };

    float rgba[12] = {};

    OCIO::LogOpData::Params params{ 0.2, 0.6, 1.1, 0.05, 0.1, 1.2 };
    const double base = 2.0;
    OCIO::TransformDirection dir{ OCIO::TRANSFORM_DIR_INVERSE };
    OCIO::ConstLogOpDataRcPtr logOp
        = std::make_shared<OCIO::LogOpData>(base, params, params, params, dir);

    OCIO::ConstOpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp, true);
    pRenderer->apply(rgbaImage, rgba, 3);

#if OCIO_USE_SSE2
    const float error = 1e-6f;
#else
    const float error = 1e-7f;
#endif // OCIO_USE_SSE2

    // Evaluating output for input rgbaImage[0-2] = 
    // { -0.168771237955f, -0.048771237955f, -0.036771237955f, ... }.
    OCIO_CHECK_CLOSE(rgba[0], -0.1f, error);
    OCIO_CHECK_CLOSE(rgba[1],  0.0f, error);
    OCIO_CHECK_CLOSE(rgba[2],  0.01f, error);

    // Evaluating output for input rgbaImage[4-6] = 
    // { 0.047228762045f, 0.170878935551f, 0.68141615509f, ... }.
    OCIO_CHECK_CLOSE(rgba[4],  0.08f, error);
    OCIO_CHECK_CLOSE(rgba[5],  0.16f, error);
    OCIO_CHECK_CLOSE(rgba[6],  1.16f, 10.0f * error);

    // Evaluating output for input rgbaImage[8-10] = { -inf, inf, qnan, ... }.
    OCIO_CHECK_EQUAL(rgba[8], -inf);
    OCIO_CHECK_EQUAL(rgba[9], inf);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[10]));
}

