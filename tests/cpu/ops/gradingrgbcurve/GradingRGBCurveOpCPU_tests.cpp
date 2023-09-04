// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradingrgbcurve/GradingRGBCurveOpCPU.cpp"

#include "testutils/UnitTest.h"
#include "utils/StringUtils.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{
void ValidateImage(const float * expected, const float * res, long numPix, unsigned line)
{
#if OCIO_USE_SSE2
    static constexpr float error = 5e-4f;
#else
    static constexpr float error = 2e-5f;
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

OCIO_ADD_TEST(GradingRGBCurveOpCPU, identity)
{
    constexpr long numPixels = 9;
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float image[4 * numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                          0.75f,  1.00f, 1.25f, 1.0f,
                                          1.25f,  1.50f, 1.75f, 0.0f,
                                          qnan,   qnan,  qnan, 0.0f,
                                          0.0f,   0.0f,  0.0f, qnan,
                                          inf,    inf,   inf, 0.0f,
                                          0.0f,   0.0f,  0.0f,  inf,
                                         -inf,   -inf,  -inf, 0.0f,
                                          0.0f,   0.0f,  0.0f, -inf };

    const float expected[4 * numPixels] = { -0.50f, -0.25f, 0.50f, 0.0f,
                                             0.75f,  1.00f, 1.25f, 1.0f,
                                             1.25f,  1.50f, 1.75f, 0.0f,
                                             qnan,   qnan,  qnan, 0.0f,
                                             0.0f,   0.0f,  0.0f, qnan,
                                             inf,    inf,   inf, 0.0f,
                                             0.0f,   0.0f,  0.0f,  inf,
                                            -inf,   -inf,  -inf, 0.0f,
                                             0.0f,   0.0f,  0.0f, -inf };

    float res[4 * numPixels]{ 0.f };

    auto gc = std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_LIN);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingRGBCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains CurveLinearFwdOp.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "CurveLinearFwdOp"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains CurveLinearRevOp.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "CurveLinearRevOp"));
    }
   OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
   ValidateImage(expected, res, numPixels, __LINE__);

    // If BypassLinToLog is true, a Curve*Op renderer rather than a CurveLinear*Op renderer will
    // be used.
    gc->setBypassLinToLog(true);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains CurveRevOp.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "CurveRevOp"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gc->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains CurveFwdOp.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "CurveFwdOp"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    gc = std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_VIDEO);
    gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
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
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    // Check that the right OpCPU is created. Check that class name contains CurveRevOp.
    {
        const OCIO::OpCPU & c = *op;
        std::string typeName = typeid(c).name();
        OCIO_CHECK_NE(std::string::npos, StringUtils::Find(typeName, "CurveRevOp"));
    }
    OCIO_CHECK_NO_THROW(op->apply(image, res, numPixels));
    ValidateImage(expected, res, numPixels, __LINE__);

    // BypassLinToLog is ignored when style is not GRADING_LIN, still creating a CurveRevOp
    // renderer.
    gc->setBypassLinToLog(true);
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
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

OCIO_ADD_TEST(GradingRGBCurveOpCPU, log)
{
    auto rnc = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.15f }, { 0.55f, 0.45f }, { 0.9f, 1.1f } });
    auto gnc = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.15f }, { 0.55f, 0.35f }, { 0.9f, 1.1f } });
    auto bnc = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.15f }, { 0.55f, 0.85f }, { 0.9f, 1.1f } });
    auto mnc = OCIO::GradingBSplineCurve::Create({ { -0.1f, 0.1f }, { 1.1f, 1.3f } });

    OCIO::ConstGradingBSplineCurveRcPtr r = rnc;
    OCIO::ConstGradingBSplineCurveRcPtr g = gnc;
    OCIO::ConstGradingBSplineCurveRcPtr b = bnc;
    OCIO::ConstGradingBSplineCurveRcPtr m = mnc;
    auto gc = std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_LOG, r, g, b, m);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingRGBCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 2;
    float res[4 * num_samples]{ 0.f };

    constexpr float input_32f[] = {
        -0.2f, 0.2f, 0.5f, 0.0f,
         0.8f, 1.0f, 2.0f, 0.5f };

    constexpr float expected_32f[] = {
        0.25306581f, 0.35779659f, 0.98416632f, 0.0f,
        1.09451043f, 1.54596428f, 1.78067802f, 0.5f };

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(expected_32f, res, num_samples));
    ValidateImage(input_32f, res, num_samples, __LINE__);
}

