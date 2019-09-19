// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/CDL/CDLOpCPU.h"
#include "ops/CDL/CDLOps.h"
#include "ops/Exponent/ExponentOps.h"
#include "ops/Matrix/MatrixOps.h"


OCIO_NAMESPACE_ENTER
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

    CDLOp(CDLOpDataRcPtr & cdl, TransformDirection direction);

    CDLOp(BitDepth inBitDepth,
          BitDepth outBitDepth,
          const FormatMetadataImpl & info,
          CDLOpData::Style style,
          const double * slope3,
          const double * offset3,
          const double * power3,
          double saturation,
          TransformDirection direction);

    virtual ~CDLOp();

    TransformDirection getDirection() const noexcept override { return m_direction; }

    OpRcPtr clone() const override;

    std::string getInfo() const override;

    bool isIdentity() const override;
    bool isSameType(ConstOpRcPtr & op) const override;
    bool isInverse(ConstOpRcPtr & op) const override;
    bool canCombineWith(ConstOpRcPtr & op) const override;
    void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const override;

    void finalize(FinalizationFlags fFlags) override;

    ConstOpCPURcPtr getCPUOp() const override;

    void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const override;
	
protected:
    ConstCDLOpDataRcPtr cdlData() const { return DynamicPtrCast<const CDLOpData>(data()); }
    CDLOpDataRcPtr cdlData() { return DynamicPtrCast<CDLOpData>(data()); }

private:
    TransformDirection m_direction;
};


CDLOp::CDLOp(CDLOpDataRcPtr & cdl, TransformDirection direction)
    :   Op()
    ,   m_direction(direction)
{
    if(m_direction == TRANSFORM_DIR_UNKNOWN)
    {
        throw Exception(
            "Cannot create CDLOp with unspecified transform direction.");
    }

    data() = cdl;
}

CDLOp::CDLOp(BitDepth inBitDepth,
             BitDepth outBitDepth,
             const FormatMetadataImpl & info,
             CDLOpData::Style style,
             const double * slope3,
             const double * offset3,
             const double * power3,
             double saturation,
             TransformDirection direction)
    :   Op()
    ,   m_direction(direction)
{
    if(m_direction == TRANSFORM_DIR_UNKNOWN)
    {
        throw Exception(
            "Cannot create CDLOp with unspecified transform direction.");
    }

    if (!slope3 || !offset3 || !power3)
    {
        throw Exception("CDLOp: NULL pointer.");
    }

    data().reset(
        new CDLOpData(inBitDepth, outBitDepth,
                      info,
                      style, 
                      CDLOpData::ChannelParams(slope3[0], slope3[1], slope3[2]), 
                      CDLOpData::ChannelParams(offset3[0], offset3[1], offset3[2]), 
                      CDLOpData::ChannelParams(power3[0], power3[1], power3[2]), 
                      saturation));
}

OpRcPtr CDLOp::clone() const
{
    CDLOpDataRcPtr f = cdlData()->clone();
    return std::make_shared<CDLOp>(f, m_direction);
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

    if(GetInverseTransformDirection(m_direction)==typedRcPtr->m_direction)
    {
        return *cdlData()==*typedRcPtr->cdlData();
    }

    ConstCDLOpDataRcPtr cdlOpData = typedRcPtr->cdlData();
    return cdlData()->isInverse(cdlOpData);
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
        std::ostringstream os;
        os << "CDLOp can only be combined with other ";
        os << "CDLOps.  secondOp:" << secondOp->getInfo();
        throw Exception(os.str().c_str());
    }

    // TODO: Implement CDLOp::combineWith()
}

