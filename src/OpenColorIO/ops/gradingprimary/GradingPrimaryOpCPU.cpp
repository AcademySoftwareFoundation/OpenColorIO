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

class GradingPrimaryOpCPU : public OpCPU
{
public:
    GradingPrimaryOpCPU() = delete;
    GradingPrimaryOpCPU(const GradingPrimaryOpCPU &) = delete;

    explicit GradingPrimaryOpCPU(ConstGradingPrimaryOpDataRcPtr & gp);

    bool hasDynamicProperty(DynamicPropertyType type) const override;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const override;
    void unifyDynamicProperty(DynamicPropertyType type,
                              DynamicPropertyGradingPrimaryImplRcPtr & prop) const override;

protected:
    mutable DynamicPropertyGradingPrimaryImplRcPtr m_gp;
};

GradingPrimaryOpCPU::GradingPrimaryOpCPU(ConstGradingPrimaryOpDataRcPtr & gp)
    : OpCPU()
{
    m_gp = gp->getDynamicPropertyInternal();
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

void GradingPrimaryOpCPU::unifyDynamicProperty(DynamicPropertyType type,
                                               DynamicPropertyGradingPrimaryImplRcPtr & prop) const
{
    if (type == DYNAMIC_PROPERTY_GRADING_PRIMARY)
    {
        if (!prop)
        {
            // First occurence, decouple.
            prop = m_gp->createEditableCopy();
        }
        m_gp = prop;
    }
    else
    {
        OpCPU::unifyDynamicProperty(type, prop);
    }
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

namespace
{
#ifdef USE_SSE
inline void ApplyOffset(__m128 & pix, const __m128 offset)
{
    pix = _mm_add_ps(pix, offset);
}

inline void ApplySlope(__m128 & pix, const __m128 slope)
{
    pix = _mm_mul_ps(pix, slope);
}

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

inline void ApplyGamma(__m128 & pix, const __m128 g, __m128 bp, __m128 wp)
{
    pix = _mm_sub_ps(pix, bp);
    __m128 sign_pix = _mm_and_ps(pix, ESIGN_MASK);
    __m128 abs_pix = _mm_and_ps(pix, EABS_MASK);
    __m128 range = _mm_sub_ps(wp, bp);
    pix = _mm_div_ps(abs_pix, range);
    pix = ssePower(pix, g);
    pix = _mm_add_ps(_mm_mul_ps(_mm_xor_ps(pix, sign_pix), range), bp);
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

inline void ApplyClamp(__m128 & pix, const __m128 bc, const __m128 wc)
{
    pix = _mm_min_ps(_mm_max_ps(pix, bc), wc);
}

#else

inline void ApplyScale(float * pix, const float scale)
{
    pix[0] = pix[0] * scale;
    pix[1] = pix[1] * scale;
    pix[2] = pix[2] * scale;
}

inline void ApplyContrast(float * pix, const float * m_contrast, const float m_pivot)
{
    pix[0] = (pix[0] - m_pivot) * m_contrast[0] + m_pivot;
    pix[1] = (pix[1] - m_pivot) * m_contrast[1] + m_pivot;
    pix[2] = (pix[2] - m_pivot) * m_contrast[2] + m_pivot;
}

inline void ApplyLinContrast(float * pix, const float * m_contrast, const float m_pivot)
{
    pix[0] = std::pow(std::abs(pix[0] / m_pivot), m_contrast[0]) * std::copysign(m_pivot, pix[0]);
    pix[1] = std::pow(std::abs(pix[1] / m_pivot), m_contrast[1]) * std::copysign(m_pivot, pix[1]);
    pix[2] = std::pow(std::abs(pix[2] / m_pivot), m_contrast[2]) * std::copysign(m_pivot, pix[2]);
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

inline void ApplyGamma(float * pix, const float * g, float bp, float wp)
{
    pix[0] = std::pow(std::abs(pix[0] - bp) / (wp - bp), g[0]) *
             std::copysign(1.f, pix[0] - bp) * (wp - bp) + bp;
    pix[1] = std::pow(std::abs(pix[1] - bp) / (wp - bp), g[1]) *
             std::copysign(1.f, pix[1] - bp) * (wp - bp) + bp;
    pix[2] = std::pow(std::abs(pix[2] - bp) / (wp - bp), g[2]) *
             std::copysign(1.f, pix[2] - bp) * (wp - bp) + bp;
}

// Apply the saturation component to the the pixel's values.
inline void ApplySaturation(float * pix, const float m_saturation)
{
    if (m_saturation != 1.0f)
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
}

///////////////////////////////////////////////////////////////////////////////

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
            memcpy(outImg, inImg, 4 * numPixels * sizeof(float));
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();

    const bool applyGamma = v.m_gamma.m_master * v.m_gamma.m_red != 1. ||
                            v.m_gamma.m_master * v.m_gamma.m_green != 1. ||
                            v.m_gamma.m_master * v.m_gamma.m_blue != 1.;

#ifdef USE_SSE
    const __m128 b = _mm_set_ps(0.f,
        static_cast<float>((v.m_brightness.m_master + v.m_brightness.m_blue) * 6.25 / 1023.),
        static_cast<float>((v.m_brightness.m_master + v.m_brightness.m_green) * 6.25 / 1023.),
        static_cast<float>((v.m_brightness.m_master + v.m_brightness.m_red) * 6.25 / 1023.));
    const __m128 c = _mm_set_ps(1.0f,
        static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_blue),
        static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_green),
        static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_red));
    const __m128 g = _mm_set_ps(1.0f,
        static_cast<float>(1. / (v.m_gamma.m_blue * v.m_gamma.m_master)),
        static_cast<float>(1. / (v.m_gamma.m_green * v.m_gamma.m_master)),
        static_cast<float>(1. / (v.m_gamma.m_red * v.m_gamma.m_master)));
    const __m128 pivot = _mm_set1_ps(0.5f + static_cast<float>(v.m_pivot) * 0.5f);
    const __m128 saturation = _mm_set1_ps(static_cast<float>(v.m_saturation));
    const __m128 bp = _mm_set1_ps(static_cast<float>(v.m_pivotBlack));
    const __m128 wp = _mm_set1_ps(static_cast<float>(v.m_pivotWhite));
    const __m128 bc = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 wc = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    if (v.m_saturation != 1.0)
    {
        if (applyGamma)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyOffset(pixel, b);
                ApplyContrast(pixel, c, pivot);
                ApplyGamma(pixel, g, bp, wp);
                ApplySaturation(pixel, saturation);
                ApplyClamp(pixel, bc, wc);

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

                ApplyOffset(pixel, b);
                ApplyContrast(pixel, c, pivot);
                ApplySaturation(pixel, saturation);
                ApplyClamp(pixel, bc, wc);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
    else
    {
        if (applyGamma)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyOffset(pixel, b);
                ApplyContrast(pixel, c, pivot);
                ApplyGamma(pixel, g, bp, wp);
                ApplyClamp(pixel, bc, wc);

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

                ApplyOffset(pixel, b);
                ApplyContrast(pixel, c, pivot);
                ApplyClamp(pixel, bc, wc);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
#else
    const float b[3]{ static_cast<float>((v.m_brightness.m_master + v.m_brightness.m_red) * 6.25 / 1023.),
                      static_cast<float>((v.m_brightness.m_master + v.m_brightness.m_green) * 6.25 / 1023.),
                      static_cast<float>((v.m_brightness.m_master + v.m_brightness.m_blue) * 6.25 / 1023.) };
    const float c[3]{ static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_red),
                      static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_green),
                      static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_blue) };
    const float g[3]{ static_cast<float>(1. / (v.m_gamma.m_red * v.m_gamma.m_master)),
                      static_cast<float>(1. / (v.m_gamma.m_green * v.m_gamma.m_master)),
                      static_cast<float>(1. / (v.m_gamma.m_blue * v.m_gamma.m_master)) };

    const float actualPivot = 0.5f + static_cast<float>(v.m_pivot) * 0.5f;
    const float saturation = static_cast<float>(v.m_saturation);
    const float pivotBlack = static_cast<float>(v.m_pivotBlack);
    const float pivotWhite = static_cast<float>(v.m_pivotWhite);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);
    if (applyGamma)
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            //
            // out = in + b;
            // out = ( out - actualPivot ) * c + actualPivot
            // normalizedOut = abs( out - blackPivot ) / ( whitePivot - blackPivot )
            // scale = sign( out - blackPivot ) * ( whitePivot - blackPivot )
            // out = pow( normalizedOut, gamma ) * scale + blackPivot
            // luma = out * lumaW
            // out = luma + saturation * ( out - luma )
            // out = clamp( out, clampBlack, clampWhite )

            // NB: 'in' and 'out' could be pointers to the same memory buffer.
            memcpy(out, in, 4 * sizeof(float));

            ApplyOffset(out, b);
            ApplyContrast(out, c, actualPivot);
            ApplyGamma(out, g, pivotBlack, pivotWhite);
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
            memcpy(out, in, 4 * sizeof(float));

            ApplyOffset(out, b);
            ApplyContrast(out, c, actualPivot);
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
            memcpy(outImg, inImg, 4 * numPixels * sizeof(float));
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();
    const bool applyGamma = v.m_gamma.m_master * v.m_gamma.m_red != 1. ||
                            v.m_gamma.m_master * v.m_gamma.m_green != 1. ||
                            v.m_gamma.m_master * v.m_gamma.m_blue != 1.;

