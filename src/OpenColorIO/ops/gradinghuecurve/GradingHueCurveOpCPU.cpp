// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>
#include <cstring>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/gradinghuecurve/GradingHueCurveOpCPU.h"
#include "ops/fixedfunction/FixedFunctionOpCPU.h"
#include "ops/fixedfunction/FixedFunctionOpData.h"
#include "ops/matrix/MatrixOpCPU.h"
#include "ops/matrix/MatrixOpData.h"

namespace OCIO_NAMESPACE
{

namespace
{

namespace LogLinConstants
{
    static constexpr float xbrk = 0.0041318374739483946f;
    static constexpr float shift = -0.000157849851665374f;
    static constexpr float m = 1.f / (0.18f + shift);
    static constexpr float gain = 363.034608563f;
    static constexpr float offs = -7.f;
    static constexpr float ybrk = -5.5f;
    static constexpr float base2 = 1.4426950408889634f; // 1/log(2)
}

inline void LinLog(float * out)
{
    out[2] = (out[2] < LogLinConstants::xbrk) ?
             out[2] * LogLinConstants::gain + LogLinConstants::offs :
             LogLinConstants::base2 * std::log((out[2] + LogLinConstants::shift) * LogLinConstants::m);
}

inline void LogLin(float * out)
{
    out[2] = (out[2] < LogLinConstants::ybrk) ?
             (out[2] - LogLinConstants::offs) / LogLinConstants::gain :
             std::pow(2.0f, out[2]) * (0.18f + LogLinConstants::shift) - LogLinConstants::shift;
}

inline void NoOp(float * /* out */)
{
}

typedef void (apply_nonlin_func)(float *out);

class GradingHueCurveOpCPU : public OpCPU
{
public:
    GradingHueCurveOpCPU() = delete;
    GradingHueCurveOpCPU(const GradingHueCurveOpCPU &) = delete;

    explicit GradingHueCurveOpCPU(ConstGradingHueCurveOpDataRcPtr & ghuec);

    bool isDynamic() const override;
    bool hasDynamicProperty(DynamicPropertyType type) const override;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const override;

protected:
    DynamicPropertyGradingHueCurveImplRcPtr m_ghuecurve;
    bool m_isLinear = false;

    ConstOpCPURcPtr m_rgbToHsyOp;
    ConstOpCPURcPtr m_hsyToRgbOp;

    apply_nonlin_func *m_applyLinLog = NoOp;
    apply_nonlin_func *m_applyLogLin = NoOp;
};

GradingHueCurveOpCPU::GradingHueCurveOpCPU(ConstGradingHueCurveOpDataRcPtr & gcData)
    : OpCPU()
{
    m_ghuecurve = gcData->getDynamicPropertyInternal();
    if (m_ghuecurve->isDynamic())
    {
        m_ghuecurve = m_ghuecurve->createEditableCopy();
    }

    const GradingStyle style = gcData->getStyle();

    FixedFunctionOpData::Style fwdStyle = FixedFunctionOpData::RGB_TO_HSY_LIN;
    FixedFunctionOpData::Style invStyle = FixedFunctionOpData::HSY_LIN_TO_RGB;
    switch(style)
    {
    case GRADING_LIN:
        fwdStyle = FixedFunctionOpData::RGB_TO_HSY_LIN;
        invStyle = FixedFunctionOpData::HSY_LIN_TO_RGB;
        m_isLinear = true;
        break;
    case GRADING_LOG:
        fwdStyle = FixedFunctionOpData::RGB_TO_HSY_LOG;
        invStyle = FixedFunctionOpData::HSY_LOG_TO_RGB;
        break;
    case GRADING_VIDEO:
        fwdStyle = FixedFunctionOpData::RGB_TO_HSY_VID;
        invStyle = FixedFunctionOpData::HSY_VID_TO_RGB;
        break;
    }

    if (gcData->getRGBToHSY() == HSYTransformStyle::HSY_TRANSFORM_NONE)
    {
        // TODO: Could template the apply function to avoid the need for a matrix op.
        ConstMatrixOpDataRcPtr fwdOpData = std::make_shared<MatrixOpData>(TRANSFORM_DIR_FORWARD);
        m_rgbToHsyOp = GetMatrixRenderer(fwdOpData);
        m_hsyToRgbOp = GetMatrixRenderer(fwdOpData);
    }
    else
    {
        ConstFixedFunctionOpDataRcPtr fwdOpData = std::make_shared<FixedFunctionOpData>(fwdStyle);
        m_rgbToHsyOp = GetFixedFunctionCPURenderer(fwdOpData, false /* fastLogExpPow */);
        ConstFixedFunctionOpDataRcPtr invOpData = std::make_shared<FixedFunctionOpData>(invStyle);
        m_hsyToRgbOp = GetFixedFunctionCPURenderer(invOpData, false /* fastLogExpPow */);
    }

    if (style == GRADING_LIN)
    {
        m_applyLinLog = LinLog;
        m_applyLogLin = LogLin;
    }
}

bool GradingHueCurveOpCPU::isDynamic() const
{
    return m_ghuecurve->isDynamic();
}

bool GradingHueCurveOpCPU::hasDynamicProperty(DynamicPropertyType type) const
{
    bool res = false;
    if (type == DYNAMIC_PROPERTY_GRADING_HUECURVE)
    {
        res = m_ghuecurve->isDynamic();
    }
    return res;
}

DynamicPropertyRcPtr GradingHueCurveOpCPU::getDynamicProperty(DynamicPropertyType type) const
{
    if (type == DYNAMIC_PROPERTY_GRADING_HUECURVE)
    {
        if (m_ghuecurve->isDynamic())
        {
            return m_ghuecurve;
        }
    }
    else
    {
        throw Exception("Dynamic property type not supported by GradingHueCurve.");
    }

    throw Exception("GradingHueCurve property is not dynamic.");
}

class GradingHueCurveDrawOpCPU : public GradingHueCurveOpCPU
{
public:
    GradingHueCurveDrawOpCPU(const GradingHueCurveOpCPU &) = delete;
    GradingHueCurveDrawOpCPU() = delete;

