// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// NOTE:
// The file is a copy and paste from the corresponding GPU unitest i.e. []/tests/gpu/LogOp_test.cpp.
// Keep the two files in sync. to increase the test coverage.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


static constexpr float g_epsilon = 1e-5f;
static constexpr float g_epsilon_inverse = 1e-4f;

static constexpr float base10 = 10.0f;
static const float eulerConstant = expf(1.0f);


OCIO_OSL_TEST(LogTransform, LogBase_10_generic_shader)
{
    OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    log->setBase(base10);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon;
}

OCIO_OSL_TEST(LogTransform, LogBase_10_inverse_generic_shader)
{
    OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    log->setBase(base10);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon_inverse;
}

OCIO_OSL_TEST(LogTransform, LogBase_euler_generic_shader)
{
    OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    log->setBase(eulerConstant);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon;
}

OCIO_OSL_TEST(LogTransform, LogBase_euler_inverse_generic_shader)
{
    OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    log->setBase(eulerConstant);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon_inverse;
}

OCIO_OSL_TEST(LogAffineTransform, base)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    log->setBase(base10);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon;
}

OCIO_OSL_TEST(LogAffineTransform, base_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    log->setBase(base10);

    m_data->m_transform = log;

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 1e-5f
}

OCIO_OSL_TEST(LogAffineTransform, linSideSlope)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    static constexpr double linSideSlope[3] = {2.0, 0.5, 3.0};
    log->setLinSideSlopeValue(linSideSlope);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon;
}

OCIO_OSL_TEST(LogAffineTransform, linSideSlope_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    static constexpr double linSideSlope[3] = { 2.0, 0.5, 3.0 };
    log->setLinSideSlopeValue(linSideSlope);

    m_data->m_transform = log;

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 1e-5f
}

OCIO_OSL_TEST(LogAffineTransform, linSideOffset)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    static constexpr double linSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLinSideOffsetValue(linSideOffset);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon;
}

OCIO_OSL_TEST(LogAffineTransform, linSideOffset_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    static constexpr double linSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLinSideOffsetValue(linSideOffset);

    m_data->m_transform = log;

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 1e-5f
}

OCIO_OSL_TEST(LogAffineTransform, logSideSlope)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    static constexpr double logSideSlope[3] = { 2.0, 0.5, 3.0 };
    log->setLogSideSlopeValue(logSideSlope);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon * 0.5f;
}

OCIO_OSL_TEST(LogAffineTransform, logSideSlope_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    static constexpr double logSideSlope[3] = { 2.0, 0.5, 3.0 };
    log->setLogSideSlopeValue(logSideSlope);

    m_data->m_transform = log;

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 1e-5f
}

OCIO_OSL_TEST(LogAffineTransform, logSideOffset)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    static constexpr double logSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLogSideOffsetValue(logSideOffset);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon;
}

OCIO_OSL_TEST(LogAffineTransform, logSideOffset_inverse)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    static constexpr double logSideOffset[3] = { 0.1, 0.2, 0.3 };
    log->setLogSideOffsetValue(logSideOffset);

    m_data->m_transform = log;

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 1e-5f
}

OCIO_OSL_TEST(LogAffineTransform, lin2log)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    static constexpr double logSideSlope[3] = { 0.2, 0.4, 0.25 };
    log->setLogSideSlopeValue(logSideSlope);
    static constexpr double logSideOffset[3] = { 0.14, 0.13, 0.12 };
    log->setLogSideOffsetValue(logSideOffset);
    static constexpr double linSideSlope[3] = { 1.5, 1.8, 1.2 };
    log->setLinSideSlopeValue(linSideSlope);
    static constexpr double linSideOffset[3] = { 0.05, 0.1, 0.15 };
    log->setLinSideOffsetValue(linSideOffset);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon * 0.5f;
}

OCIO_OSL_TEST(LogAffineTransform, log2lin)
{
    OCIO::LogAffineTransformRcPtr log = OCIO::LogAffineTransform::Create();
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    static constexpr double logSideSlope[3] = { 0.21, 0.2, 0.19 };
    log->setLogSideSlopeValue(logSideSlope);
    static constexpr double logSideOffset[3] = { 0.61, 0.6, 0.59 };
    log->setLogSideOffsetValue(logSideOffset);
    static constexpr double linSideSlope[3] = { 1.11, 1.1, 1.12 };
    log->setLinSideSlopeValue(linSideSlope);
    static constexpr double linSideOffset[3] = { 0.051, 0.05, 0.052 };
    log->setLinSideOffsetValue(linSideOffset);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon_inverse;
}

OCIO_OSL_TEST(LogCameraTransform, camera_lin2log)
{
    static constexpr double linSideBreak[3] = { 0.12, 0.13, 0.15 };
    OCIO::LogCameraTransformRcPtr log = OCIO::LogCameraTransform::Create(linSideBreak);
    log->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    static constexpr double logSideSlope[3] = { 0.2, 0.3, 0.4 };
    log->setLogSideSlopeValue(logSideSlope);
    static constexpr double logSideOffset[3] = { 0.7, 0.6, 0.5 };
    log->setLogSideOffsetValue(logSideOffset);
    static constexpr double linSideSlope[3] = { 1.4, 1.1, 1.2 };
    log->setLinSideSlopeValue(linSideSlope);
    static constexpr double linSideOffset[3] = { 0.15, 0.16, 0.25 };
    log->setLinSideOffsetValue(linSideOffset);
    static constexpr double linearSlope[3] = { 1.22, 1.33, 1.44 };
    log->setLinearSlopeValue(linearSlope);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon;
}

OCIO_OSL_TEST(LogCameraTransform, camera_log2lin)
{
    static constexpr double linSideBreak[3] = { 0.12, 0.13, 0.14 };

    OCIO::LogCameraTransformRcPtr log = OCIO::LogCameraTransform::Create(linSideBreak);
    log->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    static constexpr double logSideSlope[3] = { 0.21, 0.22, 0.23 };
    log->setLogSideSlopeValue(logSideSlope);
    static constexpr double logSideOffset[3] = { 0.6, 0.7, 0.8 };
    log->setLogSideOffsetValue(logSideOffset);
    static constexpr double linSideSlope[3] = { 1.1, 1.2, 1.3 };
    log->setLinSideSlopeValue(linSideSlope);
    static constexpr double linSideOffset[3] = { 0.051, 0.052, 0.053 };
    log->setLinSideOffsetValue(linSideOffset);
    static constexpr double linearSlope[3] = { 1.25, 1.23, 1.22 };
    log->setLinearSlopeValue(linearSlope);

    m_data->m_transform = log;

    m_data->m_threshold = g_epsilon_inverse;
}
