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


OCIO_ADD_GPU_TEST(Lut3DOp, red_only_using_CSP_file_legacy_shader)
{
    // Any other 3D LUT file format would have been good also.

    std::ostringstream content;
    content << "CSPLUTV100"                                  << "\n";
    content << "3D"                                          << "\n";
    content << ""                                            << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << ""                                            << "\n";
    content << "2 2 2"                                       << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "1.0 0.0 0.0"                                 << "\n";
    content << "1.0 0.0 0.0"                                 << "\n";
    content << "1.0 0.0 0.0"                                 << "\n";
    content << "1.0 0.0 0.0"                                 << "\n";


    const std::string filename = createTempFile(".csp", content.str());

    // Create the transform & set the unit test

    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc(filename.c_str());
    file->setInterpolation(OCIO::INTERP_LINEAR);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(2e-4f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, green_only_using_CSP_file_legacy_shader)
{
    // Any other 3D LUT file format would have been good also.

    std::ostringstream content;
    content << "CSPLUTV100"                                  << "\n";
    content << "3D"                                          << "\n";
    content << ""                                            << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << ""                                            << "\n";
    content << "2 2 2"                                       << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 1.0 0.0"                                 << "\n";
    content << "0.0 1.0 0.0"                                 << "\n";
    content << "0.0 1.0 0.0"                                 << "\n";
    content << "0.0 1.0 0.0"                                 << "\n";


    const std::string filename = createTempFile(".csp", content.str());

    // Create the transform & set the unit test

    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc(filename.c_str());
    file->setInterpolation(OCIO::INTERP_LINEAR);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(2e-4f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, blue_only_using_CSP_file_legacy_shader)
{
    // Any other 3D LUT file format would have been good also.

    std::ostringstream content;
    content << "CSPLUTV100"                                  << "\n";
    content << "3D"                                          << "\n";
    content << ""                                            << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << ""                                            << "\n";
    content << "2 2 2"                                       << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 0.0"                                 << "\n";
    content << "0.0 0.0 1.0"                                 << "\n";
    content << "0.0 0.0 1.0"                                 << "\n";
    content << "0.0 0.0 1.0"                                 << "\n";
    content << "0.0 0.0 1.0"                                 << "\n";


    const std::string filename = createTempFile(".csp", content.str());

    // Create the transform & set the unit test

    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc(filename.c_str());
    file->setInterpolation(OCIO::INTERP_LINEAR);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(2e-4f);
}


OCIO_ADD_GPU_TEST(Lut3DOp, arbitrary_using_CSP_file_legacy_shader)
{
    // Any other 3D LUT file format would have been good also.

    std::ostringstream content;
    content << "CSPLUTV100"                                  << "\n";
    content << "3D"                                          << "\n";
    content << ""                                            << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << ""                                            << "\n";
    content << "2 2 2"                                       << "\n";
    content << "0.100000 0.100000 0.100000"                  << "\n";
    content << "1.100000 0.100000 0.100000"                  << "\n";
    content << "0.100000 1.100000 0.100000"                  << "\n";
    content << "1.100000 1.100000 0.100000"                  << "\n";
    content << "0.100000 0.100000 1.100000"                  << "\n";
    content << "1.100000 0.100000 1.100000"                  << "\n";
    content << "0.100000 1.100000 1.100000"                  << "\n";
    content << "1.100000 1.100000 1.100000"                  << "\n";


    const std::string filename = createTempFile(".csp", content.str());

    // Create the transform & set the unit test

    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc(filename.c_str());
    file->setInterpolation(OCIO::INTERP_LINEAR);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(2e-4f);
}


OCIO_ADD_GPU_TEST(Lut3DOp, arbitrary_using_CSP_file)
{
    std::ostringstream content;
    content << "CSPLUTV100"                                  << "\n";
    content << "3D"                                          << "\n";
    content << ""                                            << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "2"                                           << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << "0.0 1.0"                                     << "\n";
    content << ""                                            << "\n";
    content << "2 2 2"                                       << "\n";
    content << "0.100000 0.100000 0.100000"                  << "\n";
    content << "1.100000 0.100000 0.100000"                  << "\n";
    content << "0.100000 1.100000 0.100000"                  << "\n";
    content << "1.100000 1.100000 0.100000"                  << "\n";
    content << "0.100000 0.100000 1.100000"                  << "\n";
    content << "1.100000 0.100000 1.100000"                  << "\n";
    content << "0.100000 1.100000 1.100000"                  << "\n";
    content << "1.100000 1.100000 1.100000"                  << "\n";


    const std::string filename = createTempFile(".csp", content.str());

    // Create the transform & set the unit test

    OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
    file->setSrc(filename.c_str());
    file->setInterpolation(OCIO::INTERP_LINEAR);

    OCIO::GpuShaderDescRcPtr shaderDesc  = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    // TODO: Small LUTs not being resampled for now, such error threshold is expected
    //       The legacy shader has a better error threashold because 
    //       it converts all LUTs in one 3D LUT of dimension LUT3D_EDGE_SIZE
    //       which performs a resampling of small LUTs.
    test.setErrorThreshold(1e-2f);
}



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


OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_file_legacy_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_1.spi3d");

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(5e-4f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_file_spi3d_linear)
{
    // Linear interpolation
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_1.spi3d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(5e-4f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_file_spi3d_tetra)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_1.spi3d");
    file->setInterpolation(OCIO::INTERP_TETRAHEDRAL);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);

    // TODO: Would like to be able to remove the setTestNaN(false) and
    // setTestInfinity(false) from all of these tests.
    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(Lut3DOp, inv3dlut_file_spi3d_linear)
{
#if !defined(NDEBUG) && defined(WIN32)
    // TODO: 3D LUT inversion might be very slow in debug on windows.
    OCIO_DISABLE_GPU_TEST();
#endif
    // The test uses the FAST style of inverse on both CPU and GPU.
    // The FAST style uses EXACT inversion to build an approximate inverse
    // that may be applied as a forward Lut3D.  
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_1.spi3d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1.2e-3f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, inv3dlut_file_spi3d_tetra)
{
#if !defined(NDEBUG) && defined(WIN32)
    OCIO_DISABLE_GPU_TEST();
#endif
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_1.spi3d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    // Note: Currently the interpolation style is ignored when applying the
    // inverse LUT, so this test produces the same result as the previous one.
    file->setInterpolation(OCIO::INTERP_TETRAHEDRAL);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1.2e-3f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_file_spi3d_bizarre_linear)
{
    // Linear interpolation
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_bizarre.spi3d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    // This is due to the fact that the LUT is small and to
    // the GPU 8-bit index quantization.
    test.setErrorThreshold(1e-2f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_file_spi3d_bizarre_tetra)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_bizarre.spi3d");
    file->setInterpolation(OCIO::INTERP_TETRAHEDRAL);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);

    test.setTestNaN(false);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(Lut3DOp, inv3dlut_file_spi3d_bizarre_linear)
{
#if !defined(NDEBUG) && defined(WIN32)
    OCIO_DISABLE_GPU_TEST();
#endif
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_bizarre.spi3d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(3e-4f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, inv3dlut_file_spi3d_bizarre_tetra)
{
#if !defined(NDEBUG) && defined(WIN32)
    OCIO_DISABLE_GPU_TEST();
#endif
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_bizarre.spi3d");
    file->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    file->setInterpolation(OCIO::INTERP_TETRAHEDRAL);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(3e-4f);
}

// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc GPURendererLut3D_File2_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc GPURendererLut3D_File3_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc GPURendererLut3D_File4_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc GPURendererInvLut3D_File1_test
// TODO: Port syncolor test: renderer\test\GPURenderer_cases.cpp_inc GPURendererBiggestSupportedLut3D_test
