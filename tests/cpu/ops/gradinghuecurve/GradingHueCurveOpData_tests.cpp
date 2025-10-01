// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradinghuecurve/GradingHueCurveOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingHueCurveOpData, accessors)
{
    // Create GradingHueCurveOpData and check values. Change them and check.

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
    OCIO_CHECK_ASSERT(gc.hasChannelCrosstalk());
    OCIO_CHECK_EQUAL(gc.getRGBToHSY(), OCIO::HSY_TRANSFORM_1);
    
    gc.setStyle(OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gc.getStyle(), OCIO::GRADING_LIN);
    gc.setRGBToHSY(OCIO::HSY_TRANSFORM_NONE);
    OCIO_CHECK_EQUAL(gc.getRGBToHSY(), OCIO::HSY_TRANSFORM_NONE);

    // Get dynamic property as a generic dynamic property and as a typed one and verify they are
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
    hueHue->getControlPoint(3).m_x = hueHue->getControlPoint(2).m_x + 0.25f;
    hueHue->getControlPoint(3).m_y = hueHue->getControlPoint(2).m_y + 0.5f;
    gc1.setValue(v1);
    OCIO_CHECK_ASSERT(!(gc1 == gc2));
    auto v2 = gc2.getValue()->createEditableCopy();
    auto hueHue2 = v2->getCurve(OCIO::HUE_HUE);
    hueHue2->setNumControlPoints(4);
    hueHue2->getControlPoint(3).m_x = hueHue2->getControlPoint(2).m_x + 0.25f;
    hueHue2->getControlPoint(3).m_y = hueHue2->getControlPoint(2).m_y + 0.5f;
    gc2.setValue(v2);
    OCIO_CHECK_ASSERT(gc1 == gc2);

    gc1.setSlope(OCIO::HUE_SAT, 2, 0.9f);
    OCIO_CHECK_ASSERT(!(gc1 == gc2));
    OCIO_CHECK_EQUAL(gc1.getSlope(OCIO::HUE_SAT, 2), 0.9f);
    OCIO_CHECK_ASSERT(gc1.slopesAreDefault(OCIO::HUE_LUM));
    OCIO_CHECK_ASSERT(!gc1.slopesAreDefault(OCIO::HUE_SAT));

    OCIO_CHECK_EQUAL(gc1.isIdentity(), false);
    OCIO_CHECK_ASSERT(gc1.hasChannelCrosstalk());

    // Check isInverse.

    // Make two equal non-identity ops, invert one.
    OCIO::GradingHueCurveOpData gc3{ OCIO::GRADING_LIN };
    auto v3 = gc3.getValue()->createEditableCopy();
    auto spline = v3->getCurve(OCIO::HUE_LUM);
    spline->setNumControlPoints(2);
    spline->getControlPoint(0) = OCIO::GradingControlPoint(0.f, 2.f);
    spline->getControlPoint(1) = OCIO::GradingControlPoint(0.9f, 2.f);
    gc3.setValue(v3);
    OCIO_CHECK_ASSERT(!gc3.isIdentity());
    // Need a shared pointer for the parameter.
    OCIO::ConstGradingHueCurveOpDataRcPtr gcptr3 = gc3.clone();
    gc3.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    // They start as inverses.
    OCIO_CHECK_ASSERT(gc3.isInverse(gcptr3));

    // Change value of one: no longer an inverse.
    spline->getControlPoint(1).m_y += 0.25f;
    gc3.setValue(v3);
    OCIO_CHECK_ASSERT(!gc3.isInverse(gcptr3));
    // Restore value.
    spline->getControlPoint(1).m_y -= 0.25f;
    gc3.setValue(v3);
    OCIO_CHECK_ASSERT(gc3.isInverse(gcptr3));

    // Change slope of one: no longer an inverse.
    gc3.setSlope(OCIO::HUE_SAT, 2, 0.9f);
    OCIO_CHECK_ASSERT(!gc3.isInverse(gcptr3));
    // Restore value.
    gc3.setSlope(OCIO::HUE_SAT, 2, 0.f);
    OCIO_CHECK_ASSERT(gc3.isInverse(gcptr3));

    // Change direction: no longer an inverse.
    gc3.setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(!gc3.isInverse(gcptr3));
}

