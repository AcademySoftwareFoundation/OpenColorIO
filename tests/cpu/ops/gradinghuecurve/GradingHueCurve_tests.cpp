// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "DynamicProperty.h"
#include "ops/gradinghuecurve/GradingHueCurve.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingHueCurve, basic)
{
    auto curve = OCIO::GradingBSplineCurve::Create({ { 0.f,0.f },{ 0.2f,0.2f },
                                                    { 0.5f,0.7f },{ 1.f,1.f } }, OCIO::HUE_HUE);
    OCIO::ConstGradingBSplineCurveRcPtr curveHH = curve;
    OCIO_CHECK_EQUAL(0.2f, curveHH->getControlPoint(1).m_y);
    curve->getControlPoint(1).m_y = 0.3f;
    OCIO_CHECK_EQUAL(0.3f, curveHH->getControlPoint(1).m_y);
    OCIO::ConstGradingBSplineCurveRcPtr curveHS = OCIO::GradingBSplineCurve::Create(4, OCIO::HUE_SAT);
    OCIO::ConstGradingBSplineCurveRcPtr curveHL = OCIO::GradingBSplineCurve::Create(3, OCIO::HUE_LUM);
    OCIO::ConstGradingBSplineCurveRcPtr curveLS = OCIO::GradingBSplineCurve::Create(2, OCIO::LUM_SAT);
    OCIO::ConstGradingBSplineCurveRcPtr curveSS = OCIO::GradingBSplineCurve::Create(2, OCIO::SAT_SAT);
    OCIO::ConstGradingBSplineCurveRcPtr curveLL = OCIO::GradingBSplineCurve::Create(2, OCIO::LUM_LUM);
    OCIO::ConstGradingBSplineCurveRcPtr curveSL = OCIO::GradingBSplineCurve::Create(2, OCIO::SAT_LUM);
    OCIO::ConstGradingBSplineCurveRcPtr curveHFX = OCIO::GradingBSplineCurve::Create(2, OCIO::HUE_FX);

    // The Create function takes 8 pointers to curves and creates new curves that are copies of
    // the 8 parameters.  First try to create one using curveHH instead of curveSS.
    OCIO_CHECK_THROW_WHAT(auto badHueCurve = OCIO::GradingHueCurve::Create(curveHH, curveHS, curveHL, curveLS,
                                                                           curveHH, curveLL, curveSL, curveHFX),
                          OCIO::Exception,
                          "GradingHueCurve validation failed: 'sat_sat' curve is of the wrong BSplineType.");

    // Now create a valid one.
    auto hueCurve = OCIO::GradingHueCurve::Create(curveHH, curveHS, curveHL, curveLS,
                                                  curveSS, curveLL, curveSL, curveHFX);
    OCIO_REQUIRE_ASSERT(hueCurve);
    OCIO_CHECK_NO_THROW(hueCurve->validate());

    OCIO_REQUIRE_ASSERT(hueCurve->getCurve(OCIO::HUE_HUE));
    OCIO_REQUIRE_ASSERT(hueCurve->getCurve(OCIO::HUE_SAT));
    OCIO_REQUIRE_ASSERT(hueCurve->getCurve(OCIO::HUE_LUM));
    OCIO_REQUIRE_ASSERT(hueCurve->getCurve(OCIO::LUM_SAT));
    OCIO_REQUIRE_ASSERT(hueCurve->getCurve(OCIO::SAT_SAT));
    OCIO_REQUIRE_ASSERT(hueCurve->getCurve(OCIO::LUM_LUM));
    OCIO_REQUIRE_ASSERT(hueCurve->getCurve(OCIO::SAT_LUM));
    OCIO_REQUIRE_ASSERT(hueCurve->getCurve(OCIO::HUE_FX));
    OCIO_CHECK_THROW_WHAT(hueCurve->getCurve(OCIO::HUE_NUM_CURVES), OCIO::Exception,
                         "The HueCurveType provided is illegal");

    // Validate that the Create function made copies of its curve arguments.
    OCIO::GradingBSplineCurveRcPtr copiedCurve = hueCurve->getCurve(OCIO::HUE_HUE);
    OCIO_CHECK_EQUAL(0.3f, copiedCurve->getControlPoint(1).m_y);
    curve->getControlPoint(1).m_y = 0.4f;
    OCIO_CHECK_EQUAL(0.4f, curve->getControlPoint(1).m_y);
    OCIO_CHECK_EQUAL(0.3f, copiedCurve->getControlPoint(1).m_y);

    // Set the type to the wrong BSpline type and re-validate.
    copiedCurve->setSplineType(OCIO::DIAGONAL_B_SPLINE);
    OCIO_CHECK_THROW_WHAT(hueCurve->validate(), OCIO::Exception,
                         "GradingHueCurve validation failed: 'hue_hue' curve is of the wrong BSplineType.");
    // Turn on drawCurveOnly mode and verify that any spline type is now allowed.
    hueCurve->setDrawCurveOnly(true);
    OCIO_CHECK_NO_THROW(hueCurve->validate());

    // Test default curves.
    auto hueCurveLin = OCIO::GradingHueCurve::Create(OCIO::GRADING_LIN);
    auto hueCurveLog = OCIO::GradingHueCurve::Create(OCIO::GRADING_LOG);
    auto hueCurveVideo = OCIO::GradingHueCurve::Create(OCIO::GRADING_VIDEO);
    OCIO_REQUIRE_ASSERT(hueCurveLin && hueCurveLog && hueCurveVideo);
    OCIO_CHECK_ASSERT(*hueCurveLog == *hueCurveVideo);
    OCIO_CHECK_ASSERT(*hueCurveLog != *hueCurveLin);

    OCIO_CHECK_ASSERT(*hueCurveLog->getCurve(OCIO::HUE_LUM) ==
                      *hueCurveLog->getCurve(OCIO::HUE_SAT));
    OCIO_CHECK_ASSERT(*hueCurveLog->getCurve(OCIO::SAT_SAT) ==
                      *hueCurveLog->getCurve(OCIO::LUM_LUM));
    OCIO_CHECK_ASSERT(*hueCurveLog->getCurve(OCIO::LUM_SAT) ==
                      *hueCurveLog->getCurve(OCIO::SAT_LUM));
    OCIO_CHECK_ASSERT(*hueCurveLog->getCurve(OCIO::HUE_HUE) !=
                      *hueCurveLog->getCurve(OCIO::HUE_SAT));
    OCIO_CHECK_EQUAL(3,    hueCurveLog->getCurve(OCIO::LUM_LUM)->getNumControlPoints());
    OCIO_CHECK_EQUAL(0.f,  hueCurveLog->getCurve(OCIO::LUM_LUM)->getControlPoint(0).m_x);
    OCIO_CHECK_EQUAL(0.f,  hueCurveLog->getCurve(OCIO::LUM_LUM)->getControlPoint(0).m_y);
    OCIO_CHECK_EQUAL(0.5f, hueCurveLog->getCurve(OCIO::LUM_LUM)->getControlPoint(1).m_x);
    OCIO_CHECK_EQUAL(0.5f, hueCurveLog->getCurve(OCIO::LUM_LUM)->getControlPoint(1).m_y);
    OCIO_CHECK_EQUAL(1.f,  hueCurveLog->getCurve(OCIO::LUM_LUM)->getControlPoint(2).m_x);
    OCIO_CHECK_EQUAL(1.f,  hueCurveLog->getCurve(OCIO::LUM_LUM)->getControlPoint(2).m_y);

    OCIO_CHECK_ASSERT(*hueCurveLin->getCurve(OCIO::HUE_LUM) ==
                      *hueCurveLin->getCurve(OCIO::HUE_SAT));
    OCIO_CHECK_ASSERT(*hueCurveLin->getCurve(OCIO::SAT_SAT) !=
                      *hueCurveLin->getCurve(OCIO::LUM_LUM));
    OCIO_CHECK_ASSERT(*hueCurveLin->getCurve(OCIO::LUM_SAT) !=
                      *hueCurveLin->getCurve(OCIO::SAT_LUM));
    OCIO_CHECK_ASSERT(*hueCurveLin->getCurve(OCIO::HUE_HUE) !=
                      *hueCurveLin->getCurve(OCIO::HUE_SAT));
    OCIO_CHECK_EQUAL(3,    hueCurveLin->getCurve(OCIO::LUM_LUM)->getNumControlPoints());
    OCIO_CHECK_EQUAL(-7.f, hueCurveLin->getCurve(OCIO::LUM_LUM)->getControlPoint(0).m_x);
    OCIO_CHECK_EQUAL(-7.f, hueCurveLin->getCurve(OCIO::LUM_LUM)->getControlPoint(0).m_y);
    OCIO_CHECK_EQUAL(0.f,  hueCurveLin->getCurve(OCIO::LUM_LUM)->getControlPoint(1).m_x);
    OCIO_CHECK_EQUAL(0.f,  hueCurveLin->getCurve(OCIO::LUM_LUM)->getControlPoint(1).m_y);
    OCIO_CHECK_EQUAL(7.f,  hueCurveLin->getCurve(OCIO::LUM_LUM)->getControlPoint(2).m_x);
    OCIO_CHECK_EQUAL(7.f,  hueCurveLin->getCurve(OCIO::LUM_LUM)->getControlPoint(2).m_y);

    OCIO_CHECK_ASSERT(!hueCurveLin->getDrawCurveOnly());
    hueCurveLin->setDrawCurveOnly(true);
    OCIO_CHECK_ASSERT(hueCurveLin->getDrawCurveOnly());

    // Validate that the other Create function made copies of its arguments.
    auto hueCurveLinCopy = OCIO::GradingHueCurve::Create(hueCurveLin);
    OCIO_REQUIRE_ASSERT(hueCurveLinCopy);
    OCIO_CHECK_ASSERT(hueCurveLin != hueCurveLinCopy);
    // Use overloaded op== to compare the contents of the curves.
    OCIO_CHECK_ASSERT(*hueCurveLin == *hueCurveLinCopy);

    OCIO_CHECK_ASSERT(hueCurveLinCopy->getDrawCurveOnly());

    // Test createEditableCopy.
    hueCurveLinCopy = hueCurveLin->createEditableCopy();
    OCIO_REQUIRE_ASSERT(hueCurveLinCopy);
    OCIO_CHECK_ASSERT(hueCurveLin != hueCurveLinCopy);
    OCIO_CHECK_ASSERT(*hueCurveLin == *hueCurveLinCopy);

    // Test op<<.
    std::ostringstream oss;
    oss << *hueCurveLin;
    OCIO_CHECK_EQUAL(std::string(
        "<hue_hue=<control_points=[<x=0, y=0><x=0.166667, y=0.166667>"
            "<x=0.333333, y=0.333333><x=0.5, y=0.5><x=0.666667, y=0.666667><x=0.833333, y=0.833333>]>, "
        "hue_sat=<control_points=[<x=0, y=1><x=0.166667, y=1><x=0.333333, y=1><x=0.5, y=1>"
            "<x=0.666667, y=1><x=0.833333, y=1>]>, "
        "hue_lum=<control_points=[<x=0, y=1><x=0.166667, y=1><x=0.333333, y=1><x=0.5, y=1>"
            "<x=0.666667, y=1><x=0.833333, y=1>]>, "
        "lum_sat=<control_points=[<x=-7, y=1><x=0, y=1><x=7, y=1>]>, "
        "sat_sat=<control_points=[<x=0, y=0><x=0.5, y=0.5><x=1, y=1>]>, "
        "lum_lum=<control_points=[<x=-7, y=-7><x=0, y=0><x=7, y=7>]>, "
        "sat_lum=<control_points=[<x=0, y=1><x=0.5, y=1><x=1, y=1>]>, "
        "hue_fx=<control_points=[<x=0, y=0><x=0.166667, y=0><x=0.333333, y=0><x=0.5, y=0>"
            "<x=0.666667, y=0><x=0.833333, y=0>]>>"),
        oss.str());
}

