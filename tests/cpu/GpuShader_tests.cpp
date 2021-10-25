// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "GpuShader.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


// This is unit-testing internal helper functions not exposed to the API,
// not high-level GPU processing.

OCIO_ADD_TEST(GpuShader, generic_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GenericGpuShaderDesc::Create();

    {
        OCIO_CHECK_NE(shaderDesc->getLanguage(), OCIO::GPU_LANGUAGE_GLSL_1_3);
        OCIO_CHECK_NO_THROW(shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3));
        OCIO_CHECK_EQUAL(shaderDesc->getLanguage(), OCIO::GPU_LANGUAGE_GLSL_1_3);

        OCIO_CHECK_NE(std::string(shaderDesc->getFunctionName()), "1sd234_");
        OCIO_CHECK_NO_THROW(shaderDesc->setFunctionName("1sd234_"));
        OCIO_CHECK_EQUAL(std::string(shaderDesc->getFunctionName()), "1sd234_");

        OCIO_CHECK_NE(std::string(shaderDesc->getPixelName()), "pxl_1sd234_");
        OCIO_CHECK_NO_THROW(shaderDesc->setPixelName("pxl_1sd234_"));
        OCIO_CHECK_EQUAL(std::string(shaderDesc->getPixelName()), "pxl_1sd234_");

        OCIO_CHECK_NE(std::string(shaderDesc->getResourcePrefix()), "res_1sd234_");
        OCIO_CHECK_NO_THROW(shaderDesc->setResourcePrefix("res_1sd234_"));
        OCIO_CHECK_EQUAL(std::string(shaderDesc->getResourcePrefix()), "res_1sd234_");

        OCIO_CHECK_NO_THROW(shaderDesc->finalize());
        const std::string id(shaderDesc->getCacheID());
        OCIO_CHECK_EQUAL(id, std::string("glsl_1.3 1sd234_ res_1sd234_ pxl_1sd234_ 0 "
                                         "$4dd1c89df8002b409e089089ce8f24e7"));
        OCIO_CHECK_NO_THROW(shaderDesc->setResourcePrefix("res_1"));
        OCIO_CHECK_NO_THROW(shaderDesc->finalize());
        OCIO_CHECK_NE(std::string(shaderDesc->getCacheID()), id);
    }

    {
        const unsigned width  = 3;
        const unsigned height = 2;
        const unsigned size = width*height*3;

        float values[size]
            = { 0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f,
                0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f };

        OCIO_CHECK_EQUAL(shaderDesc->getNumTextures(), 0U);
        OCIO_CHECK_NO_THROW(shaderDesc->addTexture("lut1",
                                                   "lut1Sampler",
                                                   width, height,
                                                   OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL,
                                                   OCIO::INTERP_TETRAHEDRAL,
                                                   &values[0]));

        OCIO_CHECK_EQUAL(shaderDesc->getNumTextures(), 1U);

        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        unsigned w = 0;
        unsigned h = 0;
        OCIO::GpuShaderDesc::TextureType t = OCIO::GpuShaderDesc::TEXTURE_RED_CHANNEL;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OCIO_CHECK_NO_THROW(shaderDesc->getTexture(0, textureName, samplerName, w, h, t, i));

        OCIO_CHECK_EQUAL(std::string(textureName), "lut1");
        OCIO_CHECK_EQUAL(std::string(samplerName), "lut1Sampler");
        OCIO_CHECK_EQUAL(width, w);
        OCIO_CHECK_EQUAL(height, h);
        OCIO_CHECK_EQUAL(OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL, t);
        OCIO_CHECK_EQUAL(OCIO::INTERP_TETRAHEDRAL, i);

        OCIO_CHECK_THROW_WHAT(shaderDesc->getTexture(1, textureName, samplerName, w, h, t, i),
                              OCIO::Exception,
                              "1D LUT access error");

        const float * vals = 0x0;
        OCIO_CHECK_NO_THROW(shaderDesc->getTextureValues(0, vals));
        OCIO_CHECK_NE(vals, 0x0);
        for(unsigned idx=0; idx<size; ++idx)
        {
            OCIO_CHECK_EQUAL(values[idx], vals[idx]);
        }

        OCIO_CHECK_THROW_WHAT(shaderDesc->getTextureValues(1, vals),
                              OCIO::Exception,
                              "1D LUT access error");

        // Support several 1D LUTs

        OCIO_CHECK_NO_THROW(shaderDesc->addTexture("lut2", "lut2Sampler", width, height,
                                                   OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL,
                                                   OCIO::INTERP_TETRAHEDRAL,
                                                   &values[0]));

        OCIO_CHECK_EQUAL(shaderDesc->getNumTextures(), 2U);

        OCIO_CHECK_NO_THROW(shaderDesc->getTextureValues(0, vals));
        OCIO_CHECK_NO_THROW(shaderDesc->getTextureValues(1, vals));
        OCIO_CHECK_THROW_WHAT(shaderDesc->getTextureValues(2, vals),
                              OCIO::Exception,
                              "1D LUT access error");
    }

    {
        const unsigned edgelen = 2;
        const unsigned size = edgelen*edgelen*edgelen*3;
        float values[size]
            = { 0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f,  0.7f, 0.8f, 0.9f,
                0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f,  0.7f, 0.8f, 0.9f, };

        OCIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 0U);
        OCIO_CHECK_NO_THROW(shaderDesc->add3DTexture("lut1", "lut1Sampler", edgelen,
                                                     OCIO::INTERP_TETRAHEDRAL,
                                                     &values[0]));

        OCIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 1U);

        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        unsigned e = 0;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OCIO_CHECK_NO_THROW(shaderDesc->get3DTexture(0, textureName, samplerName, e, i));

        OCIO_CHECK_EQUAL(std::string(textureName), "lut1");
        OCIO_CHECK_EQUAL(std::string(samplerName), "lut1Sampler");
        OCIO_CHECK_EQUAL(edgelen, e);
        OCIO_CHECK_EQUAL(OCIO::INTERP_TETRAHEDRAL, i);

        OCIO_CHECK_THROW_WHAT(shaderDesc->get3DTexture(1, textureName, samplerName, e, i),
                              OCIO::Exception,
                              "3D LUT access error");

        const float * vals = nullptr;
        OCIO_CHECK_NO_THROW(shaderDesc->get3DTextureValues(0, vals));
        OCIO_CHECK_ASSERT(vals!=nullptr);
        for(unsigned idx=0; idx<size; ++idx)
        {
            OCIO_CHECK_EQUAL(values[idx], vals[idx]);
        }

        OCIO_CHECK_THROW_WHAT(shaderDesc->get3DTextureValues(1, vals),
                              OCIO::Exception,
                              "3D LUT access error");

        // Supports several 3D LUTs

        OCIO_CHECK_NO_THROW(shaderDesc->add3DTexture("lut1", "lut1Sampler", edgelen,
                                                     OCIO::INTERP_TETRAHEDRAL,
                                                     &values[0]));

        OCIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 2U);

        // Check the 3D LUT limit

        OCIO_CHECK_THROW(shaderDesc->add3DTexture("lut1", "lut1Sampler", 130,
                                                  OCIO::INTERP_TETRAHEDRAL,
                                                  &values[0]),
                         OCIO::Exception);
    }

    {
        OCIO_CHECK_NO_THROW(shaderDesc->addToDeclareShaderCode("vec2 coords;\n"));
        OCIO_CHECK_NO_THROW(shaderDesc->addToHelperShaderCode("vec2 helpers() {}\n\n"));
        OCIO_CHECK_NO_THROW(shaderDesc->addToFunctionHeaderShaderCode("void func() {\n"));
        OCIO_CHECK_NO_THROW(shaderDesc->addToFunctionShaderCode("  int i;\n"));
        OCIO_CHECK_NO_THROW(shaderDesc->addToFunctionFooterShaderCode("}\n"));

        OCIO_CHECK_NO_THROW(shaderDesc->finalize());

        std::string fragText;
        fragText += "\n";
        fragText += "// Declaration of all variables\n";
        fragText += "\n";
        fragText += "vec2 coords;\n";
        fragText += "\n";
        fragText += "// Declaration of all helper methods\n";
        fragText += "\n";
        fragText += "vec2 helpers() {}\n\n";
        fragText += "void func() {\n";
        fragText += "  int i;\n";
        fragText += "}\n";

        OCIO_CHECK_EQUAL(fragText, shaderDesc->getShaderText());
    }
}

