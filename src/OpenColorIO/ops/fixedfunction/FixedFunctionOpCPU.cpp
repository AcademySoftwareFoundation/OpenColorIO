// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <iomanip>

#include <OpenColorIO/OpenColorIO.h>

#include "ACES2/Common.h"
#include "ACES2/Init.h"
#include "ACES2/ColorLib.h"
#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/fixedfunction/FixedFunctionOpCPU.h"


namespace OCIO_NAMESPACE
{

class Renderer_ACES_RedMod03_Fwd : public OpCPU
{
public:
    Renderer_ACES_RedMod03_Fwd() = delete;
    explicit Renderer_ACES_RedMod03_Fwd(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_1minusScale;
    float m_pivot;
    float m_inv_width;

    static constexpr float m_noiseLimit = 1e-2f;
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
    explicit Renderer_ACES_RedMod10_Fwd(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_1minusScale;
    float m_pivot;
    float m_inv_width;

    static constexpr float m_noiseLimit = 1e-2f;
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
    Renderer_ACES_Glow03_Fwd() = delete;
    Renderer_ACES_Glow03_Fwd(ConstFixedFunctionOpDataRcPtr & data, float glowGain, float glowMid);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_glowGain, m_glowMid;

    static constexpr float m_noiseLimit = 1e-2f;
};

class Renderer_ACES_Glow03_Inv : public Renderer_ACES_Glow03_Fwd
{
public:
    Renderer_ACES_Glow03_Inv(ConstFixedFunctionOpDataRcPtr & data, float glowGain, float glowMid);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_ACES_DarkToDim10_Fwd : public OpCPU
{
public:
    Renderer_ACES_DarkToDim10_Fwd() = delete;
    Renderer_ACES_DarkToDim10_Fwd(ConstFixedFunctionOpDataRcPtr & data, float gamma);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_gamma;
};

class Renderer_ACES_GamutComp13_Fwd : public OpCPU
{
public:
    Renderer_ACES_GamutComp13_Fwd() = delete;
    explicit Renderer_ACES_GamutComp13_Fwd(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_limCyan;
    float m_limMagenta;
    float m_limYellow;
    float m_thrCyan;
    float m_thrMagenta;
    float m_thrYellow;
    float m_power;

    float m_scaleCyan;
    float m_scaleMagenta;
    float m_scaleYellow;
};

class Renderer_ACES_GamutComp13_Inv : public Renderer_ACES_GamutComp13_Fwd
{
public:
    explicit Renderer_ACES_GamutComp13_Inv(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_ACES_RGB_TO_JMh_20 : public OpCPU
{
public:
    Renderer_ACES_RGB_TO_JMh_20() = delete;
    explicit Renderer_ACES_RGB_TO_JMh_20(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

private:
    void fwd(const void * inImg, void * outImg, long numPixels) const;
    void inv(const void * inImg, void * outImg, long numPixels) const;

protected:
    bool m_fwd;
    ACES2::JMhParams m_p;
};

class Renderer_ACES_TONESCALE_COMPRESS_20 : public OpCPU
{
public:
    Renderer_ACES_TONESCALE_COMPRESS_20() = delete;
    explicit Renderer_ACES_TONESCALE_COMPRESS_20(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

private:
    void fwd(const void * inImg, void * outImg, long numPixels) const;
    void inv(const void * inImg, void * outImg, long numPixels) const;

protected:
    bool m_fwd;
    ACES2::JMhParams m_p;
    ACES2::ToneScaleParams m_t;
    ACES2::ChromaCompressParams m_c;
};

class Renderer_ACES_GAMUT_COMPRESS_20 : public OpCPU
{
public:
    Renderer_ACES_GAMUT_COMPRESS_20() = delete;
    explicit Renderer_ACES_GAMUT_COMPRESS_20(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

private:
    void fwd(const void * inImg, void * outImg, long numPixels) const;
    void inv(const void * inImg, void * outImg, long numPixels) const;

protected:
    bool m_fwd;
    ACES2::GamutCompressParams m_g;
};

class Renderer_REC2100_Surround : public OpCPU
{
public:
    Renderer_REC2100_Surround() = delete;
    explicit Renderer_REC2100_Surround(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:
    float m_gamma;
};

class Renderer_RGB_TO_HSV : public OpCPU
{
public:
    Renderer_RGB_TO_HSV() = delete;
    explicit Renderer_RGB_TO_HSV(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_HSV_TO_RGB : public OpCPU
{
public:
    Renderer_HSV_TO_RGB() = delete;
    explicit Renderer_HSV_TO_RGB(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_XYZ_TO_xyY : public OpCPU
{
public:
    Renderer_XYZ_TO_xyY() = delete;
    explicit Renderer_XYZ_TO_xyY(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_xyY_TO_XYZ : public OpCPU
{
public:
    Renderer_xyY_TO_XYZ() = delete;
    explicit Renderer_xyY_TO_XYZ(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_XYZ_TO_uvY : public OpCPU
{
public:
    Renderer_XYZ_TO_uvY() = delete;
    explicit Renderer_XYZ_TO_uvY(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_uvY_TO_XYZ : public OpCPU
{
public:
    Renderer_uvY_TO_XYZ() = delete;
    explicit Renderer_uvY_TO_XYZ(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_XYZ_TO_LUV : public OpCPU
{
public:
    Renderer_XYZ_TO_LUV() = delete;
    explicit Renderer_XYZ_TO_LUV(ConstFixedFunctionOpDataRcPtr & data);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class Renderer_LUV_TO_XYZ : public OpCPU
{
public:
    Renderer_LUV_TO_XYZ() = delete;
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
    static constexpr float sqrt3 = 1.7320508075688772f;
    const float b = sqrt3 * (grn - blu);

    const float hue = std::atan2(b, a);

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
    static constexpr float _M[4][4] = {
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
        static constexpr float minLum = 1e-10f;

        // Calculate luminance assuming input is AP1 RGB.
        const float Y = std::max( minLum, ( 0.27222871678091454f  * red + 
                                            0.67408176581114831f  * grn + 
                                            0.053689517407937051f * blu ) );

        // TODO: Currently our fast approx. requires SSE registers.
        //       Either make this whole routine SSE or make fast scalar pow.
        const float Ypow_over_Y = std::pow(Y, m_gamma);

        out[0] = red * Ypow_over_Y;
        out[1] = grn * Ypow_over_Y;
        out[2] = blu * Ypow_over_Y;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

float compress(float dist, float thr, float scale, float power)
{
    // Normalize distance outside threshold by scale factor.
    const float nd = (dist - thr) / scale;
    const float p = std::pow(nd, power);

    // Note: assume the compiler will optimize out "1.0f / power".
    return thr + scale * nd / (std::pow(1.0f + p, 1.0f / power));
}

float uncompress(float dist, float thr, float scale, float power)
{
    // Avoid singularity
    if (dist >= (thr + scale))
    {
        return dist;
    }
    else
    {
        // Normalize distance outside threshold by scale factor.
        const float nd = (dist - thr) / scale;
        const float p = std::pow(nd, power);

        // Note: assume the compiler will optimize out "1.0f / power".
        return thr + scale * std::pow(-(p / (p - 1.0f)), 1.0f / power);
    }
}

template <typename Func>
float gamut_comp(float val, float ach, float thr, float scale, float power, Func f)
{
    // Note: Strict equality is fine here. For example, consider the RGB { 1e-7, 0, -1e-5 }.
    // This will become a dist = (1e-7 - -1e-5) / 1e-7 = 101.0. So, there will definitely be
    // very large dist values. But the compression function is able to handle those since
    // they approach the asymptote. So 101 will become something like 1.12.  Then at the
    // other end the B values is reconstructed as 1e-7 - 1.12 * 1e-7 = -1.2e-8. So it went
    // from -1e-5 to -1.2e-8, but it caused no numerical instability.
    if (ach == 0.0f)
    {
        return 0.0f;
    }

    // Distance from the achromatic axis, aka inverse RGB ratios
    const float dist = (ach - val) / std::fabs(ach);

    // No compression below threshold
    if (dist < thr)
    {
        return val;
    }

    // Compress / Uncompress distance with parameterized shaper function.
    const float comprDist = f(dist, thr, scale, power);

    // Recalculate RGB from compressed distance and achromatic.
    const float compr = ach - comprDist * std::fabs(ach);

    return compr;
}

Renderer_ACES_GamutComp13_Fwd::Renderer_ACES_GamutComp13_Fwd(ConstFixedFunctionOpDataRcPtr & data)
    :   OpCPU()
{
    m_limCyan        = (float) data->getParams()[0];
    m_limMagenta     = (float) data->getParams()[1];
    m_limYellow      = (float) data->getParams()[2];
    m_thrCyan        = (float) data->getParams()[3];
    m_thrMagenta     = (float) data->getParams()[4];
    m_thrYellow      = (float) data->getParams()[5];
    m_power          = (float) data->getParams()[6];

    // Precompute scale factor for y = 1 intersect
    auto f_scale = [this](float lim, float thr) {
        return (lim - thr) / std::pow(std::pow((1.0f - thr) / (lim - thr), -m_power) - 1.0f, 1.0f / m_power);
    };
    m_scaleCyan      = f_scale(m_limCyan,    m_thrCyan);
    m_scaleMagenta   = f_scale(m_limMagenta, m_thrMagenta);
    m_scaleYellow    = f_scale(m_limYellow,  m_thrYellow);
}

void Renderer_ACES_GamutComp13_Fwd::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float red = in[0];
        const float grn = in[1];
        const float blu = in[2];

        // Achromatic axis
        const float ach = std::max(red, std::max(grn, blu));

        out[0] = gamut_comp(red, ach, m_thrCyan,    m_scaleCyan,    m_power, compress);
        out[1] = gamut_comp(grn, ach, m_thrMagenta, m_scaleMagenta, m_power, compress);
        out[2] = gamut_comp(blu, ach, m_thrYellow,  m_scaleYellow,  m_power, compress);
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_ACES_GamutComp13_Inv::Renderer_ACES_GamutComp13_Inv(ConstFixedFunctionOpDataRcPtr & data)
    :   Renderer_ACES_GamutComp13_Fwd(data)
{
}

void Renderer_ACES_GamutComp13_Inv::apply(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float red = in[0];
        const float grn = in[1];
        const float blu = in[2];

        // Achromatic axis
        const float ach = std::max(red, std::max(grn, blu));

        out[0] = gamut_comp(red, ach, m_thrCyan,    m_scaleCyan,    m_power, uncompress);
        out[1] = gamut_comp(grn, ach, m_thrMagenta, m_scaleMagenta, m_power, uncompress);
        out[2] = gamut_comp(blu, ach, m_thrYellow,  m_scaleYellow,  m_power, uncompress);
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_ACES_RGB_TO_JMh_20::Renderer_ACES_RGB_TO_JMh_20(ConstFixedFunctionOpDataRcPtr & data)
    :   OpCPU()
{
    m_fwd = FixedFunctionOpData::ACES_RGB_TO_JMh_20 == data->getStyle();

    const float red_x   = (float) data->getParams()[0];
    const float red_y   = (float) data->getParams()[1];
    const float green_x = (float) data->getParams()[2];
    const float green_y = (float) data->getParams()[3];
    const float blue_x  = (float) data->getParams()[4];
    const float blue_y  = (float) data->getParams()[5];
    const float white_x = (float) data->getParams()[6];
    const float white_y = (float) data->getParams()[7];

    const Primaries primaries = {
        {red_x  , red_y  },
        {green_x, green_y},
        {blue_x , blue_y },
        {white_x, white_y}
    };

    m_p = ACES2::init_JMhParams(primaries);
}

void Renderer_ACES_RGB_TO_JMh_20::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_fwd)
    {
        fwd(inImg, outImg, numPixels);
    }
    else
    {
        inv(inImg, outImg, numPixels);
    }
}

float panlrc_forward(float v, float F_L)
{
    const float F_L_v = powf(F_L * std::abs(v) / 100.f, 0.42f);
    return (400.f * std::copysign(1., v) * F_L_v) / (27.13f + F_L_v);
}

float panlrc_inverse(float v, float F_L)
{
    return std::copysign(1., v) * 100.f / F_L * powf((27.13f * std::abs(v) / (400.f - std::abs(v))), 1.f / 0.42f);
}

void Renderer_ACES_RGB_TO_JMh_20::fwd(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float red = in[0];
        const float grn = in[1];
        const float blu = in[2];

        const float red_m = red * m_p.MATRIX_RGB_to_CAM16[0] + grn * m_p.MATRIX_RGB_to_CAM16[1] + blu * m_p.MATRIX_RGB_to_CAM16[2];
        const float grn_m = red * m_p.MATRIX_RGB_to_CAM16[3] + grn * m_p.MATRIX_RGB_to_CAM16[4] + blu * m_p.MATRIX_RGB_to_CAM16[5];
        const float blu_m = red * m_p.MATRIX_RGB_to_CAM16[6] + grn * m_p.MATRIX_RGB_to_CAM16[7] + blu * m_p.MATRIX_RGB_to_CAM16[8];

        const float red_a =  panlrc_forward(red_m * m_p.D_RGB[0], m_p.F_L);
        const float grn_a =  panlrc_forward(grn_m * m_p.D_RGB[1], m_p.F_L);
        const float blu_a =  panlrc_forward(blu_m * m_p.D_RGB[2], m_p.F_L);

        const float A = 2.f * red_a + grn_a + 0.05f * blu_a;
        const float a = red_a - 12.f * grn_a / 11.f + blu_a / 11.f;
        const float b = (red_a + grn_a - 2.f * blu_a) / 9.f;

        const float J = 100.f * powf(A / m_p.A_w, 0.59f * m_p.z);

        const float M = J == 0.f ? 0.f : 43.f * 0.9f * sqrt(a * a + b * b);

        const float PI = 3.14159265358979f;
        const float h_rad = std::atan2(b, a);
        float h = std::fmod(h_rad * 180.f / PI, 360.f);
        if (h < 0.f)
        {
            h += 360.f;
        }

        out[0] = J;
        out[1] = M;
        out[2] = h;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

void Renderer_ACES_RGB_TO_JMh_20::inv(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float J = in[0];
        const float M = in[1];
        const float h = in[2];

        const float PI = 3.14159265358979f;
        const float h_rad = h * PI / 180.f;

        const float scale = M / (43.f * 0.9f);
        const float A = m_p.A_w * powf(J / 100.f, 1.f / (0.59f * m_p.z));
        const float a = scale * cos(h_rad);
        const float b = scale * sin(h_rad);

        const float red_a = (460.f * A + 451.f * a + 288.f * b) / 1403.f;
        const float grn_a = (460.f * A - 891.f * a - 261.f * b) / 1403.f;
        const float blu_a = (460.f * A - 220.f * a - 6300.f * b) / 1403.f;

        float red_m = panlrc_inverse(red_a, m_p.F_L) / m_p.D_RGB[0];
        float grn_m = panlrc_inverse(grn_a, m_p.F_L) / m_p.D_RGB[1];
        float blu_m = panlrc_inverse(blu_a, m_p.F_L) / m_p.D_RGB[2];

        const float red = red_m * m_p.MATRIX_CAM16_to_RGB[0] + grn_m * m_p.MATRIX_CAM16_to_RGB[1] + blu_m * m_p.MATRIX_CAM16_to_RGB[2];
        const float grn = red_m * m_p.MATRIX_CAM16_to_RGB[3] + grn_m * m_p.MATRIX_CAM16_to_RGB[4] + blu_m * m_p.MATRIX_CAM16_to_RGB[5];
        const float blu = red_m * m_p.MATRIX_CAM16_to_RGB[6] + grn_m * m_p.MATRIX_CAM16_to_RGB[7] + blu_m * m_p.MATRIX_CAM16_to_RGB[8];

        out[0] = red;
        out[1] = grn;
        out[2] = blu;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_ACES_TONESCALE_COMPRESS_20::Renderer_ACES_TONESCALE_COMPRESS_20(ConstFixedFunctionOpDataRcPtr & data)
    :   OpCPU()
{
    m_fwd = FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_FWD == data->getStyle();

    const float peak_luminance = (float) data->getParams()[0];

    m_p = ACES2::init_JMhParams(ACES_AP0::primaries);
    m_t = ACES2::init_ToneScaleParams(peak_luminance);
    m_c = ACES2::init_ChromaCompressParams(peak_luminance);
}

void Renderer_ACES_TONESCALE_COMPRESS_20::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_fwd)
    {
        fwd(inImg, outImg, numPixels);
    }
    else
    {
        inv(inImg, outImg, numPixels);
    }
}

float chroma_compress_norm(float h, float chroma_compress_scale)
{
    const float PI = 3.14159265358979f;
    const float h_rad = h / 180.f * PI;
    const float a = cos(h_rad);
    const float b = sin(h_rad);
    const float cos_hr2 = a * a - b * b;
    const float sin_hr2 = 2.0f * a * b;
    const float cos_hr3 = 4.0f * a * a * a - 3.0f * a;
    const float sin_hr3 = 3.0f * b - 4.0f * b * b * b;

    const float M = 11.34072f * a +
              16.46899f * cos_hr2 +
               7.88380f * cos_hr3 +
              14.66441f * b +
              -6.37224f * sin_hr2 +
               9.19364f * sin_hr3 +
              77.12896f;

    return M * chroma_compress_scale;
}

float wrap_to_360(float hue)
{
    float y = std::fmod(hue, 360.f);
    if ( y < 0.f)
    {
        y = y + 360.f;
    }
    return y;
}

int hue_position_in_uniform_table(float hue, int table_size)
{
    const float wrapped_hue = wrap_to_360(hue);
    return (wrapped_hue / 360.f * table_size);
}

int next_position_in_table(int entry, int table_size)
{
    return (entry + 1) % table_size;
}

float reach_m_from_table(float h, const ACES2::Table1D &gt)
{
    int i_lo = hue_position_in_uniform_table(h, gt.size);
    int i_hi = next_position_in_table(i_lo, gt.size);

    const float t = (h - i_lo) / (i_hi - i_lo);
    return lerpf(gt.table[i_lo], gt.table[i_hi], t);
}

float toe_fwd( float x, float limit, float k1_in, float k2_in)
{
    if (x > limit)
    {
        return x;
    }

    const float k2 = std::max(k2_in, 0.001f);
    const float k1 = sqrt(k1_in * k1_in + k2 * k2);
    const float k3 = (limit + k1) / (limit + k2);
    return 0.5f * (k3 * x - k1 + sqrt((k3 * x - k1) * (k3 * x - k1) + 4.f * k2 * k3 * x));
}

float toe_inv( float x, float limit, float k1_in, float k2_in)
{
    if (x > limit)
    {
        return x;
    }

    const float k2 = std::max(k2_in, 0.001f);
    const float k1 = sqrt(k1_in * k1_in + k2 * k2);
    const float k3 = (limit + k1) / (limit + k2);
    return (x * x + k1 * x) / (k3 * (x + k2));
}

void Renderer_ACES_TONESCALE_COMPRESS_20::fwd(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float J = in[0];
        const float M = in[1];
        const float h = in[2];

        // Tonescale applied in Y (convert to and from J)
        const float A = m_p.A_w_J * pow(std::abs(J) / 100.f, 1.f / (0.59f * m_p.z));
        const float Y = std::copysign(1.f, J) * 100.f / m_p.F_L * pow((27.13f * A) / (400.f - A), 1.f / 0.42f) / 100.f;

        const float f = m_t.m_2 * pow(std::max(0.f, Y) / (Y + m_t.s_2), m_t.g);
        const float Y_ts = std::max(0.f, f * f / (f + m_t.t_1)) * m_t.n_r;

        const float F_L_Y = pow(m_p.F_L * std::abs(Y_ts) / 100.f, 0.42f);
        const float J_ts = std::copysign(1.f, Y_ts) * 100.f * pow(((400.f * F_L_Y) / (27.13f + F_L_Y)) / m_p.A_w_J, 0.59f * m_p.z);

        // ChromaCompress
        const float nJ = J_ts / m_c.limit_J_max;
        const float snJ = std::max(0.f, 1.f - nJ);
        const float Mnorm = chroma_compress_norm(h, m_c.chroma_compress_scale);
        const float limit = pow(nJ, m_c.model_gamma) * reach_m_from_table(h, m_c.reach_m_table) / Mnorm;

        float M_cp = M * pow(J_ts / J, m_c.model_gamma);
        M_cp = M_cp / Mnorm;
        M_cp = limit - toe_fwd(limit - M_cp, limit - 0.001f, snJ * m_c.sat, sqrt(nJ * nJ + m_c.sat_thr));
        M_cp = toe_fwd(M_cp, limit, nJ * m_c.compr, snJ);
        M_cp = M_cp * Mnorm;

        out[0] = J_ts;
        out[1] = M_cp;
        out[2] = h;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

void Renderer_ACES_TONESCALE_COMPRESS_20::inv(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float J_ts = in[0];
        const float M_cp = in[1];
        const float h = in[2];

        // Inverse Tonescale applied in Y (convert to and from J)
        const float A = m_p.A_w_J * pow(std::abs(J_ts) / 100.f, 1.f / (0.59f * m_p.z));
        const float Y_ts = std::copysign(1.f, J_ts) * 100.f / m_p.F_L * pow((27.13f * A) / (400.f - A), 1.f / 0.42f) / 100.f;

        const float Z = std::max(0.f, std::min(m_t.n / (m_t.u_2 * m_t.n_r), Y_ts));
        const float ht = (Z + sqrt(Z * (4.f * m_t.t_1 + Z))) / 2.f;
        const float Y = m_t.s_2 / (pow((m_t.m_2 / ht), (1.f / m_t.g)) - 1.f);

        const float F_L_Y = pow(m_p.F_L * std::abs(Y * 100.f) / 100.f, 0.42f);
        const float J = std::copysign(1.f, Y) * 100.f * pow(((400.f * F_L_Y) / (27.13f + F_L_Y)) / m_p.A_w_J, 0.59f * m_p.z);

        // Inverse ChromaCompress
        const float nJ = J_ts / m_c.limit_J_max;
        const float snJ = std::max(0.f, 1.f - nJ);
        const float Mnorm = chroma_compress_norm(h, m_c.chroma_compress_scale);
        const float limit = pow(nJ, m_c.model_gamma) * reach_m_from_table(h, m_c.reach_m_table) / Mnorm;

        float M = M_cp / Mnorm;
        M = toe_inv(M, limit, nJ * m_c.compr, snJ);
        M = limit - toe_inv(limit - M, limit - 0.001f, snJ * m_c.sat, sqrt(nJ * nJ + m_c.sat_thr));
        M = M * Mnorm;
        M = M * pow(J_ts / J, -m_c.model_gamma);

        out[0] = J;
        out[1] = M;
        out[2] = h;
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

Renderer_ACES_GAMUT_COMPRESS_20::Renderer_ACES_GAMUT_COMPRESS_20(ConstFixedFunctionOpDataRcPtr & data)
    :   OpCPU()
{
    m_fwd = FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_FWD == data->getStyle();

    const float peakLuminance = (float) data->getParams()[0];

    const float red_x   = (float) data->getParams()[1];
    const float red_y   = (float) data->getParams()[2];
    const float green_x = (float) data->getParams()[3];
    const float green_y = (float) data->getParams()[4];
    const float blue_x  = (float) data->getParams()[5];
    const float blue_y  = (float) data->getParams()[6];
    const float white_x = (float) data->getParams()[7];
    const float white_y = (float) data->getParams()[8];

    const Primaries limitingPrimaries = {
        {red_x  , red_y  },
        {green_x, green_y},
        {blue_x , blue_y },
        {white_x, white_y}
    };

    m_g = ACES2::init_GamutCompressParams(peakLuminance, limitingPrimaries);
}

void Renderer_ACES_GAMUT_COMPRESS_20::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_fwd)
    {
        fwd(inImg, outImg, numPixels);
    }
    else
    {
        inv(inImg, outImg, numPixels);
    }
}

float smin(float a, float b, float s)
{
    const float h = std::max(s - std::abs(a - b), 0.f) / s;
    return std::min(a, b) - h * h * h * s * (1.f / 6.f);
}

f2 cusp_from_table(float h, const ACES2::Table3D &gt)
{
    int i_lo = 0;
    int i_hi = gt.base_index + gt.size; // allowed as we have an extra entry in the table
    int i = hue_position_in_uniform_table(h, gt.size) + gt.base_index;

    while (i_lo + 1 < i_hi)
    {
        if (h > gt.table[i][2])
        {
            i_lo = i;
        }
        else
        {
            i_hi = i;
        }
        i = (i_lo + i_hi) / 2.f;
    }

    float lo[3]{};
    lo[0] = gt.table[i_hi-1][0];
    lo[1] = gt.table[i_hi-1][1];
    lo[2] = gt.table[i_hi-1][2];

    float hi[3]{};
    hi[0] = gt.table[i_hi][0];
    hi[1] = gt.table[i_hi][1];
    hi[2] = gt.table[i_hi][2];

    float t = (h - lo[2]) / (hi[2] - lo[2]);
    float cuspJ = lerpf(lo[0], hi[0], t);
    float cuspM = lerpf(lo[1], hi[1], t);

    return { cuspJ, cuspM };
}

float hue_dependent_upper_hull_gamma(float h, const ACES2::Table1D &gt)
{
    const int i_lo = hue_position_in_uniform_table(h, gt.size) + gt.base_index;
    const int i_hi = next_position_in_table(i_lo, gt.size);

    const float base_hue = i_lo - gt.base_index;

    const float t = wrap_to_360(h) - base_hue;

    return lerpf(gt.table[i_lo], gt.table[i_hi], t);
}

float get_focus_gain(float J, float cuspJ, float limit_J_max)
{
    const float thr = lerpf(cuspJ, limit_J_max, 0.3f);

    if (J > thr)
    {
        // Approximate inverse required above threshold
        float gain = (limit_J_max - thr) / std::max(0.0001f, (limit_J_max - std::min(limit_J_max, J)));
        return pow(log10(gain), 1.f / 0.55f) + 1.f;
    }
    else
    {
        // Analytic inverse possible below cusp
        return 1.f;
    }
}

float solve_J_intersect(float J, float M, float focusJ, float maxJ, float slope_gain)
{
    const float a = M / (focusJ * slope_gain);
    float b = 0.f;
    float c = 0.f;
    float intersectJ = 0.f;

    if (J < focusJ)
    {
        b = 1.f - M / slope_gain;
    }
    else
    {
        b = - (1.f + M / slope_gain + maxJ * M / (focusJ * slope_gain));
    }

    if (J < focusJ)
    {
        c = -J;
    }
    else
    {
        c = maxJ * M / slope_gain + J;
    }

    const float root = sqrt(b * b - 4.f * a * c);

    if (J < focusJ)
    {
        intersectJ = 2.f * c / (-b - root);
    }
    else
    {
        intersectJ = 2.f * c / (-b + root);
    }

    return intersectJ;
}

f3 find_gamut_boundary_intersection(
    const f3 &JMh_s, const f2 &JM_cusp_in, float J_focus, float J_max, float slope_gain, float gamma_top, float gamma_bottom)
{
    const f2 JM_cusp = {
        JM_cusp_in[0],
        JM_cusp_in[1] * (1.f + 0.27f * 0.12f)
    };

    const float J_intersect_source = solve_J_intersect(JMh_s[0], JMh_s[1], J_focus, J_max, slope_gain);
    const float J_intersect_cusp = solve_J_intersect(JM_cusp[0], JM_cusp[1], J_focus, J_max, slope_gain);

    float slope = 0.f;
    if (J_intersect_source < J_focus)
    {
        slope = J_intersect_source * (J_intersect_source - J_focus) / (J_focus * slope_gain);
    }
    else
    {
        slope = (J_max - J_intersect_source) * (J_intersect_source - J_focus) / (J_focus * slope_gain);
    }

    const float M_boundary_lower = J_intersect_cusp * pow(J_intersect_source / J_intersect_cusp, 1. / gamma_bottom) / (JM_cusp[0] / JM_cusp[1] - slope);
    const float M_boundary_upper = JM_cusp[1] * (J_max - J_intersect_cusp) * pow((J_max - J_intersect_source) / (J_max - J_intersect_cusp), 1. / gamma_top) / (slope * JM_cusp[1] + J_max - JM_cusp[0]);
    const float M_boundary = JM_cusp[1] * smin(M_boundary_lower / JM_cusp[1], M_boundary_upper / JM_cusp[1], 0.12f);
    const float J_boundary = J_intersect_source + slope * M_boundary;

    return {J_boundary, M_boundary, J_intersect_source};
}

f3 get_reach_boundary(
    float J,
    float M,
    float h,
    const f2 &JMcusp,
    float limit_J_max,
    float mid_J,
    float model_gamma,
    float focus_dist,
    const ACES2::Table1D & reach_m_table
)
{
    const float reachMaxM = reach_m_from_table(h, reach_m_table);

    const float focusJ = lerpf(JMcusp[0], mid_J, std::min(1.f, 1.3f - (JMcusp[0] / limit_J_max)));
    const float slope_gain = limit_J_max * focus_dist * get_focus_gain(J, JMcusp[0], limit_J_max);

    const float intersectJ = solve_J_intersect(J, M, focusJ, limit_J_max, slope_gain);

    float slope;
    if (intersectJ < focusJ)
    {
        slope = intersectJ * (intersectJ - focusJ) / (focusJ * slope_gain);
    }
    else
    {
        slope = (limit_J_max - intersectJ) * (intersectJ - focusJ) / (focusJ * slope_gain);
    }

    const float boundary = limit_J_max * pow(intersectJ / limit_J_max, model_gamma) * reachMaxM / (limit_J_max - slope * reachMaxM);
    return {J, boundary, h};
}

float compression_function(
    float v,
    float thr,
    float lim,
    bool invert)
{
    float s = (lim - thr) * (1.0 - thr) / (lim - 1.0);
    float nd = (v - thr) / s;

    float vCompressed;

    if (invert) {
        if (v < thr || lim < 1.0001 || v > thr + s) {
            vCompressed = v;
        } else {
            vCompressed = thr + s * (-nd / (nd - 1));
        }
    } else {
        if (v < thr || lim < 1.0001) {
            vCompressed = v;
        } else {
            vCompressed = thr + s * nd / (1.0 + nd);
        }
    }

    return vCompressed;
}

f3 compressGamut(const f3 &JMh, float Jx, const ACES2::GamutCompressParams& p, bool invert)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];

    if (M < 0.0001f || J > p.limit_J_max)
    {
        return {J, 0.f, h};
    }
    else
    {
        const f2 project_from = {J, M};
        const f2 JMcusp = cusp_from_table(h, p.gamut_cusp_table);
        const float focusJ = lerpf(JMcusp[0], p.mid_J, std::min(1.f, 1.3f - (JMcusp[0] / p.limit_J_max)));
        const float slope_gain = p.limit_J_max * p.focus_dist * get_focus_gain(Jx, JMcusp[0], p.limit_J_max);

        const float gamma_top = hue_dependent_upper_hull_gamma(h, p.upper_hull_gamma_table);
        const float gamma_bottom = 1.14f;

        const f3 boundaryReturn = find_gamut_boundary_intersection({J, M, h}, JMcusp, focusJ, p.limit_J_max, slope_gain, gamma_top, gamma_bottom);
        const f2 JMboundary = {boundaryReturn[0], boundaryReturn[1]};
        const f2 project_to = {boundaryReturn[2], 0.f};

        const f3 reachBoundary = get_reach_boundary(JMboundary[0], JMboundary[1], h, JMcusp, p.limit_J_max, p.mid_J, p.model_gamma, p.focus_dist, p.reach_m_table);

        const float difference = std::max(1.0001f, reachBoundary[1] / JMboundary[1]);
        const float threshold = std::max(0.75f, 1.f / difference);

        float v = project_from[1] / JMboundary[1];
        v = compression_function(v, threshold, difference, invert);

        const f2 JMcompressed {
            project_to[0] + v * (JMboundary[0] - project_to[0]),
            project_to[1] + v * (JMboundary[1] - project_to[1])
        };

        return {JMcompressed[0], JMcompressed[1], h};
    }
}

void Renderer_ACES_GAMUT_COMPRESS_20::fwd(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float J = in[0];
        const float M = in[1];
        const float h = in[2];

        const f3 compressedJMh = compressGamut({J, M, h}, J, m_g, false);

        out[0] = compressedJMh[0];
        out[1] = compressedJMh[1];
        out[2] = compressedJMh[2];
        out[3] = in[3];

        in  += 4;
        out += 4;
    }
}

void Renderer_ACES_GAMUT_COMPRESS_20::inv(const void * inImg, void * outImg, long numPixels) const
{
    const float * in = (const float *)inImg;
    float * out = (float *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float J = in[0];
        const float M = in[1];
        const float h = in[2];

        const f2 JMcusp = cusp_from_table(h, m_g.gamut_cusp_table);
        float Jx = J;

        f3 unCompressedJMh;

        // Analytic inverse below threshold
        if (Jx <= lerpf(JMcusp[0], m_g.limit_J_max, 0.3f))
        {
            unCompressedJMh = compressGamut({J, M, h}, Jx, m_g, true);
        }
        // Approximation above threshold
        else
        {
            Jx = compressGamut({J, M, h}, Jx, m_g, true)[0];
            unCompressedJMh = compressGamut({J, M, h}, Jx, m_g, true);
        }

        out[0] = unCompressedJMh[0];
        out[1] = unCompressedJMh[1];
        out[2] = unCompressedJMh[2];
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
        case FixedFunctionOpData::ACES_GAMUT_COMP_13_FWD:
        {
            return std::make_shared<Renderer_ACES_GamutComp13_Fwd>(func);
        }
        case FixedFunctionOpData::ACES_GAMUT_COMP_13_INV:
        {
            return std::make_shared<Renderer_ACES_GamutComp13_Inv>(func);
        }

        case FixedFunctionOpData::ACES_RGB_TO_JMh_20:
        case FixedFunctionOpData::ACES_JMh_TO_RGB_20:
        {
            // Sharing same renderer (param will be inverted to handle direction).
            return std::make_shared<Renderer_ACES_RGB_TO_JMh_20>(func);
        }

        case FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_FWD:
        case FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_INV:
        {
            // Sharing same renderer (param will be inverted to handle direction).
            return std::make_shared<Renderer_ACES_TONESCALE_COMPRESS_20>(func);
        }

        case FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_FWD:
        case FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_INV:
        {
            // Sharing same renderer (param will be inverted to handle direction).
            return std::make_shared<Renderer_ACES_GAMUT_COMPRESS_20>(func);
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
