// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/gradingprimary/GradingPrimaryOpCPU.h"
#include "SSE.h"

namespace OCIO_NAMESPACE
{

namespace
{
class GradingPrimaryOpCPU : public OpCPU
{
public:
    GradingPrimaryOpCPU() = delete;
    GradingPrimaryOpCPU(const GradingPrimaryOpCPU &) = delete;

    explicit GradingPrimaryOpCPU(ConstGradingPrimaryOpDataRcPtr & gp);

    bool hasDynamicProperty(DynamicPropertyType type) const override;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const override;

protected:
    DynamicPropertyGradingPrimaryImplRcPtr m_gp;
};

GradingPrimaryOpCPU::GradingPrimaryOpCPU(ConstGradingPrimaryOpDataRcPtr & gp)
    : OpCPU()
{
    m_gp = gp->getDynamicPropertyInternal();
    if (m_gp->isDynamic())
    {
        m_gp = m_gp->createEditableCopy();
    }
}

bool GradingPrimaryOpCPU::hasDynamicProperty(DynamicPropertyType type) const
{
    bool res = false;
    if (type == DYNAMIC_PROPERTY_GRADING_PRIMARY)
    {
        res = m_gp->isDynamic();
    }
    return res;
}

DynamicPropertyRcPtr GradingPrimaryOpCPU::getDynamicProperty(DynamicPropertyType type) const
{
    if (type == DYNAMIC_PROPERTY_GRADING_PRIMARY)
    {
        if (m_gp->isDynamic())
        {
            return m_gp;
        }
    }
    else
    {
        throw Exception("Dynamic property type not supported by GradingPrimary.");
    }

    throw Exception("GradingPrimary property is not dynamic.");
}

class GradingPrimaryLogFwdOpCPU : public GradingPrimaryOpCPU
{
public:
    explicit GradingPrimaryLogFwdOpCPU(ConstGradingPrimaryOpDataRcPtr & gp);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class GradingPrimaryLogRevOpCPU : public GradingPrimaryLogFwdOpCPU
{
public:
    explicit GradingPrimaryLogRevOpCPU(ConstGradingPrimaryOpDataRcPtr & gp);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class GradingPrimaryLinFwdOpCPU : public GradingPrimaryOpCPU
{
public:
    explicit GradingPrimaryLinFwdOpCPU(ConstGradingPrimaryOpDataRcPtr & gp);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class GradingPrimaryLinRevOpCPU : public GradingPrimaryLinFwdOpCPU
{
public:
    explicit GradingPrimaryLinRevOpCPU(ConstGradingPrimaryOpDataRcPtr & gp);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class GradingPrimaryVidFwdOpCPU : public GradingPrimaryOpCPU
{
public:
    explicit GradingPrimaryVidFwdOpCPU(ConstGradingPrimaryOpDataRcPtr & gp);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class GradingPrimaryVidRevOpCPU : public GradingPrimaryVidFwdOpCPU
{
public:
    explicit GradingPrimaryVidRevOpCPU(ConstGradingPrimaryOpDataRcPtr & gp);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

///////////////////////////////////////////////////////////////////////////////

#ifdef USE_SSE
inline void ApplyContrast(__m128 & pix, const __m128 contrast, const __m128 pivot)
{
    pix = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(pix, pivot), contrast), pivot);
}

inline void ApplyLinContrast(__m128 & pix, const __m128 contrast, const __m128 pivot)
{
    pix = _mm_div_ps(pix, pivot);
    __m128 sign_pix = _mm_and_ps(pix, ESIGN_MASK);
    __m128 abs_pix  = _mm_and_ps(pix, EABS_MASK);
    pix = _mm_mul_ps(ssePower(abs_pix, contrast), pivot);
    pix = _mm_xor_ps(pix, sign_pix);
}

inline void ApplyGamma(__m128 & pix, const __m128 gamma, __m128 blackPivot, __m128 whitePivot)
{
    pix = _mm_sub_ps(pix, blackPivot);
    __m128 sign_pix = _mm_and_ps(pix, ESIGN_MASK);
    __m128 abs_pix = _mm_and_ps(pix, EABS_MASK);
    __m128 range = _mm_sub_ps(whitePivot, blackPivot);
    pix = _mm_div_ps(abs_pix, range);
    pix = ssePower(pix, gamma);
    pix = _mm_add_ps(_mm_mul_ps(_mm_xor_ps(pix, sign_pix), range), blackPivot);
}

static const __m128 LumaWeights = _mm_setr_ps(0.2126f, 0.7152f, 0.0722f, 0.0);

inline void ApplySaturation(__m128 & pix, const __m128 saturation)
{
    // Compute luma: dot product of pixel values and the luma weights.
    __m128 luma = _mm_mul_ps(pix, LumaWeights);

    // luma = [ x+y , y+x , z+w , w+z ]
    luma = _mm_add_ps(luma, _mm_shuffle_ps(luma, luma, _MM_SHUFFLE(2, 3, 0, 1)));

    // luma = [ x+y+z+w , y+x+w+z , z+w+x+y , w+z+y+x ]
    luma = _mm_add_ps(luma, _mm_shuffle_ps(luma, luma, _MM_SHUFFLE(1, 0, 3, 2)));

    // Apply saturation
    pix = _mm_add_ps(luma, _mm_mul_ps(saturation, _mm_sub_ps(pix, luma)));
}

inline void ApplyClamp(__m128 & pix, const __m128 blackClamp, const __m128 whiteClamp)
{
    pix = _mm_min_ps(_mm_max_ps(pix, blackClamp), whiteClamp);
}

#else

inline void ApplyScale(float * pix, const float scale)
{
    pix[0] = pix[0] * scale;
    pix[1] = pix[1] * scale;
    pix[2] = pix[2] * scale;
}

inline void ApplyContrast(float * pix, const float * contrast, const float pivot)
{
    pix[0] = (pix[0] - pivot) * contrast[0] + pivot;
    pix[1] = (pix[1] - pivot) * contrast[1] + pivot;
    pix[2] = (pix[2] - pivot) * contrast[2] + pivot;
}

inline void ApplyLinContrast(float * pix, const float * contrast, const float pivot)
{
    pix[0] = std::pow(std::abs(pix[0] / pivot), contrast[0]) * std::copysign(pivot, pix[0]);
    pix[1] = std::pow(std::abs(pix[1] / pivot), contrast[1]) * std::copysign(pivot, pix[1]);
    pix[2] = std::pow(std::abs(pix[2] / pivot), contrast[2]) * std::copysign(pivot, pix[2]);
}

// Apply the slope component to the the pixel's values
inline void ApplySlope(float * pix, const float * slope)
{
    pix[0] = pix[0] * slope[0];
    pix[1] = pix[1] * slope[1];
    pix[2] = pix[2] * slope[2];
}

// Apply the offset component to the the pixel's values.
inline void ApplyOffset(float * pix, const float * m_offset)
{
    pix[0] = pix[0] + m_offset[0];
    pix[1] = pix[1] + m_offset[1];
    pix[2] = pix[2] + m_offset[2];
}

inline void ApplyGamma(float * pix, const float * gamma, float blackPivot, float whitePivot)
{
    pix[0] = std::pow(std::abs(pix[0] - blackPivot) / (whitePivot - blackPivot), gamma[0]) *
             std::copysign(1.f, pix[0] - blackPivot) * (whitePivot - blackPivot) + blackPivot;
    pix[1] = std::pow(std::abs(pix[1] - blackPivot) / (whitePivot - blackPivot), gamma[1]) *
             std::copysign(1.f, pix[1] - blackPivot) * (whitePivot - blackPivot) + blackPivot;
    pix[2] = std::pow(std::abs(pix[2] - blackPivot) / (whitePivot - blackPivot), gamma[2]) *
             std::copysign(1.f, pix[2] - blackPivot) * (whitePivot - blackPivot) + blackPivot;
}

// Apply the saturation component to the the pixel's values.
inline void ApplySaturation(float * pix, const float m_saturation)
{
    if (m_saturation != 1.f)
    {
        const float srcpix[3] = { pix[0], pix[1], pix[2] };

        static const float LumaWeights[3] = { 0.2126f, 0.7152f, 0.0722f };

        // Compute luma: dot product of pixel values and the luma weights.
        ApplySlope(pix, LumaWeights);
        const float luma = pix[0] + pix[1] + pix[2];

        // Apply saturation
        pix[0] = luma + m_saturation * (srcpix[0] - luma);
        pix[1] = luma + m_saturation * (srcpix[1] - luma);
        pix[2] = luma + m_saturation * (srcpix[2] - luma);
    }
}

inline void ApplyClamp(float * pix, float clampMin, float clampMax)
{
    pix[0] = std::min(std::max(pix[0], clampMin), clampMax);
    pix[1] = std::min(std::max(pix[1], clampMin), clampMax);
    pix[2] = std::min(std::max(pix[2], clampMin), clampMax);
    // if we want NaNs to become 0, use:
    //    pix[0] = Clamp(pix[0], clampMin, clampMax);
    // Default values that should not clamp will change clamp.
}
#endif // USE_SSE

///////////////////////////////////////////////////////////////////////////////

static constexpr auto PixelSize = 4 * sizeof(float);

GradingPrimaryLogFwdOpCPU::GradingPrimaryLogFwdOpCPU(ConstGradingPrimaryOpDataRcPtr & gp)
    : GradingPrimaryOpCPU(gp)
{
}

void GradingPrimaryLogFwdOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_gp->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();
    auto & comp = m_gp->getComputedValue();

    const bool isGammaIdentity = comp.isGammaIdentity();

#ifdef USE_SSE
    const __m128 brightness = _mm_set_ps(0.f, comp.getBrightness()[2],
                                              comp.getBrightness()[1],
                                              comp.getBrightness()[0]);
    const __m128 contrast = _mm_set_ps(1.f, comp.getContrast()[2],
                                            comp.getContrast()[1],
                                            comp.getContrast()[0]);
    const __m128 gamma = _mm_set_ps(1.f, comp.getGamma()[2],
                                         comp.getGamma()[1],
                                         comp.getGamma()[0]);
    const __m128 pivot = _mm_set1_ps(static_cast<float>(comp.getPivot()));
    const __m128 saturation = _mm_set1_ps(static_cast<float>(v.m_saturation));
    const __m128 blackPivot = _mm_set1_ps(static_cast<float>(v.m_pivotBlack));
    const __m128 whitePivot = _mm_set1_ps(static_cast<float>(v.m_pivotWhite));
    const __m128 blackClamp = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 whiteClamp = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    if (v.m_saturation != 1.0)
    {
        if (!isGammaIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, brightness);
                ApplyContrast(pixel, contrast, pivot);
                ApplyGamma(pixel, gamma, blackPivot, whitePivot);
                ApplySaturation(pixel, saturation);
                ApplyClamp(pixel, blackClamp, whiteClamp);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, brightness);
                ApplyContrast(pixel, contrast, pivot);
                ApplySaturation(pixel, saturation);
                ApplyClamp(pixel, blackClamp, whiteClamp);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
    else
    {
        if (!isGammaIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, brightness);
                ApplyContrast(pixel, contrast, pivot);
                ApplyGamma(pixel, gamma, blackPivot, whitePivot);
                ApplyClamp(pixel, blackClamp, whiteClamp);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, brightness);
                ApplyContrast(pixel, contrast, pivot);
                ApplyClamp(pixel, blackClamp, whiteClamp);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
#else
    const float * brightness = comp.getBrightness().data();
    const float * contrast = comp.getContrast().data();
    const float * gamma = comp.getGamma().data();

    const float actualPivot = static_cast<float>(comp.getPivot());
    const float saturation = static_cast<float>(v.m_saturation);
    const float pivotBlack = static_cast<float>(v.m_pivotBlack);
    const float pivotWhite = static_cast<float>(v.m_pivotWhite);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);
    if (!isGammaIdentity)
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            //
            // out = in + brightness;
            // out = ( out - actualPivot ) * contrast + actualPivot
            // normalizedOut = abs( out - blackPivot ) / ( whitePivot - blackPivot )
            // scale = sign( out - blackPivot ) * ( whitePivot - blackPivot )
            // out = pow( normalizedOut, gamma ) * scale + blackPivot
            // luma = out * lumaW
            // out = luma + saturation * ( out - luma )
            // out = clamp( out, clampBlack, clampWhite )

            // NB: 'in' and 'out' could be pointers to the same memory buffer.
            memcpy(out, in, PixelSize);

            ApplyOffset(out, brightness);
            ApplyContrast(out, contrast, actualPivot);
            ApplyGamma(out, gamma, pivotBlack, pivotWhite);
            ApplySaturation(out, saturation);
            ApplyClamp(out, clampBlack, clampWhite);

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, PixelSize);

            ApplyOffset(out, brightness);
            ApplyContrast(out, contrast, actualPivot);
            ApplySaturation(out, saturation);
            ApplyClamp(out, clampBlack, clampWhite);

            in += 4;
            out += 4;
        }
    }
#endif  // USE_SSE
}

GradingPrimaryLogRevOpCPU::GradingPrimaryLogRevOpCPU(ConstGradingPrimaryOpDataRcPtr & gp)
    : GradingPrimaryLogFwdOpCPU(gp)
{
}

void GradingPrimaryLogRevOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_gp->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();
    auto & comp = m_gp->getComputedValue();

