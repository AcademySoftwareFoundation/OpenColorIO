/*
Copyright (c) 2019 Autodesk Inc., et al.
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

    LogOpCPU(ConstLogOpDataRcPtr & log);

protected:
    // Update renderer parameters.
    virtual void updateData(ConstLogOpDataRcPtr & pL);

protected:
    float m_inScale;
    float m_outScale;
    float m_alphaScale;

private:
    LogOpCPU() = delete;
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

    void apply(float * rgbaBuffer, long numPixels) const override;
};

// Renderer for Lin2Log operations.
class Lin2LogRenderer : public L2LBaseRenderer
{
public:
    explicit Lin2LogRenderer(ConstLogOpDataRcPtr & log);

    void apply(float * rgbaBuffer, long numPixels) const override;

};

// Renderer for Log10 and Log2 operations.
class LogRenderer : public LogOpCPU
{
public:
    explicit LogRenderer(ConstLogOpDataRcPtr & log, float logScale);

    void apply(float * rgbaBuffer, long numPixels) const override;

private:
    float m_logScale;
};

// Renderer for AntiLog10 and AntiLog2 operations.
class AntiLogRenderer : public LogOpCPU
{
public:
    explicit AntiLogRenderer(ConstLogOpDataRcPtr & log, float log2base);

    void apply(float * rgbaBuffer, long numPixels) const override;

private:
    float m_log2_base;
};

static const float LOG2_10 = ((float) 3.3219280948873623478703194294894);
static const float LOG10_2 = ((float) 0.3010299956639811952137388947245);

OpCPURcPtr GetLogRenderer(ConstLogOpDataRcPtr & log)
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
    , m_inScale(1.f)
    , m_outScale(1.f)
    , m_alphaScale(1.f)
{
}

void LogOpCPU::updateData(ConstLogOpDataRcPtr & pL)
{
    m_inScale = (float)(
        1. / GetBitDepthMaxValue(pL->getInputBitDepth()));

    m_outScale = (float)(
        GetBitDepthMaxValue(pL->getOutputBitDepth()));

    m_alphaScale = m_inScale * m_outScale;
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
    pix[0] = std::max(pix[0], minValue);
    pix[1] = std::max(pix[1], minValue);
    pix[2] = std::max(pix[2], minValue);
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

void LogRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    //
    // out = log2( max(in*inScale, minValue) ) * logScale * outScale;
    //
    const float minValue = std::numeric_limits<float>::min();

#ifdef USE_SSE
    const __m128 mm_inScale = _mm_set1_ps(m_inScale);
    const __m128 mm_minValue = _mm_set1_ps(minValue);
    const __m128 mm_outScale = _mm_set1_ps(m_outScale * m_logScale);

    __m128 mm_pixel;

    // TODO: Add the ability to not overwrite the input buffer.
    // At that time, memory align the output buffer (see OCIO_ALIGN).
    float * rgba = rgbaBuffer;
    
    for (long idx = 0; idx<numPixels; ++idx)
    {
        mm_pixel = _mm_set_ps(0.0f, rgba[2], rgba[1], rgba[0]);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_inScale);
        mm_pixel = _mm_max_ps(mm_pixel, mm_minValue);
        mm_pixel = sseLog2(mm_pixel);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_outScale);

        const float alphares = rgba[3] * m_alphaScale;

        _mm_storeu_ps(rgba, mm_pixel);
        rgba[3] = alphares;

        rgba += 4;
    }
#else
    const float outScale = m_outScale * m_logScale;

    // TODO: Add the ability to not overwrite the input buffer.
    // At that time, memory align the output buffer (see OCIO_ALIGN).
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        ApplyScale(rgba, m_inScale);
        ApplyMax(rgba, minValue);
        ApplyLog2(rgba);
        ApplyScale(rgba, outScale);

        rgba[3] = rgba[3] * m_alphaScale;

        rgba += 4;
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

void AntiLogRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    //
    // out = pow(base, in*inScale) * outScale;
    //
    // This computation is decomposed into:
    //   out = exp2( log2(base) * (in*inScale) ) * outScale;
    //   so that the constant factor log2(base) can be moved outside the loop.
    //
#ifdef USE_SSE
    const __m128 mm_inScale = _mm_set1_ps(m_inScale);
    const __m128 mm_outScale = _mm_set1_ps(m_outScale);
    const __m128 mm_log2_base = _mm_set1_ps(m_log2_base);

    __m128 mm_pixel;

    // TODO: Add the ability to not overwrite the input buffer.
    // At that time, memory align the output buffer (see OCIO_ALIGN).
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        mm_pixel = _mm_set_ps(0.0f, rgba[2], rgba[1], rgba[0]);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_inScale);
        mm_pixel = sseExp2(_mm_mul_ps(mm_pixel, mm_log2_base));
        mm_pixel = _mm_mul_ps(mm_pixel, mm_outScale);

        const float alphares = rgba[3] * m_alphaScale;

        _mm_storeu_ps(rgba, mm_pixel);
        rgba[3] = alphares;

        rgba += 4;
    }
#else
    const float inScale = m_inScale * m_log2_base;

    // TODO: Add the ability to not overwrite the input buffer.
    // At that time, memory align the output buffer (see OCIO_ALIGN).
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        ApplyScale(rgba, inScale);
        ApplyExp2(rgba);
        ApplyScale(rgba, m_outScale);

        rgba[3] = rgba[3] * m_alphaScale;

        rgba += 4;
    }
#endif
}

// Renderer for LogToLin operations
Log2LinRenderer::Log2LinRenderer(ConstLogOpDataRcPtr & log)
    : L2LBaseRenderer(log)
{
    updateData(log);
}

void Log2LinRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    //
    // out = ( pow( base, (in*inScale - logOffset) / logSlope ) - linOffset )
    //       * outScale / linSlope;
    //
    // out = ( exp2( log2(base)*inScale/logSlope * (in - logOffset/inScale) ) - linOffset )
    //       * outScale / linSlope;
    //
    // The power computation is decomposed into:
    //   pow(base, exponent) = exp2( log2(base) * exponent )
    //   so that the constant factor log2(base) can be moved outside the loop.
    //
    const float inscalekinv[] = {
        m_inScale * log2f(m_base) / (float)m_paramsR[LOG_SIDE_SLOPE],
        m_inScale * log2f(m_base) / (float)m_paramsG[LOG_SIDE_SLOPE],
        m_inScale * log2f(m_base) / (float)m_paramsB[LOG_SIDE_SLOPE] };
    const float minuskb[] = {
        -(float)m_paramsR[LOG_SIDE_OFFSET] / m_inScale,
        -(float)m_paramsG[LOG_SIDE_OFFSET] / m_inScale,
        -(float)m_paramsB[LOG_SIDE_OFFSET] / m_inScale };
    const float minusb[] = {
        -(float)m_paramsR[LIN_SIDE_OFFSET],
        -(float)m_paramsG[LIN_SIDE_OFFSET],
        -(float)m_paramsB[LIN_SIDE_OFFSET] };
    const float outscaleminv[] = {
        m_outScale / (float)m_paramsR[LIN_SIDE_SLOPE],
        m_outScale / (float)m_paramsG[LIN_SIDE_SLOPE],
        m_outScale / (float)m_paramsB[LIN_SIDE_SLOPE] };

#ifdef USE_SSE
    const __m128 mm_inscalekinv = _mm_set_ps(
        0.0f, inscalekinv[2], inscalekinv[1], inscalekinv[0]);
    const __m128 mm_minuskb = _mm_set_ps(
        0.0f, minuskb[2], minuskb[1], minuskb[0]);
    const __m128 mm_minusb = _mm_set_ps(
        0.0f, minusb[2], minusb[1], minusb[0]);
    const __m128 mm_outscaleminv = _mm_set_ps(
        0.0f, outscaleminv[2], outscaleminv[1], outscaleminv[0]);

    __m128 mm_pixel;

    // TODO: Add the ability to not overwrite the input buffer.
    // At that time, memory align the output buffer (see OCIO_ALIGN).
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        mm_pixel = _mm_set_ps(0.0f, rgba[2], rgba[1], rgba[0]);
        mm_pixel = _mm_add_ps(mm_pixel, mm_minuskb);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_inscalekinv);
        mm_pixel = sseExp2(mm_pixel);
        mm_pixel = _mm_add_ps(mm_pixel, mm_minusb);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_outscaleminv);

        const float alphares = rgba[3] * m_alphaScale;

        _mm_storeu_ps(rgba, mm_pixel);
        rgba[3] = alphares;

        rgba += 4;
    }
#else
    // TODO: Add the ability to not overwrite the input buffer.
    // At that time, memory align the output buffer (see OCIO_ALIGN).
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        ApplyAdd(rgba, minuskb);
        ApplyScale(rgba, inscalekinv);
        ApplyExp2(rgba);
        ApplyAdd(rgba, minusb);
        ApplyScale(rgba, outscaleminv);

        rgba[3] = rgba[3] * m_alphaScale;

        rgba += 4;
    }
#endif
}

// Renderer for Lin2Log operations
Lin2LogRenderer::Lin2LogRenderer(ConstLogOpDataRcPtr & log)
    : L2LBaseRenderer(log)
{
    updateData(log);
}

void Lin2LogRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    // out = ( logSlope * log( base, max( minValue, (in*linSlope*inScale + linOffset) ) ) + logOffset ) * outscale
    //
    // out = log2( max( minValue, (in*linSlope*inScale + linOffset) ) ) * logSlope * outscale / log2(base) 
    //       + logOffset * outscale
    //
    const float minValue = std::numeric_limits<float>::min();

    const float inscalem[] = {
        m_inScale * (float)m_paramsR[LIN_SIDE_SLOPE],
        m_inScale * (float)m_paramsG[LIN_SIDE_SLOPE],
        m_inScale * (float)m_paramsB[LIN_SIDE_SLOPE] };
    const float b[] = {
        (float)m_paramsR[LIN_SIDE_OFFSET],
        (float)m_paramsG[LIN_SIDE_OFFSET],
        (float)m_paramsB[LIN_SIDE_OFFSET] };
    const float klogoutscale[] = {
        (float)(m_outScale * m_paramsR[LOG_SIDE_SLOPE] / log2(m_base)),
        (float)(m_outScale * m_paramsG[LOG_SIDE_SLOPE] / log2(m_base)),
        (float)(m_outScale * m_paramsB[LOG_SIDE_SLOPE] / log2(m_base)) };
    const float kboutscale[] = {
        (float)m_paramsR[LOG_SIDE_OFFSET] * m_outScale,
        (float)m_paramsG[LOG_SIDE_OFFSET] * m_outScale,
        (float)m_paramsB[LOG_SIDE_OFFSET] * m_outScale };

#ifdef USE_SSE
    const __m128 mm_minValue = _mm_set1_ps(minValue);

    const __m128 mm_inscalem = _mm_set_ps(
        0.0f, inscalem[2], inscalem[1], inscalem[0]);
    const __m128 mm_b = _mm_set_ps(
        0.0f, b[2], b[1], b[0]);
    const __m128 mm_klogoutscale = _mm_set_ps(
        0.0f, klogoutscale[2], klogoutscale[1], klogoutscale[0]);
    const __m128 mm_kboutscale = _mm_set_ps(
        0.0f, kboutscale[2], kboutscale[1], kboutscale[0]);

    __m128 mm_pixel;

    // TODO: Add the ability to not overwrite the input buffer.
    // At that time, memory align the output buffer (see OCIO_ALIGN).
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        mm_pixel = _mm_set_ps(0.0f, rgba[2], rgba[1], rgba[0]);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_inscalem);
        mm_pixel = _mm_add_ps(mm_pixel, mm_b);
        mm_pixel = _mm_max_ps(mm_pixel, mm_minValue);
        mm_pixel = sseLog2(mm_pixel);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_klogoutscale);
        mm_pixel = _mm_add_ps(mm_pixel, mm_kboutscale);

        const float alphares = rgba[3] * m_alphaScale;

        _mm_storeu_ps(rgba, mm_pixel);
        rgba[3] = alphares;

        rgba += 4;
    }
#else
    // TODO: Add the ability to not overwrite the input buffer.
    // At that time, memory align the output buffer (see OCIO_ALIGN).
    float * rgba = rgbaBuffer;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        ApplyScale(rgba, inscalem);
        ApplyAdd(rgba, b);
        ApplyMax(rgba, minValue);
        ApplyLog2(rgba);
        ApplyScale(rgba, klogoutscale);
        ApplyAdd(rgba, kboutscale);

        rgba[3] = rgba[3] * m_alphaScale;

        rgba += 4;
    }
#endif
}

}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

void TestLog(float logBase)
{
    const float rgbaImage[8] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                 0.2f,       0.f,   .99f, 128.f };
    float rgba[8] = {0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f};
    memcpy(rgba, &rgbaImage[0], 8 * sizeof(float));

    OCIO::ConstLogOpDataRcPtr logOp = std::make_shared<OCIO::LogOpData>(
        logBase, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::OpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp);
    pRenderer->apply(rgba, 2);

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

        OIIO_CHECK_CLOSE(result, expected, error);
    }
}

OIIO_ADD_TEST(LogOpCPU, log_test)
{
    // Log base 10 case, no scaling.
    TestLog(10.0f);

    // Log base 2 case, no scaling.
    TestLog(2.0f);
}

void TestAntiLog(float logBase)
{
    const float rgbaImage[8] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                 0.2f,       0.f,   .99f, 128.f };
    float rgba[8] = { 0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f };
    memcpy(rgba, &rgbaImage[0], 8 * sizeof(float));

    OCIO::ConstLogOpDataRcPtr logOp = std::make_shared<OCIO::LogOpData>(
        logBase, OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::OpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp);
    pRenderer->apply(rgba, 2);

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
        OIIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(result, expected, rtol, 1.0f));
    }
}

OIIO_ADD_TEST(LogOpCPU, anti_log_test)
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

OIIO_ADD_TEST(LogOpCPU, log2lin_test)
{
    const float rgbaImage[8] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                 0.2f,       0.f,   .99f, 128.f };

    float rgba[8] = { 0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f };
    memcpy(rgba, &rgbaImage[0], 8 * sizeof(float));

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
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
        dir, base, paramsR, paramsG, paramsB);

    OCIO::OpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp);
    pRenderer->apply(rgba, 2);

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
        OIIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(result, expected, rtol, 1.0f));
    }
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

OIIO_ADD_TEST(LogOpCPU, lin2log_test)
{
    const float rgbaImage[8] = { 0.0367126f, 0.5f, 1.f,     0.f,
                                 0.2f,       0.f,   .99f, 128.f };

    float rgba[8] = { 0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f };
    memcpy(rgba, &rgbaImage[0], 8 * sizeof(float));

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
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
        dir, base, paramsR, paramsG, paramsB);

    OCIO::OpCPURcPtr pRenderer = OCIO::GetLogRenderer(logOp);
    pRenderer->apply(rgba, 2);

    const OCIO::LogUtil::CTFParams::Params noParam;

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
        const float error = 1e-4f;
        OIIO_CHECK_CLOSE(result, expected, error);
    }
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