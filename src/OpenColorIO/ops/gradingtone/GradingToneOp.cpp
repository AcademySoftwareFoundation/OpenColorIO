// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/gradingtone/GradingToneOpCPU.h"
#include "ops/gradingtone/GradingToneOpGPU.h"
#include "ops/gradingtone/GradingToneOp.h"
#include "transforms/GradingToneTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{

class GradingToneOp;
typedef OCIO_SHARED_PTR<GradingToneOp> GradingToneOpRcPtr;
typedef OCIO_SHARED_PTR<const GradingToneOp> ConstGradingToneOpRcPtr;

class GradingToneOp : public Op
{
public:
    GradingToneOp() = delete;
    GradingToneOp(const GradingToneOp &) = delete;
    explicit GradingToneOp(GradingToneOpDataRcPtr & tone);

    virtual ~GradingToneOp();

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isIdentity() const override;
    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    std::string getCacheID() const override;

    bool isDynamic() const override;
    bool hasDynamicProperty(DynamicPropertyType type) const override;
    DynamicPropertyRcPtr getDynamicProperty(DynamicPropertyType type) const override;
    void replaceDynamicProperty(DynamicPropertyType type,
                                DynamicPropertyGradingToneImplRcPtr & prop) override;
    void removeDynamicProperties() override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstGradingToneOpDataRcPtr toneData() const
    {
        return DynamicPtrCast<const GradingToneOpData>(data());
    }
    GradingToneOpDataRcPtr toneData()
    {
        return DynamicPtrCast<GradingToneOpData>(data());
    }
};


GradingToneOp::GradingToneOp(GradingToneOpDataRcPtr & tone)
    :   Op()
{
    data() = tone;
}

OpRcPtr GradingToneOp::clone() const
{
    GradingToneOpDataRcPtr p = toneData()->clone();
    return std::make_shared<GradingToneOp>(p);
}

GradingToneOp::~GradingToneOp()
{
}

std::string GradingToneOp::getInfo() const
{
    return "<GradingToneOp>";
}

bool GradingToneOp::isIdentity() const
{
    return toneData()->isIdentity();
}

bool GradingToneOp::isSameType(ConstOpRcPtr & op) const
{
    ConstGradingToneOpRcPtr typedRcPtr = DynamicPtrCast<const GradingToneOp>(op);
    return (bool)typedRcPtr;
}

bool GradingToneOp::isInverse(ConstOpRcPtr & op) const
{
    ConstGradingToneOpRcPtr typedRcPtr = DynamicPtrCast<const GradingToneOp>(op);
    if (!typedRcPtr) return false;

    ConstGradingToneOpDataRcPtr toneOpData = typedRcPtr->toneData();
    return toneData()->isInverse(toneOpData);
}

bool GradingToneOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    return false;
}

void GradingToneOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("GradingToneOp: canCombineWith must be checked "
                        "before calling combineWith.");
    }
}

std::string GradingToneOp::getCacheID() const
{
    // Create the cacheID.
    std::ostringstream cacheIDStream;
    cacheIDStream << "<GradingToneOp ";
    cacheIDStream << toneData()->getCacheID();
    cacheIDStream << ">";

    return cacheIDStream.str();
}

bool GradingToneOp::isDynamic() const
{
    return toneData()->isDynamic();
}

bool GradingToneOp::hasDynamicProperty(DynamicPropertyType type) const
{
    if (type != DYNAMIC_PROPERTY_GRADING_TONE)
    {
        return false;
    }
    return toneData()->isDynamic();
}

DynamicPropertyRcPtr GradingToneOp::getDynamicProperty(DynamicPropertyType type) const
{
    if (type != DYNAMIC_PROPERTY_GRADING_TONE)
    {
        throw Exception("Dynamic property type not supported by grading tone op.");
    }
    if (!isDynamic())
    {
        throw Exception("Grading tone property is not dynamic.");
    }
    return toneData()->getDynamicProperty();
}

void GradingToneOp::replaceDynamicProperty(DynamicPropertyType type,
                                           DynamicPropertyGradingToneImplRcPtr & prop)
{
    if (type != DYNAMIC_PROPERTY_GRADING_TONE)
    {
        throw Exception("Dynamic property type not supported by grading tone op.");
    }
    if (!isDynamic())
    {
        throw Exception("Grading tone property is not dynamic.");
    }
    toneData()->replaceDynamicProperty(prop);
}

void GradingToneOp::removeDynamicProperties()
{
    toneData()->removeDynamicProperty();
}

ConstOpCPURcPtr GradingToneOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    ConstGradingToneOpDataRcPtr data = toneData();
    return GetGradingToneCPURenderer(data);
}

void GradingToneOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstGradingToneOpDataRcPtr data = toneData();
    GetGradingToneGPUShaderProgram(shaderCreator, data);
}


}  // Anon namespace




///////////////////////////////////////////////////////////////////////////


void CreateGradingToneOp(OpRcPtrVec & ops,
                         GradingToneOpDataRcPtr & toneData,
                         TransformDirection direction)
{
    auto tone = toneData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        tone = tone->inverse();
    }

    ops.push_back(std::make_shared<GradingToneOp>(tone));
}

///////////////////////////////////////////////////////////////////////////

void CreateGradingToneTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto gt = DynamicPtrCast<const GradingToneOp>(op);
    if (!gt)
    {
        throw Exception("CreateGradingToneTransform: op has to be a GradingToneOp.");
    }
    auto gtData = DynamicPtrCast<const GradingToneOpData>(op->data());
    auto gtTransform = GradingToneTransform::Create(gtData->getStyle());
    auto & data = dynamic_cast<GradingToneTransformImpl *>(gtTransform.get())->data();
    data = *gtData;

    group->appendTransform(gtTransform);
}

void BuildGradingToneOp(OpRcPtrVec & ops,
                        const Config & /*config*/,
                        const ConstContextRcPtr & /*context*/,
                        const GradingToneTransform & transform,
                        TransformDirection dir)
{
    const auto & data = dynamic_cast<const GradingToneTransformImpl &>(transform).data();
    data.validate();

    auto toneData = data.clone();
    CreateGradingToneOp(ops, toneData, dir);
}


} // namespace OCIO_NAMESPACE

