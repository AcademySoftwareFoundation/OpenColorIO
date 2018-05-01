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

//#include <stdio.h>
#include <sstream>
//#include <string>

OCIO_NAMESPACE_USING


#define _STR(x) #x
#define STR(x) _STR(x)


static const std::string ociodir(STR(OCIO_SOURCE_DIR));



std::string createConfig()
{
    const std::string search_path(
        ociodir + std::string("/src/core_tests/testfiles/"));

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
        "    to_reference: !<GroupTransform>\n"
        "      children:\n"
        "        - !<FileTransform> {src: lut1d_1.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_2.spi1d, interpolation: linear}\n"
        "        - !<FileTransform> {src: lut1d_3.spi1d, interpolation: linear}\n";

    return ocioConfigStr;
}

OCIO_ADD_GPU_TEST(Config, several_1D_luts_legacy_shader)
{
    std::istringstream is;
    is.str(createConfig());

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is);
    config->sanityCheck();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");

    OCIO::GpuShaderDescRcPtr shaderDesc
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(32);

    // TODO: Investigate why the test needs such a threshold
    test.setContext(processor, shaderDesc, 1e-2f);
}

OCIO_ADD_GPU_TEST(Config, several_1D_luts_generic_shader)
{
    std::istringstream is;
    is.str(createConfig());

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromStream(is);
    config->sanityCheck();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor("raw", "lgh");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    // TODO: Investigate why the test needs such a threshold
    test.setContext(processor, shaderDesc, 1e-2f);
}