OCIO_ADD_TEST(GradingHueCurve, curves)
{
    auto curves = OCIO::GradingHueCurve::Create(OCIO::GRADING_VIDEO);
    OCIO_CHECK_ASSERT(curves->isIdentity());
    // Use non const curve accessor to modify one of the spline curves.
    OCIO::GradingBSplineCurveRcPtr spline = curves->getCurve(OCIO::HUE_SAT);
    OCIO_CHECK_EQUAL(curves->getCurve(OCIO::HUE_SAT)->getNumControlPoints(), 6);
    // For this spline type, all y values must be 1. to be an identity.
    OCIO_CHECK_ASSERT(curves->isIdentity());
    spline->getControlPoint(3).m_x = 0.9f;
    spline->getControlPoint(3).m_y = 1.1f;
    OCIO_CHECK_ASSERT(!curves->isIdentity());
    spline->getControlPoint(3).m_y = 1.f;
    OCIO_CHECK_ASSERT(curves->isIdentity());
    spline->setNumControlPoints(4);
    OCIO_CHECK_EQUAL(4, spline->getNumControlPoints());

    // Changing the pointer does not change the curves.
    auto newSpline = OCIO::GradingBSplineCurve::Create({ { 0.f,0.f },{ 1.f,2.f } });
    spline = newSpline;
    OCIO_CHECK_EQUAL(curves->getCurve(OCIO::HUE_SAT)->getNumControlPoints(), 4);
}

OCIO_ADD_TEST(GradingHueCurve, max_ctrl_pnts)
{
    auto hueCurve = OCIO::GradingHueCurve::Create(OCIO::GRADING_VIDEO);
    for (int c = 0; c < OCIO::HUE_NUM_CURVES; ++c)
    {
        // Use non const curve accessor to modify the curves.
        OCIO::GradingBSplineCurveRcPtr spline = hueCurve->getCurve(static_cast<OCIO::HueCurveType>(c));
        spline->setNumControlPoints(28);
    }

    OCIO::DynamicPropertyGradingHueCurveImplRcPtr res;
    OCIO_CHECK_THROW_WHAT(res = std::make_shared<OCIO::DynamicPropertyGradingHueCurveImpl>(hueCurve, false),
                         OCIO::Exception, "Hue curve: maximum number of control points reached");
}