OCIO_ADD_TEST(GpuShader, MetalLutTest)
{
    static constexpr char sFromSpace[] = "ACEScg";
    static constexpr char sDiplay[] = "AdobeRGB";
    static constexpr char sView[] = "raw";

    static constexpr char CONFIG[] = { R"(ocio_profile_version: 2
environment: {}
search_path: "./"
roles:
  data: Raw
  default: Raw
  scene_linear: ACEScg

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  AdobeRGB:
    - !<View> {name: Raw, colorspace: Raw}

colorspaces:
  - !<ColorSpace>
    name: ACEScg
    to_reference: !<MatrixTransform> {matrix: [ 0.695452241357, 0.140678696470, 0.163869062172, 0, 0.044794563372, 0.859671118456, 0.095534318172, 0, -0.005525882558, 0.004025210306, 1.001500672252, 0, 0, 0, 0, 1 ]}
  - !<ColorSpace>
    name: Raw
    isdata: true)" };

    {
        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConstConfigRcPtr mOCIOCfg;
        OCIO_CHECK_NO_THROW(mOCIOCfg = OCIO::Config::CreateFromStream(iss));
        OCIO_CHECK_NO_THROW(mOCIOCfg->validate());

        OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
        transform->setSrc(sFromSpace);
        transform->setDisplay(sDiplay);
        transform->setView(sView);
        
        auto processor = mOCIOCfg->getProcessor(transform);

        const unsigned edgelen = 2;
        auto gpuProcessor = processor->getOptimizedLegacyGPUProcessor(OCIO::OPTIMIZATION_ALL, edgelen);
    
        auto shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_METAL);

        gpuProcessor->extractGpuShaderInfo(shaderDesc);
        const std::string text = shaderDesc->getShaderText();;
        const std::string expected =
            "\n"
            "// Declaration of class wrapper\n"
            "\n"
            "struct OCIO\n"
            "{\n"
            "OCIO(\n"
            ")\n"
            "{\n"
            "}\n"
            "\n"
            "\n"
            "\n"
            "// Declaration of the OCIO shader function\n"
            "\n"
            "float4 Display(float4 inPixel)\n"
            "{\n"
            "  float4 outColor = inPixel;\n"
            "\n"
            "  return outColor;\n"
            "}\n"
            "\n"
            "// close class wrapper\n"
            "\n"
            "\n"
            "};\n"
            "float4 Display(\n"
            "  float4 inPixel)\n"
            "{\n"
            "  return OCIO(\n"
            "  ).Display(inPixel);\n"
            "}\n"
            "\n";

        OCIO_CHECK_EQUAL(expected, text);
    }
}

