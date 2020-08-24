// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

// For explanation, refer to https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
#define _STR(x) #x
#define STR(x) _STR(x)

static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));


// Based on testings, the interpolation precision for GPU textures is 8-bits
// so it is the default error threshold for all GPU unit tests.
const float defaultErrorThreshold = 1.0f/256.0f;


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
    config->validate();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");
    test.setProcessor(processor);
    test.setLegacyShader(true);
    test.setErrorThreshold(defaultErrorThreshold);
}

OCIO_ADD_GPU_TEST(Config, several_1D_luts_generic_shader)
{
    std::string configStr = createConfig();
    configStr +=
        "        - !<FileTransform> {src: lut1d_1.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_2.spi1d, interpolation: linear}\n";

    std::istringstream is;
    is.str(configStr);

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is);
    config->validate();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");
    test.setProcessor(processor);
    test.setErrorThreshold(defaultErrorThreshold);

    // TODO: Would like to be able to remove the setTestNaN(false) and
    // setTestInfinity(false) from all of these tests.
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(Config, arbitrary_generic_shader)
{
    std::string configStr = createConfig();
    configStr +=
        "        - !<FileTransform> {src: lut1d_1.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_2.spi1d, interpolation: linear}\n"
        "        - !<LogTransform> {base: 10}\n"
        "        - !<MatrixTransform> {matrix: [0.075573, 0.022197,  0.00223,  0, "\
                                               "0.005901, 0.096928, -0.002829, 0, "\
                                               "0.016134, 0.007406,  0.07646,  0, "\
                                               "0,        0,         0,        1]}\n";

    std::istringstream is;
    is.str(configStr);

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is);
    config->validate();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");

    test.setProcessor(processor);

    auto shaderDesc = test.getShaderDesc();
    shaderDesc->setPixelName("another_pixel_name");
    shaderDesc->setFunctionName("another_func_name");

    // TODO: To be investigated when the new LUT 1D OpData will be in
    test.setErrorThreshold(5e-3f);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(Config, several_luts_generic_shader)
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
    config->validate();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");

    test.setProcessor(processor);
    test.setErrorThreshold(defaultErrorThreshold);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(Config, with_underscores)
{
    // The unit tests validates that there will be no double underscores for
    // GPU resource names as it's forbidden by GLSL.

    std::string configStr = createConfig();
    configStr +=
        "        - !<LogTransform> {base: 10}\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: __lgh__\n"
        "    family: \"\"\n"
        "    equalitygroup: \"\"\n"
        "    bitdepth: unknown\n"
        "    isdata: false\n"
        "    allocation: uniform\n"
        "    allocationvars: [0, 1]\n"
        "    from_reference: !<GroupTransform>\n"
        "      children:\n"
        "        - !<MatrixTransform> {matrix: [0.075573, 0.022197,  0.00223,  0, "\
                                               "0.005901, 0.096928, -0.002829, 0, "\
                                               "0.016134, 0.007406,  0.07646,  0, "\
                                               "0,        0,         0,        1]}\n"
        "        - !<FileTransform> {src: lut1d_3.spi1d, interpolation: linear}\n";

    std::istringstream is;
    is.str(configStr);

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is);
    config->validate();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "__lgh__");
    test.setProcessor(processor);

    auto shaderDesc = test.getShaderDesc();
    shaderDesc->setResourcePrefix("ocio___");
    shaderDesc->setPixelName("another_pixel_name__");
    shaderDesc->setFunctionName("__another_func_name____");

    test.setErrorThreshold(defaultErrorThreshold);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}
