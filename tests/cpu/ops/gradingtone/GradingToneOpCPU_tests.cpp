// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cmath>

#include "ops/gradingtone/GradingToneOpCPU.cpp"

#include "testutils/UnitTest.h"
#include "utils/StringUtils.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{
void ValidateImage(const float * expected, const float * res, long numPix, unsigned line)
{
#if OCIO_USE_SSE2
    static constexpr float error = 2e-4f;
#else
    static constexpr float error = 1e-6f;
#endif // OCIO_USE_SSE2

    for (long i = 0; i < numPix; ++i)
    {
        for (long j = 0; j < 4; ++j)
        {
            if (OCIO::IsNan(expected[i * 4 + j]))
            {
#if OCIO_USE_SSE2
                // Do not test nan in SSE mode.
#else
                OCIO_CHECK_ASSERT(OCIO::IsNan(res[i * 4 + j]));
#endif // OCIO_USE_SSE2
            }
            else if (expected[i * 4 + j] != res[i * 4 + j])
            {
                const float errorFactor = std::abs(expected[i * 4 + j]) < 1.f ? 1.f :
                                          std::abs(expected[i * 4 + j]);
                OCIO_CHECK_CLOSE_FROM(expected[i * 4 + j], res[i * 4 + j], error * errorFactor,
                                      line);
            }
        }
    }
}
}

OCIO_ADD_TEST(GradingToneOpCPU, identity)
{
    constexpr long numPixels = 8;
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    // inf is clamp, so inverse would fail.
    const float image[4*numPixels] = { -0.50f, -0.25f,   0.50f, 0.0f,
                                        0.75f,  1.00f,   1.25f, 1.0f,
                                      65000.f,  1.50f, -65000.f, 0.0f,
                                         qnan,   qnan,    qnan, 0.0f,
                                         0.0f,   0.0f,    0.0f, qnan,
                                         0.0f,   0.0f,    0.0f,  inf,
                                         -inf,   -inf,    -inf, 0.0f,
                                         0.0f,   0.0f,    0.0f, -inf };

    const float expected[4*numPixels] = { -0.50f,  -0.25f,    0.50f, 0.0f,
                                           0.75f,   1.00f,    1.25f, 1.0f,
                                         65000.f,   1.50f, -65000.f, 0.0f,
                                            qnan,    qnan,     qnan, 0.0f,
                                            0.0f,    0.0f,     0.0f, qnan,
                                            0.0f,    0.0f,     0.0f,  inf,
                                            -inf,    -inf,     -inf, 0.0f,
                                            0.0f,    0.0f,     0.0f, -inf };

    float res[4 * numPixels]{ 0.f };

    auto gd = std::make_shared<OCIO::GradingToneOpData>(OCIO::GRADING_LOG);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingToneOpDataRcPtr gdc = gd;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains GradingToneFwdOpCPU.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "GradingToneFwdOpCPU"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gd->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains GradingToneRevOpCPU.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "GradingToneRevOpCPU"));
    }
    OCIO_CHECK_NO_THROW(op->apply(expected, res, numPixels));
    ValidateImage(image, res, numPixels, __LINE__);

    gd = std::make_shared<OCIO::GradingToneOpData>(OCIO::GRADING_LIN);
    gdc = gd;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains GradingToneLinearFwdOpCPU.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "GradingToneLinearFwdOpCPU"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gd->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains GradingToneLinearRevOpCPU.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "GradingToneLinearRevOpCPU"));
    }
    OCIO_CHECK_NO_THROW(op->apply(expected, res, numPixels));
    ValidateImage(image, res, numPixels, __LINE__);

    gd = std::make_shared<OCIO::GradingToneOpData>(OCIO::GRADING_VIDEO);
    gdc = gd;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains GradingToneFwdOpCPU.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "GradingToneFwdOpCPU"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gd->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains GradingToneRevOpCPU.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "GradingToneRevOpCPU"));
    }
    OCIO_CHECK_NO_THROW(op->apply(expected, res, numPixels));
    ValidateImage(image, res, numPixels, __LINE__);
}