OCIO_ADD_TEST(GradingRGBCurveOpCPU, log_partial_identity)
{
    auto rnc = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.1f },{ 0.9f, 0.9f } });
    auto gnc = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.15f },{ 0.55f, 0.35f },{ 0.9f, 1.1f } });
    auto bnc = OCIO::GradingBSplineCurve::Create({ { 0.f, 0.f },{ 0.5f, 0.5f },{ 1.f, 1.f } });
    auto mnc = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.1f },{ 1.1f, 1.1f } });

    OCIO::ConstGradingBSplineCurveRcPtr r = rnc;
    OCIO::ConstGradingBSplineCurveRcPtr g = gnc;
    OCIO::ConstGradingBSplineCurveRcPtr b = bnc;
    OCIO::ConstGradingBSplineCurveRcPtr m = mnc;
    auto gc = std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_LOG, r, g, b, m);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingRGBCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 2;
    float res[4 * num_samples]{ 0.f };

    float input_32f[] = {
        -0.2f, 0.2f, 0.5f, 0.0f,
         0.8f, 1.0f, 2.0f, 0.5f };

    constexpr float expected_32f[] = {
        -0.2f, 0.15779659f, 0.5f, 0.0f,
         0.8f, 1.34596419f, 2.0f, 0.5f };

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(expected_32f, res, num_samples));
    ValidateImage(input_32f, res, num_samples, __LINE__);
}

OCIO_ADD_TEST(GradingRGBCurveOpCPU, monotonic)
{
    auto rnc = OCIO::GradingBSplineCurve::Create({ { 0.0f,   0.0f },   { 0.785f, 0.231f },
                                                   { 0.809f, 0.631f }, { 0.948f, 0.704f },
                                                   { 1.0f,   1.0f } });
    auto gnc = OCIO::GradingBSplineCurve::Create({ { -0.1f, -0.1f }, { 1.1f, 1.1f } });
    auto bnc = OCIO::GradingBSplineCurve::Create({ { -0.1f, -0.1f }, { 1.1f, 1.1f } });
    auto mnc = OCIO::GradingBSplineCurve::Create({ { -0.1f, -0.1f }, { 1.1f, 1.1f } });

    OCIO::ConstGradingBSplineCurveRcPtr r = rnc;
    OCIO::ConstGradingBSplineCurveRcPtr g = gnc;
    OCIO::ConstGradingBSplineCurveRcPtr b = bnc;
    OCIO::ConstGradingBSplineCurveRcPtr m = mnc;
    auto gc = std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_LOG, r, g, b, m);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingRGBCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 2;
    float res[4 * num_samples]{ 0.f };

    float input_32f[] = {
        0.8f, 0.2f, 0.5f, 0.0f,
        0.9f, 1.0f, 2.0f, 0.5f };

    constexpr float expected_32f[] = {
        0.52230538f, 0.2f, 0.5f, 0.0f,
        0.68079938f, 1.0f, 2.0f, 0.5f };

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(expected_32f, res, num_samples));
    ValidateImage(input_32f, res, num_samples, __LINE__);
}

OCIO_ADD_TEST(GradingRGBCurveOpCPU, lin_bypass)
{
    auto rnc = OCIO::GradingBSplineCurve::Create({ { -6.f, -8.f }, { -2.f, -5.f },
                                                   {  2.f,  4.f }, {  5.f,  6.f } });
    auto gnc = OCIO::GradingBSplineCurve::Create({ { -6.f, -8.f }, { -2.f, -5.f },
                                                   {  2.f,  4.f }, {  5.f,  6.f } });
    auto bnc = OCIO::GradingBSplineCurve::Create({ { -6.f, -8.f }, { -2.f, -5.f },
                                                   {  2.f,  4.f }, {  5.f,  6.f } });
    auto mnc = OCIO::GradingBSplineCurve::Create({ { 0.f, 0.f }, { 0.5f, 0.5f }, { 1.f, 1.f } });

    OCIO::ConstGradingBSplineCurveRcPtr r = rnc;
    OCIO::ConstGradingBSplineCurveRcPtr g = gnc;
    OCIO::ConstGradingBSplineCurveRcPtr b = bnc;
    OCIO::ConstGradingBSplineCurveRcPtr m = mnc;
    auto gc = std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_LIN, r, g, b, m);
    gc->setBypassLinToLog(true);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingRGBCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 2;
    float res[4 * num_samples]{ 0.f };

    float input_32f[] = {
        -8.f, -3.f, -1.f,  0.0f,
         1.f,  2.5f, 4.0f, 0.5f };

    constexpr float expected_32f[] = {
        -8.50508935f, -6.37181915f, -3.01264257f, 0.0f,
         1.95205522f,  4.76796850f,  5.76796850f, 0.5f };

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(expected_32f, res, num_samples));
    ValidateImage(input_32f, res, num_samples, __LINE__);
}

