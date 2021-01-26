// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradingrgbcurve/GradingRGBCurveOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingRGBCurveOpData, accessors)
{
    // Create GradingRGBCurveOpData and check values. Changes them and check.
    OCIO::GradingRGBCurveOpData gc{ OCIO::GRADING_LOG };

    static constexpr char expected[]{ "log forward "
        "<red=<control_points=[<x=0, y=0><x=0.5, y=0.5><x=1, y=1>]>, "
        "green=<control_points=[<x=0, y=0><x=0.5, y=0.5><x=1, y=1>]>, "
        "blue=<control_points=[<x=0, y=0><x=0.5, y=0.5><x=1, y=1>]>, "
        "master=<control_points=[<x=0, y=0><x=0.5, y=0.5><x=1, y=1>]>>" };
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
    OCIO_CHECK_EQUAL(dp->getType(), OCIO::DYNAMIC_PROPERTY_GRADING_RGBCURVE);
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
    OCIO::GradingRGBCurveOpData gc1{ OCIO::GRADING_LIN };
    OCIO::GradingRGBCurveOpData gc2{ OCIO::GRADING_LIN };

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
    auto red = v1->getCurve(OCIO::RGB_RED);
    red->setNumControlPoints(4);
    red->getControlPoint(3).m_x = red->getControlPoint(2).m_x + 1.f;
    red->getControlPoint(3).m_y = red->getControlPoint(2).m_y + 0.5f;
    gc1.setValue(v1);
    OCIO_CHECK_ASSERT(!(gc1 == gc2));
    auto v2 = gc2.getValue()->createEditableCopy();
    auto red2 = v2->getCurve(OCIO::RGB_RED);
    red2->setNumControlPoints(4);
    red2->getControlPoint(3).m_x = red2->getControlPoint(2).m_x + 1.f;
    red2->getControlPoint(3).m_y = red2->getControlPoint(2).m_y + 0.5f;
    gc2.setValue(v2);
    OCIO_CHECK_ASSERT(gc1 == gc2);

    gc1.setSlope(OCIO::RGB_BLUE, 2, 0.9f);
    OCIO_CHECK_EQUAL(gc1.getSlope(OCIO::RGB_BLUE, 2), 0.9f);
    OCIO_CHECK_ASSERT(gc1.slopesAreDefault(OCIO::RGB_GREEN));
    OCIO_CHECK_ASSERT(!gc1.slopesAreDefault(OCIO::RGB_BLUE));

    OCIO_CHECK_EQUAL(gc1.isIdentity(), false);
    OCIO_CHECK_ASSERT(!gc1.hasChannelCrosstalk());

    // Check isInverse.

    // We have equal ops, inverse one.
    gc1.setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    // Need a shared pointer for the parameter.
    OCIO::ConstGradingRGBCurveOpDataRcPtr gcptr2 = gc1.clone();
    gc1.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(gc1.isInverse(gcptr2));
    // Change value of one: no longer inverse.
    red->getControlPoint(3).m_y += 0.1f;
    gc1.setValue(v1);
    OCIO_CHECK_ASSERT(!gc1.isInverse(gcptr2));
    // Restore value.
    red->getControlPoint(3).m_y -= 0.1f;
    gc1.setValue(v1);
    OCIO_CHECK_ASSERT(gc1.isInverse(gcptr2));
    // Change direction: no longer inverse.
    gc1.setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(!gc1.isInverse(gcptr2));
}

OCIO_ADD_TEST(GradingRGBCurveOpData, validate)
{
    // Default is valid.
    OCIO::GradingRGBCurveOpData gc{ OCIO::GRADING_LOG };
    OCIO_CHECK_NO_THROW(gc.validate());

    // Curves with a single control point are not valid.
    auto curve = OCIO::GradingBSplineCurve::Create(1);
    auto curves = OCIO::GradingRGBCurve::Create(curve, curve, curve, curve);
    OCIO_CHECK_THROW_WHAT(gc.setValue(curves), OCIO::Exception,
                          "There must be at least 2 control points.");

    // Curve x coordinates have to increase.
    curve = OCIO::GradingBSplineCurve::Create({ { 0.f,0.f },{ 0.7f,0.3f },
                                                { 0.5f,0.7f },{ 1.f,1.f } });
    curves = OCIO::GradingRGBCurve::Create(curve, curve, curve, curve);
    OCIO_CHECK_THROW_WHAT(gc.setValue(curves), OCIO::Exception,
                          "has a x coordinate '0.5' that is less from previous control "
                          "point x cooordinate '0.7'.");

    // Fix the curve x coordinate.
    curve->getControlPoint(1).m_x = 0.3f;
    curves = OCIO::GradingRGBCurve::Create(curve, curve, curve, curve);
    OCIO_CHECK_NO_THROW(gc.setValue(curves));
    OCIO_CHECK_NO_THROW(gc.validate());
}
