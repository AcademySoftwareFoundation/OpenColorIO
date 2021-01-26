// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstring>
#include <cmath>
#ifndef USE_SSE
#include <tuple>
#endif

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
    virtual void updateData(ConstLogOpDataRcPtr & log);
};

// Base class for LogToLin and LinToLog renderers.
class L2LBaseRenderer : public LogOpCPU
{
public:
    explicit L2LBaseRenderer(ConstLogOpDataRcPtr & log);

protected:
    void updateData(ConstLogOpDataRcPtr & log) override;

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

protected:
    void updateData(ConstLogOpDataRcPtr & log) override;

    float m_kinv[3];
    float m_minuskb[3];
    float m_minusb[3];
    float m_minv[3];
};

#ifdef USE_SSE
class Log2LinRendererSSE : public Log2LinRenderer
{
public:
    explicit Log2LinRendererSSE(ConstLogOpDataRcPtr & log);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

// Renderer for Lin2Log operations.
class Lin2LogRenderer : public L2LBaseRenderer
{
public:
    explicit Lin2LogRenderer(ConstLogOpDataRcPtr & log);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void updateData(ConstLogOpDataRcPtr & log) override;

    float m_m[3];
    float m_b[3];
    float m_klog[3];
    float m_kb[3];
};

#ifdef USE_SSE
class Lin2LogRendererSSE : public Lin2LogRenderer
{
public:
    explicit Lin2LogRendererSSE(ConstLogOpDataRcPtr & log);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

class CameraL2LBaseRenderer : public L2LBaseRenderer
{
public:
    explicit CameraL2LBaseRenderer(ConstLogOpDataRcPtr & log);

protected:
    void updateData(ConstLogOpDataRcPtr & log) override;

    float m_logSideBreak[3];
    float m_linearSlope[3];
    float m_linearOffset[3];
    float m_log2_base;
};

// Renderer for CameraLogToLin operations.
class CameraLog2LinRenderer : public CameraL2LBaseRenderer
{
public:
    explicit CameraLog2LinRenderer(ConstLogOpDataRcPtr & log);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void updateData(ConstLogOpDataRcPtr & log) override;

    float m_kinv[3];
    float m_minuskb[3];
    float m_minusb[3];
    float m_minv[3];
    float m_linsinv[3];
    float m_minuslino[3];
};

#ifdef USE_SSE
class CameraLog2LinRendererSSE : public CameraLog2LinRenderer
{
public:
    explicit CameraLog2LinRendererSSE(ConstLogOpDataRcPtr & log);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

// Renderer for CameraLin2Log operations.
class CameraLin2LogRenderer : public CameraL2LBaseRenderer
{
public:
    explicit CameraLin2LogRenderer(ConstLogOpDataRcPtr & log);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    void updateData(ConstLogOpDataRcPtr & log) override;

