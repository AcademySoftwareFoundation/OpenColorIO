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


#ifdef OCIO_SOURCE_DIR


// For explanation, refer to https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html 
#define _STR(x) #x
#define STR(x) _STR(x)


static const std::string ociodir(STR(OCIO_SOURCE_DIR));


namespace
{
    OCIO::FileTransformRcPtr GetFileTransform(const std::string & filename)
    {
        const std::string 
            filepath(ociodir + std::string("/src/core_tests/testfiles/") + filename);

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
    file->setInterpolation(OCIO::INTERP_NEAREST);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    test.setContext(file->createEditableCopy(), shaderDesc);
}

#endif