void CDLOp::finalize(FinalizationFlags /*fFlags*/)
{
    if(m_direction == TRANSFORM_DIR_INVERSE)
    {
        data() = cdlData()->inverse();
        m_direction = TRANSFORM_DIR_FORWARD;
    }

    cdlData()->finalize();

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<CDLOp ";
    cacheIDStream << cdlData()->getCacheID() << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";
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
    if (m_direction != TRANSFORM_DIR_FORWARD)
    {
        throw Exception(
            "CDLOp direction should have been set to forward by finalize");
    }

    if(getInputBitDepth()!=BIT_DEPTH_F32 || getOutputBitDepth()!=BIT_DEPTH_F32)
    {
        throw Exception(
            "Only 32F bit depth is supported for the GPU shader");
    }

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
                 const FormatMetadataImpl & info,
                 CDLOpData::Style style,
                 const double * slope3,
                 const double * offset3,
                 const double * power3,
                 double saturation,
                 TransformDirection direction)
{

    CDLOpDataRcPtr cdlData(
        new CDLOpData(BIT_DEPTH_F32, BIT_DEPTH_F32,
                      info, style,
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
    ops.push_back(std::make_shared<CDLOp>(cdlData, direction));
}

///////////////////////////////////////////////////////////////////////////

void CreateCDLTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
{
    const auto cdl = DynamicPtrCast<const CDLOp>(op);
    if (!cdl)
    {
        throw Exception("CreateCDLTransform: op has to be a CDLOp");
    }
    auto cdlTransform = CDLTransform::Create();
    cdlTransform->setDirection(cdl->getDirection());

    const auto cdlData = DynamicPtrCast<const CDLOpData>(op->data());
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

    group->push_back(cdlTransform);
}

void BuildCDLOps(OpRcPtrVec & ops,
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

        // TODO: how metadata should be handled?
    }
    else
    {
        // Starting with the version 2, OCIO is now using a CDL Op
        // complying with the Common LUT Format (i.e. CLF) specification.
        CDLOpDataRcPtr cdlData = std::make_shared<CDLOpData>(
            BIT_DEPTH_F32, BIT_DEPTH_F32,
            FormatMetadataImpl(cdlTransform.getFormatMetadata()),
            CDLOpData::CDL_V1_2_FWD,
            CDLOpData::ChannelParams(slope4[0], slope4[1], slope4[2]),
            CDLOpData::ChannelParams(offset4[0], offset4[1], offset4[2]),
            CDLOpData::ChannelParams(power4[0], power4[1], power4[2]),
            sat);

        CreateCDLOp(ops, 
                    cdlData,
                    combinedDir);
    }
}

}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

#include <limits>

#include "UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