    const bool isGammaIdentity = comp.isGammaIdentity();

#ifdef USE_SSE
    const __m128 brightnessInv = _mm_set_ps(0.f, comp.getBrightness()[2],
                                                 comp.getBrightness()[1],
                                                 comp.getBrightness()[0]);
    const __m128 contrastInv = _mm_set_ps(1.f, comp.getContrast()[2],
                                               comp.getContrast()[1],
                                               comp.getContrast()[0]);
    const __m128 gammaInv = _mm_set_ps(1.f, comp.getGamma()[2],
                                            comp.getGamma()[1],
                                            comp.getGamma()[0]);

    const __m128 pivotBlack = _mm_set1_ps(static_cast<float>(v.m_pivotBlack));
    const __m128 pivotWhite = _mm_set1_ps(static_cast<float>(v.m_pivotWhite));
    const __m128 clampB = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 clampW = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    const __m128 actualPivot = _mm_set1_ps(static_cast<float>(comp.getPivot()));

    if (v.m_saturation != 1. && v.m_saturation != 0.)
    {
        const __m128 satInv = _mm_set1_ps(static_cast<float>(1. / v.m_saturation));
        if (!isGammaIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplySaturation(pixel, satInv);
                ApplyGamma(pixel, gammaInv, pivotBlack, pivotWhite);
                ApplyContrast(pixel, contrastInv, actualPivot);
                pixel = _mm_add_ps(pixel, brightnessInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplySaturation(pixel, satInv);
                ApplyContrast(pixel, contrastInv, actualPivot);
                pixel = _mm_add_ps(pixel, brightnessInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
    else
    {
        if (!isGammaIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplyGamma(pixel, gammaInv, pivotBlack, pivotWhite);
                ApplyContrast(pixel, contrastInv, actualPivot);
                pixel = _mm_add_ps(pixel, brightnessInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplyContrast(pixel, contrastInv, actualPivot);
                pixel = _mm_add_ps(pixel, brightnessInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }

#else
    const float * brightnessInv = comp.getBrightness().data();
    const float * contrastInv = comp.getContrast().data();
    const float * gammaInv = comp.getGamma().data();

    const float pivotBlack = static_cast<float>(v.m_pivotBlack);
    const float pivotWhite = static_cast<float>(v.m_pivotWhite);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);

    const float actualPivot = static_cast<float>(comp.getPivot());
    const float sat = static_cast<float>(v.m_saturation);
    const float satInv = 1.f / (sat != 0.f ? sat : 1.f);

    if (!isGammaIdentity)
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, PixelSize);

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplyGamma(out, gammaInv, pivotBlack, pivotWhite);
            ApplyContrast(out, contrastInv, actualPivot);
            ApplyOffset(out, brightnessInv);

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, PixelSize);

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplyContrast(out, contrastInv, actualPivot);
            ApplyOffset(out, brightnessInv);

            in += 4;
            out += 4;
        }
    }
#endif // USE_SSE
}

GradingPrimaryLinFwdOpCPU::GradingPrimaryLinFwdOpCPU(ConstGradingPrimaryOpDataRcPtr & gp)
    : GradingPrimaryOpCPU(gp)
{
}

void GradingPrimaryLinFwdOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_gp->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();
    auto & comp = m_gp->getComputedValue();

    const bool isContrastIdentity = comp.isContrastIdentity();

#ifdef USE_SSE
    const __m128 offset = _mm_set_ps(0.f, comp.getOffset()[2],
                                          comp.getOffset()[1],
                                          comp.getOffset()[0]);
    const __m128 exposure = _mm_set_ps(1.f, comp.getExposure()[2],
                                            comp.getExposure()[1],
                                            comp.getExposure()[0]);
    const __m128 contrast = _mm_set_ps(1.f, comp.getContrast()[2],
                                            comp.getContrast()[1],
                                            comp.getContrast()[0]);
    const __m128 pivot = _mm_set1_ps(static_cast<float>(comp.getPivot()));
    const __m128 saturation = _mm_set1_ps(static_cast<float>(v.m_saturation));
    const __m128 clampB = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 clampW = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    if (v.m_saturation != 1.0)
    {
        if (!isContrastIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, offset);
                pixel = _mm_mul_ps(pixel, exposure);
                ApplyLinContrast(pixel, contrast, pivot);
                ApplySaturation(pixel, saturation);
                ApplyClamp(pixel, clampB, clampW);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, offset);
                pixel = _mm_mul_ps(pixel, exposure);
                ApplySaturation(pixel, saturation);
                ApplyClamp(pixel, clampB, clampW);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
    else
    {
        if (!isContrastIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, offset);
                pixel = _mm_mul_ps(pixel, exposure);
                ApplyLinContrast(pixel, contrast, pivot);
                ApplyClamp(pixel, clampB, clampW);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, offset);
                pixel = _mm_mul_ps(pixel, exposure);
                ApplyClamp(pixel, clampB, clampW);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
#else
    const float * offset   = comp.getOffset().data();
    const float * exposure = comp.getExposure().data();
    const float * contrast = comp.getContrast().data();

    const float actualPivot = static_cast<float>(comp.getPivot());

    const float saturation = static_cast<float>(v.m_saturation);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);

