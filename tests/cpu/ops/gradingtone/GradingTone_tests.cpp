// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradingtone/GradingTone.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingTone, basic)
{
    const OCIO::GradingRGBMSW rgbm0;
    OCIO_CHECK_EQUAL(rgbm0.m_red, 1.);
    OCIO_CHECK_EQUAL(rgbm0.m_green, 1.);
    OCIO_CHECK_EQUAL(rgbm0.m_blue, 1.);
    OCIO_CHECK_EQUAL(rgbm0.m_master, 1.);
    OCIO_CHECK_EQUAL(rgbm0.m_start, 0.);
    OCIO_CHECK_EQUAL(rgbm0.m_width, 1.);

    const OCIO::GradingRGBMSW rgbm1{ 1., 2., 3., 4., 5., 6. };
    OCIO_CHECK_EQUAL(rgbm1.m_red, 1.);
    OCIO_CHECK_EQUAL(rgbm1.m_green, 2.);
    OCIO_CHECK_EQUAL(rgbm1.m_blue, 3.);
    OCIO_CHECK_EQUAL(rgbm1.m_master, 4.);
    OCIO_CHECK_EQUAL(rgbm1.m_start, 5.);
    OCIO_CHECK_EQUAL(rgbm1.m_width, 6.);

    OCIO::GradingRGBMSW rgbm2{ rgbm1 };
    OCIO_CHECK_EQUAL(rgbm2.m_red, 1.);
    OCIO_CHECK_EQUAL(rgbm2.m_green, 2.);
    OCIO_CHECK_EQUAL(rgbm2.m_blue, 3.);
    OCIO_CHECK_EQUAL(rgbm2.m_master, 4.);
    OCIO_CHECK_EQUAL(rgbm2.m_start, 5.);
    OCIO_CHECK_EQUAL(rgbm2.m_width, 6.);

    // Check operator==().
    OCIO_CHECK_EQUAL(rgbm1, rgbm2);
    rgbm2.m_red += 0.1111;
    OCIO_CHECK_NE(rgbm1, rgbm2);

    // Check defaults.
    const OCIO::GradingTone toneLog{ OCIO::GRADING_LOG };
    OCIO_CHECK_EQUAL(toneLog.m_blacks, OCIO::GradingRGBMSW(1., 1., 1., 1., 0.4, 0.4));
    OCIO_CHECK_EQUAL(toneLog.m_shadows, OCIO::GradingRGBMSW(1., 1., 1., 1., 0.5, 0.));
    OCIO_CHECK_EQUAL(toneLog.m_midtones, OCIO::GradingRGBMSW(1., 1., 1., 1., 0.4, 0.6));
    OCIO_CHECK_EQUAL(toneLog.m_highlights, OCIO::GradingRGBMSW(1., 1., 1., 1., 0.3, 1.));
    OCIO_CHECK_EQUAL(toneLog.m_whites, OCIO::GradingRGBMSW(1., 1., 1., 1., 0.4, 0.5));
    OCIO_CHECK_EQUAL(toneLog.m_scontrast, 1.);
    const OCIO::GradingTone toneLin{ OCIO::GRADING_LIN };
    OCIO_CHECK_EQUAL(toneLin.m_blacks, OCIO::GradingRGBMSW(1., 1., 1., 1., 0., 4.));
    OCIO_CHECK_EQUAL(toneLin.m_shadows, OCIO::GradingRGBMSW(1., 1., 1., 1., 2., -7.));
    OCIO_CHECK_EQUAL(toneLin.m_midtones, OCIO::GradingRGBMSW(1., 1., 1., 1., 0., 8.));
    OCIO_CHECK_EQUAL(toneLin.m_highlights, OCIO::GradingRGBMSW(1., 1., 1., 1., -2., 9.));
    OCIO_CHECK_EQUAL(toneLin.m_whites, OCIO::GradingRGBMSW(1., 1., 1., 1., 0., 8.));
    OCIO_CHECK_EQUAL(toneLin.m_scontrast, 1.);
    const OCIO::GradingTone toneVid{ OCIO::GRADING_VIDEO };
    OCIO_CHECK_EQUAL(toneVid.m_blacks, OCIO::GradingRGBMSW(1., 1., 1., 1., 0.4, 0.4));
    OCIO_CHECK_EQUAL(toneVid.m_shadows, OCIO::GradingRGBMSW(1., 1., 1., 1., 0.6, 0.));
    OCIO_CHECK_EQUAL(toneVid.m_midtones, OCIO::GradingRGBMSW(1., 1., 1., 1., 0.4, 0.7));
    OCIO_CHECK_EQUAL(toneVid.m_highlights, OCIO::GradingRGBMSW(1., 1., 1., 1., 0.2, 1.));
    OCIO_CHECK_EQUAL(toneVid.m_whites, OCIO::GradingRGBMSW(1., 1., 1., 1., 0.5, 0.5));
    OCIO_CHECK_EQUAL(toneVid.m_scontrast, 1.);

    OCIO::GradingTone gt1{ OCIO::GRADING_LOG };
    gt1.m_midtones.m_start = 0.1;

    const OCIO::GradingTone gt2{ gt1 };
    // Check operator==().
    OCIO_CHECK_EQUAL(gt1, gt2);
    gt1.m_highlights.m_red += 0.1111;
    OCIO_CHECK_NE(gt1, gt2);
}