void ApplyCDL(float * in, const float * ref, unsigned numPixels,
              const double * slope, const double * offset,
              const double * power, double saturation,
              OCIO::CDLOpData::Style style,
              float errorThreshold)
{
    OCIO::CDLOp cdlOp(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                      OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                      style, slope, offset, power, saturation, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(cdlOp.finalize(OCIO::FINALIZATION_EXACT));

    cdlOp.apply(in, in, numPixels);

    for(unsigned idx=0; idx<(numPixels*4); ++idx)
    {
        // Using rel error with a large minExpected value of 1 will transition
        // from absolute error for expected values < 1 and
        // relative error for values > 1.
        const bool equalRel = OCIO::EqualWithSafeRelError(in[idx],
                                                          ref[idx],
                                                          errorThreshold,
                                                          1.0f);
        if (!equalRel)
        {
            std::ostringstream message;
            message << "Index: " << idx;
            message << " - Values: " << in[idx] << " and: " << ref[idx];
            message << " - Threshold: " << errorThreshold;
            OCIO_CHECK_ASSERT_MESSAGE(0, message.str());
        }
    }

}


// TODO: CDL Unit tests with bit depth are missing.


namespace CDL_DATA_1
{
    const double slope[3]  = { 1.35,  1.1,  0.071 };
    const double offset[3] = { 0.05, -0.23, 0.11  };
    const double power[3]  = { 0.93,  0.81, 1.27  };
    const double saturation = 1.23;
};

OCIO_ADD_TEST(CDLOps, computed_identifier)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateCDLOp(ops, 
                      OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation, 
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::CreateCDLOp(ops, 
                      OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation, 
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO_CHECK_NO_THROW( ops[0]->finalize(OCIO::FINALIZATION_EXACT) );
    OCIO_CHECK_NO_THROW( ops[1]->finalize(OCIO::FINALIZATION_EXACT) );

    OCIO_CHECK_EQUAL( ops[0]->getCacheID(), ops[1]->getCacheID() );

    OCIO::FormatMetadataImpl metadata(OCIO::METADATA_ROOT);
	metadata.addAttribute(OCIO::METADATA_ID, "1");
    OCIO::CreateCDLOp(ops, 
                      metadata,
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 3);

    OCIO_CHECK_NO_THROW( ops[2]->finalize(OCIO::FINALIZATION_EXACT) );

    OCIO_CHECK_ASSERT( ops[0]->getCacheID() != ops[2]->getCacheID() );
    OCIO_CHECK_ASSERT( ops[1]->getCacheID() != ops[2]->getCacheID() );

    OCIO::CreateCDLOp(ops, 
                      metadata,
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation + 0.002f, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 4);

    OCIO_CHECK_NO_THROW( ops[3]->finalize(OCIO::FINALIZATION_EXACT) );

    OCIO_CHECK_ASSERT( ops[0]->getCacheID() != ops[3]->getCacheID() );
    OCIO_CHECK_ASSERT( ops[1]->getCacheID() != ops[3]->getCacheID() );
    OCIO_CHECK_ASSERT( ops[2]->getCacheID() != ops[3]->getCacheID() );

    OCIO::CreateCDLOp(ops, 
                      metadata,
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation + 0.002f, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 5);

    OCIO_CHECK_NO_THROW( ops[4]->finalize(OCIO::FINALIZATION_EXACT) );

    OCIO_CHECK_ASSERT( ops[0]->getCacheID() != ops[4]->getCacheID() );
    OCIO_CHECK_ASSERT( ops[1]->getCacheID() != ops[4]->getCacheID() );
    OCIO_CHECK_ASSERT( ops[2]->getCacheID() != ops[4]->getCacheID() );
    OCIO_CHECK_ASSERT( ops[3]->getCacheID() == ops[4]->getCacheID() );

    OCIO::CreateCDLOp(ops, 
                      metadata,
                      OCIO::CDLOpData::CDL_NO_CLAMP_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation + 0.002f, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 6);

    OCIO_CHECK_NO_THROW( ops[5]->finalize(OCIO::FINALIZATION_EXACT) );

    OCIO_CHECK_ASSERT( ops[3]->getCacheID() != ops[5]->getCacheID() );
    OCIO_CHECK_ASSERT( ops[4]->getCacheID() != ops[5]->getCacheID() );
}

OCIO_ADD_TEST(CDLOps, is_inverse)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateCDLOp(ops, 
                      OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation, 
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::CreateCDLOp(ops, 
                      OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation, 
                      OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(ops[0]->isInverse(op1));
    OCIO_CHECK_ASSERT(ops[1]->isInverse(op0));

    OCIO::CreateCDLOp(ops, 
                      OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset, 
                      CDL_DATA_1::power, 1.30, 
                      OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO::ConstOpRcPtr op2 = ops[2];

    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op2));
    OCIO_CHECK_ASSERT(!ops[1]->isInverse(op2));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op0));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op1));

    OCIO::CreateCDLOp(ops, 
                      OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                      OCIO::CDLOpData::CDL_V1_2_REV, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset, 
                      CDL_DATA_1::power, 1.30, 
                      OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO::ConstOpRcPtr op3 = ops[3];

    OCIO_CHECK_ASSERT(ops[2]->isInverse(op3));

    OCIO::CreateCDLOp(ops,
                      OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                      OCIO::CDLOpData::CDL_V1_2_REV, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset, 
                      CDL_DATA_1::power, 1.30, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO::ConstOpRcPtr op4 = ops[4];

    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op4));
    OCIO_CHECK_ASSERT(ops[3]->isInverse(op4));

    OCIO::CreateCDLOp(ops,
                      OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                      OCIO::CDLOpData::CDL_NO_CLAMP_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset, 
                      CDL_DATA_1::power, 1.30, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_REQUIRE_EQUAL(ops.size(), 6);
    OCIO::ConstOpRcPtr op5 = ops[5];

    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op5));
    OCIO_CHECK_ASSERT(!ops[3]->isInverse(op5));
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op5));

    OCIO::CreateCDLOp(ops,
                      OCIO::FormatMetadataImpl(OCIO::METADATA_ROOT),
                      OCIO::CDLOpData::CDL_NO_CLAMP_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset, 
                      CDL_DATA_1::power, 1.30, 
                      OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_REQUIRE_EQUAL(ops.size(), 7);
    OCIO::ConstOpRcPtr op6 = ops[6];

    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op6));
    OCIO_CHECK_ASSERT(!ops[3]->isInverse(op6));
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op6));
    OCIO_CHECK_ASSERT(ops[5]->isInverse(op6));
}

