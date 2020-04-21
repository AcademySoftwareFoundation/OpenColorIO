// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>


#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


const float g_epsilon = 5e-7f;


// Helper method to build unit tests
void AddMatrixTest(OCIOGPUTest & test, OCIO::TransformDirection direction,
                   const double * m, const double * o, bool genericShaderDesc)
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

    test.setErrorThreshold(g_epsilon);

    test.setProcessor(matrix);

    test.setLegacyShader(!genericShaderDesc);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix)
{
    const double m[16] = { 1.1, 0.2, 0.3, 0.4,
                           0.5, 1.6, 0.7, 0.8,
                           0.2, 0.1, 1.1, 0.2,
                           0.3, 0.4, 0.5, 1.6 };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_FORWARD, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, scale)
{
    const double m[16] = { 1.0,  0.0, 0.0, 0.0,
                           0.0, -0.3, 0.0, 0.0,
                           0.0,  0.0, 0.6, 0.0,
                           0.0,  0.0, 0.0, 1.0 };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_FORWARD, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, offset)
{
    const double o[4] = { -0.5, +0.25, -0.25, 0.0 };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_FORWARD, 0x0, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset)
{
    const double m[16] = { 1.1, 0.2, 0.3, 0.4,
                           0.5, 1.6, 0.7, 0.8,
                           0.2, 0.1, 1.1, 0.2,
                           0.3, 0.4, 0.5, 1.6 };

    const double o[4] = { -0.5, -0.25, 0.25, 0.0 };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_FORWARD, m, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_inverse)
{
    const double m[16] = { 1.1, 0.2, 0.3, 0.4,
                           0.5, 1.6, 0.7, 0.8,
                           0.2, 0.1, 1.1, 0.2,
                           0.3, 0.4, 0.5, 1.6 };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_INVERSE, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, scale_inverse)
{
    const double m[16] = { 1.0,  0.0, 0.0, 0.0,
                           0.0, -0.3, 0.0, 0.0,
                           0.0,  0.0, 0.6, 0.0,
                           0.0,  0.0, 0.0, 1.0 };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_INVERSE, m, 0x0, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, offset_inverse)
{
    const double o[4] = { -0.5, +0.25, -0.25, 0.0 };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_INVERSE, 0x0, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset_inverse)
{
    const double m[16] = { 1.1, 0.2, 0.3, 0.4,
                           0.5, 1.6, 0.7, 0.8,
                           0.2, 0.1, 1.1, 0.2,
                           0.3, 0.4, 0.5, 1.6 };

    const double o[4] = { -0.5, -0.25, 0.25, 0.0 };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_INVERSE, m, o, false);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset_generic_shader)
{
    const double m[16] = { 1.1, 0.2, 0.3, 0.4,
                           0.5, 1.6, 0.7, 0.8,
                           0.2, 0.1, 1.1, 0.2,
                           0.3, 0.4, 0.5, 1.6 };

    const double o[4] = { -0., -0.25, 0.25, 0.0 };

    AddMatrixTest(test, OCIO::TRANSFORM_DIR_FORWARD, m, o, true);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset_inverse_generic_shader)
{
    const double m[16] = { 1.1, 0.2, 0.3, 0.4,
                           0.5, 1.6, 0.7, 0.8,
                           0.2, 0.1, 1.1, 0.2,
                           0.3, 0.4, 0.5, 1.6 };

    const double o[4] = { -0.5, -0.25, 0.25, 0.0 };

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
