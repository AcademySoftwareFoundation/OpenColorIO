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

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ops/CDL/CDLOpData.h"
#include "ops/Matrix/MatrixOps.h"
#include "ops/Range/RangeOpData.h"
#include "Platform.h"


OCIO_NAMESPACE_ENTER
{

namespace DefaultValues
{
    const int FLOAT_DECIMALS = 7;
}


static const CDLOpData::ChannelParams kOneParams(1.0);
static const CDLOpData::ChannelParams kZeroParams(0.0);

// Original CTF styles:
static const char V1_2_FWD_NAME[] = "v1.2_Fwd";
static const char V1_2_REV_NAME[] = "v1.2_Rev";
static const char NO_CLAMP_FWD_NAME[] = "noClampFwd";
static const char NO_CLAMP_REV_NAME[] = "noClampRev";

// CLF styles (also allowed now in CTF):
static const char V1_2_FWD_CLF_NAME[] = "Fwd";
static const char V1_2_REV_CLF_NAME[] = "Rev";
static const char NO_CLAMP_FWD_CLF_NAME[] = "FwdNoClamp";
static const char NO_CLAMP_REV_CLF_NAME[] = "RevNoClamp";

CDLOpData::Style CDLOpData::GetStyle(const char* name)
{

// Get the style enum  of the CDL from the name stored
// in the "name" variable.
#define RETURN_STYLE_FROM_NAME(CDL_STYLE_NAME, CDL_STYLE)  \
if( 0==Platform::Strcasecmp(name, CDL_STYLE_NAME) )        \
{                                                          \
return CDL_STYLE;                                          \
}

    if (name && *name)
    {
        RETURN_STYLE_FROM_NAME(V1_2_FWD_NAME, CDL_V1_2_FWD);
        RETURN_STYLE_FROM_NAME(V1_2_FWD_CLF_NAME, CDL_V1_2_FWD);
        RETURN_STYLE_FROM_NAME(V1_2_REV_NAME, CDL_V1_2_REV);
        RETURN_STYLE_FROM_NAME(V1_2_REV_CLF_NAME, CDL_V1_2_REV);
        RETURN_STYLE_FROM_NAME(NO_CLAMP_FWD_NAME, CDL_NO_CLAMP_FWD);
        RETURN_STYLE_FROM_NAME(NO_CLAMP_FWD_CLF_NAME, CDL_NO_CLAMP_FWD);
        RETURN_STYLE_FROM_NAME(NO_CLAMP_REV_NAME, CDL_NO_CLAMP_REV);
        RETURN_STYLE_FROM_NAME(NO_CLAMP_REV_CLF_NAME, CDL_NO_CLAMP_REV);
    }

#undef RETURN_STYLE_FROM_NAME

    throw Exception("Unknown style for CDL.");
}

const char * CDLOpData::GetStyleName(CDLOpData::Style style)
{
    // Get the name of the CDL style from the enum stored
    // in the "style" variable.
    switch (style)
    {
        case CDL_V1_2_FWD:     return V1_2_FWD_CLF_NAME;
        case CDL_V1_2_REV:     return V1_2_REV_CLF_NAME;
        case CDL_NO_CLAMP_FWD: return NO_CLAMP_FWD_CLF_NAME;
        case CDL_NO_CLAMP_REV: return NO_CLAMP_REV_CLF_NAME;
    }

    throw Exception("Unknown style for CDL.");

    return 0x0;
}

CDLOpData::CDLOpData()
    :   OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    ,   m_style(GetDefaultStyle())
    ,   m_slopeParams(1.0)
    ,   m_offsetParams(0.0)
    ,   m_powerParams(1.0)
    ,   m_saturation(1.0)
{
}

CDLOpData::CDLOpData(BitDepth inBitDepth,
                     BitDepth outBitDepth,
                     const std::string & id,
                     const Descriptions & desc,
                     const CDLOpData::Style & style,
                     const ChannelParams & slopeParams,
                     const ChannelParams & offsetParams,
                     const ChannelParams & powerParams,
                     const double saturation)
    :   OpData(inBitDepth, outBitDepth, id, desc)
    ,   m_style(style)
    ,   m_slopeParams(slopeParams)
    ,   m_offsetParams(offsetParams)
    ,   m_powerParams(powerParams)
    ,   m_saturation(saturation)
{
    validate();
}

CDLOpData::CDLOpData(BitDepth inBitDepth,
                     BitDepth outBitDepth,
                     const CDLOpData::Style & style,
                     const ChannelParams & slopeParams,
                     const ChannelParams & offsetParams,
                     const ChannelParams & powerParams,
                     const double saturation)
    :   OpData(inBitDepth, outBitDepth)
    ,   m_style(style)
    ,   m_slopeParams(slopeParams)
    ,   m_offsetParams(offsetParams)
    ,   m_powerParams(powerParams)
    ,   m_saturation(saturation)
{
    validate();
}

CDLOpData::~CDLOpData()
{
}

CDLOpDataRcPtr CDLOpData::clone() const
{
    return std::make_shared<CDLOpData>(*this);
}

bool CDLOpData::operator==(const OpData& other) const
{
    if (this == &other) return true;

    if (!OpData::operator==(other)) return false;

    const CDLOpData* cdl = static_cast<const CDLOpData*>(&other);

    return m_style        == cdl->m_style 
        && m_slopeParams  == cdl->m_slopeParams
        && m_offsetParams == cdl->m_offsetParams
        && m_powerParams  == cdl->m_powerParams
        && m_saturation   == cdl->m_saturation;
}

void CDLOpData::setStyle(const CDLOpData::Style & style)
{
    m_style = style;
}

void CDLOpData::setSlopeParams(const ChannelParams & slopeParams)
{
    m_slopeParams = slopeParams;
}

void CDLOpData::setOffsetParams(const ChannelParams & offsetParams)
{
    m_offsetParams = offsetParams;
}

void CDLOpData::setPowerParams(const ChannelParams & powerParams)
{
    m_powerParams = powerParams;
}

void CDLOpData::setSaturation(const double saturation)
{
    m_saturation = saturation;
}

// Validate if a parameter is greater than or equal to threshold value
void validateGreaterEqual(const char * name, 
                          const double value, 
                          const double threshold)
{
    if (!(value >= threshold))
    {
        std::ostringstream oss;
        oss << "CDL: Invalid '";
        oss << name;
        oss << "' " << value;
        oss << " should be greater than ";
        oss << threshold << ".";
        throw Exception(oss.str().c_str());
    }
}

// Validate if a parameter is greater than a threshold value
void validateGreaterThan(const char * name, 
                        const double value, 
                         const double threshold)
{ 
    if (!(value > threshold))
    {
        std::ostringstream oss;
        oss << "CDLOpData: Invalid '";
        oss << name;
        oss << "' " << value;
        oss << " should be greater than ";
        oss << threshold << ".";
        throw Exception(oss.str().c_str());
    }
}

typedef void(*parameter_validation_function)(const char *, 
                                             const double, 
                                             const double);

template<parameter_validation_function fnVal>
void validateChannelParams(const char * name, 
                           const CDLOpData::ChannelParams& params,
                           double threshold)
{
    for (unsigned i = 0; i < 3; ++i)
    {
        fnVal(name, params[i], threshold);
    }
}

// Validate number of SOP parameters and saturation
// The ASC v1.2 spec 2009-05-04 places the following restrictions:
//   slope >= 0, power > 0, sat >= 0, (offset unbounded).
void validateParams(const CDLOpData::ChannelParams& slopeParams,
                    const CDLOpData::ChannelParams& powerParams,
                    const double saturation)
{
    // slope >= 0
    validateChannelParams<validateGreaterEqual>("slope", slopeParams, 0.0);

    // power > 0
    validateChannelParams<validateGreaterThan>("power", powerParams, 0.0);

    // saturation >= 0
    validateGreaterEqual("saturation", saturation, 0.0);
}

bool CDLOpData::isNoOp() const
{
    return getInputBitDepth() == getOutputBitDepth()
        && isIdentity()
        && !isClamping();
}

bool CDLOpData::isIdentity() const
{
    return  m_slopeParams  == kOneParams  &&
            m_offsetParams == kZeroParams &&
            m_powerParams  == kOneParams  &&
            m_saturation   == 1.0;
}

OpDataRcPtr CDLOpData::getIdentityReplacement() const
{
    OpDataRcPtr op;
    switch(getStyle())
    {
        // These clamp values below 0 -- replace with range.
        case CDL_V1_2_FWD:
        case CDL_V1_2_REV:
        {
            op.reset(new RangeOpData(getInputBitDepth(),
                                     getOutputBitDepth(),
                                     0.,
                                     RangeOpData::EmptyValue(), // don't clamp high end
                                     0.,
                                     RangeOpData::EmptyValue()));
            break;
        }

        // These pass through the full range of values -- replace with matrix.
        case CDL_NO_CLAMP_FWD:
        case CDL_NO_CLAMP_REV:
        {
            op.reset(new MatrixOpData(getInputBitDepth(), getOutputBitDepth()));
            break;
        }
    }
    return op;
}

bool CDLOpData::hasChannelCrosstalk() const
{
    return m_saturation != 1.0;
}

void CDLOpData::validate() const
{
    OpData::validate();

    validateParams(m_slopeParams, m_powerParams, m_saturation);
}

std::string CDLOpData::getSlopeString() const
{
    return GetChannelParametersString(m_slopeParams);
}

std::string CDLOpData::getOffsetString() const
{
    return GetChannelParametersString(m_offsetParams);
}

std::string CDLOpData::getPowerString() const
{
    return GetChannelParametersString(m_powerParams);
}

std::string CDLOpData::getSaturationString() const
{
    std::ostringstream oss;
    oss.precision(DefaultValues::FLOAT_DECIMALS);
    oss << m_saturation;
    return oss.str();
}

bool CDLOpData::isReverse() const
{
    const CDLOpData::Style style = getStyle();
    switch (style)
    {
        case CDLOpData::CDL_V1_2_FWD:     return false;
        case CDLOpData::CDL_V1_2_REV:     return true;
        case CDLOpData::CDL_NO_CLAMP_FWD: return false;
        case CDLOpData::CDL_NO_CLAMP_REV: return true;
    }
    return false;
}

bool CDLOpData::isClamping() const
{
    const CDLOpData::Style style = getStyle();
    switch (style)
    {
        case CDLOpData::CDL_V1_2_FWD:     return true;
        case CDLOpData::CDL_V1_2_REV:     return true;
        case CDLOpData::CDL_NO_CLAMP_FWD: return false;
        case CDLOpData::CDL_NO_CLAMP_REV: return false;
    }
    return false;
}

std::string CDLOpData::GetChannelParametersString(ChannelParams params)
{
    std::ostringstream oss;
    oss.precision(DefaultValues::FLOAT_DECIMALS);
    oss << params[0] << ", " << params[1] << ", " << params[2];
    return oss.str();
}

bool CDLOpData::isInverse(ConstCDLOpDataRcPtr & r) const
{
    return *r == *inverse();
}

CDLOpDataRcPtr CDLOpData::inverse() const
{
    CDLOpDataRcPtr cdl = std::make_shared<CDLOpData>(getOutputBitDepth(),
                                                     getInputBitDepth(),
                                                     getStyle(),
                                                     getSlopeParams(),
                                                     getOffsetParams(),
                                                     getPowerParams(),
                                                     getSaturation());
    switch(cdl->getStyle())
    {
        case CDL_V1_2_FWD: cdl->setStyle(CDL_V1_2_REV); break;
        case CDL_V1_2_REV: cdl->setStyle(CDL_V1_2_FWD); break;
        case CDL_NO_CLAMP_FWD: cdl->setStyle(CDL_NO_CLAMP_REV); break;
        case CDL_NO_CLAMP_REV: cdl->setStyle(CDL_NO_CLAMP_FWD); break;
    }

    return cdl;
}

void CDLOpData::finalize()
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    cacheIDStream << getID() << " ";

    cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);

    cacheIDStream << GetStyleName(getStyle()) << " ";
    cacheIDStream << getSlopeString() << " ";
    cacheIDStream << getOffsetString() << " ";
    cacheIDStream << getPowerString() << " ";
    cacheIDStream << getSaturationString() << " ";

    m_cacheID = cacheIDStream.str();
}

}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(CDLOpData, accessors)
{
    OCIO::CDLOpData::ChannelParams slopeParams(1.35, 1.1, 0.71);
    OCIO::CDLOpData::ChannelParams offsetParams(0.05, -0.23, 0.11);
    OCIO::CDLOpData::ChannelParams powerParams(0.93, 0.81, 1.27);

    OCIO::CDLOpData cdlOp(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_F16,
                          OCIO::CDLOpData::CDL_V1_2_FWD,
                          slopeParams, offsetParams, powerParams, 1.23);

    // Update slope parameters with the same value
    OCIO::CDLOpData::ChannelParams newSlopeParams(0.66);
    cdlOp.setSlopeParams(newSlopeParams);

    OIIO_CHECK_ASSERT(cdlOp.getSlopeParams() == newSlopeParams);
    OIIO_CHECK_ASSERT(cdlOp.getOffsetParams() == offsetParams);
    OIIO_CHECK_ASSERT(cdlOp.getPowerParams() == powerParams);
    OIIO_CHECK_EQUAL(cdlOp.getSaturation(), 1.23);

    // Update offset parameters with the same value
    OCIO::CDLOpData::ChannelParams newOffsetParams(0.09);
    cdlOp.setOffsetParams(newOffsetParams);

    OIIO_CHECK_ASSERT(cdlOp.getSlopeParams() == newSlopeParams);
    OIIO_CHECK_ASSERT(cdlOp.getOffsetParams() == newOffsetParams);
    OIIO_CHECK_ASSERT(cdlOp.getPowerParams() == powerParams);
    OIIO_CHECK_EQUAL(cdlOp.getSaturation(), 1.23);

    // Update power parameters with the same value
    OCIO::CDLOpData::ChannelParams newPowerParams(1.1);
    cdlOp.setPowerParams(newPowerParams);

    OIIO_CHECK_ASSERT(cdlOp.getSlopeParams() == newSlopeParams);
    OIIO_CHECK_ASSERT(cdlOp.getOffsetParams() == newOffsetParams);
    OIIO_CHECK_ASSERT(cdlOp.getPowerParams() == newPowerParams);
    OIIO_CHECK_EQUAL(cdlOp.getSaturation(), 1.23);

    // Update the saturation parameter
    cdlOp.setSaturation(0.99);

    OIIO_CHECK_ASSERT(cdlOp.getSlopeParams() == newSlopeParams);
    OIIO_CHECK_ASSERT(cdlOp.getOffsetParams() == newOffsetParams);
    OIIO_CHECK_ASSERT(cdlOp.getPowerParams() == newPowerParams);
    OIIO_CHECK_EQUAL(cdlOp.getSaturation(), 0.99);

}

