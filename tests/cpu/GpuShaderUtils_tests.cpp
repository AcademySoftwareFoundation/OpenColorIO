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
#include <OpenColorIO/OpenColorIO.h>
#include <OpenColorIO/OpenColorTypes.h>

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

std::vector<OCIO::GpuLanguage> GetAllLanguagesExcept(std::set<OCIO::GpuLanguage> excluded)
{
    std::vector<OCIO::GpuLanguage> included;
    for(int i = OCIO::GpuLanguage::FirstLanguage; i <= OCIO::GpuLanguage::LastLanguage; i++)
    {
        OCIO::GpuLanguage current = (OCIO::GpuLanguage)i;
        if(excluded.find(current) == excluded.end())
        {
            included.push_back(current);
        }
    }
    return included;
}

void assertLanguageHasClassWrapper(OCIO::GpuLanguage lang, bool flag)
{
    OCIO::GpuShaderText ss(lang);
    bool hasWrapper = ss.hasClassWrapper();
    OCIO_CHECK_EQUAL(hasWrapper, flag);
}

OCIO_ADD_TEST(GpuShadersUtils, has_class_wrapper_true)
{
    assertLanguageHasClassWrapper(OCIO::GPU_LANGUAGE_METAL, true);
}

OCIO_ADD_TEST(GpuShadersUtils, has_class_wrapper_false)
{
    auto nonMetalLanguages = GetAllLanguagesExcept({OCIO::GPU_LANGUAGE_METAL});
    std::for_each(nonMetalLanguages.begin(), nonMetalLanguages.end(), [](OCIO::GpuLanguage lang){
        assertLanguageHasClassWrapper(lang, false);
    });
}

OCIO_ADD_TEST(GpuShaderUtils, class_wrapper_header_invalid_languages_empty_string)
{
    auto nonMetalLanguages = GetAllLanguagesExcept({OCIO::GPU_LANGUAGE_METAL});
    std::for_each(nonMetalLanguages.begin(), nonMetalLanguages.end(), [](OCIO::GpuLanguage lang){
        OCIO::GpuShaderText ss(lang);
        std::string classWrapperHeader = ss.classWrapperHeader("anyClassName", {});
        OCIO_CHECK_ASSERT(classWrapperHeader.empty());
    });
}

OCIO_ADD_TEST(GpuShaderUtils, class_wrapper_header_no_textures)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    std::string classWrapper = ss.classWrapperHeader("className", {});
    std::string expected = "class className\n{\n\nclassName()\n{\n}";
    OCIO_CHECK_EQUAL(classWrapper, expected);
}

OCIO_ADD_TEST(GpuShaderUtils, class_wrapper_header_1_texture)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    std::vector<OCIO::TextureInfo> textureInfo {
        OCIO::TextureInfo{"texName", OCIO::TextureDimensions::D2}
    };
    std::string classWrapper = ss.classWrapperHeader("className", textureInfo);
    std::string expected =
    "class className\n"
    "{\n\n"
    "className(texture2d<float> texName)\n"
    "{\n"
        "\tthis->texName = texName;\n"
    "}";
    OCIO_CHECK_EQUAL(classWrapper, expected);
}
    
OCIO_ADD_TEST(GpuShaderUtils, class_wrapper_header_2_texture)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    std::vector<OCIO::TextureInfo> textureInfo {
        OCIO::TextureInfo{"texName1", OCIO::TextureDimensions::D2},
        OCIO::TextureInfo{"texName2", OCIO::TextureDimensions::D2}
    };
    std::string classWrapper = ss.classWrapperHeader("className", textureInfo);
    std::string expected =
        "class className\n"
        "{\n\n"
            "className(texture2d<float> texName1, texture2d<float> texName2)\n"
            "{\n"
                "\tthis->texName1 = texName1;\n"
                "\tthis->texName2 = texName2;\n"
            "}";
    OCIO_CHECK_EQUAL(classWrapper, expected);
}

OCIO_ADD_TEST(GpuShaderUtils, class_wrapper_footer_invalid_languages_empty_string)
{
    auto nonMetalLanguages = GetAllLanguagesExcept({OCIO::GPU_LANGUAGE_METAL});
    std::for_each(nonMetalLanguages.begin(), nonMetalLanguages.end(), [](OCIO::GpuLanguage lang){
        OCIO::GpuShaderText ss(lang);
        std::string classWrapperFooter = ss.classWrapperFooter("anyClassName", {}, "OCIODisplay");
        OCIO_CHECK_ASSERT(classWrapperFooter.empty());
    });
}
OCIO_ADD_TEST(GpuShaderUtils, class_footer_header_no_textures)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    std::string classWrapper = ss.classWrapperFooter("className", {}, "OCIODisplay");
    std::string expected =
        "};\n\n"
        "float4 OCIODisplay(float4 inPixel)\n"
        "{\n"
            "\treturn className().(inPixel);\n"
        "}";
    OCIO_CHECK_EQUAL(classWrapper, expected);
}

