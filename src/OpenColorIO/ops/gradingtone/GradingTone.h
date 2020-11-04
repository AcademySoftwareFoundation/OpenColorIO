// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGTONE_H
#define INCLUDED_OCIO_GRADINGTONE_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

bool operator==(const GradingRGBMSW & lhs, const GradingRGBMSW & rhs);
bool operator!=(const GradingRGBMSW & lhs, const GradingRGBMSW & rhs);
bool operator==(const GradingTone & lhs, const GradingTone & rhs);
bool operator!=(const GradingTone & lhs, const GradingTone & rhs);

enum RGBMChannel
{
    R = 0,
    G,
    B,
    M
};

float GetChannelValue(const GradingRGBMSW & value, RGBMChannel channel);

bool IsIdentity(const GradingTone & value);

// Pre-computed constants used in the CPU evaluation that vary with the dynamic property, but do
// not vary per pixel. Note GPU is only using a few of them.
struct GradingTonePreRender
{
    GradingTonePreRender & operator=(const GradingTonePreRender &) = default;
    GradingTonePreRender(const GradingTonePreRender &) = default;

    explicit GradingTonePreRender(GradingStyle style);
    ~GradingTonePreRender() = default;

    // These values are used by CPU & GPU.
    double m_shadowsStart{ 0. };     // Pre computed shadow start
    double m_shadowsWidth{ 0. };     // Pre computed shadow width
    double m_highlightsStart{ 0. };  // Pre computed highlight start
    double m_highlightsWidth{ 0. };  // Pre computed highlight width
    double m_blacksStart{ 0. };      // Pre computed blacks start
    double m_blacksWidth{ 0. };      // Pre computed blacks width
    double m_whitesStart{ 0. };      // Pre computed whites start
    double m_whitesWidth{ 0. };      // Pre computed whites width

    void setStyle(GradingStyle style);
    void update(const GradingTone & v);

    // Arrays are currently only used by CPU.
    float m_midX[4][6]{{0.f}};
    float m_midY[4][6]{{0.f}};
    float m_midM[4][6]{{0.f}};

    float m_hsX[2][4][3]{{{0.f}}};
    float m_hsY[2][4][3]{{{0.f}}};
    float m_hsM[2][4][2]{{{0.f}}}; // m1 not used, m_hsM[][][1] is m2.

    float m_wbX[2][4][2]{{{0.f}}};
    float m_wbY[2][4][2]{{{0.f}}};
    float m_wbM[2][4][2]{{{0.f}}};
    float m_wbGain[2][4]{{0.f}};

    float m_scX[2][4]{{0.f}}; // Top/bottom, 4 values,
    float m_scY[2][4]{{0.f}};
    float m_scM[2][2]{{0.f}}; // m0 & m3

    // These values are changing with the style.
    float m_top{ 1.f };
    float m_topSC{ 1.f };
    float m_bottom{ 0.f };
    float m_pivot{ 0.4f };

    static void FromStyle(GradingStyle style, float & top, float & topSC,
                          float & bottom, float & pivot);

    // Do not apply the op if all params are identity.
    bool m_localBypass{ false };

private:
    GradingStyle m_style{ GRADING_LOG };

    void mids_precompute(const GradingTone & v, float top, float bottom);
    void highlightShadow_precompute(const GradingTone & v);
    void whiteBlack_precompute(const GradingTone & v);
    void scontrast_precompute(const GradingTone & v, float topSC, float bottom, float pivot);
};

} // namespace OCIO_NAMESPACE

#endif
