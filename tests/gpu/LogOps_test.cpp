// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include <math.h>

#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


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
                OCIO::TransformDirection direction,
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

    AddLogTest(test, shaderDesc, OCIO::TRANSFORM_DIR_FORWARD, base10, g_epsilon);

    // TODO: Would like to be able to remove the setTestNaN(false) from all of these tests.
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_legacy_inverse)
{
    OCIO::GpuShaderDescRcPtr shaderDesc
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, OCIO::TRANSFORM_DIR_INVERSE, base10, g_epsilon_inverse);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_generic_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, OCIO::TRANSFORM_DIR_FORWARD, base10, g_epsilon);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_inverse_generic_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, OCIO::TRANSFORM_DIR_INVERSE, base10, g_epsilon_inverse);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_legacy)
{
    OCIO::GpuShaderDescRcPtr shaderDesc
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, OCIO::TRANSFORM_DIR_FORWARD, eulerConstant, g_epsilon);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_legacy_inverse)
{
    OCIO::GpuShaderDescRcPtr shaderDesc
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, OCIO::TRANSFORM_DIR_INVERSE, eulerConstant, g_epsilon_inverse);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_generic_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, OCIO::TRANSFORM_DIR_FORWARD, eulerConstant, g_epsilon);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_inverse_generic_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, OCIO::TRANSFORM_DIR_INVERSE, eulerConstant, g_epsilon_inverse);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, base)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    log->setBase(base10);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, base_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
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
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

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
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

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
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

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
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

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
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

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
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

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
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

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
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double logSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLogSideOffsetValue(logSideOffset);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(log->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(g_epsilon);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}
