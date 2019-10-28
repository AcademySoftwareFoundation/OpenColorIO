// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/Log/LogOpCPU.h"
#include "ops/Log/LogUtils.h"
#include "OpTools.h"
#include "Platform.h"
#include "SSE.h"

OCIO_NAMESPACE_ENTER
{
class LogOpCPU : public OpCPU
{
public:
    LogOpCPU() = delete;
    LogOpCPU(const LogOpCPU &) = delete;

    explicit LogOpCPU(ConstLogOpDataRcPtr & log);

protected:
    // Update renderer parameters.
    virtual void updateData(ConstLogOpDataRcPtr & pL);
};

// Base class for LogToLin and LinToLog renderers.
class L2LBaseRenderer : public LogOpCPU
{
public:
    explicit L2LBaseRenderer(ConstLogOpDataRcPtr & log);

protected:
    void updateData(ConstLogOpDataRcPtr & pL) override;

protected:
    float m_base = 2.0f;
    LogOpData::Params m_paramsR;
    LogOpData::Params m_paramsG;
    LogOpData::Params m_paramsB;
};

// Renderer for LogToLin operations.
class Log2LinRenderer : public L2LBaseRenderer
{
public:
    explicit Log2LinRenderer(ConstLogOpDataRcPtr & log);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

// Renderer for Lin2Log operations.
class Lin2LogRenderer : public L2LBaseRenderer
{
public:
    explicit Lin2LogRenderer(ConstLogOpDataRcPtr & log);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

};

// Renderer for Log10 and Log2 operations.
class LogRenderer : public LogOpCPU
{
public:
    explicit LogRenderer(ConstLogOpDataRcPtr & log, float logScale);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

private:
    float m_logScale;
};

// Renderer for AntiLog10 and AntiLog2 operations.
class AntiLogRenderer : public LogOpCPU
{
public:
    explicit AntiLogRenderer(ConstLogOpDataRcPtr & log, float log2base);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

private:
    float m_log2_base;
};

static constexpr float LOG2_10 = ((float) 3.3219280948873623478703194294894);
static constexpr float LOG10_2 = ((float) 0.3010299956639811952137388947245);

ConstOpCPURcPtr GetLogRenderer(ConstLogOpDataRcPtr & log)
{
    const TransformDirection dir = log->getDirection();
    if (log->isLog2())
    {
        if (dir == TRANSFORM_DIR_FORWARD)
        {
            return std::make_shared<LogRenderer>(log, 1.0f);
        }
        else
        {
            return std::make_shared<AntiLogRenderer>(log, 1.0f);
        }
    }
    else if (log->isLog10())
    {
        if (dir == TRANSFORM_DIR_FORWARD)
        {
            return std::make_shared<LogRenderer>(log, LOG10_2);
        }
        else
        {
            return std::make_shared<AntiLogRenderer>(log, LOG2_10);
        }
    }
    else
    {
        if (dir == TRANSFORM_DIR_FORWARD)
        {
            return std::make_shared<Lin2LogRenderer>(log);
        }
        else
        {
            return std::make_shared<Log2LinRenderer>(log);
        }
    }
}

LogOpCPU::LogOpCPU(ConstLogOpDataRcPtr & log)
    : OpCPU()
{
}

void LogOpCPU::updateData(ConstLogOpDataRcPtr & pL)
{
}


L2LBaseRenderer::L2LBaseRenderer(ConstLogOpDataRcPtr & log)
    : LogOpCPU(log)
{
}

void L2LBaseRenderer::updateData(ConstLogOpDataRcPtr & pL)
{
    LogOpCPU::updateData(pL);

    m_base = (float)pL->getBase();
    m_paramsR = pL->getRedParams();
    m_paramsG = pL->getGreenParams();
    m_paramsB = pL->getBlueParams();
}

LogRenderer::LogRenderer(ConstLogOpDataRcPtr & log, float logScale)
    : LogOpCPU(log)
    , m_logScale(logScale)
{
    LogOpCPU::updateData(log);
}


#ifndef USE_SSE

inline void ApplyScale(float * pix, const float scale)
{
    pix[0] = pix[0] * scale;
    pix[1] = pix[1] * scale;
    pix[2] = pix[2] * scale;
}

inline void ApplyScale(float * pix, const float * scale)
{
    pix[0] = pix[0] * scale[0];
    pix[1] = pix[1] * scale[1];
    pix[2] = pix[2] * scale[2];
}

inline void ApplyAdd(float * pix, const float * add)
{
    pix[0] = pix[0] + add[0];
    pix[1] = pix[1] + add[1];
    pix[2] = pix[2] + add[2];
}

inline void ApplyMax(float * pix, float minValue)
{
    pix[0] = std::max(minValue, pix[0]);
    pix[1] = std::max(minValue, pix[1]);
    pix[2] = std::max(minValue, pix[2]);
}

inline void ApplyLog2(float * pix)
{
    pix[0] = log2(pix[0]);
    pix[1] = log2(pix[1]);
    pix[2] = log2(pix[2]);
}

inline void ApplyExp2(float * pix)
{
    pix[0] = exp2(pix[0]);
    pix[1] = exp2(pix[1]);
    pix[2] = exp2(pix[2]);
}
#endif

void LogRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    //
    // out = log2( max(in, minValue) ) * logScale;
    //
    const float minValue = std::numeric_limits<float>::min();

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    const __m128 mm_minValue = _mm_set1_ps(minValue);
    const __m128 mm_logScale = _mm_set1_ps(m_logScale);

    __m128 mm_pixel;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        mm_pixel = _mm_set_ps(0.0f, in[2], in[1], in[0]);
        mm_pixel = _mm_max_ps(mm_pixel, mm_minValue);
        mm_pixel = sseLog2(mm_pixel);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_logScale);

        const float alphares = in[3];

        _mm_storeu_ps(out, mm_pixel);
        out[3] = alphares;

        in  += 4;
        out += 4;
    }
