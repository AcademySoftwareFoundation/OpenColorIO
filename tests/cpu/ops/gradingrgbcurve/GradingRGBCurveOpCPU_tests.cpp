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
#ifdef USE_SSE
    static constexpr float error = 5e-4f;
#else
    static constexpr float error = 1e-6f;
#endif // USE_SSE

    for (long i = 0; i < numPix; ++i)
    {
        for (long j = 0; j < 4; ++j)
        {
            if (OCIO::IsNan(expected[i * 4 + j]))
            {
#ifdef USE_SSE
                // Do not test nan in SSE mode.
#else
                OCIO_CHECK_ASSERT(OCIO::IsNan(res[i * 4 + j]));
#endif // USE_SSE
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
    // TODO: implement inverse.

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
    // TODO: implement inverse.

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
    // TODO: implement inverse.

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
    // TODO: implement inverse.
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

    const long num_samples = 2;

    float input_32f[] = {
        -0.2f, 0.2f, 0.5f, 0.0f,
         0.8f, 1.0f, 2.0f, 0.5f };

    const float expected_32f[] = {
        0.25306581f, 0.35779659f, 0.98416632f, 0.0f,
        1.09451043f, 1.54596428f, 1.78067802f, 0.5f };

    OCIO_CHECK_NO_THROW(op->apply(input_32f, input_32f, num_samples));
    ValidateImage(expected_32f, input_32f, num_samples, __LINE__);
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

    const long num_samples = 2;

    float input_32f[] = {
        -0.2f, 0.2f, 0.5f, 0.0f,
         0.8f, 1.0f, 2.0f, 0.5f };

    const float expected_32f[] = {
        -0.2f, 0.15779659f, 0.5f, 0.0f,
         0.8f, 1.34596419f, 2.0f, 0.5f };

    OCIO_CHECK_NO_THROW(op->apply(input_32f, input_32f, num_samples));
    ValidateImage(expected_32f, input_32f, num_samples, __LINE__);
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

    const long num_samples = 2;

    float input_32f[] = {
        0.8f, 0.2f, 0.5f, 0.0f,
        0.9f, 1.0f, 2.0f, 0.5f };

    const float expected_32f[] = {
        0.52230538f, 0.2f, 0.5f, 0.0f,
        0.68079938f, 1.0f, 2.0f, 0.5f };

    OCIO_CHECK_NO_THROW(op->apply(input_32f, input_32f, num_samples));
    ValidateImage(expected_32f, input_32f, num_samples, __LINE__);
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

    const long num_samples = 2;

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

    float input_32f[] = {
        -8.f, -3.f, -1.f, 0.0f,
         1.f, 2.5f, 4.0f, 0.5f };

    const float expected_32f[] = {
        -8.50508935f, -6.37181915f, -3.01264257f, 0.0f,
         1.95205522f,  4.76796850f,  5.76796850f, 0.5f };

    OCIO_CHECK_NO_THROW(op->apply(input_32f, input_32f, num_samples));
    ValidateImage(expected_32f, input_32f, num_samples, __LINE__);
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

    const long num_samples = 2;

    OCIO::ConstGradingBSplineCurveRcPtr r = rnc;
    OCIO::ConstGradingBSplineCurveRcPtr g = gnc;
    OCIO::ConstGradingBSplineCurveRcPtr b = bnc;
    OCIO::ConstGradingBSplineCurveRcPtr m = mnc;
    auto gc = std::make_shared<OCIO::GradingRGBCurveOpData>(OCIO::GRADING_LIN, r, g, b, m);
    OCIO::ConstOpCPURcPtr op;
    OCIO::ConstGradingRGBCurveOpDataRcPtr gcc = gc;
    OCIO_CHECK_NO_THROW(op = OCIO::GetGradingRGBCurveCPURenderer(gcc));
    OCIO_CHECK_ASSERT(op);

    float input_32f[] = {
        -0.003f, 0.02f, 0.09f, 0.0f,
         0.360f, 1.00f, 3.00f, 0.5f };

    const float expected_32f[] = {
        -4.20784139e-03f, 1.26825221e-03f, 2.23983977e-02f, 0.0f,
         6.96706128e-01f, 4.79411018e+00f, 9.95152432e+00f, 0.5f };

    OCIO_CHECK_NO_THROW(op->apply(input_32f, input_32f, num_samples));
    ValidateImage(expected_32f, input_32f, num_samples, __LINE__);
}

// TODO: implement inverse.
