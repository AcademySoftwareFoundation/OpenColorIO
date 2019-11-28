// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/cdl/CDLOpCPU.h"
#include "ops/cdl/CDLOp.h"
#include "ops/exponent/ExponentOp.h"
#include "ops/matrix/MatrixOp.h"

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

    void finalize(OptimizationFlags oFlags) override;

    ConstOpCPURcPtr getCPUOp() const override;

    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;
	
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

void CDLOp::finalize(OptimizationFlags /*oFlags*/)
{
    cdlData()->finalize();

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<CDLOp ";
    cacheIDStream << cdlData()->getCacheID() << " ";
    cacheIDStream << ">";

    m_cacheID = cacheIDStream.str();
}

ConstOpCPURcPtr CDLOp::getCPUOp() const
{
    ConstCDLOpDataRcPtr data = cdlData();
    return CDLOpCPU::GetRenderer(data);
}

void CDLOp::extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const
{
    RenderParams params;
    ConstCDLOpDataRcPtr cdlOpData = cdlData();
    params.update(cdlOpData);

    const float * slope    = params.getSlope();
    const float * offset   = params.getOffset();
    const float * power    = params.getPower();
    const float saturation = params.getSaturation();

    GpuShaderText ss(shaderDesc->getLanguage());
    ss.indent();

    ss.newLine() << "";
    ss.newLine() << "// Add CDL processing";
    ss.newLine() << "";

    ss.newLine() << "{";
    ss.indent();

    // Since alpha is not affected, only need to use the RGB components
    ss.declareVec3f("lumaWeights", 0.2126f,   0.7152f,   0.0722f  );
    ss.declareVec3f("slope",       slope [0], slope [1], slope [2]);
    ss.declareVec3f("offset",      offset[0], offset[1], offset[2]);
    ss.declareVec3f("power",       power [0], power [1], power [2]);

    ss.declareVar("saturation" , saturation);

    ss.newLine() << ss.vec3fDecl("pix") << " = "
                 << shaderDesc->getPixelName() << ".xyz;";

    if ( !params.isReverse() )
    {
        // Forward style

        // Slope
        ss.newLine() << "pix = pix * slope;";

        // Offset
        ss.newLine() << "pix = pix + offset;";

        // Power
        if ( !params.isNoClamp() )
        {
            ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
            ss.newLine() << "pix = pow(pix, power);";
        }
        else
        {
            ss.newLine() << ss.vec3fDecl("posPix") << " = step(0.0, pix);";
            ss.newLine() << ss.vec3fDecl("pixPower") << " = pow(abs(pix), power);";
            ss.newLine() << "pix = " << ss.lerp("pix", "pixPower", "posPix") << ";";
        }

        // Saturation
        ss.newLine() << "float luma = dot(pix, lumaWeights);";
        ss.newLine() << "pix = luma + saturation * (pix - luma);";

        // Post-saturation clamp
        if ( !params.isNoClamp() )
        {
            ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
        }
    }
    else
    {
        // Reverse style

        // Pre-saturation clamp
        if ( !params.isNoClamp() )
        {
            ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
        }

        // Saturation
        ss.newLine() << "float luma = dot(pix, lumaWeights);";
        ss.newLine() << "pix = luma + saturation * (pix - luma);";

        // Power
        if ( !params.isNoClamp() )
        {
            ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
            ss.newLine() << "pix = pow(pix, power);";
        }
        else
        {
            ss.newLine() << ss.vec3fDecl("posPix") << " = step(0.0, pix);";
            ss.newLine() << ss.vec3fDecl("pixPower") << " = pow(abs(pix), power);";
            ss.newLine() << "pix = " << ss.lerp("pix", "pixPower", "posPix") << ";";
        }

        // Offset
        ss.newLine() << "pix = pix + offset;";

        // Slope
        ss.newLine() << "pix = pix * slope;";

        // Post-slope clamp
        if ( !params.isNoClamp() )
        {
            ss.newLine() << "pix = clamp(pix, 0.0, 1.0);";
        }
    }

    ss.newLine() << shaderDesc->getPixelName() << ".xyz = pix;";

    ss.dedent();
    ss.newLine() << "}";

    shaderDesc->addToFunctionShaderCode(ss.string().c_str());
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
    const auto cdlStyle = cdlData->getStyle();
    if (cdlStyle == CDLOpData::CDL_NO_CLAMP_FWD || cdlStyle == CDLOpData::CDL_NO_CLAMP_REV)
    {
        throw Exception("CreateCDLTransform: NO_CLAMP style not supported.");
    }

    auto cdlDirection = TRANSFORM_DIR_FORWARD;
    if (cdlStyle == CDLOpData::CDL_V1_2_REV)
    {
        cdlDirection = TRANSFORM_DIR_INVERSE;
    }
    cdlTransform->setDirection(cdlDirection);

    auto & formatMetadata = cdlTransform->getFormatMetadata();
    auto & metadata = dynamic_cast<FormatMetadataImpl &>(formatMetadata);
    metadata = cdlData->getFormatMetadata();

    const CDLOpData::ChannelParams & slopes = cdlData->getSlopeParams();
    const CDLOpData::ChannelParams & offsets = cdlData->getOffsetParams();
    const CDLOpData::ChannelParams & powers = cdlData->getPowerParams();
    const double vec9[9]{ slopes[0], slopes[1], slopes[2],
                          offsets[0], offsets[1], offsets[2],
                          powers[0], powers[1], powers[2] };
    cdlTransform->setSOP(vec9);
    cdlTransform->setSat(cdlData->getSaturation());

    group->appendTransform(cdlTransform);
}