// The expected values below were calculated via an independent ASC CDL
// implementation.
// Note that the error thresholds are higher for the SSE version because
// of the use of a much faster, but somewhat less accurate, implementation
// of the power function.
// TODO: The NaN and Inf handling is probably not ideal, as shown by the
// tests below, and could be improved.
OCIO_ADD_TEST(CDLOps, apply_clamp_fwd)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
         qnan,    qnan,  qnan,  0.0f,
         0.0f,    0.0f,  0.0f,  qnan,
         inf,     inf,   inf,   inf,
        -inf,    -inf,  -inf,  -inf,
         0.3278f, 0.01f, 1.0f,  0.0f,
         0.25f,   0.5f,  0.75f, 1.0f,
         1.25f,   1.5f,  1.75f, 0.75f,
        -0.2f,    0.5f,  1.4f,  0.0f,
        -0.25f,  -0.5f, -0.75f, 0.25f,
         0.0f,    0.8f,  0.99f, 0.5f };

    const float expected_32f[] = {
        0.0f,      0.0f,      0.0f,      0.0f,
        0.071827f, 0.0f,      0.070533f, qnan,
        1.0f,      1.0f,      1.0f,      inf,
        0.0f,      0.0f,      0.0f,     -inf,
        0.609399f, 0.000000f, 0.113130f, 0.0f,
        0.422056f, 0.401466f, 0.035820f, 1.0f,
        1.000000f, 1.000000f, 0.000000f, 0.75f,
        0.000000f, 0.421096f, 0.101225f, 0.0f,
        0.000000f, 0.000000f, 0.031735f, 0.25f,
        0.000000f, 0.746748f, 0.018691f, 0.5f  };

    ApplyCDL(input_32f, expected_32f, 10,
             CDL_DATA_1::slope, CDL_DATA_1::offset,
             CDL_DATA_1::power, CDL_DATA_1::saturation,
             OCIO::CDLOpData::CDL_V1_2_FWD,
#ifdef USE_SSE
             4e-6f);
#else
             2e-6f);
#endif
}

OCIO_ADD_TEST(CDLOps, apply_clamp_rev)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
       qnan,       qnan,       qnan,      0.0f,
       0.0f,       0.0f,       0.0f,      qnan,
       inf,        inf,        inf,       inf,
      -inf,       -inf,       -inf,      -inf,
       0.609399f,  0.100000f,  0.113130f, 0.0f,
       0.001000f,  0.746748f,  0.018691f, 0.5f,
       0.422056f,  0.401466f,  0.035820f, 1.0f,
      -0.25f,     -0.5f,      -0.75f,     0.25f,
       1.25f,      1.5f,       1.75f,     0.75f,
      -0.2f,       0.5f,       1.4f,      0.0f };

    const float expected_32f[] = {
      0.0f,      0.209091f, 0.0f,      0.0f,
      0.0f,      0.209091f, 0.0f,      qnan,
      0.703713f, 1.0f,      1.0f,       inf,
      0.0f,      0.209091f, 0.0f,      -inf,
      0.340710f, 0.275726f, 1.000000f, 0.0f,
      0.025902f, 0.801895f, 1.000000f, 0.5f,
      0.250000f, 0.500000f, 0.750006f, 1.0f,
      0.000000f, 0.209091f, 0.000000f, 0.25f,
      0.703704f, 1.000000f, 1.000000f, 0.75f,
      0.012206f, 0.582944f, 1.000000f, 0.0f };

    ApplyCDL(input_32f, expected_32f, 10,
             CDL_DATA_1::slope, CDL_DATA_1::offset,
             CDL_DATA_1::power, CDL_DATA_1::saturation,
             OCIO::CDLOpData::CDL_V1_2_REV,
#ifdef USE_SSE
             9e-6f);
#else
             1e-5f);
#endif
}

