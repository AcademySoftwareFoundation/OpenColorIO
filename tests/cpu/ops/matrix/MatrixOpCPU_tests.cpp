// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/matrix/MatrixOpCPU.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


// TODO: syncolor also tests various bit-depths and pixel formats.
// CPURenderer_cases.cpp_inc - CPURendererMatrix
// CPURenderer_cases.cpp_inc - CPURendererMatrixWithOffsets
// CPURenderer_cases.cpp_inc - CPURendererMatrixWithOffsets4
// CPURenderer_cases.cpp_inc - CPURendererMatrixWithOffsets_check_scaling
// CPURenderer_cases.cpp_inc - CPURendererMatrixWithOffsets4_check_scaling
// CPURenderer_cases.cpp_inc - CPURendererMatrix_half

OCIO_ADD_TEST(MatrixOpCPU, scale_renderer)
{
    OCIO::ConstMatrixOpDataRcPtr mat(OCIO::MatrixOpData::CreateDiagonalMatrix(2.0));

    OCIO::ConstOpCPURcPtr op = OCIO::GetMatrixRenderer(mat);
    OCIO_CHECK_ASSERT((bool)op);

    const OCIO::ScaleRenderer* scaleOp = dynamic_cast<const OCIO::ScaleRenderer*>(op.get());
    OCIO_CHECK_ASSERT(scaleOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, rgba, 1);

    OCIO_CHECK_EQUAL(rgba[0], 8.f);
    OCIO_CHECK_EQUAL(rgba[1], 6.f);
    OCIO_CHECK_EQUAL(rgba[2], 4.f);
    OCIO_CHECK_EQUAL(rgba[3], 2.f);
}

OCIO_ADD_TEST(MatrixOpCPU, scale_with_offset_renderer)
{
    OCIO::MatrixOpDataRcPtr mat(OCIO::MatrixOpData::CreateDiagonalMatrix(2.0));

    mat->setOffsetValue(0, 1.f);
    mat->setOffsetValue(1, 2.f);
    mat->setOffsetValue(2, 3.f);
    mat->setOffsetValue(3, 4.f);

    OCIO::ConstMatrixOpDataRcPtr m = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(mat);
    OCIO::ConstOpCPURcPtr op = OCIO::GetMatrixRenderer(m);
    OCIO_CHECK_ASSERT((bool)op);

    const OCIO::ScaleWithOffsetRenderer * scaleOffOp
        = dynamic_cast<const OCIO::ScaleWithOffsetRenderer*>(op.get());
    OCIO_CHECK_ASSERT(scaleOffOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, rgba, 1);

    OCIO_CHECK_EQUAL(rgba[0], 9.f);
    OCIO_CHECK_EQUAL(rgba[1], 8.f);
    OCIO_CHECK_EQUAL(rgba[2], 7.f);
    OCIO_CHECK_EQUAL(rgba[3], 6.f);
}


OCIO_ADD_TEST(MatrixOpCPU, matrix_with_offset_renderer)
{
    OCIO::MatrixOpDataRcPtr mat(OCIO::MatrixOpData::CreateDiagonalMatrix(2.0));

    // set offset
    mat->setOffsetValue(0, 1.f);
    mat->setOffsetValue(1, 2.f);
    mat->setOffsetValue(2, 3.f);
    mat->setOffsetValue(3, 4.f);

    // make not diag
    mat->setArrayValue(3, 0.5f);

    OCIO::ConstMatrixOpDataRcPtr m = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(mat);
    OCIO::ConstOpCPURcPtr op = OCIO::GetMatrixRenderer(m);
    OCIO_CHECK_ASSERT((bool)op);

    const OCIO::MatrixWithOffsetRenderer * matOffOp
        = dynamic_cast<const OCIO::MatrixWithOffsetRenderer*>(op.get());
    OCIO_CHECK_ASSERT(matOffOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, rgba, 1);

    OCIO_CHECK_EQUAL(rgba[0], 9.5f);
    OCIO_CHECK_EQUAL(rgba[1], 8.f);
    OCIO_CHECK_EQUAL(rgba[2], 7.f);
    OCIO_CHECK_EQUAL(rgba[3], 6.f);
}

OCIO_ADD_TEST(MatrixOpCPU, matrix_renderer)
{
    OCIO::MatrixOpDataRcPtr mat(OCIO::MatrixOpData::CreateDiagonalMatrix(2.0));

    // Make not diagonal.
    mat->setArrayValue(3, 0.5f);

    OCIO::ConstMatrixOpDataRcPtr m = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(mat);
    OCIO::ConstOpCPURcPtr op = OCIO::GetMatrixRenderer(m);
    OCIO_CHECK_ASSERT((bool)op);

    const OCIO::MatrixRenderer* matOp = dynamic_cast<const OCIO::MatrixRenderer*>(op.get());
    OCIO_CHECK_ASSERT(matOp);

    float rgba[4] = { 4.f, 3.f, 2.f, 1.f };

    op->apply(rgba, rgba, 1);

    OCIO_CHECK_EQUAL(rgba[0], 8.5f);
    OCIO_CHECK_EQUAL(rgba[1], 6.f);
    OCIO_CHECK_EQUAL(rgba[2], 4.f);
    OCIO_CHECK_EQUAL(rgba[3], 2.f);
}