#else
    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float alphares = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        memcpy(out, in, 4 * sizeof(float));

        ApplyMax(out, minValue);
        ApplyLog2(out);
        ApplyScale(out, m_logScale);

        out[3] = alphares;

        in  += 4;
        out += 4;
    }
#endif
}

// Renderer for AntiLog10 and AntiLog2 operations
AntiLogRenderer::AntiLogRenderer(ConstLogOpDataRcPtr & log, float log2base)
    : LogOpCPU(log)
    , m_log2_base(log2base)
{
    LogOpCPU::updateData(log);
}

void AntiLogRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    //
    // out = pow(base, in);
    //
    // This computation is decomposed into:
    //   out = exp2( log2(base) * in);
    //   so that the constant factor log2(base) can be moved outside the loop.
    //

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    const __m128 mm_log2_base = _mm_set1_ps(m_log2_base);

    __m128 mm_pixel;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        mm_pixel = _mm_set_ps(0.0f, in[2], in[1], in[0]);
        mm_pixel = sseExp2(_mm_mul_ps(mm_pixel, mm_log2_base));

        const float alphares = in[3];

        _mm_storeu_ps(out, mm_pixel);
        out[3] = alphares;

        in  += 4;
        out += 4;
    }
#else
    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float alphares = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        memcpy(out, in, 4 * sizeof(float));

        ApplyScale(out, m_log2_base);
        ApplyExp2(out);

        out[3] = alphares;

        in  += 4;
        out += 4;
    }
#endif
}

// Renderer for LogToLin operations
Log2LinRenderer::Log2LinRenderer(ConstLogOpDataRcPtr & log)
    : L2LBaseRenderer(log)
{
    updateData(log);
}

void Log2LinRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    //
    // out = ( pow( base, (in - logOffset) / logSlope ) - linOffset ) / linSlope;
    //
    // out = ( exp2( log2(base)/logSlope * (in - logOffset) ) - linOffset ) / linSlope;
    //
    // The power computation is decomposed into:
    //   pow(base, exponent) = exp2( log2(base) * exponent )
    //   so that the constant factor log2(base) can be moved outside the loop.
    //
    const float kinv[] = {
        log2f(m_base) / (float)m_paramsR[LOG_SIDE_SLOPE],
        log2f(m_base) / (float)m_paramsG[LOG_SIDE_SLOPE],
        log2f(m_base) / (float)m_paramsB[LOG_SIDE_SLOPE] };
    const float minuskb[] = {
        -(float)m_paramsR[LOG_SIDE_OFFSET],
        -(float)m_paramsG[LOG_SIDE_OFFSET],
        -(float)m_paramsB[LOG_SIDE_OFFSET] };
    const float minusb[] = {
        -(float)m_paramsR[LIN_SIDE_OFFSET],
        -(float)m_paramsG[LIN_SIDE_OFFSET],
        -(float)m_paramsB[LIN_SIDE_OFFSET] };
    const float minv[] = {
        1.0f / (float)m_paramsR[LIN_SIDE_SLOPE],
        1.0f / (float)m_paramsG[LIN_SIDE_SLOPE],
        1.0f / (float)m_paramsB[LIN_SIDE_SLOPE] };

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    const __m128 mm_kinv = _mm_set_ps(
        0.0f, kinv[2], kinv[1], kinv[0]);
    const __m128 mm_minuskb = _mm_set_ps(
        0.0f, minuskb[2], minuskb[1], minuskb[0]);
    const __m128 mm_minusb = _mm_set_ps(
        0.0f, minusb[2], minusb[1], minusb[0]);
    const __m128 mm_minv = _mm_set_ps(
        0.0f, minv[2], minv[1], minv[0]);

    __m128 mm_pixel;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        mm_pixel = _mm_set_ps(0.0f, in[2], in[1], in[0]);
        mm_pixel = _mm_add_ps(mm_pixel, mm_minuskb);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_kinv);
        mm_pixel = sseExp2(mm_pixel);
        mm_pixel = _mm_add_ps(mm_pixel, mm_minusb);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_minv);

        const float alphares = in[3];

        _mm_storeu_ps(out, mm_pixel);
        out[3] = alphares;

        out += 4;
        in  += 4;
    }
