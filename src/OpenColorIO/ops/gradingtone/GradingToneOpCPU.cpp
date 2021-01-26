// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/gradingtone/GradingToneOpCPU.h"
#include "SSE.h"

namespace OCIO_NAMESPACE
{

namespace
{
class GradingToneOpCPU : public OpCPU
{
public:
    GradingToneOpCPU() = delete;
    GradingToneOpCPU(const GradingToneOpCPU &) = delete;

    explicit GradingToneOpCPU(ConstGradingToneOpDataRcPtr & gt);

    bool hasDynamicProperty(DynamicPropertyType type) const override;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const override;

protected:
    DynamicPropertyGradingToneImplRcPtr m_gt;
    GradingStyle m_style;
};

GradingToneOpCPU::GradingToneOpCPU(ConstGradingToneOpDataRcPtr & gt)
    : OpCPU()
{
    m_gt = gt->getDynamicPropertyInternal();
    m_style = gt->getStyle();
    if (m_gt->isDynamic())
    {
        m_gt = m_gt->createEditableCopy();
    }
}

bool GradingToneOpCPU::hasDynamicProperty(DynamicPropertyType type) const
{
    bool res = false;
    if (type == DYNAMIC_PROPERTY_GRADING_TONE)
    {
        res = m_gt->isDynamic();
    }
    return res;
}

DynamicPropertyRcPtr GradingToneOpCPU::getDynamicProperty(DynamicPropertyType type) const
{
    if (type == DYNAMIC_PROPERTY_GRADING_TONE)
    {
        if (m_gt->isDynamic())
        {
            return m_gt;
        }
        else
        {
            throw Exception("GradingTone property is not dynamic.");
        }
    }
    throw Exception("Dynamic property type not supported by GradingTone.");
}

class GradingToneFwdOpCPU : public GradingToneOpCPU
{
public:
    explicit GradingToneFwdOpCPU(ConstGradingToneOpDataRcPtr & gt);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:

    void mids(const GradingTone & v, const GradingTonePreRender & vpr, RGBMChannel channel, float * out) const;
    void highlightShadow(const GradingTone & v, const GradingTonePreRender & vpr, RGBMChannel channel, bool isShadow, float * out) const;
    void whiteBlack(const GradingTone & v, const GradingTonePreRender & vpr, RGBMChannel channel, bool isBlack, float * out) const;
    void scontrast(const GradingTone & v, const GradingTonePreRender & vpr, float * out) const;
};

class GradingToneLinearFwdOpCPU : public GradingToneFwdOpCPU
{
public:
    explicit GradingToneLinearFwdOpCPU(ConstGradingToneOpDataRcPtr & gt);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

class GradingToneRevOpCPU : public GradingToneOpCPU
{
public:
    explicit GradingToneRevOpCPU(ConstGradingToneOpDataRcPtr & gt);

    void apply(const void * inImg, void * outImg, long numPixels) const override;

protected:

    void mids(const GradingTone & v, const GradingTonePreRender & vpr, RGBMChannel channel, float * out) const;
    void highlightShadow(const GradingTone & v, const GradingTonePreRender & vpr, RGBMChannel channel, bool isShadow, float * out) const;
    void whiteBlack(const GradingTone & v, const GradingTonePreRender & vpr, RGBMChannel channel, bool isBlack, float * out) const;
    void scontrast(const GradingTone & v, const GradingTonePreRender & vpr, float * out) const;
};

class GradingToneLinearRevOpCPU : public GradingToneRevOpCPU
{
public:
    explicit GradingToneLinearRevOpCPU(ConstGradingToneOpDataRcPtr & gt);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

///////////////////////////////////////////////////////////////////////////////

GradingToneFwdOpCPU::GradingToneFwdOpCPU(ConstGradingToneOpDataRcPtr & gt)
    : GradingToneOpCPU(gt)
{
}

GradingToneLinearFwdOpCPU::GradingToneLinearFwdOpCPU(ConstGradingToneOpDataRcPtr & gt)
    : GradingToneFwdOpCPU(gt)
{
}

GradingToneRevOpCPU::GradingToneRevOpCPU(ConstGradingToneOpDataRcPtr & gt)
    : GradingToneOpCPU(gt)
{
}

GradingToneLinearRevOpCPU::GradingToneLinearRevOpCPU(ConstGradingToneOpDataRcPtr & gt)
    : GradingToneRevOpCPU(gt)
{
}

///////////////////////////////////////////////////////////////////////////////

struct float3
{
    float3()
    {
        m_val[0] = 0.f;
        m_val[1] = 0.f;
        m_val[2] = 0.f;
    }

    float3(float r, float g, float b)
    {
        m_val[0] = r;
        m_val[1] = g;
        m_val[2] = b;
    }

    float3(const float * v)
    {
        m_val[0] = v[0];
        m_val[1] = v[1];
        m_val[2] = v[2];
    }

    void setOnLimit(const float3 & val, float limit, const float3 & below, const float3 & above)
    {
        m_val[0] = val[0] < limit ? below[0] : above[0];
        m_val[1] = val[1] < limit ? below[1] : above[1];
        m_val[2] = val[2] < limit ? below[2] : above[2];
    }