OCIO_ADD_TEST(CDLOps, apply_noclamp_fwd)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
       qnan,    qnan,  qnan,  0.0f,
       0.0f,    0.0f,  0.0f,  qnan,
       inf,     inf,   inf,   inf,
      -inf,    -inf,  -inf,  -inf,
       0.3278f, 0.01f, 1.0f,  0.0f,
       0.0f,    0.8f,  0.99f, 0.5f,
       0.25f,   0.5f,  0.75f, 1.0f,
      -0.25f,  -0.5f, -0.75f, 0.25f,
       1.25f,   1.5f,  1.75f, 0.75f,
      -0.2f,    0.5f,  1.4f,  0.0f };

    const float expected_32f[] = {
       0.0f,       0.0f,       0.0f,      0.0f,
       0.109661f, -0.249088f,  0.108368f, qnan,
       qnan,      qnan,        qnan,       inf,
       qnan,      qnan,        qnan,      -inf,
       0.645424f, -0.260548f,  0.149154f, 0.0f,
      -0.045094f,  0.746748f,  0.018691f, 0.5f,
       0.422056f,  0.401466f,  0.035820f, 1.0f,
      -0.211694f, -0.817469f,  0.174100f, 0.25f,
       1.753162f,  1.331130f, -0.108181f, 0.75f,
      -0.327485f,  0.431854f,  0.111983f, 0.0f };

    ApplyCDL(input_32f, expected_32f, 10,
             CDL_DATA_1::slope, CDL_DATA_1::offset,
             CDL_DATA_1::power, CDL_DATA_1::saturation,
             OCIO::CDLOpData::CDL_NO_CLAMP_FWD,
#ifdef USE_SSE
             2e-5f);
#else
             2e-6f);
#endif
}

OCIO_ADD_TEST(CDLOps, apply_noclamp_rev)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
       qnan,       qnan,       qnan,      0.0f,
       0.0f,       0.0f,       0.0f,      qnan,
       inf,        inf,        inf,       inf,
      -inf,       -inf,       -inf,      -inf,
       0.609399f,  0.100000f,  0.113130f, 0.0f,
       0.001000f,  0.746748f,  0.018691f, 0.5f,
       0.422056f,  0.401466f,  0.035820f, 1.0f,
      -0.25f,     -0.5f,      -0.75f,     0.25f,
       1.25f,      1.5f,       1.75f,     0.75f,
      -0.2f,       0.5f,       1.4f,      0.0f };

    const float expected_32f[] = {
      -0.037037f,  0.209091f, -1.549296f,  0.0f,
      -0.037037f,  0.209091f, -1.549296f,  qnan,
      -0.037037f,  0.209091f, -1.549296f,  inf,
      -0.037037f,  0.209091f, -1.549296f, -inf,
       0.340710f,  0.275726f,  1.294827f,  0.0f,
       0.025902f,  0.801895f,  1.022221f,  0.5f,
       0.250000f,  0.500000f,  0.750006f,  1.0f,
      -0.251989f, -0.239488f, -11.361812f, 0.25f,
       0.937160f,  1.700692f,  19.807237f, 0.75f,
      -0.099839f,  0.580528f,  14.880301f, 0.0f };

    ApplyCDL(input_32f, expected_32f, 10,
             CDL_DATA_1::slope, CDL_DATA_1::offset,
             CDL_DATA_1::power, CDL_DATA_1::saturation,
             OCIO::CDLOpData::CDL_NO_CLAMP_REV,
#ifdef USE_SSE
             3e-5f);
#else
             1e-6f);
#endif
}

namespace CDL_DATA_2
{
    const double slope[3]  = { 1.15, 1.10, 0.9  };
    const double offset[3] = { 0.05, 0.02, 0.07 };
    const double power[3]  = { 1.2,  0.95, 1.13 };
    const double saturation = 0.87;
};

OCIO_ADD_TEST(CDLOps, apply_clamp_fwd_2)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
      qnan,  qnan,  qnan,  0.0f,
      0.0f,  0.0f,  0.0f,  qnan,
      inf,   inf,   inf,   inf,
     -inf,  -inf,  -inf,  -inf,
      0.65f, 0.55f, 0.20f, 0.0f,
      0.41f, 0.81f, 0.39f, 0.5f,
      0.25f, 0.50f, 0.75f, 1.0f };

    const float expected_32f[] = {
      0.0f,      0.0f,      0.0f,      0.0f,
      0.027379f, 0.024645f, 0.046585f, qnan,
      1.0f,      1.0f,      1.0f,      inf,
      0.0f,      0.0f,      0.0,      -inf,
      0.745644f, 0.639197f, 0.264149f, 0.0f,
      0.499594f, 0.897554f, 0.428591f, 0.5f,
      0.305035f, 0.578779f, 0.692558f, 1.0f };

    ApplyCDL(input_32f, expected_32f, 7,
             CDL_DATA_2::slope, CDL_DATA_2::offset,
             CDL_DATA_2::power, CDL_DATA_2::saturation,
             OCIO::CDLOpData::CDL_V1_2_FWD,
#ifdef USE_SSE
             7e-6f);
