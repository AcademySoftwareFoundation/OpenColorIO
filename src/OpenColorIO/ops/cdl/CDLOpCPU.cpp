// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "CDLOpCPU.h"
#include "SSE.h"


namespace OCIO_NAMESPACE
{

const float RcpMinValue = 1e-2f;

inline float Reciprocal(float x)
{
    return 1.0f / std::max(x, RcpMinValue);
}

RenderParams::RenderParams()
{
    setSlope (1.0f, 1.0f, 1.0f);
    setOffset(0.0f, 0.0f, 0.0f);
    setPower (1.0f, 1.0f, 1.0f);
    setSaturation(1.0f);
}

void RenderParams::setSlope(float r, float g, float b)
{
    m_slope[0] = r;
    m_slope[1] = g;
    m_slope[2] = b;
    m_slope[3] = 1.f;
}

void RenderParams::setOffset(float r, float g, float b)
{
    m_offset[0] = r;
    m_offset[1] = g;
    m_offset[2] = b;
    m_offset[3] = 0.f;
}

void RenderParams::setPower(float r, float g, float b)
{
    m_power[0] = r;
    m_power[1] = g;
    m_power[2] = b;
    m_power[3] = 1.f;
}

void RenderParams::setSaturation(float sat)
{
    m_saturation = sat;
}

void RenderParams::update(ConstCDLOpDataRcPtr & cdl)
{
    double slope[3], offset[3], power[3];
    cdl->getSlopeParams().getRGB(slope);
    cdl->getOffsetParams().getRGB(offset);
    cdl->getPowerParams().getRGB(power);

    const float saturation = (float)cdl->getSaturation();
    const CDLOpData::Style style = cdl->getStyle();

    m_isReverse = (style == CDLOpData::CDL_V1_2_REV) || (style == CDLOpData::CDL_NO_CLAMP_REV);

    m_isNoClamp = (style == CDLOpData::CDL_NO_CLAMP_FWD) || (style == CDLOpData::CDL_NO_CLAMP_REV);

    if (isReverse())
    {
        // Reverse render parameters
        setSlope(Reciprocal((float)slope[0]),
                 Reciprocal((float)slope[1]),
                 Reciprocal((float)slope[2]));

        setOffset((float)-offset[0], (float)-offset[1], (float)-offset[2]);

        setPower(Reciprocal((float)power[0]),
                 Reciprocal((float)power[1]),
                 Reciprocal((float)power[2]));

        setSaturation(Reciprocal(saturation));
    }
    else
    {
        // Forward render parameters
        setSlope((float)slope[0], (float)slope[1], (float)slope[2]);
        setOffset((float)offset[0], (float)offset[1], (float)offset[2]);
        setPower((float)power[0], (float)power[1], (float)power[2]);
        setSaturation(saturation);
    }
}


#ifdef USE_SSE

static const __m128 LumaWeights = _mm_setr_ps(0.2126f, 0.7152f, 0.0722f, 0.0);

// Load a given pixel from the pixel list into a SSE register
inline __m128 LoadPixel(const float * rgbaBuffer, float & inAlpha)
{
    inAlpha = rgbaBuffer[3];
    return _mm_loadu_ps(rgbaBuffer);
}

// Store the pixel's values into a given pixel list's position
inline void StorePixel(float * rgbaBuffer, const __m128 pix, const float outAlpha)
{
    _mm_storeu_ps(rgbaBuffer, pix);
    rgbaBuffer[3] = outAlpha;
}

// Conditionally clamp the pixel's values to the range [0, 1]
// When the template argument is true, the clamp mode is used,
// and the values in pix are clamped to the range [0,1]. When
// the argument is false, nothing is done.
template<bool>
inline void ApplyClamp(__m128& pix)
{
    pix = _mm_min_ps(_mm_max_ps(pix, EZERO), EONE);
}

template<>
inline void ApplyClamp<false>(__m128&)
{
}

// Apply the power component to the the pixel's values.
// When the template argument is true, the values in pix
// are clamped to the range [0,1] and the power operation is
// applied. When the argument is false, the values in pix are
// not clamped before the power operation is applied. When the
// base is negative in this mode, pixel values are just passed
// through.
template<bool CLAMP>
inline void ApplyPower(__m128& pix, const __m128& power)
{
    ApplyClamp<CLAMP>(pix);
    pix = ssePower(pix, power);
}

template<>
inline void ApplyPower<false>(__m128& pix, const __m128& power)
{
    __m128 negMask = _mm_cmplt_ps(pix, EZERO);
    __m128 pixPower = ssePower(pix, power);
    pix = sseSelect(negMask, pix, pixPower);
}

// Apply the saturation component to the the pixel's values
inline void ApplySaturation(__m128& pix, const __m128 saturation)
{
    // Compute luma: dot product of pixel values and the luma weights
    __m128 luma = _mm_mul_ps(pix, LumaWeights);

    // luma = [ x+y , y+x , z+w , w+z ]
    luma = _mm_add_ps(luma, _mm_shuffle_ps(luma, luma, _MM_SHUFFLE(2,3,0,1)));

    // luma = [ x+y+z+w , y+x+w+z , z+w+x+y , w+z+y+x ]
    luma = _mm_add_ps(luma, _mm_shuffle_ps(luma, luma, _MM_SHUFFLE(1,0,3,2)));

    // Apply saturation
    pix = _mm_add_ps(luma, _mm_mul_ps(saturation, _mm_sub_ps(pix, luma)));
}

#endif // USE_SSE

inline void ApplyScale(float * pix, const float scale)
{
    pix[0] = pix[0] * scale;
    pix[1] = pix[1] * scale;
    pix[2] = pix[2] * scale;
}

// Apply the slope component to the the pixel's values
inline void ApplySlope(float * pix, const float * slope)
{
    pix[0] = pix[0] * slope[0];
    pix[1] = pix[1] * slope[1];
    pix[2] = pix[2] * slope[2];
}

// Apply the offset component to the the pixel's values
inline void ApplyOffset(float * pix, const float * offset)
{
    pix[0] = pix[0] + offset[0];
    pix[1] = pix[1] + offset[1];
    pix[2] = pix[2] + offset[2];
}

// Apply the saturation component to the the pixel's values
inline void ApplySaturation(float * pix, const float saturation)
{
    const float srcpix[3] = { pix[0], pix[1], pix[2] };

    static const float LumaWeights[3] = { 0.2126f, 0.7152f, 0.0722f };

    // Compute luma: dot product of pixel values and the luma weights
    ApplySlope(pix, LumaWeights);

    // luma = x+y+z+w
    const float luma = pix[0] + pix[1] + pix[2];

    // Apply saturation
    pix[0] = luma + saturation * (srcpix[0] - luma);
    pix[1] = luma + saturation * (srcpix[1] - luma);
    pix[2] = luma + saturation * (srcpix[2] - luma);
}

// Conditionally clamp the pixel's values to the range [0, 1]
// When the template argument is true, the clamp mode is used,
// and the values in pix are clamped to the range [0,1]. When
// the argument is false, nothing is done.
template<bool>
inline void ApplyClamp(float * pix)
{
    // NaNs become 0.
    pix[0] = Clamp(pix[0], 0.f, 1.f);
    pix[1] = Clamp(pix[1], 0.f, 1.f);
    pix[2] = Clamp(pix[2], 0.f, 1.f);
}

template<>
inline void ApplyClamp<false>(float *)
{
}

// Apply the power component to the the pixel's values.
// When the template argument is true, the values in pix
// are clamped to the range [0,1] and the power operation is
// applied. When the argument is false, the values in pix are
// not clamped before the power operation is applied. When the
// base is negative in this mode, pixel values are just passed
// through.
template<bool>
inline void ApplyPower(float * pix, const float * power)
{
    ApplyClamp<true>(pix);
    pix[0] = powf(pix[0], power[0]);
    pix[1] = powf(pix[1], power[1]);
    pix[2] = powf(pix[2], power[2]);
}

template<>
inline void ApplyPower<false>(float * pix, const float * power)
{
    // Note: Set NaNs to 0 to match the SSE path.
    pix[0] = IsNan(pix[0]) ? 0.0f : (pix[0]<0.f ? pix[0] : powf(pix[0], power[0]));
    pix[1] = IsNan(pix[1]) ? 0.0f : (pix[1]<0.f ? pix[1] : powf(pix[1], power[1]));
    pix[2] = IsNan(pix[2]) ? 0.0f : (pix[2]<0.f ? pix[2] : powf(pix[2], power[2]));
}

class CDLOpCPU;
typedef OCIO_SHARED_PTR<CDLOpCPU> CDLOpCPURcPtr;

// Base class for the CDL operation renderers
class CDLOpCPU : public OpCPU
{
public:
    CDLOpCPU() = delete;
    CDLOpCPU(ConstCDLOpDataRcPtr & cdl);

protected:
    RenderParams m_renderParams;
};

template<bool CLAMP>
class CDLRendererFwd : public CDLOpCPU
{
public:
    CDLRendererFwd(ConstCDLOpDataRcPtr & cdl)
        : CDLOpCPU(cdl)
    {
    }

