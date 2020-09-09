// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_GRADINGRGBCURVE_H
#define INCLUDED_OCIO_GRADINGRGBCURVE_H


#include <OpenColorIO/OpenColorIO.h>

#include "ops/gradingrgbcurve/GradingBSplineCurve.h"

namespace OCIO_NAMESPACE
{

// Class to hold the RGB curve data that is used in the corresponding dynamic property and in
// the CTF reader..  This allows moving some of the code from DynamicProperty to here.  The
// dynamic property is then used by the OpData, which is then used by the Op and Transform.
class GradingRGBCurveImpl : public GradingRGBCurve
{
public:
    GradingRGBCurveImpl(GradingStyle style);
    GradingRGBCurveImpl(const ConstGradingBSplineCurveRcPtr & red,
                        const ConstGradingBSplineCurveRcPtr & green,
                        const ConstGradingBSplineCurveRcPtr & blue,
                        const ConstGradingBSplineCurveRcPtr & master);
    GradingRGBCurveImpl(const ConstGradingRGBCurveRcPtr & rhs);

    GradingRGBCurveRcPtr createEditableCopy() const override;

    void validate() const override;
    bool isIdentity() const override;
    ConstGradingBSplineCurveRcPtr getCurve(RGBCurveType c) const override;
    GradingBSplineCurveRcPtr getCurve(RGBCurveType c) override;

    static const GradingBSplineCurveImpl Default;
    static const GradingBSplineCurveImpl DefaultLin;

private:
    GradingBSplineCurveRcPtr m_curves[RGB_NUM_CURVES];
};

typedef OCIO_SHARED_PTR<const GradingRGBCurveImpl> ConstGradingRGBCurveImplRcPtr;
typedef OCIO_SHARED_PTR<GradingRGBCurveImpl> GradingRGBCurveImplRcPtr;

bool operator==(const GradingRGBCurve & lhs, const GradingRGBCurve & rhs);
bool operator!=(const GradingRGBCurve & lhs, const GradingRGBCurve & rhs);
}

#endif //INCLUDED_OCIO_GRADINGRGBCURVE_H