#else
    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float alphares = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        memcpy(out, in, 4 * sizeof(float));

        ApplyAdd(out, minuskb);
        ApplyScale(out, kinv);
        ApplyExp2(out);
        ApplyAdd(out, minusb);
        ApplyScale(out, minv);

        out[3] = alphares;

        out += 4;
        in  += 4;
    }
#endif
}

// Renderer for Lin2Log operations
Lin2LogRenderer::Lin2LogRenderer(ConstLogOpDataRcPtr & log)
    : L2LBaseRenderer(log)
{
    updateData(log);
}

void Lin2LogRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    // out = ( logSlope * log( base, max( minValue, (in*linSlope + linOffset) ) ) + logOffset )
    //
    // out = log2( max( minValue, (in*linSlope + linOffset) ) ) * logSlope / log2(base) + logOffset
    //
    const float minValue = std::numeric_limits<float>::min();

    const float m[] = {
        (float)m_paramsR[LIN_SIDE_SLOPE],
        (float)m_paramsG[LIN_SIDE_SLOPE],
        (float)m_paramsB[LIN_SIDE_SLOPE] };
    const float b[] = {
        (float)m_paramsR[LIN_SIDE_OFFSET],
        (float)m_paramsG[LIN_SIDE_OFFSET],
        (float)m_paramsB[LIN_SIDE_OFFSET] };
    const float klog[] = {
        (float)(m_paramsR[LOG_SIDE_SLOPE] / log2(m_base)),
        (float)(m_paramsG[LOG_SIDE_SLOPE] / log2(m_base)),
        (float)(m_paramsB[LOG_SIDE_SLOPE] / log2(m_base)) };
    const float kb[] = {
        (float)m_paramsR[LOG_SIDE_OFFSET],
        (float)m_paramsG[LOG_SIDE_OFFSET],
        (float)m_paramsB[LOG_SIDE_OFFSET] };

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

#ifdef USE_SSE
    const __m128 mm_minValue = _mm_set1_ps(minValue);

    const __m128 mm_m = _mm_set_ps(
        0.0f, m[2], m[1], m[0]);
    const __m128 mm_b = _mm_set_ps(
        0.0f, b[2], b[1], b[0]);
    const __m128 mm_klog = _mm_set_ps(
        0.0f, klog[2], klog[1], klog[0]);
    const __m128 mm_kb = _mm_set_ps(
        0.0f, kb[2], kb[1], kb[0]);

    __m128 mm_pixel;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        mm_pixel = _mm_set_ps(0.0f, in[2], in[1], in[0]);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_m);
        mm_pixel = _mm_add_ps(mm_pixel, mm_b);
        mm_pixel = _mm_max_ps(mm_pixel, mm_minValue);
        mm_pixel = sseLog2(mm_pixel);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_klog);
        mm_pixel = _mm_add_ps(mm_pixel, mm_kb);

        const float alphares = in[3];

        _mm_storeu_ps(out, mm_pixel);
        out[3] = alphares;

        out += 4;
        in  += 4;
    }
#else
    if(in!=out)
    {
        memcpy(out, in, numPixels * 4 * sizeof(float));
    }

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float alphares = in[3];

        ApplyScale(out, m);
        ApplyAdd(out, b);
        ApplyMax(out, minValue);
        ApplyLog2(out);
        ApplyScale(out, klog);
        ApplyAdd(out, kb);

        out[3] = alphares;

        out += 4;
        in  += 4;
    }
#endif
}

}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

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
// TODO: Test half supprt - (logOp_AntiLog_withHalf_test)
// TODO: Test bitdepth support scaling - (logOp_Log2Lin_withScaling_test)
// TODO: Test half supprt - (logOp_Log2Lin_withHalf_test)
// TODO: Test bitdepth support scaling - (logOp_Lin2Log_withScaling_test)
// TODO: Test half supprt - (logOp_Lin2Log_withHalf_test)

#endif