OCIO_ADD_TEST(GpuShaderUtils, class_wrapper_footer_1_texture)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    std::vector<OCIO::TextureInfo> textureInfo {
        OCIO::TextureInfo{"texName", OCIO::TextureDimensions::D2}
    };
    std::string classWrapper = ss.classWrapperFooter("className", textureInfo,"OCIODisplay");
    std::string expected =
        "};\n\n"
        "float4 OCIODisplay(texture2d<float> texName, float4 inPixel)\n"
        "{\n"
            "\treturn className(texName).(inPixel);\n"
        "}";
    OCIO_CHECK_EQUAL(classWrapper, expected);
}

OCIO_ADD_TEST(GpuShaderUtils, class_wrapper_footer_2_texture)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    std::vector<OCIO::TextureInfo> textureInfo {
        OCIO::TextureInfo{"texName1", OCIO::TextureDimensions::D2},
        OCIO::TextureInfo{"texName2", OCIO::TextureDimensions::D2}
    };
    std::string classWrapper = ss.classWrapperFooter("className", textureInfo, "OCIODisplay");
    std::string expected =
        "};\n\n"
        "float4 OCIODisplay(texture2d<float> texName1, texture2d<float> texName2, float4 inPixel)\n"
        "{\n"
            "\treturn className(texName1texName2).(inPixel);\n"
        "}";
    OCIO_CHECK_EQUAL(classWrapper, expected);
}

OCIO_ADD_TEST(GpuShaderUtils, get_texture_dimensions_success)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    OCIO::TextureDimensions dim1 = ss.getDimensions("texture1d");
    OCIO_CHECK_EQUAL((int)dim1, (int)OCIO::TextureDimensions::D1);
    OCIO::TextureDimensions  dim2 = ss.getDimensions("texture2d");
    OCIO_CHECK_EQUAL((int)dim2, (int)OCIO::TextureDimensions::D2);
    OCIO::TextureDimensions dim3 = ss.getDimensions("texture3d");
    OCIO_CHECK_EQUAL((int)dim3, (int)OCIO::TextureDimensions::D3);
}

OCIO_ADD_TEST(GpuShaderUtils, get_texture_dimensions_failure)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    OCIO_CHECK_THROW_WHAT(ss.getDimensions("texture4d<float>"),
                          OCIO::Exception,
                          "Unable to parse dimensions from textureType. Invalid string passed in: texture4d<float>");
    ;
}

OCIO_ADD_TEST(GpuShaderUtils, getTexType_success)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    std::string dim1 = ss.getTexType(1, "float");
    OCIO_CHECK_EQUAL(dim1, "texture1d<float>");
    std::string dim2 = ss.getTexType(2, "float");
    OCIO_CHECK_EQUAL(dim2, "texture2d<float>");
    std::string dim3 = ss.getTexType(3, "float");
    OCIO_CHECK_EQUAL(dim3, "texture3d<float>");
}

OCIO_ADD_TEST(GpuShaderUtils, getTexType_failure)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    OCIO_CHECK_THROW_WHAT(ss.getTexType(4, "float"),
                          OCIO::Exception,
                          "Texture dimensions must be 3 or less and more than 0. Passed in was dimensions: 4")
    OCIO_CHECK_THROW_WHAT(ss.getTexType(1, ""),
                          OCIO::Exception,
                          "Texture format must contain at least one character")
}

OCIO_ADD_TEST(GpuShaderUtils, getTexPram_success)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    std::string textureParam;
    ss.getTexParam("textureName", "float", textureParam, 2);
    OCIO_CHECK_EQUAL("texture2d<float> textureName", textureParam);
}

OCIO_ADD_TEST(GpuShaderUtils, getTexPram_failure)
{
    OCIO::GpuShaderText ss(OCIO::GpuLanguage::GPU_LANGUAGE_METAL);
    std::string textureParam;
    OCIO_CHECK_THROW_WHAT(ss.getTexParam("", "float", textureParam, 2),
                          OCIO::Exception,
                          "Name of texture variable must be at least 1 character")
}
