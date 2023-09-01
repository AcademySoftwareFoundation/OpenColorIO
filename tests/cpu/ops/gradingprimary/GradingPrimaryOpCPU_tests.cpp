// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradingprimary/GradingPrimaryOpCPU.cpp"

#include "testutils/UnitTest.h"
#include "utils/StringUtils.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{
void ValidateImage(const float * expected, const float * res, long numPix, unsigned line)
{
#if OCIO_USE_SSE2
    static constexpr float error = 1e-4f;
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
                OCIO_CHECK_CLOSE_FROM(expected[i * 4 + j], res[i * 4 + j], error, line);
            }
        }
    }
}
}

OCIO_ADD_TEST(GradingPrimaryOpCPU, identity)
{
    constexpr long numPixels = 9;
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float image[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                        0.75f,  1.00f, 1.25f, 1.0f,
                                        1.25f,  1.50f, 1.75f, 0.0f,
                                         qnan,   qnan,  qnan, 0.0f,
                                         0.0f,   0.0f,  0.0f, qnan,
                                          inf,    inf,   inf, 0.0f,
                                         0.0f,   0.0f,  0.0f,  inf,
                                         -inf,   -inf,  -inf, 0.0f,
                                         0.0f,   0.0f,  0.0f, -inf };

    const float expected[4*numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                           0.75f,  1.00f, 1.25f, 1.0f,
                                           1.25f,  1.50f, 1.75f, 0.0f,
                                            qnan,   qnan,  qnan, 0.0f,
                                            0.0f,   0.0f,  0.0f, qnan,
                                             inf,    inf,   inf, 0.0f,
                                            0.0f,   0.0f,  0.0f,  inf,
                                            -inf,   -inf,  -inf, 0.0f,
                                            0.0f,   0.0f,  0.0f, -inf };

    float res[4 * numPixels]{ 0.f };

    auto gd = std::make_shared<OCIO::GradingPrimaryOpData>(OCIO::GRADING_LOG);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingPrimaryOpDataRcPtr gdc = gd;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains LogFwd.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "LogFwd"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gd->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains LogRev.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "LogRev"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gd = std::make_shared<OCIO::GradingPrimaryOpData>(OCIO::GRADING_LIN);
    gdc = gd;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains LinFwd.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "LinFwd"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gd->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains LinRev.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "LinRev"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gd = std::make_shared<OCIO::GradingPrimaryOpData>(OCIO::GRADING_VIDEO);
    gdc = gd;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains VidFwd.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "VidFwd"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gd->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains VidRev.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "VidRev"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);
}

namespace TS1
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
static const OCIO::GradingRGBM brightness{ -10., 45., -5.,  50. };
static const OCIO::GradingRGBM contrast  { 0.9, 1.4,  0.7,  0.75 };
static const OCIO::GradingRGBM gamma     { 1.1, 0.7,  1.05, 1.15 };
static constexpr double pivot       = -0.3;
static constexpr double saturation  =  1.21;
static constexpr double clampBlack = -0.05;
static constexpr double clampWhite =  1.50;
static constexpr double pivotBlack =  0.05;
static constexpr double pivotWhite =  0.9;
static constexpr long num_samples = 2;
const float input_32f[] = {
     0.1f, 0.9f, 1.2f, 1.0f,
    -0.4f, 0.2f, 1.2f, 0.5f };
const float expected_32f[] = {
     0.23327083f, 1.77384381f, 0.86027701f, 1.0f,
    -0.10117631f, 0.79016840f, 1.02051931f, 0.5f };
const float expected_clamp_32f[] = {
     0.23327083f, 1.50000000f, 0.86027701f, 1.0f,
    -0.05000000f, 0.79016840f, 1.02051931f, 0.5f };
const float expected_wbpivot_32f[] = {
     0.21137053f, 1.82456972f, 0.83339811f, 1.0f,
    -0.16370305f, 0.81365125f, 0.99945772f, 0.5f };
}

