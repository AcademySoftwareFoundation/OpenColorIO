/*
Copyright (c) 2018 Autodesk Inc., et al.
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
#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "CDLOpCPU.h"
#include "SSE.h"


OCIO_NAMESPACE_ENTER
{

const float RcpMinValue = 1e-2f;

inline float Reciprocal(float x)
{
    return 1.0f / std::max(x, RcpMinValue);
}

RenderParams::RenderParams()
    : m_isReverse(false)
    , m_isNoClamp(false)
{
    setSlope (1.0f, 1.0f, 1.0f, 1.0f);
    setOffset(0.0f, 0.0f, 0.0f, 1.0f);
    setPower (1.0f, 1.0f, 1.0f, 1.0f);
    setSaturation(1.0f);
}

void RenderParams::setSlope(float r, float g, float b, float a)
{
    m_slope[0] = r;
    m_slope[1] = g;
    m_slope[2] = b;
    m_slope[3] = a;
}

void RenderParams::setOffset(float r, float g, float b, float a)
{
    m_offset[0] = r;
    m_offset[1] = g;
    m_offset[2] = b;
    m_offset[3] = a;
}

void RenderParams::setPower(float r, float g, float b, float a)
{
    m_power[0] = r;
    m_power[1] = g;
    m_power[2] = b;
    m_power[3] = a;
}

void RenderParams::setSaturation(float sat)
{
    m_saturation = sat;
}

void RenderParams::update(ConstCDLOpDataRcPtr & cdl)
{
    double slope[4], offset[4], power[4];
    cdl->getSlopeParams().getRGBA(slope);
    cdl->getOffsetParams().getRGBA(offset);
    cdl->getPowerParams().getRGBA(power);

    const float saturation = (float)cdl->getSaturation();
    const CDLOpData::Style style = cdl->getStyle();

    m_isReverse
        = (style == CDLOpData::CDL_V1_2_REV)
        || (style == CDLOpData::CDL_NO_CLAMP_REV);

    m_isNoClamp
        = (style == CDLOpData::CDL_NO_CLAMP_FWD)
        || (style == CDLOpData::CDL_NO_CLAMP_REV);

    if (isReverse())
    {
        // Reverse render parameters
        setSlope(Reciprocal((float)slope[0]),
                 Reciprocal((float)slope[1]),
                 Reciprocal((float)slope[2]),
                 Reciprocal((float)slope[3]));
   
        setOffset((float)-offset[0], (float)-offset[1], (float)-offset[2], (float)-offset[3]);
   
        setPower(Reciprocal((float)power[0]),
                 Reciprocal((float)power[1]),
                 Reciprocal((float)power[2]),
                 Reciprocal((float)power[3]));
       
        setSaturation(Reciprocal(saturation));
    }
    else
    {
        // Forward render parameters
        setSlope((float)slope[0], (float)slope[1], (float)slope[2], (float)slope[3]);
        setOffset((float)offset[0], (float)offset[1], (float)offset[2], (float)offset[3]);
        setPower((float)power[0], (float)power[1], (float)power[2], (float)power[3]);
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

// Map pixel values from the input domain to the unit domain
inline void ApplyInScale(__m128 & pix, const __m128 inScale)
{
    pix = _mm_mul_ps(pix, inScale);
}

// Map result from unit domain to the output domain
inline void ApplyOutScale(__m128& pix, const __m128 outScale)
{
    pix = _mm_mul_ps(pix, outScale);
}

// Apply the slope component to the the pixel's values
inline void ApplySlope(__m128& pix, const __m128 slope)
{
    pix = _mm_mul_ps(pix, slope);
}

// Apply the offset component to the the pixel's values
inline void ApplyOffset(__m128& pix, const __m128 offset)
{
    pix = _mm_add_ps(pix, offset);
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

// Apply the power component to the the pixel's values
// When the template argument is true, the the values in pix
// are clamped to the range [0,1] and the power operation is
// applied. When the argument is false, the values in pix are
// not clamped before the power operation is applied. When the
// base is negative in this mode, pixel values are just passed
// through.
template<bool>
inline void ApplyPower(__m128& pix, const __m128& power)
{
    ApplyClamp<true>(pix);
    pix = ssePower(pix, power);
}

template<>
inline void ApplyPower<false>(__m128& pix, const __m128& power)
{
    __m128 negMask = _mm_cmplt_ps(pix, EZERO);
    __m128 pixPower = ssePower(pix, power);
    pix = sseSelect(pixPower, pix, negMask);
}

// Apply the saturation component to the the pixel's values
inline void ApplySaturation(__m128& pix, const __m128 saturation)
{
    // Compute luma: dot product of pixel values and the luma weigths
    __m128 luma = _mm_mul_ps(pix, LumaWeights);

    // luma = [ x+y , y+x , z+w , w+z ]
    luma = _mm_add_ps(luma, _mm_shuffle_ps(luma, luma, _MM_SHUFFLE(2,3,0,1)));

    // luma = [ x+y+z+w , y+x+w+z , z+w+x+y , w+z+y+x ]
    luma = _mm_add_ps(luma, _mm_shuffle_ps(luma, luma, _MM_SHUFFLE(1,0,3,2)));

    // Apply saturation
    pix = _mm_add_ps(luma, _mm_mul_ps(saturation, _mm_sub_ps(pix, luma)));
}

#else // USE_SSE

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

    // Compute luma: dot product of pixel values and the luma weigths
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

// Apply the power component to the the pixel's values
// When the template argument is true, the the values in pix
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

#endif // USE_SSE


CDLOpCPU::CDLOpCPU(ConstCDLOpDataRcPtr & cdl)
    :   OpCPU()
    ,   m_inScale(1.0f)
    ,   m_outScale(1.0f)
    ,   m_alphaScale(1.0f)
{
    m_inScale = 1.0f / GetBitDepthMaxValue(cdl->getInputBitDepth());
    m_outScale = GetBitDepthMaxValue(cdl->getOutputBitDepth());
    m_alphaScale = m_inScale * m_outScale;

    m_renderParams.update(cdl);
}

#ifdef USE_SSE
void LoadRenderParams(float inScaleVal,
                      float outScaleVal,
                      const RenderParams & renderParams,
                      __m128 & inScale,
                      __m128 & outScale,
                      __m128 & slope,
                      __m128 & offset,
                      __m128 & power,
                      __m128 & saturation)
{
    inScale    = _mm_set1_ps(inScaleVal);
    outScale   = _mm_set1_ps(outScaleVal);
    slope      = _mm_loadu_ps(renderParams.getSlope());
    offset     = _mm_loadu_ps(renderParams.getOffset());
    power      = _mm_loadu_ps(renderParams.getPower());
    saturation = _mm_set1_ps(renderParams.getSaturation());
}
#endif

CDLRendererV1_2Fwd::CDLRendererV1_2Fwd(ConstCDLOpDataRcPtr & cdl)
    :   CDLOpCPU(cdl)
{
}

void CDLRendererV1_2Fwd::apply(const void * inImg, void * outImg, long numPixels) const
{
    _apply<true>((const float *)inImg, (float *)outImg, numPixels);
}

template<bool CLAMP>
void CDLRendererV1_2Fwd::_apply(const float * inImg, float * outImg, long numPixels) const
{
#ifdef USE_SSE
    __m128 inScale, outScale, slope, offset, power, saturation, pix;
    LoadRenderParams(m_inScale,
                     m_outScale,
                     m_renderParams,
                     inScale,
                     outScale,
                     slope,
                     offset,
                     power,
                     saturation);

    float inAlpha;

    // Combine scale and slope so that they can be applied at the same time
    __m128 inScaleSlope = _mm_mul_ps(slope, inScale);

    const float * in = inImg;
    float * out = outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        pix = LoadPixel(in, inAlpha);

        // inScale is combined with slope
        ApplySlope(pix, inScaleSlope);
        ApplyOffset(pix, offset);

        ApplyPower<CLAMP>(pix, power);

        ApplySaturation(pix, saturation);
        ApplyClamp<CLAMP>(pix);

        ApplyOutScale(pix, outScale);

        StorePixel(out, pix, inAlpha * m_alphaScale);

        in  += 4;
        out += 4;
    }
#else
    const float * in = inImg;
    float * out = outImg;

    // Combine inScale and slope
    const float * slope = m_renderParams.getSlope();
    float inScaleSlope[3] = {slope[0], slope[1], slope[2]};
    ApplyScale(inScaleSlope, m_inScale);

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float inAlpha = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        memcpy(out, in, 4 * sizeof(float));

        ApplySlope(out, inScaleSlope);
        ApplyOffset(out, m_renderParams.getOffset());

        ApplyPower<CLAMP>(out, m_renderParams.getPower());

        ApplySaturation(out, m_renderParams.getSaturation());
        ApplyClamp<CLAMP>(out);

        ApplyScale(out, m_outScale);

        out[3] = inAlpha * m_alphaScale;

        in  += 4;
        out += 4;
    }
#endif
}

CDLRendererNoClampFwd::CDLRendererNoClampFwd(ConstCDLOpDataRcPtr & cdl)
    :   CDLRendererV1_2Fwd(cdl)
{
}

void CDLRendererNoClampFwd::apply(const void * inImg, void * outImg, long numPixels) const
{
    _apply<false>((const float *)inImg, (float *)outImg, numPixels);
}

CDLRendererV1_2Rev::CDLRendererV1_2Rev(ConstCDLOpDataRcPtr & cdl)
    :   CDLOpCPU(cdl)
{
}

void CDLRendererV1_2Rev::apply(const void * inImg, void * outImg, long numPixels) const
{
    _apply<true>((const float *)inImg, (float *)outImg, numPixels);
}

template<bool CLAMP>
void CDLRendererV1_2Rev::_apply(const float * inImg, float * outImg, long numPixels) const
{
#ifdef USE_SSE
    __m128 inScale, outScale, slopeRev, offsetRev, powerRev, saturationRev, pix;
    LoadRenderParams(m_inScale,
                     m_outScale,
                     m_renderParams,
                     inScale,
                     outScale, 
                     slopeRev,
                     offsetRev,
                     powerRev,
                     saturationRev);

    float inAlpha = 1.0f;

    const float * in = inImg;
    float * out = outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        pix = LoadPixel(in, inAlpha);

        ApplyInScale(pix, inScale);

        ApplyClamp<CLAMP>(pix);
        ApplySaturation(pix, saturationRev);

        ApplyPower<CLAMP>(pix, powerRev);

        ApplyOffset(pix, offsetRev);
        ApplySlope(pix, slopeRev);
        ApplyClamp<CLAMP>(pix);

        ApplyOutScale(pix, outScale);
        StorePixel(out, pix, inAlpha * m_alphaScale);

        in  += 4;
        out += 4;
    }
#else
    const float * in = inImg;
    float * out = outImg;

    for (long idx = 0; idx<numPixels; ++idx)
    {
        const float inAlpha = in[3];

        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        memcpy(out, in, 4 * sizeof(float));

        ApplyScale(out, m_inScale);

        ApplyClamp<CLAMP>(out);
        ApplySaturation(out, m_renderParams.getSaturation());

        ApplyPower<CLAMP>(out, m_renderParams.getPower());

        ApplyOffset(out, m_renderParams.getOffset());
        ApplySlope(out, m_renderParams.getSlope());
        ApplyClamp<CLAMP>(out);

        ApplyScale(out, m_outScale);

        out[3] = inAlpha * m_alphaScale;

        in  += 4;
        out += 4;
    }

#endif
}

CDLRendererNoClampRev::CDLRendererNoClampRev(ConstCDLOpDataRcPtr & cdl)
    :   CDLRendererV1_2Rev(cdl)
{
}

void CDLRendererNoClampRev::apply(const void * inImg, void * outImg, long numPixels) const
{
    _apply<false>((const float *)inImg, (float *)outImg, numPixels);
}

// TODO:  Add a faster renderer for the case where power and saturation are 1.
OpCPURcPtr CDLOpCPU::GetRenderer(ConstCDLOpDataRcPtr & cdl)
{
    switch(cdl->getStyle())
    {
        case CDLOpData::CDL_V1_2_FWD:
            return std::make_shared<CDLRendererV1_2Fwd>(cdl);
        case CDLOpData::CDL_NO_CLAMP_FWD:
            return std::make_shared<CDLRendererNoClampFwd>(cdl);
        case CDLOpData::CDL_V1_2_REV:
            return std::make_shared<CDLRendererV1_2Rev>(cdl);
        case CDLOpData::CDL_NO_CLAMP_REV:
            return std::make_shared<CDLRendererNoClampRev>(cdl);
    }

    throw Exception("Unknown CDL style");
    return OpCPURcPtr();
}

}
OCIO_NAMESPACE_EXIT
