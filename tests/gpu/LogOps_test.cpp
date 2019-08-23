/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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

#include <math.h>


namespace OCIO = OCIO_NAMESPACE;
#include "GPUUnitTest.h"

OCIO_NAMESPACE_USING


const int LUT3D_EDGE_SIZE = 64;

// TODO: Once this project is aware of SSE mode the non SSE errors can be restored.
// const float g_epsilon = 1e-5f;
// const float g_epsilon_inverse = 1e-4f;
const float g_epsilon = 1e-4f;
const float g_epsilon_inverse = 1e-3f;

const float base10 = 10.0f;
const float eulerConstant = expf(1.0f);


// Helper method to build unit tests
void AddLogTest(OCIOGPUTest & test, 
                OCIO::GpuShaderDescRcPtr & shaderDesc,
                TransformDirection direction,
                float base, 
                float epsilon)
{
    OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();
    log->setDirection(direction);
    log->setBase(base);

    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(epsilon);

    // TODO: Would like to be able to remove the setTestNaN(false) and
    // setTestInfinity(false) from all of these tests.
    test.setTestInfinity(false);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_legacy)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, base10, g_epsilon);

    // TODO: Would like to be able to remove the setTestNaN(false) from all of these tests.
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_legacy_inverse)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, base10, g_epsilon_inverse);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_generic_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, base10, g_epsilon);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_inverse_generic_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, base10, g_epsilon_inverse);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_legacy)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, eulerConstant, g_epsilon);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_legacy_inverse)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, eulerConstant, g_epsilon_inverse);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_generic_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, eulerConstant, g_epsilon);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_inverse_generic_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, eulerConstant, g_epsilon_inverse);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, base)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(TRANSFORM_DIR_FORWARD);
    log->setBase(base10);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, base_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(TRANSFORM_DIR_INVERSE);
    log->setBase(base10);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setRelativeComparison(true);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, linSideSlope)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(TRANSFORM_DIR_FORWARD);

    const double linSideSlope[3] = {2.0, 0.5, 3.0};
    log->setLinSideSlopeValue(linSideSlope);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, linSideSlope_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(TRANSFORM_DIR_INVERSE);

    const double linSideSlope[3] = { 2.0, 0.5, 3.0 };
    log->setLinSideSlopeValue(linSideSlope);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, linSideOffset)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(TRANSFORM_DIR_FORWARD);

    const double linSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLinSideOffsetValue(linSideOffset);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, linSideOffset_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(TRANSFORM_DIR_INVERSE);

    const double linSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLinSideOffsetValue(linSideOffset);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, logSideSlope)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(TRANSFORM_DIR_FORWARD);

    const double logSideSlope[3] = { 2.0, 0.5, 3.0 };
    log->setLogSideSlopeValue(logSideSlope);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, logSideSlope_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(TRANSFORM_DIR_INVERSE);

    const double logSideSlope[3] = { 2.0, 0.5, 3.0 };
    log->setLogSideSlopeValue(logSideSlope);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, logSideOffset)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(TRANSFORM_DIR_FORWARD);

    const double logSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLogSideOffsetValue(logSideOffset);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, logSideOffset_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(TRANSFORM_DIR_INVERSE);

    const double logSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLogSideOffsetValue(logSideOffset);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}