    float m_m[3];
    float m_b[3];
    float m_klog[3];
    float m_kb[3];
    float m_linb[3];
};

#ifdef USE_SSE
class CameraLin2LogRendererSSE : public CameraLin2LogRenderer
{
public:
    explicit CameraLin2LogRendererSSE(ConstLogOpDataRcPtr & log);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

// Renderer for Log10 and Log2 operations.
class LogRenderer : public LogOpCPU
{
public:
    explicit LogRenderer(ConstLogOpDataRcPtr & log, float logScale);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_logScale;
};

#ifdef USE_SSE
class LogRendererSSE : public LogRenderer
{
public:
    explicit LogRendererSSE(ConstLogOpDataRcPtr & log, float logScale);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

// Renderer for AntiLog10 and AntiLog2 operations.
class AntiLogRenderer : public LogOpCPU
{
public:
    explicit AntiLogRenderer(ConstLogOpDataRcPtr & log, float log2base);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_log2_base;
};

#ifdef USE_SSE
class AntiLogRendererSSE : public AntiLogRenderer
{
public:
    explicit AntiLogRendererSSE(ConstLogOpDataRcPtr & log, float log2base);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};
#endif

static constexpr float LOG2_10 = ((float) 3.3219280948873623478703194294894);
static constexpr float LOG10_2 = ((float) 0.3010299956639811952137388947245);

ConstOpCPURcPtr GetLogRenderer(ConstLogOpDataRcPtr & log, bool fastExp)
{
#ifndef USE_SSE
    std::ignore = fastExp;
#endif

    const TransformDirection dir = log->getDirection();
    if (log->isLog2())
    {
        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
#ifdef USE_SSE
            if (fastExp) return std::make_shared<LogRendererSSE>(log, 1.0f);
            else
#endif
                return std::make_shared<LogRenderer>(log, 1.0f);
            break;
        case TRANSFORM_DIR_INVERSE:
#ifdef USE_SSE
            if (fastExp) return std::make_shared<AntiLogRendererSSE>(log, 1.0f);
            else
#endif
                return std::make_shared<AntiLogRenderer>(log, 1.0f);
            break;
        }
    }
    else if (log->isLog10())
    {
        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
#ifdef USE_SSE
            if (fastExp) return std::make_shared<LogRendererSSE>(log, LOG10_2);
            else
#endif
                return std::make_shared<LogRenderer>(log, LOG10_2);
            break;
        case TRANSFORM_DIR_INVERSE:
#ifdef USE_SSE
            if (fastExp) return std::make_shared<AntiLogRendererSSE>(log, LOG2_10);
            else
#endif
                return std::make_shared<AntiLogRenderer>(log, LOG2_10);
            break;
        }
    }
    else
    {
        if (log->isCamera())
        {
            switch (dir)
            {
            case TRANSFORM_DIR_FORWARD:
#ifdef USE_SSE
                if (fastExp) return std::make_shared<CameraLin2LogRendererSSE>(log);
                else
#endif
                    return std::make_shared<CameraLin2LogRenderer>(log);
                break;
            case TRANSFORM_DIR_INVERSE:
#ifdef USE_SSE
                if (fastExp) return std::make_shared<CameraLog2LinRendererSSE>(log);
                else
#endif
                    return std::make_shared<CameraLog2LinRenderer>(log);
                break;
            }
        }
        else
        {
            switch (dir)
            {
            case TRANSFORM_DIR_FORWARD:
#ifdef USE_SSE
                if (fastExp) return std::make_shared<Lin2LogRendererSSE>(log);
                else
#endif
                    return std::make_shared<Lin2LogRenderer>(log);
                break;
            case TRANSFORM_DIR_INVERSE:
#ifdef USE_SSE
                if (fastExp) return std::make_shared<Log2LinRendererSSE>(log);
                else
#endif
                    return std::make_shared<Log2LinRenderer>(log);
                break;
            }
        }
    }
    throw Exception("Illegal Log direction.");
}

LogOpCPU::LogOpCPU(ConstLogOpDataRcPtr & log)
    : OpCPU()
{
}

void LogOpCPU::updateData(ConstLogOpDataRcPtr & log)
{
}


L2LBaseRenderer::L2LBaseRenderer(ConstLogOpDataRcPtr & log)
    : LogOpCPU(log)
{
}

void L2LBaseRenderer::updateData(ConstLogOpDataRcPtr & log)
{
    LogOpCPU::updateData(log);

    m_base = (float)log->getBase();
    m_paramsR = log->getRedParams();
    m_paramsG = log->getGreenParams();
    m_paramsB = log->getBlueParams();
}

LogRenderer::LogRenderer(ConstLogOpDataRcPtr & log, float logScale)
    : LogOpCPU(log)
    , m_logScale(logScale)
{
    LogOpCPU::updateData(log);
}

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

void LogRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    static constexpr float minValue = std::numeric_limits<float>::min();

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float alphares = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        std::memcpy(out, in, 4 * sizeof(float));

        ApplyMax(out, minValue);
        ApplyLog2(out);
        ApplyScale(out, m_logScale);

        out[3] = alphares;

        in  += 4;
        out += 4;
    }
}

#ifdef USE_SSE
LogRendererSSE::LogRendererSSE(ConstLogOpDataRcPtr & log, float logScale)
    : LogRenderer(log, logScale)
{
}
void LogRendererSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    //
    // out = log2( max(in, minValue) ) * logScale;
    //
    static constexpr float minValue = std::numeric_limits<float>::min();

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

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
}
#endif

// Renderer for AntiLog10 and AntiLog2 operations
AntiLogRenderer::AntiLogRenderer(ConstLogOpDataRcPtr & log, float log2base)
    : LogOpCPU(log)
    , m_log2_base(log2base)
{
    LogOpCPU::updateData(log);
}

void AntiLogRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float alphares = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        std::memcpy(out, in, 4 * sizeof(float));

        ApplyScale(out, m_log2_base);
        ApplyExp2(out);

        out[3] = alphares;

        in  += 4;
        out += 4;
    }
}