    explicit GradingHueCurveDrawOpCPU(ConstGradingHueCurveOpDataRcPtr & ghuec);
    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

GradingHueCurveDrawOpCPU::GradingHueCurveDrawOpCPU(ConstGradingHueCurveOpDataRcPtr & ghuec)
    : GradingHueCurveOpCPU(ghuec)
{
}

void GradingHueCurveDrawOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    // NB: LocalBypass does not matter, need to evaluate even if it's an identity.

    const GradingBSplineCurveImpl::KnotsCoefs & knotsCoefs = m_ghuecurve->getKnotsCoefs();

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        // In drawCurveOnly mode, only evaluate the HueSat curve, with no RGB-to-HSY or LogLin.
        // (In practice it may be any of the curves, but only eval the one stored in that slot.)
        out[0] = knotsCoefs.evalCurve(static_cast<int>(HUE_SAT), in[0], 1.f);
        out[1] = knotsCoefs.evalCurve(static_cast<int>(HUE_SAT), in[1], 1.f);
        out[2] = knotsCoefs.evalCurve(static_cast<int>(HUE_SAT), in[2], 1.f);
        out[3] = in[3];

        in += 4;
        out += 4;
    }
}

class GradingHueCurveFwdOpCPU : public GradingHueCurveOpCPU
{
public:
    GradingHueCurveFwdOpCPU(const GradingHueCurveOpCPU &) = delete;
    GradingHueCurveFwdOpCPU() = delete;

    explicit GradingHueCurveFwdOpCPU(ConstGradingHueCurveOpDataRcPtr & ghuec);
    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

GradingHueCurveFwdOpCPU::GradingHueCurveFwdOpCPU(ConstGradingHueCurveOpDataRcPtr & ghuec)
    : GradingHueCurveOpCPU(ghuec)
{
}

static constexpr auto PixelSize = 4 * sizeof(float);

void GradingHueCurveFwdOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_ghuecurve->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const GradingBSplineCurveImpl::KnotsCoefs & knotsCoefs = m_ghuecurve->getKnotsCoefs();

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        m_rgbToHsyOp->apply(in, out, 1L);

        m_applyLinLog(out);

        // HUE-SAT
        const float hueSatGain = std::max(0.f, knotsCoefs.evalCurve(static_cast<int>(HUE_SAT), out[0], 1.f));
        // HUE-LUM
        float hueLumGain = std::max(0.f, knotsCoefs.evalCurve(static_cast<int>(HUE_LUM), out[0], 1.f));
        // HUE-HUE
        out[0] = knotsCoefs.evalCurve(static_cast<int>(HUE_HUE), out[0], out[0]);
        // SAT-SAT
        out[1] = std::max(0.f, knotsCoefs.evalCurve(static_cast<int>(SAT_SAT), out[1], out[1]));
        // LUM-SAT
        const float lumSatGain = std::max(0.f, knotsCoefs.evalCurve(static_cast<int>(LUM_SAT), out[2], 1.f));

        // Apply sat gain.
        const float satGain = lumSatGain * hueSatGain;
        out[1] *= satGain;

        // SAT-LUM
        const float satLumGain = std::max(0.f, knotsCoefs.evalCurve(static_cast<int>(SAT_LUM), out[1], 1.f));
        // LUM-LUM
        out[2] = knotsCoefs.evalCurve(static_cast<int>(LUM_LUM), out[2], out[2]);

        m_applyLogLin(out);

        // Limit hue-lum gain at low sat, since the hue is more noisy,
        // and when sat is 0 the hue becomes unknown (and is not invertible).
        hueLumGain = 1.f - (1.f - hueLumGain) * std::min(out[1], 1.f);

        // Apply lum gain.
        out[2] = m_isLinear ? out[2] * hueLumGain * satLumGain :
                              out[2] + (hueLumGain + satLumGain - 2.f) * 0.1f;

