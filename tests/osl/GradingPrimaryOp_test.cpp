// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// NOTE:
// The file is a copy and paste from the corresponding GPU unitest i.e. []/tests/gpu/GradingPrimaryOp_test.cpp.
// Keep the two files in sync. to increase the test coverage.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


namespace GPTest1
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
static const OCIO::GradingRGBM brightness{ -10., 45., -5.,  50. };
static const OCIO::GradingRGBM contrast{ 0.9, 1.4,  0.7,  0.75 };
static const OCIO::GradingRGBM gamma{ 1.1, 0.7,  1.05, 1.15 };
static constexpr double saturation = 1.21;
static constexpr double pivot      = -0.3;
static constexpr double pivotBlack = 0.05;
static constexpr double pivotWhite = 0.9;
static constexpr double clampBlack = -0.05;
static constexpr double clampWhite = 1.50;
}

OCIO::GradingPrimaryTransformRcPtr GradingPrimaryLog(OCIO::TransformDirection dir, bool dynamic)
{
    auto gp = OCIO::GradingPrimaryTransform::Create(GPTest1::style);
    gp->setDirection(dir);
    if (dynamic)
    {
        gp->makeDynamic();
    }

    OCIO::GradingPrimary gplog{ GPTest1::style };
    gplog.m_brightness = GPTest1::brightness;
    gplog.m_contrast   = GPTest1::contrast;
    gplog.m_gamma      = GPTest1::gamma;
    gplog.m_saturation = GPTest1::saturation;
    gplog.m_pivot      = GPTest1::pivot;
    gplog.m_pivotBlack = GPTest1::pivotBlack;
    gplog.m_pivotWhite = GPTest1::pivotWhite;
    gplog.m_clampBlack = GPTest1::clampBlack;
    gplog.m_clampWhite = GPTest1::clampWhite;
    gp->setValue(gplog);

    return gp;
}

OCIO_OSL_TEST(GradingPrimary, style_log_fwd)
{
    m_data->m_transform = GradingPrimaryLog(OCIO::TRANSFORM_DIR_FORWARD, false);

    m_data->m_threshold = 2e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingPrimary, style_log_fwd_dynamic)
{
    m_data->m_transform = GradingPrimaryLog(OCIO::TRANSFORM_DIR_FORWARD, true);

    m_data->m_threshold = 2e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingPrimary, style_log_rev)
{
    m_data->m_transform = GradingPrimaryLog(OCIO::TRANSFORM_DIR_INVERSE, false);

    m_data->m_threshold = 5e-4f; // Slight difference with the GLSL unit test i.e. 2e-5f
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingPrimary, style_log_rev_dynamic)
{
    m_data->m_transform = GradingPrimaryLog(OCIO::TRANSFORM_DIR_INVERSE, true);

    m_data->m_threshold = 5e-4f; // Slight difference with the GLSL unit test i.e. 2e-5f
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

namespace GPTest2
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LIN;
static const OCIO::GradingRGBM exposure{ 0.5,  -0.2,  0.4, -0.25 };
static const OCIO::GradingRGBM offset{ -0.03,  0.02, 0.1, -0.1 };
static const OCIO::GradingRGBM contrast{ 0.9,   1.4,  0.7,  0.75 };
static constexpr double saturation = 1.33;
static constexpr double pivot      = 0.5;
static constexpr double clampBlack = -0.40;
static constexpr double clampWhite = 1.05;
}

OCIO::GradingPrimaryTransformRcPtr GradingPrimaryLin(OCIO::TransformDirection dir, bool dynamic)
{
    auto gp = OCIO::GradingPrimaryTransform::Create(GPTest2::style);
    gp->setDirection(dir);
    if (dynamic)
    {
        gp->makeDynamic();
    }

    OCIO::GradingPrimary gplin{ GPTest2::style };
    gplin.m_exposure   = GPTest2::exposure;
    gplin.m_contrast   = GPTest2::contrast;
    gplin.m_offset     = GPTest2::offset;
    gplin.m_pivot      = GPTest2::pivot;
    gplin.m_saturation = GPTest2::saturation;
    gplin.m_clampBlack = GPTest2::clampBlack;
    gplin.m_clampWhite = GPTest2::clampWhite;
    gp->setValue(gplin);

    return gp;
}