#ifdef USE_SSE
    const __m128 bInv = _mm_set_ps(0.f,
                                   static_cast<float>(-(v.m_brightness.m_master + v.m_brightness.m_blue) * 6.25 / 1023.),
                                   static_cast<float>(-(v.m_brightness.m_master + v.m_brightness.m_green) * 6.25 / 1023.),
                                   static_cast<float>(-(v.m_brightness.m_master + v.m_brightness.m_red) * 6.25 / 1023.));
    const __m128 cInv = _mm_set_ps(1.f,
                                   static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_blue)),
                                   static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_green)),
                                   static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_red)));
    const __m128 gInv = _mm_set_ps(1.f,
                                   static_cast<float>(v.m_gamma.m_master * v.m_gamma.m_blue),
                                   static_cast<float>(v.m_gamma.m_master * v.m_gamma.m_green),
                                   static_cast<float>(v.m_gamma.m_master * v.m_gamma.m_red));

    const __m128 satInv = _mm_set1_ps(static_cast<float>(1. / v.m_saturation));
    const __m128 pivotBlack = _mm_set1_ps(static_cast<float>(v.m_pivotBlack));
    const __m128 pivotWhite = _mm_set1_ps(static_cast<float>(v.m_pivotWhite));
    const __m128 clampB = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 clampW = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    const __m128 actualPivot = _mm_set1_ps(0.5f + static_cast<float>(v.m_pivot) * 0.5f);

    if (v.m_saturation != 1.0)
    {
        if (applyGamma)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplySaturation(pixel, satInv);
                ApplyGamma(pixel, gInv, pivotBlack, pivotWhite);
                ApplyContrast(pixel, cInv, actualPivot);
                ApplyOffset(pixel, bInv);

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
                ApplyContrast(pixel, cInv, actualPivot);
                ApplyOffset(pixel, bInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
    else
    {
        if (applyGamma)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplyGamma(pixel, gInv, pivotBlack, pivotWhite);
                ApplyContrast(pixel, cInv, actualPivot);
                ApplyOffset(pixel, bInv);

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
                ApplyContrast(pixel, cInv, actualPivot);
                ApplyOffset(pixel, bInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }

#else
    const float bInv[3]{ static_cast<float>(-(v.m_brightness.m_master + v.m_brightness.m_red) * 6.25 / 1023.),
                         static_cast<float>(-(v.m_brightness.m_master + v.m_brightness.m_green) * 6.25 / 1023.),
                         static_cast<float>(-(v.m_brightness.m_master + v.m_brightness.m_blue) * 6.25 / 1023.) };
    const float cInv[3]{ static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_red)),
                         static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_green)),
                         static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_blue)) };
    const float gInv[3]{ static_cast<float>(v.m_gamma.m_master * v.m_gamma.m_red),
                         static_cast<float>(v.m_gamma.m_master * v.m_gamma.m_green),
                         static_cast<float>(v.m_gamma.m_master * v.m_gamma.m_blue) };

    const float pivotBlack = static_cast<float>(v.m_pivotBlack);
    const float pivotWhite = static_cast<float>(v.m_pivotWhite);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);

    const float actualPivot = 0.5f + static_cast<float>(v.m_pivot) * 0.5f;
    const float satInv      = 1.0f / static_cast<float>(v.m_saturation);

    if (applyGamma)
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, 4 * sizeof(float));

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplyGamma(out, gInv, pivotBlack, pivotWhite);
            ApplyContrast(out, cInv, actualPivot);
            ApplyOffset(out, bInv);

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, 4 * sizeof(float));

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplyContrast(out, cInv, actualPivot);
            ApplyOffset(out, bInv);

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
            memcpy(outImg, inImg, 4 * numPixels * sizeof(float));
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();
    const bool applyContrast = v.m_contrast.m_master * v.m_contrast.m_red != 1. ||
                               v.m_contrast.m_master * v.m_contrast.m_green != 1. ||
                               v.m_contrast.m_master * v.m_contrast.m_blue != 1.;

