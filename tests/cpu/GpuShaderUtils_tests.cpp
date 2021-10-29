// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "GpuShaderUtils.h"
#include "GpuShaderUtils.cpp"

#include "testutils/UnitTest.h"
#include <OpenColorIO/OpenColorIO.h>
#include <OpenColorIO/OpenColorTypes.h>

#include "GpuShader.h"
#include <set>
#include <algorithm>

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

OCIO_ADD_TEST(GpuShaderUtils, getTexType_success)
{
    std::string dim1 = OCIO::GpuShaderText::getTextureKeyword(OCIO::GpuLanguage::GPU_LANGUAGE_MSL_2_0, 1, "float");
    OCIO_CHECK_EQUAL(dim1, "texture1d<float>");
    std::string dim2 = OCIO::GpuShaderText::getTextureKeyword(OCIO::GpuLanguage::GPU_LANGUAGE_MSL_2_0, 2, "float");
    OCIO_CHECK_EQUAL(dim2, "texture2d<float>");
    std::string dim3 = OCIO::GpuShaderText::getTextureKeyword(OCIO::GpuLanguage::GPU_LANGUAGE_MSL_2_0, 3, "float");
    OCIO_CHECK_EQUAL(dim3, "texture3d<float>");
}

OCIO_ADD_TEST(GpuShaderUtils, getTexType_failure)
{
    OCIO_CHECK_THROW_WHAT(OCIO::GpuShaderText::getTextureKeyword(OCIO::GpuLanguage::GPU_LANGUAGE_MSL_2_0, 4, "float"),
                          OCIO::Exception,
                          "Texture dimensions must be 3 or less and more than 0. Passed in was dimensions: 4")
    OCIO_CHECK_THROW_WHAT(OCIO::GpuShaderText::getTextureKeyword(OCIO::GpuLanguage::GPU_LANGUAGE_MSL_2_0, 1, ""),
                          OCIO::Exception,
                          "Texture format must contain at least one character")
}

OCIO_ADD_TEST(GpuShaderUtils, getTexPram_success)
{
    std::string textureParam = OCIO::GpuShaderText::getTextureDeclaration(OCIO::GpuLanguage::GPU_LANGUAGE_MSL_2_0,
                                                                2, "float", "textureName");
    OCIO_CHECK_EQUAL("texture2d<float> textureName", textureParam);
}

OCIO_ADD_TEST(GpuShaderUtils, getTexPram_failure)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_MSL_2_0);
    OCIO_CHECK_THROW_WHAT(OCIO::GpuShaderText::getTextureDeclaration(OCIO::GpuLanguage::GPU_LANGUAGE_MSL_2_0,
                                                                      2, "float", ""),
                            OCIO::Exception,
                            "Name of texture variable must be at least 1 character")
}
