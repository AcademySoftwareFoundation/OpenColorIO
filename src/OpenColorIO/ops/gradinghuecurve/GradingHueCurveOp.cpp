// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
//#include "ops/gradingrgbcurve/GradingRGBCurveOpCPU.h"
#include "ops/gradinghuecurve/GradingHueCurveOpGPU.h"
#include "ops/gradinghuecurve/GradingHueCurveOp.h"
#include "transforms/HueCurveTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{

class HueCurveOp;
typedef OCIO_SHARED_PTR<HueCurveOp> HueCurveOpRcPtr;
typedef OCIO_SHARED_PTR<const HueCurveOp> ConstHueCurveOpRcPtr;

class HueCurveOp : public Op
{
public:
    HueCurveOp() = delete;
    HueCurveOp(const HueCurveOp &) = delete;
    explicit HueCurveOp(HueCurveOpDataRcPtr & data);

    virtual ~HueCurveOp();

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
                                DynamicPropertyGradingRGBCurveImplRcPtr & prop) override;
    void removeDynamicProperties() override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstHueCurveOpDataRcPtr hueCurveData() const
    {
        return DynamicPtrCast<const HueCurveOpData>(data());
    }
    HueCurveOpDataRcPtr hueCurveData()
    {
        return DynamicPtrCast<HueCurveOpData>(data());
    }
};


HueCurveOp::HueCurveOp(HueCurveOpDataRcPtr & hueCurveData)
    : Op()
{
    data() = hueCurveData;
}

OpRcPtr HueCurveOp::clone() const
{
   HueCurveOpDataRcPtr p = hueCurveData()->clone();
    return std::make_shared<HueCurveOp>(p);
}

HueCurveOp::~HueCurveOp()
{
}

std::string HueCurveOp::getInfo() const
{
    return "<HueCurveOp>";
}

bool HueCurveOp::isIdentity() const
{
    return hueCurveData()->isIdentity();
}

bool HueCurveOp::isSameType(ConstOpRcPtr & op) const
{
    ConstHueCurveOpRcPtr typedRcPtr = DynamicPtrCast<const HueCurveOp>(op);
    return (bool)typedRcPtr;
}

bool HueCurveOp::isInverse(ConstOpRcPtr & op) const
{
    ConstHueCurveOpRcPtr typedRcPtr = DynamicPtrCast<const HueCurveOp>(op);
    if (!typedRcPtr) return false;

    ConstHueCurveOpDataRcPtr hueOpData = typedRcPtr->hueCurveData();
    return hueCurveData()->isInverse(hueOpData);
}

bool HueCurveOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    return false;
}

void HueCurveOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("HueCurveOp: canCombineWith must be checked "
                        "before calling combineWith.");
    }
}

std::string HueCurveOp::getCacheID() const
{
    // Create the cacheID.
    std::ostringstream cacheIDStream;
    cacheIDStream << "<HueCurveOp ";
    cacheIDStream << hueCurveData()->getCacheID();
    cacheIDStream << ">";

    return cacheIDStream.str();
}

bool HueCurveOp::isDynamic() const
{
    return hueCurveData()->isDynamic();
}

bool HueCurveOp::hasDynamicProperty(DynamicPropertyType type) const
{
    if (type != DYNAMIC_PROPERTY_HUE_CURVE)
    {
        return false;
    }

    return hueCurveData()->isDynamic();
}

DynamicPropertyRcPtr HueCurveOp::getDynamicProperty(DynamicPropertyType type) const
{
    if (type != DYNAMIC_PROPERTY_HUE_CURVE)
    {
        throw Exception("Dynamic property type not supported by hue curve op.");
    }
    if (!isDynamic())
    {
        throw Exception("Hue curve property is not dynamic.");
    }

    return hueCurveData()->getDynamicProperty();
}

void HueCurveOp::replaceDynamicProperty(DynamicPropertyType type,
                                        DynamicPropertyGradingRGBCurveImplRcPtr & prop)
{
    if (type != DYNAMIC_PROPERTY_HUE_CURVE)
    {
        throw Exception("Dynamic property type not supported by hue curve op.");
    }
    if (!isDynamic())
    {
        throw Exception("Hue curve property is not dynamic.");
    }
    auto propGC = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyHueCurveImpl>(prop);
    if (!propGC)
    {
        throw Exception("Dynamic property type not supported by hue curve op.");
    }

    hueCurveData()->replaceDynamicProperty(propGC);
}

void HueCurveOp::removeDynamicProperties()
{
    hueCurveData()->removeDynamicProperty();
}

ConstOpCPURcPtr HueCurveOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    // TODO: Add CPU renderer before merging back the adsk fork    
    //ConstGradingRGBCurveOpDataRcPtr data = rgbCurveData();
    //return GetGradingRGBCurveCPURenderer(data);
    return ConstOpCPURcPtr();
}

void HueCurveOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstHueCurveOpDataRcPtr data = hueCurveData();
    GetHueCurveGPUShaderProgram(shaderCreator, data);
}


}  // Anon namespace




///////////////////////////////////////////////////////////////////////////


void CreateHueCurveOp(OpRcPtrVec & ops,
                             HueCurveOpDataRcPtr & curveData,
                             TransformDirection direction)
{
    auto curve = curveData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        curve = curve->inverse();
    }

    ops.push_back(std::make_shared<HueCurveOp>(curve));
}

///////////////////////////////////////////////////////////////////////////

void CreateHueCurveTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto gc = DynamicPtrCast<const HueCurveOp>(op);
    if (!gc)
    {
        throw Exception("CreateHueCurveTransform: op has to be a HueCurveOp.");
    }
    auto gcData = DynamicPtrCast<const HueCurveOpData>(op->data());
    auto gcTransform = HueCurveTransform::Create(gcData->getStyle());
    auto & data = dynamic_cast<HueCurveTransformImpl *>(gcTransform.get())->data();
    data = *gcData;

    group->appendTransform(gcTransform);
}

void BuildHueCurveOp(OpRcPtrVec & ops,
                            const Config & /*config*/,
                            const ConstContextRcPtr & /*context*/,
                            const HueCurveTransform & transform,
                            TransformDirection dir)
{
    const auto & data = dynamic_cast<const HueCurveTransformImpl &>(transform).data();
    data.validate();

    auto curveData = data.clone();
    CreateHueCurveOp(ops, curveData, dir);
}

} // namespace OCIO_NAMESPACE

