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

#include <math.h>


namespace OCIO = OCIO_NAMESPACE;
#include "GPUUnitTest.h"

OCIO_NAMESPACE_USING


const int LUT3D_EDGE_SIZE = 64;

const float g_epsilon = 1e-5f;
const float g_epsilon_inverse = 1e-4f;


// Helper method to build unit tests
void AddLogTest(OCIOGPUTest & test, 
                OCIO::GpuShaderDescRcPtr & shaderDesc,
                TransformDirection direction,
                float base, 
                float epsilon)
{
    OCIO::LogTransformRcPtr log = OCIO::LogTransform::Create();
    log->setDirection(direction);
    log->setBase(base);

    test.setErrorThreshold(epsilon);

    test.setContext(log->createEditableCopy(), shaderDesc);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_10)
{
    const float base10 = 10.0f;

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, base10, g_epsilon);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_inverse)
{
    const float base10 = 10.0f;

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, base10, g_epsilon_inverse);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_generic_shader)
{
    const float base10 = 10.0f;

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, base10, g_epsilon);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_10_inverse_generic_shader)
{
    const float base10 = 10.0f;

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, base10, g_epsilon_inverse);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler)
{
    const float eulerConstant = expf(1.0f);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, eulerConstant, g_epsilon);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_inverse)
{
    const float eulerConstant = expf(1.0f);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, eulerConstant, g_epsilon_inverse);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_generic_shader)
{
    const float eulerConstant = expf(1.0f);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, eulerConstant, g_epsilon);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_euler_inverse_generic_shader)
{
    const float eulerConstant = expf(1.0f);

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, eulerConstant, g_epsilon_inverse);
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_1_inverse)
{
    const float base1 = 1.0f;

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, base1, g_epsilon);   
}


OCIO_ADD_GPU_TEST(LogTransform, LogBase_1_inverse_generic_shader)
{
    const float base1 = 1.0f;

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddLogTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, base1, g_epsilon);   
}