OCIO_ADD_TEST(GpuShader, MetalLutTest2)
{
    static constexpr char sFromSpace[] = "ACEScg";
    static constexpr char sDiplay[] = "AdobeRGB";
    static constexpr char sView[] = "raw";

    static constexpr char CONFIG[] = { R"(ocio_profile_version: 2
environment: {}
search_path: "./"
roles:
  data: Raw
  default: Raw
  scene_linear: ACEScg

file_rules:
  - !<Rule> {name: Default, colorspace: default}

displays:
  AdobeRGB:
    - !<View> {name: Raw, colorspace: Raw}

colorspaces:
  - !<ColorSpace>
    name: ACEScg
    to_reference: !<MatrixTransform> {matrix: [ 0.695452241357, 0.140678696470, 0.163869062172, 0, 0.044794563372, 0.859671118456, 0.095534318172, 0, -0.005525882558, 0.004025210306, 1.001500672252, 0, 0, 0, 0, 1 ]}
  - !<ColorSpace>
    name: Raw
    isdata: true)" };

    {
        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConstConfigRcPtr mOCIOCfg;
        OCIO_CHECK_NO_THROW(mOCIOCfg = OCIO::Config::CreateFromStream(iss));
        OCIO_CHECK_NO_THROW(mOCIOCfg->validate());

        OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
        transform->setSrc(sFromSpace);
        transform->setDisplay(sDiplay);
        transform->setView(sView);
        
        auto processor = mOCIOCfg->getProcessor(transform);

        auto gpuProcessor = processor->getDefaultGPUProcessor();
    
        auto shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_METAL);

        gpuProcessor->extractGpuShaderInfo(shaderDesc);
        const std::string text = shaderDesc->getShaderText();;
        const std::string expected =
            "\n"
            "// Declaration of class wrapper\n"
            "\n"
            "struct OCIO\n"
            "{\n"
            "OCIO(\n"
            ")\n"
            "{\n"
            "}\n"
            "\n"
            "\n"
            "\n"
            "// Declaration of the OCIO shader function\n"
            "\n"
            "float4 Display(float4 inPixel)\n"
            "{\n"
            "  float4 outColor = inPixel;\n"
            "\n"
            "  return outColor;\n"
            "}\n"
            "\n"
            "// close class wrapper\n"
            "\n"
            "\n"
            "};\n"
            "float4 Display(\n"
            "  float4 inPixel)\n"
            "{\n"
            "  return OCIO(\n"
            "  ).Display(inPixel);\n"
            "}\n"
            "\n";
        
        OCIO_CHECK_EQUAL(expected, text);
    }
}

