// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "GpuShader.cpp"

#include "testutils/UnitTest.h"

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
                                                   "1234",
                                                   width, height,
                                                   OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL,
                                                   OCIO::INTERP_TETRAHEDRAL,
                                                   &values[0]));

        OCIO_CHECK_EQUAL(shaderDesc->getNumTextures(), 1U);

        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        const char * uid         = nullptr;
        unsigned w = 0;
        unsigned h = 0;
        OCIO::GpuShaderDesc::TextureType t = OCIO::GpuShaderDesc::TEXTURE_RED_CHANNEL;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OCIO_CHECK_NO_THROW(shaderDesc->getTexture(0, textureName, samplerName, uid, w, h, t, i));

        OCIO_CHECK_EQUAL(std::string(textureName), "lut1");
        OCIO_CHECK_EQUAL(std::string(samplerName), "lut1Sampler");
        OCIO_CHECK_EQUAL(std::string(uid), "1234");
        OCIO_CHECK_EQUAL(width, w);
        OCIO_CHECK_EQUAL(height, h);
        OCIO_CHECK_EQUAL(OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL, t);
        OCIO_CHECK_EQUAL(OCIO::INTERP_TETRAHEDRAL, i);

        OCIO_CHECK_THROW_WHAT(shaderDesc->getTexture(1, textureName, samplerName, uid, w, h, t, i),
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

        OCIO_CHECK_NO_THROW(shaderDesc->addTexture("lut2", "lut2Sampler", "1234", width, height,
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
        OCIO_CHECK_NO_THROW(shaderDesc->add3DTexture("lut1", "lut1Sampler", "1234", edgelen,
                                                     OCIO::INTERP_TETRAHEDRAL,
                                                     &values[0]));

        OCIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 1U);

        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        const char * uid         = nullptr;
        unsigned e = 0;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OCIO_CHECK_NO_THROW(shaderDesc->get3DTexture(0, textureName, samplerName, uid, e, i));

        OCIO_CHECK_EQUAL(std::string(textureName), "lut1");
        OCIO_CHECK_EQUAL(std::string(samplerName), "lut1Sampler");
        OCIO_CHECK_EQUAL(std::string(uid), "1234");
        OCIO_CHECK_EQUAL(edgelen, e);
        OCIO_CHECK_EQUAL(OCIO::INTERP_TETRAHEDRAL, i);

        OCIO_CHECK_THROW_WHAT(shaderDesc->get3DTexture(1, textureName, samplerName, uid, e, i),
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

        OCIO_CHECK_NO_THROW(shaderDesc->add3DTexture("lut1", "lut1Sampler", "1234", edgelen,
                                                     OCIO::INTERP_TETRAHEDRAL,
                                                     &values[0]));

        OCIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 2U);

        // Check the 3D LUT limit

        OCIO_CHECK_THROW(shaderDesc->add3DTexture("lut1", "lut1Sampler", "1234", 130,
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

OCIO_ADD_TEST(GpuShader, legacy_shader)
{
    const unsigned edgelen = 2;

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::LegacyGpuShaderDesc::Create(edgelen);

    {
        const unsigned width  = 3;
        const unsigned height = 2;
        const unsigned size = width*height*3;
        float values[size];

        OCIO_CHECK_EQUAL(shaderDesc->getNumTextures(), 0U);
        OCIO_CHECK_THROW_WHAT(shaderDesc->addTexture("lut1", "lut1Sampler", "1234", width, height,
                                                     OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL,
                                                     OCIO::INTERP_TETRAHEDRAL,
                                                     &values[0]),
                              OCIO::Exception,
                              "1D LUTs are not supported");

        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        const char * uid         = nullptr;
        unsigned w = 0;
        unsigned h = 0;
        OCIO::GpuShaderDesc::TextureType t = OCIO::GpuShaderDesc::TEXTURE_RED_CHANNEL;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OCIO_CHECK_THROW_WHAT(shaderDesc->getTexture(0, textureName, samplerName, uid, w, h, t, i),
                              OCIO::Exception,
                              "1D LUTs are not supported");
    }

    {
        const unsigned size = edgelen*edgelen*edgelen*3;
        float values[size]
            = { 0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f,  0.7f, 0.8f, 0.9f,
                0.1f, 0.2f, 0.3f,  0.4f, 0.5f, 0.6f,  0.7f, 0.8f, 0.9f,  0.7f, 0.8f, 0.9f, };

        OCIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 0U);
        OCIO_CHECK_NO_THROW(shaderDesc->add3DTexture("lut1", "lut1Sampler", "1234", edgelen,
                                                     OCIO::INTERP_TETRAHEDRAL,
                                                     &values[0]));

        OCIO_CHECK_EQUAL(shaderDesc->getNum3DTextures(), 1U);

        const char * textureName = nullptr;
        const char * samplerName = nullptr;
        const char * uid         = nullptr;
        unsigned e = 0;
        OCIO::Interpolation i = OCIO::INTERP_UNKNOWN;

        OCIO_CHECK_NO_THROW(shaderDesc->get3DTexture(0, textureName, samplerName, uid, e, i));

        OCIO_CHECK_EQUAL(std::string(textureName), "lut1");
        OCIO_CHECK_EQUAL(std::string(samplerName), "lut1Sampler");
        OCIO_CHECK_EQUAL(std::string(uid),         "1234");
        OCIO_CHECK_EQUAL(edgelen, e);
        OCIO_CHECK_EQUAL(OCIO::INTERP_TETRAHEDRAL, i);

        OCIO_CHECK_THROW_WHAT(shaderDesc->get3DTexture(1, textureName, samplerName, uid, e, i),
                              OCIO::Exception,
                              "3D LUT access error");

        const float * vals = 0x0;
        OCIO_CHECK_NO_THROW(shaderDesc->get3DTextureValues(0, vals));
        OCIO_CHECK_NE(vals, 0x0);
        for(unsigned idx=0; idx<size; ++idx)
        {
            OCIO_CHECK_EQUAL(values[idx], vals[idx]);
        }

        OCIO_CHECK_THROW_WHAT(shaderDesc->get3DTextureValues(1, vals),
                              OCIO::Exception,
                              "3D LUT access error");

        // Only one 3D LUT

        OCIO_CHECK_THROW_WHAT(shaderDesc->add3DTexture("lut1", "lut1Sampler", "1234", edgelen,
                                                       OCIO::INTERP_TETRAHEDRAL,
                                                       &values[0]),
                              OCIO::Exception,
                              "only one 3D texture allowed");

    }
}