#ifdef USE_SSE
AntiLogRendererSSE::AntiLogRendererSSE(ConstLogOpDataRcPtr & log, float log2base)
    : AntiLogRenderer(log, log2base)
{
}

void AntiLogRendererSSE::apply(const void * inImg, void * outImg, long numPixels) const
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
}
#endif

// Renderer for LogToLin operations
Log2LinRenderer::Log2LinRenderer(ConstLogOpDataRcPtr & log)
    : L2LBaseRenderer(log)
{
    updateData(log);
}

void Log2LinRenderer::updateData(ConstLogOpDataRcPtr & log)
{
    L2LBaseRenderer::updateData(log);

    m_kinv[0] = log2f(m_base) / (float)m_paramsR[LOG_SIDE_SLOPE];
    m_kinv[1] = log2f(m_base) / (float)m_paramsG[LOG_SIDE_SLOPE];
    m_kinv[2] = log2f(m_base) / (float)m_paramsB[LOG_SIDE_SLOPE];
    m_minuskb[0] = -(float)m_paramsR[LOG_SIDE_OFFSET];
    m_minuskb[1] = -(float)m_paramsG[LOG_SIDE_OFFSET];
    m_minuskb[2] = -(float)m_paramsB[LOG_SIDE_OFFSET];
    m_minusb[0] = -(float)m_paramsR[LIN_SIDE_OFFSET];
    m_minusb[1] = -(float)m_paramsG[LIN_SIDE_OFFSET];
    m_minusb[2] = -(float)m_paramsB[LIN_SIDE_OFFSET];
    m_minv[0] = 1.0f / (float)m_paramsR[LIN_SIDE_SLOPE];
    m_minv[1] = 1.0f / (float)m_paramsG[LIN_SIDE_SLOPE];
    m_minv[2] = 1.0f / (float)m_paramsB[LIN_SIDE_SLOPE];
}


void Log2LinRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float alphares = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        std::memcpy(out, in, 4 * sizeof(float));

        ApplyAdd(out, m_minuskb);
        ApplyScale(out, m_kinv);
        ApplyExp2(out);
        ApplyAdd(out, m_minusb);
        ApplyScale(out, m_minv);

        out[3] = alphares;

        out += 4;
        in  += 4;
    }
}

#ifdef USE_SSE
Log2LinRendererSSE::Log2LinRendererSSE(ConstLogOpDataRcPtr & log)
    : Log2LinRenderer(log)
{

}

void Log2LinRendererSSE::apply(const void * inImg, void * outImg, long numPixels) const
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

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 mm_kinv = _mm_set_ps(0.0f, m_kinv[2], m_kinv[1], m_kinv[0]);
    const __m128 mm_minuskb = _mm_set_ps(0.0f, m_minuskb[2], m_minuskb[1], m_minuskb[0]);
    const __m128 mm_minusb = _mm_set_ps(0.0f, m_minusb[2], m_minusb[1], m_minusb[0]);
    const __m128 mm_minv = _mm_set_ps(0.0f, m_minv[2], m_minv[1], m_minv[0]);

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
}
#endif

// Renderer for Lin2Log operations
Lin2LogRenderer::Lin2LogRenderer(ConstLogOpDataRcPtr & log)
    : L2LBaseRenderer(log)
{
    updateData(log);
}

void Lin2LogRenderer::updateData(ConstLogOpDataRcPtr & log)
{
    L2LBaseRenderer::updateData(log);

    m_m[0] = (float)m_paramsR[LIN_SIDE_SLOPE];
    m_m[1] = (float)m_paramsG[LIN_SIDE_SLOPE];
    m_m[2] = (float)m_paramsB[LIN_SIDE_SLOPE];
    m_b[0] = (float)m_paramsR[LIN_SIDE_OFFSET];
    m_b[1] = (float)m_paramsG[LIN_SIDE_OFFSET];
    m_b[2] = (float)m_paramsB[LIN_SIDE_OFFSET];
    m_klog[0] = (float)(m_paramsR[LOG_SIDE_SLOPE] / log2(m_base));
    m_klog[1] = (float)(m_paramsG[LOG_SIDE_SLOPE] / log2(m_base));
    m_klog[2] = (float)(m_paramsB[LOG_SIDE_SLOPE] / log2(m_base));
    m_kb[0] = (float)m_paramsR[LOG_SIDE_OFFSET];
    m_kb[1] = (float)m_paramsG[LOG_SIDE_OFFSET];
    m_kb[2] = (float)m_paramsB[LOG_SIDE_OFFSET];
}