OCIO_ADD_TEST(GpuShader, MetalSupport3)
{
    // The unit test validates a single 1D LUT.

    static const std::string CONFIG =
        "ocio_profile_version: 2\n"
        "\n"
        "environment: {ENV1: " + OCIO::GetTestFilesDir() + "}\n"
        "\n"
        "search_path: $ENV1\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "  reference: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs2}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<FileTransform> {src: lut1d_green.ctf}\n";

    {
        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConstConfigRcPtr mOCIOCfg;
        OCIO_CHECK_NO_THROW(mOCIOCfg = OCIO::Config::CreateFromStream(iss));
        OCIO_CHECK_NO_THROW(mOCIOCfg->validate());

        OCIO::ColorSpaceTransformRcPtr transform = OCIO::ColorSpaceTransform::Create();
        transform->setSrc("cs1");
        transform->setDst("cs2");
        
        auto processor = mOCIOCfg->getProcessor(transform);
        auto gpuProcessor = processor->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_NONE);
    
        auto shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_METAL);
        shaderDesc->setFunctionName("MyMethodName");
        shaderDesc->setPixelName("myPixelName");
        
        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        const std::string text(shaderDesc->getShaderText());
        const std::string expected =
            "\n"
            "// Declaration of class wrapper\n"
            "\n"
            "struct OCIO\n"
            "{\n"
            "OCIO(\n"
            "  texture1d<float> ocio_lut1d_0, sampler ocio_lut1d_0Sampler\n"
            ")\n"
            "{\n"
            "  this->ocio_lut1d_0 = ocio_lut1d_0;\n"
            "  this->ocio_lut1d_0Sampler = ocio_lut1d_0Sampler;\n"
            "}\n"
            "\n"
            "\n"
            "\n"
            "// Declaration of all variables\n"
            "\n"
            "texture1d<float> ocio_lut1d_0;\n"
            "sampler ocio_lut1d_0Sampler;\n"
            "\n"
            "// Declaration of the OCIO shader function\n"
            "\n"
            "float4 MyMethodName(float4 inPixel)\n"
            "{\n"
            "  float4 myPixelName = inPixel;\n"
            "  \n"
            "  // Add LUT 1D processing for ocio_lut1d_0\n"
            "  \n"
            "  {\n"
            "    float3 ocio_lut1d_0_coords = (myPixelName.rgb * float3(31., 31., 31.) + float3(0.5, 0.5, 0.5) ) / float3(32., 32., 32.);\n"
            "    myPixelName.r = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.r).r;\n"
            "    myPixelName.g = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.g).g;\n"
            "    myPixelName.b = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.b).b;\n"
            "  }\n"
            "\n"
            "  return myPixelName;\n"
            "}\n"
            "\n"
            "// close class wrapper\n"
            "\n"
            "\n"
            "};\n"
            "float4 MyMethodName(\n"
            "  texture1d<float> ocio_lut1d_0, sampler ocio_lut1d_0Sampler\n"
            "  ,float4 inPixel)\n"
            "{\n"
            "  return OCIO(\n"
            "    ocio_lut1d_0, ocio_lut1d_0Sampler\n"
            "  ).MyMethodName(inPixel);\n"
            "}\n"
            "\n";

        OCIO_CHECK_EQUAL(expected, text);
    }
}

