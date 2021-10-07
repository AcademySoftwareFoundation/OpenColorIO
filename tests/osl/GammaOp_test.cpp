// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// NOTE:
// The file is a copy and paste from the corresponding GPU unitest i.e. []/tests/gpu/GammaOp_test.cpp.
// Keep the two files in sync. to increase the test coverage.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


namespace
{

// Helper method to build unit tests
OCIO::ExponentTransformRcPtr AddExponent(OCIO::TransformDirection direction,
                                                   const double(&gamma)[4],
                                                   OCIO::NegativeStyle style)
{
    OCIO::ExponentTransformRcPtr exp = OCIO::ExponentTransform::Create();
    exp->setNegativeStyle(style);
    exp->setDirection(direction);
    exp->setValue(gamma);

    return exp;
}

// Helper method to build unit tests
OCIO::ExponentWithLinearTransformRcPtr AddExponentWithLinear(OCIO::TransformDirection direction,
                                                             const double(&gamma)[4],
                                                             const double(&offset)[4],
                                                             OCIO::NegativeStyle style)
{
    OCIO::ExponentWithLinearTransformRcPtr exp = OCIO::ExponentWithLinearTransform::Create();
    exp->setDirection(direction);
    exp->setGamma(gamma);
    exp->setOffset(offset);
    exp->setNegativeStyle(style);

    return exp;
}

} // anon. namespace

OCIO_OSL_TEST(ExponentOp, forward_v1)
{
    static constexpr double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    m_data->m_transform
        = AddExponent(OCIO::TRANSFORM_DIR_FORWARD, exp, OCIO::NEGATIVE_CLAMP);

    m_data->m_config->setMajorVersion(1);

    m_data->m_threshold = 1e-5f;
}

OCIO_OSL_TEST(ExponentOp, forward)
{
    static constexpr double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    m_data->m_transform
        = AddExponent(OCIO::TRANSFORM_DIR_FORWARD, exp, OCIO::NEGATIVE_CLAMP);

    m_data->m_threshold = 1e-5f;
}

OCIO_OSL_TEST(ExponentOp, forward_mirror)
{
    static constexpr double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    m_data->m_transform
        = AddExponent(OCIO::TRANSFORM_DIR_FORWARD, exp, OCIO::NEGATIVE_MIRROR);

    m_data->m_threshold = 1e-5f;
}

OCIO_OSL_TEST(ExponentOp, forward_pass_thru)
{
    static constexpr double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    m_data->m_transform
        = AddExponent(OCIO::TRANSFORM_DIR_FORWARD, exp, OCIO::NEGATIVE_PASS_THRU);

    m_data->m_threshold = 1e-5f;
}

OCIO_OSL_TEST(ExponentOp, inverse_v1)
{
    static constexpr double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    m_data->m_transform
        = AddExponent(OCIO::TRANSFORM_DIR_INVERSE, exp, OCIO::NEGATIVE_CLAMP);

    m_data->m_config->setMajorVersion(1);

    m_data->m_threshold = 1e-5f; // Slight difference with the GLSL unit test i.e. g_epsilon
}

OCIO_OSL_TEST(ExponentOp, inverse)
{
    static constexpr double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    m_data->m_transform
        = AddExponent(OCIO::TRANSFORM_DIR_INVERSE, exp, OCIO::NEGATIVE_CLAMP);

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 1e-6f
}

OCIO_OSL_TEST(ExponentOp, inverse_mirror)
{
    static constexpr double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    m_data->m_transform
        = AddExponent(OCIO::TRANSFORM_DIR_INVERSE, exp, OCIO::NEGATIVE_MIRROR);

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 1e-6f
}

OCIO_OSL_TEST(ExponentOp, inverse_pass_thru)
{
    static constexpr double exp[4] = { 2.6, 1.0, 1.8, 1.1 };

    m_data->m_transform
        = AddExponent(OCIO::TRANSFORM_DIR_INVERSE, exp, OCIO::NEGATIVE_PASS_THRU);

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 1e-6f
}

static constexpr double gammaVals[4]  = { 2.1,  1.0,  2.3,  1.5  };
static constexpr double offsetVals[4] = {  .01, 0.,    .03,  .05 };

OCIO_OSL_TEST(ExponentWithLinearOp, forward)
{
    m_data->m_transform
        = AddExponentWithLinear(OCIO::TRANSFORM_DIR_FORWARD, gammaVals, offsetVals, OCIO::NEGATIVE_LINEAR);

    m_data->m_threshold = 1e-5f; // Slight difference with the GLSL unit test i.e. 5e-6f
}

OCIO_OSL_TEST(ExponentWithLinearOp, mirror_forward)
{
    m_data->m_transform
        = AddExponentWithLinear(OCIO::TRANSFORM_DIR_FORWARD, gammaVals, offsetVals, OCIO::NEGATIVE_MIRROR);

    m_data->m_threshold = 1e-5f; // Slight difference with the GLSL unit test i.e. 5e-6f
}

OCIO_OSL_TEST(ExponentWithLinearOp, inverse)
{
    m_data->m_transform
        = AddExponentWithLinear(OCIO::TRANSFORM_DIR_INVERSE, gammaVals, offsetVals, OCIO::NEGATIVE_LINEAR);

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 5e-7f
}

OCIO_OSL_TEST(ExponentWithLinearOp, mirror_inverse)
{
    m_data->m_transform
        = AddExponentWithLinear(OCIO::TRANSFORM_DIR_INVERSE, gammaVals, offsetVals, OCIO::NEGATIVE_MIRROR);

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 5e-7f
}

