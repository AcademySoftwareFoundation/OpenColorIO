// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGPRIMARY_H
#define INCLUDED_OCIO_GRADINGPRIMARY_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

// Hold pre-computed values used at render time.
struct GradingPrimaryPreRender
{
    GradingPrimaryPreRender & operator=(const GradingPrimaryPreRender &) = default;
    GradingPrimaryPreRender(const GradingPrimaryPreRender &) = default;

    GradingPrimaryPreRender() = default;
    ~GradingPrimaryPreRender() = default;

    void update(GradingStyle style, TransformDirection dir, const GradingPrimary & v) noexcept;

    // Do not apply the op if all params are identity.
    bool m_localBypass{ false };

    // Access to the precomputed values. Note that values are already inversed based on the
    // direction so that no computation is required before using them.
    const Float3 & getBrightness() const { return m_brightness; }
    const Float3 & getContrast() const { return m_contrast; }
    const Float3 & getGamma() const { return m_gamma; }
    double getPivot() const { return m_pivot; }
    bool isGammaIdentity() const { return m_isPowerIdentity; }

    const Float3 & getExposure() const { return m_exposure; }
    const Float3 & getOffset() const { return m_offset; }
    bool isContrastIdentity() const { return m_isPowerIdentity; }

    const Float3 & getSlope() const { return m_slope; }

private:

    // Brightness, contrast, gamma
    // Exposure, contrast, offset
    // Slope, offset, gamma

    // Precomputed values are adjusted for the direction. Ex. slope is holding inverse slope
    // values for the inverse direction.
    Float3 m_brightness{ { 0.f, 0.f, 0.f } };
    Float3 m_contrast{ { 0.f, 0.f, 0.f } };
    Float3 m_gamma{ { 0.f, 0.f, 0.f } };
    Float3 m_exposure{ { 0.f, 0.f, 0.f } };
    Float3 m_offset{ { 0.f, 0.f, 0.f } };
    Float3 m_slope{ { 0.f, 0.f, 0.f } };

    double m_pivot{ 0. };

    bool m_isPowerIdentity{ false };
};

bool operator==(const GradingRGBM & lhs, const GradingRGBM & rhs);
bool operator!=(const GradingRGBM & lhs, const GradingRGBM & rhs);
bool operator==(const GradingPrimary & lhs, const GradingPrimary & rhs);
bool operator!=(const GradingPrimary & lhs, const GradingPrimary & rhs);

} // namespace OCIO_NAMESPACE

#endif
