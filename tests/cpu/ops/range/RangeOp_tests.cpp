// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "BitDepthUtils.h"
#include "ops/matrix/MatrixOp.h"

#include "ops/range/RangeOp.cpp"

#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{
const float g_error = 1e-7f;
}

OCIO_ADD_TEST(RangeOp, apply_arbitrary)
{
    OCIO::RangeOpDataRcPtr range = std::make_shared<OCIO::RangeOpData>(-0.101, 0.95, 0.194, 1.001);

    OCIO::RangeOp r(range, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_NO_THROW(r.finalize(OCIO::OPTIMIZATION_NONE));

    float image[4*3] = { -0.50f,  0.25f, 0.50f, 0.0f,
                          0.75f,  1.00f, 1.25f, 1.0f,
                          1.25f,  1.50f, 1.75f, 0.0f };

    OCIO_CHECK_NO_THROW(r.apply(&image[0], 3));

    OCIO_CHECK_CLOSE(image[0],  0.194f,        g_error);
    OCIO_CHECK_CLOSE(image[1],  0.4635119438f, g_error);
    OCIO_CHECK_CLOSE(image[2],  0.6554719806f, g_error);
    OCIO_CHECK_CLOSE(image[3],  0.0f,          g_error);
    OCIO_CHECK_CLOSE(image[4],  0.8474320173f, g_error);
    OCIO_CHECK_CLOSE(image[5],  1.001f,        g_error);
    OCIO_CHECK_CLOSE(image[6],  1.001f,        g_error);
    OCIO_CHECK_CLOSE(image[7],  1.0f,          g_error);
    OCIO_CHECK_CLOSE(image[8],  1.001f,        g_error);
    OCIO_CHECK_CLOSE(image[9],  1.001f,        g_error);
    OCIO_CHECK_CLOSE(image[10], 1.001f,        g_error);
    OCIO_CHECK_CLOSE(image[11], 0.0f,          g_error);
}

OCIO_ADD_TEST(RangeOp, combining)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::OPTIMIZATION_NONE));
    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::OPTIMIZATION_NONE));

    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_NO_THROW(ops[0]->combineWith(ops, op1));
    OCIO_CHECK_EQUAL(ops.size(), 3);

}

OCIO_ADD_TEST(RangeOp, combining_with_inverse)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::OPTIMIZATION_NONE));
    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::OPTIMIZATION_NONE));

    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_NO_THROW(ops[0]->combineWith(ops, op1));
    OCIO_CHECK_EQUAL(ops.size(), 3);
}

OCIO_ADD_TEST(RangeOp, computed_identifier)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.9, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.9, OCIO::TRANSFORM_DIR_INVERSE);
    for(OCIO::OpRcPtrVec::reference op : ops) { op->finalize(OCIO::OPTIMIZATION_NONE); }

    OCIO_REQUIRE_EQUAL(ops.size(), 4);

    OCIO_CHECK_ASSERT(ops[0]->getCacheID() == ops[1]->getCacheID());
    OCIO_CHECK_ASSERT(ops[0]->getCacheID() != ops[2]->getCacheID());
    OCIO_CHECK_ASSERT(ops[1]->getCacheID() != ops[2]->getCacheID());
    OCIO_CHECK_ASSERT(ops[2]->getCacheID() != ops[3]->getCacheID());

    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.90001, OCIO::TRANSFORM_DIR_FORWARD);
    for(OCIO::OpRcPtrVec::reference op : ops) { op->finalize(OCIO::OPTIMIZATION_NONE); }

    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO_CHECK_ASSERT(ops[2]->getCacheID() != ops[4]->getCacheID());
    OCIO_CHECK_ASSERT(ops[3]->getCacheID() != ops[4]->getCacheID());
}

