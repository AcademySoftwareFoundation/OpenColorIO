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
                                         "6001c324468d497f99aa06d3014798d8"));
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
                                                   OCIO::GpuShaderDesc::TEXTURE_2D,
                                                   OCIO::INTERP_TETRAHEDRAL,
                                                   &values[0]));

        OCIO_CHECK_EQUAL(shaderDesc->getNumTextures(), 1U);

        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        unsigned w = 0;
        unsigned h = 0;
        OCIO::GpuShaderDesc::TextureType t = OCIO::GpuShaderDesc::TEXTURE_RED_CHANNEL;
        OCIO::GpuShaderDesc::TextureDimensions d = OCIO::GpuShaderDesc::TEXTURE_1D;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OCIO_CHECK_NO_THROW(shaderDesc->getTexture(0, textureName, samplerName, w, h, t, d, i));

        OCIO_CHECK_EQUAL(std::string(textureName), "lut1");
        OCIO_CHECK_EQUAL(std::string(samplerName), "lut1Sampler");
        OCIO_CHECK_EQUAL(width, w);
        OCIO_CHECK_EQUAL(height, h);
        OCIO_CHECK_EQUAL(OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL, t);
        OCIO_CHECK_EQUAL(OCIO::INTERP_TETRAHEDRAL, i);

        OCIO_CHECK_THROW_WHAT(shaderDesc->getTexture(1, textureName, samplerName, w, h, t, d, i),
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
                                                   d,
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

        shaderDesc->setFunctionName("Display");
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);

        gpuProcessor->extractGpuShaderInfo(shaderDesc);
        const std::string text = shaderDesc->getShaderText();;
        static constexpr char expected[] = { R"(
// Declaration of class wrapper

struct ocioDisplay
{
ocioDisplay(
)
{
}


// Declaration of the OCIO shader function

float4 Display(float4 inPixel)
{
  float4 outColor = inPixel;

  return outColor;
}

// Close class wrapper


};
float4 Display(
  float4 inPixel)
{
  return ocioDisplay(
  ).Display(inPixel);
}
)" };

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

        shaderDesc->setFunctionName("Display");
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);

        gpuProcessor->extractGpuShaderInfo(shaderDesc);
        const std::string text = shaderDesc->getShaderText();
        static constexpr char expected[] = { R"(
// Declaration of class wrapper

struct ocioDisplay
{
ocioDisplay(
)
{
}


// Declaration of the OCIO shader function

float4 Display(float4 inPixel)
{
  float4 outColor = inPixel;

  return outColor;
}

// Close class wrapper


};
float4 Display(
  float4 inPixel)
{
  return ocioDisplay(
  ).Display(inPixel);
}
)" };
        
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
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);
        shaderDesc->setFunctionName("MyMethodName");
        shaderDesc->setPixelName("myPixelName");
        
        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        const std::string text(shaderDesc->getShaderText());
        static constexpr char expected[] = { R"(
// Declaration of class wrapper

struct ocioMyMethodName
{
ocioMyMethodName(
  texture1d<float> ocio_lut1d_0
  , sampler ocio_lut1d_0Sampler
)
{
  this->ocio_lut1d_0 = ocio_lut1d_0;
  this->ocio_lut1d_0Sampler = ocio_lut1d_0Sampler;
}


// Declaration of all variables

texture1d<float> ocio_lut1d_0;
sampler ocio_lut1d_0Sampler;

// Declaration of the OCIO shader function

float4 MyMethodName(float4 inPixel)
{
  float4 myPixelName = inPixel;
  
  // Add LUT 1D processing for ocio_lut1d_0
  
  {
    float3 ocio_lut1d_0_coords = (myPixelName.rgb * float3(31., 31., 31.) + float3(0.5, 0.5, 0.5) ) / float3(32., 32., 32.);
    myPixelName.r = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.r).r;
    myPixelName.g = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.g).g;
    myPixelName.b = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.b).b;
  }

  return myPixelName;
}

