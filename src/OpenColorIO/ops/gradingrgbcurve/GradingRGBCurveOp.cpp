// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOpCPU.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOpGPU.h"
#include "ops/gradingrgbcurve/GradingRGBCurveOp.h"
#include "transforms/GradingRGBCurveTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{

class GradingRGBCurveOp;
typedef OCIO_SHARED_PTR<GradingRGBCurveOp> GradingRGBCurveOpRcPtr;
typedef OCIO_SHARED_PTR<const GradingRGBCurveOp> ConstGradingRGBCurveOpRcPtr;

class GradingRGBCurveOp : public Op
{
public:
    GradingRGBCurveOp() = delete;
    GradingRGBCurveOp(const GradingRGBCurveOp &) = delete;
    explicit GradingRGBCurveOp(GradingRGBCurveOpDataRcPtr & prim);

    virtual ~GradingRGBCurveOp();

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
    ConstGradingRGBCurveOpDataRcPtr rgbCurveData() const
    {
        return DynamicPtrCast<const GradingRGBCurveOpData>(data());
    }
    GradingRGBCurveOpDataRcPtr rgbCurveData()
    {
        return DynamicPtrCast<GradingRGBCurveOpData>(data());
    }
};


GradingRGBCurveOp::GradingRGBCurveOp(GradingRGBCurveOpDataRcPtr & prim)
    :   Op()
{
    data() = prim;
}

OpRcPtr GradingRGBCurveOp::clone() const
{
    GradingRGBCurveOpDataRcPtr p = rgbCurveData()->clone();
    return std::make_shared<GradingRGBCurveOp>(p);
}

GradingRGBCurveOp::~GradingRGBCurveOp()
{
}

std::string GradingRGBCurveOp::getInfo() const
{
    return "<GradingRGBCurveOp>";
}

bool GradingRGBCurveOp::isIdentity() const
{
    return rgbCurveData()->isIdentity();
}

bool GradingRGBCurveOp::isSameType(ConstOpRcPtr & op) const
{
    ConstGradingRGBCurveOpRcPtr typedRcPtr = DynamicPtrCast<const GradingRGBCurveOp>(op);
    return (bool)typedRcPtr;
}

bool GradingRGBCurveOp::isInverse(ConstOpRcPtr & op) const
{
    ConstGradingRGBCurveOpRcPtr typedRcPtr = DynamicPtrCast<const GradingRGBCurveOp>(op);
    if (!typedRcPtr) return false;

    ConstGradingRGBCurveOpDataRcPtr primOpData = typedRcPtr->rgbCurveData();
    return rgbCurveData()->isInverse(primOpData);
}

bool GradingRGBCurveOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    return false;
}

void GradingRGBCurveOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if (!canCombineWith(secondOp))
    {
        throw Exception("GradingRGBCurveOp: canCombineWith must be checked "
                        "before calling combineWith.");
    }
}

std::string GradingRGBCurveOp::getCacheID() const
{
    // Create the cacheID.
    std::ostringstream cacheIDStream;
    cacheIDStream << "<GradingRGBCurveOp ";
    cacheIDStream << rgbCurveData()->getCacheID();
    cacheIDStream << ">";

    return cacheIDStream.str();
}

bool GradingRGBCurveOp::isDynamic() const
{
    return rgbCurveData()->isDynamic();
}

bool GradingRGBCurveOp::hasDynamicProperty(DynamicPropertyType type) const
{
    if (type != DYNAMIC_PROPERTY_GRADING_RGBCURVE)
    {
        return false;
    }
    return rgbCurveData()->isDynamic();
}

DynamicPropertyRcPtr GradingRGBCurveOp::getDynamicProperty(DynamicPropertyType type) const
{
    if (type != DYNAMIC_PROPERTY_GRADING_RGBCURVE)
    {
        throw Exception("Dynamic property type not supported by grading rgb curve op.");
    }
    if (!isDynamic())
    {
        throw Exception("Grading rgb curve property is not dynamic.");
    }
    return rgbCurveData()->getDynamicProperty();
}

void GradingRGBCurveOp::replaceDynamicProperty(DynamicPropertyType type,
                                               DynamicPropertyGradingRGBCurveImplRcPtr & prop)
{
    if (type != DYNAMIC_PROPERTY_GRADING_RGBCURVE)
    {
        throw Exception("Dynamic property type not supported by grading rgb curve op.");
    }
    if (!isDynamic())
    {
        throw Exception("Grading rgb curve property is not dynamic.");
    }
    auto propGC = OCIO_DYNAMIC_POINTER_CAST<DynamicPropertyGradingRGBCurveImpl>(prop);
    if (!propGC)
    {
        throw Exception("Dynamic property type not supported by grading rgb curve op.");
    }

    rgbCurveData()->replaceDynamicProperty(propGC);
}

void GradingRGBCurveOp::removeDynamicProperties()
{
    rgbCurveData()->removeDynamicProperty();
}

ConstOpCPURcPtr GradingRGBCurveOp::getCPUOp(bool /*fastLogExpPow*/) const
{
    ConstGradingRGBCurveOpDataRcPtr data = rgbCurveData();
    return GetGradingRGBCurveCPURenderer(data);
}

void GradingRGBCurveOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstGradingRGBCurveOpDataRcPtr data = rgbCurveData();
    GetGradingRGBCurveGPUShaderProgram(shaderCreator, data);
}


}  // Anon namespace




///////////////////////////////////////////////////////////////////////////


void CreateGradingRGBCurveOp(OpRcPtrVec & ops,
                             GradingRGBCurveOpDataRcPtr & curveData,
                             TransformDirection direction)
{
    auto curve = curveData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        curve = curve->inverse();
    }

    ops.push_back(std::make_shared<GradingRGBCurveOp>(curve));
}

///////////////////////////////////////////////////////////////////////////

void CreateGradingRGBCurveTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto gc = DynamicPtrCast<const GradingRGBCurveOp>(op);
    if (!gc)
    {
        throw Exception("CreateGradingRGBCurveTransform: op has to be a GradingRGBCurveOp.");
    }
    auto gcData = DynamicPtrCast<const GradingRGBCurveOpData>(op->data());
    auto gcTransform = GradingRGBCurveTransform::Create(gcData->getStyle());
    auto & data = dynamic_cast<GradingRGBCurveTransformImpl *>(gcTransform.get())->data();
    data = *gcData;

    group->appendTransform(gcTransform);
}

void BuildGradingRGBCurveOp(OpRcPtrVec & ops,
                            const Config & /*config*/,
                            const ConstContextRcPtr & /*context*/,
                            const GradingRGBCurveTransform & transform,
                            TransformDirection dir)
{
    const auto & data = dynamic_cast<const GradingRGBCurveTransformImpl &>(transform).data();
    data.validate();

    auto curveData = data.clone();
    CreateGradingRGBCurveOp(ops, curveData, dir);
}


} // namespace OCIO_NAMESPACE

