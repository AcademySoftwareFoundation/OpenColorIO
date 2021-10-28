// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// NOTE:
// The file is a copy and paste from the corresponding GPU unitest i.e. []/tests/gpu/GradingToneOp_test.cpp.
// Keep the two files in sync. to increase the test coverage.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


namespace GTTest1
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
// These are {R, G, B, master, center, width};
static const OCIO::GradingRGBMSW midtones{ 0.3, 1.0, 1.8,  1.2, 0.47, 0.6 };
}

void GradingToneLogMidtones(OSLDataRcPtr & data, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest1::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest1::style);
    tone.m_midtones = GTTest1::midtones;
    gt->setValue(tone);

    data->m_transform = gt;

    data->m_threshold = 2e-5f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingTone, style_log_midtones_fwd)
{
    GradingToneLogMidtones(m_data, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_OSL_TEST(GradingTone, style_log_midtones_fwd_dynamic)
{
    GradingToneLogMidtones(m_data, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_OSL_TEST(GradingTone, style_log_midtones_rev)
{
    GradingToneLogMidtones(m_data, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_OSL_TEST(GradingTone, style_log_midtones_rev_dynamic)
{
    GradingToneLogMidtones(m_data, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GTTest2
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
// These are {R, G, B, master, start, pivot};
static const OCIO::GradingRGBMSW highlights{ 0.3, 1.0, 1.8, 1.4, -0.1, 0.9 };
}

void GradingToneLogHighlights(OSLDataRcPtr & data, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest2::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest2::style);
    tone.m_highlights = GTTest2::highlights;
    gt->setValue(tone);

    data->m_transform = gt;

    data->m_threshold = 2e-5f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingTone, style_log_highlights_fwd)
{
    GradingToneLogHighlights(m_data, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_OSL_TEST(GradingTone, style_log_highlights_fwd_dynamic)
{
    GradingToneLogHighlights(m_data, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_OSL_TEST(GradingTone, style_log_highlights_rev)
{
    GradingToneLogHighlights(m_data, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_OSL_TEST(GradingTone, style_log_highlights_rev_dynamic)
{
    GradingToneLogHighlights(m_data, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GTTest3
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_VIDEO;
// These are {R, G, B, master, start, pivot};
static const OCIO::GradingRGBMSW shadows{ 0.3, 1., 1.79, 0.6, 0.8, -0.1 };
}

void GradingToneVideoShadows(OSLDataRcPtr & data, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest3::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest3::style);
    tone.m_shadows = GTTest3::shadows;
    gt->setValue(tone);

    data->m_transform = gt;

    data->m_threshold = 3e-5f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingTone, style_video_shadows_fwd)
{
    GradingToneVideoShadows(m_data, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_OSL_TEST(GradingTone, style_video_shadows_fwd_dynamic)
{
    GradingToneVideoShadows(m_data, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_OSL_TEST(GradingTone, style_video_shadows_rev)
{
    GradingToneVideoShadows(m_data, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_OSL_TEST(GradingTone, style_video_shadows_rev_dynamic)
{
    GradingToneVideoShadows(m_data, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GTTest4
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_VIDEO;
// These are {R, G, B, master, start, width};
static const OCIO::GradingRGBMSW whites{ 0.3, 1., 1.9, 0.6, -0.2, 1.4 };
}

void GradingToneVideoWhites(OSLDataRcPtr & data, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest4::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest4::style);
    tone.m_whites = GTTest4::whites;
    gt->setValue(tone);

    data->m_transform = gt;

    data->m_threshold = 3e-5f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingTone, style_video_white_detail_fwd)
{
    GradingToneVideoWhites(m_data, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_OSL_TEST(GradingTone, style_video_white_detail_fwd_dynamic)
{
    GradingToneVideoWhites(m_data, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_OSL_TEST(GradingTone, style_video_white_detail_rev)
{
    GradingToneVideoWhites(m_data, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_OSL_TEST(GradingTone, style_video_white_detail_rev_dynamic)
{
    GradingToneVideoWhites(m_data, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GTTest5
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
// These are {R, G, B, master, start, width};
static const OCIO::GradingRGBMSW blacks{ 0.3, 1., 1.9, 0.6, 0.8, 0.9 };
}

void GradingToneLogBlacks(OSLDataRcPtr & data, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest5::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest5::style);
    tone.m_blacks = GTTest5::blacks;
    gt->setValue(tone);

    data->m_transform = gt;

    data->m_threshold = 3e-5f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;}

OCIO_OSL_TEST(GradingTone, style_log_black_detail_fwd)
{
    GradingToneLogBlacks(m_data, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_OSL_TEST(GradingTone, style_log_black_detail_fwd_dynamic)
{
    GradingToneLogBlacks(m_data, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_OSL_TEST(GradingTone, style_log_black_detail_rev)
{
    GradingToneLogBlacks(m_data, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_OSL_TEST(GradingTone, style_log_black_detail_rev_dynamic)
{
    GradingToneLogBlacks(m_data, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GTTest6
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
static constexpr double scontrast = 1.8;
static constexpr double scontrast2 = 0.3;
}

void GradingToneLogSContrast(OSLDataRcPtr & data, 
                             OCIO::TransformDirection dir,
                             bool dynamic,
                             double scontrast)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest6::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest6::style);
    tone.m_scontrast = scontrast;
    gt->setValue(tone);

    data->m_transform = gt;

    data->m_threshold = 3e-5f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingTone, style_log_scontrast_fwd)
{
    GradingToneLogSContrast(m_data, OCIO::TRANSFORM_DIR_FORWARD, false, GTTest6::scontrast);
}

OCIO_OSL_TEST(GradingTone, style_log_scontrast_fwd_dynamic)
{
    GradingToneLogSContrast(m_data, OCIO::TRANSFORM_DIR_FORWARD, true, GTTest6::scontrast);
}

OCIO_OSL_TEST(GradingTone, style_log_scontrast2_fwd)
{
    GradingToneLogSContrast(m_data, OCIO::TRANSFORM_DIR_FORWARD, false, GTTest6::scontrast2);
}

OCIO_OSL_TEST(GradingTone, style_log_scontrast2_fwd_dynamic)
{
    GradingToneLogSContrast(m_data, OCIO::TRANSFORM_DIR_FORWARD, true, GTTest6::scontrast2);
}

OCIO_OSL_TEST(GradingTone, style_log_scontrast_rev)
{
    GradingToneLogSContrast(m_data, OCIO::TRANSFORM_DIR_INVERSE, false, GTTest6::scontrast);
}

OCIO_OSL_TEST(GradingTone, style_log_scontrast_rev_dynamic)
{
    GradingToneLogSContrast(m_data, OCIO::TRANSFORM_DIR_INVERSE, true, GTTest6::scontrast);
}

OCIO_OSL_TEST(GradingTone, style_log_scontrast2_rev)
{
    GradingToneLogSContrast(m_data, OCIO::TRANSFORM_DIR_INVERSE, false, GTTest6::scontrast2);
}

OCIO_OSL_TEST(GradingTone, style_log_scontrast2_rev_dynamic)
{
    GradingToneLogSContrast(m_data, OCIO::TRANSFORM_DIR_INVERSE, true, GTTest6::scontrast2);
}

namespace GTTest7
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LIN;
// These are {R, G, B, master, center, width};
const OCIO::GradingRGBMSW midtones{ 0.3, 1.4, 1.8, 1., 1., 8. };
}

void GradingToneLinMidtones(OSLDataRcPtr & data, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest7::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest7::style);
    tone.m_midtones = GTTest7::midtones;
    gt->setValue(tone);

    data->m_transform = gt;

    data->m_threshold = 5e-5f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingTone, style_lin_midtones_fwd)
{
    GradingToneLinMidtones(m_data, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_OSL_TEST(GradingTone, style_lin_midtones_fwd_dynamic)
{
    GradingToneLinMidtones(m_data, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_OSL_TEST(GradingTone, style_lin_midtones_rev)
{
    GradingToneLinMidtones(m_data, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_OSL_TEST(GradingTone, style_lin_midtones_rev_dynamic)
{
    GradingToneLinMidtones(m_data, OCIO::TRANSFORM_DIR_INVERSE, true);
}

OCIO_OSL_TEST(GradingTone, style_log_dynamic_retests)
{
    auto gt = OCIO::GradingToneTransform::Create(OCIO::GRADING_LOG);
    gt->makeDynamic();

    OCIO::GradingTone gtlog{ OCIO::GRADING_LOG };
    gt->setValue(gtlog);

    m_data->m_transform = gt;

    m_data->m_threshold = 5e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingTone, two_transforms_retests)
{
    auto gtDyn = OCIO::GradingToneTransform::Create(OCIO::GRADING_LOG);
    gtDyn->makeDynamic();

    OCIO::GradingTone gtlog{ OCIO::GRADING_LOG };
    gtDyn->setValue(gtlog);

    auto gtNonDyn = OCIO::GradingToneTransform::Create(GTTest6::style);

    OCIO::GradingTone tone{ OCIO::GRADING_LIN };
    tone.m_scontrast = 1.8;
    tone.m_midtones = OCIO::GradingRGBMSW(0.3, 1.4, 1.8, 1., 1., 8.);

    gtNonDyn->setValue(tone);

    auto group = OCIO::GroupTransform::Create();
    group->appendTransform(gtDyn);
    group->appendTransform(gtNonDyn);

    m_data->m_transform = group;

    m_data->m_threshold = 5e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