    if (!isContrastIdentity)
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            //
            // out = ( in + offset ) * pow( 2, exposure )
            // out = pow( abs(out / actualPivot), contrast ) * sign(out) * actualPivot
            // luma = out * lumaW
            // out = luma + saturation * ( out - luma )
            // out = clamp( out, clampBlack, clampWhite )

            // NB: 'in' and 'out' could be pointers to the same memory buffer.
            memcpy(out, in, PixelSize);

            ApplyOffset(out, offset);
            ApplySlope(out, exposure);
            ApplyLinContrast(out, contrast, actualPivot);
            ApplySaturation(out, saturation);
            ApplyClamp(out, clampBlack, clampWhite);

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, PixelSize);

            ApplyOffset(out, offset);
            ApplySlope(out, exposure);
            ApplySaturation(out, saturation);
            ApplyClamp(out, clampBlack, clampWhite);

            in += 4;
            out += 4;
        }
    }
#endif // USE_SSE
}

GradingPrimaryLinRevOpCPU::GradingPrimaryLinRevOpCPU(ConstGradingPrimaryOpDataRcPtr & gp)
    : GradingPrimaryLinFwdOpCPU(gp)
{
}

void GradingPrimaryLinRevOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_gp->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();
    auto & comp = m_gp->getComputedValue();

    const bool isContrastIdentity = comp.isContrastIdentity();

