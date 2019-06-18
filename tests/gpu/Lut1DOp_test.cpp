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
#include "GPUHelpers.h"

#include <stdio.h>
#include <sstream>
#include <string>

OCIO_NAMESPACE_USING


const int LUT3D_EDGE_SIZE = 32;


#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

// For explanation, refer to https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html 
#define _STR(x) #x
#define STR(x) _STR(x)

static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));


namespace
{
    OCIO::FileTransformRcPtr GetFileTransform(const std::string & filename)
    {
        const std::string 
            filepath(ocioTestFilesDir + std::string("/") + filename);

        OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
        file->setSrc(filepath.c_str());
        file->setInterpolation(OCIO::INTERP_LINEAR);
        file->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

        return file;
    }
}

// The LUTs below are identities unless otherwise noted.
// Various sizes are used to test different 1D LUT texture packings on the GPU.
// lut1d_1.spi1d has    512 entries
// lut1d_2.spi1d has   8192 entries
// lut1d_3.spi1d has 131072 entries

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_1_small_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_1.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_1_small_inverse_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_1.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_1_small_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_1.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_1_small_inverse_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_1.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_2_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_2.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(2e-4f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_2_inverse_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_2.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(2e-4f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_2_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_2.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);

    // TODO: Would like to be able to remove the setTestNaN(false) and
    // setTestInfinity(false) from all of these tests.
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_2_inverse_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_2.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-4f);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_3_big_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_3.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_3_big_inverse_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_3.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_3_big_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_3.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_3_big_inverse_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_3.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_3_big_nearest_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_3.spi1d");
    file->setInterpolation(OCIO::INTERP_NEAREST);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, scale_lut1d_4_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_4.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(2e-4f);

    // lut1d_4.spi1d has values outside [0, 1]. Legacy shader is baking ops
    // into a 3D LUT and would clamp outside of [0, 1].
    test.setTestWideRange(false);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, scale_lut1d_4_inverse_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_4.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(5e-5f);

    test.setTestWideRange(false);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, scale_lut1d_4_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_4.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    // TODO: Should be smaller.
    test.setErrorThreshold(1e-4f);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, scale_lut1d_4_inverse_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_4.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, not_linear_lut1d_5_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_5.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(5e-4f);    // Good value for a relative error threshold.

    test.setRelativeComparison(true); // LUT contains values from 0.0f to 64.0f
                                      // explaining why an absolute error could not be used.
}

OCIO_ADD_GPU_TEST(Lut1DOp, not_linear_lut1d_5_inverse_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_5.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_half_domain_unequal_channels)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_halfdom.ctf");
    file->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_file2_test)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_green.ctf");
    file->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);
    
    // LUT has just 32 entries and thus requires a larger tolerance due to
    // index quantization on GPUs.
    test.setErrorThreshold(1e-4f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_hue_adjust_test)
{
    // Note: This LUT has 1024 entries so it tests the "small LUT" path.
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_1024_hue_adjust_test.ctf");
    file->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    // NB: This test has required a tolerance of 0.1 on older graphics cards.
    test.setErrorThreshold(1e-5f);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_half_domain_hue_adjust_test)
{
    // Note: This LUT is half domain and also a "big LUT" so it tests that path.
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_hd_hueAdjust.ctf");
    file->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    // NB: This test has required a tolerance of 0.1 on older graphics cards.
    test.setErrorThreshold(1e-6f);

    // LUT range is 0.0001 -> 10000.0.
    test.setRelativeComparison(true);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_inverse_file1_test)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_inv.ctf");
    file->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    // Inverse LUT leads bigger errors.
    test.setErrorThreshold(1e-4f);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_inverse_file2_test)
{
    // This LUT has an extended domain (entries outside [0,1]) and hence the fast LUT
    // that gets built from it must have a halfDomain for both CPU and GPU.
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_inverse_gpu_err.ctf");
    file->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_inverse_half_file1_test)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_inverse_halfdom_slog_fclut.ctf");
    file->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-4f);

    test.setRelativeComparison(true);
    test.setExpectedMinimalValue(1e-3f);

    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_inverse_half_hue_adjust_file1_test)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_inverse_hd_hueAdjust.ctf");
    file->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

// TODO: Add tests with LUTs with Inf and NaN values.
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc - GPURendererLut1D_8i_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc - GPURendererLut1D_16i_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc - GPURendererLut1D_16f_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc - GPURendererLut1D_16f_half_domain_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc - GPURendererLut1D_32f_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc - GPURendererLut1DTooManyTextures_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc - GPURendererLut1D_File1_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc - GPURendererLut1D_too_small
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc - GPURendererLut1D_half_domain_test
