// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGHUECURVE_H
#define INCLUDED_OCIO_GRADINGHUECURVE_H


#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingrgbcurve/GradingBSplineCurve.h"

namespace OCIO_NAMESPACE
{

// Class to hold the hue curve data that is used in the corresponding dynamic property and in
// the CTF reader.  This allows moving some of the code from DynamicProperty to here.  The
// dynamic property is then used by the OpData, which is then used by the Op and Transform.
class GradingHueCurveImpl : public GradingHueCurve
{
public:
    GradingHueCurveImpl();
    GradingHueCurveImpl(GradingStyle style);
    GradingHueCurveImpl(
        ConstGradingBSplineCurveRcPtr hueHue,
        ConstGradingBSplineCurveRcPtr hueSat,
        ConstGradingBSplineCurveRcPtr hueLum,
        ConstGradingBSplineCurveRcPtr lumSat,
        ConstGradingBSplineCurveRcPtr satSat,
        ConstGradingBSplineCurveRcPtr lumLum,
        ConstGradingBSplineCurveRcPtr satLum,
        ConstGradingBSplineCurveRcPtr hueFx );
    GradingHueCurveImpl(const ConstGradingHueCurveRcPtr & rhs);

    GradingHueCurveRcPtr createEditableCopy() const override;

    void validate() const override;
    bool isIdentity() const override;
    bool getDrawCurveOnly() const override;
    void setDrawCurveOnly(bool drawCurveOnly) override;
    ConstGradingBSplineCurveRcPtr getCurve(HueCurveType) const override;
    GradingBSplineCurveRcPtr getCurve(HueCurveType) override;

    static const GradingBSplineCurveImpl DefaultHueHue;
    static const GradingBSplineCurveImpl DefaultHueSat;
    static const GradingBSplineCurveImpl DefaultHueLum;
    static const GradingBSplineCurveImpl DefaultHueFx;
    static const GradingBSplineCurveImpl DefaultLumSat;
    static const GradingBSplineCurveImpl DefaultLumSatLin;
    static const GradingBSplineCurveImpl DefaultSatSat;
    static const GradingBSplineCurveImpl DefaultSatLum;
    static const GradingBSplineCurveImpl DefaultLumLum;
    static const GradingBSplineCurveImpl DefaultLumLumLin;

    static const std::array<std::reference_wrapper<const GradingBSplineCurveImpl>, static_cast<size_t>(HUE_NUM_CURVES)> DefaultCurvesLin;
    static const std::array<std::reference_wrapper<const GradingBSplineCurveImpl>, static_cast<size_t>(HUE_NUM_CURVES)> DefaultCurves;

private:
    bool isHueCurveTypeValid(HueCurveType c) const;

    bool m_drawCurveOnly = false;
    std::array<GradingBSplineCurveRcPtr, static_cast<size_t>(HUE_NUM_CURVES)> m_curves;
};

typedef OCIO_SHARED_PTR<const GradingHueCurveImpl> ConstHueCurveImplRcPtr;
typedef OCIO_SHARED_PTR<GradingHueCurveImpl> HueCurveImplRcPtr;

}

#endif //INCLUDED_OCIO_GRADINGHUECURVE_H
