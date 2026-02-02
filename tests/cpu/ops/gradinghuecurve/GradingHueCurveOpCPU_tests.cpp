// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradinghuecurve/GradingHueCurveOpCPU.cpp"

#include "testutils/UnitTest.h"
#include "utils/StringUtils.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{
void ValidateImage(const float * expected, const float * res, long numPix, unsigned line)
{
    static constexpr float error = 2e-5f;

    for (long i = 0; i < numPix; ++i)
    {
        for (long j = 0; j < 4; ++j)
        {
            if (OCIO::IsNan(expected[i * 4 + j]))
            {
                OCIO_CHECK_ASSERT(OCIO::IsNan(res[i * 4 + j]));
            }
            else if (expected[i * 4 + j] != res[i * 4 + j])
            {
                OCIO_CHECK_CLOSE_FROM(expected[i * 4 + j], res[i * 4 + j], error, line);
            }
        }
    }
}
}  // anon

OCIO_ADD_TEST(GradingHueCurveOpCPU, identity)
{
    constexpr long numPixels = 9;
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float image[4 * numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                          0.75f,  1.00f, 1.25f, 1.0f,
                                          1.25f,  1.50f, 1.75f, 0.0f,
                                          0.0f,   0.0f,  0.0f, qnan,
                                          0.0f,   0.0f,  0.0f,  inf,
                                          0.0f,   0.0f,  0.0f, -inf };

    const float expected[4 * numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                             0.75f,  1.00f, 1.25f, 1.0f,
                                             1.25f,  1.50f, 1.75f, 0.0f,
                                             0.0f,   0.0f,  0.0f, qnan,
                                             0.0f,   0.0f,  0.0f,  inf,
                                             0.0f,   0.0f,  0.0f, -inf };

    // TODO: Nan/Inf handling

    float res[4 * numPixels]{ 0.f };

    auto gc = std::make_shared<OCIO::GradingHueCurveOpData>(OCIO::GRADING_LIN);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingHueCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    // Check that the right OpCPU is created. Check that class name contains CurveFwdOp.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "CurveFwdOp"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    // Check that the right OpCPU is created. Check that class name contains CurveRevOp.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "CurveRevOp"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);
}

OCIO_ADD_TEST(GradingHueCurveOpCPU, log_identity)
{
    // Identity curves (for log or video) that are different from the default curves.
    auto hh = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.f}, {0.1f, 0.1f}, {0.2f, 0.2f}, {0.4f, 0.4f}, {0.6f, 0.6f}, {0.8f, 0.8f} },
        OCIO::HUE_HUE);
    auto hs = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.f}, {0.1f, 1.f}, {0.2f, 1.f}, {0.4f, 1.f}, {0.6f, 1.f}, {0.8f, 1.f} },
        OCIO::HUE_SAT);
    auto hl = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.f}, {0.1f, 1.f}, {0.2f, 1.f}, {0.4f, 1.f}, {0.6f, 1.f}, {0.8f, 1.f} },
        OCIO::HUE_LUM);
    auto ls = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.f}, {1.f, 1.f} }, 
        OCIO::LUM_SAT);
    auto ss = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.f}, {0.25f, 0.25f}, {1.f, 1.f} },
        OCIO::SAT_SAT);
    auto ll = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.f}, {0.25f, 0.25f}, {0.5f, 0.5f}, {1.f, 1.f} },
        OCIO::LUM_LUM);
    auto sl = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.f}, {0.25f, 1.f}, {0.5f, 1.f}, {1.f, 1.f} },
        OCIO::SAT_LUM);
    auto hfx = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.f}, {0.1f, 0.f}, {0.2f, 0.f}, {0.4f, 0.f}, {0.6f, 0.f}, {0.8f, 0.f} },
        OCIO::HUE_FX);

    auto gc = std::make_shared<OCIO::GradingHueCurveOpData>(OCIO::GRADING_LOG, 
        hh, hs, hl, ls, ss, ll, sl, hfx);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingHueCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 2;
    float res[4 * num_samples]{ 0.f };

    constexpr float input_32f[] = {
        -0.2f, 0.2f, 0.5f, 0.0f,
         0.8f, 1.0f, 2.0f, 0.5f };

    constexpr float expected_32f[] = {
        -0.2f, 0.2f, 0.5f, 0.0f,
         0.8f, 1.0f, 2.0f, 0.5f };

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(expected_32f, res, num_samples));
    ValidateImage(input_32f, res, num_samples, __LINE__);
}