// Close class wrapper


};
float4 MyMethodName(
  texture1d<float> ocio_lut1d_0
  , sampler ocio_lut1d_0Sampler
  , float4 inPixel)
{
  return ocioMyMethodName(
    ocio_lut1d_0
    , ocio_lut1d_0Sampler
  ).MyMethodName(inPixel);
}
)" };

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
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);
        shaderDesc->setFunctionName("MyMethodName");
        shaderDesc->setPixelName("myPixelName");

        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        const std::string text(shaderDesc->getShaderText());
        static constexpr char expected[] = { R"(
// Declaration of class wrapper

struct ocioMyMethodName
{
ocioMyMethodName(
  texture3d<float> ocio_lut3d_0
  , sampler ocio_lut3d_0Sampler
)
{
  this->ocio_lut3d_0 = ocio_lut3d_0;
  this->ocio_lut3d_0Sampler = ocio_lut3d_0Sampler;
}


// Declaration of all variables

texture3d<float> ocio_lut3d_0;
sampler ocio_lut3d_0Sampler;

// Declaration of the OCIO shader function

float4 MyMethodName(float4 inPixel)
{
  float4 myPixelName = inPixel;
  
  // Add LUT 3D processing for ocio_lut3d_0
  
  float3 ocio_lut3d_0_coords = (myPixelName.zyx * float3(47., 47., 47.) + float3(0.5, 0.5, 0.5)) / float3(48., 48., 48.);
  myPixelName.rgb = ocio_lut3d_0.sample(ocio_lut3d_0Sampler, ocio_lut3d_0_coords).rgb;

  return myPixelName;
}

// Close class wrapper


};
float4 MyMethodName(
  texture3d<float> ocio_lut3d_0
  , sampler ocio_lut3d_0Sampler
  , float4 inPixel)
{
  return ocioMyMethodName(
    ocio_lut3d_0
    , ocio_lut3d_0Sampler
  ).MyMethodName(inPixel);
}
)" };
        
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
        "    from_scene_reference: !<FileTransform> {src: clf/lut1d_long.clf}\n";

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
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);

        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        const std::string text(shaderDesc->getShaderText());
        static constexpr char expected[] = { R"(
// Declaration of class wrapper

struct ocioOCIOMain
{
ocioOCIOMain(
  texture2d<float> ocio_lut1d_0
  , sampler ocio_lut1d_0Sampler
)
{
  this->ocio_lut1d_0 = ocio_lut1d_0;
  this->ocio_lut1d_0Sampler = ocio_lut1d_0Sampler;
}


// Declaration of all variables

texture2d<float> ocio_lut1d_0;
sampler ocio_lut1d_0Sampler;

// Declaration of all helper methods

float2 ocio_lut1d_0_computePos(float f)
{
  float dep = clamp(f, 0.0, 1.0) * 131071.;
  float2 retVal;
  retVal.y = floor(dep / 4095.);
  retVal.x = dep - retVal.y * 4095.;
  retVal.x = (retVal.x + 0.5) / 4096.;
  retVal.y = (retVal.y + 0.5) / 33.;
  return retVal;
}

// Declaration of the OCIO shader function

float4 OCIOMain(float4 inPixel)
{
  float4 outColor = inPixel;
  
  // Add LUT 1D processing for ocio_lut1d_0
  
  {
    outColor.r = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_computePos(outColor.r)).r;
    outColor.g = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_computePos(outColor.g)).r;
    outColor.b = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_computePos(outColor.b)).r;
  }

  return outColor;
}

