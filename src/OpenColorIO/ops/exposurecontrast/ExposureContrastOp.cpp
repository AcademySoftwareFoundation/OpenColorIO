// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/exposurecontrast/ExposureContrastOpCPU.h"
#include "ops/exposurecontrast/ExposureContrastOpGPU.h"
#include "ops/exposurecontrast/ExposureContrastOp.h"
#include "transforms/ExposureContrastTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{

class ExposureContrastOp;
typedef OCIO_SHARED_PTR<ExposureContrastOp> ExposureContrastOpRcPtr;
typedef OCIO_SHARED_PTR<const ExposureContrastOp> ConstExposureContrastOpRcPtr;

class ExposureContrastOp : public Op
{
public:
    ExposureContrastOp() = delete;
    ExposureContrastOp(const ExposureContrastOp &) = delete;
    explicit ExposureContrastOp(ExposureContrastOpDataRcPtr & ec);

    virtual ~ExposureContrastOp();

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
    void replaceDynamicProperty(DynamicPropertyType type, DynamicPropertyDoubleImplRcPtr & prop) override;
    void removeDynamicProperties() override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstExposureContrastOpDataRcPtr ecData() const
    {
        return DynamicPtrCast<const ExposureContrastOpData>(data());
    }
    ExposureContrastOpDataRcPtr ecData()
    {
        return DynamicPtrCast<ExposureContrastOpData>(data());
    }
};


ExposureContrastOp::ExposureContrastOp(ExposureContrastOpDataRcPtr & ec)
    : Op()
{
    data() = ec;
}

OpRcPtr ExposureContrastOp::clone() const
{
    ExposureContrastOpDataRcPtr f = ecData()->clone();
    return std::make_shared<ExposureContrastOp>(f);
}

ExposureContrastOp::~ExposureContrastOp()
{
}

std::string ExposureContrastOp::getInfo() const
{
    return "<ExposureContrastOp>";
}

bool ExposureContrastOp::isIdentity() const
{
    return ecData()->isIdentity();
}

bool ExposureContrastOp::isSameType(ConstOpRcPtr & op) const
{
    ConstExposureContrastOpRcPtr typedRcPtr = DynamicPtrCast<const ExposureContrastOp>(op);
    return (bool)typedRcPtr;
}

bool ExposureContrastOp::isInverse(ConstOpRcPtr & op) const
{
    ConstExposureContrastOpRcPtr typedRcPtr = DynamicPtrCast<const ExposureContrastOp>(op);
    if (!typedRcPtr) return false;

    ConstExposureContrastOpDataRcPtr ecOpData = typedRcPtr->ecData();
    return ecData()->isInverse(ecOpData);
}

bool ExposureContrastOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    return false;
}

void ExposureContrastOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("ExposureContrastOp: canCombineWith must be checked "
                        "before calling combineWith.");
    }
}

std::string ExposureContrastOp::getCacheID() const
{
    // Create the cacheID.
    std::ostringstream cacheIDStream;
    cacheIDStream << "<ExposureContrastOp ";
    cacheIDStream << ecData()->getCacheID();
    cacheIDStream << ">";

    return cacheIDStream.str();
}

ConstOpCPURcPtr ExposureContrastOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    ConstExposureContrastOpDataRcPtr ecOpData = ecData();
    return GetExposureContrastCPURenderer(ecOpData);
}

void ExposureContrastOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstExposureContrastOpDataRcPtr ecOpData = ecData();
    GetExposureContrastGPUShaderProgram(shaderCreator, ecOpData);
}

bool ExposureContrastOp::isDynamic() const
{
    return ecData()->isDynamic();
}

bool ExposureContrastOp::hasDynamicProperty(DynamicPropertyType type) const
{
    return ecData()->hasDynamicProperty(type);
}

DynamicPropertyRcPtr ExposureContrastOp::getDynamicProperty(DynamicPropertyType type) const
{
    return ecData()->getDynamicProperty(type);
}

void ExposureContrastOp::replaceDynamicProperty(DynamicPropertyType type,
                                                DynamicPropertyDoubleImplRcPtr & prop)
{
    ecData()->replaceDynamicProperty(type, prop);
}

void ExposureContrastOp::removeDynamicProperties()
{
    ecData()->removeDynamicProperties();
}

}  // Anon namespace


///////////////////////////////////////////////////////////////////////////

void CreateExposureContrastOp(OpRcPtrVec & ops,
                              ExposureContrastOpDataRcPtr & data,
                              TransformDirection direction)
{
    switch (direction)
    {
    case TRANSFORM_DIR_FORWARD:
    {
        ops.push_back(std::make_shared<ExposureContrastOp>(data));
        break;
    }
    case TRANSFORM_DIR_INVERSE:
    {
        ExposureContrastOpDataRcPtr dataInv = data->inverse();
        ops.push_back(std::make_shared<ExposureContrastOp>(dataInv));
        break;
    }
    }
}

///////////////////////////////////////////////////////////////////////////

void CreateExposureContrastTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto ec = DynamicPtrCast<const ExposureContrastOp>(op);
    if (!ec)
    {
        throw Exception("CreateExposureContrastTransform: op has to be a ExposureContrastOp");
    }
    auto ecData = DynamicPtrCast<const ExposureContrastOpData>(op->data());
    auto ecTransform = ExposureContrastTransform::Create();
    auto & data = dynamic_cast<ExposureContrastTransformImpl *>(ecTransform.get())->data();
    data = *ecData;

    group->appendTransform(ecTransform);
}

void BuildExposureContrastOp(OpRcPtrVec & ops,
                             const ExposureContrastTransform & transform,
                             TransformDirection dir)
{
    const auto & data = dynamic_cast<const ExposureContrastTransformImpl &>(transform).data();
    data.validate();

    auto ecData = data.clone();
    CreateExposureContrastOp(ops, ecData, dir);
}

} // namespace OCIO_NAMESPACE

