// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "OpenEXR/half.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/lut1d/Lut1DOpData.h"
#include "ops/lut3d/Lut3DOpData.h"
#include "ops/range/RangeOpCPU.h"
#include "ops/range/RangeOpGPU.h"
#include "ops/range/RangeOp.h"
#include "transforms/RangeTransform.h"

namespace OCIO_NAMESPACE
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

    RangeOp(RangeOpDataRcPtr & range);

    virtual ~RangeOp();

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    void finalize() override;
    std::string getCacheID() const override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstRangeOpDataRcPtr rangeData() const { return DynamicPtrCast<const RangeOpData>(data()); }
    RangeOpDataRcPtr rangeData() { return DynamicPtrCast<RangeOpData>(data()); }

};


RangeOp::RangeOp(RangeOpDataRcPtr & range)
    :   Op()
{
    range->validate();
    data() = range;
}

OpRcPtr RangeOp::clone() const
{
    RangeOpDataRcPtr clonedData = rangeData()->clone();
    return std::make_shared<RangeOp>(clonedData);
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
    // It is simpler to handle a pair of inverses by combining them and then removing
    // the identity.  So we just return false here.
    // NB: A clamp cannot be undone, so the exact same Range with opposite direction
    // flags cannot simply be removed like some other ops.
    return false;
}

// Ops must have been validated and finalized.
bool RangeOp::canCombineWith(ConstOpRcPtr & op2) const
{
    auto opData2 = op2->data();
    auto type2 = opData2->getType();
    auto range1 = rangeData();

    // Need to validate prior to calling isIdentity to make sure scale and offset are updated.
    range1->validate();
    if (range1->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        throw Exception("Op::finalize has to be called.");
    }

    if (range1->isIdentity())
    {
        // If op is LUT range op can be removed.
        if (type2 == OpData::Lut1DType)
        {
            auto lut = OCIO_DYNAMIC_POINTER_CAST<const Lut1DOpData>(opData2);
            // Keep range for half domain LUT.
            if (lut && !lut->isInputHalfDomain() && lut->getDirection() == TRANSFORM_DIR_FORWARD)
            {
                return true;
            }
        }
        else if (type2 == OpData::Lut3DType)
        {
            auto lut = OCIO_DYNAMIC_POINTER_CAST<const Lut3DOpData>(opData2);
            if (lut && lut->getDirection() == TRANSFORM_DIR_FORWARD)
            {
                return true;
            }
        }
    }

    if (type2 == OpData::RangeType)
    {
        auto range2 = DynamicPtrCast<const RangeOpData>(opData2);

        if (range2->getDirection() == TRANSFORM_DIR_INVERSE)
        {
            throw Exception("Op::finalize has to be called.");
        }

        return true;
    }
    return false;
}

void RangeOp::combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const
{
    if(!canCombineWith(secondOp))
    {
        throw Exception("RangeOp: canCombineWith must be checked before calling combineWith.");
    }

    auto opData = secondOp->data();
    auto type = opData->getType();
    if (type == OpData::Lut1DType || type == OpData::Lut3DType)
    {
        // Avoid clone (we actually want to use the second op).
        auto lut = std::const_pointer_cast<Op>(secondOp);
        ops.push_back(lut);
    }
    else
    {
        // Range + Range.
        auto range1 = rangeData();
        auto typedRcPtr = DynamicPtrCast<const RangeOp>(secondOp);
        auto range2 = typedRcPtr->rangeData();
        auto resRange = range1->compose(range2);
        CreateRangeOp(ops, resRange, TRANSFORM_DIR_FORWARD);
    }
}

void RangeOp::finalize()
{
    ConstRangeOpDataRcPtr range = rangeData();
    if (range->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        data() = range->getAsForward();
    }
}

std::string RangeOp::getCacheID() const
{
    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<RangeOp ";
    cacheIDStream << rangeData()->getCacheID() << " ";
    cacheIDStream << ">";

    return cacheIDStream.str();
}

ConstOpCPURcPtr RangeOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    ConstRangeOpDataRcPtr data = rangeData();
    return GetRangeRenderer(data);
}

void RangeOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstRangeOpDataRcPtr data = rangeData();
    if (data->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        throw Exception("Op::finalize has to be called.");
    }
    GetRangeGPUShaderProgram(shaderCreator, data);
}

}  // Anon namespace




///////////////////////////////////////////////////////////////////////////



void CreateRangeOp(OpRcPtrVec & ops,
                   double minInValue, double maxInValue,
                   double minOutValue, double maxOutValue,
                   TransformDirection direction)
{
    auto data = std::make_shared<RangeOpData>(minInValue, maxInValue, minOutValue, maxOutValue);

    CreateRangeOp(ops, data, direction);
}

void CreateRangeOp(OpRcPtrVec & ops, RangeOpDataRcPtr & rangeData, TransformDirection direction)
{
    auto range = rangeData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        range = rangeData->clone();
        auto newDir = CombineTransformDirections(range->getDirection(), direction);
        range->setDirection(newDir);
    }

    ops.push_back(std::make_shared<RangeOp>(range));
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
    RangeOpData & data = dynamic_cast<RangeTransformImpl*>(rangeTransform.get())->data();

    ConstRangeOpDataRcPtr rangeDataSrc = DynamicPtrCast<const RangeOpData>(op->data());

    data = *rangeDataSrc;

    group->appendTransform(rangeTransform);
}

void BuildRangeOp(OpRcPtrVec & ops,
                  const RangeTransform & transform,
                  TransformDirection dir)
{
    const auto & data = dynamic_cast<const RangeTransformImpl*>(&transform)->data();

    data.validate();

    if (transform.getStyle() == RANGE_CLAMP)
    {
        auto d = data.clone();
        CreateRangeOp(ops, d, dir);
    }
    else
    {
        MatrixOpDataRcPtr m = data.convertToMatrix();
        CreateMatrixOp(ops, m, dir);
    }
}

} // namespace OCIO_NAMESPACE

