// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/fixedfunction/FixedFunctionOpCPU.h"


namespace OCIO_NAMESPACE
{

class Renderer_ACES_RedMod03_Fwd : public OpCPU
{
public:
    Renderer_ACES_RedMod03_Fwd() = delete;
    Renderer_ACES_RedMod03_Fwd(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_ACES_RedMod03_Fwd(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_1minusScale;
    float m_pivot;
    float m_inv_width;

    const float m_noiseLimit = 1e-2f;
};

class Renderer_ACES_RedMod03_Inv : public Renderer_ACES_RedMod03_Fwd
{
public:
    explicit Renderer_ACES_RedMod03_Inv(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_ACES_RedMod10_Fwd : public OpCPU
{
public:
    Renderer_ACES_RedMod10_Fwd() = delete;
    Renderer_ACES_RedMod10_Fwd(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_ACES_RedMod10_Fwd(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_1minusScale;
    float m_pivot;
    float m_inv_width;

    const float m_noiseLimit = 1e-2f;
};

class Renderer_ACES_RedMod10_Inv : public Renderer_ACES_RedMod10_Fwd
{
public:
    explicit Renderer_ACES_RedMod10_Inv(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_ACES_Glow03_Fwd : public OpCPU
{
public:
    Renderer_ACES_Glow03_Fwd(ConstFixedFunctionOpDataRcPtr & data,
                             float glowGain, float glowMid);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_glowGain, m_glowMid;

    const float m_noiseLimit = 1e-2f;
};

class Renderer_ACES_Glow03_Inv : public Renderer_ACES_Glow03_Fwd
{
public:
    Renderer_ACES_Glow03_Inv(ConstFixedFunctionOpDataRcPtr & data,
                            float glowGain, float glowMid);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_ACES_DarkToDim10_Fwd : public OpCPU
{
public:
    Renderer_ACES_DarkToDim10_Fwd() = delete;
    Renderer_ACES_DarkToDim10_Fwd(const Renderer_ACES_RedMod03_Fwd &) = delete;

    Renderer_ACES_DarkToDim10_Fwd(ConstFixedFunctionOpDataRcPtr & data, float gamma);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_gamma;
};

class Renderer_REC2100_Surround : public OpCPU
{
public:
    Renderer_REC2100_Surround() = delete;
    Renderer_REC2100_Surround(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_REC2100_Surround(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_gamma;
};

class Renderer_RGB_TO_HSV : public OpCPU
{
public:
    Renderer_RGB_TO_HSV() = delete;
    Renderer_RGB_TO_HSV(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_RGB_TO_HSV(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_HSV_TO_RGB : public OpCPU
{
public:
    Renderer_HSV_TO_RGB() = delete;
    Renderer_HSV_TO_RGB(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_HSV_TO_RGB(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_XYZ_TO_xyY : public OpCPU
{
public:
    Renderer_XYZ_TO_xyY() = delete;
    Renderer_XYZ_TO_xyY(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_XYZ_TO_xyY(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_xyY_TO_XYZ : public OpCPU
{
public:
    Renderer_xyY_TO_XYZ() = delete;
    Renderer_xyY_TO_XYZ(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_xyY_TO_XYZ(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_XYZ_TO_uvY : public OpCPU
{
public:
    Renderer_XYZ_TO_uvY() = delete;
    Renderer_XYZ_TO_uvY(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_XYZ_TO_uvY(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_uvY_TO_XYZ : public OpCPU
{
public:
    Renderer_uvY_TO_XYZ() = delete;
    Renderer_uvY_TO_XYZ(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_uvY_TO_XYZ(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_XYZ_TO_LUV : public OpCPU
{
public:
    Renderer_XYZ_TO_LUV() = delete;
    Renderer_XYZ_TO_LUV(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_XYZ_TO_LUV(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_LUV_TO_XYZ : public OpCPU
{
public:
    Renderer_LUV_TO_XYZ() = delete;
    Renderer_LUV_TO_XYZ(const Renderer_ACES_RedMod03_Fwd &) = delete;

    explicit Renderer_LUV_TO_XYZ(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};


///////////////////////////////////////////////////////////////////////////////




// Calculate a saturation measure in a safe manner.
__inline float CalcSatWeight(const float red, const float grn, const float blu,
                             const float noiseLimit)
{
    const float minVal = std::min( red, std::min( grn, blu ) );
    const float maxVal = std::max( red, std::max( grn, blu ) );

    // The numerator is clamped to prevent problems from negative values,
    // the denominator is clamped higher to prevent dark noise from being
    // classified as having high saturation.
    const float sat = ( std::max( 1e-10f, maxVal) - std::max( 1e-10f, minVal) )  /
                        std::max( noiseLimit, maxVal);
    return sat;
}

Renderer_ACES_RedMod03_Fwd::Renderer_ACES_RedMod03_Fwd(ConstFixedFunctionOpDataRcPtr & /*data*/)
    :   OpCPU()
{
    // Constants that define a scale and offset to be applied to the red channel.
    m_1minusScale = 1.f - 0.85f;   // (1. - scale) from the original ctl code
    m_pivot = 0.03f; // offset will be applied to unnormalized input values

    //float width = 120;  // width of hue region (in degrees)
    // Actually want to multiply by 4 / width (in radians).
    // Note: _inv_width = 4 / (width * pi/180)
    m_inv_width = 1.9098593171027443f;
}

__inline float CalcHueWeight(const float red, const float grn, const float blu, 
                             const float inv_width)
{
    // Convert RGB to Yab (luma/chroma).
    const float a = 2.f * red - (grn + blu);
    static const float sqrt3 = 1.7320508075688772f;
    const float b = sqrt3 * (grn - blu);

    const float hue = atan2f(b, a);

    // NB: The code in RedMod03 apply() assumes that in the range of the modification
    // window that red will be the largest channel.  The center and width must be
    // chosen to maintain this.

    // Center the hue and re-wrap to +/-pi.
    //float _center = 0.0f;  // center hue angle (in radians, red = 0.)
    // Note: For this version, center = 0, so this is a no-op.
    // Leaving the code here in case center needs to be tweaked.
    //hue -= _center;
    //hue = (hue < -_pi) ? hue + _twopi: hue;
    //hue = (hue >  _pi) ? hue - _twopi: hue;

    // Determine normalized input coords to B-spline.
    const float knot_coord = hue * inv_width + 2.f;
    const int j = (int) knot_coord;    // index

    // These are the coefficients for a quadratic B-spline basis function.
    // (All coefs taken from the ACES ctl code on github.)
    static const float _M[4][4] = {
        { 0.25f,  0.00f,  0.00f,  0.00f},
        {-0.75f,  0.75f,  0.75f,  0.25f},
        { 0.75f, -1.50f,  0.00f,  1.00f},
        {-0.25f,  0.75f, -0.75f,  0.25f} };

    // Hue is in range of the window, calculate weight.
    float f_H = 0.f;
    if ( j >= 0 && j < 4)
    {
        const float t = knot_coord - j;  // fractional component

        // Calculate quadratic B-spline weighting function.
        const float* coefs = _M[j];
        f_H = coefs[3] + t * (coefs[2] + t * (coefs[1] + t * coefs[0]));
    }

    return f_H;
}

void Renderer_ACES_RedMod03_Fwd::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        float red = in[0];
        float grn = in[1];
        float blu = in[2];

        const float f_H = CalcHueWeight(red, grn, blu, m_inv_width);

        // Hue is in range of the window, apply mod.
        if (f_H > 0.f)
        {
            const float f_S = CalcSatWeight(red, grn, blu, m_noiseLimit);

            // Apply red modifier.  NB:  Red is still at inScale.

            //const float modRed = (red - _pivot) * _scale + _pivot;
            //const float tmp = red * (1.f - f_S) + f_S * modRed;
            //const float newRed = red * (1.f - f_H) + f_H * tmp;
            // The above is easier to understand, but reduces down to the following:
            const float newRed = red + f_H * f_S * (m_pivot - red) * m_1minusScale;

            // Restore hue.
            if (grn >= blu) // red >= grn >= blu
            {
                const float hue_fac = (grn - blu) / std::max( 1e-10f, red - blu);
                grn = hue_fac * (newRed - blu) + blu;
            }
            else            // red >= blu >= grn
            {
                const float hue_fac = (blu - grn) / std::max( 1e-10f, red - grn);
                blu = hue_fac * (newRed - grn) + grn;
            }

            red = newRed;
        }

        out[0] = red;
        out[1] = grn;
        out[2] = blu;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_ACES_RedMod03_Inv::Renderer_ACES_RedMod03_Inv(ConstFixedFunctionOpDataRcPtr & data)
    :   Renderer_ACES_RedMod03_Fwd(data)
{
}

void Renderer_ACES_RedMod03_Inv::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        float red = in[0];
        float grn = in[1];
        float blu = in[2];

        const float f_H = CalcHueWeight(red, grn, blu, m_inv_width);
        if (f_H > 0.f)
        {
            const float minChan = (grn < blu) ? grn: blu;

            const float a = f_H * m_1minusScale - 1.f;
            const float b = red - f_H * (m_pivot + minChan) * m_1minusScale;
            const float c = f_H * m_pivot * minChan * m_1minusScale;

            const float newRed = ( -b - sqrt( b * b - 4.f * a * c)) / ( 2.f * a);

            // Restore hue.
            if (grn >= blu) // red >= grn >= blu
            {
                const float hue_fac = (grn - blu) / std::max( 1e-10f, red - blu);
                grn = hue_fac * (newRed - blu) + blu;
            }
            else            // red >= blu >= grn
            {
                const float hue_fac = (blu - grn) / std::max( 1e-10f, red - grn);
                blu = hue_fac * (newRed - grn) + grn;
            }

            red = newRed;
        }

        out[0] = red;
        out[1] = grn;
        out[2] = blu;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_ACES_RedMod10_Fwd::Renderer_ACES_RedMod10_Fwd(ConstFixedFunctionOpDataRcPtr & /*data*/)
    :   OpCPU()
{
    // Constants that define a scale and offset to be applied to the red channel.
    m_1minusScale = 1.f - 0.82f;  // (1. - scale) from the original ctl code
    m_pivot = 0.03f;  // offset will be applied to unnormalized input values

    //float width = 135;  // width of hue region (in degrees)
    // Actually want to multiply by 4 / width (in radians).
    // Note: _inv_width = 4 / (width * pi/180)
    m_inv_width = 1.6976527263135504f;
}

void Renderer_ACES_RedMod10_Fwd::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        float red = in[0];
        const float grn = in[1];
        const float blu = in[2];

        const float f_H = CalcHueWeight(red, grn, blu, m_inv_width);

        // Hue is in range of the window, apply mod.
        if (f_H > 0.f)
        {
            const float f_S = CalcSatWeight(red, grn, blu, m_noiseLimit);

            // Apply red modifier.  NB:  Red is still at inScale.

            //const float modRed = (red - _pivot) * _scale + _pivot;
            //const float tmp = red * (1.f - f_S) + f_S * modRed;
            //const float newRed = red * (1.f - f_H) + f_H * tmp;
            // The above is easier to understand, but reduces down to the following:
            red = red + f_H * f_S * (m_pivot - red) * m_1minusScale;
        }

        out[0] = red;
        out[1] = grn;
        out[2] = blu;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_ACES_RedMod10_Inv::Renderer_ACES_RedMod10_Inv(ConstFixedFunctionOpDataRcPtr & data)
    :   Renderer_ACES_RedMod10_Fwd(data)
{
}

void Renderer_ACES_RedMod10_Inv::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        float red = in[0];
        const float grn = in[1];
        const float blu = in[2];

        const float f_H = CalcHueWeight(red, grn, blu, m_inv_width);
        if (f_H > 0.f)
        {
            const float minChan = (grn < blu) ? grn: blu;

            const float a = f_H * m_1minusScale - 1.f;
            const float b = red - f_H * (m_pivot + minChan) * m_1minusScale;
            const float c = f_H * m_pivot * minChan * m_1minusScale;

            // TODO: Replace sqrt with faster approx. (also in RedMod03 above).
            red = ( -b - sqrt( b * b - 4.f * a * c)) / ( 2.f * a);
        }

        out[0] = red;
        out[1] = grn;
        out[2] = blu;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_ACES_Glow03_Fwd::Renderer_ACES_Glow03_Fwd(ConstFixedFunctionOpDataRcPtr & /*data*/,
                                                   float glowGain,
                                                   float glowMid)
    :   OpCPU()
{
    m_glowGain = glowGain;
    m_glowMid = glowMid;
}

__inline float rgbToYC(const float red, const float grn, const float blu)
{
    // Convert RGB to YC (luma + chroma factor).
    const float YCRadiusWeight = 1.75f;
    const float chroma = sqrt( blu*(blu-grn) + grn*(grn-red) + red*(red-blu) );
    const float YC = (blu + grn + red + YCRadiusWeight * chroma) / 3.f;
    return YC;
}

__inline float SigmoidShaper(const float sat)
{
    const float x = (sat - 0.4f) * 5.f;
    const float sign = std::copysignf(1.f, x);
    const float t = std::max(0.f, 1.f - 0.5f * sign * x);
    const float s = (1.f + sign * (1.f - t * t)) * 0.5f;
    return s;
}

void Renderer_ACES_Glow03_Fwd::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float red = in[0];
        const float grn = in[1];
        const float blu = in[2];

        // NB: YC is at inScale.
        const float YC = rgbToYC(red, grn, blu);

        const float sat = CalcSatWeight(red, grn, blu, m_noiseLimit);

        const float s = SigmoidShaper(sat);

        const float GlowGain = m_glowGain * s;
        const float GlowMid = m_glowMid;

        // Apply FwdGlow.
        float glowGainOut;
        if (YC >= GlowMid * 2.f)
        {
            glowGainOut = 0.f;
        }
        else if (YC <= GlowMid * 2.f / 3.f)
        {
            glowGainOut = GlowGain;
        }
        else
        {
            glowGainOut = GlowGain * (GlowMid / YC - 0.5f);
        }

        // Calculate glow factor.
        const float addedGlow = 1.f + glowGainOut;

        out[0] = red * addedGlow;
        out[1] = grn * addedGlow;
        out[2] = blu * addedGlow;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_ACES_Glow03_Inv::Renderer_ACES_Glow03_Inv(ConstFixedFunctionOpDataRcPtr & data,
                                                   float glowGain,
                                                   float glowMid)
    :   Renderer_ACES_Glow03_Fwd(data, glowGain, glowMid)
{
}

void Renderer_ACES_Glow03_Inv::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float red = in[0];
        const float grn = in[1];
        const float blu = in[2];

        // NB: YC is at inScale.
        const float YC = rgbToYC(red, grn, blu);

        const float sat = CalcSatWeight(red, grn, blu, m_noiseLimit);

        const float s = SigmoidShaper(sat);

        const float GlowGain = m_glowGain * s;
        const float GlowMid = m_glowMid;

        // Apply InvGlow.
        float glowGainOut;
        if (YC >= GlowMid * 2.f)
        {
            glowGainOut = 0.f;
        }
        else if (YC <= (1.f + GlowGain) * GlowMid * 2.f / 3.f)
        {
            glowGainOut = -GlowGain / (1.f + GlowGain);
        }
        else
        {
            glowGainOut = GlowGain * (GlowMid / YC - 0.5f) / (GlowGain * 0.5f - 1.f);
        }

        // Calculate glow factor.
        const float reducedGlow = 1.f + glowGainOut;

        out[0] = red * reducedGlow;
        out[1] = grn * reducedGlow;
        out[2] = blu * reducedGlow;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_ACES_DarkToDim10_Fwd::Renderer_ACES_DarkToDim10_Fwd(ConstFixedFunctionOpDataRcPtr & /*data*/,
                                                             float gamma)
    :   OpCPU()
{
    m_gamma = gamma - 1.f;  // compute Y^gamma / Y
}

void Renderer_ACES_DarkToDim10_Fwd::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float red = in[0];
        const float grn = in[1];
        const float blu = in[2];


        // With the modest 2% ACES surround, this minLum allows the min/max gain
        // applied to dark colors to be about 0.6 to 1.6.
        constexpr float minLum = 1e-10f;

        // Calculate luminance assuming input is AP1 RGB.
        const float Y = std::max( minLum, ( 0.27222871678091454f  * red + 
                                            0.67408176581114831f  * grn + 
                                            0.053689517407937051f * blu ) );

        // TODO: Currently our fast approx. requires SSE registers.
        //       Either make this whole routine SSE or make fast scalar pow.
        const float Ypow_over_Y = powf(Y, m_gamma);

        out[0] = red * Ypow_over_Y;
        out[1] = grn * Ypow_over_Y;
        out[2] = blu * Ypow_over_Y;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_REC2100_Surround::Renderer_REC2100_Surround(ConstFixedFunctionOpDataRcPtr & data)
    :   OpCPU()
{
    const auto fwd = FixedFunctionOpData::REC2100_SURROUND_FWD == data->getStyle();
    const float gamma = fwd ? (float)data->getParams()[0] : (float)(1. / data->getParams()[0]);

    m_gamma = gamma - 1.f;  // compute Y^gamma / Y
}

void Renderer_REC2100_Surround::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float red = in[0];
        const float grn = in[1];
        const float blu = in[2];

        // This threshold needs to be bigger than 1e-10 (used above) to prevent extreme
        // gain in dark colors, yet smaller than 1e-2 to prevent distorting the shape of
        // the HLG EOTF curve.  Max gain = 1e-4 ** (0.78-1) = 7.6 for HLG min gamma of 0.78.
        // 
        // TODO: Should have forward & reverse versions of this so the threshold can be
        //       adjusted correctly for the reverse direction.
        constexpr float minLum = 1e-4f;

        // Calculate luminance assuming input is Rec.2100 RGB.
        // TODO: Add another parameter to allow using other primaries.
        const float Y = std::max( minLum, ( 0.2627f * red + 
                                            0.6780f * grn + 
                                            0.0593f * blu ) );

        // TODO: Currently our fast approx. requires SSE registers.
        //       Either make this whole routine SSE or make fast scalar pow.
        const float Ypow_over_Y = powf(Y, m_gamma);

        out[0] = red * Ypow_over_Y;
        out[1] = grn * Ypow_over_Y;
        out[2] = blu * Ypow_over_Y;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_RGB_TO_HSV::Renderer_RGB_TO_HSV(ConstFixedFunctionOpDataRcPtr & /*data*/)
    :   OpCPU()
{
}

// These HSV conversion routines are designed to handle extended range values.  If RGB are
// non-negative or all negative, S is on [0,1].  If RGB are a mix of pos and neg, S is on [1,2].
// The H is [0,1] for all inputs, with 1 meaning 360 degrees.  For RGB on [0,1], the algorithm
// is the classic HSV formula.

void Renderer_RGB_TO_HSV::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float red = in[0];
        const float grn = in[1];
        const float blu = in[2];

        const float rgb_min = std::min( std::min( red, grn ), blu );
        const float rgb_max = std::max( std::max( red, grn ), blu );

        float val = rgb_max;
        float sat = 0.f;
        float hue = 0.f;

        if (rgb_min != rgb_max)
        {
            // Sat
            float delta = rgb_max - rgb_min;
            if (rgb_max != 0.f)
            {
                sat = delta / rgb_max;
            }

            // Hue
            if (red == rgb_max)
            {
                hue = (grn - blu) / delta;
            }
            else
            {
                if (grn == rgb_max)
                {
                    hue = 2.0f + (blu - red) / delta;
                }
                else
                {
                    hue = 4.0f + (red - grn) / delta;
                }
            }
            if (hue < 0.f)
            {
                hue += 6.f;
            }
            hue *= 0.16666666666666666f;
        }

        // Handle extended range inputs.
        if (rgb_min < 0.f)
        {
            val += rgb_min;
        }
        if (-rgb_min > rgb_max)
        {
            sat = (rgb_max - rgb_min) / -rgb_min;
        }

        out[0] = hue;
        out[1] = sat;
        out[2] = val;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_HSV_TO_RGB::Renderer_HSV_TO_RGB(ConstFixedFunctionOpDataRcPtr & /*data*/)
    :   OpCPU()
{   
}

// This algorithm is designed to handle extended range values.  H is nominally on [0,1],
// but values outside this are accepted and wrapped back into range.  S is nominally on
// [0,1] for non-negative RGB but may extend up to 2.  S values outside [0,MAX_SAT] are
// clamped.  MAX_SAT should be slightly below 2 since very large RGB outputs will result
// as S approaches 2.  Applications may want to design UIs to limit S even more than this
// function does (e.g. 1.9) to avoid RGB results in the thousands.

void Renderer_HSV_TO_RGB::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for (long idx=0; idx<numPixels; ++idx)
    {
        constexpr float MAX_SAT = 1.999f;

        const float hue = ( in[0] - std::floor( in[0] ) ) * 6.f;
        const float sat = Clamp( in[1], 0.f, MAX_SAT );
        const float val = in[2];

        const float red = Clamp( std::fabs(hue - 3.f) - 1.f, 0.f, 1.f );
        const float grn = Clamp( 2.f - std::fabs(hue - 2.f), 0.f, 1.f );
        const float blu = Clamp( 2.f - std::fabs(hue - 4.f), 0.f, 1.f );

        float rgb_max = val;
        float rgb_min = val * (1.f - sat);

        // Handle extended range inputs.
        if (sat > 1.f)
        {
            rgb_min = val * (1.f - sat) / (2.f - sat);
            rgb_max = val - rgb_min;
        }
        if (val < 0.f)
        {
            rgb_min = val / (2.f - sat);
            rgb_max = val - rgb_min;
        }

        const float delta = rgb_max - rgb_min;
        out[0] = red * delta + rgb_min;
        out[1] = grn * delta + rgb_min;
        out[2] = blu * delta + rgb_min;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_XYZ_TO_xyY::Renderer_XYZ_TO_xyY(ConstFixedFunctionOpDataRcPtr & /*data*/)
    :   OpCPU()
{   
}

void Renderer_XYZ_TO_xyY::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float X = in[0];
        const float Y = in[1];
        const float Z = in[2];

        float d = X + Y + Z;
        d = (d == 0.f) ? 0.f : 1.f / d;
        const float x = X * d;
        const float y = Y * d;

        out[0] = x;
        out[1] = y;
        out[2] = Y;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_xyY_TO_XYZ::Renderer_xyY_TO_XYZ(ConstFixedFunctionOpDataRcPtr & /*data*/)
    :   OpCPU()
{   
}

void Renderer_xyY_TO_XYZ::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float x = in[0];
        const float y = in[1];
        const float Y = in[2];

        const float d = (y == 0.f) ? 0.f : 1.f / y;
        const float X = Y * x * d;
        const float Z = Y * (1.f - x - y) * d;

        out[0] = X;
        out[1] = Y;
        out[2] = Z;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_XYZ_TO_uvY::Renderer_XYZ_TO_uvY(ConstFixedFunctionOpDataRcPtr & /*data*/)
    :   OpCPU()
{   
}

void Renderer_XYZ_TO_uvY::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // TODO: Check robustness for arbitrary float inputs.

        const float X = in[0];
        const float Y = in[1];
        const float Z = in[2];

        float d = X + 15.f * Y + 3.f * Z;
        d = (d == 0.f) ? 0.f : 1.f / d;
        const float u = 4.f * X * d;
        const float v = 9.f * Y * d;

        out[0] = u;
        out[1] = v;
        out[2] = Y;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_uvY_TO_XYZ::Renderer_uvY_TO_XYZ(ConstFixedFunctionOpDataRcPtr & /*data*/)
    :   OpCPU()
{   
}

void Renderer_uvY_TO_XYZ::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // TODO: Check robustness for arbitrary float inputs.

        const float u = in[0];
        const float v = in[1];
        const float Y = in[2];

        const float d = (v == 0.f) ? 0.f : 1.f / v;
        const float X = (9.f / 4.f) * Y * u * d;
        const float Z = (3.f / 4.f) * Y * (4.f - u - 6.666666666666667f * v) * d;

        out[0] = X;
        out[1] = Y;
        out[2] = Z;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_XYZ_TO_LUV::Renderer_XYZ_TO_LUV(ConstFixedFunctionOpDataRcPtr & /*data*/)
    :   OpCPU()
{   
}

void Renderer_XYZ_TO_LUV::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // TODO: Check robustness for arbitrary float inputs.

        const float X = in[0];
        const float Y = in[1];
        const float Z = in[2];

        float d = X + 15.f * Y + 3.f * Z;
        d = (d == 0.f) ? 0.f : 1.f / d;
        const float u = 4.f * X * d;
        const float v = 9.f * Y * d;

        const float Lstar = ( Y <= 0.008856451679f ) ? 9.0329629629629608f * Y : 
                                                     1.16f * powf( Y, 0.333333333f ) - 0.16f;
        const float ustar = 13.f * Lstar * (u - 0.19783001f);   // D65 white
        const float vstar = 13.f * Lstar * (v - 0.46831999f);   // D65 white

        out[0] = Lstar;
        out[1] = ustar;
        out[2] = vstar;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_LUV_TO_XYZ::Renderer_LUV_TO_XYZ(ConstFixedFunctionOpDataRcPtr & /*data*/)
    :   OpCPU()
{   
}

void Renderer_LUV_TO_XYZ::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // TODO: Check robustness for arbitrary float inputs.

        const float Lstar = in[0];
        const float ustar = in[1];
        const float vstar = in[2];

        const float d = (Lstar == 0.f) ? 0.f : 0.076923076923076927f / Lstar;
        const float u = ustar * d + 0.19783001f;    // D65 white
        const float v = vstar * d + 0.46831999f;    // D65 white

        const float tmp = (Lstar + 0.16f) * 0.86206896551724144f;
        const float Y = (Lstar <= 0.08f) ? 0.11070564598794539f * Lstar : tmp * tmp * tmp;

        const float dd = (v == 0.f) ? 0.f : 0.25f / v;
        const float X = 9.f * Y * u * dd;
        const float Z = Y * (12.f - 3.f * u - 20.f * v) * dd;

        out[0] = X;
        out[1] = Y;
        out[2] = Z;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}




///////////////////////////////////////////////////////////////////////////////



ConstOpCPURcPtr GetFixedFunctionCPURenderer(ConstFixedFunctionOpDataRcPtr & func)
{
    switch(func->getStyle())
    {
        case FixedFunctionOpData::ACES_RED_MOD_03_FWD:
        {
            return std::make_shared<Renderer_ACES_RedMod03_Fwd>(func);
        }
        case FixedFunctionOpData::ACES_RED_MOD_03_INV:
        {
            return std::make_shared<Renderer_ACES_RedMod03_Inv>(func);
        }
        case FixedFunctionOpData::ACES_RED_MOD_10_FWD:
        {
            return std::make_shared<Renderer_ACES_RedMod10_Fwd>(func);
        }
        case FixedFunctionOpData::ACES_RED_MOD_10_INV:
        {
            return std::make_shared<Renderer_ACES_RedMod10_Inv>(func);
        }
        case FixedFunctionOpData::ACES_GLOW_03_FWD:
        {
            return std::make_shared<Renderer_ACES_Glow03_Fwd>(func, 0.075f, 0.1f);
        }        
        case FixedFunctionOpData::ACES_GLOW_03_INV:
        {
            return std::make_shared<Renderer_ACES_Glow03_Inv>(func, 0.075f, 0.1f);
        }        
        case FixedFunctionOpData::ACES_GLOW_10_FWD:
        {
            return std::make_shared<Renderer_ACES_Glow03_Fwd>(func, 0.05f, 0.08f);
        }
        case FixedFunctionOpData::ACES_GLOW_10_INV:
        {
            return std::make_shared<Renderer_ACES_Glow03_Inv>(func, 0.05f, 0.08f);
        }
        case FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD:
        {
            return std::make_shared<Renderer_ACES_DarkToDim10_Fwd>(func, 0.9811f);
        }
        case FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV:
        {
            return std::make_shared<Renderer_ACES_DarkToDim10_Fwd>(func, 1.0192640913260627f);
        }
        case FixedFunctionOpData::REC2100_SURROUND_FWD:
        case FixedFunctionOpData::REC2100_SURROUND_INV:
        {
            // Sharing same renderer (param will be inverted to handle direction).
            return std::make_shared<Renderer_REC2100_Surround>(func);
        }

        case FixedFunctionOpData::RGB_TO_HSV:
        {
            return std::make_shared<Renderer_RGB_TO_HSV>(func);
        }
        case FixedFunctionOpData::HSV_TO_RGB:
        {
            return std::make_shared<Renderer_HSV_TO_RGB>(func);
        }

        case FixedFunctionOpData::XYZ_TO_xyY:
        {
            return std::make_shared<Renderer_XYZ_TO_xyY>(func);
        }
        case FixedFunctionOpData::xyY_TO_XYZ:
        {
            return std::make_shared<Renderer_xyY_TO_XYZ>(func);
        }

        case FixedFunctionOpData::XYZ_TO_uvY:
        {
            return std::make_shared<Renderer_XYZ_TO_uvY>(func);
        }
        case FixedFunctionOpData::uvY_TO_XYZ:
        {
            return std::make_shared<Renderer_uvY_TO_XYZ>(func);
        }

        case FixedFunctionOpData::XYZ_TO_LUV:
        {
            return std::make_shared<Renderer_XYZ_TO_LUV>(func);
        }
        case FixedFunctionOpData::LUV_TO_XYZ:
        {
            return std::make_shared<Renderer_LUV_TO_XYZ>(func);
        }
    }

    throw Exception("Unsupported FixedFunction style");
}

} // namespace OCIO_NAMESPACE