OCIO_ADD_TEST(GpuShader, MetalSupport4)
{
    // The unit test validates a single 3D LUT.

    static const std::string CONFIG =
        "ocio_profile_version: 2\n"
        "\n"
        "environment: {ENV1: " + OCIO::GetTestFilesDir() + "}\n"
        "\n"
        "search_path: $ENV1\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "  reference: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs2}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<FileTransform> {src: lut3d_example_Inv.ctf}\n";

    {
        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConstConfigRcPtr mOCIOCfg;
        OCIO_CHECK_NO_THROW(mOCIOCfg = OCIO::Config::CreateFromStream(iss));
        OCIO_CHECK_NO_THROW(mOCIOCfg->validate());

        OCIO::ColorSpaceTransformRcPtr transform = OCIO::ColorSpaceTransform::Create();
        transform->setSrc("cs1");
        transform->setDst("cs2");
        
        auto processor = mOCIOCfg->getProcessor(transform);
        auto gpuProcessor = processor->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_NONE);
    
        auto shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_METAL);
        shaderDesc->setFunctionName("MyMethodName");
        shaderDesc->setPixelName("myPixelName");

        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        const std::string text(shaderDesc->getShaderText());
        const std::string expected =
            "\n"
            "// Declaration of class wrapper\n"
            "\n"
            "struct OCIO\n"
            "{\n"
            "OCIO(\n"
            "  texture3d<float> ocio_lut3d_0, sampler ocio_lut3d_0Sampler\n"
            ")\n"
            "{\n"
            "  this->ocio_lut3d_0 = ocio_lut3d_0;\n"
            "  this->ocio_lut3d_0Sampler = ocio_lut3d_0Sampler;\n"
            "}\n"
            "\n"
            "\n"
            "\n"
            "// Declaration of all variables\n"
            "\n"
            "texture3d<float> ocio_lut3d_0;\n"
            "sampler ocio_lut3d_0Sampler;\n"
            "\n"
            "// Declaration of the OCIO shader function\n"
            "\n"
            "float4 MyMethodName(float4 inPixel)\n"
            "{\n"
            "  float4 myPixelName = inPixel;\n"
            "  \n"
            "  // Add LUT 3D processing for ocio_lut3d_0\n"
            "  \n"
            "  float3 ocio_lut3d_0_coords = (myPixelName.zyx * float3(47., 47., 47.) + float3(0.5, 0.5, 0.5)) / float3(48., 48., 48.);\n"
            "  myPixelName.rgb = ocio_lut3d_0.sample(ocio_lut3d_0Sampler, ocio_lut3d_0_coords).rgb;\n"
            "\n"
            "  return myPixelName;\n"
            "}\n"
            "\n"
            "// close class wrapper\n"
            "\n"
            "\n"
            "};\n"
            "float4 MyMethodName(\n"
            "  texture3d<float> ocio_lut3d_0, sampler ocio_lut3d_0Sampler\n"
            "  ,float4 inPixel)\n"
            "{\n"
            "  return OCIO(\n"
            "    ocio_lut3d_0, ocio_lut3d_0Sampler\n"
            "  ).MyMethodName(inPixel);\n"
            "}\n"
            "\n";

        OCIO_CHECK_EQUAL(expected, text);
    }
}