void Lin2LogRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    static constexpr float minValue = std::numeric_limits<float>::min();

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float alphares = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        std::memcpy(out, in, 4 * sizeof(float));

        ApplyScale(out, m_m);
        ApplyAdd(out, m_b);
        ApplyMax(out, minValue);
        ApplyLog2(out);
        ApplyScale(out, m_klog);
        ApplyAdd(out, m_kb);

        out[3] = alphares;

        out += 4;
        in  += 4;
    }
}

#ifdef USE_SSE
Lin2LogRendererSSE::Lin2LogRendererSSE(ConstLogOpDataRcPtr & log)
    : Lin2LogRenderer(log)
{
}

void Lin2LogRendererSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    // out = ( logSlope * log( base, max( minValue, (in*linSlope + linOffset) ) ) + logOffset )
    //
    // out = log2( max( minValue, (in*linSlope + linOffset) ) ) * logSlope / log2(base) + logOffset
    //
    static constexpr float minValue = std::numeric_limits<float>::min();

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 mm_minValue = _mm_set1_ps(minValue);

    const __m128 mm_m = _mm_set_ps(0.0f, m_m[2], m_m[1], m_m[0]);
    const __m128 mm_b = _mm_set_ps(0.0f, m_b[2], m_b[1], m_b[0]);
    const __m128 mm_klog = _mm_set_ps(0.0f, m_klog[2], m_klog[1], m_klog[0]);
    const __m128 mm_kb = _mm_set_ps(0.0f, m_kb[2], m_kb[1], m_kb[0]);

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
}
#endif

CameraL2LBaseRenderer::CameraL2LBaseRenderer(ConstLogOpDataRcPtr & log)
    : L2LBaseRenderer(log)
{
}

void CameraL2LBaseRenderer::updateData(ConstLogOpDataRcPtr & log)
{
    L2LBaseRenderer::updateData(log);
    m_linearSlope[0] = LogUtil::GetLinearSlope(m_paramsR, m_base);
    m_linearSlope[1] = LogUtil::GetLinearSlope(m_paramsG, m_base);
    m_linearSlope[2] = LogUtil::GetLinearSlope(m_paramsB, m_base);
    m_logSideBreak[0] = LogUtil::GetLogSideBreak(m_paramsR, m_base);
    m_logSideBreak[1] = LogUtil::GetLogSideBreak(m_paramsG, m_base);
    m_logSideBreak[2] = LogUtil::GetLogSideBreak(m_paramsB, m_base);
    m_linearOffset[0] = LogUtil::GetLinearOffset(m_paramsR, m_linearSlope[0], m_logSideBreak[0]);
    m_linearOffset[1] = LogUtil::GetLinearOffset(m_paramsG, m_linearSlope[1], m_logSideBreak[1]);
    m_linearOffset[2] = LogUtil::GetLinearOffset(m_paramsB, m_linearSlope[2], m_logSideBreak[2]);

    m_log2_base = log2((float)m_base);
}

CameraLog2LinRenderer::CameraLog2LinRenderer(ConstLogOpDataRcPtr & log)
    : CameraL2LBaseRenderer(log)
{
    updateData(log);
}

void CameraLog2LinRenderer::updateData(ConstLogOpDataRcPtr & log)
{
    CameraL2LBaseRenderer::updateData(log);

    m_kinv[0] = m_log2_base / (float)m_paramsR[LOG_SIDE_SLOPE];
    m_kinv[1] = m_log2_base / (float)m_paramsG[LOG_SIDE_SLOPE];
    m_kinv[2] = m_log2_base / (float)m_paramsB[LOG_SIDE_SLOPE];
    m_minuskb[0] = -(float)m_paramsR[LOG_SIDE_OFFSET];
    m_minuskb[1] = -(float)m_paramsG[LOG_SIDE_OFFSET];
    m_minuskb[2] = -(float)m_paramsB[LOG_SIDE_OFFSET];
    m_minusb[0] = -(float)m_paramsR[LIN_SIDE_OFFSET];
    m_minusb[1] = -(float)m_paramsG[LIN_SIDE_OFFSET];
    m_minusb[2] = -(float)m_paramsB[LIN_SIDE_OFFSET];
    m_minv[0] = 1.0f / (float)m_paramsR[LIN_SIDE_SLOPE];
    m_minv[1] = 1.0f / (float)m_paramsG[LIN_SIDE_SLOPE];
    m_minv[2] = 1.0f / (float)m_paramsB[LIN_SIDE_SLOPE];
    m_linsinv[0] = 1.0f / m_linearSlope[0];
    m_linsinv[1] = 1.0f / m_linearSlope[1];
    m_linsinv[2] = 1.0f / m_linearSlope[2];
    m_minuslino[0] = -m_linearOffset[0];
    m_minuslino[1] = -m_linearOffset[1];
    m_minuslino[2] = -m_linearOffset[2];
}

