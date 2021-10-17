// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "GpuShaderUtils.cpp"

#include "testutils/UnitTest.h"
#include <OpenColorIO/OpenColorIO.h>

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

OCIO_ADD_TEST(GpuShaderUtils2, float_to_string)
{
    // Use client provided OCIO values, or use default fallback values
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

    const char* display =
                          config->getDefaultDisplay();

    const char* view =
                       config->getDefaultView(display);

    //std::string inputColorSpace = _colorspaceOCIO;
    std::string inputColorSpace = "p3dci8";
    if (inputColorSpace.empty()) {
        OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace("default");
        if (cs) {
            inputColorSpace = cs->getName();
        } else {
            inputColorSpace = OCIO::ROLE_SCENE_LINEAR;
        }
    }

    // Setup the transformation we need to apply
    OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
    transform->setDisplay(display);
    transform->setView(view);
    transform->setSrc(inputColorSpace.c_str());


    OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);
    auto gpuProcessor = processor->getDefaultGPUProcessor();



    // Create a GPU Shader Description
    auto shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    //shaderDesc->
    //TODO read curernt language from USD
    shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_METAL);
    shaderDesc->setFunctionName("OCIODisplay");

    gpuProcessor->extractGpuShaderInfo(shaderDesc);
    const float *lutValues = nullptr;
    shaderDesc->get3DTextureValues(0, lutValues);
}