OCIO_ADD_TEST(GpuShader, MetalSupport5)
{
    // The unit test validates a single 1D LUT needing an helper method.

    static const std::string CONFIG =
        "ocio_profile_version: 2\n"
        "\n"
        "environment: {ENV1: " + OCIO::GetTestFilesDir() + "}\n"
        "\n"
        "search_path: $ENV1\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "  reference: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs2}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<FileTransform> {src: clf/lut1d_half_domain_raw_half_set.clf}\n";

    {
        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConstConfigRcPtr mOCIOCfg;
        OCIO_CHECK_NO_THROW(mOCIOCfg = OCIO::Config::CreateFromStream(iss));
        OCIO_CHECK_NO_THROW(mOCIOCfg->validate());

        OCIO::ColorSpaceTransformRcPtr transform = OCIO::ColorSpaceTransform::Create();
        transform->setSrc("cs1");
        transform->setDst("cs2");
        
        auto processor = mOCIOCfg->getProcessor(transform);
        auto gpuProcessor = processor->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_NONE);
    
        auto shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_METAL);

        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        const std::string text(shaderDesc->getShaderText());
        const std::string expected =
            "\n"
            "// Declaration of class wrapper\n"
            "\n"
            "struct OCIO\n"
            "{\n"
            "OCIO(\n"
            "  texture2d<float> ocio_lut1d_0, sampler ocio_lut1d_0Sampler\n"
            ")\n"
            "{\n"
            "  this->ocio_lut1d_0 = ocio_lut1d_0;\n"
            "  this->ocio_lut1d_0Sampler = ocio_lut1d_0Sampler;\n"
            "}\n"
            "\n"
            "\n"
            "\n"
            "// Declaration of all variables\n"
            "\n"
            "texture2d<float> ocio_lut1d_0;\n"
            "sampler ocio_lut1d_0Sampler;\n"
            "\n"
            "// Declaration of all helper methods\n"
            "\n"
            "float2 ocio_lut1d_0_computePos(float f)\n"
            "{\n"
            "  float dep;\n"
            "  float abs_f = abs(f);\n"
            "  if (abs_f > 6.10351562e-05)\n"
            "  {\n"
            "    float3 fComp = float3(15., 15., 15.);\n"
            "    float absarr = min( abs_f, 65504.);\n"
            "    fComp.x = floor( log2( absarr ) );\n"
            "    float lower = pow( 2.0, fComp.x );\n"
            "    fComp.y = ( absarr - lower ) / lower;\n"
            "    float3 scale = float3(1024., 1024., 1024.);\n"
            "    dep = dot( fComp, scale );\n"
            "  }\n"
            "  else\n"
            "  {\n"
            "    dep = abs_f * 1023.0 / 6.09755516e-05;\n"
            "  }\n"
            "  dep += step(f, 0.0) * 32768.0;\n"
            "  float2 retVal;\n"
            "  retVal.y = floor(dep / 4095.);\n"
            "  retVal.x = dep - retVal.y * 4095.;\n"
            "  retVal.x = (retVal.x + 0.5) / 4096.;\n"
            "  retVal.y = (retVal.y + 0.5) / 17.;\n"
            "  return retVal;\n"
            "}\n"
            "\n"
            "// Declaration of the OCIO shader function\n"
            "\n"
            "float4 Display(float4 inPixel)\n"
            "{\n"
            "  float4 outColor = inPixel;\n"
            "  \n"
            "  // Add LUT 1D processing for ocio_lut1d_0\n"
            "  \n"
            "  {\n"
            "    outColor.r = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_computePos(outColor.r)).r;\n"
            "    outColor.g = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_computePos(outColor.g)).r;\n"
            "    outColor.b = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_computePos(outColor.b)).r;\n"
            "  }\n"
            "\n"
            "  return outColor;\n"
            "}\n"
            "\n"
            "// close class wrapper\n"
            "\n"
            "\n"
            "};\n"
            "float4 Display(\n"
            "  texture2d<float> ocio_lut1d_0, sampler ocio_lut1d_0Sampler\n"
            "  ,float4 inPixel)\n"
            "{\n"
            "  return OCIO(\n"
            "    ocio_lut1d_0, ocio_lut1d_0Sampler\n"
            "  ).Display(inPixel);\n"
            "}\n"
            "\n";

        OCIO_CHECK_EQUAL(expected, text);
    }
}

