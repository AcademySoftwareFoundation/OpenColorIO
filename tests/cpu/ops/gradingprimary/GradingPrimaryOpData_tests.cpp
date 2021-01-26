// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradingprimary/GradingPrimaryOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingPrimaryOpData, accessors)
{
    // Create GradingPrimaryOpData and check values. More tests are done using
    // GradingPrimaryTransform.
    OCIO::GradingPrimaryOpData gp{ OCIO::GRADING_LIN };

    OCIO_CHECK_EQUAL(gp.getStyle(), OCIO::GRADING_LIN);
    const OCIO::GradingPrimary gdpLin(OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gp.getValue(), gdpLin);
    OCIO_CHECK_EQUAL(gp.getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    gp.setStyle(OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gp.getStyle(), OCIO::GRADING_LOG);
    const OCIO::GradingPrimary gdpLog(OCIO::GRADING_LOG);
    gp.setValue(gdpLog);
    OCIO_CHECK_EQUAL(gp.getValue(), gdpLog);
    gp.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(gp.getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_EQUAL(gp.getType(), OCIO::OpData::GradingPrimaryType);
    OCIO_CHECK_EQUAL(gp.isNoOp(), true);
    OCIO_CHECK_EQUAL(gp.isIdentity(), true);
    OCIO_CHECK_EQUAL(gp.hasChannelCrosstalk(), false);

    const std::string expected{ "log inverse <brightness=<r=0, g=0, b=0, m=0>, "
                                "contrast=<r=1, g=1, b=1, m=1>, gamma=<r=1, g=1, b=1, m=1>, "
                                "offset=<r=0, g=0, b=0, m=0>, exposure=<r=0, g=0, b=0, m=0>, "
                                "lift=<r=0, g=0, b=0, m=0>, gain=<r=1, g=1, b=1, m=1>, "
                                "saturation=1, pivot=<contrast=-0.2, black=0, white=1>>" };
    OCIO_CHECK_EQUAL(gp.getCacheID(), expected);

    // Test operator==.
    OCIO::GradingPrimaryOpData gp1{ OCIO::GRADING_LIN };
    OCIO::GradingPrimaryOpData gp2{ OCIO::GRADING_LIN };

    OCIO_CHECK_ASSERT(gp1 == gp2);
    gp1.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(!(gp1 == gp2));
    gp2.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(gp1 == gp2);

    gp1.setStyle(OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(!(gp1 == gp2));
    gp2.setStyle(OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(gp1 == gp2);

    auto v1 = gp1.getValue();
    v1.m_brightness.m_red += 0.1;
    gp1.setValue(v1);
    OCIO_CHECK_ASSERT(!(gp1 == gp2));
    auto v2 = gp2.getValue();
    v2.m_brightness.m_red += 0.1;
    gp2.setValue(v2);
    OCIO_CHECK_ASSERT(gp1 == gp2);

    v1.m_pivotBlack += 0.1;
    gp1.setValue(v1);
    OCIO_CHECK_ASSERT(!(gp1 == gp2));
    v2.m_pivotBlack += 0.1;
    gp2.setValue(v2);
    OCIO_CHECK_ASSERT(gp1 == gp2);

    // IsIdentity.

    OCIO_CHECK_ASSERT(!gp1.isIdentity());

    OCIO::GradingPrimaryOpData gp3{ OCIO::GRADING_LIN };
    OCIO_CHECK_ASSERT(gp3.isIdentity());

    auto v3 = gp3.getValue();
    v3.m_clampBlack = 0.5;
    gp3.setValue(v3);
    OCIO_CHECK_ASSERT(!gp3.isIdentity());

    // Channel crosstalk.

    OCIO_CHECK_ASSERT(!gp3.hasChannelCrosstalk());
    v3.m_saturation = 0.5;
    gp3.setValue(v3);
    OCIO_CHECK_ASSERT(gp3.hasChannelCrosstalk());

    // Check isInverse.

    // We have equal ops, inverse one.
    gp1.setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    // Need a shared pointer for the parameter.
    OCIO::ConstGradingPrimaryOpDataRcPtr gpptr2 = gp1.clone();
    gp1.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(gp1.isInverse(gpptr2));
    // Change value of one: no longer inverse.
    v1.m_pivotBlack += 0.1;
    gp1.setValue(v1);
    OCIO_CHECK_ASSERT(!gp1.isInverse(gpptr2));
    // Restore value.
    v1.m_pivotBlack -= 0.1;
    gp1.setValue(v1);
    OCIO_CHECK_ASSERT(gp1.isInverse(gpptr2));
    // Change direction: no longer inverse.
    gp1.setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(!gp1.isInverse(gpptr2));
}

OCIO_ADD_TEST(GradingPrimaryOpData, validate)
{
    OCIO::GradingPrimaryOpData gp{ OCIO::GRADING_LOG };
    OCIO_CHECK_NO_THROW(gp.validate());

    auto v = gp.getValue();

    // Test invalid gamma.
    v.m_gamma.m_red = 0.0001;
    OCIO_CHECK_THROW_WHAT(gp.setValue(v), OCIO::Exception,
                          "GradingPrimary gamma '<r=0.0001, g=1, b=1, m=1>' "
                          "are below lower bound (0.01)");

    v.m_gamma.m_red = 1.;
    v.m_gamma.m_green = 0.0001;
    OCIO_CHECK_THROW_WHAT(gp.setValue(v), OCIO::Exception,
                          "are below lower bound (0.01)");

    v.m_gamma.m_green = 1.;
    v.m_gamma.m_blue = 0.0001;
    OCIO_CHECK_THROW_WHAT(gp.setValue(v), OCIO::Exception,
                          "are below lower bound (0.01)");

    v.m_gamma.m_blue = 1.;
    v.m_gamma.m_master = 0.0001;
    OCIO_CHECK_THROW_WHAT(gp.setValue(v), OCIO::Exception,
                          "are below lower bound (0.01)");

    v.m_gamma.m_master = 1.;
    OCIO_CHECK_NO_THROW(gp.setValue(v));
    OCIO_CHECK_NO_THROW(gp.validate());

    // Test invalid pivot.
    v.m_pivotBlack = 0.5;
    v.m_pivotWhite = 0.4;
    OCIO_CHECK_THROW_WHAT(gp.setValue(v), OCIO::Exception,
                          "black pivot should be smaller than white pivot");

    v.m_pivotBlack = 0.;
    OCIO_CHECK_NO_THROW(gp.setValue(v));
    OCIO_CHECK_NO_THROW(gp.validate());

    // Test invalid clamp.
    v.m_clampBlack = 0.5;
    v.m_clampWhite = 0.4;
    OCIO_CHECK_THROW_WHAT(gp.setValue(v), OCIO::Exception,
                          "black clamp should be smaller than white clamp");

    v.m_clampBlack = 0.;
    OCIO_CHECK_NO_THROW(gp.setValue(v));
    OCIO_CHECK_NO_THROW(gp.validate());
}

OCIO_ADD_TEST(GradingPrimaryOpData, dynamic)
{
    // Create GradingPrimaryOpData and check values. More tests are done using
    // GradingPrimaryTransform.
    OCIO::GradingPrimaryOpData gp{ OCIO::GRADING_LIN };

    OCIO_CHECK_ASSERT(!gp.isDynamic());
    OCIO::DynamicPropertyRcPtr dp;
    OCIO_CHECK_NO_THROW(dp = gp.getDynamicProperty());
    OCIO_REQUIRE_ASSERT(dp);

    auto dgp = gp.getDynamicPropertyInternal();
    OCIO_REQUIRE_ASSERT(dgp);
    OCIO_CHECK_ASSERT(!dgp->isDynamic());
    dgp->makeDynamic();

    OCIO_CHECK_ASSERT(gp.isDynamic());
    OCIO_CHECK_EQUAL(dp->getType(), OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY);

    OCIO::GradingPrimary gdp{ OCIO::GRADING_LIN };
    gdp.m_pivotBlack = 0.01;
    OCIO_CHECK_NO_THROW(dgp->setValue(gdp));

    OCIO_CHECK_EQUAL(gp.getValue().m_pivotBlack, 0.01);

    dgp->makeNonDynamic();

    OCIO_CHECK_ASSERT(!gp.isDynamic());
}