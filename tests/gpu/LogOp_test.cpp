// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include <math.h>

#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


#if OCIO_USE_SSE2
const float g_epsilon = 1e-4f;
const float g_epsilon_inverse = 1e-3f;
#else
const float g_epsilon = 1e-5f;
const float g_epsilon_inverse = 1e-4f;
#endif

const float base10 = 10.0f;
const float eulerConstant = expf(1.0f);


// Helper method to build unit tests
void AddLogTest(OCIOGPUTest & test,
                OCIO::TransformDirection direction,
                float base,
                float epsilon)
{
    OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();
    log->setDirection(direction);
    log->setBase(base);

    test.setProcessor(log);

    test.setErrorThreshold(epsilon);

    // TODO: Would like to be able to remove the setTestNaN(false) and
    // setTestInfinity(false) from all of these tests.
    test.setTestInfinity(false);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_legacy)
{
    AddLogTest(test, OCIO::TRANSFORM_DIR_FORWARD, base10, g_epsilon);

    test.setLegacyShader(true);
    // TODO: Would like to be able to remove the setTestNaN(false) from all of these tests.
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_legacy_inverse)
{
    AddLogTest(test, OCIO::TRANSFORM_DIR_INVERSE, base10, g_epsilon_inverse);

    test.setLegacyShader(true);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_generic_shader)
{
    AddLogTest(test, OCIO::TRANSFORM_DIR_FORWARD, base10, g_epsilon);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_inverse_generic_shader)
{
    AddLogTest(test, OCIO::TRANSFORM_DIR_INVERSE, base10, g_epsilon_inverse);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_legacy)
{
    AddLogTest(test, OCIO::TRANSFORM_DIR_FORWARD, eulerConstant, g_epsilon);

    test.setLegacyShader(true);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_legacy_inverse)
{
    AddLogTest(test, OCIO::TRANSFORM_DIR_INVERSE, eulerConstant, g_epsilon_inverse);

    test.setLegacyShader(true);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_generic_shader)
{
    AddLogTest(test, OCIO::TRANSFORM_DIR_FORWARD, eulerConstant, g_epsilon);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_inverse_generic_shader)
{
    AddLogTest(test, OCIO::TRANSFORM_DIR_INVERSE, eulerConstant, g_epsilon_inverse);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, base)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    log->setBase(base10);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, base_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    log->setBase(base10);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon);  // Use the tighter tolerance than g_epsilon_inverse

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

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, linSideSlope_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double linSideSlope[3] = { 2.0, 0.5, 3.0 };
    log->setLinSideSlopeValue(linSideSlope);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon);  // Use the tighter tolerance than g_epsilon_inverse

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, linSideOffset)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    const double linSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLinSideOffsetValue(linSideOffset);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, linSideOffset_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double linSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLinSideOffsetValue(linSideOffset);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon);  // Use the tighter tolerance than g_epsilon_inverse

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, logSideSlope)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    const double logSideSlope[3] = { 2.0, 0.5, 3.0 };
    log->setLogSideSlopeValue(logSideSlope);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon * 5.0f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, logSideSlope_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double logSideSlope[3] = { 2.0, 0.5, 3.0 };
    log->setLogSideSlopeValue(logSideSlope);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon);  // Use the tighter tolerance than g_epsilon_inverse

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, logSideOffset)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    const double logSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLogSideOffsetValue(logSideOffset);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, logSideOffset_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double logSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLogSideOffsetValue(logSideOffset);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon);  // Use the tighter tolerance than g_epsilon_inverse

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, lin2log)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    const double logSideSlope[3] = { 0.2, 0.4, 0.25 };
    log->setLogSideSlopeValue(logSideSlope);
    const double logSideOffset[3] = { 0.14, 0.13, 0.12 };
    log->setLogSideOffsetValue(logSideOffset);
    const double linSideSlope[3] = { 1.5, 1.8, 1.2 };
    log->setLinSideSlopeValue(linSideSlope);
    const double linSideOffset[3] = { 0.05, 0.1, 0.15 };
    log->setLinSideOffsetValue(linSideOffset);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon * 5.0f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogAffineTransform, log2lin)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double logSideSlope[3] = { 0.21, 0.2, 0.19 };
    log->setLogSideSlopeValue(logSideSlope);
    const double logSideOffset[3] = { 0.61, 0.6, 0.59 };
    log->setLogSideOffsetValue(logSideOffset);
    const double linSideSlope[3] = { 1.11, 1.1, 1.12 };
    log->setLinSideSlopeValue(linSideSlope);
    const double linSideOffset[3] = { 0.051, 0.05, 0.052 };
    log->setLinSideOffsetValue(linSideOffset);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon_inverse);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(LogCameraTransform, camera_lin2log)
{
    const double linSideBreak[3] = { 0.12, 0.13, 0.15 };
    OCIO::LogCameraTransformRcPtr log = OCIO::LogCameraTransform::Create(linSideBreak);
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    const double logSideSlope[3] = { 0.2, 0.3, 0.4 };
    log->setLogSideSlopeValue(logSideSlope);
    const double logSideOffset[3] = { 0.7, 0.6, 0.5 };
    log->setLogSideOffsetValue(logSideOffset);
    const double linSideSlope[3] = { 1.4, 1.1, 1.2 };
    log->setLinSideSlopeValue(linSideSlope);
    const double linSideOffset[3] = { 0.15, 0.16, 0.25 };
    log->setLinSideOffsetValue(linSideOffset);
    const double linearSlope[3] = { 1.22, 1.33, 1.44 };
    log->setLinearSlopeValue(linearSlope);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon);

#ifdef __APPLE__
    test.setTestNaN(false);
    test.setTestInfinity(false);
#endif
}

OCIO_ADD_GPU_TEST(LogCameraTransform, camera_log2lin)
{
    const double linSideBreak[3] = { 0.12, 0.13, 0.14 };
    OCIO::LogCameraTransformRcPtr log = OCIO::LogCameraTransform::Create(linSideBreak);
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    const double logSideSlope[3] = { 0.21, 0.22, 0.23 };
    log->setLogSideSlopeValue(logSideSlope);
    const double logSideOffset[3] = { 0.6, 0.7, 0.8 };
    log->setLogSideOffsetValue(logSideOffset);
    const double linSideSlope[3] = { 1.1, 1.2, 1.3 };
    log->setLinSideSlopeValue(linSideSlope);
    const double linSideOffset[3] = { 0.051, 0.052, 0.053 };
    log->setLinSideOffsetValue(linSideOffset);
    const double linearSlope[3] = { 1.25, 1.23, 1.22 };
    log->setLinearSlopeValue(linearSlope);

    test.setProcessor(log);

    test.setErrorThreshold(g_epsilon_inverse);

#ifdef __APPLE__
    test.setTestNaN(false);
    test.setTestInfinity(false);
#endif
}
