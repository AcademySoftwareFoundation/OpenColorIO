// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/MatrixTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(MatrixTransform, basic)
{
    OCIO::MatrixTransformRcPtr matrix = OCIO::MatrixTransform::Create();
    OCIO_CHECK_EQUAL(matrix->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    double m44[16];
    double offset4[4];
    matrix->getMatrix(m44);
    matrix->getOffset(offset4);

    OCIO_CHECK_EQUAL(m44[0], 1.0);
    OCIO_CHECK_EQUAL(m44[1], 0.0);
    OCIO_CHECK_EQUAL(m44[2], 0.0);
    OCIO_CHECK_EQUAL(m44[3], 0.0);

    OCIO_CHECK_EQUAL(m44[4], 0.0);
    OCIO_CHECK_EQUAL(m44[5], 1.0);
    OCIO_CHECK_EQUAL(m44[6], 0.0);
    OCIO_CHECK_EQUAL(m44[7], 0.0);

    OCIO_CHECK_EQUAL(m44[8], 0.0);
    OCIO_CHECK_EQUAL(m44[9], 0.0);
    OCIO_CHECK_EQUAL(m44[10], 1.0);
    OCIO_CHECK_EQUAL(m44[11], 0.0);

    OCIO_CHECK_EQUAL(m44[12], 0.0);
    OCIO_CHECK_EQUAL(m44[13], 0.0);
    OCIO_CHECK_EQUAL(m44[14], 0.0);
    OCIO_CHECK_EQUAL(m44[15], 1.0);

    OCIO_CHECK_EQUAL(offset4[0], 0.0);
    OCIO_CHECK_EQUAL(offset4[1], 0.0);
    OCIO_CHECK_EQUAL(offset4[2], 0.0);
    OCIO_CHECK_EQUAL(offset4[3], 0.0);

    m44[0]  = 1.0;
    m44[1]  = 1.01;
    m44[2]  = 1.02;
    m44[3]  = 1.03;

    m44[4]  = 1.04;
    m44[5]  = 1.05;
    m44[6]  = 1.06;
    m44[7]  = 1.07;

    m44[8]  = 1.08;
    m44[9]  = 1.09;
    m44[10] = 1.10;
    m44[11] = 1.11;

    m44[12] = 1.12;
    m44[13] = 1.13;
    m44[14] = 1.14;
    m44[15] = 1.15;

    offset4[0] = 1.0;
    offset4[1] = 1.1;
    offset4[2] = 1.2;
    offset4[3] = 1.3;

    matrix->setMatrix(m44);
    matrix->setOffset(offset4);

    double m44r[16];
    double offset4r[4];

    matrix->getMatrix(m44r);
    matrix->getOffset(offset4r);

    for (int i = 0; i < 16; ++i)
    {
        OCIO_CHECK_EQUAL(m44r[i], m44[i]);
    }

    OCIO_CHECK_EQUAL(offset4r[0], 1.0);
    OCIO_CHECK_EQUAL(offset4r[1], 1.1);
    OCIO_CHECK_EQUAL(offset4r[2], 1.2);
    OCIO_CHECK_EQUAL(offset4r[3], 1.3);

    OCIO_CHECK_EQUAL(matrix->getFileInputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);
    OCIO_CHECK_EQUAL(matrix->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);

    matrix->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT8);
    matrix->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    OCIO_CHECK_EQUAL(matrix->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(matrix->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    // File bit-depth does not affect values.
    matrix->getMatrix(m44r);
    matrix->getOffset(offset4r);

    for (int i = 0; i < 16; ++i)
    {
        OCIO_CHECK_EQUAL(m44r[i], m44[i]);
    }

    OCIO_CHECK_EQUAL(offset4r[0], 1.0);
    OCIO_CHECK_EQUAL(offset4r[1], 1.1);
    OCIO_CHECK_EQUAL(offset4r[2], 1.2);
    OCIO_CHECK_EQUAL(offset4r[3], 1.3);

    OCIO_CHECK_EQUAL(matrix->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    matrix->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(matrix->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_EQUAL(matrix->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(matrix->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);
}

OCIO_ADD_TEST(MatrixTransform, equals)
{
    OCIO::MatrixTransformRcPtr matrix1 = OCIO::MatrixTransform::Create();
    OCIO::MatrixTransformRcPtr matrix2 = OCIO::MatrixTransform::Create();

    OCIO_CHECK_ASSERT(matrix1->equals(*matrix2));

    matrix1->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(!matrix1->equals(*matrix2));
    matrix1->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    double m44[16];
    double offset4[4];
    matrix1->getMatrix(m44);
    matrix1->getOffset(offset4);
    m44[0] = 1.0 + 1e-6;
    matrix1->setMatrix(m44);
    OCIO_CHECK_ASSERT(!matrix1->equals(*matrix2));
    m44[0] = 1.0;
    matrix1->setMatrix(m44);
    OCIO_CHECK_ASSERT(matrix1->equals(*matrix2));

    offset4[0] = 1e-6;
    matrix1->setOffset(offset4);
    OCIO_CHECK_ASSERT(!matrix1->equals(*matrix2));
}