void CameraLog2LinRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        const float alphares = in[3];

        for (int i = 0; i < 3; ++i)
        {
            if (in[i] < m_logSideBreak[i])
            {
                out[i] = m_linsinv[i] * (in[i] + m_minuslino[i]);
            }
            else
            {
                out[i] = (in[i] + m_minuskb[i]) * m_kinv[i];
                out[i] = exp2(out[i]);
                out[i] = (out[i] + m_minusb[i]) * m_minv[i];
            }
        }

        out[3] = alphares;

        out += 4;
        in += 4;
    }
}

#ifdef USE_SSE
CameraLog2LinRendererSSE::CameraLog2LinRendererSSE(ConstLogOpDataRcPtr & log)
    : CameraLog2LinRenderer(log)
{
}

void CameraLog2LinRendererSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    // if in <= logBreak
    //  out = ( in - linearOffset ) / linearSlope
    // else
    //  out = ( pow( base, (in - logOffset) / logSlope ) - linOffset ) / linSlope;
    //
    //  out = ( exp2( log2(base)/logSlope * (in - logOffset) ) - linOffset ) / linSlope;
    //

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 mm_kinv = _mm_set_ps(0.0f, m_kinv[2], m_kinv[1], m_kinv[0]);
    const __m128 mm_minuskb = _mm_set_ps(0.0f, m_minuskb[2], m_minuskb[1], m_minuskb[0]);
    const __m128 mm_minusb = _mm_set_ps(0.0f, m_minusb[2], m_minusb[1], m_minusb[0]);
    const __m128 mm_minv = _mm_set_ps(0.0f, m_minv[2], m_minv[1], m_minv[0]);
    const __m128 breakPnt = _mm_set_ps(0.f, m_logSideBreak[2], m_logSideBreak[1], m_logSideBreak[0]);
    const __m128 mm_linoinv = _mm_set_ps(0.0f, m_minuslino[2], m_minuslino[1], m_minuslino[0]);
    const __m128 mm_linsinv = _mm_set_ps(0.0f, m_linsinv[2], m_linsinv[1], m_linsinv[0]);

    __m128 mm_pixel;
    __m128 mm_pixel_lin;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        mm_pixel = _mm_set_ps(0.0f, in[2], in[1], in[0]);
        __m128 flag = _mm_cmpgt_ps(mm_pixel, breakPnt);

        mm_pixel_lin = _mm_add_ps(mm_pixel, mm_linoinv);
        mm_pixel_lin = _mm_mul_ps(mm_pixel_lin, mm_linsinv);

        mm_pixel = _mm_add_ps(mm_pixel, mm_minuskb);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_kinv);
        mm_pixel = sseExp2(mm_pixel);
        mm_pixel = _mm_add_ps(mm_pixel, mm_minusb);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_minv);

        mm_pixel = _mm_or_ps(_mm_and_ps(flag, mm_pixel),
                             _mm_andnot_ps(flag, mm_pixel_lin));

        const float alphares = in[3];

        _mm_storeu_ps(out, mm_pixel);
        out[3] = alphares;

        out += 4;
        in += 4;
    }
}
#endif

CameraLin2LogRenderer::CameraLin2LogRenderer(ConstLogOpDataRcPtr & log)
    : CameraL2LBaseRenderer(log)
{
    updateData(log);
}

