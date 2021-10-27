// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// NOTE:
// The file is a copy and paste from the corresponding GPU unitest i.e. []/tests/gpu/MatrixOp_test.cpp.
// Keep the two files in sync. to increase the test coverage.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


OCIO_OSL_TEST(MatrixOps, matrix)
{
    static constexpr double m[16] = { 1.1, 0.2, 0.3, 0.4,
                                      0.5, 1.6, 0.7, 0.8,
                                      0.2, 0.1, 1.1, 0.2,
                                      0.3, 0.4, 0.5, 1.6 };

    auto matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);

    m_data->m_transform = matrix;
}


OCIO_OSL_TEST(MatrixOps, scale)
{
    static constexpr double m[16] = { 1.0,  0.0, 0.0, 0.0,
                                      0.0, -0.3, 0.0, 0.0,
                                      0.0,  0.0, 0.6, 0.0,
                                      0.0,  0.0, 0.0, 1.0 };

    auto matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);

    m_data->m_transform = matrix;
}


OCIO_OSL_TEST(MatrixOps, offset)
{
    static constexpr double o[4] = { -0.5, +0.25, -0.25, 0.0 };

    auto matrix = OCIO::MatrixTransform::Create();
    matrix->setOffset(o);

    m_data->m_transform = matrix;
}


OCIO_OSL_TEST(MatrixOps, matrix_offset)
{
    static constexpr double m[16] = { 1.1, 0.2, 0.3, 0.4,
                                      0.5, 1.6, 0.7, 0.8,
                                      0.2, 0.1, 1.1, 0.2,
                                      0.3, 0.4, 0.5, 1.6 };

    static constexpr double o[4] = { -0.5, -0.25, 0.25, 0.0 };

    auto matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);
    matrix->setOffset(o);

    m_data->m_transform = matrix;
}


OCIO_OSL_TEST(MatrixOps, matrix_inverse)
{
    static constexpr double m[16] = { 1.1, 0.2, 0.3, 0.4,
                                      0.5, 1.6, 0.7, 0.8,
                                      0.2, 0.1, 1.1, 0.2,
                                      0.3, 0.4, 0.5, 1.6 };

    auto matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);
    matrix->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = matrix;
}


OCIO_OSL_TEST(MatrixOps, scale_inverse)
{
    static constexpr double m[16] = { 1.0,  0.0, 0.0, 0.0,
                                      0.0, -0.3, 0.0, 0.0,
                                      0.0,  0.0, 0.6, 0.0,
                                      0.0,  0.0, 0.0, 1.0 };

    auto matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);
    matrix->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = matrix;
}


OCIO_OSL_TEST(MatrixOps, offset_inverse)
{
    static constexpr double o[4] = { -0.5, +0.25, -0.25, 0.0 };

    auto matrix = OCIO::MatrixTransform::Create();
    matrix->setOffset(o);
    matrix->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = matrix;
}


OCIO_OSL_TEST(MatrixOps, matrix_offset_inverse)
{
    static constexpr double m[16] = { 1.1, 0.2, 0.3, 0.4,
                                      0.5, 1.6, 0.7, 0.8,
                                      0.2, 0.1, 1.1, 0.2,
                                      0.3, 0.4, 0.5, 1.6 };

    static constexpr double o[4] = { -0.5, -0.25, 0.25, 0.0 };

    auto matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);
    matrix->setOffset(o);
    matrix->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = matrix;
}


OCIO_OSL_TEST(MatrixOps, matrix_offset_generic_shader)
{
    static constexpr double m[16] = { 1.1, 0.2, 0.3, 0.4,
                                      0.5, 1.6, 0.7, 0.8,
                                      0.2, 0.1, 1.1, 0.2,
                                      0.3, 0.4, 0.5, 1.6 };

    static constexpr double o[4] = { -0., -0.25, 0.25, 0.0 };

    auto matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);
    matrix->setOffset(o);

    m_data->m_transform = matrix;
}


OCIO_OSL_TEST(MatrixOps, matrix_offset_inverse_generic_shader)
{
    static constexpr double m[16] = { 1.1, 0.2, 0.3, 0.4,
                                      0.5, 1.6, 0.7, 0.8,
                                      0.2, 0.1, 1.1, 0.2,
                                      0.3, 0.4, 0.5, 1.6 };

    static constexpr double o[4] = { -0.5, -0.25, 0.25, 0.0 };

    auto matrix = OCIO::MatrixTransform::Create();
    matrix->setMatrix(m);
    matrix->setOffset(o);
    matrix->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = matrix;
}
