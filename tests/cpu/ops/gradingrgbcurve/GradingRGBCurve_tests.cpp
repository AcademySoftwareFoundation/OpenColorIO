// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "DynamicProperty.h"
#include "ops/gradingrgbcurve/GradingRGBCurve.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingRGBCurve, basic)
{
    auto curve = OCIO::GradingBSplineCurve::Create({ { 0.f,0.f },{ 0.2f,0.2f },
                                                     { 0.5f,0.7f },{ 1.f,1.f } });
    OCIO::ConstGradingBSplineCurveRcPtr curveR = curve;
    OCIO_CHECK_EQUAL(0.2f, curveR->getControlPoint(1).m_y);
    curve->getControlPoint(1).m_y = 0.3f;
    OCIO_CHECK_EQUAL(0.3f, curveR->getControlPoint(1).m_y);
    OCIO::ConstGradingBSplineCurveRcPtr curveG = OCIO::GradingBSplineCurve::Create(4);
    OCIO::ConstGradingBSplineCurveRcPtr curveB = OCIO::GradingBSplineCurve::Create(3);
    OCIO::ConstGradingBSplineCurveRcPtr curveM = OCIO::GradingBSplineCurve::Create(2);
    // The Create function takes 4 pointers to curves and creates new curves that are copies of
    // the 4 parameters.
    auto rgbCurve = OCIO::GradingRGBCurve::Create(curveR, curveG, curveB, curveM);
    OCIO_REQUIRE_ASSERT(rgbCurve);
    OCIO_REQUIRE_ASSERT(rgbCurve->getCurve(OCIO::RGB_RED));
    OCIO_REQUIRE_ASSERT(rgbCurve->getCurve(OCIO::RGB_GREEN));
    OCIO_REQUIRE_ASSERT(rgbCurve->getCurve(OCIO::RGB_BLUE));
    OCIO_REQUIRE_ASSERT(rgbCurve->getCurve(OCIO::RGB_MASTER));
    OCIO_CHECK_THROW_WHAT(rgbCurve->getCurve(OCIO::RGB_NUM_CURVES), OCIO::Exception,
                          "Invalid curve.");
    auto copiedCurve = rgbCurve->getCurve(OCIO::RGB_RED);
    OCIO_CHECK_EQUAL(0.3f, copiedCurve->getControlPoint(1).m_y);
    curve->getControlPoint(1).m_y = 0.4f;
    OCIO_CHECK_EQUAL(0.3f, copiedCurve->getControlPoint(1).m_y);

    // Test default curves.
    auto rgbCurveLin = OCIO::GradingRGBCurve::Create(OCIO::GRADING_LIN);
    auto rgbCurveLog = OCIO::GradingRGBCurve::Create(OCIO::GRADING_LOG);
    auto rgbCurveVideo = OCIO::GradingRGBCurve::Create(OCIO::GRADING_VIDEO);
    OCIO_REQUIRE_ASSERT(rgbCurveLin && rgbCurveLog && rgbCurveVideo);
    OCIO_CHECK_ASSERT(*rgbCurveLog == *rgbCurveVideo);
    OCIO_CHECK_ASSERT(*rgbCurveLog != *rgbCurveLin);
    OCIO_CHECK_ASSERT(*rgbCurveLog->getCurve(OCIO::RGB_RED) ==
                      *rgbCurveLog->getCurve(OCIO::RGB_GREEN));
    OCIO_CHECK_ASSERT(*rgbCurveLog->getCurve(OCIO::RGB_RED) ==
                      *rgbCurveLog->getCurve(OCIO::RGB_BLUE));
    OCIO_CHECK_ASSERT(*rgbCurveLog->getCurve(OCIO::RGB_RED) ==
                      *rgbCurveLog->getCurve(OCIO::RGB_MASTER));
    OCIO_CHECK_EQUAL(3, rgbCurveLog->getCurve(OCIO::RGB_RED)->getNumControlPoints());
    OCIO_CHECK_EQUAL(0.f,  rgbCurveLog->getCurve(OCIO::RGB_RED)->getControlPoint(0).m_x);
    OCIO_CHECK_EQUAL(0.f,  rgbCurveLog->getCurve(OCIO::RGB_RED)->getControlPoint(0).m_y);
    OCIO_CHECK_EQUAL(0.5f, rgbCurveLog->getCurve(OCIO::RGB_RED)->getControlPoint(1).m_x);
    OCIO_CHECK_EQUAL(0.5f, rgbCurveLog->getCurve(OCIO::RGB_RED)->getControlPoint(1).m_y);
    OCIO_CHECK_EQUAL(1.f,  rgbCurveLog->getCurve(OCIO::RGB_RED)->getControlPoint(2).m_x);
    OCIO_CHECK_EQUAL(1.f,  rgbCurveLog->getCurve(OCIO::RGB_RED)->getControlPoint(2).m_y);

    OCIO_CHECK_ASSERT(*rgbCurveLin->getCurve(OCIO::RGB_RED) ==
                      *rgbCurveLin->getCurve(OCIO::RGB_GREEN));
    OCIO_CHECK_ASSERT(*rgbCurveLin->getCurve(OCIO::RGB_RED) ==
                      *rgbCurveLin->getCurve(OCIO::RGB_BLUE));
    OCIO_CHECK_ASSERT(*rgbCurveLin->getCurve(OCIO::RGB_RED) ==
                      *rgbCurveLin->getCurve(OCIO::RGB_MASTER));
    OCIO_CHECK_EQUAL(3,    rgbCurveLin->getCurve(OCIO::RGB_RED)->getNumControlPoints());
    OCIO_CHECK_EQUAL(-7.f, rgbCurveLin->getCurve(OCIO::RGB_RED)->getControlPoint(0).m_x);
    OCIO_CHECK_EQUAL(-7.f, rgbCurveLin->getCurve(OCIO::RGB_RED)->getControlPoint(0).m_y);
    OCIO_CHECK_EQUAL(0.f,  rgbCurveLin->getCurve(OCIO::RGB_RED)->getControlPoint(1).m_x);
    OCIO_CHECK_EQUAL(0.f,  rgbCurveLin->getCurve(OCIO::RGB_RED)->getControlPoint(1).m_y);
    OCIO_CHECK_EQUAL(7.f,  rgbCurveLin->getCurve(OCIO::RGB_RED)->getControlPoint(2).m_x);
    OCIO_CHECK_EQUAL(7.f,  rgbCurveLin->getCurve(OCIO::RGB_RED)->getControlPoint(2).m_y);

    auto rgbCurveLinCopy = OCIO::GradingRGBCurve::Create(rgbCurveLin);
    OCIO_REQUIRE_ASSERT(rgbCurveLinCopy);
    OCIO_CHECK_ASSERT(*rgbCurveLin == *rgbCurveLinCopy);

    rgbCurveLinCopy = rgbCurveLin->createEditableCopy();
    OCIO_REQUIRE_ASSERT(rgbCurveLinCopy);
    OCIO_CHECK_ASSERT(*rgbCurveLin == *rgbCurveLinCopy);

    std::ostringstream oss;
    oss << *rgbCurveLin;
    OCIO_CHECK_EQUAL(std::string("<red=<control_points=[<x=-7, y=-7><x=0, y=0><x=7, y=7>]>, "
                                 "green=<control_points=[<x=-7, y=-7><x=0, y=0><x=7, y=7>]>, "
                                 "blue=<control_points=[<x=-7, y=-7><x=0, y=0><x=7, y=7>]>, "
                                 "master=<control_points=[<x=-7, y=-7><x=0, y=0><x=7, y=7>]>>"),
                     oss.str());
}

