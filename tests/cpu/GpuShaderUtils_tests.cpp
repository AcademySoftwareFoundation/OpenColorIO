// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "GpuShaderUtils.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


// This is unit-testing internal helper functions not exposed to the API,
// not high-level GPU processing.

OCIO_ADD_TEST(GpuShaderUtils, float_to_string)
{
    OCIO_CHECK_EQUAL(OCIO::getFloatString(1.0f, OCIO::GPU_LANGUAGE_GLSL_1_3), "1.");
    OCIO_CHECK_EQUAL(OCIO::getFloatString(-11.0f, OCIO::GPU_LANGUAGE_GLSL_1_3), "-11.");
    OCIO_CHECK_EQUAL(OCIO::getFloatString(-1.0f, OCIO::GPU_LANGUAGE_GLSL_1_3), "-1.");
    OCIO_CHECK_EQUAL(OCIO::getFloatString((float)-1, OCIO::GPU_LANGUAGE_GLSL_1_3), "-1.");
    OCIO_CHECK_EQUAL(OCIO::getFloatString((float)1, OCIO::GPU_LANGUAGE_GLSL_1_3), "1.");
}