OIIO_ADD_TEST(CDLOpData, constructors)
{
    // Check default constructor
    OCIO::CDLOpData cdlOpDefault;

    OIIO_CHECK_EQUAL(cdlOpDefault.getType(), OCIO::CDLOpData::CDLType);
    OIIO_CHECK_EQUAL(cdlOpDefault.getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(cdlOpDefault.getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(cdlOpDefault.getID(), "");
    OIIO_CHECK_ASSERT(cdlOpDefault.getDescriptions().empty());

    OIIO_CHECK_EQUAL(cdlOpDefault.getStyle(),
                     OCIO::CDLOpData::CDL_V1_2_FWD);

    OIIO_CHECK_ASSERT(!cdlOpDefault.isReverse());

    OIIO_CHECK_ASSERT(cdlOpDefault.getSlopeParams()
        == OCIO::CDLOpData::ChannelParams(1.0));
    OIIO_CHECK_ASSERT(cdlOpDefault.getOffsetParams() 
        == OCIO::CDLOpData::ChannelParams(0.0));
    OIIO_CHECK_ASSERT(cdlOpDefault.getPowerParams()
        == OCIO::CDLOpData::ChannelParams(1.0));
    OIIO_CHECK_EQUAL(cdlOpDefault.getSaturation(), 1.0);

    // Check complete constructor
    OCIO::OpData::Descriptions descriptions;
    descriptions += "first_test_description";
    descriptions += "second_test_description";

    OCIO::CDLOpData cdlOpComplete(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT12,
                                  "test_id", descriptions,
                                  OCIO::CDLOpData::CDL_NO_CLAMP_REV,
                                  OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71),
                                  OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11),
                                  OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27),
                                  1.23);

    OIIO_CHECK_EQUAL(cdlOpComplete.getType(), OCIO::OpData::CDLType);
    OIIO_CHECK_EQUAL(cdlOpComplete.getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(cdlOpComplete.getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OIIO_CHECK_EQUAL(cdlOpComplete.getID(), "test_id");
    OIIO_CHECK_ASSERT(cdlOpComplete.getDescriptions() == descriptions);

    OIIO_CHECK_EQUAL(cdlOpComplete.getStyle(),
                     OCIO::CDLOpData::CDL_NO_CLAMP_REV);

    OIIO_CHECK_ASSERT(cdlOpComplete.isReverse());

    OIIO_CHECK_ASSERT(cdlOpComplete.getSlopeParams()
        == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));
    OIIO_CHECK_ASSERT(cdlOpComplete.getOffsetParams()
        == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));
    OIIO_CHECK_ASSERT(cdlOpComplete.getPowerParams()
        == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));
    OIIO_CHECK_EQUAL(cdlOpComplete.getSaturation(), 1.23);
}