OCIO_ADD_TEST(GradingHueCurveOpCPU, hh_hfx_curves)
{
    // Validate that the hue curves round-trip.

    // Identity curves.
    auto hs = OCIO::GradingBSplineCurve::Create({ {0.f, 1.f}, {0.9f, 1.f} }, OCIO::HUE_SAT);
    auto hl = OCIO::GradingBSplineCurve::Create({ {0.f, 1.f}, {0.9f, 1.f} }, OCIO::HUE_LUM);
    auto ls = OCIO::GradingBSplineCurve::Create({ {0.f, 1.f}, {0.9f, 1.f} }, OCIO::LUM_SAT);
    auto ss = OCIO::GradingBSplineCurve::Create({ {0.f, 0.f}, {0.9f, 0.9f} }, OCIO::SAT_SAT);
    auto ll = OCIO::GradingBSplineCurve::Create({ {0.f, 0.f}, {0.9f, 0.9f} }, OCIO::LUM_LUM);
    auto sl = OCIO::GradingBSplineCurve::Create({ {0.f, 1.f}, {0.9f, 1.f} }, OCIO::SAT_LUM);

    // Set hh and hfx to non-identities.
    auto hh = OCIO::GradingBSplineCurve::Create(
        { {0.05f, 0.15f}, {0.2f, 0.3f}, {0.35f, 0.4f}, {0.45f, 0.45f}, {0.6f, 0.7f}, {0.8f, 0.85f} },
        OCIO::HUE_HUE);
    auto hfx = OCIO::GradingBSplineCurve::Create(
        { {0.2f, 0.05f}, {0.4f, -0.09f}, {0.6f, -0.2f}, { 0.8f, 0.05f}, {0.99f, -0.02f} },
        OCIO::HUE_FX);

    auto gc = std::make_shared<OCIO::GradingHueCurveOpData>(OCIO::GRADING_LOG, 
        hh, hs, hl, ls, ss, ll, sl, hfx);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingHueCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 4;
    float res[4 * num_samples]{ 0.f };

    const float input_32f[] = {
    0.1f, 0.5f, 0.7f, 0.0f,
    0.6f, 0.9f, 0.8f, 0.5f,
    0.4f, 0.35f, 0.3f, 0.f,
    0.0f, 0.0f, 0.0f, 1.0f
    };

    const float expected_32f[] = {
    0.3984785676f,  0.3790940642f,  1.0187726020f,  0.0f,
    0.6117081642f,  0.8883015513f,  0.8814064860f,  0.5f,
    0.3847683966f,  0.3567464352f,  0.2780219615f,  0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
    }; 

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(res, res, num_samples));
    ValidateImage(input_32f, res, num_samples, __LINE__);
}