namespace TS1
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
// These are {R, G, B, master, center, width};
static const OCIO::GradingRGBMSW midtones{ 0.3, 1.0, 1.8,  1.2, 0.47, 0.6 };
static constexpr unsigned num_samples = 3u;
static constexpr float input_32f[] = {
    0.1f,-0.4f, 0.9f, 1.0f,
    0.3f, 0.6f, 0.7f, 0.5f,
    0.8f, 2.2f, 0.5f, 0.0f };
static constexpr float expected_32f[] = {
    0.09440361f,-0.40000000f, 0.90645507f, 1.0f,
    0.23564218f, 0.62838000f, 0.76080927f, 0.5f,
    0.78783701f, 2.20000000f, 0.67159981f, 0.0f };
}

OCIO_ADD_TEST(GradingToneOpCPU, log_midtones)
{
    float res[4 * TS1::num_samples]{ 0.f };

    auto gt = std::make_shared<OCIO::GradingToneOpData>(TS1::style);

    // Test in forward direction.

    OCIO::GradingTone gtd(TS1::style);
    gtd.m_midtones = TS1::midtones;

    gt->setValue(gtd);
    gt->getDynamicPropertyInternal()->makeDynamic();
    OCIO::ConstGradingToneOpDataRcPtr gtc = gt;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS1::input_32f, res, TS1::num_samples));
    ValidateImage(TS1::expected_32f, res, TS1::num_samples, __LINE__);

    // Test in inverse direction.

    gt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    gt->setValue(gtd);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS1::expected_32f, res, TS1::num_samples));
    ValidateImage(TS1::input_32f, res, TS1::num_samples, __LINE__);
}

namespace TS2
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
// These are {R, G, B, master, start, pivot};
static const OCIO::GradingRGBMSW highlights{ 0.3, 1.0, 1.8, 1.4, -0.1, 0.9 };
static constexpr unsigned num_samples = 3u;
static constexpr float input_32f[] = {
     0.8f, 0.2f, -0.05f, 1.0f,
    -0.4f, 0.7f,  0.8f,  0.5f,
     0.5f, 1.0f,  2.2f,  0.0f };
static constexpr float expected_32f[] = {
     0.75833820f, 0.21800000f, -0.04847980f, 1.0f,
    -0.40000000f, 0.75600000f,  0.88018560f, 0.5f,
     0.46114011f, 0.96000000f,  1.05600000f, 0.0f};
}

OCIO_ADD_TEST(GradingToneOpCPU, log_highlights)
{
    float res[4 * TS2::num_samples]{ 0.f };

    auto gt = std::make_shared<OCIO::GradingToneOpData>(TS2::style);

    // Test in forward direction.

    OCIO::GradingTone gtd(TS2::style);
    gtd.m_highlights = TS2::highlights;

    gt->setValue(gtd);
    gt->getDynamicPropertyInternal()->makeDynamic();
    OCIO::ConstGradingToneOpDataRcPtr gtc = gt;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS2::input_32f, res, TS2::num_samples));
    ValidateImage(TS2::expected_32f, res, TS2::num_samples, __LINE__);

    // Test in inverse direction.

    gt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    gt->setValue(gtd);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS2::expected_32f, res, TS2::num_samples));
    ValidateImage(TS2::input_32f, res, TS2::num_samples, __LINE__);
}

namespace TS3
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_VIDEO;
// These are {R, G, B, master, start, pivot};
static const OCIO::GradingRGBMSW shadows{ 0.3, 1., 1.79, 0.6, 0.8, -0.1 };
static constexpr unsigned num_samples = 3u;
static constexpr float input_32f[] = {
    -0.05f,-0.3f,-0.05f, 1.0f,
     0.20f, 0.2f, 0.10f, 0.5f,
     0.50f, 1.2f, 0.40f, 0.0f };
static constexpr float expected_32f[] = {
    -0.08903600f, -0.22000000f, -0.0101064f, 1.0f,
     0.04235000f,  0.14000000f, 0.158287734f, 0.5f,
     0.44006111f,  1.20000000f, 0.426106364f, 0.0f };
}

