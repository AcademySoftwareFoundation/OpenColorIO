// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/cdl/CDLOp.h"
#include "ops/cdl/CDLOpCPU.h"
#include "ops/cdl/CDLOpGPU.h"
#include "ops/exponent/ExponentOp.h"
#include "ops/matrix/MatrixOp.h"
#include "transforms/CDLTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{

class CDLOp;
typedef OCIO_SHARED_PTR<CDLOp> CDLOpRcPtr;
typedef OCIO_SHARED_PTR<const CDLOp> ConstCDLOpRcPtr;

class CDLOp : public Op
{
public:
    CDLOp() = delete;

    CDLOp(CDLOpDataRcPtr & cdl);

    CDLOp(CDLOpData::Style style,
          const double * slope3,
          const double * offset3,
          const double * power3,
          double saturation);

    virtual ~CDLOp();

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isIdentity() const override;
    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    std::string getCacheID() const override;

    ConstOpCPURcPtr getCPUOp(bool fastLogExpPow) const override;

    void extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const override;

protected:
    ConstCDLOpDataRcPtr cdlData() const { return DynamicPtrCast<const CDLOpData>(data()); }
    CDLOpDataRcPtr cdlData() { return DynamicPtrCast<CDLOpData>(data()); }
};


CDLOp::CDLOp(CDLOpDataRcPtr & cdl)
    :   Op()
{
    data() = cdl;
}

CDLOp::CDLOp(CDLOpData::Style style,
             const double * slope3,
             const double * offset3,
             const double * power3,
             double saturation)
    :   Op()
{
    if (!slope3 || !offset3 || !power3)
    {
        throw Exception("CDLOp: NULL pointer.");
    }

    data().reset(
        new CDLOpData(style,
                      CDLOpData::ChannelParams(slope3[0], slope3[1], slope3[2]),
                      CDLOpData::ChannelParams(offset3[0], offset3[1], offset3[2]),
                      CDLOpData::ChannelParams(power3[0], power3[1], power3[2]),
                      saturation));
}

OpRcPtr CDLOp::clone() const
{
    CDLOpDataRcPtr cdl = cdlData()->clone();
    return std::make_shared<CDLOp>(cdl);
}

CDLOp::~CDLOp()
{
}

std::string CDLOp::getInfo() const
{
    return "<CDLOp>";
}

bool CDLOp::isIdentity() const
{
    return cdlData()->isIdentity();
}

bool CDLOp::isSameType(ConstOpRcPtr & op) const
{
    ConstCDLOpRcPtr typedRcPtr = DynamicPtrCast<const CDLOp>(op);
    return (bool)typedRcPtr;
}

bool CDLOp::isInverse(ConstOpRcPtr & op) const
{
    ConstCDLOpRcPtr typedRcPtr = DynamicPtrCast<const CDLOp>(op);
    if(!typedRcPtr) return false;

    ConstCDLOpDataRcPtr cdlData2 = typedRcPtr->cdlData();
    return cdlData()->isInverse(cdlData2);
}

bool CDLOp::canCombineWith(ConstOpRcPtr & /*op*/) const
{
    // TODO: Allow combining with LUTs.
    // TODO: Allow combining with matrices.
    return false;
}

void CDLOp::combineWith(OpRcPtrVec & /*ops*/, ConstOpRcPtr & secondOp) const
{
    if(!canCombineWith(secondOp))
    {
        throw Exception("CDLOp: canCombineWith must be checked before calling combineWith.");
    }

    // TODO: Implement CDLOp::combineWith()
}

std::string CDLOp::getCacheID() const
{
    // Create the cacheID.
    std::ostringstream cacheIDStream;
    cacheIDStream << "<CDLOp ";
    cacheIDStream << cdlData()->getCacheID();
    cacheIDStream << ">";

    return cacheIDStream.str();
}

ConstOpCPURcPtr CDLOp::getCPUOp(bool fastLogExpPow) const
{
    ConstCDLOpDataRcPtr data = cdlData();
    return GetCDLCPURenderer(data, fastLogExpPow);
}

void CDLOp::extractGpuShaderInfo(GpuShaderCreatorRcPtr & shaderCreator) const
{
    ConstCDLOpDataRcPtr data = cdlData();
    GetCDLGPUShaderProgram(shaderCreator, data);
}

}  // Anon namespace