OCIO_ADD_TEST(GradingPrimaryOpCPU, log)
{
    float res[4 * TS1::num_samples]{ 0.f };

    auto gd = std::make_shared<OCIO::GradingPrimaryOpData>(TS1::style);

    // Test in forward direction.

    OCIO::GradingPrimary gdp(TS1::style);
    gdp.m_brightness  = TS1::brightness;
    gdp.m_contrast    = OCIO::GradingRGBM(TS1::contrast);
    gdp.m_gamma       = OCIO::GradingRGBM(TS1::gamma);
    gdp.m_pivot       = TS1::pivot;
    gdp.m_saturation  = TS1::saturation;

    gd->setValue(gdp);
    gd->getDynamicPropertyInternal()->makeDynamic();

    OCIO::ConstGradingPrimaryOpDataRcPtr gdc = gd;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS1::input_32f, res, TS1::num_samples));
    ValidateImage(TS1::expected_32f, res, TS1::num_samples, __LINE__);

    // The CPUOp has a copy of gd.  Get the dynamic property ptr in order to change the value for
    // the apply.
    OCIO::DynamicPropertyGradingPrimaryRcPtr dpgp;
    auto dp = op->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY);
    OCIO_CHECK_NO_THROW(dpgp = OCIO::DynamicPropertyValue::AsGradingPrimary(dp));

    gdp.m_clampBlack = TS1::clampBlack;
    gdp.m_clampWhite = TS1::clampWhite;

    dpgp->setValue(gdp);
    OCIO_CHECK_NO_THROW(op->apply(TS1::input_32f, res, TS1::num_samples));
    ValidateImage(TS1::expected_clamp_32f, res, TS1::num_samples, __LINE__);

    gdp.m_clampBlack = -100;
    gdp.m_clampWhite = 100;
    gdp.m_pivotBlack = TS1::pivotBlack;
    gdp.m_pivotWhite = TS1::pivotWhite;

    dpgp->setValue(gdp);
    OCIO_CHECK_NO_THROW(op->apply(TS1::input_32f, res, TS1::num_samples));
    ValidateImage(TS1::expected_wbpivot_32f, res, TS1::num_samples, __LINE__);

    // Test in inverse direction.

    gd->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    gdp = OCIO::GradingPrimary(TS1::style);
    gdp.m_brightness = OCIO::GradingRGBM(TS1::brightness);
    gdp.m_contrast = OCIO::GradingRGBM(TS1::contrast);
    gdp.m_gamma = OCIO::GradingRGBM(TS1::gamma);
    gdp.m_pivot = TS1::pivot;
    gdp.m_saturation = TS1::saturation;

    gd->setValue(gdp);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS1::expected_32f, res, TS1::num_samples));
    ValidateImage(TS1::input_32f, res, TS1::num_samples, __LINE__);

    dp = op->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY);
    OCIO_CHECK_NO_THROW(dpgp = OCIO::DynamicPropertyValue::AsGradingPrimary(dp));

    // Clamping prevents full inversion. Skip.

    gdp.m_pivotBlack = TS1::pivotBlack;
    gdp.m_pivotWhite = TS1::pivotWhite;

    dpgp->setValue(gdp);
    OCIO_CHECK_NO_THROW(op->apply(TS1::expected_wbpivot_32f, res, TS1::num_samples));
    ValidateImage(TS1::input_32f, res, TS1::num_samples, __LINE__);
}

namespace TS2
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LIN;
static const OCIO::GradingRGBM exposure{  0.5,  -0.2,  0.4, -0.25 };
static const OCIO::GradingRGBM offset  { -0.03,  0.02, 0.1, -0.1 };
static const OCIO::GradingRGBM contrast{  0.9,   1.4,  0.7,  0.75 };
static constexpr double pivot       =  0.5;
static constexpr double saturation  =  1.33;
static constexpr double clampBlack = -0.40;
static constexpr double clampWhite =  1.05;
static constexpr long num_samples = 2;
const float input_32f[] = {
     0.1f, 0.9f, 1.2f, 1.0f,
    -0.1f, 0.9f, 3.2f, 0.5f };
const float expected_32f[] = {
    -0.24746465f, 0.67575505f, 0.64940625f, 1.0f,
    -0.50871492f, 0.68002410f, 1.19721858f, 0.5f };
const float expected_clamp_32f[] = {
    -0.24746465f, 0.67575505f, 0.64940625f, 1.0f,
    -0.40000000f, 0.68002410f, 1.05000000f, 0.5f };
}

OCIO_ADD_TEST(GradingPrimaryOpCPU, lin)
{
    float res[4 * TS2::num_samples]{ 0.f };

    auto gd = std::make_shared<OCIO::GradingPrimaryOpData>(TS2::style);

    // Test in forward direction.

    OCIO::GradingPrimary gdp(TS2::style);
    gdp.m_exposure    = OCIO::GradingRGBM(TS2::exposure);
    gdp.m_offset      = OCIO::GradingRGBM(TS2::offset);
    gdp.m_contrast    = OCIO::GradingRGBM(TS2::contrast);
    gdp.m_pivot       = TS2::pivot;
    gdp.m_saturation  = TS2::saturation;

    gd->setValue(gdp);

    OCIO::ConstGradingPrimaryOpDataRcPtr gdc = gd;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS2::input_32f, res, TS2::num_samples));
    ValidateImage(TS2::expected_32f, res, TS2::num_samples, __LINE__);

    gdp.m_clampBlack = TS2::clampBlack;
    gdp.m_clampWhite = TS2::clampWhite;

    gd->setValue(gdp);
    OCIO_CHECK_NO_THROW(op->apply(TS2::input_32f, res, TS2::num_samples));
    ValidateImage(TS2::expected_clamp_32f, res, TS2::num_samples, __LINE__);

    // Test in inverse direction.

    gd->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    gdp.m_clampBlack = -100;
    gdp.m_clampWhite = 100;

    gd->setValue(gdp);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS2::expected_32f, res, TS2::num_samples));
    ValidateImage(TS2::input_32f, res, TS2::num_samples, __LINE__);
}

