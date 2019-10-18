// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "OpenEXR/half.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/Range/RangeOpCPU.h"
#include "ops/Range/RangeOpGPU.h"
#include "ops/Range/RangeOps.h"
#include "transforms/RangeTransform.h"


OCIO_NAMESPACE_ENTER
{

namespace
{

class RangeOp;
typedef OCIO_SHARED_PTR<RangeOp> RangeOpRcPtr;
typedef OCIO_SHARED_PTR<const RangeOp> ConstRangeOpRcPtr;

class RangeOp : public Op
{
public:
    RangeOp() = delete;

    RangeOp(RangeOpDataRcPtr & range, TransformDirection direction);

    virtual ~RangeOp();

    TransformDirection getDirection() const noexcept override { return m_direction; }

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    void finalize(FinalizationFlags fFlags) override;

    ConstOpCPURcPtr getCPUOp() const override;

    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;

protected:
    ConstRangeOpDataRcPtr rangeData() const { return DynamicPtrCast<const RangeOpData>(data()); }
    RangeOpDataRcPtr rangeData() { return DynamicPtrCast<RangeOpData>(data()); }

private:
    // The range direction
    TransformDirection m_direction;
};


RangeOp::RangeOp(RangeOpDataRcPtr & range, TransformDirection direction)
    :   Op()
    ,   m_direction(direction)
{
    if(m_direction == TRANSFORM_DIR_UNKNOWN)
    {
        throw Exception(
            "Cannot create RangeOp with unspecified transform direction.");
    }

    range->validate();
    data() = range;
}

OpRcPtr RangeOp::clone() const
{
    RangeOpDataRcPtr clonedData = rangeData()->clone();
    return std::make_shared<RangeOp>(clonedData, m_direction);
}

RangeOp::~RangeOp()
{
}

std::string RangeOp::getInfo() const
{
    return "<RangeOp>";
}

bool RangeOp::isSameType(ConstOpRcPtr & op) const
{
    ConstRangeOpRcPtr typedRcPtr = DynamicPtrCast<const RangeOp>(op);
    return (bool)typedRcPtr;
}

bool RangeOp::isInverse(ConstOpRcPtr & op) const
{
    ConstRangeOpRcPtr typedRcPtr = DynamicPtrCast<const RangeOp>(op);
    if(!typedRcPtr) return false;

    if(GetInverseTransformDirection(m_direction)==typedRcPtr->m_direction)
    {
        return *rangeData()==*(typedRcPtr->rangeData());
    }

    ConstRangeOpDataRcPtr rangeOpData = typedRcPtr->rangeData();
    return rangeData()->isInverse(rangeOpData);
}

bool RangeOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    // TODO: Implement RangeOp::canCombineWith()
    return false;
}

void RangeOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if(!canCombineWith(secondOp))
    {
        throw Exception("TODO: Range can't be combined.");
    }

    // TODO: Implement RangeOp::combineWith()
}

void RangeOp::finalize(FinalizationFlags /*fFlags*/)
{
    const RangeOp & constThis = *this;
    if(m_direction == TRANSFORM_DIR_INVERSE)
    {
        data() = constThis.rangeData()->inverse();
        m_direction = TRANSFORM_DIR_FORWARD;
    }

    rangeData()->finalize();

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<RangeOp ";
    cacheIDStream << constThis.rangeData()->getCacheID() << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";
    cacheIDStream << ">";

    m_cacheID = cacheIDStream.str();
}

ConstOpCPURcPtr RangeOp::getCPUOp() const
{
    ConstRangeOpDataRcPtr data = rangeData();
    return GetRangeRenderer(data);
}

void RangeOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
{
    if (m_direction != TRANSFORM_DIR_FORWARD)
    {
        throw Exception(
            "RangeOp direction should have been set to forward by finalize");
    }

    ConstRangeOpDataRcPtr data = rangeData();
    GetRangeGPUShaderProgram(shaderDesc, data);
}

}  // Anon namespace




///////////////////////////////////////////////////////////////////////////



void CreateRangeOp(OpRcPtrVec & ops,
                   double minInValue, double maxInValue,
                   double minOutValue, double maxOutValue,
                   TransformDirection direction)
{
    RangeOpDataRcPtr rangeData =
        std::make_shared<RangeOpData>(minInValue, maxInValue, minOutValue, maxOutValue);

    CreateRangeOp(ops, rangeData, direction);
}

void CreateRangeOp(OpRcPtrVec & ops, RangeOpDataRcPtr & rangeData, TransformDirection direction)
{
    ops.push_back(std::make_shared<RangeOp>(rangeData, direction));
}

///////////////////////////////////////////////////////////////////////////

void CreateRangeTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto range = DynamicPtrCast<const RangeOp>(op);
    if (!range)
    {
        throw Exception("CreateRangeTransform: op has to be a RangeOp");
    }

    RangeTransformRcPtr rangeTransform = RangeTransform::Create();

    RangeOpData & data
        = dynamic_cast<RangeTransformImpl*>(rangeTransform.get())->data();

    ConstRangeOpDataRcPtr rangeDataSrc = DynamicPtrCast<const RangeOpData>(op->data());    

    data = *rangeDataSrc;
    data.getFormatMetadata() = rangeDataSrc->getFormatMetadata();

    rangeTransform->setDirection(range->getDirection());
    rangeTransform->setFileInputBitDepth(rangeDataSrc->getFileInputBitDepth());
    rangeTransform->setFileOutputBitDepth(rangeDataSrc->getFileOutputBitDepth());

    group->push_back(rangeTransform);
}

