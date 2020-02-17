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

    RangeOp(RangeOpDataRcPtr & range, TransformDirection direction);

    virtual ~RangeOp();

    TransformDirection getDirection() const noexcept { return m_direction; }

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    void finalize(OptimizationFlags oFlags) override;

    ConstOpCPURcPtr getCPUOp() const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

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
        throw Exception("Cannot create RangeOp with unspecified transform direction.");
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
    // It is simpler to handle a pair of inverses by combining them and then removing
    // the identity.  So we just return false here.
    // NB: A clamp cannot be undone, so the exact same Range with opposite direction
    // flags cannot simply be removed like some other ops.
    return false;
}

bool RangeOp::canCombineWith(ConstOpRcPtr & op2) const
{
    auto opData2 = op2->data();
    auto type2 = opData2->getType();
    auto range1 = rangeData();

    // Need to validate prior to calling isIdentity.
    if (m_direction == TRANSFORM_DIR_INVERSE)
    {
        // Inverse is validating.
        range1 = range1->inverse();
    }
    else
    {
        // Make sure scale and offset are updated.
        range1->validate();
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
        if (m_direction == TRANSFORM_DIR_INVERSE)
        {
            range1 = range1->inverse();
        }
        auto typedRcPtr = DynamicPtrCast<const RangeOp>(secondOp);
        auto range2 = typedRcPtr->rangeData();
        if (typedRcPtr->getDirection() == TRANSFORM_DIR_INVERSE)
        {
            range2 = range2->inverse();
        }
        auto resRange = range1->compose(range2);
        CreateRangeOp(ops, resRange, TRANSFORM_DIR_FORWARD);
    }
}

void RangeOp::finalize(OptimizationFlags /*oFlags*/)
{
    const RangeOp & constThis = *this;
    if (m_direction == TRANSFORM_DIR_INVERSE)
    {
        data() = constThis.rangeData()->inverse();
        m_direction = TRANSFORM_DIR_FORWARD;
    }

    rangeData()->finalize();

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<RangeOp ";
    cacheIDStream << constThis.rangeData()->getCacheID() << " ";
    cacheIDStream << TransformDirectionToString(m_direction);
    cacheIDStream << ">";

    m_cacheID = cacheIDStream.str();
}

ConstOpCPURcPtr RangeOp::getCPUOp() const
{
    ConstRangeOpDataRcPtr data = rangeData();
    return GetRangeRenderer(data);
}

void RangeOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    if (m_direction != TRANSFORM_DIR_FORWARD)
    {
        throw Exception("RangeOp direction should have been set to forward by finalize");
    }

    ConstRangeOpDataRcPtr data = rangeData();
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
    RangeOpData & data = dynamic_cast<RangeTransformImpl*>(rangeTransform.get())->data();

    ConstRangeOpDataRcPtr rangeDataSrc = DynamicPtrCast<const RangeOpData>(op->data());

    data = *rangeDataSrc;
    rangeTransform->setDirection(range->getDirection());

    group->appendTransform(rangeTransform);
}

void BuildRangeOp(OpRcPtrVec & ops,
                  const Config & /*config*/,
                  const RangeTransform & transform,
                  TransformDirection dir)
{
    const auto combinedDir = CombineTransformDirections(dir, transform.getDirection());

    const auto & data = dynamic_cast<const RangeTransformImpl*>(&transform)->data();

    data.validate();

    if (transform.getStyle() == RANGE_CLAMP)
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

} // namespace OCIO_NAMESPACE

