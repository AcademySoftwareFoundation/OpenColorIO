// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradingprimary/GradingPrimary.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingPrimary, basic)
{
    const OCIO::GradingRGBM rgbm0;
    OCIO_CHECK_EQUAL(rgbm0.m_red, 0);
    OCIO_CHECK_EQUAL(rgbm0.m_green, 0);
    OCIO_CHECK_EQUAL(rgbm0.m_blue, 0);
    OCIO_CHECK_EQUAL(rgbm0.m_master, 0);

    const OCIO::GradingRGBM rgbm1{ 1., 2., 3., 4. };
    OCIO_CHECK_EQUAL(rgbm1.m_red, 1.);
    OCIO_CHECK_EQUAL(rgbm1.m_green, 2.);
    OCIO_CHECK_EQUAL(rgbm1.m_blue, 3.);
    OCIO_CHECK_EQUAL(rgbm1.m_master, 4.);

    OCIO::GradingRGBM rgbm2{ rgbm1 };
    OCIO_CHECK_EQUAL(rgbm2.m_red, 1.);
    OCIO_CHECK_EQUAL(rgbm2.m_green, 2.);
    OCIO_CHECK_EQUAL(rgbm2.m_blue, 3.);
    OCIO_CHECK_EQUAL(rgbm2.m_master, 4.);

    // Check operator==().
    OCIO_CHECK_EQUAL(rgbm1, rgbm2);
    rgbm2.m_red += 0.1111;
    OCIO_CHECK_NE(rgbm1, rgbm2);

    const OCIO::GradingPrimary gpLog{ OCIO::GRADING_LOG };
    const OCIO::GradingPrimary gpLin{ OCIO::GRADING_LIN };
    const OCIO::GradingPrimary gpVid{ OCIO::GRADING_VIDEO };

    // Check operator==(). (The default values for linear and video are the same.  The style is
    // not part of the GradingPrimary struct.)
    OCIO_CHECK_EQUAL(gpLin, gpVid);
    OCIO_CHECK_NE(gpLog, gpLin);
}

OCIO_ADD_TEST(GradingPrimary, validate)
{
    OCIO::GradingPrimary gp{ OCIO::GRADING_LOG };
    OCIO_CHECK_NO_THROW(gp.validate(OCIO::GRADING_LOG));

    // LOG & VIDEO have to keep gamma above a threshold.
    gp.m_gamma.m_red = 0.0001;
    OCIO_CHECK_THROW_WHAT(gp.validate(OCIO::GRADING_LOG),
                          OCIO::Exception, "GradingPrimary gamma '<r=0.0001, g=1, b=1, m=1>' "
                                           "are below lower bound (0.01)");

    OCIO_CHECK_THROW_WHAT(gp.validate(OCIO::GRADING_VIDEO),
                          OCIO::Exception, "GradingPrimary gamma '<r=0.0001, g=1, b=1, m=1>' "
                                           "are below lower bound (0.01)");

    // LIN does not use gamma.
    OCIO_CHECK_NO_THROW(gp.validate(OCIO::GRADING_LIN));

    // Restore gamma.
    gp.m_gamma.m_red = 1.;

    // LIN has to keep contrast above a threshold.
    gp.m_contrast.m_green = 0.0001;
    OCIO_CHECK_THROW_WHAT(gp.validate(OCIO::GRADING_LIN),
                          OCIO::Exception, "GradingPrimary contrast '<r=1, g=0.0001, b=1, m=1>' "
                                           "are below lower bound (0.01)");

    // LOG can use any contrast value and VIDEO does not use contrast.
    OCIO_CHECK_NO_THROW(gp.validate(OCIO::GRADING_LOG));
    OCIO_CHECK_NO_THROW(gp.validate(OCIO::GRADING_VIDEO));

    // Restore contrast.
    gp.m_contrast.m_green = 1.;
}