OCIO_ADD_TEST(GradingTone, rgbmsw_channel)
{
    const OCIO::GradingRGBMSW rgbm1{ 1., 2., 3., 4., 5., 6. };
    OCIO_CHECK_EQUAL(OCIO::GetChannelValue(rgbm1, OCIO::R), 1.);
    OCIO_CHECK_EQUAL(OCIO::GetChannelValue(rgbm1, OCIO::G), 2.);
    OCIO_CHECK_EQUAL(OCIO::GetChannelValue(rgbm1, OCIO::B), 3.);
    OCIO_CHECK_EQUAL(OCIO::GetChannelValue(rgbm1, OCIO::M), 4.);
}

OCIO_ADD_TEST(GradingTone, validate)
{
    OCIO::GradingTone tone{ OCIO::GRADING_LOG };
    OCIO_CHECK_NO_THROW(tone.validate());

    auto temp = tone.m_blacks.m_red;
    tone.m_blacks.m_red = 0.08;
    OCIO_CHECK_THROW_WHAT(tone.validate(), OCIO::Exception, "are below lower bound");
    tone.m_blacks.m_red = temp;

    temp = tone.m_midtones.m_width;
    tone.m_midtones.m_width = 0.001;
    OCIO_CHECK_THROW_WHAT(tone.validate(), OCIO::Exception, "is below lower bound");
    tone.m_midtones.m_width = temp;

    temp = tone.m_whites.m_blue;
    tone.m_whites.m_blue = 2.;
    OCIO_CHECK_THROW_WHAT(tone.validate(), OCIO::Exception, "are above upper bound");
    tone.m_whites.m_blue = temp;

    temp = tone.m_shadows.m_master;
    tone.m_shadows.m_master = 0.15;
    OCIO_CHECK_THROW_WHAT(tone.validate(), OCIO::Exception, "are below lower bound");
    tone.m_shadows.m_master = temp;

    temp = tone.m_highlights.m_green;
    tone.m_highlights.m_green = 1.9;
    OCIO_CHECK_THROW_WHAT(tone.validate(), OCIO::Exception, "are above upper bound");
    tone.m_highlights.m_green = temp;

    temp = tone.m_scontrast;
    tone.m_scontrast = 2.;
    OCIO_CHECK_THROW_WHAT(tone.validate(), OCIO::Exception, "is above upper bound");
    tone.m_scontrast = temp;
}