// Close class wrapper


};
float4 OCIOMain(
  texture2d<float> ocio_lut1d_0
  , sampler ocio_lut1d_0Sampler
  , float4 inPixel)
{
  return ocioOCIOMain(
    ocio_lut1d_0
    , ocio_lut1d_0Sampler
  ).OCIOMain(inPixel);
}
)" };

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
        "        - !<FileTransform> {src: clf/lut1d_long.clf}\n";
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
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);

        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        const std::string text(shaderDesc->getShaderText());
        static constexpr char expected[] = { R"(
// Declaration of class wrapper

struct ocioOCIOMain
{
ocioOCIOMain(
  texture3d<float> ocio_lut3d_1
  , sampler ocio_lut3d_1Sampler
  , texture1d<float> ocio_lut1d_0
  , sampler ocio_lut1d_0Sampler
  , texture2d<float> ocio_lut1d_2
  , sampler ocio_lut1d_2Sampler
)
{
  this->ocio_lut3d_1 = ocio_lut3d_1;
  this->ocio_lut3d_1Sampler = ocio_lut3d_1Sampler;
  this->ocio_lut1d_0 = ocio_lut1d_0;
  this->ocio_lut1d_0Sampler = ocio_lut1d_0Sampler;
  this->ocio_lut1d_2 = ocio_lut1d_2;
  this->ocio_lut1d_2Sampler = ocio_lut1d_2Sampler;
}


// Declaration of all variables

texture1d<float> ocio_lut1d_0;
sampler ocio_lut1d_0Sampler;
texture3d<float> ocio_lut3d_1;
sampler ocio_lut3d_1Sampler;
texture2d<float> ocio_lut1d_2;
sampler ocio_lut1d_2Sampler;

// Declaration of all helper methods

float2 ocio_lut1d_2_computePos(float f)
{
  float dep = clamp(f, 0.0, 1.0) * 131071.;
  float2 retVal;
  retVal.y = floor(dep / 4095.);
  retVal.x = dep - retVal.y * 4095.;
  retVal.x = (retVal.x + 0.5) / 4096.;
  retVal.y = (retVal.y + 0.5) / 33.;
  return retVal;
}

// Declaration of the OCIO shader function

float4 OCIOMain(float4 inPixel)
{
  float4 outColor = inPixel;
  
  // Add LUT 1D processing for ocio_lut1d_0
  
  {
    float3 ocio_lut1d_0_coords = (outColor.rgb * float3(31., 31., 31.) + float3(0.5, 0.5, 0.5) ) / float3(32., 32., 32.);
    outColor.r = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.r).r;
    outColor.g = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.g).g;
    outColor.b = ocio_lut1d_0.sample(ocio_lut1d_0Sampler, ocio_lut1d_0_coords.b).b;
  }
  
  // Add LUT 3D processing for ocio_lut3d_1
  
  float3 ocio_lut3d_1_coords = (outColor.zyx * float3(47., 47., 47.) + float3(0.5, 0.5, 0.5)) / float3(48., 48., 48.);
  outColor.rgb = ocio_lut3d_1.sample(ocio_lut3d_1Sampler, ocio_lut3d_1_coords).rgb;
  
  // Add LUT 1D processing for ocio_lut1d_2
  
  {
    outColor.r = ocio_lut1d_2.sample(ocio_lut1d_2Sampler, ocio_lut1d_2_computePos(outColor.r)).r;
    outColor.g = ocio_lut1d_2.sample(ocio_lut1d_2Sampler, ocio_lut1d_2_computePos(outColor.g)).r;
    outColor.b = ocio_lut1d_2.sample(ocio_lut1d_2Sampler, ocio_lut1d_2_computePos(outColor.b)).r;
  }

  return outColor;
}

// Close class wrapper


};
float4 OCIOMain(
  texture3d<float> ocio_lut3d_1
  , sampler ocio_lut3d_1Sampler
  , texture1d<float> ocio_lut1d_0
  , sampler ocio_lut1d_0Sampler
  , texture2d<float> ocio_lut1d_2
  , sampler ocio_lut1d_2Sampler
  , float4 inPixel)
{
  return ocioOCIOMain(
    ocio_lut3d_1
    , ocio_lut3d_1Sampler
    , ocio_lut1d_0
    , ocio_lut1d_0Sampler
    , ocio_lut1d_2
    , ocio_lut1d_2Sampler
  ).OCIOMain(inPixel);
}
)" };
        
        OCIO_CHECK_EQUAL(expected, text);
    }
}

OCIO_ADD_TEST(GpuShader, MetalSupport7)
{
    // The unit test validates a single E/C which needs a uniform.

    static const std::string CONFIG =
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: " + OCIO::GetTestFilesDir() + "\n"
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
        "    from_scene_reference:\n"
        "       !<ExposureContrastTransform> {style: video, contrast: 0.5}\n";

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
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);

        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        const std::string text(shaderDesc->getShaderText());
        static constexpr char expected[] = { R"(
// Declaration of class wrapper

struct ocioOCIOMain
{
ocioOCIOMain(
  float ocio_exposure_contrast_exposureVal
  , float ocio_exposure_contrast_gammaVal
)
{
  this->ocio_exposure_contrast_exposureVal = ocio_exposure_contrast_exposureVal;
  this->ocio_exposure_contrast_gammaVal = ocio_exposure_contrast_gammaVal;
}


// Declaration of all variables

float ocio_exposure_contrast_exposureVal;
float ocio_exposure_contrast_gammaVal;

// Declaration of the OCIO shader function

float4 OCIOMain(float4 inPixel)
{
  float4 outColor = inPixel;
  
  // Add ExposureContrast 'video' processing
  
  {
    float contrastVal = 0.5;
    float exposure = pow( pow( 2., ocio_exposure_contrast_exposureVal ), 0.54644808743169393);
    float contrast = max( 0.001, ( contrastVal * ocio_exposure_contrast_gammaVal ) );
    outColor.rgb = outColor.rgb * exposure;
    if (contrast != 1.0)
    {
      outColor.rgb = pow( max( float3(0., 0., 0.), outColor.rgb / float3(0.39178254652545397, 0.39178254652545397, 0.39178254652545397) ), float3(contrast, contrast, contrast) ) * float3(0.39178254652545397, 0.39178254652545397, 0.39178254652545397);
    }
  }

  return outColor;
}

// Close class wrapper


};
float4 OCIOMain(
  float ocio_exposure_contrast_exposureVal
  , float ocio_exposure_contrast_gammaVal
  , float4 inPixel)
{
  return ocioOCIOMain(
    ocio_exposure_contrast_exposureVal
    , ocio_exposure_contrast_gammaVal
  ).OCIOMain(inPixel);
}
)"};
        OCIO_CHECK_EQUAL(expected, text);
    }
}

