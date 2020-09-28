// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "ops/gradingtone/GradingTone.h"

namespace OCIO_NAMESPACE
{

bool operator==(const GradingRGBMSW & lhs, const GradingRGBMSW & rhs)
{
    return lhs.m_red   == rhs.m_red   && lhs.m_green  == rhs.m_green &&
           lhs.m_blue  == rhs.m_blue  && lhs.m_master == rhs.m_master &&
           lhs.m_start == rhs.m_start && lhs.m_width  == rhs.m_width;
}

bool operator!=(const GradingRGBMSW & lhs, const GradingRGBMSW & rhs)
{
    return !(lhs == rhs);
}

bool operator==(const GradingTone & lhs, const GradingTone & rhs)
{
    return lhs.m_blacks     == rhs.m_blacks     &&
           lhs.m_whites     == rhs.m_whites     &&
           lhs.m_highlights == rhs.m_highlights &&
           lhs.m_midtones   == rhs.m_midtones   &&
           lhs.m_shadows    == rhs.m_shadows    &&
           lhs.m_scontrast  == rhs.m_scontrast;
}

bool operator!=(const GradingTone & lhs, const GradingTone & rhs)
{
    return !(lhs == rhs);
}

void GradingTone::validate() const
{
    // Client app is expected to limit these to these stated bounds. 
    //
    // Blacks, mids, whites: [0.1, 1.9]
    // Shadows, highlights: [0.2, 1.8]
    // Min width: 0.01.
    // SContrast: [0.01, 1.99].
    static constexpr double MinBMW = 0.1;
    static constexpr double MaxBMW = 1.9;
    static constexpr double MinSH  = 0.2;
    static constexpr double MaxSH  = 1.8;
    static constexpr double MinWSC = 0.01;
    static constexpr double MaxSC  = 1.99;
    // The bounds are widened here slightly to avoid failures due to precision issues.
    static constexpr double Error = 0.000001;
    static constexpr double MinBMWTol = MinBMW - Error;
    static constexpr double MaxBMWTol = MaxBMW + Error;
    static constexpr double MinSHTol  = MinSH - Error;
    static constexpr double MaxSHTol  = MaxSH + Error;
    static constexpr double MinWSCTol = MinWSC - Error;
    static constexpr double MaxSCTol  = MaxSC - Error;

    {
        const auto & bd = m_blacks;

        if (bd.m_red    < MinBMWTol ||
            bd.m_green  < MinBMWTol ||
            bd.m_blue   < MinBMWTol ||
            bd.m_master < MinBMWTol)
        {
            std::ostringstream oss;
            oss << "GradingTone blacks '" << bd << "' are below lower bound (" << MinBMW << ").";
            throw Exception(oss.str().c_str());
        }
        if (bd.m_width < MinWSCTol)
        {
            std::ostringstream oss;
            oss << "GradingTone blacks width '" << bd.m_width << "' is below lower bound ("
                << MinWSC << ").";
            throw Exception(oss.str().c_str());
        }
        if (bd.m_red    > MaxBMWTol ||
            bd.m_green  > MaxBMWTol ||
            bd.m_blue   > MaxBMWTol ||
            bd.m_master > MaxBMWTol)
        {
            std::ostringstream oss;
            oss << "GradingTone blacks '" << bd << "' are above upper bound (" << MaxBMW << ").";
            throw Exception(oss.str().c_str());
        }
    }
    {
        const auto & midtones = m_midtones;

        if (midtones.m_red    < MinBMWTol ||
            midtones.m_green  < MinBMWTol ||
            midtones.m_blue   < MinBMWTol ||
            midtones.m_master < MinBMWTol)
        {
            std::ostringstream oss;
            oss << "GradingTone midtones '" << midtones << "' are below lower bound ("
                << MinBMW << ").";
            throw Exception(oss.str().c_str());
        }
        if (midtones.m_width < MinWSCTol)
        {
            std::ostringstream oss;
            oss << "GradingTone midtones width '" << midtones.m_width << "' is below lower bound ("
                << MinWSC << ").";
            throw Exception(oss.str().c_str());
        }
        if (midtones.m_red    > MaxBMWTol ||
            midtones.m_green  > MaxBMWTol ||
            midtones.m_blue   > MaxBMWTol ||
            midtones.m_master > MaxBMWTol)
        {
            std::ostringstream oss;
            oss << "GradingTone midtones '" << midtones << "' are above upper bound ("
                << MaxBMW << ").";
            throw Exception(oss.str().c_str());
        }
    }
    {
        const auto & dw = m_whites;

        if (dw.m_red    < MinBMWTol ||
            dw.m_green  < MinBMWTol ||
            dw.m_blue   < MinBMWTol ||
            dw.m_master < MinBMWTol)
        {
            std::ostringstream oss;
            oss << "GradingTone whites '" << dw << "' are below lower bound ("
                << MinBMW << ").";
            throw Exception(oss.str().c_str());
        }
        if (dw.m_width < MinWSCTol)
        {
            std::ostringstream oss;
            oss << "GradingTone whites width '" << dw.m_width << "' is below lower bound ("
                << MinWSC << ").";
            throw Exception(oss.str().c_str());
        }
        if (dw.m_red    > MaxBMWTol ||
            dw.m_green  > MaxBMWTol ||
            dw.m_blue   > MaxBMWTol ||
            dw.m_master > MaxBMWTol)
        {
            std::ostringstream oss;
            oss << "GradingTone white '" << dw << "' are above upper bound ("
                << MaxBMW << ").";
            throw Exception(oss.str().c_str());
        }
    }
    {
        const auto & shadows = m_shadows;

        if (shadows.m_red    < MinSHTol ||
            shadows.m_green  < MinSHTol ||
            shadows.m_blue   < MinSHTol ||
            shadows.m_master < MinSHTol)
        {
            std::ostringstream oss;
            oss << "GradingTone shadows '" << shadows << "' are below lower bound ("
                << MinSH << ").";
            throw Exception(oss.str().c_str());
        }
        // Check that pivot is not overlapping start.
        if (shadows.m_start < shadows.m_width + MinWSCTol)
        {
            std::ostringstream oss;
            oss << "GradingTone shadows start '" << shadows.m_start << "' is less than pivot ('"
                << shadows.m_width << "' + " << MinWSC << ").";
            throw Exception(oss.str().c_str());
        }
        if (shadows.m_red    > MaxSHTol ||
            shadows.m_green  > MaxSHTol ||
            shadows.m_blue   > MaxSHTol ||
            shadows.m_master > MaxSHTol)
        {
            std::ostringstream oss;
            oss << "GradingTone shadows '" << shadows << "' are above upper bound ("
                << MaxSH << ").";
            throw Exception(oss.str().c_str());
        }
    }
    {
        const auto & hl = m_highlights;

        if (hl.m_red    < MinSHTol ||
            hl.m_green  < MinSHTol ||
            hl.m_blue   < MinSHTol ||
            hl.m_master < MinSHTol)
        {
            std::ostringstream oss;
            oss << "GradingTone highlights '" << hl << "' are below lower bound ("
                << MinSH << ").";
            throw Exception(oss.str().c_str());
        }
        // Check that pivot is not overlapping start.
        if (hl.m_start > hl.m_width - MinWSCTol)
        {
            std::ostringstream oss;
            oss << "GradingTone highlights start '" << hl.m_start << "' is greater than pivot ('"
                << hl.m_width << "' - " << MinWSC << ").";
            throw Exception(oss.str().c_str());
        }
        if (hl.m_red    > MaxSHTol ||
            hl.m_green  > MaxSHTol ||
            hl.m_blue   > MaxSHTol ||
            hl.m_master > MaxSHTol)
        {
            std::ostringstream oss;
            oss << "GradingTone highlights '" << hl << "' are above upper bound ("
                << MaxSH << ").";
            throw Exception(oss.str().c_str());
        }
    }
    {
        if (m_scontrast < MinWSCTol)
        {
            std::ostringstream oss;
            oss << "GradingTone s-contrast '" << m_scontrast << "' is below lower bound ("
                << MinWSC << ").";
            throw Exception(oss.str().c_str());
        }
        if (m_scontrast > MaxSCTol)
        {
            std::ostringstream oss;
            oss << "GradingTone s-contrast '" << m_scontrast << "' is above upper bound ("
                << MaxSC << ").";
            throw Exception(oss.str().c_str());
        }
    }
}

float GetChannelValue(const GradingRGBMSW & value, RGBMChannel channel)
{
    if (channel == R)
    {
        return static_cast<float>(value.m_red);
    }
    else if (channel == G)
    {
        return static_cast<float>(value.m_green);
    }
    else if (channel == B)
    {
        return static_cast<float>(value.m_blue);
    }
    if (channel == M)
    {
        return static_cast<float>(value.m_master);
    }
    return 0.f;
}

namespace
{
bool IsIdentity(const GradingRGBMSW & val)
{
    return val.m_red == 1. && val.m_green == 1. && val.m_blue == 1. && val.m_master == 1.;
}
}

bool IsIdentity(const GradingTone & value)
{
    return IsIdentity(value.m_blacks)   && IsIdentity(value.m_shadows)    &&
           IsIdentity(value.m_midtones) && IsIdentity(value.m_highlights) &&
           IsIdentity(value.m_whites)   && value.m_scontrast == 1.;
}

GradingTonePreRender::GradingTonePreRender(GradingStyle style)
{
    setStyle(style);
}

void GradingTonePreRender::FromStyle(GradingStyle style, float & top, float & topSC,
                                     float & bottom, float & pivot)
{
    switch (style)
    {
    case GRADING_LOG:
        top = 1.f;      // Might like to move these for ACES, but cannot for ARRI K1S1.
        topSC = 1.f;
        bottom = 0.f;
        pivot = 0.4f;
        break;
    case GRADING_LIN:
        top = 7.5;
        topSC = 6.5;
        bottom = -5.5f; // place at breakpoint of lin-to-log.
        pivot = 0.f;
        break;
    case GRADING_VIDEO:
        top = 1.f;
        topSC = 1.f;
        bottom = 0.f;
        pivot = 0.4f;   // aces 0.18 --> 0.39.
        break;
    default:
        break;
    }
}

void GradingTonePreRender::setStyle(GradingStyle style)
{
    if (m_style != style)
    {
    m_style = style;
    FromStyle(style, m_top, m_topSC, m_bottom, m_pivot);
}
}

namespace
{

double FauxCubicFwdEval(double t, double x0, double x2, double y0, double y2,
                        double m0, double m2, double x1)
{
    const double y1 = (0.5 / ((x2 - x1) + (x1 - x0))) *
                      ((2.*y0 + m0*(x1 - x0)) * (x2 - x1) + (2.*y2 - m2 * (x2 - x1)) * (x1 - x0));

    const double tL = (t - x0) / (x1 - x0);
    const double tR = (t - x1) / (x2 - x1);
    const double fL = y0 * (1. - tL*tL) + y1 * tL*tL + m0 * (1. - tL) * tL * (x1 - x0);
    const double fR = y1 * (1. - tR) * (1. - tR) + y2 * (2. - tR)*tR +
                      m2 * (tR - 1.) * tR * (x2 - x1);

    double res = (t < x1) ? fL : fR;
    res = (t < x0) ? y0 + (t - x0) * m0 : res;
    res = (t > x2) ? y2 + (t - x2) * m2 : res;
    return res;
}

double FauxCubicRevEval(double t, double x0, double x2, double y0, double y2,
                        double m0, double m2, double x1)
{
    const double y1 = (0.5 / ((x2 - x1) + (x1 - x0))) *
                      ((2.*y0 + m0*(x1 - x0)) * (x2 - x1) + (2.*y2 - m2 * (x2 - x1)) * (x1 - x0));

    const double cL = y0 - t;
    const double bL = m0 * (x1 - x0);
    const double aL = y1 - y0 - m0 * (x1 - x0);
    const double discrimL = sqrt(bL * bL - 4. * aL * cL);
    const double tmpL = (2. * cL) / (-discrimL - bL);
    const double outL = tmpL * (x1 - x0) + x0;

    const double cR = y1 - t;
    const double bR = 2.*y2 - 2.*y1 - m2 * (x2 - x1);
    const double aR = y1 - y2 + m2 * (x2 - x1);
    const double discrimR = sqrt(bR * bR - 4. * aR * cR);
    const double tmpR = (2. * cR) / (-discrimR - bR);
    const double outR = tmpR * (x2 - x1) + x1;

    double res = (t < y1) ? outL : outR;
    res = (t < y0) ? x0 + (t - y0) / m0 : res;
    res = (t > y2) ? x2 + (t - y2) / m2 : res;
    return res;
}

double HighlightFwdEval(double t, double start, double pivot, double val)
{
    const double x0 = start;
    const double x2 = pivot;
    const double y0 = x0;
    const double y2 = x2;
    const double m0 = 1.;
    const double x1 = x0 + (x2 - x0) * 0.5;
    val = 2. - val;
    if (val <= 1.)
    {
        const double m2 = (val < 0.01) ? 0.01 : val;
        return FauxCubicFwdEval(t, x0, x2, y0, y2, m0, m2, x1);
    }
    else
    {
        const double m2 = (2. - val < 0.01) ? 0.01 : 2. - val;
        return FauxCubicRevEval(t, x0, x2, y0, y2, m0, m2, x1);
    }
}

double ShadowFwdEval(double t, double start, double pivot, double val)
{
    const double x0 = start;
    const double x2 = pivot;
    const double y0 = x0;
    const double y2 = x2;
    const double m2 = 1.;
    const double x1 = x0 + (x2 - x0) * 0.5;
    if (val <= 1.)
    {
        const double m0 = (val < 0.01) ? 0.01 : val;
        return FauxCubicFwdEval(t, x0, x2, y0, y2, m0, m2, x1);
    }
    else
    {
        const double m0 = (2. - val < 0.01) ? 0.01 : 2. - val;
        return FauxCubicRevEval(t, x0, x2, y0, y2, m0, m2, x1);
    }
}
}
void GradingTonePreRender::update(const GradingTone & v)
{
    m_localBypass = IsIdentity(v);
    if (m_localBypass) return;

    {
        const double master = v.m_highlights.m_master;
        const double start  = v.m_highlights.m_start;
        const double pivot  = v.m_highlights.m_width;
        const double startw = v.m_whites.m_start;
        const double widthw = v.m_whites.m_width;

        m_highlightsStart = (start > pivot - 0.01) ? pivot - 0.01 : start;
        m_highlightsWidth = pivot;

        const double new_start = HighlightFwdEval(startw, m_highlightsStart, m_highlightsWidth,
                                                  master);
        const double new_end = HighlightFwdEval(startw + widthw, m_highlightsStart,
                                                m_highlightsWidth, master);
        m_whitesStart = new_start;
        m_whitesWidth = new_end - new_start;
    }
    {
        const double master = v.m_shadows.m_master;
        const double start  = v.m_shadows.m_start;
        const double pivot  = v.m_shadows.m_width;
        const double startb = v.m_blacks.m_start;
        const double widthb = v.m_blacks.m_width;

        m_shadowsStart = (start < pivot + 0.01) ? pivot + 0.01 : start;
        m_shadowsWidth = pivot;

        const double new_start = ShadowFwdEval(startb, m_shadowsWidth, m_shadowsStart, master);
        const double new_end = ShadowFwdEval(startb - widthb, m_shadowsWidth, m_shadowsStart,
                                             master);
        m_blacksStart = new_start;
        m_blacksWidth = new_start - new_end;
    }


    mids_precompute(v, m_top, m_bottom);
    highlightShadow_precompute(v);
    whiteBlack_precompute(v);
    scontrast_precompute(v, m_topSC, m_bottom, m_pivot);
}

void GradingTonePreRender::mids_precompute(const GradingTone & v, float top, float bottom)
{
    static constexpr float halo = 0.4f;

    for (const auto channel : { R, G, B, M })
    {
        auto & x0 = m_midX[channel][0];
        auto & x1 = m_midX[channel][1];
        auto & x2 = m_midX[channel][2];
        auto & x3 = m_midX[channel][3];
        auto & x4 = m_midX[channel][4];
        auto & x5 = m_midX[channel][5];
        auto & y0 = m_midY[channel][0];
        auto & y1 = m_midY[channel][1];
        auto & y2 = m_midY[channel][2];
        auto & y3 = m_midY[channel][3];
        auto & y4 = m_midY[channel][4];
        auto & y5 = m_midY[channel][5];
        auto & m0 = m_midM[channel][0];
        auto & m1 = m_midM[channel][1];
        auto & m2 = m_midM[channel][2];
        auto & m3 = m_midM[channel][3];
        auto & m4 = m_midM[channel][4];
        auto & m5 = m_midM[channel][5];

        float mid_adj = Clamp(GetChannelValue(v.m_midtones, channel), 0.01f, 1.99f);

        if (mid_adj != 1.f)
        {
            x0 = bottom;
            x5 = top;

            const float max_width = (x5 - x0) * 0.95f;
            const float width = Clamp(static_cast<float>(v.m_midtones.m_width), 0.01f, max_width);
            const float min_cent = x0 + width * 0.51f;
            const float max_cent = x5 - width * 0.51f;
            const float center = Clamp(static_cast<float>(v.m_midtones.m_start), min_cent,
                                                          max_cent);

            x1 = center - width * 0.5f;
            x4 = x1 + width;

            x2 = x1 + (x4 - x1) * 0.25f;
            x3 = x1 + (x4 - x1) * 0.75f;
            y0 = x0;
            m0 = 1.f;
            m5 = 1.f;

            const float min_slope = 0.1f;

            mid_adj = mid_adj - 1.f;
            mid_adj = mid_adj * (1.f - min_slope);

            m2 = 1.f + mid_adj;
            m3 = 1.f - mid_adj;
            m1 = 1.f + mid_adj * halo;
            m4 = 1.f - mid_adj * halo;

            if (center <= (x5 + x0) * 0.5f)
            {
                const float area = (x1 - x0) * (m1 - m0) * 0.5f +
                                   (x2 - x1) * ((m1 - m0) + (m2 - m1) * 0.5f) +
                                   (center - x2) * (m2 - m0) * 0.5f;
                m4 = (-0.5f * (x5 - x4) * m5 + (x4 - x3) * (0.5f * m3 - m5) +
                     (x3 - center) * (m3 - m5) * 0.5f + area) / (-0.5f * (x5 - x3));
            }
            else
            {
                const float area = (x5 - x4) * (m4 - m5) * 0.5f +
                                   (x4 - x3) * ((m4 - m5) + (m3 - m4) * 0.5f) +
                                   (x3 - center) * (m3 - m5) * 0.5f;
                m1 = (-0.5f * (x1 - x0) * m0 + (x2 - x1) * (0.5f * m2 - m0) +
                     (center - x2) * (m2 - m0) * 0.5f + area) / (-0.5f * (x2 - x0));
            }

            y1 = y0 + (m0 + m1) * (x1 - x0) * 0.5f;
            y2 = y1 + (m1 + m2) * (x2 - x1) * 0.5f;
            y3 = y2 + (m2 + m3) * (x3 - x2) * 0.5f;
            y4 = y3 + (m3 + m4) * (x4 - x3) * 0.5f;
            y5 = y4 + (m4 + m5) * (x5 - x4) * 0.5f;
        }
    }
}

void GradingTonePreRender::highlightShadow_precompute(const GradingTone & v)
{
    for (const auto isShadow : { false, true })
    {
        for (const auto channel : { R, G, B, M })
        {
            auto & x0 = m_hsX[isShadow ? 1 : 0][channel][0];
            auto & x1 = m_hsX[isShadow ? 1 : 0][channel][1];
            auto & x2 = m_hsX[isShadow ? 1 : 0][channel][2];
            auto & y0 = m_hsY[isShadow ? 1 : 0][channel][0];
            auto & y1 = m_hsY[isShadow ? 1 : 0][channel][1];
            auto & y2 = m_hsY[isShadow ? 1 : 0][channel][2];
            auto & m0 = m_hsM[isShadow ? 1 : 0][channel][0];
            auto & m2 = m_hsM[isShadow ? 1 : 0][channel][1];

            float val = isShadow ? GetChannelValue(v.m_shadows, channel) :
                                   GetChannelValue(v.m_highlights, channel);
            if (!isShadow)
            {
                val = 2.f - val;
            }
            if (val != 1.f)
            {
                const float start = static_cast<float>(isShadow ? m_shadowsStart :
                                                                  m_highlightsStart);
                const float pivot = static_cast<float>(isShadow ? m_shadowsWidth :
                                                                  m_highlightsWidth);
    
                x0 = isShadow ? pivot : start;
                x2 = isShadow ? start : pivot;
                y0 = x0;
                y2 = x2;
                x1 = x0 + (x2 - x0) * 0.5f;
    
                if (val < 1.f)
                {
                    m0 = isShadow ? std::max(0.01f, val) : 1.f;
                    m2 = isShadow ? 1.f : std::max(0.01f, val);
                    y1 = (0.5f / (x2 - x0)) * ((2.f * y0 + m0 * (x1 - x0)) * (x2 - x1) +
                        (2.f * y2 - m2 * (x2 - x1)) * (x1 - x0));
                }
                else if (val > 1.f)
                {
                    m0 = isShadow ? std::max(0.01f, 2.f - val) : 1.f;
                    m2 = isShadow ? 1.f : std::max(0.01f, 2.f - val);
                    y1 = (0.5f / ((x2 - x1) + (x1 - x0))) * ((2.f*y0 + m0 * (x1 - x0)) * (x2 - x1) +
                        (2.f*y2 - m2 * (x2 - x1)) * (x1 - x0));
                }
            }
        }
    }
}

void GradingTonePreRender::whiteBlack_precompute(const GradingTone & v)
{
    for (const auto isBlack : { false, true })
    {
        for (const auto channel : { R, G, B, M })
        {
            auto & x0 = m_wbX[isBlack ? 1 : 0][channel][0];
            auto & x1 = m_wbX[isBlack ? 1 : 0][channel][1];
            auto & y0 = m_wbY[isBlack ? 1 : 0][channel][0];
            auto & y1 = m_wbY[isBlack ? 1 : 0][channel][1];
            auto & m0 = m_wbM[isBlack ? 1 : 0][channel][0];
            auto & m1 = m_wbM[isBlack ? 1 : 0][channel][1];
            auto & gain = m_wbGain[isBlack ? 1 : 0][channel];

            const float start = static_cast<float>(isBlack ? m_blacksStart :
                                                             m_whitesStart);
            const float width = static_cast<float>(isBlack ? m_blacksWidth :
                                                             m_whitesWidth);

            float val = isBlack ? GetChannelValue(v.m_blacks, channel) :
                                  GetChannelValue(v.m_whites, channel);

            x0 = (!isBlack) ? start : start - width;
            x1 = (!isBlack) ? x0 + width : start;

            const float mtest = (!isBlack) ? val : 2.f - val;

            if (mtest < 1.f)
            {
                // Slope is decreasing case.

                if (!isBlack)
                {
                    m0 = 1.f;
                    m1 = std::max(0.01f, val);
                    y0 = x0;   // for !isBlack
                    y1 = y0 + (m0 + m1) * (x1 - x0) * 0.5f;
                }
                else
                {
                    m0 = 2.f - val;
                    m0 = std::max(0.01f, m0);
                    m1 = 1.f;
                    y1 = x1;   // for isBlack
                    y0 = y1 - (m0 + m1) * (x1 - x0) * 0.5f;
                }

            }
            else if (mtest > 1.f)
            {
                // Slope is increasing case.

                if (!isBlack)
                {
                    m0 = 1.f;
                    m1 = 2.f - val;
                    m1 = std::max(0.01f, m1);
                    y0 = x0;   // for !isBlack
                               // y1 won't be used.
                }
                else
                {
                    m0 = val;
                    m0 = std::max(0.01f, m0);
                    m1 = 1.f;
                    y1 = x1;   // for isBlack
                    y0 = y1 - (m0 + m1) * (x1 - x0) * 0.5f;
                }
                gain = (m0 + m1) * 0.5f;
            }
        }
    }
}

void GradingTonePreRender::scontrast_precompute(const GradingTone & v, float topSC, float bottom,
                                                float pivot)
{
    float contrast = static_cast<float>(v.m_scontrast);
    if (contrast != 1.f)
    {
        // Limit the range of values to prevent reversals.
        contrast = (contrast > 1.f) ? 1.f / (1.8125f - 0.8125f * std::min(contrast, 1.99f)) :
                                      0.28125f + 0.71875f * std::max(contrast, 0.01f);

        // Top end
        {
            auto & x0 = m_scX[0][0];
            auto & x1 = m_scX[0][1];
            auto & x2 = m_scX[0][2];
            auto & x3 = m_scX[0][3];
            auto & y0 = m_scY[0][0];
            auto & y1 = m_scY[0][1];
            auto & y2 = m_scY[0][2];
            auto & y3 = m_scY[0][3];
            auto & m0 = m_scM[0][0];
            auto & m3 = m_scM[0][1];

            x3 = topSC;
            y3 = topSC;
            y0 = pivot + (y3 - pivot) * 0.25f;
            m0 = contrast;
            x0 = pivot + (y0 - pivot) / m0;
            const float min_width = (x3 - x0) * 0.3f;
            m3 = 1.f / m0;
            // NB: Due to the if (contrast != 1.) clause above, m0 != m3.
            const float center = (y3 - y0 - m3*x3 + m0*x0) / (m0 - m3);
            x1 = x0;
            x2 = 2.f * center - x1;
            if (x2 > x3)
            {
                x2 = x3;
                x1 = 2.f * center - x2;
            }
            else if ((x2 - x1) < min_width)
            {
                x2 = x1 + min_width;
                const float new_center = (x2 + x1) * 0.5f;
                m3 = (y3 - y0 + m0*x0 - new_center * m0) / (x3 - new_center);
            }
            y1 = y0;
            y2 = y1 + (m0 + m3) * (x2 - x1) * 0.5f;
        }

        // Bottom end
        {
            auto & x0 = m_scX[1][0];
            auto & x1 = m_scX[1][1];
            auto & x2 = m_scX[1][2];
            auto & x3 = m_scX[1][3];
            auto & y0 = m_scY[1][0];
            auto & y1 = m_scY[1][1];
            auto & y2 = m_scY[1][2];
            auto & y3 = m_scY[1][3];
            auto & m0 = m_scM[1][0];
            auto & m3 = m_scM[1][1];

            x0 = bottom;
            y0 = bottom;
            y3 = pivot - (pivot - y0) * 0.25f;
            m3 = contrast;
            x3 = pivot - (pivot - y3) / m3;
            const float min_width = (x3 - x0) * 0.3f;
            m0 = 1.f / m3;
            const float center = (y3 - y0 - m3*x3 + m0*x0) / (m0 - m3);
            x2 = x3;
            x1 = 2.f * center - x2;
            if (x1 < x0)
            {
                x1 = x0;
                x2 = 2.f * center - x1;
            }
            else if ((x2 - x1) < min_width)
            {
                x1 = x2 - min_width;
                const float new_center = (x2 + x1) * 0.5f;
                m0 = (y3 - y0 - m3*x3 + new_center * m3) / (new_center - x0);
            }
            y2 = y3;
            y1 = y2 - (m0 + m3) * (x2 - x1) * 0.5f;
        }
    }  // end if contrast != 1.
}

} // namespace OCIO_NAMESPACE

