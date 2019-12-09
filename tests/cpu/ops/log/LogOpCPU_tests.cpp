// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "ops/log/LogOpCPU.cpp"

#include "UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;

void TestLog(float logBase)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float rgbaImage[32] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                  0.2f,       0.f,   .99f, 128.f,
                                  qnan,       qnan, qnan,    0.f,
                                  0.f,        0.f,  0.f,     qnan,
                                  inf,        inf,  inf,     0.f,
                                  0.f,        0.f,  0.f,     inf,
                                 -inf,       -inf, -inf,     0.f,
                                  0.f,        0.f,  0.f,    -inf };
    float rgba[32] = {};
    memcpy(rgba, &rgbaImage[0], 32 * sizeof(float));

    OCIO::ConstLogOpDataRcPtr logOp = std::make_shared<OCIO::LogOpData>(
        logBase, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::ConstOpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp);
    pRenderer->apply(rgba, rgba, 8);

    const float minValue = std::numeric_limits<float>::min();

    // LogOpCPU implementation uses optimized logarithm approximation
    // cannot use strict comparison.
#ifdef USE_SSE
    const float error = 5e-5f;
#else
    const float error = 1e-5f;
#endif // USE_SSE

    for (unsigned i = 0; i < 8; ++i)
    {
        const bool isAlpha = (i % 4 == 3);

        const float result = rgba[i];
        float expected = rgbaImage[i];
        if (!isAlpha)
        {
            expected = logf(std::max(minValue, (float)expected)) / logf(logBase);
        }

        OCIO_CHECK_CLOSE(result, expected, error);
    }

    const float resMin = logf(minValue) / logf(logBase);
    OCIO_CHECK_CLOSE(rgba[8], resMin, error);
    OCIO_CHECK_EQUAL(rgba[11], 0.0f);
    OCIO_CHECK_CLOSE(rgba[12], resMin, error);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[15]));
    // SSE implementation of sseLog2 & sseExp2 do not behave like CPU for
    // infinity & NaN. Some tests had to be disabled.
    //OCIO_CHECK_EQUAL(rgba[16], inf);
    OCIO_CHECK_EQUAL(rgba[19], 0.0f);
    OCIO_CHECK_CLOSE(rgba[20], resMin, error);
    OCIO_CHECK_EQUAL(rgba[23], inf);
    OCIO_CHECK_CLOSE(rgba[24], resMin, error);
    OCIO_CHECK_EQUAL(rgba[27], 0.0f);
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
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float rgbaImage[32] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                 0.2f,       0.f,   .99f, 128.f,
                                 qnan,       qnan, qnan,    0.f,
                                 0.f,        0.f,  0.f,     qnan,
                                 inf,        inf,  inf,     0.f,
                                 0.f,        0.f,  0.f,     inf,
                                -inf,       -inf, -inf,     0.f,
                                 0.f,        0.f,  0.f,    -inf };

    float rgba[32] = {};
    memcpy(rgba, &rgbaImage[0], 32 * sizeof(float));

    OCIO::ConstLogOpDataRcPtr logOp = std::make_shared<OCIO::LogOpData>(
        logBase, OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::ConstOpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp);
    pRenderer->apply(rgba, rgba, 8);

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
        OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(result, expected, rtol, 1.0f));
    }
    //OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[8]));
    OCIO_CHECK_EQUAL(rgba[11], 0.0f);
    OCIO_CHECK_CLOSE(rgba[12], 1.0f, rtol);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[15]));
    //OCIO_CHECK_EQUAL(rgba[16], inf);
    OCIO_CHECK_EQUAL(rgba[19], 0.0f);
    OCIO_CHECK_CLOSE(rgba[20], 1.0f, rtol);
    OCIO_CHECK_EQUAL(rgba[23], inf);
    //OCIO_CHECK_EQUAL(rgba[24], 0.0f);
    OCIO_CHECK_EQUAL(rgba[27], 0.0f);
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
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float rgbaImage[32] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                  0.2f,       0.f,   .99f, 128.f,
                                  qnan,       qnan, qnan,    0.f,
                                  0.f,        0.f,  0.f,     qnan,
                                  inf,        inf,  inf,     0.f,
                                  0.f,        0.f,  0.f,     inf,
                                 -inf,       -inf, -inf,     0.f,
                                  0.f,        0.f,  0.f,    -inf };

    float rgba[32] = {};
    memcpy(rgba, &rgbaImage[0], 32 * sizeof(float));

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
    OCIO::TransformDirection dir;
    OCIO::LogUtil::ConvertLogParameters(params, base, paramsR, paramsG, paramsB, dir);

    OCIO::ConstLogOpDataRcPtr logOp = std::make_shared<OCIO::LogOpData>(
        dir, base, paramsR, paramsG, paramsB);

    OCIO::ConstOpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp);
    pRenderer->apply(rgba, rgba, 8);

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
        OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(result, expected, rtol, 1.0f));
    }

    const float res0 = ComputeLog2LinEval(0.0f, redP);

    //OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[8]));
    OCIO_CHECK_EQUAL(rgba[11], 0.0f);

    OCIO_CHECK_CLOSE(rgba[12], res0, rtol);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[15]));

    //OCIO_CHECK_EQUAL(rgba[16], inf);
    OCIO_CHECK_EQUAL(rgba[19], 0.0f);

    OCIO_CHECK_CLOSE(rgba[20], res0, rtol);
    OCIO_CHECK_EQUAL(rgba[23], inf);

    //OCIO_CHECK_CLOSE(rgba[24], ComputeLog2LinEval(-inf, redP), rtol);
    OCIO_CHECK_EQUAL(rgba[27], 0.0f);

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
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float rgbaImage[32] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                  0.2f,       0.f,   .99f, 128.f,
                                  qnan,       qnan, qnan,    0.f,
                                  0.f,        0.f,  0.f,     qnan,
                                  inf,        inf,  inf,     0.f,
                                  0.f,        0.f,  0.f,     inf,
                                 -inf,       -inf, -inf,     0.f,
                                  0.f,        0.f,  0.f,    -inf };

    float rgba[32] = {};
    memcpy(rgba, &rgbaImage[0], 32 * sizeof(float));

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
    OCIO::TransformDirection dir;
    OCIO::LogUtil::ConvertLogParameters(params, base, paramsR, paramsG, paramsB, dir);

    OCIO::ConstLogOpDataRcPtr logOp = std::make_shared<OCIO::LogOpData>(
        dir, base, paramsR, paramsG, paramsB);

    OCIO::ConstOpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp);
    pRenderer->apply(rgba, rgba, 8);

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
        OCIO_CHECK_CLOSE(result, expected, error);
    }

    const float res0 = ComputeLin2LogEval(0.0f, redP);
    const float resMin = ComputeLin2LogEval(-100.0f, redP);

    OCIO_CHECK_CLOSE(rgba[8], resMin, error);
    OCIO_CHECK_EQUAL(rgba[11], 0.0f);

    OCIO_CHECK_CLOSE(rgba[12], res0, error);
    OCIO_CHECK_ASSERT(OCIO::IsNan(rgba[15]));

    //OCIO_CHECK_EQUAL(rgba[16], inf);
    OCIO_CHECK_EQUAL(rgba[19], 0.0f);

    OCIO_CHECK_CLOSE(rgba[20], res0, error);
    OCIO_CHECK_EQUAL(rgba[23], inf);

    OCIO_CHECK_CLOSE(rgba[24], resMin, error);
    OCIO_CHECK_EQUAL(rgba[27], 0.0f);

    OCIO_CHECK_CLOSE(rgba[28], res0, error);
    OCIO_CHECK_EQUAL(rgba[31], -inf);
}

// TODO: Test bitdepth support scaling - (logOp_Log_withScaling_test)
// TODO: Test half support - (logOp_Log_withHalf_test)
// TODO: Test bitdepth support scaling - (logOp_AntiLog_withScaling_test)
// TODO: Test half support - (logOp_AntiLog_withHalf_test)
// TODO: Test bitdepth support scaling - (logOp_Log2Lin_withScaling_test)
// TODO: Test half support - (logOp_Log2Lin_withHalf_test)
// TODO: Test bitdepth support scaling - (logOp_Lin2Log_withScaling_test)
// TODO: Test half support - (logOp_Lin2Log_withHalf_test)