    float operator[](int c) const { return m_val[c]; }
    float m_val[3];
};

float3 operator+(const float3 & f3, float p)
{
    return float3{ f3[0] + p, f3[1] + p, f3[2] + p };
}

float3 operator+(float p, const float3 & f3)
{
    return f3 + p;
}

float3 operator+(const float3 & f3a, const float3 & f3b)
{
    return float3{ f3a[0] + f3b[0], f3a[1] + f3b[1], f3a[2] + f3b[2] };
}

float3 operator-(const float3 & f3, float m)
{
    return float3{ f3[0] - m, f3[1] - m, f3[2] - m };
}

float3 operator-(float m, const float3 & f3)
{
    return float3{ m - f3[0], m - f3[1], m - f3[2] };
}

float3 operator*(const float3 & f3, float m)
{
    return float3{ f3[0] * m, f3[1] * m, f3[2] * m };
}

float3 operator*(float m, const float3 & f3)
{
    return f3 * m;
}

float3 operator*(const float3 & f3a, const float3 & f3b)
{
    return float3{ f3a[0] * f3b[0],
                   f3a[1] * f3b[1],
                   f3a[2] * f3b[2] };
}

float3 operator/(const float3 & f3, float d)
{
    return float3{ f3[0] / d, f3[1] / d, f3[2] / d };
}

float3 operator/(const float3 & f3a, const float3 & f3b)
{
    return float3{ f3a[0] / f3b[0], f3a[1] / f3b[1], f3a[2] / f3b[2] };
}


template<typename type>
void setOnLimit(type & res, const type & val, float limit, const type & below, const type & above)
{
    res.setOnLimit(val, limit, below, above);
}

template<>
void setOnLimit<float>(float & res, const float & val, float limit, const float & below, const float & above)
{
    res = val < limit ? below : above;
}

template<typename type>
type Sqrt(const type & val)
{
    return type{ sqrtf(val[0]), sqrtf(val[1]), sqrtf(val[2]) };
}

template<>
float Sqrt<float>(const float & val)
{
    return sqrtf(val);
}

template<typename type>
void Set(RGBMChannel channel, float * out, const type & val)
{
    out[0] = val[0];
    out[1] = val[1];
    out[2] = val[2];
}

template<>
void Set<float>(RGBMChannel channel, float * out, const float & val)
{
    out[channel] = val;
}

///////////////////////////////////////////////////////////////////////////////

void GradingToneFwdOpCPU::mids(const GradingTone & v, const GradingTonePreRender & vpr,
                               RGBMChannel channel, float * out) const
{
    float mid_adj = Clamp(GetChannelValue(v.m_midtones, channel), 0.01f, 1.99f);

    if (mid_adj != 1.f)
    {
        const auto & x0 = vpr.m_midX[channel][0];
        const auto & x1 = vpr.m_midX[channel][1];
        const auto & x2 = vpr.m_midX[channel][2];
        const auto & x3 = vpr.m_midX[channel][3];
        const auto & x4 = vpr.m_midX[channel][4];
        const auto & x5 = vpr.m_midX[channel][5];
        const auto & y0 = vpr.m_midY[channel][0];
        const auto & y1 = vpr.m_midY[channel][1];
        const auto & y2 = vpr.m_midY[channel][2];
        const auto & y3 = vpr.m_midY[channel][3];
        const auto & y4 = vpr.m_midY[channel][4];
        const auto & y5 = vpr.m_midY[channel][5];
        const auto & m0 = vpr.m_midM[channel][0];
        const auto & m1 = vpr.m_midM[channel][1];
        const auto & m2 = vpr.m_midM[channel][2];
        const auto & m3 = vpr.m_midM[channel][3];
        const auto & m4 = vpr.m_midM[channel][4];
        const auto & m5 = vpr.m_midM[channel][5];

        if (channel != M)
        {
            float t = out[channel];
            float tL = (t - x0) / (x1 - x0);
            float tM = (t - x1) / (x2 - x1);
            float tR = (t - x2) / (x3 - x2);
            float tR2 = (t - x3) / (x4 - x3);
            float tR3 = (t - x4) / (x5 - x4);

            float fL = tL * (x1 - x0) * ( tL * 0.5f * (m1 - m0) + m0 ) + y0;
            float fM = tM * (x2 - x1) * ( tM * 0.5f * (m2 - m1) + m1 ) + y1;
            float fR = tR * (x3 - x2) * ( tR * 0.5f * (m3 - m2) + m2 ) + y2;
            float fR2 = tR2 * (x4 - x3) * ( tR2 * 0.5f * (m4 - m3) + m3 ) + y3;
            float fR3 = tR3 * (x5 - x4) * ( tR3 * 0.5f * (m5 - m4) + m4 ) + y4;

            float res = (t < x1) ? fL : fM;
            if (t > x2) res = fR;
            if (t > x3) res = fR2;
            if (t > x4) res = fR3;
            if (t < x0) res = y0 + (t - x0) * m0;
            if (t > x5) res = y5 + (t - x5) * m5;
            out[channel] = res;
        }
        else
        {
            float3 t{ out };
            float3 tL  = (t - x0) / (x1 - x0);
            float3 tM  = (t - x1) / (x2 - x1);
            float3 tR  = (t - x2) / (x3 - x2);
            float3 tR2 = (t - x3) / (x4 - x3);
            float3 tR3 = (t - x4) / (x5 - x4);

            float3 fL  = tL * (x1 - x0) * ( tL * 0.5f * (m1 - m0) + m0 ) + y0;
            float3 fM  = tM * (x2 - x1) * ( tM * 0.5f * (m2 - m1) + m1 ) + y1;
            float3 fR  = tR * (x3 - x2) * ( tR * 0.5f * (m3 - m2) + m2 ) + y2;
            float3 fR2 = tR2 * (x4 - x3) * ( tR2 * 0.5f * (m4 - m3) + m3 ) + y3;
            float3 fR3 = tR3 * (x5 - x4) * ( tR3 * 0.5f * (m5 - m4) + m4 ) + y4;

            float3 fR4 = (t - x0) * m0 + y0;
            float3 fR5 = (t - x5) * m5 + y5;

            float3 res;
            res.setOnLimit(t, x1, fL, fM);
            res.setOnLimit(t, x2, res, fR);
            res.setOnLimit(t, x3, res, fR2);
            res.setOnLimit(t, x4, res, fR3);
            res.setOnLimit(t, x0, fR4, res);
            res.setOnLimit(t, x5, res, fR5);

            out[0] = res[0];
            out[1] = res[1];
            out[2] = res[2];
        }
    }
}

void GradingToneRevOpCPU::mids(const GradingTone & v, const GradingTonePreRender & vpr,
                               RGBMChannel channel, float * out) const
{
    float mid_adj = Clamp(GetChannelValue(v.m_midtones, channel), 0.01f, 1.99f);

    if (mid_adj != 1.f)
    {
        const auto & x0 = vpr.m_midX[channel][0];
        const auto & x1 = vpr.m_midX[channel][1];
        const auto & x2 = vpr.m_midX[channel][2];
        const auto & x3 = vpr.m_midX[channel][3];
        const auto & x4 = vpr.m_midX[channel][4];
        const auto & x5 = vpr.m_midX[channel][5];
        const auto & y0 = vpr.m_midY[channel][0];
        const auto & y1 = vpr.m_midY[channel][1];
        const auto & y2 = vpr.m_midY[channel][2];
        const auto & y3 = vpr.m_midY[channel][3];
        const auto & y4 = vpr.m_midY[channel][4];
        const auto & y5 = vpr.m_midY[channel][5];
        const auto & m0 = vpr.m_midM[channel][0];
        const auto & m1 = vpr.m_midM[channel][1];
        const auto & m2 = vpr.m_midM[channel][2];
        const auto & m3 = vpr.m_midM[channel][3];
        const auto & m4 = vpr.m_midM[channel][4];
        const auto & m5 = vpr.m_midM[channel][5];

        if (channel != M)
        {
            float t = out[channel];
            float res = 0.f;
            if (t >= y5)
            {
                res = x0 + (t - y0) / m0;
            }
            else if (t >= y4)
            {
                const float c = y4 - t;
                const float b = m4 * (x5 - x4);
                const float a = 0.5f * (m5 - m4) * (x5 - x4);
                const float discrim = sqrt( b * b - 4.f * a * c);
                const float tmp = (2.f * c) / (-discrim - b);
                res = tmp * (x5 - x4) + x4;
            }
            else if (t >= y3)
            {
                const float c = y3 - t;
                const float b = m3 * (x4 - x3);
                const float a = 0.5f * (m4 - m3) * (x4 - x3);
                const float discrim = sqrt( b * b - 4.f * a * c);
                const float tmp = (2.f * c) / (-discrim - b);
                res = tmp * (x4 - x3) + x3;
            }
            else if (t >= y2)
            {
                const float c = y2 - t;
                const float b = m2 * (x3 - x2);
                const float a = 0.5f * (m3 - m2) * (x3 - x2);
                const float discrim = sqrt( b * b - 4.f * a * c);
                const float tmp = (2.f * c) / (-discrim - b);
                res = tmp * (x3 - x2) + x2;
            }
            else if (t >= y1)
            {
                const float c = y1 - t;
                const float b = m1 * (x2 - x1);
                const float a = 0.5f * (m2 - m1) * (x2 - x1);
                const float discrim = sqrt( b * b - 4.f * a * c);
                const float tmp = (2.f * c) / (-discrim - b);
                res = tmp * (x2 - x1) + x1;
            }
            else if (t >= y0)
            {
                const float c = y0 - t;
                const float b = m0 * (x1 - x0);
                const float a = 0.5f * (m1 - m0) * (x1 - x0);
                const float discrim = sqrt( b * b - 4.f * a * c);
                const float tmp = (2.f * c) / (-discrim - b);
                res = tmp * (x1 - x0) + x0;
            }
            else
            {
                res = x0 + (t - y0) / m0;
            }
            out[channel] = res;
        }
        else
        {
            float3 t{ out };
            float3 outL0, outL, outM, outR, outR2, outR3, outR4;

            outR4 = x5 + (t - y5) / m5;
            {
                float3 c = y4 - t;
                const float b = m4 * (x5 - x4);
                const float a = 0.5f * (m5 - m4) * (x5 - x4);
                float3 discrim = Sqrt( b * b - 4.f * a * c);
                float3 tmp = (2.f * c) / (-b - discrim);
                outR3 = tmp * (x5 - x4) + x4;
            }
            {
                float3 c = y3 - t;
                const float b = m3 * (x4 - x3);
                const float a = 0.5f * (m4 - m3) * (x4 - x3);
                float3 discrim = Sqrt( b * b - 4.f * a * c);
                float3 tmp = (2.f * c) / (-b - discrim);
                outR2 = tmp * (x4 - x3) + x3;
            }
            {
                float3 c = y2 - t;
                const float b = m2 * (x3 - x2);
                const float a = 0.5f * (m3 - m2) * (x3 - x2);
                float3 discrim = Sqrt( b * b - 4.f * a * c);
                float3 tmp = (2.f * c) / (-b - discrim);
                outR = tmp * (x3 - x2) + x2;
            }
            {
                float3 c = y1 - t;
                const float b = m1 * (x2 - x1);
                const float a = 0.5f * (m2 - m1) * (x2 - x1);
                float3 discrim = Sqrt( b * b - 4.f * a * c);
                float3 tmp = (2.f * c) / (-b - discrim);
                outM = tmp * (x2 - x1) + x1;
            }
            {
                float3 c = y0 - t;
                const float b = m0 * (x1 - x0);
                const float a = 0.5f * (m1 - m0) * (x1 - x0);
                float3 discrim = Sqrt( b * b - 4.f * a * c);
                float3 tmp = (2.f * c) / (-b - discrim);
                outL = tmp * (x1 - x0) + x0;
            }
            outL0 = x0 + (t - y0) / m0;

            float3 res;
            res.setOnLimit(t, y1, outL, outM);
            res.setOnLimit(t, y2, res, outR);
            res.setOnLimit(t, y3, res, outR2);
            res.setOnLimit(t, y4, res, outR3);
            res.setOnLimit(t, y0, outL0, res);
            res.setOnLimit(t, y5, res, outR4);

            out[0] = res[0];
            out[1] = res[1];
            out[2] = res[2];
        }
    }
}

template<typename type>
void ComputeHSFwd(RGBMChannel channel, float * out, float val, float x0, float x1, float x2,
                  float y0, float y1, float y2, float m0, float m2, type & t)
{
    type res{ t }, tL, tR, fL, fR;

    tL = (t - x0) / (x1 - x0);
    tR = (t - x1) / (x2 - x1);
    fL = y0 * (1.f - tL*tL) + y1 * tL*tL + m0 * (1.f - tL) * tL * (x1 - x0);
    fR = y1 * (1.f - tR)*(1.f - tR) + y2 * (2.f - tR)*tR + m2 * (tR - 1.f)*tR * (x2 - x1);

    setOnLimit(res, t, x1, fL, fR);
    type r0 = (t - x0) * m0 + y0;
    setOnLimit(res, t, x0, r0, res);
    type r2 = (t - x2) * m2 + y2;
    setOnLimit(res, t, x2, res, r2);

    Set(channel, out, res);
}

template<typename type>
void ComputeHSRev(RGBMChannel channel, float * out, float val, float x0, float x1, float x2,
                  float y0, float y1, float y2, float m0, float m2, type & t)
{
    type res{ t }, cL, cR, discrimL, discrimR, outL, outR;

    float bL = m0 * (x1 - x0);
    float aL = y1 - y0 - m0 * (x1 - x0);
    cL = y0 - t;
    discrimL = Sqrt(bL * bL - 4.f * aL * cL);
    outL = (-2.f * cL) / (discrimL + bL) * (x1 - x0) + x0;
    float bR = 2.f*y2 - 2.f*y1 - m2 * (x2 - x1);
    float aR = y1 - y2 + m2 * (x2 - x1);
    cR = y1 - t;
    discrimR = Sqrt(bR * bR - 4.f * aR * cR);
    outR = (-2.f * cR) / (discrimR + bR) * (x2 - x1) + x1;

    setOnLimit(res, t, y1, outL, outR);
    type r0 = (t - y0) / m0 + x0;
    setOnLimit(res, t, y0, r0, res);
    type r2 = (t - y2) / m2 + x2;
    setOnLimit(res, t, y2, res, r2);

    Set(channel, out, res);
}

void GradingToneFwdOpCPU::highlightShadow(const GradingTone & v, const GradingTonePreRender & vpr,
                                          RGBMChannel channel,
                                          bool isShadow, float * out) const
{
    // The effect of val is symmetric around 1 (<1 uses Fwd algorithm, >1 uses Rev algorithm).
    float val = isShadow ? GetChannelValue(v.m_shadows, channel) :
                           GetChannelValue(v.m_highlights, channel);
    if (!isShadow)
    {
        val = 2.f - val;
    }
    if (val == 1.) return;

    const auto & x0 = vpr.m_hsX[isShadow ? 1 : 0][channel][0];
    const auto & x1 = vpr.m_hsX[isShadow ? 1 : 0][channel][1];
    const auto & x2 = vpr.m_hsX[isShadow ? 1 : 0][channel][2];
    const auto & y0 = vpr.m_hsY[isShadow ? 1 : 0][channel][0];
    const auto & y1 = vpr.m_hsY[isShadow ? 1 : 0][channel][1];
    const auto & y2 = vpr.m_hsY[isShadow ? 1 : 0][channel][2];
    const auto & m0 = vpr.m_hsM[isShadow ? 1 : 0][channel][0];
    const auto & m2 = vpr.m_hsM[isShadow ? 1 : 0][channel][1];

    if (val < 1.)
    {
        if (channel != M)
        {
            float t = out[channel];
            ComputeHSFwd(channel, out, val, x0, x1, x2, y0, y1, y2, m0, m2, t);     // Fwd
        }
        else
        {
            float3 t{ out };
            ComputeHSFwd(channel, out, val, x0, x1, x2, y0, y1, y2, m0, m2, t);
        }
    }
    else
    {
        if (channel != M)
        {
            float t = out[channel];
            ComputeHSRev(channel, out, val, x0, x1, x2, y0, y1, y2, m0, m2, t);     // Rev
        }
        else
        {
            float3 t{ out };
            ComputeHSRev(channel, out, val, x0, x1, x2, y0, y1, y2, m0, m2, t);
        }
    }
}

void GradingToneRevOpCPU::highlightShadow(const GradingTone & v, const GradingTonePreRender & vpr,
                                          RGBMChannel channel,
                                          bool isShadow, float * out) const
{
    float val = isShadow ? GetChannelValue(v.m_shadows, channel) :
                           GetChannelValue(v.m_highlights, channel);
    if (!isShadow)
    {
        val = 2.f - val;
    }
    if (val == 1.) return;

    const auto & x0 = vpr.m_hsX[isShadow ? 1 : 0][channel][0];
    const auto & x1 = vpr.m_hsX[isShadow ? 1 : 0][channel][1];
    const auto & x2 = vpr.m_hsX[isShadow ? 1 : 0][channel][2];
    const auto & y0 = vpr.m_hsY[isShadow ? 1 : 0][channel][0];
    const auto & y1 = vpr.m_hsY[isShadow ? 1 : 0][channel][1];
    const auto & y2 = vpr.m_hsY[isShadow ? 1 : 0][channel][2];
    const auto & m0 = vpr.m_hsM[isShadow ? 1 : 0][channel][0];
    const auto & m2 = vpr.m_hsM[isShadow ? 1 : 0][channel][1];

    if (val < 1.)
    {
        if (channel != M)
        {
            float t = out[channel];
            ComputeHSRev(channel, out, val, x0, x1, x2, y0, y1, y2, m0, m2, t);     // Rev
        }
        else
        {
            float3 t{ out };
            ComputeHSRev(channel, out, val, x0, x1, x2, y0, y1, y2, m0, m2, t);
        }
    }
    else
    {
        if (channel != M)
        {
            float t = out[channel];
            ComputeHSFwd(channel, out, val, x0, x1, x2, y0, y1, y2, m0, m2, t);     // Fwd
        }
        else
        {
            float3 t{ out };
            ComputeHSFwd(channel, out, val, x0, x1, x2, y0, y1, y2, m0, m2, t);
        }
    }
}

template<typename type>
void ComputeWBFwd(RGBMChannel channel, bool isBlack, float * out, float val, float x0, float x1,
                  float y0, float y1, float m0, float m1, float gain, type & t)
{
    const float mtest = (!isBlack) ? val : 2.f - val;

    if (mtest < 1.f)
    {
        // Slope is decreasing case.

        type tlocal = (t - x0) / (x1 - x0);
        type res = tlocal * (x1 - x0) * (tlocal * 0.5f * (m1 - m0) + m0) + y0;
        type res0 = y0 + (t - x0) * m0;
        setOnLimit(res, t, x0, res0, res);
        type res1 = y1 + (t - x1) * m1;
        setOnLimit(res, t, x1, res, res1);

        Set(channel, out, res);
    }
    else if (mtest > 1.f)
    {
        // Slope is increasing case.

        t = (!isBlack) ? (t - x0) * gain + x0 : (t - x1) * gain + x1;

        const float a = 0.5f * (m1 - m0) * (x1 - x0);
        const float b = m0 * (x1 - x0);

        type c = y0 - t;
        type discrim = Sqrt(b * b - 4.f * a * c);
        type tmp = (-2.f * c) / (discrim + b);
        type res = tmp * (x1 - x0) + x0;
        type res0 = x0 + (t - y0) / m0;
        setOnLimit(res, t, y0, res0, res);

        if (!isBlack)
        {
            res = (res - x0) / gain + x0;
            // Quadratic extrapolation for better HDR control.
            // TODO: These values are not per pixel and could be pre-calculated.
            const float new_y1 = (x1 - x0) / gain + x0;
            const float xd = x0 + (x1 - x0) * 0.99f;
            float md = m0 + (xd - x0) * (m1 - m0) / (x1 - x0);
            md = 1.f / md;
            const float aa = 0.5f * (1.f / m1 - md) / (x1 - xd);
            const float bb = 1.f / m1 - 2.f * aa * x1;
            const float cc = new_y1 - bb * x1 - aa * x1 * x1;
            t = (t - x0) / gain + x0;

            type res1 = (aa * t + bb) * t + cc;
            setOnLimit(res, t, x1, res, res1);
        }
        else
        {
            type res1 = x1 + (t - y1) / m1;
            setOnLimit(res, t, y1, res, res1);
            res = (res - x1) / gain + x1;
        }

        Set(channel, out, res);
    }
}

template<typename type>
void ComputeWBRev(RGBMChannel channel, bool isBlack, float * out, float val, float x0, float x1,
                  float y0, float y1, float m0, float m1, float gain, type & t)
{
    const float mtest = (!isBlack) ? val : 2.f - val;

    // TODO: Refactor to reduce duplication of Fwd code.
    if (mtest < 1.f)
    {
        // Slope is decreasing case.

        const float a = 0.5f * (m1 - m0) * (x1 - x0);
        const float b = m0 * (x1 - x0);

        type c = y0 - t;
        type discrim = Sqrt(b * b - 4.f * a * c);
        type tmp = (-2.f * c) / (discrim + b);
        type res = tmp * (x1 - x0) + x0;
        type res0 = x0 + (t - y0) / m0;
        setOnLimit(res, t, y0, res0, res);

        type res1 = x1 + (t - y1) / m1;
        setOnLimit(res, t, y1, res, res1);

        Set(channel, out, res);
    }
    else if (mtest > 1.f)
    {
        // Slope is increasing case.

        t = (!isBlack) ? (t - x0) * gain + x0 : (t - x1) * gain + x1;

        type tlocal = (t - x0) / (x1 - x0);
        type res = tlocal * (x1 - x0) * (tlocal * 0.5f * (m1 - m0) + m0) + y0;
        type res0 = y0 + (t - x0) * m0;
        setOnLimit(res, t, x0, res0, res);

        if (!isBlack)
        {
            res = (res - x0) / gain + x0;
            // Quadratic extrapolation for better HDR control.
            // TODO: These values are not per pixel and could be pre-calculated.
            const float new_y1 = (x1 - x0) / gain + x0;
            const float xd = x0 + (x1 - x0) * 0.99f;
            float md = m0 + (xd - x0) * (m1 - m0) / (x1 - x0);
            md = 1.f / md;
            const float aa = 0.5f * (1.f / m1 - md) / (x1 - xd);
            const float bb = 1.f / m1 - 2.f * aa * x1;
            const float cc = new_y1 - bb * x1 - aa * x1 * x1;
            t = (t - x0) / gain + x0;

            type c = cc - t;
            type discrim = Sqrt(bb * bb - 4.f * aa * c);
            type res1 = (-2.f * c) / (discrim + bb);
            const float brk = (aa * x1 + bb) * x1 + cc;
            setOnLimit(res, t, brk, res, res1);
        }
        else
        {
            type res1 = y1 + (t - x1) * m1;
            setOnLimit(res, t, x1, res, res1);
            res = (res - x1) / gain + x1;
        }

        Set(channel, out, res);
    }
}

void GradingToneFwdOpCPU::whiteBlack(const GradingTone & v, const GradingTonePreRender & vpr,
                                     RGBMChannel channel, bool isBlack, float * out) const
{
    float val = isBlack ? GetChannelValue(v.m_blacks, channel) :
                          GetChannelValue(v.m_whites, channel);

    const auto & x0 = vpr.m_wbX[isBlack ? 1 : 0][channel][0];
    const auto & x1 = vpr.m_wbX[isBlack ? 1 : 0][channel][1];
    const auto & y0 = vpr.m_wbY[isBlack ? 1 : 0][channel][0];
    const auto & y1 = vpr.m_wbY[isBlack ? 1 : 0][channel][1];
    const auto & m0 = vpr.m_wbM[isBlack ? 1 : 0][channel][0];
    const auto & m1 = vpr.m_wbM[isBlack ? 1 : 0][channel][1];
    const auto & gain = vpr.m_wbGain[isBlack ? 1 : 0][channel];
    
    if (channel != M)
    {
        float t = out[channel];
        ComputeWBFwd(channel, isBlack, out, val, x0, x1, y0, y1, m0, m1, gain, t);
    }
    else
    {
        float3 t{ out };
        ComputeWBFwd(channel, isBlack, out, val, x0, x1, y0, y1, m0, m1, gain, t);
    }
}

void GradingToneRevOpCPU::whiteBlack(const GradingTone & v, const GradingTonePreRender & vpr,
                                     RGBMChannel channel, bool isBlack, float * out) const
{
    float val = isBlack ? GetChannelValue(v.m_blacks, channel) :
                          GetChannelValue(v.m_whites, channel);

    const auto & x0 = vpr.m_wbX[isBlack ? 1 : 0][channel][0];
    const auto & x1 = vpr.m_wbX[isBlack ? 1 : 0][channel][1];
    const auto & y0 = vpr.m_wbY[isBlack ? 1 : 0][channel][0];
    const auto & y1 = vpr.m_wbY[isBlack ? 1 : 0][channel][1];
    const auto & m0 = vpr.m_wbM[isBlack ? 1 : 0][channel][0];
    const auto & m1 = vpr.m_wbM[isBlack ? 1 : 0][channel][1];
    const auto & gain = vpr.m_wbGain[isBlack ? 1 : 0][channel];
    
    if (channel != M)
    {
        float t = out[channel];
        ComputeWBRev(channel, isBlack, out, val, x0, x1, y0, y1, m0, m1, gain, t);
    }
    else
    {
        float3 t{ out };
        ComputeWBRev(channel, isBlack, out, val, x0, x1, y0, y1, m0, m1, gain, t);
    }
}

void GradingToneFwdOpCPU::scontrast(const GradingTone & v, const GradingTonePreRender & vpr, float * out) const
{
    float contrast = static_cast<float>(v.m_scontrast);
    if (contrast != 1.)
    {
        // Limit the range of values to prevent reversals.
        contrast = (contrast > 1.f) ? 1.f / (1.8125f - 0.8125f * std::min(contrast, 1.99f)) :
                                            0.28125f + 0.71875f * std::max(contrast, 0.01f);

        float3 t{ out };
        float3 outColor{ (t - vpr.m_pivot) * contrast + vpr.m_pivot };

        // Top end
        {
            const auto & x1 = vpr.m_scX[0][1];
            const auto & x2 = vpr.m_scX[0][2];
            const auto & y1 = vpr.m_scY[0][1];
            const auto & y2 = vpr.m_scY[0][2];
            const auto & m0 = vpr.m_scM[0][0];
            const auto & m3 = vpr.m_scM[0][1];

            float3 tR  = (t - x1) / (x2 - x1);
            float3 res = tR * (x2 - x1) * ( tR * 0.5f * (m3 - m0) + m0 ) + y1;

            setOnLimit(outColor, t, x1, outColor, res);

            float3 res2 = y2 + (t - x2) * m3;
            setOnLimit(outColor, t, x2, outColor, res2);
        }

        // Bottom end
        {
            const auto & x1 = vpr.m_scX[1][1];
            const auto & x2 = vpr.m_scX[1][2];
            const auto & y1 = vpr.m_scY[1][1];
            const auto & m0 = vpr.m_scM[1][0];
            const auto & m3 = vpr.m_scM[1][1];

            float3 tR = (t - x1) / (x2 - x1);
            float3 res = tR * (x2 - x1) * (tR * 0.5f * (m3 - m0) + m0) + y1;

            setOnLimit(outColor, t, x2, res, outColor);

            float3 res1 = y1 + (t - x1) * m0;
            setOnLimit(outColor, t, x1, res1, outColor);
        }

        out[0] = outColor[0];
        out[1] = outColor[1];
        out[2] = outColor[2];
    }  // end if contrast != 1.
}

void GradingToneRevOpCPU::scontrast(const GradingTone & v, const GradingTonePreRender & vpr, float * out) const
{
    float contrast = static_cast<float>(v.m_scontrast);
    if (contrast != 1.)
    {
        // Limit the range of values to prevent reversals.
        contrast = (contrast > 1.f) ? 1.f / (1.8125f - 0.8125f * std::min(contrast, 1.99f)) :
                                            0.28125f + 0.71875f * std::max(contrast, 0.01f);

        float3 t{ out };
        float3 outColor{ (t - vpr.m_pivot) / contrast + vpr.m_pivot };

        // Top end
        {
            const auto & x1 = vpr.m_scX[0][1];
            const auto & x2 = vpr.m_scX[0][2];
            const auto & y1 = vpr.m_scY[0][1];
            const auto & y2 = vpr.m_scY[0][2];
            const auto & m0 = vpr.m_scM[0][0];
            const auto & m3 = vpr.m_scM[0][1];

            float b = m0 * (x2 - x1);
            float a = (m3 - m0) * 0.5f * (x2 - x1);
            float3 c = y1 - t;
            float3 discrim = Sqrt(b * b - 4.f * a * c);
            float3 res =  (x2 - x1) * (-2.f * c) / (discrim + b) + x1;

            setOnLimit(outColor, t, y1, outColor, res);
            setOnLimit(outColor, t, y2, outColor, x2 + (t - y2) / m3);
        }

        // Bottom end
        {
            const auto & x1 = vpr.m_scX[1][1];
            const auto & x2 = vpr.m_scX[1][2];
            const auto & y1 = vpr.m_scY[1][1];
            const auto & y2 = vpr.m_scY[1][2];
            const auto & m0 = vpr.m_scM[1][0];
            const auto & m3 = vpr.m_scM[1][1];

            float b = m0 * (x2 - x1);
            float a = (m3 - m0) * 0.5f * (x2 - x1);
            float3 c = y1 - t;
            float3 discrim = Sqrt(b * b - 4.f * a * c);
            float3 res =  (x2 - x1) * (-2.f * c) / (discrim + b) + x1;

            setOnLimit(outColor, t, y2, res, outColor);
            setOnLimit(outColor, t, y1, x1 + (t - y1) / m0, outColor);
        }

        out[0] = outColor[0];
        out[1] = outColor[1];
        out[2] = outColor[2];
    }  // end if contrast != 1.
}

///////////////////////////////////////////////////////////////////////////////

inline void ClampMaxRGB(float * out)
{
    // TODO: The grading controls at high values are able to push values above the max half-float
    // at which point they overflow to infinity.  Currently the ACES view transforms make black for
    // Inf but even if it is probably not desirable to output Inf under any circumstances.
    out[0] = std::min(out[0], 65504.f);
    out[1] = std::min(out[1], 65504.f);
    out[2] = std::min(out[2], 65504.f);
}

static constexpr auto PixelSize = 4 * sizeof(float);

void GradingToneFwdOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_gt->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gt->getValue();
    auto & vpr = m_gt->getComputedValue();