    virtual void apply(const void * inImg, void * outImg, long numPixels) const;
};

#ifdef USE_SSE
template<bool CLAMP>
class CDLRendererFwdSSE : public CDLRendererFwd<CLAMP>
{
public:
    CDLRendererFwdSSE(ConstCDLOpDataRcPtr & cdl)
        : CDLRendererFwd<CLAMP>(cdl)
    {
    }

    virtual void apply(const void * inImg, void * outImg, long numPixels) const;
};
#endif

template<bool CLAMP>
class CDLRendererRev : public CDLOpCPU
{
public:
    CDLRendererRev(ConstCDLOpDataRcPtr & cdl)
        : CDLOpCPU(cdl)
    {
    }

    virtual void apply(const void * inImg, void * outImg, long numPixels) const;
};

#ifdef USE_SSE
template<bool CLAMP>
class CDLRendererRevSSE : public CDLRendererRev<CLAMP>
{
public:
    CDLRendererRevSSE(ConstCDLOpDataRcPtr & cdl)
        : CDLRendererRev<CLAMP>(cdl)
    {
    }

    virtual void apply(const void * inImg, void * outImg, long numPixels) const;
};
#endif

CDLOpCPU::CDLOpCPU(ConstCDLOpDataRcPtr & cdl)
    :   OpCPU()
{
    m_renderParams.update(cdl);
}

#ifdef USE_SSE
void LoadRenderParams(const RenderParams & renderParams,
                      __m128 & slope,
                      __m128 & offset,
                      __m128 & power,
                      __m128 & saturation)
{
    slope      = _mm_loadu_ps(renderParams.getSlope());
    offset     = _mm_loadu_ps(renderParams.getOffset());
    power      = _mm_loadu_ps(renderParams.getPower());
    saturation = _mm_set1_ps(renderParams.getSaturation());
}
#endif

#ifdef USE_SSE
template<bool CLAMP>
void CDLRendererFwdSSE<CLAMP>::apply(const void * inImg, void * outImg, long numPixels) const
{
    __m128 slope, offset, power, saturation, pix;
    LoadRenderParams(this->m_renderParams, slope, offset, power, saturation);

    float inAlpha;

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        pix = LoadPixel(in, inAlpha);

        pix = _mm_mul_ps(pix, slope);
        pix = _mm_add_ps(pix, offset);

        ApplyPower<CLAMP>(pix, power);

        ApplySaturation(pix, saturation);
        ApplyClamp<CLAMP>(pix);

        StorePixel(out, pix, inAlpha);

        in  += 4;
        out += 4;
    }
}
#endif

