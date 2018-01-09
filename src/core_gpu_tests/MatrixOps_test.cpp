/*
    Made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
*/


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO = OCIO_NAMESPACE;
#include "GPUUnitTest.h"

OCIO_NAMESPACE_USING



const float epsilon = 1e-6f;


OCIO_ADD_GPU_TEST(MatrixOps, matrix)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);

    test.setContext(matrix->createEditableCopy(), epsilon);
}


OCIO_ADD_GPU_TEST(MatrixOps, scale)
{
    const float m[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                          0.0f, -0.3f, 0.0f, 0.0f,
                          0.0f, 0.0f, 0.6f, 0.0f,
                          0.0f, 0.0f, 0.0f, 1.0f };

    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);

    test.setContext(matrix->createEditableCopy(), epsilon);
}


OCIO_ADD_GPU_TEST(MatrixOps, offset)
{
    const float o[4] = { -0.5f, +0.25f, -0.25f, 0.0f };

    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    matrix->setOffset(o);

    test.setContext(matrix->createEditableCopy(), epsilon);
}


OCIO_ADD_GPU_TEST(MatrixOps, matrix_offset)
{
    const float m[16] = { 1.1f, 0.2f, 0.3f, 0.4f,
                          0.5f, 1.6f, 0.7f, 0.8f,
                          0.2f, 0.1f, 1.1f, 0.2f,
                          0.3f, 0.4f, 0.5f, 1.6f };

    const float o[4] = { -0.5f, -0.25f, 0.25f, 0.0f };
    
    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);
    matrix->setOffset(o);

    test.setContext(matrix->createEditableCopy(), epsilon);
}
