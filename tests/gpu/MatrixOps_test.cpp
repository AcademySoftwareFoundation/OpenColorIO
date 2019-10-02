// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>


#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


const int LUT3D_EDGE_SIZE = 32;
const float g_epsilon = 5e-7f;


// Helper method to build unit tests
void AddMatrixTest(OCIOGPUTest & test, OCIO::TransformDirection direction,
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

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_FORWARD, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, scale)
{
    const float m[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                          0.0f, -0.3f, 0.0f, 0.0f,
                          0.0f, 0.0f, 0.6f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_FORWARD, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, offset)
{
    const float o[4] = { -0.5f, +0.25f, -0.25f, 0.0f };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_FORWARD, 0x0, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    const float o[4] = { -0.5f, -0.25f, 0.25f, 0.0f };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_FORWARD, m, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_inverse)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_INVERSE, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, scale_inverse)
{
    const float m[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                          0.0f, -0.3f, 0.0f, 0.0f,
                          0.0f, 0.0f, 0.6f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_INVERSE, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, offset_inverse)
{
    const float o[4] = { -0.5f, +0.25f, -0.25f, 0.0f };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_INVERSE, 0x0, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset_inverse)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    const float o[4] = { -0.5f, -0.25f, 0.25f, 0.0f };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_INVERSE, m, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset_generic_shader)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    const float o[4] = { -0.5f, -0.25f, 0.25f, 0.0f };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_FORWARD, m, o, true);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset_inverse_generic_shader)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    const float o[4] = { -0.5f, -0.25f, 0.25f, 0.0f };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_INVERSE, m, o, true);
}


// TODO: syncolor also tests various bit-depths and pixel formats.
// GPURenderer_cases.cpp_inc - IdentityMatrix_test
// GPURenderer_cases.cpp_inc - MatrixWithDiffBitDepth1_test
// GPURenderer_cases.cpp_inc - MatrixWithDiffBitDepth2_test
// GPURenderer_cases.cpp_inc - MatrixWithDiffBitDepth3_test
// GPURenderer_cases.cpp_inc - MatrixRedOnly_test
// GPURenderer_cases.cpp_inc - MatrixGreenOnly_test
// GPURenderer_cases.cpp_inc - MatrixWithOffsets4_test
// GPURenderer_cases.cpp_inc - MatrixOffsetsNotScaledBug_test