#ifdef USE_SSE
    const __m128 o = _mm_set_ps(0.f,
        static_cast<float>(v.m_offset.m_master + v.m_offset.m_blue),
        static_cast<float>(v.m_offset.m_master + v.m_offset.m_green),
        static_cast<float>(v.m_offset.m_master + v.m_offset.m_red));
    const __m128 e = _mm_set_ps(1.0f,
        std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_blue)),
        std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_green)),
        std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_red)));
    const __m128 c = _mm_set_ps(1.0f,
        static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_blue),
        static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_green),
        static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_red));
    const __m128 pivot = _mm_set1_ps(0.18f * std::pow(2.f, static_cast<float>(v.m_pivot)));
    const __m128 saturation = _mm_set1_ps(static_cast<float>(v.m_saturation));
    const __m128 clampB = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 clampW = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    if (v.m_saturation != 1.0)
    {
        if (applyContrast)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyOffset(pixel, o);
                ApplySlope(pixel, e);
                ApplyLinContrast(pixel, c, pivot);
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

                ApplyOffset(pixel, o);
                ApplySlope(pixel, e);
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
        if (applyContrast)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyOffset(pixel, o);
                ApplySlope(pixel, e);
                ApplyLinContrast(pixel, c, pivot);
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

                ApplyOffset(pixel, o);
                ApplySlope(pixel, e);
                ApplyClamp(pixel, clampB, clampW);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
#else
    const float o[3]{ static_cast<float>(v.m_offset.m_master + v.m_offset.m_red),
                      static_cast<float>(v.m_offset.m_master + v.m_offset.m_green),
                      static_cast<float>(v.m_offset.m_master + v.m_offset.m_blue) };
    const float e[3]{ std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_red)),
                      std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_green)),
                      std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_blue)) };
    const float c[3]{ static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_red),
                      static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_green),
                      static_cast<float>(v.m_contrast.m_master * v.m_contrast.m_blue) };

    const float actualPivot = 0.18f * std::pow(2.f, static_cast<float>(v.m_pivot));

    const float saturation = static_cast<float>(v.m_saturation);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);

    if (applyContrast)
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
            memcpy(out, in, 4 * sizeof(float));

            ApplyOffset(out, o);
            ApplySlope(out, e);
            ApplyLinContrast(out, c, actualPivot);
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
            memcpy(out, in, 4 * sizeof(float));

            ApplyOffset(out, o);
            ApplySlope(out, e);
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
            memcpy(outImg, inImg, 4 * numPixels * sizeof(float));
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();

    const bool applyContrast = v.m_contrast.m_master * v.m_contrast.m_red != 1. ||
                               v.m_contrast.m_master * v.m_contrast.m_green != 1. ||
                               v.m_contrast.m_master * v.m_contrast.m_blue != 1.;
