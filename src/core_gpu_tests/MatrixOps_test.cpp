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

OCIO_NAMESPACE_USING



const int LUT3D_EDGE_SIZE = 32;
const float g_epsilon = 5e-7f;


// Helper method to build unit tests
void AddMatrixTest(OCIOGPUTest & test, TransformDirection direction, 
                   const float * m, const float * o, bool genericShaderDesc)
{
    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    matrix->setDirection(direction);
    if(m)
    {
        matrix->setMatrix(m);
    }
    if(o)
    {
        matrix->setOffset(o);
    }

    OCIO::GpuShaderDescRcPtr shaderDesc 
        = genericShaderDesc ? OCIO::GpuShaderDesc::CreateShaderDesc()
          : OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    test.setErrorThreshold(g_epsilon);

    test.setContext(matrix->createEditableCopy(), shaderDesc);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    AddMatrixTest(test, TRANSFORM_DIR_FORWARD, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, scale)
{
    const float m[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                          0.0f, -0.3f, 0.0f, 0.0f,
                          0.0f, 0.0f, 0.6f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f };

    AddMatrixTest(test, TRANSFORM_DIR_FORWARD, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, offset)
{
    const float o[4] = { -0.5f, +0.25f, -0.25f, 0.0f };

    AddMatrixTest(test, TRANSFORM_DIR_FORWARD, 0x0, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    const float o[4] = { -0.5f, -0.25f, 0.25f, 0.0f };
    
    AddMatrixTest(test, TRANSFORM_DIR_FORWARD, m, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_inverse)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    AddMatrixTest(test, TRANSFORM_DIR_INVERSE, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, scale_inverse)
{
    const float m[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                          0.0f, -0.3f, 0.0f, 0.0f,
                          0.0f, 0.0f, 0.6f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f };

    AddMatrixTest(test, TRANSFORM_DIR_INVERSE, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, offset_inverse)
{
    const float o[4] = { -0.5f, +0.25f, -0.25f, 0.0f };

    AddMatrixTest(test, TRANSFORM_DIR_INVERSE, 0x0, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset_inverse)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    const float o[4] = { -0.5f, -0.25f, 0.25f, 0.0f };
    
    AddMatrixTest(test, TRANSFORM_DIR_INVERSE, m, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset_generic_shader)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    const float o[4] = { -0.5f, -0.25f, 0.25f, 0.0f };
    
    AddMatrixTest(test, TRANSFORM_DIR_FORWARD, m, o, true);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset_inverse_generic_shader)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    const float o[4] = { -0.5f, -0.25f, 0.25f, 0.0f };
    
    AddMatrixTest(test, TRANSFORM_DIR_INVERSE, m, o, true);
}
