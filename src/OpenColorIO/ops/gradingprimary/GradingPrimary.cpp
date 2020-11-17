// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingprimary/GradingPrimary.h"

namespace OCIO_NAMESPACE
{

bool operator==(const GradingRGBM & lhs, const GradingRGBM & rhs)
{
    return lhs.m_red  == rhs.m_red  && lhs.m_green  == rhs.m_green &&
           lhs.m_blue == rhs.m_blue && lhs.m_master == rhs.m_master;
}

bool operator!=(const GradingRGBM & lhs, const GradingRGBM & rhs)
{
    return !(lhs == rhs);
}

bool operator==(const GradingPrimary & lhs, const GradingPrimary & rhs)
{
    return lhs.m_brightness == rhs.m_brightness &&
           lhs.m_contrast   == rhs.m_contrast   &&
           lhs.m_gamma      == rhs.m_gamma      &&
           lhs.m_offset     == rhs.m_offset     &&
           lhs.m_exposure   == rhs.m_exposure   &&
           lhs.m_lift       == rhs.m_lift       &&
           lhs.m_gain       == rhs.m_gain       &&
           lhs.m_pivot      == rhs.m_pivot      &&
           lhs.m_saturation == rhs.m_saturation &&
           lhs.m_clampWhite == rhs.m_clampWhite &&
           lhs.m_clampBlack == rhs.m_clampBlack &&
           lhs.m_pivotWhite == rhs.m_pivotWhite &&
           lhs.m_pivotBlack == rhs.m_pivotBlack;
}

bool operator!=(const GradingPrimary & lhs, const GradingPrimary & rhs)
{
    return !(lhs == rhs);
}

double GradingPrimary::NoClampBlack()
{
    // Note that this is not a magic number, renderers do rely on this value.
    return -std::numeric_limits<double>::max();
}

double GradingPrimary::NoClampWhite()
{
    // Note that this is not a magic number, renderers do rely on this value.
    return std::numeric_limits<double>::max();
}

void GradingPrimary::validate(GradingStyle style) const
{
    // Validate all value based on the style.

    static constexpr double GradingPrimaryLowerBound = 0.01;
    static constexpr double GradingPrimaryBoundError = 0.000001;
    static constexpr double GradingPrimaryMin = GradingPrimaryLowerBound -
                                                GradingPrimaryBoundError;

    if (style != GRADING_LIN &&
       (m_gamma.m_red    < GradingPrimaryMin ||
        m_gamma.m_green  < GradingPrimaryMin ||
        m_gamma.m_blue   < GradingPrimaryMin ||
        m_gamma.m_master < GradingPrimaryMin))
    {
        std::ostringstream oss;
        oss << "GradingPrimary gamma '" << m_gamma << "' are below lower bound ("
            << GradingPrimaryLowerBound << ").";
        throw Exception(oss.str().c_str());
    }

    if (style == GRADING_LIN &&
       (m_contrast.m_red    < GradingPrimaryMin ||
        m_contrast.m_green  < GradingPrimaryMin ||
        m_contrast.m_blue   < GradingPrimaryMin ||
        m_contrast.m_master < GradingPrimaryMin))
    {
        std::ostringstream oss;
        oss << "GradingPrimary contrast '" << m_contrast << "' are below lower bound ("
            << GradingPrimaryLowerBound << ").";
        throw Exception(oss.str().c_str());
    }

    if ((m_pivotWhite - m_pivotBlack) < GradingPrimaryMin)
    {
        throw Exception("GradingPrimary black pivot should be smaller than white pivot.");
    }

    if (m_clampBlack > m_clampWhite)
    {
        throw Exception("GradingPrimary black clamp should be smaller than white clamp.");
    }
}

void GradingPrimaryPreRender::update(GradingStyle style,
                                     TransformDirection dir,
                                     const GradingPrimary & v) noexcept
{
    m_localBypass = v.m_clampBlack == GradingPrimary::NoClampBlack() &&
                    v.m_clampWhite == GradingPrimary::NoClampWhite();

    switch (style)
    {
    case GRADING_LOG:
    {
        const GradingRGBM & b = v.m_brightness;
        const GradingRGBM & c = v.m_contrast;
        const GradingRGBM & g = v.m_gamma;
        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
        {
            m_brightness[0] = static_cast<float>((b.m_master + b.m_red)   * 6.25 / 1023.);
            m_brightness[1] = static_cast<float>((b.m_master + b.m_green) * 6.25 / 1023.);
            m_brightness[2] = static_cast<float>((b.m_master + b.m_blue)  * 6.25 / 1023.);
            m_contrast[0] = static_cast<float>(c.m_master * c.m_red);
            m_contrast[1] = static_cast<float>(c.m_master * c.m_green);
            m_contrast[2] = static_cast<float>(c.m_master * c.m_blue);
            m_gamma[0] = static_cast<float>(1. / (g.m_master * g.m_red));
            m_gamma[1] = static_cast<float>(1. / (g.m_master * g.m_green));
            m_gamma[2] = static_cast<float>(1. / (g.m_master * g.m_blue));
            break;
        }
        case TRANSFORM_DIR_INVERSE:
        {
            m_brightness[0] = -static_cast<float>((b.m_master + b.m_red)   * 6.25 / 1023.);
            m_brightness[1] = -static_cast<float>((b.m_master + b.m_green) * 6.25 / 1023.);
            m_brightness[2] = -static_cast<float>((b.m_master + b.m_blue)  * 6.25 / 1023.);
            const double c0 = c.m_master * c.m_red;
            const double c1 = c.m_master * c.m_green;
            const double c2 = c.m_master * c.m_blue;
            m_contrast[0] = static_cast<float>(1. / (c0 == 0. ? 1. : c0));
            m_contrast[1] = static_cast<float>(1. / (c1 == 0. ? 1. : c1));
            m_contrast[2] = static_cast<float>(1. / (c2 == 0. ? 1. : c2));
            m_gamma[0] = static_cast<float>(g.m_master * g.m_red);
            m_gamma[1] = static_cast<float>(g.m_master * g.m_green);
            m_gamma[2] = static_cast<float>(g.m_master * g.m_blue);
            break;
        }
        }
        m_isPowerIdentity = m_gamma[0] == 1.0f && m_gamma[1] == 1.0f && m_gamma[2] == 1.0f;
        m_pivot = 0.5 + v.m_pivot * 0.5;
        m_localBypass = m_localBypass && m_isPowerIdentity &&
                        m_brightness[0] == 0.f && m_brightness[2] == 0.f && m_brightness[2] == 0.f &&
                        m_contrast[0] == 1.f && m_contrast[2] == 1.f && m_contrast[2] == 1.f;
        break;
    }
    case GRADING_LIN:
    {
        const GradingRGBM & o = v.m_offset;
        const GradingRGBM & e = v.m_exposure;
        const GradingRGBM & c = v.m_contrast;
        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
        {
            m_offset[0] = static_cast<float>(o.m_master + o.m_red);
            m_offset[1] = static_cast<float>(o.m_master + o.m_green);
            m_offset[2] = static_cast<float>(o.m_master + o.m_blue);
            m_exposure[0] = std::pow(2.0f, static_cast<float>(e.m_master + e.m_red));
            m_exposure[1] = std::pow(2.0f, static_cast<float>(e.m_master + e.m_green));
            m_exposure[2] = std::pow(2.0f, static_cast<float>(e.m_master + e.m_blue));
            m_contrast[0] = static_cast<float>(c.m_master * c.m_red);
            m_contrast[1] = static_cast<float>(c.m_master * c.m_green);
            m_contrast[2] = static_cast<float>(c.m_master * c.m_blue);
            break;
        }
        case TRANSFORM_DIR_INVERSE:
        {
            m_offset[0] = -static_cast<float>(o.m_master + o.m_red);
            m_offset[1] = -static_cast<float>(o.m_master + o.m_green);
            m_offset[2] = -static_cast<float>(o.m_master + o.m_blue);
            m_exposure[0] = 1.f / std::pow(2.f, static_cast<float>(e.m_master + e.m_red));
            m_exposure[1] = 1.f / std::pow(2.f, static_cast<float>(e.m_master + e.m_green));
            m_exposure[2] = 1.f / std::pow(2.f, static_cast<float>(e.m_master + e.m_blue));
            // Validate ensures contrast is above a threshold.
            m_contrast[0] = static_cast<float>(1. / (c.m_master * c.m_red));
            m_contrast[1] = static_cast<float>(1. / (c.m_master * c.m_green));
            m_contrast[2] = static_cast<float>(1. / (c.m_master * c.m_blue));
            break;
        }
        }
        m_isPowerIdentity = m_contrast[0] == 1.0f || m_contrast[1] == 1.0f ||
                            m_contrast[2] == 1.0f;
        m_pivot = 0.18 * std::pow(2., v.m_pivot);
        m_localBypass = m_localBypass && m_isPowerIdentity &&
                        m_exposure[0] == 1.f && m_exposure[2] == 1.f && m_exposure[2] == 1.f &&
                        m_offset[0] == 0.f && m_offset[2] == 0.f && m_offset[2] == 0.f;
        break;
    }
    case GRADING_VIDEO:
    {
        const GradingRGBM & o = v.m_offset;
        const GradingRGBM & l = v.m_lift;
        double gain0 = v.m_gain.m_master * v.m_gain.m_red;
        double gain1 = v.m_gain.m_master * v.m_gain.m_green;
        double gain2 = v.m_gain.m_master * v.m_gain.m_blue;
        gain0 = gain0 == 0. ? 1. : gain0;
        gain1 = gain1 == 0. ? 1. : gain1;
        gain2 = gain2 == 0. ? 1. : gain2;
        const GradingRGBM & g = v.m_gamma;
        switch (dir)
        {
        case TRANSFORM_DIR_FORWARD:
        {
            m_offset[0] = static_cast<float>(o.m_master + o.m_red + l.m_master + l.m_red);
            m_offset[1] = static_cast<float>(o.m_master + o.m_green + l.m_master + l.m_green);
            m_offset[2] = static_cast<float>(o.m_master + o.m_blue + l.m_master + l.m_blue);
            const double slopeDen0 = v.m_pivotWhite / gain0 + l.m_master + l.m_red - v.m_pivotBlack;
            const double slopeDen1 = v.m_pivotWhite / gain1 + l.m_master + l.m_green - v.m_pivotBlack;
            const double slopeDen2 = v.m_pivotWhite / gain2 + l.m_master + l.m_blue - v.m_pivotBlack;
            m_slope[0] = static_cast<float>((v.m_pivotWhite - v.m_pivotBlack) /
                                            (slopeDen0 == 0. ? 1. : slopeDen0));
            m_slope[1] = static_cast<float>((v.m_pivotWhite - v.m_pivotBlack) /
                                            (slopeDen1 == 0. ? 1. : slopeDen1));
            m_slope[2] = static_cast<float>((v.m_pivotWhite - v.m_pivotBlack) /
                                            (slopeDen2 == 0. ? 1. : slopeDen2));
            m_gamma[0] = static_cast<float>(1. / (g.m_master * g.m_red));
            m_gamma[1] = static_cast<float>(1. / (g.m_master * g.m_green));
            m_gamma[2] = static_cast<float>(1. / (g.m_master * g.m_blue));
            break;
        }
        case TRANSFORM_DIR_INVERSE:
        {
            m_offset[0] = -static_cast<float>(o.m_master + o.m_red + l.m_master + l.m_red);
            m_offset[1] = -static_cast<float>(o.m_master + o.m_green + l.m_master + l.m_green);
            m_offset[2] = -static_cast<float>(o.m_master + o.m_blue + l.m_master + l.m_blue);
            m_slope[0] = static_cast<float>((v.m_pivotWhite / gain0 +
                                            (l.m_master + l.m_red - v.m_pivotBlack)) /
                                            (v.m_pivotWhite - v.m_pivotBlack));
            m_slope[1] = static_cast<float>((v.m_pivotWhite / gain1 +
                                            (l.m_master + l.m_green - v.m_pivotBlack)) /
                                            (v.m_pivotWhite - v.m_pivotBlack));
            m_slope[2] = static_cast<float>((v.m_pivotWhite / gain2 +
                                            (l.m_master + l.m_blue - v.m_pivotBlack)) /
                                            (v.m_pivotWhite - v.m_pivotBlack));
            m_gamma[0] = static_cast<float>(g.m_master * g.m_red);
            m_gamma[1] = static_cast<float>(g.m_master * g.m_green);
            m_gamma[2] = static_cast<float>(g.m_master * g.m_blue);
            break;
        }
        }
        m_isPowerIdentity = m_gamma[0] == 1.0f || m_gamma[1] == 1.0f || m_gamma[2] == 1.0f;
        m_localBypass = m_localBypass && m_isPowerIdentity &&
                        m_slope[0] == 1.f && m_slope[2] == 1.f && m_slope[2] == 1.f &&
                        m_offset[0] == 0.f && m_offset[2] == 0.f && m_offset[2] == 0.f;
        break;
    }
    }
}

} // namespace OCIO_NAMESPACE