void BuildCDLOp(OpRcPtrVec & ops,
                const Config & config,
                const CDLTransform & cdlTransform,
                TransformDirection dir)
{
    TransformDirection combinedDir = CombineTransformDirections(dir,
                                                                cdlTransform.getDirection());

    double slope4[] = { 1.0, 1.0, 1.0, 1.0 };
    cdlTransform.getSlope(slope4);

    double offset4[] = { 0.0, 0.0, 0.0, 0.0 };
    cdlTransform.getOffset(offset4);

    double power4[] = { 1.0, 1.0, 1.0, 1.0 };
    cdlTransform.getPower(power4);

    double lumaCoef3[] = { 1.0, 1.0, 1.0 };
    cdlTransform.getSatLumaCoefs(lumaCoef3);

    double sat = cdlTransform.getSat();

    if(config.getMajorVersion()==1)
    {
        const double p[4] = { power4[0], power4[1], power4[2], power4[3] };

        if(combinedDir == TRANSFORM_DIR_FORWARD)
        {
            // 1) Scale + Offset
            CreateScaleOffsetOp(ops, slope4, offset4, TRANSFORM_DIR_FORWARD);

            // 2) Power + Clamp at 0 (NB: This is not in accord with the 
            //    ASC v1.2 spec since it also requires clamping at 1.)
            CreateExponentOp(ops, p, TRANSFORM_DIR_FORWARD);

            // 3) Saturation (NB: Does not clamp at 0 and 1
            //    as per ASC v1.2 spec)
            CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD);
        }
        else if(combinedDir == TRANSFORM_DIR_INVERSE)
        {
            // 3) Saturation (NB: Does not clamp at 0 and 1
            //    as per ASC v1.2 spec)
            CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_INVERSE);

            // 2) Power + Clamp at 0 (NB: This is not in accord with the 
            //    ASC v1.2 spec since it also requires clamping at 1.)
            CreateExponentOp(ops, p, TRANSFORM_DIR_INVERSE);

            // 1) Scale + Offset
            CreateScaleOffsetOp(ops, slope4, offset4, TRANSFORM_DIR_INVERSE);
        }
    }
    else
    {
        // Starting with the version 2, OCIO is now using a CDL Op
        // complying with the Common LUT Format (i.e. CLF) specification.
        CDLOpDataRcPtr cdlData = std::make_shared<CDLOpData>(
            CDLOpData::CDL_V1_2_FWD,
            CDLOpData::ChannelParams(slope4[0], slope4[1], slope4[2]),
            CDLOpData::ChannelParams(offset4[0], offset4[1], offset4[2]),
            CDLOpData::ChannelParams(power4[0], power4[1], power4[2]),
            sat);
        cdlData->getFormatMetadata() = cdlTransform.getFormatMetadata();

        CreateCDLOp(ops, 
                    cdlData,
                    combinedDir);
    }
}

} // namespace OCIO_NAMESPACE