OCIO_ADD_TEST(GradingRGBCurve, curves)
{
    auto curves = OCIO::GradingRGBCurve::Create(OCIO::GRADING_VIDEO);
    OCIO_CHECK_ASSERT(curves->isIdentity());
    // Use non const curve accessor to modify one of the spline of the curves.
    OCIO::GradingBSplineCurveRcPtr spline = curves->getCurve(OCIO::RGB_GREEN);
    spline->setNumControlPoints(4);
    spline->getControlPoint(3).m_x = 1.1f;
    spline->getControlPoint(3).m_y = 2.f;
    OCIO_CHECK_ASSERT(!curves->isIdentity());
    spline->getControlPoint(3).m_x = 2.f;
    OCIO_CHECK_ASSERT(curves->isIdentity());
    OCIO_CHECK_EQUAL(curves->getCurve(OCIO::RGB_GREEN)->getNumControlPoints(), 4);

    // Changing the pointer does not change the curves.
    auto newSpline = OCIO::GradingBSplineCurve::Create({ { 0.f,0.f },{ 1.f,2.f } });
    spline = newSpline;
    OCIO_CHECK_EQUAL(curves->getCurve(OCIO::RGB_GREEN)->getNumControlPoints(), 4);
}

OCIO_ADD_TEST(GradingRGBCurve, max_ctrl_pnts)
{
    auto curveR = OCIO::GradingBSplineCurve::Create({ { 0.f, 10.f },{ 2.f, 10.f },{ 3.f, 10.f },{ 5.f, 10.f },{ 6.f, 10.f },
        { 8.f, 10.f },{ 9.f, 10.5f },{ 11.f, 15.f },{ 12.f, 50.f },{ 14.f, 60.f },{ 15.f, 85.f } });

    auto curveG = OCIO::GradingBSplineCurve::Create({ { 0.f, 10.f },{ 2.f, 10.f },{ 3.f, 10.f },{ 5.f, 10.f },{ 6.f, 10.f },
        { 8.f, 10.f },{ 9.f, 10.5f },{ 11.f, 15.f },{ 12.f, 50.f },{ 14.f, 60.f },{ 15.f, 85.f } });

    auto curveB = OCIO::GradingBSplineCurve::Create({ { 0.f, 10.f },{ 2.f, 10.f },{ 3.f, 10.f },{ 5.f, 10.f },{ 6.f, 10.f },
        { 8.f, 10.f },{ 9.f, 10.5f },{ 11.f, 15.f },{ 12.f, 50.f },{ 14.f, 60.f },{ 15.f, 85.f } });

    auto curveM = OCIO::GradingBSplineCurve::Create({ { 0.f, 10.f },{ 2.f, 10.f },{ 3.f, 10.f },{ 5.f, 10.f },{ 6.f, 10.f },
        { 8.f, 10.f },{ 9.f, 10.5f },{ 11.f, 15.f },{ 12.f, 50.f },{ 14.f, 60.f },{ 15.f, 85.f } });

    auto rgbCurve = OCIO::GradingRGBCurve::Create(curveR, curveG, curveB, curveM);
    OCIO_REQUIRE_ASSERT(rgbCurve);

    OCIO::DynamicPropertyGradingRGBCurveImplRcPtr res;
    OCIO_CHECK_THROW_WHAT(res = std::make_shared<OCIO::DynamicPropertyGradingRGBCurveImpl>(rgbCurve, false),
                          OCIO::Exception, "RGB curve: maximum number of control points reached");
}