///////////////////////////////////////////////////////////////////////////



void CreateCDLOp(OpRcPtrVec & ops,
                 CDLOpData::Style style,
                 const double * slope3,
                 const double * offset3,
                 const double * power3,
                 double saturation,
                 TransformDirection direction)
{

    CDLOpDataRcPtr cdlData(
        new CDLOpData(style,
                      CDLOpData::ChannelParams(slope3[0], slope3[1], slope3[2]),
                      CDLOpData::ChannelParams(offset3[0], offset3[1], offset3[2]),
                      CDLOpData::ChannelParams(power3[0], power3[1], power3[2]),
                      saturation) );

    CreateCDLOp(ops, cdlData, direction);
}

void CreateCDLOp(OpRcPtrVec & ops,
                 CDLOpDataRcPtr & cdlData,
                 TransformDirection direction)
{
    auto cdl = cdlData;
    if (direction == TRANSFORM_DIR_INVERSE)
    {
        cdl = cdl->inverse();
    }

    ops.push_back(std::make_shared<CDLOp>(cdl));
}

///////////////////////////////////////////////////////////////////////////

void CreateCDLTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    const auto cdl = DynamicPtrCast<const CDLOp>(op);
    if (!cdl)
    {
        throw Exception("CreateCDLTransform: op has to be a CDLOp");
    }
    const auto cdlData = DynamicPtrCast<const CDLOpData>(cdl->data());
    auto cdlTransform = CDLTransform::Create();
    auto & data = dynamic_cast<CDLTransformImpl *>(cdlTransform.get())->data();

    data = *cdlData;
    group->appendTransform(cdlTransform);
}

void BuildCDLOp(OpRcPtrVec & ops,
                const Config & config,
                const CDLTransform & cdlTransform,
                TransformDirection dir)
{
    if (config.getMajorVersion() == 1)
    {
        const auto combinedDir = CombineTransformDirections(dir, cdlTransform.getDirection());

        double slope4[] = { 1.0, 1.0, 1.0, 1.0 };
        cdlTransform.getSlope(slope4);

        double offset4[] = { 0.0, 0.0, 0.0, 0.0 };
        cdlTransform.getOffset(offset4);

        double power4[] = { 1.0, 1.0, 1.0, 1.0 };
        cdlTransform.getPower(power4);

        double lumaCoef3[] = { 1.0, 1.0, 1.0 };
        cdlTransform.getSatLumaCoefs(lumaCoef3);

        double sat = cdlTransform.getSat();

        switch (combinedDir)
        {
        case TRANSFORM_DIR_FORWARD:
        {
            // 1) Scale + Offset
            CreateScaleOffsetOp(ops, slope4, offset4, TRANSFORM_DIR_FORWARD);

            // 2) Power + Clamp at 0 (NB: This is not in accord with the
            //    ASC v1.2 spec since it also requires clamping at 1.)
            CreateExponentOp(ops, power4, TRANSFORM_DIR_FORWARD);

            // 3) Saturation (NB: Does not clamp at 0 and 1
            //    as per ASC v1.2 spec)
            CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD);
            break;
        }
        case TRANSFORM_DIR_INVERSE:
        {
            // 3) Saturation (NB: Does not clamp at 0 and 1
            //    as per ASC v1.2 spec)
            CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_INVERSE);

            // 2) Power + Clamp at 0 (NB: This is not in accord with the
            //    ASC v1.2 spec since it also requires clamping at 1.)
            CreateExponentOp(ops, power4, TRANSFORM_DIR_INVERSE);

            // 1) Scale + Offset
            CreateScaleOffsetOp(ops, slope4, offset4, TRANSFORM_DIR_INVERSE);
            break;
        }
        }
    }
    else
    {
        // Starting with the version 2, OCIO is now using a CDL Op
        // complying with the Common LUT Format (i.e. CLF) specification.
        const auto & data = dynamic_cast<const CDLTransformImpl &>(cdlTransform).data();
        data.validate();

        auto cdl = data.clone();
        CreateCDLOp(ops, cdl, dir);
    }
}

} // namespace OCIO_NAMESPACE