void BuildRangeOps(OpRcPtrVec & ops,
                   const Config & /*config*/,
                   const RangeTransform & transform,
                   TransformDirection dir)
{
    const TransformDirection combinedDir
        = CombineTransformDirections(dir, transform.getDirection());

    const RangeOpData & data
        = dynamic_cast<const RangeTransformImpl*>(&transform)->data();

    data.validate();

    if(transform.getStyle()==RANGE_CLAMP)
    {
        auto d = data.clone();
        CreateRangeOp(ops, d, combinedDir);
    }
    else
    {
        MatrixOpDataRcPtr m = data.convertToMatrix();
        CreateMatrixOp(ops, m, combinedDir);
    }
}

}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

#include "BitDepthUtils.h"
#include "ops/Matrix/MatrixOps.h"
#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


const float g_error = 1e-7f;

OCIO_ADD_TEST(RangeOps, apply_arbitrary)
{
    OCIO::RangeOpDataRcPtr range = std::make_shared<OCIO::RangeOpData>(-0.101, 0.95, 0.194, 1.001);

    OCIO::RangeOp r(range, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_NO_THROW(r.finalize(OCIO::FINALIZATION_EXACT));

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

OCIO_ADD_TEST(RangeOps, combining)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

    OCIO::ConstOpRcPtr op1 = ops[1];

    // TODO: implement Range combine
    OCIO_CHECK_THROW_WHAT(ops[0]->combineWith(ops, op1),
                          OCIO::Exception, "TODO: Range can't be combined");
    OCIO_CHECK_EQUAL(ops.size(), 2);

}

OCIO_ADD_TEST(RangeOps, combining_with_inverse)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_NO_THROW(ops[0]->finalize(OCIO::FINALIZATION_EXACT));
    OCIO::CreateRangeOp(ops, 0., 1., 0.5, 1.5, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    OCIO_CHECK_NO_THROW(ops[1]->finalize(OCIO::FINALIZATION_EXACT));

    OCIO::ConstOpRcPtr op1 = ops[1];

    // TODO: implement Range combine
    OCIO_CHECK_THROW_WHAT(ops[0]->combineWith(ops, op1),
                          OCIO::Exception, "TODO: Range can't be combined");
    OCIO_CHECK_EQUAL(ops.size(), 2);

}

OCIO_ADD_TEST(RangeOps, is_inverse)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1., OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(ops.size(), 1);
    // Skip finalize so that inverse direction is kept
    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1., OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(ops.size(), 2);

    const double offset[] = { 1.1, -1.3, 0.3, 0.0 };
    OCIO_CHECK_NO_THROW(CreateOffsetOp(ops, offset, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];
    OCIO::ConstOpRcPtr op2 = ops[2];

    OCIO_CHECK_ASSERT(ops[0]->isSameType(op1));
    OCIO_CHECK_ASSERT(!ops[0]->isSameType(op2));

    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op2));
    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op0));

    // Inverse based on Op direction

    OCIO_CHECK_ASSERT(ops[0]->isInverse(op1));

    OCIO::CreateRangeOp(ops, 0.000002, 0.5, 0.5, 1., OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO::ConstOpRcPtr op3 = ops[3];

    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op3));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op3));

    OCIO::CreateRangeOp(ops, 0.000002, 0.5, 0.5, 1., OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO::ConstOpRcPtr op4 = ops[4];

    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op4));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op4));
    OCIO_CHECK_ASSERT(ops[3]->isInverse(op4));

    OCIO::CreateRangeOp(ops, 0.5, 1., 0.000002, 0.5, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_REQUIRE_EQUAL(ops.size(), 6);
    OCIO::ConstOpRcPtr op5 = ops[5];

    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op5));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op5));
    OCIO_CHECK_ASSERT(ops[3]->isInverse(op5));
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op5));
}

OCIO_ADD_TEST(RangeOps, computed_identifier)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateRangeOp(ops, 0., 0.5, 0.5, 1.0, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.9, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.9, OCIO::TRANSFORM_DIR_INVERSE);
    for(OCIO::OpRcPtrVec::reference op : ops) { op->finalize(OCIO::FINALIZATION_EXACT); }

    OCIO_REQUIRE_EQUAL(ops.size(), 4);

    OCIO_CHECK_ASSERT(ops[0]->getCacheID() == ops[1]->getCacheID());
    OCIO_CHECK_ASSERT(ops[0]->getCacheID() != ops[2]->getCacheID());
    OCIO_CHECK_ASSERT(ops[1]->getCacheID() != ops[2]->getCacheID());
    OCIO_CHECK_ASSERT(ops[2]->getCacheID() != ops[3]->getCacheID());

    OCIO::CreateRangeOp(ops, 0.1, 1., 0.3, 1.90001, OCIO::TRANSFORM_DIR_FORWARD);
    for(OCIO::OpRcPtrVec::reference op : ops) { op->finalize(OCIO::FINALIZATION_EXACT); }

    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO_CHECK_ASSERT(ops[2]->getCacheID() != ops[4]->getCacheID());
    OCIO_CHECK_ASSERT(ops[3]->getCacheID() != ops[4]->getCacheID());
}

OCIO_ADD_TEST(RangeOps, create_transform)
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
    OCIO_REQUIRE_EQUAL(group->size(), 1);
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

    range->setMinInValue(0.0);
    range->setMaxInValue(0.5);
    range->setMinOutValue(0.5);
    range->setMaxOutValue(1.5);

    // Test the resulting Range Op

    OCIO_CHECK_NO_THROW(
        OCIO::BuildRangeOps(ops, *config, *range, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO::ConstOpRcPtr op0 = ops[0];
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
        OCIO::BuildRangeOps(ops, *config, *range, OCIO::TRANSFORM_DIR_FORWARD));

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

#endif