template<bool CLAMP>
void CDLRendererFwd<CLAMP>::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    const float * slope = m_renderParams.getSlope();
    float inSlope[3] = {slope[0], slope[1], slope[2]};

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float inAlpha = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        memcpy(out, in, 4 * sizeof(float));

        ApplySlope(out, inSlope);
        ApplyOffset(out, m_renderParams.getOffset());

        ApplyPower<CLAMP>(out, m_renderParams.getPower());

        ApplySaturation(out, m_renderParams.getSaturation());
        ApplyClamp<CLAMP>(out);

        out[3] = inAlpha;

        in  += 4;
        out += 4;
    }
}

#ifdef USE_SSE
template<bool CLAMP>
void CDLRendererRevSSE<CLAMP>::apply(const void * inImg, void * outImg, long numPixels) const
{
    __m128 slopeRev, offsetRev, powerRev, saturationRev, pix;
    LoadRenderParams(this->m_renderParams, slopeRev, offsetRev, powerRev, saturationRev);

    float inAlpha = 1.0f;

    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        pix = LoadPixel(in, inAlpha);

        ApplyClamp<CLAMP>(pix);
        ApplySaturation(pix, saturationRev);

        ApplyPower<CLAMP>(pix, powerRev);

        pix = _mm_add_ps(pix, offsetRev);
        pix = _mm_mul_ps(pix, slopeRev);
        ApplyClamp<CLAMP>(pix);

        StorePixel(out, pix, inAlpha);

        in  += 4;
        out += 4;
    }
}
#endif