void CameraLin2LogRenderer::updateData(ConstLogOpDataRcPtr & log)
{
    CameraL2LBaseRenderer::updateData(log);

    m_m[0] = (float)m_paramsR[LIN_SIDE_SLOPE];
    m_m[1] = (float)m_paramsG[LIN_SIDE_SLOPE];
    m_m[2] = (float)m_paramsB[LIN_SIDE_SLOPE];
    m_b[0] = (float)m_paramsR[LIN_SIDE_OFFSET];
    m_b[1] = (float)m_paramsG[LIN_SIDE_OFFSET];
    m_b[2] = (float)m_paramsB[LIN_SIDE_OFFSET];
    m_klog[0] = (float)(m_paramsR[LOG_SIDE_SLOPE] / m_log2_base);
    m_klog[1] = (float)(m_paramsG[LOG_SIDE_SLOPE] / m_log2_base);
    m_klog[2] = (float)(m_paramsB[LOG_SIDE_SLOPE] / m_log2_base);
    m_kb[0] = (float)m_paramsR[LOG_SIDE_OFFSET];
    m_kb[1] = (float)m_paramsG[LOG_SIDE_OFFSET];
    m_kb[2] = (float)m_paramsB[LOG_SIDE_OFFSET];
    m_linb[0] = (float)m_paramsR[LIN_SIDE_BREAK];
    m_linb[1] = (float)m_paramsG[LIN_SIDE_BREAK];
    m_linb[2] = (float)m_paramsB[LIN_SIDE_BREAK];
}

void CameraLin2LogRenderer::apply(const void * inImg, void * outImg, long numPixels) const
{
    static constexpr float minValue = std::numeric_limits<float>::min();

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float alphares = in[3];

        for (int i = 0; i < 3; ++i)
        {
            if (in[i] < m_linb[i])
            {
                out[i] = m_linearSlope[i] * in[i] + m_linearOffset[i];
            }
            else
            {
                out[i] = in[i] * m_m[i] + m_b[i];
                out[i] = std::max(minValue, out[i]);
                out[i] = log2(out[i]);
                out[i] = out[i] * m_klog[i] + m_kb[i];
            }
        }

        out[3] = alphares;

        out += 4;
        in += 4;
    }
}

#ifdef USE_SSE
CameraLin2LogRendererSSE::CameraLin2LogRendererSSE(ConstLogOpDataRcPtr & log)
    : CameraLin2LogRenderer(log)
{
}

void CameraLin2LogRendererSSE::apply(const void * inImg, void * outImg, long numPixels) const
{
    // if in <= linBreak
    //  out = linearSlope * in + linearOffset 
    // else
    //  out = ( logSlope * log( base, max( minValue, (in*linSlope + linOffset) ) ) + logOffset )
    //
    //  out = log2( max( minValue, (in*linSlope + linOffset) ) ) * logSlope / log2(base) + logOffset
    //
    static constexpr float minValue = std::numeric_limits<float>::min();

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const __m128 mm_minValue = _mm_set1_ps(minValue);

    const __m128 mm_m = _mm_set_ps(0.0f, m_m[2], m_m[1], m_m[0]);
    const __m128 mm_b = _mm_set_ps(0.0f, m_b[2], m_b[1], m_b[0]);
    const __m128 mm_klog = _mm_set_ps(0.0f, m_klog[2], m_klog[1], m_klog[0]);
    const __m128 mm_kb = _mm_set_ps(0.0f, m_kb[2], m_kb[1], m_kb[0]);
    const __m128 mm_lins = _mm_set_ps(0.0f, m_linearSlope[2], m_linearSlope[1], m_linearSlope[0]);
    const __m128 mm_lino = _mm_set_ps(0.0f, m_linearOffset[2], m_linearOffset[1], m_linearOffset[0]);
    const __m128 breakPnt = _mm_set_ps(0.f, m_linb[2], m_linb[1], m_linb[0]);

    __m128 mm_pixel;
    __m128 mm_pixel_lin;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        mm_pixel = _mm_set_ps(0.0f, in[2], in[1], in[0]);
        __m128 flag = _mm_cmpgt_ps(mm_pixel, breakPnt);

        mm_pixel_lin = _mm_mul_ps(mm_pixel, mm_lins);
        mm_pixel_lin = _mm_add_ps(mm_pixel_lin, mm_lino);

        mm_pixel = _mm_mul_ps(mm_pixel, mm_m);
        mm_pixel = _mm_add_ps(mm_pixel, mm_b);
        mm_pixel = _mm_max_ps(mm_pixel, mm_minValue);
        mm_pixel = sseLog2(mm_pixel);
        mm_pixel = _mm_mul_ps(mm_pixel, mm_klog);
        mm_pixel = _mm_add_ps(mm_pixel, mm_kb);

        mm_pixel = _mm_or_ps(_mm_and_ps(flag, mm_pixel),
                             _mm_andnot_ps(flag, mm_pixel_lin));

        const float alphares = in[3];

        _mm_storeu_ps(out, mm_pixel);
        out[3] = alphares;

        out += 4;
        in += 4;
    }
}
#endif

} // namespace OCIO_NAMESPACE