#else
             1e-6f);
#endif
}

namespace CDL_DATA_3
{
    const double slope[3]  = {  3.405,  1.0,    1.0   };
    const double offset[3] = { -0.178, -0.178, -0.178 };
    const double power[3]  = {  1.095,  1.095,  1.095 };
    const double saturation = 0.99;
};

OCIO_ADD_TEST(CDLOps, apply_clamp_fwd_3)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
      qnan,  qnan, qnan, 0.0f,
      0.0f,  0.0f, 0.0f, qnan,
      inf,   inf,  inf,  inf,
     -inf,  -inf, -inf, -inf,

      0.02f, 0.0f, 0.0f, 0.0f,
      0.17f, 0.0f, 0.0f, 0.0f,
      0.65f, 0.0f, 0.0f, 0.0f,
      0.97f, 0.0f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.0f, 0.0f,
      0.17f, 0.13f, 0.0f, 0.0f,
      0.65f, 0.13f, 0.0f, 0.0f,
      0.97f, 0.13f, 0.0f, 0.0f,

      0.02f, 0.23f, 0.0f, 0.0f,
      0.17f, 0.23f, 0.0f, 0.0f,
      0.65f, 0.23f, 0.0f, 0.0f,
      0.97f, 0.23f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.23f, 0.0f,
      0.17f, 0.13f, 0.23f, 0.0f,
      0.65f, 0.13f, 0.23f, 0.0f,
      0.97f, 0.13f, 0.23f, 0.0f };

    const float expected_32f[] = {
      0.000000f, 0.000000f, 0.000000f, 0.0f,
      0.000000f, 0.000000f, 0.000000f, qnan,
      1.0f,      1.0f,      1.0f,      inf,
      0.0f,      0.0f,      0.0f,     -inf,

      0.000000f, 0.000000f, 0.000000f, 0.0f,
      0.364613f, 0.000781f, 0.000781f, 0.0f,
      0.992126f, 0.002126f, 0.002126f, 0.0f,
      0.992126f, 0.002126f, 0.002126f, 0.0f,

      0.000000f, 0.000000f, 0.000000f, 0.0f,
      0.364613f, 0.000781f, 0.000781f, 0.0f,
      0.992126f, 0.002126f, 0.002126f, 0.0f,
      0.992126f, 0.002126f, 0.002126f, 0.0f,

      0.000281f, 0.039155f, 0.0002808f, 0.0f,
      0.364894f, 0.039936f, 0.0010621f, 0.0f,
      0.992407f, 0.041281f, 0.0024068f, 0.0f,
      0.992407f, 0.041281f, 0.0024068f, 0.0f,

      0.000028f, 0.000028f, 0.0389023f, 0.0f,
      0.364641f, 0.000810f, 0.0396836f, 0.0f,
      0.992154f, 0.002154f, 0.0410283f, 0.0f,
      0.992154f, 0.002154f, 0.0410283f, 0.0f };

    ApplyCDL(input_32f, expected_32f, 20,
             CDL_DATA_3::slope, CDL_DATA_3::offset,
             CDL_DATA_3::power, CDL_DATA_3::saturation,
             OCIO::CDLOpData::CDL_V1_2_FWD,
#ifdef USE_SSE
             2e-5f);
#else
             1e-6f);
#endif
}