#ifdef USE_SSE
    const __m128 offsetInv = _mm_set_ps(0.f, comp.getOffset()[2],
                                             comp.getOffset()[1],
                                             comp.getOffset()[0]);
    const __m128 exposureInv = _mm_set_ps(1.f, comp.getExposure()[2],
                                               comp.getExposure()[1],
                                               comp.getExposure()[0]);
    const __m128 contrastInv = _mm_set_ps(1.f, comp.getContrast()[2],
                                               comp.getContrast()[1],
                                               comp.getContrast()[0]);
    const __m128 pivot = _mm_set1_ps(static_cast<float>(comp.getPivot()));
    const __m128 clampB = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 clampW = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    if (v.m_saturation != 1. && v.m_saturation != 0.)
    {
        const __m128 satInv = _mm_set1_ps(static_cast<float>(1. / v.m_saturation));
        if (!isContrastIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplySaturation(pixel, satInv);
                ApplyLinContrast(pixel, contrastInv, pivot);
                pixel = _mm_mul_ps(pixel, exposureInv);
                pixel = _mm_add_ps(pixel, offsetInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplySaturation(pixel, satInv);
                pixel = _mm_mul_ps(pixel, exposureInv);
                pixel = _mm_add_ps(pixel, offsetInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
    else
    {
        if (!isContrastIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplyLinContrast(pixel, contrastInv, pivot);
                pixel = _mm_mul_ps(pixel, exposureInv);
                pixel = _mm_add_ps(pixel, offsetInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplyLinContrast(pixel, contrastInv, pivot);
                pixel = _mm_mul_ps(pixel, exposureInv);
                pixel = _mm_add_ps(pixel, offsetInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
#else
    const float * offsetInv   = comp.getOffset().data();
    const float * exposureInv = comp.getExposure().data();
    const float * contrastInv = comp.getContrast().data();

    const float actualPivot = static_cast<float>(comp.getPivot());
    const float sat = static_cast<float>(v.m_saturation);
    const float satInv = 1.f / (sat != 0.f ? sat : 1.f);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);

    if (!isContrastIdentity)
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            // NB: 'in' and 'out' could be pointers to the same memory buffer.
            memcpy(out, in, PixelSize);

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplyLinContrast(out, contrastInv, actualPivot);
            ApplySlope(out, exposureInv);
            ApplyOffset(out, offsetInv);

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, PixelSize);

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplySlope(out, exposureInv);
            ApplyOffset(out, offsetInv);

            in += 4;
            out += 4;
        }
    }
#endif // USE_SSE
}

GradingPrimaryVidFwdOpCPU::GradingPrimaryVidFwdOpCPU(ConstGradingPrimaryOpDataRcPtr & gp)
    : GradingPrimaryOpCPU(gp)
{
}

void GradingPrimaryVidFwdOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_gp->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();
    auto & comp = m_gp->getComputedValue();

    const bool isGammaIdentity = comp.isGammaIdentity();

#ifdef USE_SSE
    const __m128 offset = _mm_set_ps(0.f, comp.getOffset()[2],
                                          comp.getOffset()[1],
                                          comp.getOffset()[0]);
    const __m128 slope = _mm_set_ps(1.f, comp.getSlope()[2],
                                         comp.getSlope()[1],
                                         comp.getSlope()[0]);
    const __m128 gamma = _mm_set_ps(1.f, comp.getGamma()[2],
                                         comp.getGamma()[1],
                                         comp.getGamma()[0]);

    const __m128 saturation = _mm_set1_ps(static_cast<float>(v.m_saturation));
    const __m128 pivotBlack = _mm_set1_ps(static_cast<float>(v.m_pivotBlack));
    const __m128 pivotWhite = _mm_set1_ps(static_cast<float>(v.m_pivotWhite));
    const __m128 clampB = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 clampW = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    if (v.m_saturation != 1.0)
    {
        if (!isGammaIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, offset);
                ApplyContrast(pixel, slope, pivotBlack);
                ApplyGamma(pixel, gamma, pivotBlack, pivotWhite);
                ApplySaturation(pixel, saturation);
                ApplyClamp(pixel, clampB, clampW);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, offset);
                ApplyContrast(pixel, slope, pivotBlack);
                ApplySaturation(pixel, saturation);
                ApplyClamp(pixel, clampB, clampW);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
    else
    {
        if (!isGammaIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, offset);
                ApplyContrast(pixel, slope, pivotBlack);
                ApplyGamma(pixel, gamma, pivotBlack, pivotWhite);
                ApplyClamp(pixel, clampB, clampW);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                pixel = _mm_add_ps(pixel, offset);
                ApplyContrast(pixel, slope, pivotBlack);
                ApplyClamp(pixel, clampB, clampW);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
#else
    const float * gamma  = comp.getGamma().data();
    const float * offset = comp.getOffset().data();
    const float * slope  = comp.getSlope().data();

    const float saturation = static_cast<float>(v.m_saturation);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);
    const float pivotBlack = static_cast<float>(v.m_pivotBlack);
    const float pivotWhite = static_cast<float>(v.m_pivotWhite);

    if (!isGammaIdentity)
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            //
            // out = in + (lift + offset)
            // out = ( out - blackPivot ) * slope + blackPivot
            // normalizedOut = abs( out - blackPivot ) / ( whitePivot - blackPivot )
            // scale = sign( out - blackPivot ) * ( whitePivot - blackPivot )
            // out = pow( normalizedOut, gamma ) * scale + blackPivot
            // luma = out * lumaW
            // out = luma + saturation * ( out - luma )
            // out = clamp( out, clampBlack, clampWhite )

            // NB: 'in' and 'out' could be pointers to the same memory buffer.
            memcpy(out, in, PixelSize);

            ApplyOffset(out, offset);
            ApplyContrast(out, slope, pivotBlack);
            ApplyGamma(out, gamma, pivotBlack, pivotWhite);
            ApplySaturation(out, saturation);
            ApplyClamp(out, clampBlack, clampWhite);

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, PixelSize);

            ApplyOffset(out, offset);
            ApplyContrast(out, slope, pivotBlack);
            ApplySaturation(out, saturation);
            ApplyClamp(out, clampBlack, clampWhite);

            in += 4;
            out += 4;
        }
    }