#ifdef USE_SSE
    const __m128 oInv = _mm_set_ps(0.f,
        static_cast<float>(-v.m_offset.m_master - v.m_offset.m_blue),
        static_cast<float>(-v.m_offset.m_master - v.m_offset.m_green),
        static_cast<float>(-v.m_offset.m_master - v.m_offset.m_red));
    const __m128 eInv = _mm_set_ps(1.0f,
        1.0f / std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_blue)),
        1.0f / std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_green)),
        1.0f / std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_red)));
    const __m128 cInv = _mm_set_ps(1.0f,
        static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_blue)),
        static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_green)),
        static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_red)));
    const __m128 pivot = _mm_set1_ps(0.18f * std::pow(2.f, static_cast<float>(v.m_pivot)));
    const __m128 satInv = _mm_set1_ps(static_cast<float>(1. / v.m_saturation));
    const __m128 clampB = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 clampW = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    if (v.m_saturation != 1.0)
    {
        if (applyContrast)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplySaturation(pixel, satInv);
                ApplyLinContrast(pixel, cInv, pivot);
                ApplySlope(pixel, eInv);
                ApplyOffset(pixel, oInv);

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
                ApplySlope(pixel, eInv);
                ApplyOffset(pixel, oInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
    else
    {
        if (applyContrast)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplyLinContrast(pixel, cInv, pivot);
                ApplySlope(pixel, eInv);
                ApplyOffset(pixel, oInv);

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
                ApplyLinContrast(pixel, cInv, pivot);
                ApplySlope(pixel, eInv);
                ApplyOffset(pixel, oInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
#else
    const float oInv[3]{ static_cast<float>(-v.m_offset.m_master - v.m_offset.m_red),
                         static_cast<float>(-v.m_offset.m_master - v.m_offset.m_green),
                         static_cast<float>(-v.m_offset.m_master - v.m_offset.m_blue) };
    const float eInv[3]{ 1.0f / std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_red)),
                         1.0f / std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_green)),
                         1.0f / std::pow(2.0f, static_cast<float>(v.m_exposure.m_master + v.m_exposure.m_blue)) };
    const float cInv[3]{ static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_red)),
                         static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_green)),
                         static_cast<float>(1. / (v.m_contrast.m_master * v.m_contrast.m_blue)) };

    const float actualPivot = 0.18f * std::pow(2.f, static_cast<float>(v.m_pivot));
    const float satInv = 1.0f / static_cast<float>(v.m_saturation);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);

    if (applyContrast)
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            // NB: 'in' and 'out' could be pointers to the same memory buffer.
            memcpy(out, in, 4 * sizeof(float));

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplyLinContrast(out, cInv, actualPivot);
            ApplySlope(out, eInv);
            ApplyOffset(out, oInv);

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, 4 * sizeof(float));

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplySlope(out, eInv);
            ApplyOffset(out, oInv);

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
            memcpy(outImg, inImg, 4 * numPixels * sizeof(float));
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();
    const bool applyGamma = v.m_gamma.m_master * v.m_gamma.m_red != 1. ||
                            v.m_gamma.m_master * v.m_gamma.m_green != 1. ||
                            v.m_gamma.m_master * v.m_gamma.m_blue != 1.;