OCIO_ADD_TEST(GradingPrimary, precompute)
{
    OCIO::GradingPrimary gp{ OCIO::GRADING_LOG };

    OCIO::GradingPrimaryPreRender comp;
    comp.update(OCIO::GRADING_LOG, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(comp.getBrightness() == OCIO::Float3({ 0.f, 0.f, 0.f }));
    OCIO_CHECK_ASSERT(comp.getContrast() == OCIO::Float3({ 1.f, 1.f, 1.f }));
    OCIO_CHECK_ASSERT(comp.getGamma() == OCIO::Float3({ 1.f, 1.f, 1.f }));
    OCIO_CHECK_CLOSE(comp.getPivot(), 0.4f, 1.e-6f);
    OCIO_CHECK_ASSERT(comp.getLocalBypass());
    OCIO_CHECK_ASSERT(comp.isGammaIdentity());

    gp.m_saturation = 0.5;
    comp.update(OCIO::GRADING_LOG, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(!comp.getLocalBypass());
    gp.m_saturation = 1.;
    comp.update(OCIO::GRADING_LOG, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(comp.getLocalBypass());

    gp.m_brightness.m_green = 0.1 * 1023. / 6.25;
    comp.update(OCIO::GRADING_LOG, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(comp.getBrightness() == OCIO::Float3({ 0.f, 0.1f, 0.f }));
    OCIO_CHECK_ASSERT(!comp.getLocalBypass());
    OCIO_CHECK_ASSERT(comp.isGammaIdentity());

    gp.m_brightness.m_red = 0.1 * 1023. / 6.25;
    gp.m_brightness.m_green = 0.;
    gp.m_contrast.m_red = 0.; // Inverse will be 1.
    gp.m_contrast.m_green = 1.25;
    gp.m_gamma.m_blue = 0.8;
    gp.m_pivot = 1.;
    comp.update(OCIO::GRADING_LOG, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(comp.getBrightness() == OCIO::Float3({ 0.1f, 0.f, 0.f }));
    OCIO_CHECK_ASSERT(comp.getContrast() == OCIO::Float3({ 0.f, 1.25f, 1.f }));
    OCIO_CHECK_ASSERT(comp.getGamma() == OCIO::Float3({ 1.f, 1.f, 1.25f }));
    OCIO_CHECK_CLOSE(comp.getPivot(), 1.f, 1.e-6f);
    OCIO_CHECK_ASSERT(!comp.isGammaIdentity());

    comp.update(OCIO::GRADING_LOG, OCIO::TRANSFORM_DIR_INVERSE, gp);
    OCIO_CHECK_ASSERT(comp.getBrightness() == OCIO::Float3({ -0.1f, 0.f, 0.f }));
    OCIO_CHECK_ASSERT(comp.getContrast() == OCIO::Float3({ 1.f, 0.8f, 1.f }));
    OCIO_CHECK_ASSERT(comp.getGamma() == OCIO::Float3({ 1.f, 1.f, 0.8f }));

    gp = OCIO::GradingPrimary{ OCIO::GRADING_LOG };

    // Test identity checks for GRADING_LOG
    gp.m_gamma.m_red = 0.8;
    comp.update(OCIO::GRADING_LOG, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(!comp.isGammaIdentity());
    OCIO_CHECK_ASSERT(!comp.getLocalBypass());
    gp.m_gamma.m_red = 1.0;
    comp.update(OCIO::GRADING_LOG, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(comp.isGammaIdentity());
    OCIO_CHECK_ASSERT(comp.getLocalBypass());

    // Test identity checks for GRADING_LIN
    gp.m_contrast.m_red = 0.8;
    comp.update(OCIO::GRADING_LIN, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(!comp.isContrastIdentity());
    OCIO_CHECK_ASSERT(!comp.getLocalBypass());
    gp.m_contrast.m_red = 1.0;
    comp.update(OCIO::GRADING_LIN, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(comp.isContrastIdentity());
    OCIO_CHECK_ASSERT(comp.getLocalBypass());

    // Test identity checks for GRADING_VIDEO
    gp.m_gamma.m_red = 0.8;
    comp.update(OCIO::GRADING_VIDEO, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(!comp.isGammaIdentity());
    OCIO_CHECK_ASSERT(!comp.getLocalBypass());
    gp.m_gamma.m_red = 1.0;
    comp.update(OCIO::GRADING_VIDEO, OCIO::TRANSFORM_DIR_FORWARD, gp);
    OCIO_CHECK_ASSERT(comp.isGammaIdentity());
    OCIO_CHECK_ASSERT(comp.getLocalBypass());
}