OCIO_ADD_TEST(GradingHueCurveOpCPU_1, log_all_curves)
{
    // All curves are non-identities.
    auto hh = OCIO::GradingBSplineCurve::Create(
        { {0.05f, 0.15f}, {0.2f, 0.3f}, {0.35f, 0.4f}, {0.45f, 0.45f}, {0.6f, 0.7f}, {0.8f, 0.85f} },
        OCIO::HUE_HUE);
    auto hs = OCIO::GradingBSplineCurve::Create(
        { {-0.1f, 1.2f}, {0.2f, 0.7f}, {0.4f, 1.5f}, {0.5f, 0.5f}, {0.6f, 1.4f}, {0.8f, 0.7f} }, 
        OCIO::HUE_SAT);
    auto hl = OCIO::GradingBSplineCurve::Create(
        { {0.1f, 1.5f}, {0.2f, 0.7f}, {0.4f, 1.4f}, {0.5f, 0.8f}, {0.8f, 0.5f} }, 
        OCIO::HUE_LUM);
    auto ls = OCIO::GradingBSplineCurve::Create(
        { {0.05f, 1.5f}, {0.5f, 0.9f}, {1.1f, 1.4f} }, 
        OCIO::LUM_SAT);
    auto ss = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.1f}, {0.5f, 0.45f}, {1.f, 1.1f} },
        OCIO::SAT_SAT);
    auto ll = OCIO::GradingBSplineCurve::Create(
        { {-0.02f, -0.04f}, {0.2f, 0.1f}, {0.8f, 0.95f}, {1.1f, 1.2f} },
        OCIO::LUM_LUM);
    auto sl = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.2f}, {0.6f, 0.8f}, {0.9f, 1.1f} },
        OCIO::SAT_LUM);
    auto hfx = OCIO::GradingBSplineCurve::Create(
        { {0.2f, 0.05f}, {0.4f, -0.09f}, {0.6f, -0.2f}, { 0.8f, 0.05f}, {0.99f, -0.02f} }, 
        OCIO::HUE_FX);

    auto gc = std::make_shared<OCIO::GradingHueCurveOpData>(OCIO::GRADING_LOG, 
        hh, hs, hl, ls, ss, ll, sl, hfx);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingHueCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 5;
    float res[4 * num_samples]{ 0.f };

    const float input_32f[] = {
    0.1f, 0.5f, 0.7f, 0.0f,
    0.6f, 0.9f, 0.8f, 0.5f,
    0.4f, 0.35f,0.3f, 0.f,
    0.4f,-0.2f,-0.05f, 0.f,    
    0.0f, 0.0f, 0.0f, 1.0f
    };

    const float expected_32f[] = {
    0.651269494808f,  0.630018105394f,  1.331314732772f,  0.0f,
    0.787401154155f,  1.286561695129f,  1.274118545611f,  0.5f, 
    0.317389674917f,  0.297787779440f,  0.242718507572f,  0.0f,
    0.830653473122f,  0.449246419743f, -0.173027078802f,  0.0f,
    0.004989255546f, -0.033773428950f, -0.019725339077f,  1.0f
    }; 

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(res, res, num_samples));
    ValidateImage(input_32f, res, num_samples, __LINE__);
}

OCIO_ADD_TEST(GradingHueCurveOpCPU, lin_all_curves)
{
    // All curves are non-identities.
    auto hh = OCIO::GradingBSplineCurve::Create(
        { {0.05f, 0.15f}, {0.2f, 0.3f}, {0.35f, 0.4f}, {0.45f, 0.45f}, {0.6f, 0.7f}, {0.8f, 0.85f} },
        OCIO::HUE_HUE);
    auto hs = OCIO::GradingBSplineCurve::Create(
        { {-0.1f, 1.2f}, {0.2f, 0.7f}, {0.4f, 1.5f}, {0.5f, 0.5f}, {0.6f, 1.4f}, {0.8f, 0.7f} }, 
        OCIO::HUE_SAT);
    auto hl = OCIO::GradingBSplineCurve::Create(
        { {0.1f, 1.5f}, {0.2f, 0.7f}, {0.4f, 1.4f}, {0.5f, 0.8f}, {0.8f, 0.5f} }, 
        OCIO::HUE_LUM);
    auto ss = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.1f}, {0.5f, 0.45f}, {1.f, 1.1f} },
        OCIO::SAT_SAT);
    auto sl = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.2f}, {0.6f, 0.8f}, {0.9f, 1.1f} },
        OCIO::SAT_LUM);
    auto hfx = OCIO::GradingBSplineCurve::Create(
        { {0.2f, 0.05f}, {0.4f, -0.09f}, {0.6f, -0.2f}, { 0.8f, 0.05f}, {0.99f, -0.02f} }, 
        OCIO::HUE_FX);
    // Adjust these two, relative to previous test, to work in f-stops.
    auto ls = OCIO::GradingBSplineCurve::Create(
        { {-6.f, 0.9f}, {-3.f, 0.8f}, {0.f, 1.2f}, {2.f, 1.f}, {4.f, 0.6f}, {6.f, 0.55f} }, 
        OCIO::LUM_SAT);
    auto ll = OCIO::GradingBSplineCurve::Create(
        { {-8.f, -7.f}, {-2.f, -3.f}, {2.f, 3.5f}, {8.f, 7.f} },
        OCIO::LUM_LUM);

    auto gc = std::make_shared<OCIO::GradingHueCurveOpData>(OCIO::GRADING_LIN, 
        hh, hs, hl, ls, ss, ll, sl, hfx);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingHueCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 5;
    float res[4 * num_samples]{ 0.f };

    const float input_32f[] = {
    0.1f, 0.5f, 0.7f, 0.0f,
    0.6f, 0.9f, 0.8f, 0.5f,
    2.4f, 2.35f, 2.3f, 0.f,
    0.4f, 0.2f, -0.05f, 0.f,
    0.0f, 0.0f, 0.0f, 1.0f
    };

    const float expected_32f[] = {
    0.527229344453f,  0.490778616791f,  1.693653961874f,  0.f,
    1.253512415394f,  2.240034381083f,  2.215442212203f,  0.5f,
    6.983003751281f,  6.772174817271f,  6.179875164501f,  0.f,
    0.527554073346f,  0.360480028655f, -0.135388205576f,  0.f,
    0.011308048228f, -0.001711436982f,  0.003006990049f,  1.f
    }; 

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(res, res, num_samples));
    ValidateImage(input_32f, res, num_samples, __LINE__);
}

