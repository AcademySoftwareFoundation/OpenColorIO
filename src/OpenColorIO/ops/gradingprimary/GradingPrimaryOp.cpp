// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/gradingprimary/GradingPrimaryOpCPU.h"
#include "ops/gradingprimary/GradingPrimaryOpGPU.h"
#include "ops/gradingprimary/GradingPrimaryOp.h"
#include "transforms/GradingPrimaryTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{

class GradingPrimaryOp;
typedef OCIO_SHARED_PTR<GradingPrimaryOp> GradingPrimaryOpRcPtr;
typedef OCIO_SHARED_PTR<const GradingPrimaryOp> ConstGradingPrimaryOpRcPtr;

class GradingPrimaryOp : public Op
{
public:
    GradingPrimaryOp() = delete;
    GradingPrimaryOp(const GradingPrimaryOp &) = delete;
    explicit GradingPrimaryOp(GradingPrimaryOpDataRcPtr & prim);

    virtual ~GradingPrimaryOp();

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
                                DynamicPropertyGradingPrimaryImplRcPtr & prop) override;
    void removeDynamicProperties() override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstGradingPrimaryOpDataRcPtr primaryData() const
    {
        return DynamicPtrCast<const GradingPrimaryOpData>(data());
    }
    GradingPrimaryOpDataRcPtr primaryData()
    {
        return DynamicPtrCast<GradingPrimaryOpData>(data());
    }
};


GradingPrimaryOp::GradingPrimaryOp(GradingPrimaryOpDataRcPtr & prim)
    :   Op()
{
    data() = prim;
}

OpRcPtr GradingPrimaryOp::clone() const
{
    GradingPrimaryOpDataRcPtr p = primaryData()->clone();
    return std::make_shared<GradingPrimaryOp>(p);
}

GradingPrimaryOp::~GradingPrimaryOp()
{
}

std::string GradingPrimaryOp::getInfo() const
{
    return "<GradingPrimaryOp>";
}

bool GradingPrimaryOp::isIdentity() const
{
    return primaryData()->isIdentity();
}

bool GradingPrimaryOp::isSameType(ConstOpRcPtr & op) const
{
    ConstGradingPrimaryOpRcPtr typedRcPtr = DynamicPtrCast<const GradingPrimaryOp>(op);
    return (bool)typedRcPtr;
}

bool GradingPrimaryOp::isInverse(ConstOpRcPtr & op) const
{
    ConstGradingPrimaryOpRcPtr typedRcPtr = DynamicPtrCast<const GradingPrimaryOp>(op);
    if (!typedRcPtr) return false;

    ConstGradingPrimaryOpDataRcPtr primOpData = typedRcPtr->primaryData();
    return primaryData()->isInverse(primOpData);
}

bool GradingPrimaryOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    // TODO: In some cases this could be combined with itself or other ops.
    return false;
}

void GradingPrimaryOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("GradingPrimaryOp: canCombineWith must be checked "
                        "before calling combineWith.");
    }
}

std::string GradingPrimaryOp::getCacheID() const
{
    // Create the cacheID.
    std::ostringstream cacheIDStream;
    cacheIDStream << "<GradingPrimaryOp ";
    cacheIDStream << primaryData()->getCacheID();
    cacheIDStream << ">";

    return cacheIDStream.str();
}

bool GradingPrimaryOp::isDynamic() const
{
    return primaryData()->isDynamic();
}

bool GradingPrimaryOp::hasDynamicProperty(DynamicPropertyType type) const
{
    if (type != DYNAMIC_PROPERTY_GRADING_PRIMARY)
    {
        return false;
    }
    return primaryData()->isDynamic();
}

DynamicPropertyRcPtr GradingPrimaryOp::getDynamicProperty(DynamicPropertyType type) const
{
    if (type != DYNAMIC_PROPERTY_GRADING_PRIMARY)
    {
        throw Exception("Dynamic property type not supported by grading primary op.");
    }
    if (!isDynamic())
    {
        throw Exception("Grading primary property is not dynamic.");
    }
    return primaryData()->getDynamicProperty();
}

void GradingPrimaryOp::replaceDynamicProperty(DynamicPropertyType type,
                                              DynamicPropertyGradingPrimaryImplRcPtr & prop)
{
    if (type != DYNAMIC_PROPERTY_GRADING_PRIMARY)
    {
        throw Exception("Dynamic property type not supported by grading primary op.");
    }
    if (!isDynamic())
    {
        throw Exception("Grading primary property is not dynamic.");
    }
    auto propGP = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyGradingPrimaryImpl>(prop);
    if (!propGP)
    {
        throw Exception("Dynamic property type not supported by grading primary op.");
    }
    primaryData()->replaceDynamicProperty(propGP);
}

void GradingPrimaryOp::removeDynamicProperties()
{
    primaryData()->removeDynamicProperty();
}

ConstOpCPURcPtr GradingPrimaryOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    ConstGradingPrimaryOpDataRcPtr data = primaryData();
    return GetGradingPrimaryCPURenderer(data);
}

void GradingPrimaryOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstGradingPrimaryOpDataRcPtr data = primaryData();
    GetGradingPrimaryGPUShaderProgram(shaderCreator, data);
}


}  // Anon namespace




///////////////////////////////////////////////////////////////////////////


void CreateGradingPrimaryOp(OpRcPtrVec & ops,
                            GradingPrimaryOpDataRcPtr & primData,
                            TransformDirection direction)
{
    auto prim = primData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        prim = prim->inverse();
    }

    ops.push_back(std::make_shared<GradingPrimaryOp>(prim));
}

///////////////////////////////////////////////////////////////////////////

void CreateGradingPrimaryTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto prim = DynamicPtrCast<const GradingPrimaryOp>(op);
    if (!prim)
    {
        throw Exception("CreateGradingPrimaryTransform: op has to be a GradingPrimaryOp.");
    }
    auto primData = DynamicPtrCast<const GradingPrimaryOpData>(op->data());
    auto primTransform = GradingPrimaryTransform::Create(primData->getStyle());
    auto & data = dynamic_cast<GradingPrimaryTransformImpl *>(primTransform.get())->data();
    data = *primData;

    group->appendTransform(primTransform);
}

void BuildGradingPrimaryOp(OpRcPtrVec & ops,
                           const Config & /*config*/,
                           const ConstContextRcPtr & /*context*/,
                           const GradingPrimaryTransform & transform,
                           TransformDirection dir)
{
    const auto & data = dynamic_cast<const GradingPrimaryTransformImpl &>(transform).data();
    data.validate();

    auto primData = data.clone();
    CreateGradingPrimaryOp(ops, primData, dir);
}


} // namespace OCIO_NAMESPACE