        // HUE-FX
        out[0] = out[0] - std::floor(out[0]);   // wrap to [0,1)
        out[0] = out[0] + knotsCoefs.evalCurve(static_cast<int>(HUE_FX), out[0], 0.f);

        m_hsyToRgbOp->apply(out, out, 1L);

        in += 4;
        out += 4;
    }
}

class GradingHueCurveRevOpCPU : public GradingHueCurveOpCPU
{
public:
    GradingHueCurveRevOpCPU(const GradingHueCurveOpCPU &) = delete;
    GradingHueCurveRevOpCPU() = delete;

    explicit GradingHueCurveRevOpCPU(ConstGradingHueCurveOpDataRcPtr & ghuec);
    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

GradingHueCurveRevOpCPU::GradingHueCurveRevOpCPU(ConstGradingHueCurveOpDataRcPtr & ghuec)
    : GradingHueCurveOpCPU(ghuec)
{
}

void GradingHueCurveRevOpCPU::apply(const void * inImg, void * outImg, long numPixels) const
{
    if (m_ghuecurve->getLocalBypass())
    {
        if (inImg != outImg)
        {
            memcpy(outImg, inImg, numPixels * PixelSize);
        }
        return;
    }

    const GradingBSplineCurveImpl::KnotsCoefs & knotsCoefs = m_ghuecurve->getKnotsCoefs();

    const float * in = (float *)inImg;
    float * out = (float *)outImg;

    for (long idx = 0; idx < numPixels; ++idx)
    {
        m_rgbToHsyOp->apply(in, out, 1L);

        // Invert HUE-FX.
        out[0] = knotsCoefs.evalCurveRevHue(static_cast<int>(HUE_FX), out[0]);

        // Invert HUE-HUE.
        out[0] = knotsCoefs.evalCurveRevHue(static_cast<int>(HUE_HUE), out[0]);

        // Use the inverted hue to calculate the HUE-SAT & HUE-LUM gains.
        out[0] = out[0] - std::floor(out[0]);   // wrap to [0,1)
        const float hue_sat_gain = std::max( 0.f, knotsCoefs.evalCurve(static_cast<int>(HUE_SAT), out[0], 1.f) );
        float hue_lum_gain = std::max( 0.f, knotsCoefs.evalCurve(static_cast<int>(HUE_LUM), out[0], 1.f) );

        // Use the output sat to calculate the SAT-LUM gain.
        out[1] = std::max(0.f, out[1]);         // guard against negative saturation
        const float sat_lum_gain = std::max( 0.f, knotsCoefs.evalCurve(static_cast<int>(SAT_LUM), out[1], 1.f) );

        hue_lum_gain = 1.f - (1.f - hue_lum_gain) * std::min(out[1], 1.f);

        // Invert the lum gain.
        const float lum_gain = hue_lum_gain * sat_lum_gain;
        out[2] = m_isLinear ? out[2] / std::max(0.01f, lum_gain) :
                              out[2] - (hue_lum_gain + sat_lum_gain - 2.f) * 0.1f;

        m_applyLinLog(out);

        // Invert LUM-LUM.
        out[2] = knotsCoefs.evalCurveRev(static_cast<int>(LUM_LUM), out[2]);

        // Use it to calc the LUM-SAT gain.
        const float lum_sat_gain = std::max( 0.f, knotsCoefs.evalCurve(static_cast<int>(LUM_SAT), out[2], 1.f) );

        m_applyLogLin(out);

        // Invert the sat gain.
        const float sat_gain = lum_sat_gain * hue_sat_gain;
        out[1] /= std::max(0.01f, sat_gain);

        // Invert SAT-SAT.
        out[1] = std::max( 0.f, knotsCoefs.evalCurveRev(static_cast<int>(SAT_SAT), out[1]) );

        m_hsyToRgbOp->apply(out, out, 1L);

        in += 4;
        out += 4;
    }
}

} // Anonymous namespace

///////////////////////////////////////////////////////////////////////////////

ConstOpCPURcPtr GetGradingHueCurveCPURenderer(ConstGradingHueCurveOpDataRcPtr & prim)
{

    // DrawCurveOnly mode ignores the direction, it's always the forward transform.
    auto dynProp = prim->getDynamicPropertyInternal();
    if (dynProp->getValue()->getDrawCurveOnly())
    {
        return std::make_shared<GradingHueCurveDrawOpCPU>(prim);
    }

    switch (prim->getDirection())
    {
    case TRANSFORM_DIR_FORWARD:
    {
        return std::make_shared<GradingHueCurveFwdOpCPU>(prim);
        break;
    }
    case TRANSFORM_DIR_INVERSE:
    {
        return std::make_shared<GradingHueCurveRevOpCPU>(prim);
        break;
    }
    }

    throw Exception("Illegal GradingHueCurve direction.");
}

} // namespace OCIO_NAMESPACE