OIIO_ADD_TEST(CDLOpData, inverse)
{
    OCIO::CDLOpData cdlOp(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT12,
                          "test_id", 
                          OCIO::OpData::Descriptions("inverse_op_test_description"),
                          OCIO::CDLOpData::CDL_V1_2_FWD,
                          OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71),
                          OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11),
                          OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27),
                          1.23);

    // Test CDL_V1_2_FWD inverse
    {
        cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_FWD);
        const OCIO::CDLOpDataRcPtr invOp = cdlOp.inverse();

        // Ensure bit depths are swapped
        OIIO_CHECK_EQUAL(invOp->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(invOp->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

        // Ensure id, name and descriptions are empty
        OIIO_CHECK_EQUAL(invOp->getID(), "");
        OIIO_CHECK_ASSERT(invOp->getDescriptions().empty());

        // Ensure style is inverted
        OIIO_CHECK_EQUAL(invOp->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);

        OIIO_CHECK_ASSERT(invOp->isReverse());

        // Ensure CDL parameters are unchanged
        OIIO_CHECK_ASSERT(invOp->getSlopeParams()
                          == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));

        OIIO_CHECK_ASSERT(invOp->getOffsetParams()
                         == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));

        OIIO_CHECK_ASSERT(invOp->getPowerParams()
                          == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));

        OIIO_CHECK_EQUAL(invOp->getSaturation(), 1.23);
    }

    // Test CDL_V1_2_REV inverse
    {
        cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_REV);
        const OCIO::CDLOpDataRcPtr invOp = cdlOp.inverse();

        // Ensure bit depths are swapped
        OIIO_CHECK_EQUAL(invOp->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(invOp->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

        // Ensure id, name and descriptions are empty
        OIIO_CHECK_EQUAL(invOp->getID(), "");
        OIIO_CHECK_ASSERT(invOp->getDescriptions().empty());

        // Ensure style is inverted
        OIIO_CHECK_EQUAL(invOp->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);

        OIIO_CHECK_EQUAL(invOp->isReverse(), false);

        // Ensure CDL parameters are unchanged
        OIIO_CHECK_ASSERT(invOp->getSlopeParams()
                          == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));

        OIIO_CHECK_ASSERT(invOp->getOffsetParams()
                          == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));

        OIIO_CHECK_ASSERT(invOp->getPowerParams()
                          == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));

        OIIO_CHECK_EQUAL(invOp->getSaturation(), 1.23);
    }

    // Test CDL_NO_CLAMP_FWD inverse
    {
        cdlOp.setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
        const OCIO::CDLOpDataRcPtr invOp = cdlOp.inverse();

        // Ensure bit depths are swapped
        OIIO_CHECK_EQUAL(invOp->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(invOp->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

        // Ensure id, name and descriptions are empty
        OIIO_CHECK_EQUAL(invOp->getID(), "");
        OIIO_CHECK_ASSERT(invOp->getDescriptions().empty());

        // Ensure style is inverted
        OIIO_CHECK_EQUAL(invOp->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);
        OIIO_CHECK_ASSERT(invOp->isReverse());

        // Ensure CDL parameters are unchanged
        OIIO_CHECK_ASSERT(invOp->getSlopeParams()
                           == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));

        OIIO_CHECK_ASSERT(invOp->getOffsetParams()
                           == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));

        OIIO_CHECK_ASSERT(invOp->getPowerParams()
                           == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));

        OIIO_CHECK_EQUAL(invOp->getSaturation(), 1.23);
    }

    // Test CDL_NO_CLAMP_REV inverse
    {
        cdlOp.setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_REV);
        const OCIO::CDLOpDataRcPtr invOp = cdlOp.inverse();

        // Ensure bit depths are swapped
        OIIO_CHECK_EQUAL(invOp->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(invOp->getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

        // Ensure id, name and descriptions are empty
        OIIO_CHECK_EQUAL(invOp->getID(), "");
        OIIO_CHECK_ASSERT(invOp->getDescriptions().empty());

        // Ensure style is inverted
        OIIO_CHECK_EQUAL(invOp->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
        OIIO_CHECK_ASSERT(!invOp->isReverse());

        // Ensure CDL parameters are unchanged
        OIIO_CHECK_ASSERT(invOp->getSlopeParams()
                          == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));

        OIIO_CHECK_ASSERT(invOp->getOffsetParams()
                          == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));

        OIIO_CHECK_ASSERT(invOp->getPowerParams()
                          == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));

        OIIO_CHECK_EQUAL(invOp->getSaturation(), 1.23);
    }
}