template<bool CLAMP>
void CDLRendererRev<CLAMP>::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float inAlpha = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        memcpy(out, in, 4 * sizeof(float));

        ApplyClamp<CLAMP>(out);
        ApplySaturation(out, m_renderParams.getSaturation());

        ApplyPower<CLAMP>(out, m_renderParams.getPower());

        ApplyOffset(out, m_renderParams.getOffset());
        ApplySlope(out, m_renderParams.getSlope());
        ApplyClamp<CLAMP>(out);

        out[3] = inAlpha;

        in  += 4;
        out += 4;
    }
}

// Note that if power is 1, the optimizer is able to convert the CDL op into a pair of matrices and
// clamp (when needed).  So by default, the following will only get called when power is not 1.
ConstOpCPURcPtr GetCDLCPURenderer(ConstCDLOpDataRcPtr & cdl, bool fastPower)
{
#ifndef USE_SSE
    std::ignore = fastPower;
#endif
    switch(cdl->getStyle())
    {
        case CDLOpData::CDL_V1_2_FWD:
#ifdef USE_SSE
            if (fastPower) return std::make_shared<CDLRendererFwdSSE<true>>(cdl);
            else
#endif
                return std::make_shared<CDLRendererFwd<true>>(cdl);
        case CDLOpData::CDL_NO_CLAMP_FWD:
#ifdef USE_SSE
            if (fastPower) return std::make_shared<CDLRendererFwdSSE<false>>(cdl);
            else
#endif
                return std::make_shared<CDLRendererFwd<false>>(cdl);
        case CDLOpData::CDL_V1_2_REV:
#ifdef USE_SSE
            if (fastPower) return std::make_shared<CDLRendererRevSSE<true>>(cdl);
            else
#endif
                return std::make_shared<CDLRendererRev<true>>(cdl);
        case CDLOpData::CDL_NO_CLAMP_REV:
#ifdef USE_SSE
            if (fastPower) return std::make_shared<CDLRendererRevSSE<false>>(cdl);
            else
#endif
                return std::make_shared<CDLRendererRev<false>>(cdl);
    }

    throw Exception("Unknown CDL style");
    return ConstOpCPURcPtr();
}

} // namespace OCIO_NAMESPACE