OCIO_ADD_TEST(RangeOp, create_transform)
{
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_INVERSE;

    OCIO::RangeOpDataRcPtr range = std::make_shared<OCIO::RangeOpData>(0.1, 0.9, 0.2, 0.7);

    range->getFormatMetadata().addAttribute("name", "test");

    range->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT10);
    range->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(OCIO::CreateRangeOp(ops, range, direction));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_REQUIRE_ASSERT(ops[0]);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();

    OCIO::ConstOpRcPtr op(ops[0]);

    OCIO::CreateRangeTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto rTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::RangeTransform>(transform);
    OCIO_REQUIRE_ASSERT(rTransform);
    OCIO_CHECK_EQUAL(rTransform->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT10);
    OCIO_CHECK_EQUAL(rTransform->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    const auto & metadata = rTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), "name");
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "test");

    OCIO_CHECK_EQUAL(rTransform->getDirection(), direction);

    OCIO_CHECK_EQUAL(0.1, rTransform->getMinInValue());
    OCIO_CHECK_EQUAL(0.9, rTransform->getMaxInValue());
    OCIO_CHECK_EQUAL(0.2, rTransform->getMinOutValue());
    OCIO_CHECK_EQUAL(0.7, rTransform->getMaxOutValue());
}

OCIO_ADD_TEST(RangeTransform, no_clamp_converts_to_matrix)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    OCIO::OpRcPtrVec ops;

    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    OCIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(range->getStyle(), OCIO::RANGE_CLAMP);
    OCIO_CHECK_ASSERT(!range->hasMinInValue());
    OCIO_CHECK_ASSERT(!range->hasMaxInValue());
    OCIO_CHECK_ASSERT(!range->hasMinOutValue());
    OCIO_CHECK_ASSERT(!range->hasMaxOutValue());

    OCIO_CHECK_NO_THROW(
        OCIO::BuildRangeOp(ops, *config, *range, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO_REQUIRE_EQUAL(op0->data()->getType(), OCIO::OpData::RangeType);
    OCIO_CHECK_ASSERT(op0->isNoOp());
    ops.clear();

    range->setMinInValue(0.0);
    range->setMaxInValue(0.5);
    range->setMinOutValue(0.5);
    range->setMaxOutValue(1.5);

    // Test the resulting Range Op

    OCIO_CHECK_NO_THROW(
        OCIO::BuildRangeOp(ops, *config, *range, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    op0 = ops[0];
    OCIO_REQUIRE_EQUAL(op0->data()->getType(), OCIO::OpData::RangeType);

    OCIO::ConstRangeOpDataRcPtr rangeData
        = OCIO::DynamicPtrCast<const OCIO::RangeOpData>(op0->data());

    OCIO_CHECK_EQUAL(rangeData->getMinInValue(), range->getMinInValue());
    OCIO_CHECK_EQUAL(rangeData->getMaxInValue(), range->getMaxInValue());
    OCIO_CHECK_EQUAL(rangeData->getMinOutValue(), range->getMinOutValue());
    OCIO_CHECK_EQUAL(rangeData->getMaxOutValue(), range->getMaxOutValue());

    // Test the resulting Matrix Op

    range->setStyle(OCIO::RANGE_NO_CLAMP);

    OCIO_CHECK_NO_THROW(
        OCIO::BuildRangeOp(ops, *config, *range, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO_REQUIRE_EQUAL(op1->data()->getType(), OCIO::OpData::MatrixType);

    OCIO::ConstMatrixOpDataRcPtr matrixData
        = OCIO::DynamicPtrCast<const OCIO::MatrixOpData>(op1->data());

    OCIO_CHECK_EQUAL(matrixData->getOffsetValue(0), rangeData->getOffset());

    OCIO_CHECK_EQUAL(matrixData->getOffsetValue(0), 0.5);
    OCIO_CHECK_EQUAL(matrixData->getOffsetValue(1), 0.5);
    OCIO_CHECK_EQUAL(matrixData->getOffsetValue(2), 0.5);
    OCIO_CHECK_EQUAL(matrixData->getOffsetValue(3), 0.0);

    OCIO_CHECK_ASSERT(matrixData->isDiagonal());

    OCIO_CHECK_EQUAL(matrixData->getArray()[0], rangeData->getScale());

    OCIO_CHECK_EQUAL(matrixData->getArray()[0], 2.0);
    OCIO_CHECK_EQUAL(matrixData->getArray()[5], 2.0);
    OCIO_CHECK_EQUAL(matrixData->getArray()[10], 2.0);
    OCIO_CHECK_EQUAL(matrixData->getArray()[15], 1.0);
}