OCIO_ADD_TEST(GpuShader, MetalSupport8)
{
    // The unit test validates a single Grading transform.

    static const std::string CONFIG =
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: " + OCIO::GetTestFilesDir() + "\n"
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
        "    from_scene_reference: \n"
        "       !<GradingRGBCurveTransform>\n"
        "          style: log\n"
        "          red: {control_points: [0, 0, 0.5, 0.5, 1, 1.123456]}\n";

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
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);

        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        const std::string text(shaderDesc->getShaderText());
        static constexpr char expected[] = { R"(
// Declaration of class wrapper

struct ocioOCIOMain
{
ocioOCIOMain(
)
{
}


// Declaration of all helper methods


const int ocio_grading_rgbcurve_knotsOffsets_0[8] = {0, 5, -1, 0, -1, 0, -1, 0};
const float ocio_grading_rgbcurve_knots_0[5] = {0., 0.333333343, 0.5, 0.666666508, 1.};
const int ocio_grading_rgbcurve_coefsOffsets_0[8] = {0, 12, -1, 0, -1, 0, -1, 0};
const float ocio_grading_rgbcurve_coefs_0[12] = {0.0982520878, 0.393008381, 0.347727984, 0.08693178, 0.934498608, 1., 1.13100278, 1.246912, 0., 0.322416425, 0.5, 0.698159397};

float ocio_grading_rgbcurve_evalBSplineCurve_0(int curveIdx, float x)
{
  int knotsOffs = ocio_grading_rgbcurve_knotsOffsets_0[curveIdx * 2];
  int knotsCnt = ocio_grading_rgbcurve_knotsOffsets_0[curveIdx * 2 + 1];
  int coefsOffs = ocio_grading_rgbcurve_coefsOffsets_0[curveIdx * 2];
  int coefsCnt = ocio_grading_rgbcurve_coefsOffsets_0[curveIdx * 2 + 1];
  int coefsSets = coefsCnt / 3;
  if (coefsSets == 0)
  {
    return x;
  }
  float knStart = ocio_grading_rgbcurve_knots_0[knotsOffs];
  float knEnd = ocio_grading_rgbcurve_knots_0[knotsOffs + knotsCnt - 1];
  if (x <= knStart)
  {
    float B = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets];
    float C = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets * 2];
    return (x - knStart) * B + C;
  }
  else if (x >= knEnd)
  {
    float A = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets - 1];
    float B = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets * 2 - 1];
    float C = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets * 3 - 1];
    float kn = ocio_grading_rgbcurve_knots_0[knotsOffs + knotsCnt - 2];
    float t = knEnd - kn;
    float slope = 2. * A * t + B;
    float offs = ( A * t + B ) * t + C;
    return (x - knEnd) * slope + offs;
  }
  int i = 0;
  for (i = 0; i < knotsCnt - 2; ++i)
  {
    if (x < ocio_grading_rgbcurve_knots_0[knotsOffs + i + 1])
    {
      break;
    }
  }
  float A = ocio_grading_rgbcurve_coefs_0[coefsOffs + i];
  float B = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets + i];
  float C = ocio_grading_rgbcurve_coefs_0[coefsOffs + coefsSets * 2 + i];
  float kn = ocio_grading_rgbcurve_knots_0[knotsOffs + i];
  float t = x - kn;
  return ( A * t + B ) * t + C;
}

// Declaration of the OCIO shader function

