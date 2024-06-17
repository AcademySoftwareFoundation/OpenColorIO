// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradinghuecurve/GradingHueCurveOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingHueCurveOpData, accessors)
{
    // Create GradingHueCurveOpData and check values. Changes them and check.
    OCIO::GradingHueCurveOpData gc( OCIO::GRADING_LOG );

    static constexpr char expected[]{ "log forward "
        "<hue_hue=<control_points=[<x=0, y=0><x=0.1666667, y=0.1666667><x=0.3333333, y=0.3333333><x=0.5, y=0.5><x=0.6666667, y=0.6666667><x=0.8333333, y=0.8333333>]>, "
        "hue_sat=<control_points=[<x=0, y=1><x=0.1666667, y=1><x=0.3333333, y=1><x=0.5, y=1><x=0.6666667, y=1><x=0.8333333, y=1>]>, "
        "hue_lum=<control_points=[<x=0, y=1><x=0.1666667, y=1><x=0.3333333, y=1><x=0.5, y=1><x=0.6666667, y=1><x=0.8333333, y=1>]>, "
        "lum_sat=<control_points=[<x=0, y=1><x=0.5, y=1><x=1, y=1>]>, "
        "sat_sat=<control_points=[<x=0, y=0><x=0.5, y=0.5><x=1, y=1>]>, "
        "lum_lum=<control_points=[<x=0, y=0><x=0.5, y=0.5><x=1, y=1>]>, "
        "sat_lum=<control_points=[<x=0, y=1><x=0.5, y=1><x=1, y=1>]>, "
        "hue_fx=<control_points=[<x=0, y=0><x=0.1666667, y=0><x=0.3333333, y=0><x=0.5, y=0><x=0.6666667, y=0><x=0.8333333, y=0>]>>" };
    OCIO_CHECK_EQUAL(gc.getCacheID(), expected);

    OCIO_CHECK_EQUAL(gc.getStyle(), OCIO::GRADING_LOG);
    auto curves = gc.getValue();
    OCIO_REQUIRE_ASSERT(curves);
    OCIO_CHECK_ASSERT(curves->isIdentity());
    OCIO_CHECK_ASSERT(gc.isIdentity());
    OCIO_CHECK_ASSERT(gc.isNoOp());
    OCIO_CHECK_ASSERT(!gc.hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!gc.getBypassLinToLog());
    
    gc.setStyle(OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gc.getStyle(), OCIO::GRADING_LIN);
    gc.setBypassLinToLog(true);
    OCIO_CHECK_ASSERT(gc.getBypassLinToLog());

    // Get dynamic property as a generic dynamic property and as a type one and verify they are
    // the same and can be made dynamic.
    OCIO_CHECK_ASSERT(!gc.isDynamic());
    auto dp = gc.getDynamicProperty();
    OCIO_REQUIRE_ASSERT(dp);
    OCIO_CHECK_EQUAL(dp->getType(), OCIO::DYNAMIC_PROPERTY_GRADING_HUECURVE);
    auto dpImpl = gc.getDynamicPropertyInternal();
    OCIO_REQUIRE_ASSERT(dpImpl);
    OCIO_CHECK_ASSERT(dp == dpImpl);
    OCIO_CHECK_ASSERT(!dpImpl->isDynamic());
    dpImpl->makeDynamic();
    OCIO_CHECK_ASSERT(gc.isDynamic());

    OCIO_CHECK_EQUAL(gc.getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    gc.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(gc.getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    // Test operator==.
    OCIO::GradingHueCurveOpData gc1{ OCIO::GRADING_LIN };
    OCIO::GradingHueCurveOpData gc2{ OCIO::GRADING_LIN };

    OCIO_CHECK_ASSERT(gc1 == gc2);
    gc1.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(!(gc1 == gc2));
    gc2.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(gc1 == gc2);

    gc1.setStyle(OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(!(gc1 == gc2));
    gc2.setStyle(OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(gc1 == gc2);

    auto v1 = gc1.getValue()->createEditableCopy();
    auto hueHue = v1->getCurve(OCIO::HUE_HUE);
    hueHue->setNumControlPoints(4);
    hueHue->getControlPoint(3).m_x = hueHue->getControlPoint(2).m_x + 1.f;
    hueHue->getControlPoint(3).m_y = hueHue->getControlPoint(2).m_y + 0.5f;
    gc1.setValue(v1);
    OCIO_CHECK_ASSERT(!(gc1 == gc2));
    auto v2 = gc2.getValue()->createEditableCopy();
    auto hueHue2 = v2->getCurve(OCIO::HUE_HUE);
    hueHue2->setNumControlPoints(4);
    hueHue2->getControlPoint(3).m_x = hueHue2->getControlPoint(2).m_x + 1.f;
    hueHue2->getControlPoint(3).m_y = hueHue2->getControlPoint(2).m_y + 0.5f;
    gc2.setValue(v2);
    OCIO_CHECK_ASSERT(gc1 == gc2);

    gc1.setSlope(OCIO::HUE_SAT, 2, 0.9f);
    OCIO_CHECK_EQUAL(gc1.getSlope(OCIO::HUE_SAT, 2), 0.9f);
    OCIO_CHECK_ASSERT(gc1.slopesAreDefault(OCIO::HUE_LUM));
    OCIO_CHECK_ASSERT(!gc1.slopesAreDefault(OCIO::HUE_SAT));

    OCIO_CHECK_EQUAL(gc1.isIdentity(), false);
    OCIO_CHECK_ASSERT(!gc1.hasChannelCrosstalk());

    // Check isInverse.

    // We have equal ops, inverse one.
    gc1.setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    // Need a shared pointer for the parameter.
    OCIO::ConstGradingHueCurveOpDataRcPtr gcptr2 = gc1.clone();
    gc1.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(gc1.isInverse(gcptr2));
    // Change value of one: no longer inverse.
    hueHue->getControlPoint(3).m_y += 0.1f;
    gc1.setValue(v1);
    OCIO_CHECK_ASSERT(!gc1.isInverse(gcptr2));
    // Restore value.
    hueHue->getControlPoint(3).m_y -= 0.1f;
    gc1.setValue(v1);
    OCIO_CHECK_ASSERT(gc1.isInverse(gcptr2));
    // Change direction: no longer inverse.
    gc1.setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(!gc1.isInverse(gcptr2));
}

OCIO_ADD_TEST(GradingHueCurveOpData, validate)
{
    // Default is valid.
    OCIO::GradingHueCurveOpData gc{ OCIO::GRADING_LOG };
    OCIO_CHECK_NO_THROW(gc.validate());

    // Curves with a single control point are not valid.
    auto curve = OCIO::GradingBSplineCurve::Create(1);
    OCIO::GradingHueCurves curvesStruct{curve, curve, curve, curve, curve, curve, curve, curve};
    auto curves = OCIO::GradingHueCurve::Create(curvesStruct);
    OCIO_CHECK_THROW_WHAT(gc.setValue(curves), OCIO::Exception,
                          "There must be at least 2 control points.");

    // Curve x coordinates have to increase.
    curve = OCIO::GradingBSplineCurve::Create({ { 0.f,0.f },{ 0.7f,0.3f },
                                                { 0.5f,0.7f },{ 1.f,1.f } });

    curvesStruct = OCIO::GradingHueCurves{curve, curve, curve, curve, curve, curve, curve, curve};
    curves = OCIO::GradingHueCurve::Create(curvesStruct);
    OCIO_CHECK_THROW_WHAT(gc.setValue(curves), OCIO::Exception,
                          "has a x coordinate '0.5' that is less from previous control "
                          "point x cooordinate '0.7'.");

    // Fix the curve x coordinate.
    curve->getControlPoint(1).m_x = 0.3f;
    curvesStruct = OCIO::GradingHueCurves{curve, curve, curve, curve, curve, curve, curve, curve};
    curves = OCIO::GradingHueCurve::Create(curvesStruct);
    OCIO_CHECK_NO_THROW(gc.setValue(curves));
    OCIO_CHECK_NO_THROW(gc.validate());
}
