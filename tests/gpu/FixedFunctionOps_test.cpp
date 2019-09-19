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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_darktodim10_inv)
{
    OCIO::FixedFunctionTransformRcPtr func = OCIO::FixedFunctionTransform::Create();
    func->setStyle(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(func->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);

    test.setTestInfinity(false);
}