OCIO_ADD_TEST(GpuShader, MetalSupport6)
{
    // The unit test validates several arbitrary luts.

    static const std::string CONFIG =
        "ocio_profile_version: 2\n"
        "\n"
        "environment: {ENV1: " + OCIO::GetTestFilesDir() + "}\n"
        "\n"
        "search_path: $ENV1\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "  reference: cs1\n"
        "\n"
        "displays:\n"
        "  disp1:\n"
        "    - !<View> {name: view1, colorspace: cs2}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    from_scene_reference: !<GroupTransform>\n"
        "      children:\n"
        "        - !<FileTransform> {src: lut1d_green.ctf}\n"
        "        - !<FileTransform> {src: lut3d_example_Inv.ctf}\n"
        "        - !<FileTransform> {src: clf/lut1d_half_domain_raw_half_set.clf}\n";
    {
        std::istringstream iss;
        iss.str(CONFIG);

        OCIO::ConstConfigRcPtr mOCIOCfg;
        OCIO_CHECK_NO_THROW(mOCIOCfg = OCIO::Config::CreateFromStream(iss));
        OCIO_CHECK_NO_THROW(mOCIOCfg->validate());

        OCIO::ColorSpaceTransformRcPtr transform = OCIO::ColorSpaceTransform::Create();
        transform->setSrc("cs1");
        transform->setDst("cs2");
        
        auto processor = mOCIOCfg->getProcessor(transform);
        auto gpuProcessor = processor->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_NONE);
    
        auto shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_METAL);

        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        const std::string text(shaderDesc->getShaderText());
        const std::string expected =
            "\n"
            "// Declaration of class wrapper\n"
            "\n"
            "struct OCIO\n"
            "{\n"
            "OCIO(\n"
            "  texture3d<float> ocio_lut3d_1, sampler ocio_lut3d_1Sampler\n"
            "  , texture1d<float> ocio_lut1d_0, sampler ocio_lut1d_0Sampler\n"
            "  , texture2d<float> ocio_lut1d_2, sampler ocio_lut1d_2Sampler\n"
            ")\n"
            "{\n"
            "  this->ocio_lut3d_1 = ocio_lut3d_1;\n"
            "  this->ocio_lut3d_1Sampler = ocio_lut3d_1Sampler;\n"
            "  this->ocio_lut1d_0 = ocio_lut1d_0;\n"
            "  this->ocio_lut1d_0Sampler = ocio_lut1d_0Sampler;\n"
            "  this->ocio_lut1d_2 = ocio_lut1d_2;\n"
            "  this->ocio_lut1d_2Sampler = ocio_lut1d_2Sampler;\n"
            "}\n"
            "\n"
            "\n"
            "\n"
            "// Declaration of all variables\n"
            "\n"
            "texture1d<float> ocio_lut1d_0;\n"
            "sampler ocio_lut1d_0Sampler;\n"
            "texture3d<float> ocio_lut3d_1;\n"
            "sampler ocio_lut3d_1Sampler;\n"
            "texture2d<float> ocio_lut1d_2;\n"
            "sampler ocio_lut1d_2Sampler;\n"
            "\n"
            "// Declaration of all helper methods\n"
            "\n"
            "float2 ocio_lut1d_2_computePos(float f)\n"
            "{\n"
            "  float dep;\n"
            "  float abs_f = abs(f);\n"
            "  if (abs_f > 6.10351562e-05)\n"
            "  {\n"
            "    float3 fComp = float3(15., 15., 15.);\n"
            "    float absarr = min( abs_f, 65504.);\n"
            "    fComp.x = floor( log2( absarr ) );\n"
            "    float lower = pow( 2.0, fComp.x );\n"
            "    fComp.y = ( absarr - lower ) / lower;\n"
            "    float3 scale = float3(1024., 1024., 1024.);\n"
            "    dep = dot( fComp, scale );\n"
            "  }\n"
            "  else\n"
            "  {\n"
            "    dep = abs_f * 1023.0 / 6.09755516e-05;\n"
            "  }\n"
            "  dep += step(f, 0.0) * 32768.0;\n"
            "  float2 retVal;\n"
            "  retVal.y = floor(dep / 4095.);\n"
            "  retVal.x = dep - retVal.y * 4095.;\n"
            "  retVal.x = (retVal.x + 0.5) / 4096.;\n"
            "  retVal.y = (retVal.y + 0.5) / 17.;\n"
            "  return retVal;\n"
            "}\n"
            "\n"
            "// Declaration of the OCIO shader function\n"
            "\n"
            "float4 Display(float4 inPixel)\n"
            "{\n"
            "  float4 outColor = inPixel;\n"
            "  \n"
            "  // Add LUT 1D processing for ocio_lut1d_0\n"
            "  \n"
            "  {\n"
            "    float3 ocio_lut1d_0_coords = (outColor.rgb * float3(31., 31., 31.) + float3(0.5, 0.5, 0.5) ) / float3(32., 32., 32.);\n"
            "    outColor.r = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.r).r;\n"
            "    outColor.g = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.g).g;\n"
            "    outColor.b = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.b).b;\n"
            "  }\n"
            "  \n"
            "  // Add LUT 3D processing for ocio_lut3d_1\n"
            "  \n"
            "  float3 ocio_lut3d_1_coords = (outColor.zyx * float3(47., 47., 47.) + float3(0.5, 0.5, 0.5)) / float3(48., 48., 48.);\n"
            "  outColor.rgb = ocio_lut3d_1.sample(ocio_lut3d_1Sampler, ocio_lut3d_1_coords).rgb;\n"
            "  \n"
            "  // Add LUT 1D processing for ocio_lut1d_2\n"
            "  \n"
            "  {\n"
            "    outColor.r = ocio_lut1d_2.sample(ocio_lut1d_2Sampler, ocio_lut1d_2_computePos(outColor.r)).r;\n"
            "    outColor.g = ocio_lut1d_2.sample(ocio_lut1d_2Sampler, ocio_lut1d_2_computePos(outColor.g)).r;\n"
            "    outColor.b = ocio_lut1d_2.sample(ocio_lut1d_2Sampler, ocio_lut1d_2_computePos(outColor.b)).r;\n"
            "  }\n"
            "\n"
            "  return outColor;\n"
            "}\n"
            "\n"
            "// close class wrapper\n"
            "\n"
            "\n"
            "};\n"
            "float4 Display(\n"
            "  texture3d<float> ocio_lut3d_1, sampler ocio_lut3d_1Sampler\n"
            "  , texture1d<float> ocio_lut1d_0, sampler ocio_lut1d_0Sampler\n"
            "  , texture2d<float> ocio_lut1d_2, sampler ocio_lut1d_2Sampler\n"
            "  ,float4 inPixel)\n"
            "{\n"
            "  return OCIO(\n"
            "    ocio_lut3d_1, ocio_lut3d_1Sampler\n"
            "    , ocio_lut1d_0, ocio_lut1d_0Sampler\n"
            "    , ocio_lut1d_2, ocio_lut1d_2Sampler\n"
            "  ).Display(inPixel);\n"
            "}\n"
            "\n";

        OCIO_CHECK_EQUAL(expected, text);
    }
}
