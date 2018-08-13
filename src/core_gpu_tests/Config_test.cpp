/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO = OCIO_NAMESPACE;
#include "GPUUnitTest.h"

#include <sstream>

OCIO_NAMESPACE_USING


#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

// For explanation, refer to https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html 
#define _STR(x) #x
#define STR(x) _STR(x)

static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));


std::string createConfig()
{
    const std::string search_path(ocioTestFilesDir + std::string("/"));

    std::string ocioConfigStr =
        "ocio_profile_version: 1\n"
        "\n"
        "search_path: ";

    ocioConfigStr += search_path;

    ocioConfigStr +=
        "\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: raw\n"
        "  reference: raw\n"
        "  scene_linear: raw\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Lgh, colorspace: lgh}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: raw\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: lgh\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "    allocationvars: [0, 1]\n"
        "    from_reference: !<GroupTransform>\n"
        "      children:\n";

    return ocioConfigStr;
}

OCIO_ADD_GPU_TEST(Config, several_1D_luts_legacy_shader)
{
    std::string configStr = createConfig();
    configStr +=
        "        - !<FileTransform> {src: lut1d_1.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_2.spi1d, interpolation: linear}\n";

    std::istringstream is;
    is.str(configStr);

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is);
    config->sanityCheck();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");
    OCIO::GpuShaderDescRcPtr shaderDesc
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(64);
    test.setContextProcessor(processor, shaderDesc);

    test.setWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-3f);
}

OCIO_ADD_GPU_TEST(Config, several_1D_luts)
{
    std::string configStr = createConfig();
    configStr +=
        "        - !<FileTransform> {src: lut1d_1.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_2.spi1d, interpolation: linear}\n";

    std::istringstream is;
    is.str(configStr);

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is);
    config->sanityCheck();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");
    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContextProcessor(processor, shaderDesc);

    test.setWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Config, arbitrary)
{
    std::string configStr = createConfig();
    configStr +=
        "        - !<FileTransform> {src: lut1d_1.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_2.spi1d, interpolation: linear}\n"
        "        - !<LogTransform> {base: 10}\n"
        "        - !<MatrixTransform> {matrix: [0.07557378, 0.02219778,  0.00223078,  0, "\
                                               "0.00590178, 0.09692878, -0.00282978,  0, "\
                                               "0.01613478, 0.00740678,  0.07646078,  0, "\
                                               "0,          0,           0,           1]}\n";

    std::istringstream is;
    is.str(configStr);

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is);
    config->sanityCheck();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");

    // Change some default values...
    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    shaderDesc->setPixelName("another_pixel_name");
    shaderDesc->setFunctionName("another_func_name");

    test.setContextProcessor(processor, shaderDesc);

    test.setWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-5f); // because Log precision is 1e-5f, refer to LopOps_Test.cpp
}

// The test only validates that several textures could now be handled.
OCIO_ADD_GPU_TEST(Config, several_luts)
{
    std::string configStr = createConfig();
    configStr +=
        "        - !<FileTransform> {src: lut1d_1.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_2.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_1.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_2.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_1.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_2.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_3.spi1d, interpolation: linear}\n";

    std::istringstream is;
    is.str(configStr);

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is);
    config->sanityCheck();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContextProcessor(processor, shaderDesc);

    test.setWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-6f);
}


// The test only validates that op shader programs do no conflict.
OCIO_ADD_GPU_TEST(Config, several_ops)
{
    std::string configStr = createConfig();
    configStr +=
        "        - !<FileTransform> {src: lut1d_hue_adjust_test.ctf}\n"
        "        - !<FileTransform> {src: lut1d_hue_adjust_test.ctf}\n"
        "        - !<FileTransform> {src: lut1d_4.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_4.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut3d_example.clf, interpolation: tetrahedral}\n"
        "        - !<FileTransform> {src: lut3d_example.clf, interpolation: tetrahedral}\n"
        "        - !<CDLTransform> { slope: [1.1, 1, 1], offset: [0, 0.5, 0], "\
                                    "power: [1, 1, 1.3], sat: 1.2}\n"
        "        - !<CDLTransform> { slope: [1.2, 1, 1], offset: [0, 0.7, 0], "\
                                    "power: [1, 1, 1.4], sat: 1.5}\n";

    std::istringstream is;
    is.str(configStr);

    OCIO::ConfigRcPtr config = OCIO::Config::CreateFromStream(is)->createEditableCopy();
    config->setVersion(2);
    config->sanityCheck();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContextProcessor(processor, shaderDesc);

    test.setWideRange(true);
    test.setRelativeComparison(false);
    test.setErrorThreshold(1e-5f);
}