OCIO_ADD_TEST(CDLOps, apply_noclamp_fwd_3)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
      qnan,  qnan, qnan, 0.0f,
      0.0f,  0.0f, 0.0f, qnan,
      inf,   inf,  inf,  inf,
     -inf,  -inf, -inf, -inf,

      0.02f, 0.0f, 0.0f, 0.0f,
      0.17f, 0.0f, 0.0f, 0.0f,
      0.65f, 0.0f, 0.0f, 0.0f,
      0.97f, 0.0f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.0f, 0.0f,
      0.17f, 0.13f, 0.0f, 0.0f,
      0.65f, 0.13f, 0.0f, 0.0f,
      0.97f, 0.13f, 0.0f, 0.0f,

      0.02f, 0.23f, 0.0f, 0.0f,
      0.17f, 0.23f, 0.0f, 0.0f,
      0.65f, 0.23f, 0.0f, 0.0f,
      0.97f, 0.23f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.23f, 0.0f,
      0.17f, 0.13f, 0.23f, 0.0f,
      0.65f, 0.13f, 0.23f, 0.0f,
      0.97f, 0.13f, 0.23f, 0.0f };

    const float expected_32f[] = {
       0.0f,       0.0f,       0.0f,      0.0f,
      -0.178000f, -0.178000f, -0.178000f, qnan,
       qnan,       qnan,       qnan,      inf,
       qnan,       qnan,       qnan,     -inf,

      -0.110436f, -0.177855f, -0.177855f, 0.0f,
       0.363211f, -0.176840f, -0.176840f, 0.0f,
       2.158845f, -0.172992f, -0.172992f, 0.0f,
       3.453254f, -0.170219f, -0.170219f, 0.0f,

      -0.109506f, -0.048225f, -0.176925f, 0.0f,
       0.364141f, -0.047210f, -0.175910f, 0.0f,
       2.159774f, -0.043363f, -0.172063f, 0.0f,
       3.454184f, -0.040589f, -0.169289f, 0.0f,

      -0.108882f, 0.038793f, -0.176301f, 0.0f,
       0.364765f, 0.039808f, -0.175286f, 0.0f,
       2.160399f, 0.043655f, -0.171438f, 0.0f,
       3.454808f, 0.046429f, -0.168665f, 0.0f,

      -0.109350f, -0.048069f, 0.038325f, 0.0f,
       0.364298f, -0.047054f, 0.039340f, 0.0f,
       2.159931f, -0.043206f, 0.043188f, 0.0f,
       3.454341f, -0.040432f, 0.045962f, 0.0f };

    ApplyCDL(input_32f, expected_32f, 20,
             CDL_DATA_3::slope, CDL_DATA_3::offset,
             CDL_DATA_3::power, CDL_DATA_3::saturation,
             OCIO::CDLOpData::CDL_NO_CLAMP_FWD,
#ifdef USE_SSE
             5e-6f);
#else
             1e-6f);
#endif
}

OCIO_ADD_TEST(CDLOps, create_transform)
{
    OCIO::FormatMetadataImpl metadataSource(OCIO::METADATA_ROOT);
    metadataSource.addAttribute(OCIO::METADATA_ID, "Test look: 01-A.");

    OCIO::CDLOp * cdlOp = new OCIO::CDLOp(OCIO::BIT_DEPTH_UINT32,
                                          OCIO::BIT_DEPTH_UINT10,
                                          metadataSource,
                                          OCIO::CDLOpData::CDL_V1_2_FWD,
                                          CDL_DATA_1::slope,
                                          CDL_DATA_1::offset,
                                          CDL_DATA_1::power,
                                          CDL_DATA_1::saturation,
                                          OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    OCIO::ConstOpRcPtr op(cdlOp);
    OCIO::CreateCDLTransform(group, op);
    OCIO_REQUIRE_EQUAL(group->size(), 1);
    auto transform = group->getTransform(0);
    OCIO_REQUIRE_ASSERT(transform);
    auto cdlTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::CDLTransform>(transform);
    OCIO_REQUIRE_ASSERT(cdlTransform);

    const auto & metadata = cdlTransform->getFormatMetadata();
    OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), OCIO::METADATA_ID);
    OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "Test look: 01-A.");

    OCIO_CHECK_EQUAL(cdlTransform->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    double slope[3];
    cdlTransform->getSlope(slope);
    OCIO_CHECK_EQUAL(slope[0], CDL_DATA_1::slope[0]);
    OCIO_CHECK_EQUAL(slope[1], CDL_DATA_1::slope[1]);
    OCIO_CHECK_EQUAL(slope[2], CDL_DATA_1::slope[2]);

    double offset[3];
    cdlTransform->getOffset(offset);
    OCIO_CHECK_EQUAL(offset[0], CDL_DATA_1::offset[0]);
    OCIO_CHECK_EQUAL(offset[1], CDL_DATA_1::offset[1]);
    OCIO_CHECK_EQUAL(offset[2], CDL_DATA_1::offset[2]);

    double power[3];
    cdlTransform->getPower(power);
    OCIO_CHECK_EQUAL(power[0], CDL_DATA_1::power[0]);
    OCIO_CHECK_EQUAL(power[1], CDL_DATA_1::power[1]);
    OCIO_CHECK_EQUAL(power[2], CDL_DATA_1::power[2]);

    OCIO_CHECK_EQUAL(cdlTransform->getSat(), CDL_DATA_1::saturation);
}

#endif