OCIO_ADD_TEST(GradingToneOpCPU, video_shadows)
{
    float res[4 * TS3::num_samples]{ 0.f };

    auto gt = std::make_shared<OCIO::GradingToneOpData>(TS3::style);

    // Test in forward direction.

    OCIO::GradingTone gtd(TS3::style);
    gtd.m_shadows = TS3::shadows;

    gt->setValue(gtd);
    gt->getDynamicPropertyInternal()->makeDynamic();
    OCIO::ConstGradingToneOpDataRcPtr gtc = gt;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS3::input_32f, res, TS3::num_samples));
    ValidateImage(TS3::expected_32f, res, TS3::num_samples, __LINE__);

    // Test in inverse direction.

    gt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    gt->setValue(gtd);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS3::expected_32f, res, TS3::num_samples));
    ValidateImage(TS3::input_32f, res, TS3::num_samples, __LINE__);
}

namespace TS4
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_VIDEO;
// These are {R, G, B, master, start, width};
static const OCIO::GradingRGBMSW whiteDetail{ 0.3, 1., 1.9, 0.6, -0.2, 1.4 };
static constexpr unsigned num_samples = 3u;
static constexpr float input_32f[] = {
    0.9f,-0.4f, 0.8f, 1.0f,
    1.2f, 0.8f, 1.0f, 0.5f,
    8.0f, 4.0f, 2.0f, 0.0f };
static constexpr float expected_32f[] = {
    0.50664196f,-0.40000000f, 0.85713846f, 1.0f,
    0.59170000f, 0.65714286f, 1.11661389f, 0.5f,
    1.85000000f, 2.60000000f,17.73099488f, 0.0f };
}

OCIO_ADD_TEST(GradingToneOpCPU, video_white_details)
{
    float res[4 * TS4::num_samples]{ 0.f };

    auto gt = std::make_shared<OCIO::GradingToneOpData>(TS4::style);

    // Test in forward direction.

    OCIO::GradingTone gtd(TS4::style);
    gtd.m_whites = TS4::whiteDetail;

    gt->setValue(gtd);
    gt->getDynamicPropertyInternal()->makeDynamic();
    OCIO::ConstGradingToneOpDataRcPtr gtc = gt;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS4::input_32f, res, TS4::num_samples));
    ValidateImage(TS4::expected_32f, res, TS4::num_samples, __LINE__);

    // Test in inverse direction.

    gt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    gt->setValue(gtd);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS4::expected_32f, res, TS4::num_samples));
    ValidateImage(TS4::input_32f, res, TS4::num_samples, __LINE__);
}

namespace TS5
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
// These are {R, G, B, master, start, width};
static const OCIO::GradingRGBMSW blackDetail{ 0.3, 1., 1.9, 0.6, 0.8, 0.9 };
static constexpr unsigned num_samples = 3u;
static constexpr float input_32f[] = {
    -0.05f,-0.5f,-0.20f, 1.0f,
     0.05f, 0.0f,-0.05f, 0.5f,
     0.40f, 1.2f, 0.40f, 0.0f };
static constexpr float expected_32f[] = {
    -0.88574485f,-0.99166667f, 0.23906196f, 1.0f,
    -0.50105701f,-0.16583916f, 0.25926968f, 0.5f,
     0.30488108f, 1.20000000f, 0.45937302f, 0.0f };
}

OCIO_ADD_TEST(GradingToneOpCPU, log_black_details)
{
    float res[4 * TS5::num_samples]{ 0.f };

    auto gt = std::make_shared<OCIO::GradingToneOpData>(TS5::style);

    // Test in forward direction.

    OCIO::GradingTone gtd(TS5::style);
    gtd.m_blacks = TS5::blackDetail;

    gt->setValue(gtd);
    gt->getDynamicPropertyInternal()->makeDynamic();
    OCIO::ConstGradingToneOpDataRcPtr gtc = gt;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS5::input_32f, res, TS5::num_samples));
    ValidateImage(TS5::expected_32f, res, TS5::num_samples, __LINE__);

    // Test in inverse direction.

    gt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    gt->setValue(gtd);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS5::expected_32f, res, TS5::num_samples));
    ValidateImage(TS5::input_32f, res, TS5::num_samples, __LINE__);
}

namespace TS6
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
static constexpr unsigned num_samples = 3u;