#endif // USE_SSE
}

GradingPrimaryVidRevOpCPU::GradingPrimaryVidRevOpCPU(ConstGradingPrimaryOpDataRcPtr & gp)
    : GradingPrimaryVidFwdOpCPU(gp)
{
}

void GradingPrimaryVidRevOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_gp->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();
    auto & comp = m_gp->getComputedValue();

    const bool isGammaIdentity = comp.isGammaIdentity();

#ifdef USE_SSE
    const __m128 offsetInv = _mm_set_ps(0.f, comp.getOffset()[2],
                                             comp.getOffset()[1],
                                             comp.getOffset()[0]);
    const __m128 slopeInv = _mm_set_ps(1.f, comp.getSlope()[2],
                                            comp.getSlope()[1],
                                            comp.getSlope()[0]);
    const __m128 gammaInv = _mm_set_ps(1.f, comp.getGamma()[2],
                                            comp.getGamma()[1],
                                            comp.getGamma()[0]);

    const __m128 pivotBlack = _mm_set1_ps(static_cast<float>(v.m_pivotBlack));
    const __m128 pivotWhite = _mm_set1_ps(static_cast<float>(v.m_pivotWhite));
    const __m128 clampB = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 clampW = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    if (v.m_saturation != 1. && v.m_saturation != 0.)
    {
        const __m128 satInv = _mm_set1_ps(static_cast<float>(1. / v.m_saturation));
        if (!isGammaIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplySaturation(pixel, satInv);
                ApplyGamma(pixel, gammaInv, pivotBlack, pivotWhite);
                ApplyContrast(pixel, slopeInv, pivotBlack);
                pixel = _mm_add_ps(pixel, offsetInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplySaturation(pixel, satInv);
                ApplyContrast(pixel, slopeInv, pivotBlack);
                pixel = _mm_add_ps(pixel, offsetInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
    else
    {
        if (!isGammaIdentity)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplyGamma(pixel, gammaInv, pivotBlack, pivotWhite);
                ApplyContrast(pixel, slopeInv, pivotBlack);
                pixel = _mm_add_ps(pixel, offsetInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
        else
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplyContrast(pixel, slopeInv, pivotBlack);
                pixel = _mm_add_ps(pixel, offsetInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
#else
    const float * gammaInv  = comp.getGamma().data();
    const float * offsetInv = comp.getOffset().data();
    const float * slopeInv  = comp.getSlope().data();

    const float pivotBlack = static_cast<float>(v.m_pivotBlack);
    const float pivotWhite = static_cast<float>(v.m_pivotWhite);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);
    const float sat = static_cast<float>(v.m_saturation);
    const float satInv = 1.f / (sat != 0.f ? sat : 1.f);

    if (!isGammaIdentity)
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            // NB: 'in' and 'out' could be pointers to the same memory buffer.
            memcpy(out, in, PixelSize);

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplyGamma(out, gammaInv, pivotBlack, pivotWhite);
            ApplyContrast(out, slopeInv, pivotBlack);
            ApplyOffset(out, offsetInv);

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, PixelSize);

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplyContrast(out, slopeInv, pivotBlack);
            ApplyOffset(out, offsetInv);

            in += 4;
            out += 4;
        }
    }
#endif  // USE_SSE
}
} // Anonymous namespace

///////////////////////////////////////////////////////////////////////////////

ConstOpCPURcPtr GetGradingPrimaryCPURenderer(ConstGradingPrimaryOpDataRcPtr & prim)
{
    if (prim->getDirection() == TRANSFORM_DIR_FORWARD)
    {
        switch (prim->getStyle())
        {
        case GRADING_LOG:
            return std::make_shared<GradingPrimaryLogFwdOpCPU>(prim);
            break;
        case GRADING_LIN:
            return std::make_shared<GradingPrimaryLinFwdOpCPU>(prim);
            break;
        case GRADING_VIDEO:
            return std::make_shared<GradingPrimaryVidFwdOpCPU>(prim);
            break;
        }
    }
    else if (prim->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        switch (prim->getStyle())
        {
        case GRADING_LOG:
            return std::make_shared<GradingPrimaryLogRevOpCPU>(prim);
            break;
        case GRADING_LIN:
            return std::make_shared<GradingPrimaryLinRevOpCPU>(prim);
            break;
        case GRADING_VIDEO:
            return std::make_shared<GradingPrimaryVidRevOpCPU>(prim);
            break;
        }
    }

    throw Exception("Illegal GradingPrimary direction.");
}

} // namespace OCIO_NAMESPACE
