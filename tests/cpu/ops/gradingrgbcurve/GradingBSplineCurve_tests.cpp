// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradingrgbcurve/GradingBSplineCurve.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

//
// Please see DynamicProperty_tests.cpp for tests of the actual spline fitting.
//

OCIO_ADD_TEST(GradingBSplineCurve, basic)
{
    auto curve = OCIO::GradingBSplineCurve::Create(3);
    OCIO_REQUIRE_ASSERT(curve);
    OCIO_CHECK_EQUAL(3, curve->getNumControlPoints());
    OCIO_CHECK_EQUAL(0.f, curve->getControlPoint(0).m_x);
    OCIO_CHECK_EQUAL(0.f, curve->getControlPoint(0).m_y);

    curve->getControlPoint(1).m_x = 0.5f;
    curve->getControlPoint(1).m_y = 0.4f;
    curve->getControlPoint(2).m_x = 1.0f;
    curve->getControlPoint(2).m_y = 0.9f;
    OCIO_CHECK_EQUAL(0.5f, curve->getControlPoint(1).m_x);
    OCIO_CHECK_EQUAL(0.4f, curve->getControlPoint(1).m_y);
    OCIO_CHECK_EQUAL(1.0f, curve->getControlPoint(2).m_x);
    OCIO_CHECK_EQUAL(0.9f, curve->getControlPoint(2).m_y);

    OCIO_REQUIRE_ASSERT(curve->slopesAreDefault());
    curve->setSlope(2, 0.9f);
    OCIO_CHECK_EQUAL(0.9f, curve->getSlope(2));
    OCIO_REQUIRE_ASSERT(!curve->slopesAreDefault());
    
    curve->setNumControlPoints(4);
    OCIO_CHECK_EQUAL(4, curve->getNumControlPoints());
    OCIO_CHECK_EQUAL(0.f, curve->getControlPoint(3).m_x);
    OCIO_CHECK_EQUAL(0.f, curve->getControlPoint(3).m_y);

    curve = OCIO::GradingBSplineCurve::Create({ {0.f,0.f},{0.2f,0.3f},{0.5f,0.7f},{1.f,1.f} });
    OCIO_REQUIRE_ASSERT(curve);
    OCIO_CHECK_EQUAL(4, curve->getNumControlPoints());
    OCIO_CHECK_EQUAL(0.f, curve->getControlPoint(0).m_x);
    OCIO_CHECK_EQUAL(0.f, curve->getControlPoint(0).m_y);
    OCIO_CHECK_EQUAL(0.2f, curve->getControlPoint(1).m_x);
    OCIO_CHECK_EQUAL(0.3f, curve->getControlPoint(1).m_y);
    OCIO_CHECK_EQUAL(0.5f, curve->getControlPoint(2).m_x);
    OCIO_CHECK_EQUAL(0.7f, curve->getControlPoint(2).m_y);
    OCIO_CHECK_EQUAL(1.f, curve->getControlPoint(3).m_x);
    OCIO_CHECK_EQUAL(1.f, curve->getControlPoint(3).m_y);

    OCIO_CHECK_THROW_WHAT(curve->getControlPoint(42), OCIO::Exception,
                          "There are '4' control points. '42' is out of bounds.");
    OCIO_CHECK_THROW_WHAT(curve->setSlope(42, 0.2f), OCIO::Exception,
                          "There are '4' control points. '42' is out of bounds.");

    std::ostringstream oss;
    oss << *curve;
    OCIO_CHECK_EQUAL(std::string("<control_points=[<x=0, y=0><x=0.2, y=0.3>"
                                 "<x=0.5, y=0.7><x=1, y=1>]>"),
                     oss.str());
}

OCIO_ADD_TEST(GradingBSplineCurve, validate)
{
    auto curve = OCIO::GradingBSplineCurve::Create(1);
    OCIO_CHECK_THROW_WHAT(curve->validate(), OCIO::Exception,
                          "There must be at least 2 control points.");
    curve = OCIO::GradingBSplineCurve::Create({ { 0.f,0.f },{ 0.7f,0.3f },
                                                { 0.5f,0.7f },{ 1.f,1.f } });
    OCIO_CHECK_THROW_WHAT(curve->validate(), OCIO::Exception,
                          "has a x coordinate '0.5' that is less than previous control "
                          "point x coordinate '0.7'.");

    curve->getControlPoint(1).m_x = 0.3f;
    OCIO_CHECK_NO_THROW(curve->validate());
}

OCIO_ADD_TEST(GradingBSplineCurve, equals)
{
    // Identical curves.
    auto curve1 = OCIO::GradingBSplineCurve::Create({ {0.f,0.f},{0.2f,0.3f},{0.5f,0.7f},{1.f,1.f} });
    auto curve2 = OCIO::GradingBSplineCurve::Create({ {0.f,0.f},{0.2f,0.3f},{0.5f,0.7f},{1.f,1.f} });
    OCIO_CHECK_ASSERT(*curve1 == *curve2);

    // Curve has different spline type.
    auto curve3 = OCIO::GradingBSplineCurve::Create({ {0.f,0.f},{0.2f,0.3f},{0.5f,0.7f},{1.f,1.f} }, 
        OCIO::DIAGONAL_B_SPLINE);
    OCIO_CHECK_ASSERT(!(*curve1 == *curve3));

    // Curve has different slopes.
    curve2->setSlope(3, 0.9f);
    OCIO_CHECK_NO_THROW(curve2->validate());
    OCIO_CHECK_ASSERT(!(*curve1 == *curve2));

    // Curve has a different control point value.
    auto curve4 = OCIO::GradingBSplineCurve::Create({ {0.f,0.f},{0.2f,0.3f},{0.5f,0.7f},{1.f,1.f} });
    OCIO_CHECK_ASSERT(*curve1 == *curve4);
    curve4->getControlPoint(2).m_y = 0.9f;
    OCIO_CHECK_ASSERT(!(*curve1 == *curve4));
}