static constexpr double scontrast = 1.8;
static constexpr float input_32f[] = {
     0.15f, 0.3f, 0.42f, 1.0f,
    -0.1f,  0.6f, 1.2f,  0.5f,
     0.8f,  0.0f, 1.0f,  0.0f };
static constexpr float expected_32f[] = {
     0.05250000f, 0.15283050f, 0.45714286f, 1.0f,
    -0.03500000f, 0.83910667f, 1.07000000f, 0.5f,
     0.93000000f, 0.00000000f, 1.00000000f, 0.0f };

static constexpr double scontrast2 = 0.3;
static constexpr float input2_32f[] = {
     0.04f, 0.3f, 0.15f, 1.0f,
    -0.1f,  0.6f, 1.2f,  0.5f,
     0.8f,  0.0f, 1.0f,  0.0f };
static constexpr float expected2_32f[] = {
     0.08050314f, 0.35031250f, 0.26213396f, 1.0f,
    -0.20125786f, 0.49937500f, 1.40251572f, 0.5f,
     0.63561388f, 0.00000000f, 1.00000000f, 0.0f };
}

OCIO_ADD_TEST(GradingToneOpCPU, log_scontrast)
{
    float res[4 * TS6::num_samples]{ 0.f };

    auto gt = std::make_shared<OCIO::GradingToneOpData>(TS6::style);

    // Test in forward direction.

    OCIO::GradingTone gtd(TS6::style);
    gtd.m_scontrast = TS6::scontrast;

    gt->setValue(gtd);
    gt->getDynamicPropertyInternal()->makeDynamic();
    OCIO::ConstGradingToneOpDataRcPtr gtc = gt;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS6::input_32f, res, TS6::num_samples));
    ValidateImage(TS6::expected_32f, res, TS6::num_samples, __LINE__);

    // Test with second value.
    gtd.m_scontrast = TS6::scontrast2;

    gt->setValue(gtd);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS6::input2_32f, res, TS6::num_samples));
    ValidateImage(TS6::expected2_32f, res, TS6::num_samples, __LINE__);

    // Test in inverse direction with second value.

    gt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    gt->setValue(gtd);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS6::expected2_32f, res, TS6::num_samples));
    ValidateImage(TS6::input2_32f, res, TS6::num_samples, __LINE__);

    // Test inverse with first value.
    gtd.m_scontrast = TS6::scontrast;

    gt->setValue(gtd);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS6::expected_32f, res, TS6::num_samples));
    ValidateImage(TS6::input_32f, res, TS6::num_samples, __LINE__);
}

namespace TS7
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LIN;
// These are {R, G, B, master, center, width};
const OCIO::GradingRGBMSW midtones{ 0.3, 1.4, 1.8, 1., 1., 8. };
static constexpr unsigned num_samples = 3u;
static constexpr float input_32f[] = {
    0.1f,-0.1f, 0.90f, 1.0f,
    0.3f, 0.6f, 0.70f, 0.5f,
    0.8f, 1.5f, 0.05f, 0.0f };
static constexpr float expected_32f[] = {
    0.04102994f,-0.10000000f, 3.07542735f, 1.0f,
    0.08530666f, 1.19569300f, 2.65221218f, 0.5f,
    0.26080380f, 2.37429354f, 0.08896667f, 0.0f };
}

OCIO_ADD_TEST(GradingToneOpCPU, lin_midtones)
{
    float res[4 * TS7::num_samples]{ 0.f };

    auto gt = std::make_shared<OCIO::GradingToneOpData>(TS7::style);

    // Test in forward direction.

    OCIO::GradingTone gtd(TS7::style);
    gtd.m_midtones = TS7::midtones;

    gt->setValue(gtd);
    gt->getDynamicPropertyInternal()->makeDynamic();
    OCIO::ConstGradingToneOpDataRcPtr gtc = gt;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS7::input_32f, res, TS7::num_samples));
    ValidateImage(TS7::expected_32f, res, TS7::num_samples, __LINE__);

    // Test in inverse direction.

    gt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    gt->setValue(gtd);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingToneCPURenderer(gtc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS7::expected_32f, res, TS7::num_samples));
    ValidateImage(TS7::input_32f, res, TS7::num_samples, __LINE__);
}