OCIO_ADD_TEST(GradingRGBCurveOpCPU, lin)
{
    auto rnc = OCIO::GradingBSplineCurve::Create({ { -6.f, -8.f }, { -2.f, -5.f },
                                                   {  2.f,  4.f }, {  5.f,  6.f } });
    auto gnc = OCIO::GradingBSplineCurve::Create({ { -6.f, -8.f }, { -2.f, -5.f },
                                                   {  2.f,  4.f }, {  5.f,  6.f } });
    auto bnc = OCIO::GradingBSplineCurve::Create({ { -6.f, -8.f }, { -2.f, -5.f },
                                                   {  2.f,  4.f }, {  5.f,  6.f } });
    auto mnc = OCIO::GradingBSplineCurve::Create({ { 0.f, 0.f }, { 0.5f, 0.5f }, { 1.f, 1.f } });

    OCIO::ConstGradingBSplineCurveRcPtr r = rnc;
    OCIO::ConstGradingBSplineCurveRcPtr g = gnc;
    OCIO::ConstGradingBSplineCurveRcPtr b = bnc;
    OCIO::ConstGradingBSplineCurveRcPtr m = mnc;
    auto gc = std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_LIN, r, g, b, m);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingRGBCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 2;
    float res[4 * num_samples]{ 0.f };

    float input_32f[] = {
        -0.003f, 0.02f, 0.09f, 0.0f,
         0.360f, 1.00f, 3.00f, 0.5f };

    constexpr float expected_32f[] = {
        -4.20784139e-03f, 1.26825221e-03f, 2.23983977e-02f, 0.0f,
         6.96706128e-01f, 4.79411018e+00f, 9.95152432e+00f, 0.5f };

    // Test in forward direction.

    OCIO_CHECK_NO_THROW(op->apply(input_32f, res, num_samples));
    ValidateImage(expected_32f, res, num_samples, __LINE__);

    // Test in inverse direction.

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(expected_32f, res, num_samples));
    ValidateImage(input_32f, res, num_samples, __LINE__);
}

OCIO_ADD_TEST(GradingRGBCurveOpCPU, slopes)
{
    auto curve = OCIO::GradingBSplineCurve::Create({
            {-5.26017743f, -4.f},
            {-3.75502745f, -3.57868829f},
            {-2.24987747f, -1.82131329f},
            {-0.74472749f,  0.68124124f},
            { 1.06145248f,  2.87457742f},
            { 2.86763245f,  3.83406206f},
            { 4.67381243f,  4.f}
        });
    float slopes[] = { 0.f,  0.55982688f,  1.77532247f,  1.55f,  0.8787017f,  0.18374463f,  0.f };
    for (size_t i = 0; i < 7; ++i)
    {
        curve->setSlope( i, slopes[i] );
    }
    OCIO_CHECK_NO_THROW(curve->validate());

    OCIO::ConstGradingBSplineCurveRcPtr m = curve;
    auto identity = OCIO::GradingBSplineCurve::Create({ { 0.f, 0.f }, { 1.f, 1.f } });
    OCIO::ConstGradingBSplineCurveRcPtr z = identity;
    auto gc = std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_LOG, z, z, z, m);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingRGBCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    constexpr long num_samples = 2;
    float input_32f[] = {
        -3.f, -1.f, 1.f, 0.5f,
        -7.f,  0.f, 7.f, 1.0f };

    // Test that the slopes were used (the values are significantly different without slopes).
    constexpr float expected_32f[] = {
        -2.92582282f, 0.28069129f, 2.81987724f, 0.5f,
        -4.0f,        1.73250193f, 4.0f,        1.0f };

    OCIO_CHECK_NO_THROW(op->apply(input_32f, input_32f, num_samples));
    ValidateImage(expected_32f, input_32f, num_samples, __LINE__);

    // Test in inverse direction.

    float rev_input_32f[] = {
        -2.92582282f, 0.28069129f, 2.81987724f, 0.5f,
        -7.0f,        1.73250193f, 7.0f,        1.0f };

    constexpr float rev_expected_32f[] = {
        -3.f,        -1.f, 1.f,         0.5f,
        -5.26017743f, 0.f, 4.67381243f, 1.0f };

    gc->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);
    OCIO_CHECK_NO_THROW(op->apply(rev_input_32f, rev_input_32f, num_samples));
    ValidateImage(rev_expected_32f, rev_input_32f, num_samples, __LINE__);
}