#ifdef USE_SSE
    const __m128 o = _mm_set_ps(0.f,
        static_cast<float>(v.m_offset.m_master + v.m_offset.m_blue + v.m_lift.m_master + v.m_lift.m_blue),
        static_cast<float>(v.m_offset.m_master + v.m_offset.m_green + v.m_lift.m_master + v.m_lift.m_green),
        static_cast<float>(v.m_offset.m_master + v.m_offset.m_red + v.m_lift.m_master + v.m_lift.m_red));
    const __m128 slope = _mm_set_ps(
        1.0f,
        static_cast<float>((v.m_pivotWhite - v.m_pivotBlack) /
        (v.m_pivotWhite / v.m_gain.m_master / v.m_gain.m_blue + (v.m_lift.m_master + v.m_lift.m_blue - v.m_pivotBlack))),
        static_cast<float>((v.m_pivotWhite - v.m_pivotBlack) /
        (v.m_pivotWhite / v.m_gain.m_master / v.m_gain.m_green + (v.m_lift.m_master + v.m_lift.m_green - v.m_pivotBlack))),
        static_cast<float>((v.m_pivotWhite - v.m_pivotBlack) /
        (v.m_pivotWhite / v.m_gain.m_master / v.m_gain.m_red + (v.m_lift.m_master + v.m_lift.m_red - v.m_pivotBlack))));

    const __m128 g = _mm_set_ps(1.0f,
        static_cast<float>(1. / (v.m_gamma.m_blue * v.m_gamma.m_master)),
        static_cast<float>(1. / (v.m_gamma.m_green * v.m_gamma.m_master)),
        static_cast<float>(1. / (v.m_gamma.m_red * v.m_gamma.m_master)));

    const __m128 saturation = _mm_set1_ps(static_cast<float>(v.m_saturation));
    const __m128 pivotBlack = _mm_set1_ps(static_cast<float>(v.m_pivotBlack));
    const __m128 pivotWhite = _mm_set1_ps(static_cast<float>(v.m_pivotWhite));
    const __m128 clampB = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 clampW = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    if (v.m_saturation != 1.0)
    {
        if (applyGamma)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyOffset(pixel, o);
                ApplyContrast(pixel, slope, pivotBlack);
                ApplyGamma(pixel, g, pivotBlack, pivotWhite);
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

                ApplyOffset(pixel, o);
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
        if (applyGamma)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyOffset(pixel, o);
                ApplyContrast(pixel, slope, pivotBlack);
                ApplyGamma(pixel, g, pivotBlack, pivotWhite);
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

                ApplyOffset(pixel, o);
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
    const float l[3]{ static_cast<float>(v.m_lift.m_master + v.m_lift.m_red),
                      static_cast<float>(v.m_lift.m_master + v.m_lift.m_green),
                      static_cast<float>(v.m_lift.m_master + v.m_lift.m_blue) };

    const float gain[3]{ static_cast<float>(v.m_gain.m_master * v.m_gain.m_red),
                         static_cast<float>(v.m_gain.m_master * v.m_gain.m_green),
                         static_cast<float>(v.m_gain.m_master * v.m_gain.m_blue) };

    const float g[3]{ static_cast<float>(1. / (v.m_gamma.m_red * v.m_gamma.m_master)),
                      static_cast<float>(1. / (v.m_gamma.m_green * v.m_gamma.m_master)),
                      static_cast<float>(1. / (v.m_gamma.m_blue * v.m_gamma.m_master)) };

    const float o[3]{ static_cast<float>(v.m_offset.m_master + v.m_offset.m_red) + l[0],
                      static_cast<float>(v.m_offset.m_master + v.m_offset.m_green) + l[1],
                      static_cast<float>(v.m_offset.m_master + v.m_offset.m_blue) + l[2] };

    const float saturation = static_cast<float>(v.m_saturation);
    const float pivotBlack = static_cast<float>(v.m_pivotBlack);
    const float pivotWhite = static_cast<float>(v.m_pivotWhite);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);

    const float slope[3]{ (pivotWhite - pivotBlack) /
                          (pivotWhite / gain[0] + (l[0] - pivotBlack)),
                          (pivotWhite - pivotBlack) /
                          (pivotWhite / gain[1] + (l[1] - pivotBlack)),
                          (pivotWhite - pivotBlack) /
                          (pivotWhite / gain[2] + (l[2] - pivotBlack)) };

    if (applyGamma)
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
            memcpy(out, in, 4 * sizeof(float));

            ApplyOffset(out, o);
            ApplyContrast(out, slope, pivotBlack);
            ApplyGamma(out, g, pivotBlack, pivotWhite);
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
            memcpy(out, in, 4 * sizeof(float));

            ApplyOffset(out, o);
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
            memcpy(outImg, inImg, 4 * numPixels * sizeof(float));
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gp->getValue();
    const bool applyGamma = v.m_gamma.m_master * v.m_gamma.m_red != 1. ||
        v.m_gamma.m_master * v.m_gamma.m_green != 1. ||
        v.m_gamma.m_master * v.m_gamma.m_blue != 1.;

