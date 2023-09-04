// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


const float g_epsilon = 1e-6f;


enum Version : unsigned
{
    OCIO_VERSION_1 = 1,
    OCIO_VERSION_2 = 2  // version 2 or higher
};

// Helper method to build unit tests
void AddExponent(OCIOGPUTest & test,
                 OCIO::TransformDirection direction,
                 const double(&gamma)[4],
                 OCIO::NegativeStyle style,
                 float epsilon,
                 Version ver)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    exp->setNegativeStyle(style);
    exp->setDirection(direction);
    exp->setValue(gamma);

    OCIO_NAMESPACE::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    config->setMajorVersion(ver);

    test.setErrorThreshold(epsilon);
    test.setProcessor(config, exp);
}

// Helper method to build unit tests
void AddExponentWithLinear(OCIOGPUTest & test,
                           OCIO::TransformDirection direction,
                           const double(&gamma)[4],
                           const double(&offset)[4],
                           OCIO::NegativeStyle style,
                           float epsilon)
{
    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();
    exp->setDirection(direction);
    exp->setGamma(gamma);
    exp->setOffset(offset);
    exp->setNegativeStyle(style);

    OCIO_NAMESPACE::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    test.setErrorThreshold(epsilon);
    test.setProcessor(config, exp);
}


OCIO_ADD_GPU_TEST(ExponentOp, legacy_shader_v1)
{
    const double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    AddExponent(test, OCIO::TRANSFORM_DIR_FORWARD, exp, OCIO::NEGATIVE_CLAMP, 1e-5f,
                OCIO_VERSION_1);

    test.setLegacyShader(true);
    // TODO: Would like to be able to remove the setTestNaN(false) and
    // setTestInfinity(false) from all of these tests.
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(ExponentOp, forward_v1)
{
    const double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    AddExponent(test, OCIO::TRANSFORM_DIR_FORWARD, exp, OCIO::NEGATIVE_CLAMP, 1e-5f,
                OCIO_VERSION_1);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(ExponentOp, forward)
{
    const double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    AddExponent(test, OCIO::TRANSFORM_DIR_FORWARD, exp, OCIO::NEGATIVE_CLAMP,
#if OCIO_USE_SSE2
        5e-4f
#else
        1e-5f
#endif
        , OCIO_VERSION_2);
}

OCIO_ADD_GPU_TEST(ExponentOp, forward_mirror)
{
    const double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    AddExponent(test, OCIO::TRANSFORM_DIR_FORWARD, exp, OCIO::NEGATIVE_MIRROR,
#if OCIO_USE_SSE2
        5e-4f // TODO: Only related to the ssePower optimization ?
#else
        1e-5f
#endif
        , OCIO_VERSION_2);
}

OCIO_ADD_GPU_TEST(ExponentOp, forward_pass_thru)
{
    const double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    AddExponent(test, OCIO::TRANSFORM_DIR_FORWARD, exp, OCIO::NEGATIVE_PASS_THRU,
#if OCIO_USE_SSE2
        5e-4f // TODO: Only related to the ssePower optimization ?
#else
        1e-5f
#endif
        , OCIO_VERSION_2);
}

OCIO_ADD_GPU_TEST(ExponentOp, inverse_legacy_shader_v1)
{
    const double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    AddExponent(test, OCIO::TRANSFORM_DIR_INVERSE, exp, OCIO::NEGATIVE_CLAMP,
                g_epsilon, OCIO_VERSION_1);

    test.setLegacyShader(true);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(ExponentOp, inverse_v1)
{
    const double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    AddExponent(test, OCIO::TRANSFORM_DIR_INVERSE, exp, OCIO::NEGATIVE_CLAMP,
                g_epsilon, OCIO_VERSION_1);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(ExponentOp, inverse)
{
    const double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    AddExponent(test, OCIO::TRANSFORM_DIR_INVERSE, exp, OCIO::NEGATIVE_CLAMP,
#if OCIO_USE_SSE2
        5e-4f // TODO: Only related to the ssePower optimization ?
#else
        g_epsilon
#endif
        , OCIO_VERSION_2);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(ExponentOp, inverse_mirror)
{
    const double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    AddExponent(test, OCIO::TRANSFORM_DIR_INVERSE, exp, OCIO::NEGATIVE_MIRROR,
#if OCIO_USE_SSE2
        5e-4f // TODO: Only related to the ssePower optimization ?
#else
        g_epsilon
#endif
        , OCIO_VERSION_2);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(ExponentOp, inverse_pass_thru)
{
    const double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    AddExponent(test, OCIO::TRANSFORM_DIR_INVERSE, exp, OCIO::NEGATIVE_PASS_THRU,
#if OCIO_USE_SSE2
        5e-4f // TODO: Only related to the ssePower optimization ?
#else
        g_epsilon
#endif
        , OCIO_VERSION_2);
    test.setTestInfinity(false);
}

static constexpr double gammaVals[4]  = { 2.1,  1.0,  2.3,  1.5  };
static constexpr double offsetVals[4] = {  .01, 0.,    .03,  .05 };

OCIO_ADD_GPU_TEST(ExponentWithLinearOp, forward)
{
    AddExponentWithLinear(test, OCIO::TRANSFORM_DIR_FORWARD, gammaVals, offsetVals,
        OCIO::NEGATIVE_LINEAR,
#if OCIO_USE_SSE2
        1e-4f // Note: Related to the ssePower optimization !
#else
        5e-6f
#endif
        );
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(ExponentWithLinearOp, mirror_forward)
{
    AddExponentWithLinear(test, OCIO::TRANSFORM_DIR_FORWARD, gammaVals, offsetVals,
        OCIO::NEGATIVE_MIRROR,
#if OCIO_USE_SSE2
        1e-4f // Note: Related to the ssePower optimization !
#else
        5e-6f
#endif
    );
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(ExponentWithLinearOp, inverse)
{
    AddExponentWithLinear(test, OCIO::TRANSFORM_DIR_INVERSE, gammaVals, offsetVals,
        OCIO::NEGATIVE_LINEAR,
#if OCIO_USE_SSE2
        5e-5f // Note: Related to the ssePower optimization !
#else
        5e-7f
#endif
        );
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(ExponentWithLinearOp, mirror_inverse)
{
    AddExponentWithLinear(test, OCIO::TRANSFORM_DIR_INVERSE, gammaVals, offsetVals,
        OCIO::NEGATIVE_MIRROR,
#if OCIO_USE_SSE2
        5e-5f // Note: Related to the ssePower optimization !
#else
        5e-7f
#endif
    );
    test.setTestInfinity(false);
}

