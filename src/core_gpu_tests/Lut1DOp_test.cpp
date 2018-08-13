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

    // TODO: Investigate why the test needs such a threshold
    test.setErrorThreshold(3e-3f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_1_small_inverse_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_1.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);

    test.setContext(file->createEditableCopy(), shaderDesc);

    // TODO: Investigate why the test needs such a threshold
    test.setErrorThreshold(1e-2f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_1_small_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_1.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_1_small_inverse_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_1.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_2_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_2.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(5e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_2_inverse_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_2.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(5e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_2_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_2.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_2_inverse_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_2.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(5e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_3_big_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_3.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_3_big_inverse_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_3.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_3_big_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_3.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_3_big_inverse_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_3.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, lut1d_3_big_nearest_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_3.spi1d");
    // TODO: INTERP_LINEAR is actually being used.
    file->setInterpolation(OCIO::INTERP_NEAREST);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, scale_lut1d_4_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_4.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setWideRange(false);
    test.setErrorThreshold(1e-3f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, scale_lut1d_4_inverse_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_4.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(2*LUT3D_EDGE_SIZE);

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setWideRange(false);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, scale_lut1d_4_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_4.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, scale_lut1d_4_inverse_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_4.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, not_linear_lut1d_5_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_5.spi1d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setRelativeComparison(true);
    test.setErrorThreshold(1e-3f);
}


OCIO_ADD_GPU_TEST(Lut1DOp, not_linear_lut1d_5_inverse_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_5.spi1d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-4f);
}

OCIO_ADD_GPU_TEST(Lut1DOp, not_linear_lut1d_rgb_values_different)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut1d_comp.clf");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(5e-3f);
}