OCIO_ADD_TEST(GradingHueCurveOpData, validate)
{
    // Default is valid.
    OCIO::GradingHueCurveOpData gc{ OCIO::GRADING_LOG };
    OCIO_CHECK_NO_THROW(gc.validate());

    // Curves with a single control point are not valid.
    auto curve = OCIO::GradingBSplineCurve::Create(1);
    OCIO_CHECK_THROW_WHAT(auto curves = OCIO::GradingHueCurve::Create(
        curve, curve, curve, curve, curve, curve, curve, curve),
        OCIO::Exception, "There must be at least 2 control points.");

    // A periodic curve may not have only two control points that wrap to the same point.
    curve = OCIO::GradingBSplineCurve::Create({ { 0.f,0.f },{ 1.f,0.f } }, OCIO::HUE_FX);
    OCIO_CHECK_THROW_WHAT(auto curves = OCIO::GradingHueCurve::Create(
        curve, curve, curve, curve, curve, curve, curve, curve),
        OCIO::Exception, "The periodic spline x coordinates may not wrap to the same value.");

    // Curve x coordinates have to increase.
    curve = OCIO::GradingBSplineCurve::Create({ { 0.f,0.f },{ 0.7f,0.3f },
                                                { 0.5f,0.7f },{ 1.f,1.f } });
    OCIO_CHECK_THROW_WHAT(auto curves = OCIO::GradingHueCurve::Create(
        curve, curve, curve, curve, curve, curve, curve, curve),
        OCIO::Exception, "has a x coordinate '0.5' that is less than previous control "
                         "point x coordinate '0.7'.");

    // A hue-hue curve must have x-coordinates in [0,1].
    curve = OCIO::GradingBSplineCurve::Create({ { 0.1f,0.05f },{ 1.1f,1.05f } }, OCIO::HUE_HUE);
    OCIO_CHECK_THROW_WHAT(auto curves = OCIO::GradingHueCurve::Create(
        curve, curve, curve, curve, curve, curve, curve, curve),
        OCIO::Exception, "The HUE-HUE spline may not have x coordinates greater than one.");

    // Fix the curve x coordinate.
    curve->getControlPoint(1).m_x = 1.f;
    // But the curves are not all of the correct BSplineType.
    OCIO_CHECK_THROW_WHAT(auto curves = OCIO::GradingHueCurve::Create(
        curve, curve, curve, curve, curve, curve, curve, curve),
        OCIO::Exception, "GradingHueCurve validation failed: 'hue_sat' curve is of the wrong BSplineType.");

    // Curve y coordinates have to increase.
    curve = OCIO::GradingBSplineCurve::Create({ { 0.f,0.f },{ 0.3f,0.3f },
                                                { 0.5f,0.27f },{ 1.f,1.f } });
    OCIO_CHECK_THROW_WHAT(auto curves = OCIO::GradingHueCurve::Create(
        curve, curve, curve, curve, curve, curve, curve, curve),
        OCIO::Exception,  "point at index 2 has a y coordinate '0.27' that is less than "
                          "previous control point y coordinate '0.3'.");

    // Curve y coordinates have to increase. For hue-hue this includes comparing the wrapped last
    // point to the first point. In this case, 1.1 at the end is equivalent to 0.1 at the start.
    curve = OCIO::GradingBSplineCurve::Create({ { 0.1f,0.05f },{ 1.f,1.1f } }, OCIO::HUE_HUE);
    OCIO_CHECK_THROW_WHAT(auto curves = OCIO::GradingHueCurve::Create(
        curve, curve, curve, curve, curve, curve, curve, curve),
        OCIO::Exception, "Control point at index 0 has a y coordinate '0.05' that is less than "
                         "previous control point y coordinate '0.1'.");
}