OCIO_OSL_TEST(GradingPrimary, style_lin_fwd)
{
    m_data->m_transform = GradingPrimaryLin(OCIO::TRANSFORM_DIR_FORWARD, false);

    m_data->m_threshold = 2e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingPrimary, style_lin_fwd_dynamic)
{
    m_data->m_transform = GradingPrimaryLin(OCIO::TRANSFORM_DIR_FORWARD, true);

    m_data->m_threshold = 2e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingPrimary, style_lin_rev)
{
    m_data->m_transform = GradingPrimaryLin(OCIO::TRANSFORM_DIR_INVERSE, false);

    m_data->m_threshold = 5e-5f;   // Slight difference with the GLSL unit test i.e. 2e-5f
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingPrimary, style_lin_rev_dynamic)
{
    m_data->m_transform = GradingPrimaryLin(OCIO::TRANSFORM_DIR_INVERSE, true);

    m_data->m_threshold = 5e-5f;   // Slight difference with the GLSL unit test i.e. 2e-5f
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

namespace GPTest3
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LIN;
static const OCIO::GradingRGBM lift{ 0.05, -0.04, 0.02, 0.05 };
static const OCIO::GradingRGBM gamma{ 0.9,   1.4,  0.7,  0.75 };
static const OCIO::GradingRGBM gain{ 1.2,   1.1,  1.25, 0.8 };
static const OCIO::GradingRGBM offset{ -0.03,  0.02, 0.1, -0.1 };
static constexpr double saturation = 1.2;
static constexpr double pivotBlack = 0.05;
static constexpr double pivotWhite = 0.9;
static constexpr double clampBlack = -0.15;
static constexpr double clampWhite = 1.50;
}

void GradingPrimaryVideo(OSLDataRcPtr & data, OCIO::TransformDirection dir, bool dynamic)
{
    auto gp = OCIO::GradingPrimaryTransform::Create(GPTest3::style);
    gp->setDirection(dir);
    if (dynamic)
    {
        gp->makeDynamic();
    }

    OCIO::GradingPrimary gpvideo{ GPTest3::style };
    gpvideo.m_lift       = GPTest3::lift;
    gpvideo.m_gamma      = GPTest3::gamma;
    gpvideo.m_gain       = GPTest3::gain;
    gpvideo.m_offset     = GPTest3::offset;
    gpvideo.m_saturation = GPTest3::saturation;
    gpvideo.m_clampBlack = GPTest3::clampBlack;
    gpvideo.m_clampWhite = GPTest3::clampWhite;
    gpvideo.m_pivotBlack = GPTest3::pivotBlack;
    gpvideo.m_pivotWhite = GPTest3::pivotWhite;
    gp->setValue(gpvideo);

    data->m_threshold = 3e-5f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;

    data->m_transform = gp;
}

OCIO_OSL_TEST(GradingPrimary, style_video_fwd)
{
    GradingPrimaryVideo(m_data, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_OSL_TEST(GradingPrimary, style_video_fwd_dynamic)
{
    GradingPrimaryVideo(m_data, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_OSL_TEST(GradingPrimary, style_video_rev)
{
    GradingPrimaryVideo(m_data, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_OSL_TEST(GradingPrimary, style_video_rev_dynamic)
{
    GradingPrimaryVideo(m_data, OCIO::TRANSFORM_DIR_INVERSE, true);
}

OCIO_OSL_TEST(GradingPrimary, style_log_dynamic_retests)
{
    auto gp = OCIO::GradingPrimaryTransform::Create(GPTest1::style);
    gp->makeDynamic();

    OCIO::GradingPrimary gplog{ GPTest1::style };
    gplog.m_brightness = GPTest1::brightness;
    gplog.m_contrast   = GPTest1::contrast;
    gplog.m_gamma      = GPTest1::gamma;
    gplog.m_saturation = GPTest1::saturation;
    gplog.m_pivot      = GPTest1::pivot;
    gp->setValue(gplog);

    m_data->m_transform = gp;

    m_data->m_threshold = 5e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingPrimary, two_transforms_retests)
{
    auto gpDyn = OCIO::GradingPrimaryTransform::Create(GPTest1::style);
    gpDyn->makeDynamic();

    OCIO::GradingPrimary gplog{ GPTest1::style };
    gplog.m_brightness = GPTest1::brightness;
    gplog.m_contrast = GPTest1::contrast;
    gplog.m_gamma = GPTest1::gamma;
    gplog.m_saturation = GPTest1::saturation;
    gplog.m_pivot = GPTest1::pivot;
    gpDyn->setValue(gplog);

    auto gpNonDyn = OCIO::GradingPrimaryTransform::Create(GPTest2::style);
    OCIO::GradingPrimary gplin{ GPTest2::style };
    gplin.m_exposure = GPTest2::exposure;
    gplin.m_contrast = GPTest2::contrast;
    gplin.m_offset = GPTest2::offset;
    gplin.m_pivot = GPTest2::pivot;
    gplin.m_saturation = GPTest2::saturation;
    gplin.m_clampBlack = GPTest2::clampBlack;
    gplin.m_clampWhite = GPTest2::clampWhite;
    gpNonDyn->setValue(gplin);

    auto group = OCIO::GroupTransform::Create();
    group->appendTransform(gpDyn);
    group->appendTransform(gpNonDyn);

    m_data->m_transform = group;

    m_data->m_threshold = 1e-4f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}
