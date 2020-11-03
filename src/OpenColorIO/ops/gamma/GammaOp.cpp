// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "GpuShaderUtils.h"
#include "ops/exponent/ExponentOp.h"
#include "ops/gamma/GammaOp.h"
#include "ops/gamma/GammaOpCPU.h"
#include "ops/gamma/GammaOpGPU.h"
#include "ops/gamma/GammaOpUtils.h"
#include "transforms/ExponentTransform.h"
#include "transforms/ExponentWithLinearTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{

class GammaOp;
typedef OCIO_SHARED_PTR<GammaOp> GammaOpRcPtr;
typedef OCIO_SHARED_PTR<const GammaOp> ConstGammaOpRcPtr;

class GammaOp : public Op
{
public:
    GammaOp() = delete;
    explicit GammaOp(GammaOpDataRcPtr & gamma);
    GammaOp(const GammaOp &) = delete;

    GammaOp(GammaOpData::Style style,
            GammaOpData::Params red,
            GammaOpData::Params green,
            GammaOpData::Params blue,
            GammaOpData::Params alpha);

    virtual ~GammaOp();

    std::string getInfo() const override;

    OpRcPtr clone() const override;

    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    std::string getCacheID() const override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstGammaOpDataRcPtr gammaData() const { return DynamicPtrCast<const GammaOpData>(data()); }
    GammaOpDataRcPtr gammaData() { return DynamicPtrCast<GammaOpData>(data()); }
};


GammaOp::GammaOp(GammaOpDataRcPtr & gamma)
    :   Op()
{
    data() = gamma;
}

GammaOp::GammaOp(GammaOpData::Style style,
                 GammaOpData::Params red,
                 GammaOpData::Params green,
                 GammaOpData::Params blue,
                 GammaOpData::Params alpha)
    :   Op()
{
    GammaOpDataRcPtr gamma = std::make_shared<GammaOpData>(style, red, green, blue, alpha);

    data() = gamma;
}

GammaOp::~GammaOp()
{
}

std::string GammaOp::getInfo() const
{
    return "<GammaOp>";
}

OpRcPtr GammaOp::clone() const
{
    GammaOpDataRcPtr f = gammaData()->clone();
    return std::make_shared<GammaOp>(f);
}

bool GammaOp::isSameType(ConstOpRcPtr & op) const
{
    ConstGammaOpRcPtr typedRcPtr = DynamicPtrCast<const GammaOp>(op);
    return (bool)typedRcPtr;
}

bool GammaOp::isInverse(ConstOpRcPtr & op) const
{
    ConstGammaOpRcPtr typedRcPtr = DynamicPtrCast<const GammaOp>(op);
    if(!typedRcPtr) return false;

    return gammaData()->isInverse(*typedRcPtr->gammaData());
}

bool GammaOp::canCombineWith(ConstOpRcPtr & op) const
{
    ConstGammaOpRcPtr typedRcPtr = DynamicPtrCast<const GammaOp>(op);
    return typedRcPtr ? gammaData()->mayCompose(*typedRcPtr->gammaData()) : false;
}

void GammaOp::combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const
{
    if(!canCombineWith(secondOp))
    {
        throw Exception("GammaOp: canCombineWith must be checked before calling combineWith.");
    }

    ConstGammaOpRcPtr typedRcPtr = DynamicPtrCast<const GammaOp>(secondOp);

    GammaOpDataRcPtr res = gammaData()->compose(*typedRcPtr->gammaData());
    CreateGammaOp(ops, res, TRANSFORM_DIR_FORWARD);
}

std::string GammaOp::getCacheID() const
{
    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<GammaOp ";
    cacheIDStream << gammaData()->getCacheID() << " ";
    cacheIDStream << ">";

    return cacheIDStream.str();
}

ConstOpCPURcPtr GammaOp::getCPUOp(bool fastLogExpPow) const
{
    ConstGammaOpDataRcPtr data = gammaData();
    return GetGammaRenderer(data, fastLogExpPow);
}

void GammaOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstGammaOpDataRcPtr data = gammaData();
    GetGammaGPUShaderProgram(shaderCreator, data);
}

}  // Anon namespace

void CreateGammaOp(OpRcPtrVec & ops,
                   GammaOpDataRcPtr & gammaData,
                   TransformDirection direction)
{
    auto gamma = gammaData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        gamma = gamma->inverse();
    }

    ops.push_back(std::make_shared<GammaOp>(gamma));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void CreateGammaTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    auto gamma = DynamicPtrCast<const GammaOp>(op);
    if (!gamma)
    {
        throw Exception("CreateGammaTransform: op has to be a GammaOp");
    }
    auto gammaData = DynamicPtrCast<const GammaOpData>(op->data());

    const auto style = gammaData->getStyle();

    if (style == GammaOpData::MONCURVE_FWD || style == GammaOpData::MONCURVE_MIRROR_FWD ||
        style == GammaOpData::MONCURVE_REV || style == GammaOpData::MONCURVE_MIRROR_REV)
    {
        auto expTransform = ExponentWithLinearTransform::Create();
        auto & data = dynamic_cast<ExponentWithLinearTransformImpl*>(expTransform.get())->data();

        data = *gammaData;
        group->appendTransform(expTransform);
    }
    else
    {
        auto expTransform = ExponentTransform::Create();
        auto & data = dynamic_cast<ExponentTransformImpl*>(expTransform.get())->data();

        data = *gammaData;
        group->appendTransform(expTransform);
    }
}

void BuildExponentWithLinearOp(OpRcPtrVec & ops,
                               const ExponentWithLinearTransform & transform,
                               TransformDirection dir)
{
    const auto & data = dynamic_cast<const ExponentWithLinearTransformImpl &>(transform).data();
    data.validate();

    auto gamma = data.clone();
    CreateGammaOp(ops, gamma, dir);
}

void BuildExponentOp(OpRcPtrVec & ops,
                     const Config & config,
                     const ExponentTransform & transform,
                     TransformDirection dir)
{
    if (config.getMajorVersion() == 1)
    {
        // Ignore style, use a simple exponent.
        TransformDirection combinedDir = CombineTransformDirections(dir, transform.getDirection());

        double vec4[4] = { 1., 1., 1., 1. };
        transform.getValue(vec4);
        ExponentOpDataRcPtr expData = std::make_shared<ExponentOpData>(vec4);
        expData->getFormatMetadata() = transform.getFormatMetadata();
        CreateExponentOp(ops,
                         expData,
                         combinedDir);
    }
    else
    {
        const auto & data = dynamic_cast<const ExponentTransformImpl &>(transform).data();
        data.validate();

        auto gamma = data.clone();
        CreateGammaOp(ops, gamma, dir);
    }
}

} // namespace OCIO_NAMESPACE