float4 OCIOMain(float4 inPixel)
{
  float4 outColor = inPixel;
  
  // Add GradingRGBCurve 'log' forward processing
  
  {
    outColor.rgb.r = ocio_grading_rgbcurve_evalBSplineCurve_0(0, outColor.rgb.r);
    outColor.rgb.g = ocio_grading_rgbcurve_evalBSplineCurve_0(1, outColor.rgb.g);
    outColor.rgb.b = ocio_grading_rgbcurve_evalBSplineCurve_0(2, outColor.rgb.b);
    outColor.rgb.r = ocio_grading_rgbcurve_evalBSplineCurve_0(3, outColor.rgb.r);
    outColor.rgb.g = ocio_grading_rgbcurve_evalBSplineCurve_0(3, outColor.rgb.g);
    outColor.rgb.b = ocio_grading_rgbcurve_evalBSplineCurve_0(3, outColor.rgb.b);
  }

  return outColor;
}

// Close class wrapper


};
float4 OCIOMain(
  float4 inPixel)
{
  return ocioOCIOMain(
  ).OCIOMain(inPixel);
}
)" };
        OCIO_CHECK_EQUAL(expected, text);
    }
}

OCIO_ADD_TEST(GpuShader, MetalSupport9)
{
    // The unit test validates a single Grading transform.

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    auto gcta = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
    gcta->makeDynamic();

    auto processor = config->getProcessor(gcta);
    auto gpuProcessor = processor->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_NONE);

    auto shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_MSL_2_0);

    gpuProcessor->extractGpuShaderInfo(shaderDesc);

    const std::string text(shaderDesc->getShaderText());
    static constexpr char expected[] = { R"(
// Declaration of class wrapper

struct ocioOCIOMain
{
ocioOCIOMain(
  constant int ocio_grading_rgbcurve_knotsOffsets[8]
  , int ocio_grading_rgbcurve_knotsOffsets_count
  , constant float ocio_grading_rgbcurve_knots[60]
  , int ocio_grading_rgbcurve_knots_count
  , constant int ocio_grading_rgbcurve_coefsOffsets[8]
  , int ocio_grading_rgbcurve_coefsOffsets_count
  , constant float ocio_grading_rgbcurve_coefs[180]
  , int ocio_grading_rgbcurve_coefs_count
  , bool ocio_grading_rgbcurve_localBypass
)
{
  for(int i = 0; i < ocio_grading_rgbcurve_knotsOffsets_count; ++i)
  {
    this->ocio_grading_rgbcurve_knotsOffsets[i] = ocio_grading_rgbcurve_knotsOffsets[i];
  }
  for(int i = ocio_grading_rgbcurve_knotsOffsets_count; i < 8; ++i)
  {
    this->ocio_grading_rgbcurve_knotsOffsets[i] = 0;
  }
  for(int i = 0; i < ocio_grading_rgbcurve_knots_count; ++i)
  {
    this->ocio_grading_rgbcurve_knots[i] = ocio_grading_rgbcurve_knots[i];
  }
  for(int i = ocio_grading_rgbcurve_knots_count; i < 60; ++i)
  {
    this->ocio_grading_rgbcurve_knots[i] = 0;
  }
  for(int i = 0; i < ocio_grading_rgbcurve_coefsOffsets_count; ++i)
  {
    this->ocio_grading_rgbcurve_coefsOffsets[i] = ocio_grading_rgbcurve_coefsOffsets[i];
  }
  for(int i = ocio_grading_rgbcurve_coefsOffsets_count; i < 8; ++i)
  {
    this->ocio_grading_rgbcurve_coefsOffsets[i] = 0;
  }
  for(int i = 0; i < ocio_grading_rgbcurve_coefs_count; ++i)
  {
    this->ocio_grading_rgbcurve_coefs[i] = ocio_grading_rgbcurve_coefs[i];
  }
  for(int i = ocio_grading_rgbcurve_coefs_count; i < 180; ++i)
  {
    this->ocio_grading_rgbcurve_coefs[i] = 0;
  }
  this->ocio_grading_rgbcurve_localBypass = ocio_grading_rgbcurve_localBypass;
}


// Declaration of all variables

int ocio_grading_rgbcurve_knotsOffsets[8];
float ocio_grading_rgbcurve_knots[60];
int ocio_grading_rgbcurve_coefsOffsets[8];
float ocio_grading_rgbcurve_coefs[180];
bool ocio_grading_rgbcurve_localBypass;

// Declaration of all helper methods


float ocio_grading_rgbcurve_evalBSplineCurve(int curveIdx, float x)
{
  int knotsOffs = ocio_grading_rgbcurve_knotsOffsets[curveIdx * 2];
  int knotsCnt = ocio_grading_rgbcurve_knotsOffsets[curveIdx * 2 + 1];
  int coefsOffs = ocio_grading_rgbcurve_coefsOffsets[curveIdx * 2];
  int coefsCnt = ocio_grading_rgbcurve_coefsOffsets[curveIdx * 2 + 1];
  int coefsSets = coefsCnt / 3;
  if (coefsSets == 0)
  {
    return x;
  }
  float knStart = ocio_grading_rgbcurve_knots[knotsOffs];
  float knEnd = ocio_grading_rgbcurve_knots[knotsOffs + knotsCnt - 1];
  if (x <= knStart)
  {
    float B = ocio_grading_rgbcurve_coefs[coefsOffs + coefsSets];
    float C = ocio_grading_rgbcurve_coefs[coefsOffs + coefsSets * 2];
    return (x - knStart) * B + C;
  }
  else if (x >= knEnd)
  {
    float A = ocio_grading_rgbcurve_coefs[coefsOffs + coefsSets - 1];
    float B = ocio_grading_rgbcurve_coefs[coefsOffs + coefsSets * 2 - 1];
    float C = ocio_grading_rgbcurve_coefs[coefsOffs + coefsSets * 3 - 1];
    float kn = ocio_grading_rgbcurve_knots[knotsOffs + knotsCnt - 2];
    float t = knEnd - kn;
    float slope = 2. * A * t + B;
    float offs = ( A * t + B ) * t + C;
    return (x - knEnd) * slope + offs;
  }
  int i = 0;
  for (i = 0; i < knotsCnt - 2; ++i)
  {
    if (x < ocio_grading_rgbcurve_knots[knotsOffs + i + 1])
    {
      break;
    }
  }
  float A = ocio_grading_rgbcurve_coefs[coefsOffs + i];
  float B = ocio_grading_rgbcurve_coefs[coefsOffs + coefsSets + i];
  float C = ocio_grading_rgbcurve_coefs[coefsOffs + coefsSets * 2 + i];
  float kn = ocio_grading_rgbcurve_knots[knotsOffs + i];
  float t = x - kn;
  return ( A * t + B ) * t + C;
}

// Declaration of the OCIO shader function

float4 OCIOMain(float4 inPixel)
{
  float4 outColor = inPixel;
  
  // Add GradingRGBCurve 'log' forward processing
  
  {
    if (!ocio_grading_rgbcurve_localBypass)
    {
      outColor.rgb.r = ocio_grading_rgbcurve_evalBSplineCurve(0, outColor.rgb.r);
      outColor.rgb.g = ocio_grading_rgbcurve_evalBSplineCurve(1, outColor.rgb.g);
      outColor.rgb.b = ocio_grading_rgbcurve_evalBSplineCurve(2, outColor.rgb.b);
      outColor.rgb.r = ocio_grading_rgbcurve_evalBSplineCurve(3, outColor.rgb.r);
      outColor.rgb.g = ocio_grading_rgbcurve_evalBSplineCurve(3, outColor.rgb.g);
      outColor.rgb.b = ocio_grading_rgbcurve_evalBSplineCurve(3, outColor.rgb.b);
    }
  }

  return outColor;
}

// Close class wrapper


};
float4 OCIOMain(
  constant int ocio_grading_rgbcurve_knotsOffsets[8]
  , int ocio_grading_rgbcurve_knotsOffsets_count
  , constant float ocio_grading_rgbcurve_knots[60]
  , int ocio_grading_rgbcurve_knots_count
  , constant int ocio_grading_rgbcurve_coefsOffsets[8]
  , int ocio_grading_rgbcurve_coefsOffsets_count
  , constant float ocio_grading_rgbcurve_coefs[180]
  , int ocio_grading_rgbcurve_coefs_count
  , bool ocio_grading_rgbcurve_localBypass
  , float4 inPixel)
{
  return ocioOCIOMain(
    ocio_grading_rgbcurve_knotsOffsets
    , ocio_grading_rgbcurve_knotsOffsets_count
    , ocio_grading_rgbcurve_knots
    , ocio_grading_rgbcurve_knots_count
    , ocio_grading_rgbcurve_coefsOffsets
    , ocio_grading_rgbcurve_coefsOffsets_count
    , ocio_grading_rgbcurve_coefs
    , ocio_grading_rgbcurve_coefs_count
    , ocio_grading_rgbcurve_localBypass
  ).OCIOMain(inPixel);
}
)" };
    
    OCIO_CHECK_EQUAL(expected, text);
}