    for (long idx = 0; idx < numPixels; ++idx)
    {
        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        memcpy(out, in, PixelSize);

        mids(v, vpr, R, out);
        mids(v, vpr, G, out);
        mids(v, vpr, B, out);
        mids(v, vpr, M, out);

        highlightShadow(v, vpr, R, false, out);
        highlightShadow(v, vpr, G, false, out);
        highlightShadow(v, vpr, B, false, out);
        highlightShadow(v, vpr, M, false, out);

        whiteBlack(v, vpr, R, false, out);
        whiteBlack(v, vpr, G, false, out);
        whiteBlack(v, vpr, B, false, out);
        whiteBlack(v, vpr, M, false, out);

        highlightShadow(v, vpr, R, true, out);
        highlightShadow(v, vpr, G, true, out);
        highlightShadow(v, vpr, B, true, out);
        highlightShadow(v, vpr, M, true, out);

        whiteBlack(v, vpr, R, true, out);
        whiteBlack(v, vpr, G, true, out);
        whiteBlack(v, vpr, B, true, out);
        whiteBlack(v, vpr, M, true, out);

        scontrast(v, vpr, out);

        ClampMaxRGB(out);

        in += 4;
        out += 4;
    }
}

void GradingToneRevOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{

    if (m_gt->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gt->getValue();
    auto & vpr = m_gt->getComputedValue();

    for (long idx = 0; idx < numPixels; ++idx)
    {
        // NB: 'in' and 'out' could be pointers to the same memory buffer.
        memcpy(out, in, PixelSize);

        scontrast(v, vpr, out);

        whiteBlack(v, vpr, M, true, out);
        whiteBlack(v, vpr, R, true, out);
        whiteBlack(v, vpr, G, true, out);
        whiteBlack(v, vpr, B, true, out);

        highlightShadow(v, vpr, M, true, out);
        highlightShadow(v, vpr, R, true, out);
        highlightShadow(v, vpr, G, true, out);
        highlightShadow(v, vpr, B, true, out);

        whiteBlack(v, vpr, M, false, out);
        whiteBlack(v, vpr, R, false, out);
        whiteBlack(v, vpr, G, false, out);
        whiteBlack(v, vpr, B, false, out);

        highlightShadow(v, vpr, M, false, out);
        highlightShadow(v, vpr, R, false, out);
        highlightShadow(v, vpr, G, false, out);
        highlightShadow(v, vpr, B, false, out);

        mids(v, vpr, M, out);
        mids(v, vpr, R, out);
        mids(v, vpr, G, out);
        mids(v, vpr, B, out);

        ClampMaxRGB(out);

        in += 4;
        out += 4;
    }
}

namespace LogLinConstants
{
    static constexpr float xbrk = 0.0041318374739483946f;
    static constexpr float shift = -0.000157849851665374f;
    static constexpr float m = 1.f / (0.18f + shift);
    static constexpr float gain = 363.034608563f;
    static constexpr float offs = -7.f;
    static constexpr float ybrk = -5.5f;
#ifdef USE_SSE
    const __m128 mxbrk = _mm_set1_ps(xbrk);
    const __m128 mshift = _mm_set1_ps(shift);
    const __m128 mm = _mm_set1_ps(m);
    const __m128 mgain = _mm_set1_ps(gain);
    const __m128 moffs = _mm_set1_ps(offs);
    const __m128 mybrk = _mm_set1_ps(ybrk);
    const __m128 mgainInv = _mm_set1_ps(1.f / gain);
    const __m128 mshift018 = _mm_set1_ps(shift + 0.18f);
    const __m128 mpower = _mm_set1_ps(2.0f);
#else
    static constexpr float base2 = 1.4426950408889634f; // 1/log(2)
#endif
}

inline void LinLog(const float * in, float * out)
{
#ifdef USE_SSE
        __m128 pix = _mm_loadu_ps(in);
        __m128 flag = _mm_cmpgt_ps(pix, LogLinConstants::mxbrk);

        __m128 pixLin = _mm_mul_ps(pix, LogLinConstants::mgain);
        pixLin = _mm_add_ps(pixLin, LogLinConstants::moffs);

        pix = _mm_add_ps(pix, LogLinConstants::mshift);
        pix = _mm_mul_ps(pix, LogLinConstants::mm);
        pix = sseLog2(pix);

        pix = _mm_or_ps(_mm_and_ps(flag, pix),
            _mm_andnot_ps(flag, pixLin));

        _mm_storeu_ps(out, pix);
#else
        out[0] = (in[0] < LogLinConstants::xbrk) ?
                 in[0] * LogLinConstants::gain + LogLinConstants::offs :
                 LogLinConstants::base2 * std::log((in[0] + LogLinConstants::shift) * LogLinConstants::m);
        out[1] = (in[1] < LogLinConstants::xbrk) ?
                 in[1] * LogLinConstants::gain + LogLinConstants::offs :
                 LogLinConstants::base2 * std::log((in[1] + LogLinConstants::shift) * LogLinConstants::m);
        out[2] = (in[2] < LogLinConstants::xbrk) ?
                 in[2] * LogLinConstants::gain + LogLinConstants::offs :
                 LogLinConstants::base2 * std::log((in[2] + LogLinConstants::shift) * LogLinConstants::m);
        out[3] = in[3];
#endif
}

inline void LogLin(float * out)
{
#ifdef USE_SSE
        __m128 pix = _mm_loadu_ps(out);
        __m128 flag = _mm_cmpgt_ps(pix, LogLinConstants::mybrk);

        __m128 pixLin = _mm_sub_ps(pix, LogLinConstants::moffs);
        pixLin = _mm_mul_ps(pixLin, LogLinConstants::mgainInv);

        pix = ssePower(LogLinConstants::mpower, pix);
        pix = _mm_mul_ps(pix, LogLinConstants::mshift018);
        pix = _mm_sub_ps(pix, LogLinConstants::mshift);

        pix = _mm_or_ps(_mm_and_ps(flag, pix),
              _mm_andnot_ps(flag, pixLin));

        _mm_storeu_ps(out, pix);
#else
        out[0] = (out[0] < LogLinConstants::ybrk) ?
                 (out[0] - LogLinConstants::offs) / LogLinConstants::gain :
                 std::pow(2.0f, out[0]) * (0.18f + LogLinConstants::shift) - LogLinConstants::shift;
        out[1] = (out[1] < LogLinConstants::ybrk) ?
                 (out[1] - LogLinConstants::offs) / LogLinConstants::gain :
                 std::pow(2.0f, out[1]) * (0.18f + LogLinConstants::shift) - LogLinConstants::shift;
        out[2] = (out[2] < LogLinConstants::ybrk) ?
                 (out[2] - LogLinConstants::offs) / LogLinConstants::gain :
                 std::pow(2.0f, out[2]) * (0.18f + LogLinConstants::shift) - LogLinConstants::shift;
#endif
}

void GradingToneLinearFwdOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_gt->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gt->getValue();
    auto & vpr = m_gt->getComputedValue();

    for (long idx = 0; idx < numPixels; ++idx)
    {
        LinLog(in, out);

        mids(v, vpr, R, out);
        mids(v, vpr, G, out);
        mids(v, vpr, B, out);
        mids(v, vpr, M, out);

        highlightShadow(v, vpr, R, false, out);
        highlightShadow(v, vpr, G, false, out);
        highlightShadow(v, vpr, B, false, out);
        highlightShadow(v, vpr, M, false, out);

        whiteBlack(v, vpr, R, false, out);
        whiteBlack(v, vpr, G, false, out);
        whiteBlack(v, vpr, B, false, out);
        whiteBlack(v, vpr, M, false, out);

        highlightShadow(v, vpr, R, true, out);
        highlightShadow(v, vpr, G, true, out);
        highlightShadow(v, vpr, B, true, out);
        highlightShadow(v, vpr, M, true, out);

        whiteBlack(v, vpr, R, true, out);
        whiteBlack(v, vpr, G, true, out);
        whiteBlack(v, vpr, B, true, out);
        whiteBlack(v, vpr, M, true, out);

        scontrast(v, vpr, out);

        LogLin(out);

        ClampMaxRGB(out);

        in += 4;
        out += 4;
    }
}

void GradingToneLinearRevOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_gt->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    auto & v = m_gt->getValue();
    auto & vpr = m_gt->getComputedValue();

