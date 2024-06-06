// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_HUECURVE_H
#define INCLUDED_OCIO_HUECURVE_H


#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingrgbcurve/GradingBSplineCurve.h"

namespace OCIO_NAMESPACE
{

// Class to hold the RGB curve data that is used in the corresponding dynamic property and in
// the CTF reader..  This allows moving some of the code from DynamicProperty to here.  The
// dynamic property is then used by the OpData, which is then used by the Op and Transform.
class HueCurveImpl : public HueCurve
{
public:
    HueCurveImpl();
    HueCurveImpl(GradingStyle style);
    HueCurveImpl(const std::array<ConstGradingBSplineCurveRcPtr, HUE_NUM_CURVES> & curves );
    HueCurveImpl(const ConstHueCurveRcPtr & rhs);

    HueCurveRcPtr createEditableCopy() const override;

    void validate() const override;
    bool isIdentity() const override;
    ConstGradingBSplineCurveRcPtr getCurve(HueCurveType) const override;
    GradingBSplineCurveRcPtr getCurve(HueCurveType) override;

    static const GradingBSplineCurveImpl DefaultHueHue;
    static const GradingBSplineCurveImpl DefaultHueSat;
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
    std::array<GradingBSplineCurveRcPtr, static_cast<size_t>(HUE_NUM_CURVES)> m_curves;
};

typedef OCIO_SHARED_PTR<const HueCurveImpl> ConstHueCurveImplRcPtr;
typedef OCIO_SHARED_PTR<HueCurveImpl> HueCurveImplRcPtr;

}

#endif //INCLUDED_OCIO_GRADINGRGBCURVE_H