OCIO_ADD_TEST(GradingHueCurveOpCPU, draw_curve_only)
{
    auto gc = std::make_shared<OCIO::GradingHueCurveOpData>(OCIO::GRADING_LOG);

    auto val = gc->getValue()->createEditableCopy();
    auto spline = val->getCurve(OCIO::HUE_SAT);
    spline->getControlPoint(1) = OCIO::GradingControlPoint(0.15f, 1.4f);
    OCIO_CHECK_ASSERT(!val->isIdentity());

    // Enable drawCurveOnly mode.  This should only evaluate the HUE-SAT spline for
    // use in a user interface.
    val->setDrawCurveOnly(true);
    gc->setValue(val);

    OCIO::ConstGradingHueCurveOpDataRcPtr gcc = gc;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 2;
    float res[4 * num_samples]{ 0.f };

    constexpr float input_32f[] = {
        -0.2f, 0.15f, 0.15f, 0.0f,
         0.15f, 1.0f, 2.0f, 0.5f };

    constexpr float expected_32f[] = {
         1.0f, 1.4f, 1.4f, 0.0f,
         1.4f, 1.0f, 1.0f, 0.5f };

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction, which should be the same as the forward direction
    // since the direction is ignored for DrawCurveOnly mode.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);
}

OCIO_ADD_TEST(GradingHueCurveOpCPU, bypass_rgbtohsy)
{
    auto gc = std::make_shared<OCIO::GradingHueCurveOpData>(OCIO::GRADING_LOG);

    gc->setRGBToHSY(OCIO::HSY_TRANSFORM_NONE);

    auto val = gc->getValue()->createEditableCopy();
    auto spline = val->getCurve(OCIO::SAT_SAT);
    spline->getControlPoint(1) = OCIO::GradingControlPoint(0.4f, 0.8f);
    OCIO_CHECK_ASSERT(!val->isIdentity());
    gc->setValue(val);

    OCIO::ConstGradingHueCurveOpDataRcPtr gcc = gc;
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 2;
    float res[4 * num_samples]{ 0.f };

    constexpr float input_32f[] = {
         0.2f, 0.2f, -0.1f, 0.0f,
         0.1f, 0.4f,  2.0f, 0.5f };

    // Only the green channel gets processed, using the sat-sat curve.
    constexpr float expected_32f[] = {
         0.2f, 0.4475418f, -0.1f, 0.0f,
         0.1f, 0.8000000f,  2.0f, 0.5f };

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingHueCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(res, res, num_samples));
    ValidateImage(input_32f, res, num_samples, __LINE__);
}