    for (long idx = 0; idx < numPixels; ++idx)
    {
        LinLog(in, out);

        scontrast(v, vpr, out);

        whiteBlack(v, vpr, M, true, out);
        whiteBlack(v, vpr, R, true, out);
        whiteBlack(v, vpr, G, true, out);
        whiteBlack(v, vpr, B, true, out);

        highlightShadow(v, vpr, M, true, out);
        highlightShadow(v, vpr, R, true, out);
        highlightShadow(v, vpr, G, true, out);
        highlightShadow(v, vpr, B, true, out);

        whiteBlack(v, vpr, M, false, out);
        whiteBlack(v, vpr, R, false, out);
        whiteBlack(v, vpr, G, false, out);
        whiteBlack(v, vpr, B, false, out);

        highlightShadow(v, vpr, M, false, out);
        highlightShadow(v, vpr, R, false, out);
        highlightShadow(v, vpr, G, false, out);
        highlightShadow(v, vpr, B, false, out);

        mids(v, vpr, M, out);
        mids(v, vpr, R, out);
        mids(v, vpr, G, out);
        mids(v, vpr, B, out);

        LogLin(out);

        ClampMaxRGB(out);

        in += 4;
        out += 4;
    }
}

} // Anonymous namspace

///////////////////////////////////////////////////////////////////////////////

ConstOpCPURcPtr GetGradingToneCPURenderer(ConstGradingToneOpDataRcPtr & tone)
{
    switch (tone->getDirection())
    {
    case TRANSFORM_DIR_FORWARD:
        if (tone->getStyle() == GRADING_LIN)
        {
            return std::make_shared<GradingToneLinearFwdOpCPU>(tone);
        }
        return std::make_shared<GradingToneFwdOpCPU>(tone);

    case TRANSFORM_DIR_INVERSE:
        if (tone->getStyle() == GRADING_LIN)
        {
            return std::make_shared<GradingToneLinearRevOpCPU>(tone);
        }
        return std::make_shared<GradingToneRevOpCPU>(tone);
    }

    throw Exception("Illegal GradingTone direction.");
}

} // namespace OCIO_NAMESPACE
