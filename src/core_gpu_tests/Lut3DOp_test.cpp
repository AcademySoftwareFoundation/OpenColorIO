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


const float g_epsilon = 1e-3f;


OCIO_ADD_GPU_TEST(Lut3DOp, red_only_using_CSP_file_legacy_shader)
{
    // Used a format I know to create a 3D lut file.
    //  Any other file format would have been good also.

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
    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(Lut3DOp, green_only_using_CSP_file_legacy_shader)
{
    // Used a format I know to create a 3D lut file.
    //  Any other file format would have been good also.

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
    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(Lut3DOp, blue_only_using_CSP_file_legacy_shader)
{
    // Used a format I know to create a 3D lut file.
    //  Any other file format would have been good also.

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
    test.setErrorThreshold(g_epsilon);
}


OCIO_ADD_GPU_TEST(Lut3DOp, arbitrary_using_CSP_file_legacy_shader)
{
    // Used a format I know to create a 3D lut file.
    //  Any other file format would have been good also.

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

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);

    // TODO: Small luts not being resampled for now, such error threshold is expected
    //       The legacy shader has a better error threashold because 
    //       it converts all luts in one 3d lut of dimension LUT3D_EDGE_SIZE
    //       which performs a resampling of small luts.
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


OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_file_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_1.spi3d");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(5e-4f);
}


OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_file_nearest_generic_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_1.spi3d");
    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    
    // TODO: nearest is not implemented but tetrahedral is using
    // GPU texture nearest interpolation.
    file->setInterpolation(OCIO::INTERP_TETRAHEDRAL);

    test.setContext(file->createEditableCopy(), shaderDesc);
    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_identity_ctf_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_identity_32f.clf");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
    // TODO: Small luts not being resampled for now, such error threshold is expected
    //       The legacy shader has a better error threashold because 
    //       it converts all luts in one 3d lut of dimension LUT3D_EDGE_SIZE
    //       which performs a resampling of small luts.
    test.setErrorThreshold(1e-2f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_3_clf_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_3x3x3_32f.clf");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_17_clf_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_17x17x17_32f_12i.ctf");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);

    // TODO: Small luts not being resampled for now, such error threshold is expected
    //       The legacy shader has a better error threashold because 
    //       it converts all luts in one 3d lut of dimension LUT3D_EDGE_SIZE
    //       which performs a resampling of small luts.
    test.setErrorThreshold(1e-3f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, 3dlut_17_tetra_clf_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_tetra_17x17x17_32f_12i.ctf");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, 3lut3d_bizarre_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_bizarre.clf");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);

    // TODO: Small luts not being resampled for now, such error threshold is expected
    //       The legacy shader has a better error threashold because 
    //       it converts all luts in one 3d lut of dimension LUT3D_EDGE_SIZE
    //       which performs a resampling of small luts.
    test.setErrorThreshold(1e-2f);
}

OCIO_ADD_GPU_TEST(Lut3DOp, 3lut3d_bizarre_tetra_shader)
{
    OCIO::FileTransformRcPtr file = GetFileTransform("lut3d_bizarre_tetra.clf");

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);

    test.setErrorThreshold(1e-6f);
}

// TODO: Test biggest 3D LUT (OpData::Lut3D::maxSupportedLength)

