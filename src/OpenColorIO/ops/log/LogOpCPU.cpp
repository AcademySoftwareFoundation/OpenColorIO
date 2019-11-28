// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/log/LogOpCPU.h"
#include "ops/log/LogUtils.h"
#include "ops/OpTools.h"
#include "Platform.h"
#include "SSE.h"

namespace OCIO_NAMESPACE
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

} // namespace OCIO_NAMESPACE

