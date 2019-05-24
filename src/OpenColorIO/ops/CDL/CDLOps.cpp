/*
Copyright (c) 2018 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "GpuShaderUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "ops/CDL/CDLOpCPU.h"
#include "ops/CDL/CDLOps.h"



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
    CDLOp();

    CDLOp(CDLOpDataRcPtr & cdl, TransformDirection direction);

    CDLOp(BitDepth inBitDepth, 
          BitDepth outBitDepth,
          const std::string & id,
          const OpData::Descriptions & desc,
          CDLOpData::Style style,
          const double * slope3, 
          const double * offset3,
          const double * power3,
          double saturation,
          TransformDirection direction);

    virtual ~CDLOp();
    
    virtual OpRcPtr clone() const;
    
    virtual std::string getInfo() const;
    
    virtual bool isIdentity() const;
    virtual bool isSameType(ConstOpRcPtr & op) const;
    virtual bool isInverse(ConstOpRcPtr & op) const;
    virtual bool canCombineWith(ConstOpRcPtr & op) const;
    virtual void combineWith(OpRcPtrVec & ops, ConstOpRcPtr & secondOp) const;
    
    virtual void finalize();
    
    virtual void extractGpuShaderInfo(GpuShaderDescRcPtr & shaderDesc) const;

protected:
    ConstCDLOpDataRcPtr cdlData() const { return DynamicPtrCast<const CDLOpData>(data()); }
    CDLOpDataRcPtr cdlData() { return DynamicPtrCast<CDLOpData>(data()); }

private:
    TransformDirection m_direction;
};


CDLOp::CDLOp()
    :   Op()
    ,   m_direction(TRANSFORM_DIR_FORWARD)
{           
    data().reset(new CDLOpData());
}

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
             const std::string & id,
             const OpData::Descriptions & desc,
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
                      id, desc,
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

void CDLOp::finalize()
{
    const CDLOp & constThis = *this;

    if(m_direction == TRANSFORM_DIR_INVERSE)
    {
        data() = cdlData()->inverse();
        m_direction = TRANSFORM_DIR_FORWARD;
    }

    // Only the 32f processing is natively supported
    cdlData()->setInputBitDepth(BIT_DEPTH_F32);
    cdlData()->setOutputBitDepth(BIT_DEPTH_F32);

    cdlData()->validate();
    cdlData()->finalize();

    ConstCDLOpDataRcPtr cdlOpData = constThis.cdlData();
    m_cpuOp = CDLOpCPU::GetRenderer(cdlOpData);

    // Create the cacheID
    std::ostringstream cacheIDStream;
    cacheIDStream << "<CDLOp ";
    cacheIDStream << cdlData()->getCacheID() << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";
    cacheIDStream << ">";

    m_cacheID = cacheIDStream.str();
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
                 const std::string & id,
                 const OpData::Descriptions & desc,
                 CDLOpData::Style style,
                 const double * slope3, 
                 const double * offset3,
                 const double * power3,
                 double saturation,
                 TransformDirection direction)
{

    CDLOpDataRcPtr cdlData( 
        new CDLOpData(BIT_DEPTH_F32, BIT_DEPTH_F32,
                      id, desc, style,
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
    if(cdlData->isNoOp()) return;

    ops.push_back(std::make_shared<CDLOp>(cdlData, direction));
}


}
OCIO_NAMESPACE_EXIT



////////////////////////////////////////////////////////////////////////////////


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include <limits>
#include "unittest.h"
#include "UnitTestUtils.h"

OCIO_NAMESPACE_USING


void ApplyCDL(float * in, const float * ref, unsigned numPixels,
              const double * slope, const double * offset,
              const double * power, double saturation,
              OCIO::CDLOpData::Style style,
              float errorThreshold)
{
    OCIO::CDLOp cdlOp(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                      "", OCIO::OpData::Descriptions(),
                      style, slope, offset, power, saturation, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OIIO_CHECK_NO_THROW(cdlOp.finalize());

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
            OIIO_CHECK_ASSERT_MESSAGE(0, message.str());
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

OIIO_ADD_TEST(CDLOps, computed_identifier)
{
    OpRcPtrVec ops;

    CreateCDLOp(ops, 
                "", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset,
                CDL_DATA_1::power, CDL_DATA_1::saturation, 
                OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 1);

    CreateCDLOp(ops, 
                "", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset,
                CDL_DATA_1::power, CDL_DATA_1::saturation, 
                OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 2);

    OIIO_CHECK_NO_THROW( ops[0]->finalize() );
    OIIO_CHECK_NO_THROW( ops[1]->finalize() );

    OIIO_CHECK_EQUAL( ops[0]->getCacheID(), ops[1]->getCacheID() );


    CreateCDLOp(ops, 
                "1", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset,
                CDL_DATA_1::power, CDL_DATA_1::saturation, 
                OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 3);

    OIIO_CHECK_NO_THROW( ops[2]->finalize() );

    OIIO_CHECK_ASSERT( ops[0]->getCacheID() != ops[2]->getCacheID() );
    OIIO_CHECK_ASSERT( ops[1]->getCacheID() != ops[2]->getCacheID() );

    CreateCDLOp(ops, 
                "1", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset,
                CDL_DATA_1::power, CDL_DATA_1::saturation + 0.002f, 
                OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 4);

    OIIO_CHECK_NO_THROW( ops[3]->finalize() );

    OIIO_CHECK_ASSERT( ops[0]->getCacheID() != ops[3]->getCacheID() );
    OIIO_CHECK_ASSERT( ops[1]->getCacheID() != ops[3]->getCacheID() );
    OIIO_CHECK_ASSERT( ops[2]->getCacheID() != ops[3]->getCacheID() );

    CreateCDLOp(ops, 
                "1", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset,
                CDL_DATA_1::power, CDL_DATA_1::saturation + 0.002f, 
                OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 5);

    OIIO_CHECK_NO_THROW( ops[4]->finalize() );

    OIIO_CHECK_ASSERT( ops[0]->getCacheID() != ops[4]->getCacheID() );
    OIIO_CHECK_ASSERT( ops[1]->getCacheID() != ops[4]->getCacheID() );
    OIIO_CHECK_ASSERT( ops[2]->getCacheID() != ops[4]->getCacheID() );
    OIIO_CHECK_ASSERT( ops[3]->getCacheID() == ops[4]->getCacheID() );

    CreateCDLOp(ops, 
                "1", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_NO_CLAMP_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset,
                CDL_DATA_1::power, CDL_DATA_1::saturation + 0.002f, 
                OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 6);

    OIIO_CHECK_NO_THROW( ops[5]->finalize() );

    OIIO_CHECK_ASSERT( ops[3]->getCacheID() != ops[5]->getCacheID() );
    OIIO_CHECK_ASSERT( ops[4]->getCacheID() != ops[5]->getCacheID() );
}

OIIO_ADD_TEST(CDLOps, is_inverse)
{
    OpRcPtrVec ops;

    CreateCDLOp(ops, 
                "", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset,
                CDL_DATA_1::power, CDL_DATA_1::saturation, 
                OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 1);

    CreateCDLOp(ops, 
                "", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset,
                CDL_DATA_1::power, CDL_DATA_1::saturation, 
                OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OIIO_CHECK_ASSERT(ops[0]->isInverse(op1));
    OIIO_CHECK_ASSERT(ops[1]->isInverse(op0));

    CreateCDLOp(ops, 
                "", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_V1_2_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset, 
                CDL_DATA_1::power, 1.30, 
                OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO::ConstOpRcPtr op2 = ops[2];

    OIIO_CHECK_ASSERT(!ops[0]->isInverse(op2));
    OIIO_CHECK_ASSERT(!ops[1]->isInverse(op2));
    OIIO_CHECK_ASSERT(!ops[2]->isInverse(op0));
    OIIO_CHECK_ASSERT(!ops[2]->isInverse(op1));

    CreateCDLOp(ops, 
                "", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_V1_2_REV, 
                CDL_DATA_1::slope, CDL_DATA_1::offset, 
                CDL_DATA_1::power, 1.30, 
                OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO::ConstOpRcPtr op3 = ops[3];

    OIIO_CHECK_ASSERT(ops[2]->isInverse(op3));

    CreateCDLOp(ops, 
                "", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_V1_2_REV, 
                CDL_DATA_1::slope, CDL_DATA_1::offset, 
                CDL_DATA_1::power, 1.30, 
                OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO::ConstOpRcPtr op4 = ops[4];

    OIIO_CHECK_ASSERT(!ops[2]->isInverse(op4));
    OIIO_CHECK_ASSERT(ops[3]->isInverse(op4));

    CreateCDLOp(ops, 
                "", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_NO_CLAMP_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset, 
                CDL_DATA_1::power, 1.30, 
                OCIO::TRANSFORM_DIR_FORWARD);
    OIIO_REQUIRE_EQUAL(ops.size(), 6);
    OCIO::ConstOpRcPtr op5 = ops[5];

    OIIO_CHECK_ASSERT(!ops[2]->isInverse(op5));
    OIIO_CHECK_ASSERT(!ops[3]->isInverse(op5));
    OIIO_CHECK_ASSERT(!ops[4]->isInverse(op5));

    CreateCDLOp(ops, 
                "", OCIO::OpData::Descriptions(),
                OCIO::CDLOpData::CDL_NO_CLAMP_FWD, 
                CDL_DATA_1::slope, CDL_DATA_1::offset, 
                CDL_DATA_1::power, 1.30, 
                OCIO::TRANSFORM_DIR_INVERSE);
    OIIO_REQUIRE_EQUAL(ops.size(), 7);
    OCIO::ConstOpRcPtr op6 = ops[6];

    OIIO_CHECK_ASSERT(!ops[2]->isInverse(op6));
    OIIO_CHECK_ASSERT(!ops[3]->isInverse(op6));
    OIIO_CHECK_ASSERT(!ops[4]->isInverse(op6));
    OIIO_CHECK_ASSERT(ops[5]->isInverse(op6));
}

// The expected values below were calculated via an independent ASC CDL
// implementation.
// Note that the error thresholds are higher for the SSE version because
// of the use of a much faster, but somewhat less accurate, implementation
// of the power function.
// TODO: The NaN and Inf handling is probably not ideal, as shown by the 
// tests below, and could be improved.
OIIO_ADD_TEST(CDLOps, apply_clamp_fwd)
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

OIIO_ADD_TEST(CDLOps, apply_clamp_rev)
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

OIIO_ADD_TEST(CDLOps, apply_noclamp_fwd)
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

OIIO_ADD_TEST(CDLOps, apply_noclamp_rev)
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

OIIO_ADD_TEST(CDLOps, apply_clamp_fwd_2)
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

OIIO_ADD_TEST(CDLOps, apply_clamp_fwd_3)
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

OIIO_ADD_TEST(CDLOps, apply_noclamp_fwd_3)
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

#endif
