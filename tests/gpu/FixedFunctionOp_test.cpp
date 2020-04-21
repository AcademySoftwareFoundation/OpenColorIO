// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_GPU_TEST(FixedFunction, style_aces_redmod03_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.90f,  0.05f,   0.22f,   0.50f,
            0.97f,  0.097f,  0.0097f, 1.0f,
            0.89f,  0.15f,   0.56f,   0.0f,
           -1.0f,  -0.001f,  1.2f,    0.0f
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_redmod03_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.90f,  0.05f,   0.22f,   0.50f,
            0.97f,  0.097f,  0.0097f, 1.0f,
            0.89f,  0.15f,   0.56f,   0.0f,
           -1.0f,  -0.001f,  1.2f,    0.0f
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_redmod10_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.90f,  0.05f,   0.22f,   0.50f,
            0.97f,  0.097f,  0.0097f, 1.0f,
            0.89f,  0.15f,   0.56f,   0.0f,
           -1.0f,  -0.001f,  1.2f,    0.0f
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_redmod10_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.90f,  0.05f,   0.22f,   0.50f,
            0.97f,  0.097f,  0.0097f, 1.0f,
            0.89f,  0.15f,   0.56f,   0.0f,
           -1.0f,  -0.001f,  1.2f,    0.0f
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_glow03_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.11f, 0.02f, 0.f,   0.5f, // YC = 0.10
            0.01f, 0.02f, 0.03f, 1.0f, // YC = 0.03
            0.11f, 0.91f, 0.01f, 0.f,  // YC = 0.84
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-7f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_glow03_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.11f, 0.02f, 0.f,   0.5f, // YC = 0.10
            0.01f, 0.02f, 0.03f, 1.0f, // YC = 0.03
            0.11f, 0.91f, 0.01f, 0.f,  // YC = 0.84
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-7f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_glow10_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.11f, 0.02f, 0.f,   0.5f, // YC = 0.10
            0.01f, 0.02f, 0.03f, 1.0f, // YC = 0.03
            0.11f, 0.91f, 0.01f, 0.f,  // YC = 0.84
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-7f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_glow10_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.11f, 0.02f, 0.f,   0.5f, // YC = 0.10
            0.01f, 0.02f, 0.03f, 1.0f, // YC = 0.03
            0.11f, 0.91f, 0.01f, 0.f,  // YC = 0.84
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-7f);
}

// The next four tests run into a problem on some graphics cards where 0.0 * Inf = 0.0,
// rather than the correct value of NaN.  Therefore turning off TestInfinity for these tests.

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_darktodim10_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_darktodim10_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_rec2100_surround_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_REC2100_SURROUND);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    const double data[1] = { 0.7 };
    func->setParams(&data[0], 1);

    test.setProcessor(func);

    test.setErrorThreshold(2e-6f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_rec2100_surround_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_REC2100_SURROUND);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    // (Since we're not calling inverse() here, set the param to be the inverse of prev test.)
    const double data[1] = { 1./0.7 };
    func->setParams(&data[0], 1);

    test.setProcessor(func);

    test.setErrorThreshold(2e-6f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_RGB_TO_HSV_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);

#ifdef __APPLE__
    test.setTestNaN(false);
    test.setTestInfinity(false);
#endif
}

OCIO_ADD_GPU_TEST(FixedFunction, style_RGB_TO_HSV_fwd_custom)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            1.500f,   2.500f,   0.500f,   0.50f,
            3.125f,  -0.625f,   1.250f,   1.00f,
           -5.f/3.f, -4.f/3.f, -1.f/3.f,  0.25f,
            0.100f,  -0.800f,   0.400f,   0.25f,
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_RGB_TO_HSV_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    // Mostly passes at 1e-6 but there are some values where the result is large in one chan
    // and very small in another and so doing relative comparison per channel is not fair.
    test.setErrorThreshold(5e-4f);
    test.setExpectedMinimalValue(5e-3f);
    test.setRelativeComparison(true);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_RGB_TO_HSV_inv_custom)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
             3.f/12.f,  0.80f,  2.50f,  0.50f,      // val > 1
            11.f/12.f,  1.20f,  2.50f,  1.00f,      // sat > 1
            15.f/24.f,  0.80f, -2.00f,  0.25f,      // val < 0
            19.f/24.f,  1.50f, -0.40f,  0.25f,      // sat > 1, val < 0
           -89.f/24.f,  0.50f,  0.40f,  2.00f,      // under-range hue
            81.f/24.f,  1.50f, -0.40f, -0.25f,      // over-range hue, sat > 1, val < 0
            81.f/24.f, -0.50f,  0.40f,  0.00f,      // sat < 0
              0.5000f,  2.50f,  0.04f,  0.00f,      // sat > 2
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_xyY_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_XYZ_TO_xyY);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_xyY_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_XYZ_TO_xyY);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_uvY_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_XYZ_TO_uvY);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_uvY_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_XYZ_TO_uvY);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    test.setErrorThreshold(1e-5f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_LUV_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_XYZ_TO_LUV);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setErrorThreshold(1e-5f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_LUV_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_XYZ_TO_LUV);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    test.setErrorThreshold(1e-5f);
}
