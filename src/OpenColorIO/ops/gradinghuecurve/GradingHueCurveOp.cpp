// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/gradinghuecurve/GradingHueCurveOpGPU.h"
#include "ops/gradinghuecurve/GradingHueCurveOp.h"
#include "transforms/GradingHueCurveTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{

class GradingHueCurveOp;
typedef OCIO_SHARED_PTR<GradingHueCurveOp> GradingHueCurveOpRcPtr;
typedef OCIO_SHARED_PTR<const GradingHueCurveOp> ConstGradingHueCurveOpRcPtr;

class GradingHueCurveOp : public Op
{
public:
    GradingHueCurveOp() = delete;
    GradingHueCurveOp(const GradingHueCurveOp &) = delete;
    explicit GradingHueCurveOp(GradingHueCurveOpDataRcPtr & data);

    virtual ~GradingHueCurveOp();

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
                                DynamicPropertyGradingHueCurveImplRcPtr & prop) override;
    void removeDynamicProperties() override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstGradingHueCurveOpDataRcPtr hueCurveData() const
    {
        return DynamicPtrCast<const GradingHueCurveOpData>(data());
    }
    GradingHueCurveOpDataRcPtr hueCurveData()
    {
        return DynamicPtrCast<GradingHueCurveOpData>(data());
    }
};


GradingHueCurveOp::GradingHueCurveOp(GradingHueCurveOpDataRcPtr & hueCurveData)
    : Op()
{
    data() = hueCurveData;
}

OpRcPtr GradingHueCurveOp::clone() const
{
   GradingHueCurveOpDataRcPtr p = hueCurveData()->clone();
   return std::make_shared<GradingHueCurveOp>(p);
}

GradingHueCurveOp::~GradingHueCurveOp()
{
}

std::string GradingHueCurveOp::getInfo() const
{
    return "<GradingHueCurveOp>";
}

bool GradingHueCurveOp::isIdentity() const
{
    return hueCurveData()->isIdentity();
}

bool GradingHueCurveOp::isSameType(ConstOpRcPtr & op) const
{
    ConstGradingHueCurveOpRcPtr typedRcPtr = DynamicPtrCast<const GradingHueCurveOp>(op);
    return (bool)typedRcPtr;
}

bool GradingHueCurveOp::isInverse(ConstOpRcPtr & op) const
{
    ConstGradingHueCurveOpRcPtr typedRcPtr = DynamicPtrCast<const GradingHueCurveOp>(op);
    if (!typedRcPtr) return false;

    ConstGradingHueCurveOpDataRcPtr hueOpData = typedRcPtr->hueCurveData();
    return hueCurveData()->isInverse(hueOpData);
}

bool GradingHueCurveOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    return false;
}

void GradingHueCurveOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("GradingHueCurveOp: canCombineWith must be checked "
                        "before calling combineWith.");
    }
}

std::string GradingHueCurveOp::getCacheID() const
{
    // Create the cacheID.
    std::ostringstream cacheIDStream;
    cacheIDStream << "<GradingHueCurveOp ";
    cacheIDStream << hueCurveData()->getCacheID();
    cacheIDStream << ">";

    return cacheIDStream.str();
}

bool GradingHueCurveOp::isDynamic() const
{
    return hueCurveData()->isDynamic();
}

bool GradingHueCurveOp::hasDynamicProperty(DynamicPropertyType type) const
{
    if (type != DYNAMIC_PROPERTY_GRADING_HUECURVE)
    {
        return false;
    }

    return hueCurveData()->isDynamic();
}

DynamicPropertyRcPtr GradingHueCurveOp::getDynamicProperty(DynamicPropertyType type) const
{
    if (type != DYNAMIC_PROPERTY_GRADING_HUECURVE)
    {
        throw Exception("Dynamic property type not supported by hue curve op.");
    }
    if (!isDynamic())
    {
        throw Exception("Hue curve property is not dynamic.");
    }

    return hueCurveData()->getDynamicProperty();
}

void GradingHueCurveOp::replaceDynamicProperty(DynamicPropertyType type,
                                               DynamicPropertyGradingHueCurveImplRcPtr & prop)
{
    if (type != DYNAMIC_PROPERTY_GRADING_HUECURVE)
    {
        throw Exception("Dynamic property type not supported by hue curve op.");
    }
    if (!isDynamic())
    {
        throw Exception("Hue curve property is not dynamic.");
    }
    auto propGC = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyGradingHueCurveImpl>(prop);
    if (!propGC)
    {
        throw Exception("Dynamic property type not supported by hue curve op.");
    }

    hueCurveData()->replaceDynamicProperty(propGC);
}

void GradingHueCurveOp::removeDynamicProperties()
{
    hueCurveData()->removeDynamicProperty();
}

ConstOpCPURcPtr GradingHueCurveOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    // TODO: Add CPU renderer before merging back the adsk fork    
    //ConstGradingRGBCurveOpDataRcPtr data = rgbCurveData();
    //return GetGradingRGBCurveCPURenderer(data);
    return ConstOpCPURcPtr();
}

void GradingHueCurveOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstGradingHueCurveOpDataRcPtr data = hueCurveData();
    GetHueCurveGPUShaderProgram(shaderCreator, data);
}


}  // Anon namespace




///////////////////////////////////////////////////////////////////////////


void CreateGradingHueCurveOp(OpRcPtrVec & ops,
                             GradingHueCurveOpDataRcPtr & curveData,
                             TransformDirection direction)
{
    auto curve = curveData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        curve = curve->inverse();
    }

    ops.push_back(std::make_shared<GradingHueCurveOp>(curve));
}

///////////////////////////////////////////////////////////////////////////

void CreateGradingHueCurveTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto gc = DynamicPtrCast<const GradingHueCurveOp>(op);
    if (!gc)
    {
        throw Exception("CreateGradingHueCurveTransform: op has to be a GradingHueCurveOp.");
    }
    auto gcData = DynamicPtrCast<const GradingHueCurveOpData>(op->data());
    auto gcTransform = GradingHueCurveTransform::Create(gcData->getStyle());
    auto & data = dynamic_cast<GradingHueCurveTransformImpl *>(gcTransform.get())->data();
    data = *gcData;

    group->appendTransform(gcTransform);
}

void BuildGradingHueCurveOp(OpRcPtrVec & ops,
                            const Config & /*config*/,
                            const ConstContextRcPtr & /*context*/,
                            const GradingHueCurveTransform & transform,
                            TransformDirection dir)
{
    const auto & data = dynamic_cast<const GradingHueCurveTransformImpl &>(transform).data();
    data.validate();

    auto curveData = data.clone();
    CreateGradingHueCurveOp(ops, curveData, dir);
}

} // namespace OCIO_NAMESPACE