OIIO_ADD_TEST(CDLOpData, style)
{
    // Check default constructor
    OCIO::CDLOpData cdlOp;

    // Check CDL_V1_2_FWD
    cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_FWD);
    OIIO_CHECK_EQUAL(cdlOp.getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);
    OIIO_CHECK_ASSERT(!cdlOp.isReverse());

    // Check CDL_V1_2_REV
    cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_REV);
    OIIO_CHECK_EQUAL(cdlOp.getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);
    OIIO_CHECK_ASSERT(cdlOp.isReverse());

    // Check CDL_NO_CLAMP_FWD
    cdlOp.setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
    OIIO_CHECK_EQUAL(cdlOp.getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
    OIIO_CHECK_ASSERT(!cdlOp.isReverse());

    // Check CDL_NO_CLAMP_REV
    cdlOp.setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_REV);
    OIIO_CHECK_EQUAL(cdlOp.getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);
    OIIO_CHECK_ASSERT(cdlOp.isReverse());

    // Check unknown style
    OIIO_CHECK_THROW_WHAT(OCIO::CDLOpData::GetStyle("unknown_style"),
                          OCIO::Exception, 
                          "Unknown style for CDL");
}


OIIO_ADD_TEST(CDLOpData, validation_success)
{
    OCIO::CDLOpData cdlOp;

    // Set valid parameters
    const OCIO::CDLOpData::ChannelParams slopeParams(1.15);
    const OCIO::CDLOpData::ChannelParams offsetParams(-0.02);
    const OCIO::CDLOpData::ChannelParams powerParams(0.97);

    cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_FWD);

    cdlOp.setSlopeParams(slopeParams);
    cdlOp.setOffsetParams(offsetParams);
    cdlOp.setPowerParams(powerParams);
    cdlOp.setSaturation(1.22);

    OIIO_CHECK_ASSERT(!cdlOp.isIdentity());
    OIIO_CHECK_ASSERT(!cdlOp.isNoOp());

    OIIO_CHECK_NO_THROW(cdlOp.validate());

    // Set an identity operation
    cdlOp.setSlopeParams(OCIO::kOneParams);
    cdlOp.setOffsetParams(OCIO::kZeroParams);
    cdlOp.setPowerParams(OCIO::kOneParams);
    cdlOp.setSaturation(1.0);

    OIIO_CHECK_ASSERT(cdlOp.isIdentity());
    OIIO_CHECK_ASSERT(!cdlOp.isNoOp());
    // Set to non clamping
    cdlOp.setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
    OIIO_CHECK_ASSERT(cdlOp.isIdentity());
    OIIO_CHECK_ASSERT(cdlOp.isNoOp());

    OIIO_CHECK_NO_THROW(cdlOp.validate());

    // Check for slope = 0
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(0.0));
    cdlOp.setOffsetParams(offsetParams);
    cdlOp.setPowerParams(powerParams);
    cdlOp.setSaturation(1.0);

    cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_FWD);

    OIIO_CHECK_ASSERT(!cdlOp.isIdentity());
    OIIO_CHECK_ASSERT(!cdlOp.isNoOp());

    OIIO_CHECK_NO_THROW(cdlOp.validate());

    // Check for saturation = 0
    cdlOp.setSlopeParams(slopeParams);
    cdlOp.setOffsetParams(offsetParams);
    cdlOp.setPowerParams(powerParams);
    cdlOp.setSaturation(0.0);

    OIIO_CHECK_ASSERT(!cdlOp.isIdentity());
    OIIO_CHECK_ASSERT(!cdlOp.isNoOp());

    OIIO_CHECK_NO_THROW(cdlOp.validate());
}