#ifdef USE_SSE
    const __m128 oInv = _mm_set_ps(0.f,
                                   static_cast<float>(-v.m_offset.m_master - v.m_offset.m_blue - v.m_lift.m_master - v.m_lift.m_blue),
                                   static_cast<float>(-v.m_offset.m_master - v.m_offset.m_green - v.m_lift.m_master - v.m_lift.m_green),
                                   static_cast<float>(-v.m_offset.m_master - v.m_offset.m_red - v.m_lift.m_master - v.m_lift.m_red));
    const __m128 slopeInv = _mm_set_ps(
        1.0f,
        static_cast<float>((v.m_pivotWhite / v.m_gain.m_master / v.m_gain.m_blue + (v.m_lift.m_master + v.m_lift.m_blue - v.m_pivotBlack)) /
        (v.m_pivotWhite - v.m_pivotBlack)),
        static_cast<float>((v.m_pivotWhite / v.m_gain.m_master / v.m_gain.m_green + (v.m_lift.m_master + v.m_lift.m_green - v.m_pivotBlack)) /
        (v.m_pivotWhite - v.m_pivotBlack)),
        static_cast<float>((v.m_pivotWhite / v.m_gain.m_master / v.m_gain.m_red + (v.m_lift.m_master + v.m_lift.m_red - v.m_pivotBlack)) /
        (v.m_pivotWhite - v.m_pivotBlack)));

    const __m128 gInv = _mm_set_ps(1.0f,
                                   static_cast<float>(v.m_gamma.m_blue * v.m_gamma.m_master),
                                   static_cast<float>(v.m_gamma.m_green * v.m_gamma.m_master),
                                   static_cast<float>(v.m_gamma.m_red * v.m_gamma.m_master));

    const __m128 satInv     = _mm_set1_ps(static_cast<float>(1. / v.m_saturation));
    const __m128 pivotBlack = _mm_set1_ps(static_cast<float>(v.m_pivotBlack));
    const __m128 pivotWhite = _mm_set1_ps(static_cast<float>(v.m_pivotWhite));
    const __m128 clampB = _mm_set1_ps(static_cast<float>(v.m_clampBlack));
    const __m128 clampW = _mm_set1_ps(static_cast<float>(v.m_clampWhite));

    if (v.m_saturation != 1.0)
    {
        if (applyGamma)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplySaturation(pixel, satInv);
                ApplyGamma(pixel, gInv, pivotBlack, pivotWhite);
                ApplyContrast(pixel, slopeInv, pivotBlack);
                ApplyOffset(pixel, oInv);

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
                ApplyOffset(pixel, oInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
    else
    {
        if (applyGamma)
        {
            for (long idx = 0; idx < numPixels; ++idx)
            {
                const float outAlpha = in[3];
                __m128 pixel = _mm_set_ps(in[3], in[2], in[1], in[0]);

                ApplyClamp(pixel, clampB, clampW);
                ApplyGamma(pixel, gInv, pivotBlack, pivotWhite);
                ApplyContrast(pixel, slopeInv, pivotBlack);
                ApplyOffset(pixel, oInv);

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
                ApplyOffset(pixel, oInv);

                _mm_storeu_ps(out, pixel);
                out[3] = outAlpha;

                in += 4;
                out += 4;
            }
        }
    }
#else
    const float l[3]{ static_cast<float>(v.m_lift.m_master + v.m_lift.m_red),
                      static_cast<float>(v.m_lift.m_master + v.m_lift.m_green),
                      static_cast<float>(v.m_lift.m_master + v.m_lift.m_blue) };

    const float gain[3]{ static_cast<float>(v.m_gain.m_master * v.m_gain.m_red),
                         static_cast<float>(v.m_gain.m_master * v.m_gain.m_green),
                         static_cast<float>(v.m_gain.m_master * v.m_gain.m_blue) };

    const float gInv[3]{ static_cast<float>(v.m_gamma.m_master * v.m_gamma.m_red),
                         static_cast<float>(v.m_gamma.m_master * v.m_gamma.m_green),
                         static_cast<float>(v.m_gamma.m_master * v.m_gamma.m_blue) };

    const float oInv[3]{ static_cast<float>(-v.m_offset.m_master - v.m_offset.m_red) - l[0],
                         static_cast<float>(-v.m_offset.m_master - v.m_offset.m_green) - l[1],
                         static_cast<float>(-v.m_offset.m_master - v.m_offset.m_blue) - l[2] };

    const float pivotBlack = static_cast<float>(v.m_pivotBlack);
    const float pivotWhite = static_cast<float>(v.m_pivotWhite);
    const float clampBlack = static_cast<float>(v.m_clampBlack);
    const float clampWhite = static_cast<float>(v.m_clampWhite);

    const float slopeInv[3]{ (pivotWhite / gain[0] + (l[0] - pivotBlack)) /
                             (pivotWhite - pivotBlack),
                             (pivotWhite / gain[1] + (l[1] - pivotBlack)) /
                             (pivotWhite - pivotBlack),
                             (pivotWhite / gain[2] + (l[2] - pivotBlack)) /
                             (pivotWhite - pivotBlack) };

    const float satInv = 1.0f / static_cast<float>(v.m_saturation);

    if (applyGamma)
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            // NB: 'in' and 'out' could be pointers to the same memory buffer.
            memcpy(out, in, 4 * sizeof(float));

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplyGamma(out, gInv, pivotBlack, pivotWhite);
            ApplyContrast(out, slopeInv, pivotBlack);
            ApplyOffset(out, oInv);

            in += 4;
            out += 4;
        }
    }
    else
    {
        for (long idx = 0; idx < numPixels; ++idx)
        {
            memcpy(out, in, 4 * sizeof(float));

            ApplyClamp(out, clampBlack, clampWhite);
            ApplySaturation(out, satInv);
            ApplyContrast(out, slopeInv, pivotBlack);
            ApplyOffset(out, oInv);

            in += 4;
            out += 4;
        }
    }
#endif  // USE_SSE
}

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

    throw Exception("Unsupported GradingPrimary direction.");
}

} // namespace OCIO_NAMESPACE
