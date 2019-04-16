/*
Copyright (c) 2018 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO = OCIO_NAMESPACE;
#include "GPUUnitTest.h"

OCIO_NAMESPACE_USING


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