OIIO_ADD_TEST(CDLOpData, validation_failure)
{
    OCIO::CDLOpData cdlOp;

    // Fail: invalid scale
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(-0.9));
    cdlOp.setOffsetParams(OCIO::CDLOpData::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::CDLOpData::ChannelParams(1.2));
    cdlOp.setSaturation(1.17);

    OIIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");

    // Fail: invalid power
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(0.9));
    cdlOp.setOffsetParams(OCIO::CDLOpData::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::CDLOpData::ChannelParams(-1.2));
    cdlOp.setSaturation(1.17);

    OIIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");

    // Fail: invalid saturation
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(0.9));
    cdlOp.setOffsetParams(OCIO::CDLOpData::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::CDLOpData::ChannelParams(1.2));
    cdlOp.setSaturation(-1.17);

    OIIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");

    // Check for power = 0
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(0.7));
    cdlOp.setOffsetParams(OCIO::CDLOpData::ChannelParams(0.2));
    cdlOp.setPowerParams(OCIO::CDLOpData::ChannelParams(0.0));
    cdlOp.setSaturation(1.4);

    OIIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");
}

// TODO: CDLOp_inverse_bypass_test is missing

OIIO_ADD_TEST(CDLOpData, channel)
{
  {
    OCIO::CDLOpData cdlOp;

    // False: identity
    OIIO_CHECK_ASSERT(!cdlOp.hasChannelCrosstalk());
  }

  {
    OCIO::CDLOpData cdlOp;
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(-0.9));
    cdlOp.setOffsetParams(OCIO::CDLOpData::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::CDLOpData::ChannelParams(1.2));

    // False: slope, offset, and power 
    OIIO_CHECK_ASSERT(!cdlOp.hasChannelCrosstalk());
  }

  {
    OCIO::CDLOpData cdlOp;
    cdlOp.setSaturation(1.17);

    // True: saturation
    OIIO_CHECK_ASSERT(cdlOp.hasChannelCrosstalk());
  }
}

#endif