namespace TS3
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_VIDEO;
static const OCIO::GradingRGBM lift  {  0.05, -0.04, 0.02, 0.05 };
static const OCIO::GradingRGBM gamma {  0.9,   1.4,  0.7,  0.75 };
static const OCIO::GradingRGBM gain  {  1.2,   1.1,  1.25, 0.8 };
static const OCIO::GradingRGBM offset{ -0.03,  0.02, 0.1, -0.1 };
static constexpr double saturation  =  1.2;
static constexpr double clampBlack = -0.15;
static constexpr double clampWhite =  1.50;
static constexpr double pivotBlack =  0.05;
static constexpr double pivotWhite =  0.9;
static constexpr long num_samples = 2;
const float input_32f[] = {
    0.1f, 0.9f, 1.2f, 1.0f,
    -0.1f, 0.9f, 1.2f, 0.5f };
const float expected_32f[] = {
    -0.10667760f, 0.75643484f, 1.53729499f, 1.0f,
    -0.17148458f, 0.75881552f, 1.53967567f, 0.5f };
const float expected_clamp_32f[] = {
    -0.10667760f, 0.75643484f, 1.50000000f, 1.0f,
    -0.15000000f, 0.75881552f, 1.50000000f, 0.5f };
const float expected_wbpivot_32f[] = {
    -0.06553329f, 0.74984638f, 1.67741281f, 1.0f,
    -0.14759934f, 0.75286107f, 1.68042750f, 0.5f };
}

OCIO_ADD_TEST(GradingPrimaryOpCPU, video)
{
    float res[4 * TS3::num_samples]{ 0.f };

    auto gd = std::make_shared<OCIO::GradingPrimaryOpData>(TS3::style);

    // Test in forward direction.

    OCIO::GradingPrimary gdp(TS3::style);
    gdp.m_lift = OCIO::GradingRGBM(TS3::lift);
    gdp.m_gamma = OCIO::GradingRGBM(TS3::gamma);
    gdp.m_gain = OCIO::GradingRGBM(TS3::gain);
    gdp.m_offset = OCIO::GradingRGBM(TS3::offset);
    gdp.m_saturation = TS3::saturation;

    gd->setValue(gdp);

    OCIO::ConstGradingPrimaryOpDataRcPtr gdc = gd;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS3::input_32f, res, TS3::num_samples));
    ValidateImage(TS3::expected_32f, res, TS3::num_samples, __LINE__);

    gdp.m_clampBlack = TS3::clampBlack;
    gdp.m_clampWhite = TS3::clampWhite;

    gd->setValue(gdp);
    OCIO_CHECK_NO_THROW(op->apply(TS3::input_32f, res, TS3::num_samples));
    ValidateImage(TS3::expected_clamp_32f, res, TS3::num_samples, __LINE__);

    gdp.m_clampBlack = -100;
    gdp.m_clampWhite =  100;
    gdp.m_pivotBlack = TS3::pivotBlack;
    gdp.m_pivotWhite = TS3::pivotWhite;

    gd->setValue(gdp);
    OCIO_CHECK_NO_THROW(op->apply(TS3::input_32f, res, TS3::num_samples));
    ValidateImage(TS3::expected_wbpivot_32f, res, TS3::num_samples, __LINE__);

    // Test in inverse direction.

    gd->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    gdp = OCIO::GradingPrimary(TS3::style);
    gdp.m_lift = OCIO::GradingRGBM(TS3::lift);
    gdp.m_gamma = OCIO::GradingRGBM(TS3::gamma);
    gdp.m_gain = OCIO::GradingRGBM(TS3::gain);
    gdp.m_offset = OCIO::GradingRGBM(TS3::offset);
    gdp.m_saturation = TS3::saturation;

    gd->setValue(gdp);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingPrimaryCPURenderer(gdc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(TS3::expected_32f, res, TS3::num_samples));
    ValidateImage(TS3::input_32f, res, TS3::num_samples, __LINE__);

    // Clamping prevents full inversion. Skip.

    gdp.m_pivotBlack = TS3::pivotBlack;
    gdp.m_pivotWhite = TS3::pivotWhite;

    gd->setValue(gdp);
    OCIO_CHECK_NO_THROW(op->apply(TS3::expected_wbpivot_32f, res, TS3::num_samples));
    ValidateImage(TS3::input_32f, res, TS3::num_samples, __LINE__);
}
