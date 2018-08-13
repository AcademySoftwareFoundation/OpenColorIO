/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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



#include <OpenColorIO/OpenColorIO.h>

#include "OpDataCDL.h"

#include "OpDataRange.h"
#include "OpDataMatrix.h"

#include "../Platform.h"

#include <sstream>



OCIO_NAMESPACE_ENTER
{
    namespace OpData
    {
        static const CDL::ChannelParams kOneParams(1.0);
        static const CDL::ChannelParams kZeroParams(0.0);

        static const char V1_2_FWD_NAME[] = "v1.2_Fwd";
        static const char V1_2_REV_NAME[] = "v1.2_Rev";
        static const char NO_CLAMP_FWD_NAME[] = "noClampFwd";
        static const char NO_CLAMP_REV_NAME[] = "noClampRev";

        CDL::CDLStyle CDL::GetCDLStyle(const char* name)
        {

// Get the style enum  of the CDL from the name stored
// in the "name" variable.
#define RETURN_STYLE_FROM_NAME(CDL_STYLE_NAME, CDL_STYLE)      \
    if( 0==Platform::Strcasecmp(name, CDL_STYLE_NAME) )        \
    {                                                          \
        return CDL_STYLE;                                      \
    }

            if (name && *name)
            {
                RETURN_STYLE_FROM_NAME(V1_2_FWD_NAME, CDL_V1_2_FWD);
                RETURN_STYLE_FROM_NAME(V1_2_REV_NAME, CDL_V1_2_REV);
                RETURN_STYLE_FROM_NAME(NO_CLAMP_FWD_NAME, CDL_NO_CLAMP_FWD);
                RETURN_STYLE_FROM_NAME(NO_CLAMP_REV_NAME, CDL_NO_CLAMP_REV);
            }

#undef RETURN_STYLE_FROM_NAME

            throw Exception("Unknown style for CDL.");
        }

        const char* CDL::GetCDLStyleName(CDL::CDLStyle style)
        {
            // Get the name of the CDL style from the enum stored
            // in the "style" variable
            switch (style)
            {
                case CDL_V1_2_FWD:     return V1_2_FWD_NAME;
                case CDL_V1_2_REV:     return V1_2_REV_NAME;
                case CDL_NO_CLAMP_FWD: return NO_CLAMP_FWD_NAME;
                case CDL_NO_CLAMP_REV: return NO_CLAMP_REV_NAME;
            }

            throw Exception("Unknown style for CDL.");

            return 0x0;
        }

        CDL::CDL()
            :   OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
            ,   m_cdlStyle(GetDefaultStyle())
            ,   m_slopeParams(1.0)
            ,   m_offsetParams(0.0)
            ,   m_powerParams(1.0)
            ,   m_saturation(1.0)
        {
        }

        CDL::CDL(BitDepth inBitDepth,
                 BitDepth outBitDepth,
                 const std::string& id,
                 const std::string& name,
                 const Descriptions& descriptions,
                 const CDL::CDLStyle& style,
                 const ChannelParams& slopeParams,
                 const ChannelParams& offsetParams,
                 const ChannelParams& powerParams,
                 const double saturation)
            :   OpData(inBitDepth, outBitDepth, id, name, descriptions)
            ,   m_cdlStyle(style)
            ,   m_slopeParams(slopeParams)
            ,   m_offsetParams(offsetParams)
            ,   m_powerParams(powerParams)
            ,   m_saturation(saturation)
        {
            validate();
        }

        CDL::CDL(BitDepth inBitDepth,
                 BitDepth outBitDepth,
                 const CDL::CDLStyle& style,
                 const ChannelParams& slopeParams,
                 const ChannelParams& offsetParams,
                 const ChannelParams& powerParams,
                 const double saturation)
            :   OpData(inBitDepth, outBitDepth)
            ,   m_cdlStyle(style)
            ,   m_slopeParams(slopeParams)
            ,   m_offsetParams(offsetParams)
            ,   m_powerParams(powerParams)
            ,   m_saturation(saturation)
        {
            validate();
        }

        CDL::~CDL()
        {
        }

        const std::string& CDL::getOpTypeName() const
        {
            static const std::string type("ASC CDL");
            return type;
        }

        bool CDL::operator==(const OpData& other) const
        {
            if (this == &other) return true;
            if (getOpType() != other.getOpType()) return false;

            const CDL* cdl = static_cast<const CDL*>(&other);

            return OpData::operator==(other) 
                && m_cdlStyle     == cdl->m_cdlStyle 
                && m_slopeParams  == cdl->m_slopeParams
                && m_offsetParams == cdl->m_offsetParams
                && m_powerParams  == cdl->m_powerParams
                && m_saturation   == cdl->m_saturation;
        }

        void CDL::setCDLStyle(const CDL::CDLStyle& style)
        {
            m_cdlStyle = style;
        }

        void CDL::setSlopeParams(const ChannelParams& slopeParams)
        {
            m_slopeParams = slopeParams;
        }

        void CDL::setOffsetParams(const ChannelParams& offsetParams)
        {
            m_offsetParams = offsetParams;
        }

        void CDL::setPowerParams(const ChannelParams& powerParams)
        {
            m_powerParams = powerParams;
        }

        void CDL::setSaturation(const double saturation)
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
                oss << "CDL: Invalid '";
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
                                   const CDL::ChannelParams& params,
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
        void validateParams(const CDL::ChannelParams& slopeParams,
            const CDL::ChannelParams& powerParams,
            const double saturation)
        {
            // slope >= 0
            validateChannelParams<validateGreaterEqual>("slope", slopeParams, 0.0);

            // power > 0
            validateChannelParams<validateGreaterThan>("power", powerParams, 0.0);

            // saturation >= 0
            validateGreaterEqual("saturation", saturation, 0.0);
        }

        bool CDL::isIdentity() const
        {
            return  m_slopeParams == kOneParams &&
                    m_offsetParams == kZeroParams &&
                    m_powerParams == kOneParams &&
                    m_saturation == 1.0;
        }

        std::auto_ptr<OpData> CDL::getIdentityReplacement() const
        {
            std::auto_ptr<OpData> op;
            switch(getCDLStyle())
            {
                // These clamp values below 0 -- replace with range.
                case CDL_V1_2_FWD:
                case CDL_V1_2_REV:
                {
                    op.reset(new Range(getInputBitDepth(),
                                       getOutputBitDepth(),
                                       0.,
                                       Range::EmptyValue(), // don't clamp high end
                                       0.,
                                       Range::EmptyValue()));
                    break;
                }

                // These pass through the full range of values -- replace with matrix.
                case CDL_NO_CLAMP_FWD:
                case CDL_NO_CLAMP_REV:
                {
                    op.reset(new Matrix(getInputBitDepth(), getOutputBitDepth()));
                    break;
                }
            }
            return op;
        }

        bool CDL::hasChannelCrosstalk() const
        {
            return m_saturation != 1.0;
        }

        void CDL::validate() const
        {
            OpData::validate();

            validateParams(m_slopeParams, m_powerParams, m_saturation);
        }

        std::string CDL::getSlopeString() const
        {
            return GetChannelParametersString(m_slopeParams);
        }

        std::string CDL::getOffsetString() const
        {
            return GetChannelParametersString(m_offsetParams);
        }

        std::string CDL::getPowerString() const
        {
            return GetChannelParametersString(m_powerParams);
        }

        std::string CDL::getSaturationString() const
        {
            std::ostringstream oss;
            oss << m_saturation;
            return oss.str();
        }

        bool CDL::isReverse() const
        {
            // Return a boolean status based on the enum stored in the "style" variable.
            const CDL::CDLStyle style = getCDLStyle();
            switch (style)
            {
                case CDL::CDL_V1_2_FWD:     return false;
                case CDL::CDL_V1_2_REV:     return true;
                case CDL::CDL_NO_CLAMP_FWD: return false;
                case CDL::CDL_NO_CLAMP_REV: return true;
            }
            return false;
        }

        bool CDL::isClamping() const
        {
            // Return a boolean status based on the enum stored in the "style" variable.
            const CDL::CDLStyle style = getCDLStyle();
            switch (style)
            {
                case CDL::CDL_V1_2_FWD:     return true;
                case CDL::CDL_V1_2_REV:     return true;
                case CDL::CDL_NO_CLAMP_FWD: return false;
                case CDL::CDL_NO_CLAMP_REV: return false;
            }
            return false;
        }

        std::string CDL::GetChannelParametersString(ChannelParams params)
        {
            std::ostringstream oss;
            oss << params[0] << ", " << params[1] << ", " << params[2];
            return oss.str();
        }

        OpData * CDL::clone(CloneType) const
        {
            return new CDL(*this);
        }

        void CDL::inverse(OpDataVec & v) const
        {
            std::auto_ptr<CDL> cdl(new CDL(getOutputBitDepth(),
                                           getInputBitDepth(),
                                           getCDLStyle(),
                                           getSlopeParams(),
                                           getOffsetParams(),
                                           getPowerParams(),
                                           getSaturation()));
            switch(cdl->getCDLStyle())
            {
                case CDL_V1_2_FWD: cdl->setCDLStyle(CDL_V1_2_REV); break;
                case CDL_V1_2_REV: cdl->setCDLStyle(CDL_V1_2_FWD); break;
                case CDL_NO_CLAMP_FWD: cdl->setCDLStyle(CDL_NO_CLAMP_REV); break;
                case CDL_NO_CLAMP_REV: cdl->setCDLStyle(CDL_NO_CLAMP_FWD); break;
            }
            v.append(cdl.release());
        }

        bool CDL::isInverse(const OpDataCDLRcPtr & /*r*/) const
        {
            // TODO: To be implemented
            return false;
        }

        OpDataCDLRcPtr CDL::compose(const OpDataCDLRcPtr & /*r*/) const
        {
            // TODO: To be implemented
            return OpDataCDLRcPtr(new CDL);
        }

    } // exit OpData namespace
}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"

OIIO_ADD_TEST(OpDataCDL, Accessors)
{
    OCIO::OpData::CDL::ChannelParams slopeParams(1.35, 1.1, 0.71);
    OCIO::OpData::CDL::ChannelParams offsetParams(0.05, -0.23, 0.11);
    OCIO::OpData::CDL::ChannelParams powerParams(0.93, 0.81, 1.27);

    OCIO::OpData::CDL cdlOp(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_F16,
        "", "", OCIO::OpData::Descriptions(),
        OCIO::OpData::CDL::CDL_V1_2_FWD,
        slopeParams, offsetParams, powerParams, 1.23);

    // Update slope parameters with the same value
    OCIO::OpData::CDL::ChannelParams newSlopeParams(0.66);
    cdlOp.setSlopeParams(newSlopeParams);

    OIIO_CHECK_ASSERT(cdlOp.getSlopeParams() == newSlopeParams);
    OIIO_CHECK_ASSERT(cdlOp.getOffsetParams() == offsetParams);
    OIIO_CHECK_ASSERT(cdlOp.getPowerParams() == powerParams);
    OIIO_CHECK_EQUAL(cdlOp.getSaturation(), 1.23);

    // Update offset parameters with the same value
    OCIO::OpData::CDL::ChannelParams newOffsetParams(0.09);
    cdlOp.setOffsetParams(newOffsetParams);

    OIIO_CHECK_ASSERT(cdlOp.getSlopeParams() == newSlopeParams);
    OIIO_CHECK_ASSERT(cdlOp.getOffsetParams() == newOffsetParams);
    OIIO_CHECK_ASSERT(cdlOp.getPowerParams() == powerParams);
    OIIO_CHECK_EQUAL(cdlOp.getSaturation(), 1.23);

    // Update power parameters with the same value
    OCIO::OpData::CDL::ChannelParams newPowerParams(1.1);
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

OIIO_ADD_TEST(OpDataCDL, Constructors)
{
    // Check default constructor
    OCIO::OpData::CDL cdlOpDefault;

    OIIO_CHECK_EQUAL(cdlOpDefault.getOpType(), OCIO::OpData::OpData::CDLType);
    OIIO_CHECK_EQUAL(cdlOpDefault.getOpTypeName(), "ASC CDL");
    OIIO_CHECK_EQUAL(cdlOpDefault.getInputBitDepth(), OCIO::BIT_DEPTH_F32);
    OIIO_CHECK_EQUAL(cdlOpDefault.getOutputBitDepth(), OCIO::BIT_DEPTH_F32);

    OIIO_CHECK_EQUAL(cdlOpDefault.getId(), "");
    OIIO_CHECK_EQUAL(cdlOpDefault.getName(), "");
    OIIO_CHECK_EQUAL(cdlOpDefault.getDescriptions().getList().size(), 0u);

    OIIO_CHECK_EQUAL(cdlOpDefault.getCDLStyle(),
        OCIO::OpData::CDL::CDL_V1_2_FWD);

    OIIO_CHECK_EQUAL(cdlOpDefault.isReverse(), false);

    OIIO_CHECK_ASSERT(cdlOpDefault.getSlopeParams() ==
        OCIO::OpData::CDL::ChannelParams(1.0));
    OIIO_CHECK_ASSERT(cdlOpDefault.getOffsetParams() ==
        OCIO::OpData::CDL::ChannelParams(0.0));
    OIIO_CHECK_ASSERT(cdlOpDefault.getPowerParams() ==
        OCIO::OpData::CDL::ChannelParams(1.0));
    OIIO_CHECK_EQUAL(cdlOpDefault.getSaturation(), 1.0);

    // Check complete constructor
    OCIO::OpData::Descriptions descriptions;
    descriptions += "first_test_description";
    descriptions += "second_test_description";

    OCIO::OpData::CDL cdlOpComplete(
        OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT12,
        "test_id", "test_name", descriptions,
        OCIO::OpData::CDL::CDL_NO_CLAMP_REV,
        OCIO::OpData::CDL::ChannelParams(1.35, 1.1, 0.71),
        OCIO::OpData::CDL::ChannelParams(0.05, -0.23, 0.11),
        OCIO::OpData::CDL::ChannelParams(0.93, 0.81, 1.27),
        1.23);

    OIIO_CHECK_EQUAL(cdlOpComplete.getOpType(), OCIO::OpData::OpData::CDLType);
    OIIO_CHECK_EQUAL(cdlOpComplete.getOpTypeName(), "ASC CDL");
    OIIO_CHECK_EQUAL(cdlOpComplete.getInputBitDepth(), OCIO::BIT_DEPTH_F16);
    OIIO_CHECK_EQUAL(cdlOpComplete.getOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OIIO_CHECK_EQUAL(cdlOpComplete.getId(), "test_id");
    OIIO_CHECK_EQUAL(cdlOpComplete.getName(), "test_name");

    OIIO_CHECK_ASSERT(cdlOpComplete.getDescriptions().getList().size() == 2);
    OIIO_CHECK_EQUAL(cdlOpComplete.getDescriptions().getList()[0],
        "first_test_description");
    OIIO_CHECK_EQUAL(cdlOpComplete.getDescriptions().getList()[1],
        "second_test_description");

    OIIO_CHECK_EQUAL(cdlOpComplete.getCDLStyle(),
        OCIO::OpData::CDL::CDL_NO_CLAMP_REV);

    OIIO_CHECK_EQUAL(cdlOpComplete.isReverse(), true);

    OIIO_CHECK_ASSERT(cdlOpComplete.getSlopeParams() ==
        OCIO::OpData::CDL::ChannelParams(1.35, 1.1, 0.71));
    OIIO_CHECK_ASSERT(cdlOpComplete.getOffsetParams() ==
        OCIO::OpData::CDL::ChannelParams(0.05, -0.23, 0.11));
    OIIO_CHECK_ASSERT(cdlOpComplete.getPowerParams() ==
        OCIO::OpData::CDL::ChannelParams(0.93, 0.81, 1.27));
    OIIO_CHECK_EQUAL(cdlOpComplete.getSaturation(), 1.23);
}


OIIO_ADD_TEST(OpDataCDL, Inverse)
{
    OCIO::OpData::Descriptions descriptions;
    descriptions += "inverse_op_test_description";

    OCIO::OpData::CDL cdlOp(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT12,
                            "test_id", "test_name", descriptions,
                            OCIO::OpData::CDL::CDL_V1_2_FWD,
                            OCIO::OpData::CDL::ChannelParams(1.35, 1.1, 0.71),
                            OCIO::OpData::CDL::ChannelParams(0.05, -0.23, 0.11),
                            OCIO::OpData::CDL::ChannelParams(0.93, 0.81, 1.27),
                            1.23);

    // Test CDL_V1_2_FWD inverse
    {
        OCIO::OpData::OpDataVec invOps;

        cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_V1_2_FWD);
        cdlOp.inverse(invOps);

        const OCIO::OpData::CDL * pInvOp 
            = dynamic_cast<const OCIO::OpData::CDL*>( invOps.get(0) );
        OIIO_CHECK_ASSERT(pInvOp != 0x0);

        const OCIO::OpData::CDL & invOp = *pInvOp;

        // Ensure bit depths are swapped
        OIIO_CHECK_EQUAL(invOp.getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(invOp.getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

        // Ensure id, name and descriptions are empty
        OIIO_CHECK_EQUAL(invOp.getId(), "");
        OIIO_CHECK_EQUAL(invOp.getName(), "");
        OIIO_CHECK_EQUAL(invOp.getDescriptions().getList().size(), 0u);

        // Ensure style is inverted
        OIIO_CHECK_EQUAL(invOp.getCDLStyle(), OCIO::OpData::CDL::CDL_V1_2_REV);

        OIIO_CHECK_EQUAL(invOp.isReverse(), true);

        // Ensure CDL parameters are unchanged
        OIIO_CHECK_ASSERT(invOp.getSlopeParams()
                          == OCIO::OpData::CDL::ChannelParams(1.35, 1.1, 0.71));

        OIIO_CHECK_ASSERT(invOp.getOffsetParams()
                         == OCIO::OpData::CDL::ChannelParams(0.05, -0.23, 0.11));

        OIIO_CHECK_ASSERT(invOp.getPowerParams()
                          == OCIO::OpData::CDL::ChannelParams(0.93, 0.81, 1.27));

        OIIO_CHECK_EQUAL(invOp.getSaturation(), 1.23);
    }

    // Test CDL_V1_2_REV inverse
    {
        OCIO::OpData::OpDataVec invOps;

        cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_V1_2_REV);
        cdlOp.inverse(invOps);

        const OCIO::OpData::CDL * pInvOp 
            = dynamic_cast<const OCIO::OpData::CDL*>( invOps.get(0) );
        OIIO_CHECK_ASSERT(pInvOp != 0x0);

        const OCIO::OpData::CDL & invOp = *pInvOp;

        // Ensure bit depths are swapped
        OIIO_CHECK_EQUAL(invOp.getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(invOp.getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

        // Ensure id, name and descriptions are empty
        OIIO_CHECK_EQUAL(invOp.getId(), "");
        OIIO_CHECK_EQUAL(invOp.getName(), "");
        OIIO_CHECK_EQUAL(invOp.getDescriptions().getList().size(), 0u);

        // Ensure style is inverted
        OIIO_CHECK_EQUAL(invOp.getCDLStyle(), OCIO::OpData::CDL::CDL_V1_2_FWD);

        OIIO_CHECK_EQUAL(invOp.isReverse(), false);

        // Ensure CDL parameters are unchanged
        OIIO_CHECK_ASSERT(invOp.getSlopeParams()
                          == OCIO::OpData::CDL::ChannelParams(1.35, 1.1, 0.71));

        OIIO_CHECK_ASSERT(invOp.getOffsetParams()
                          == OCIO::OpData::CDL::ChannelParams(0.05, -0.23, 0.11));

        OIIO_CHECK_ASSERT(invOp.getPowerParams()
                          == OCIO::OpData::CDL::ChannelParams(0.93, 0.81, 1.27));

        OIIO_CHECK_EQUAL(invOp.getSaturation(), 1.23);
    }

    // Test CDL_NO_CLAMP_FWD inverse
    {
        OCIO::OpData::OpDataVec invOps;

        cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_NO_CLAMP_FWD);
        cdlOp.inverse(invOps);

        const OCIO::OpData::CDL * pInvOp 
            = dynamic_cast<const OCIO::OpData::CDL *>( invOps.get(0) );
        OIIO_CHECK_ASSERT(pInvOp != 0x0);

        const OCIO::OpData::CDL & invOp = *pInvOp;

        // Ensure bit depths are swapped
        OIIO_CHECK_EQUAL(invOp.getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(invOp.getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

        // Ensure id, name and descriptions are empty
        OIIO_CHECK_EQUAL(invOp.getId(), "");
        OIIO_CHECK_EQUAL(invOp.getName(), "");
        OIIO_CHECK_EQUAL(invOp.getDescriptions().getList().size(), 0u);

        // Ensure style is inverted
        OIIO_CHECK_EQUAL(invOp.getCDLStyle(), OCIO::OpData::CDL::CDL_NO_CLAMP_REV);

        OIIO_CHECK_ASSERT(invOp.isReverse());

        // Ensure CDL parameters are unchanged
        OIIO_CHECK_ASSERT(invOp.getSlopeParams()
                           == OCIO::OpData::CDL::ChannelParams(1.35, 1.1, 0.71));

        OIIO_CHECK_ASSERT(invOp.getOffsetParams()
                           == OCIO::OpData::CDL::ChannelParams(0.05, -0.23, 0.11));

        OIIO_CHECK_ASSERT(invOp.getPowerParams()
                           == OCIO::OpData::CDL::ChannelParams(0.93, 0.81, 1.27));

        OIIO_CHECK_EQUAL(invOp.getSaturation(), 1.23);
    }

    // Test CDL_NO_CLAMP_REV inverse
    {
        OCIO::OpData::OpDataVec invOps;

        cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_NO_CLAMP_REV);
        cdlOp.inverse(invOps);

        const OCIO::OpData::CDL * pInvOp 
            = dynamic_cast<const OCIO::OpData::CDL *>( invOps.get(0) );
        OIIO_CHECK_ASSERT(pInvOp != 0x0);

        const OCIO::OpData::CDL & invOp = *pInvOp;

        // Ensure bit depths are swapped
        OIIO_CHECK_EQUAL(invOp.getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
        OIIO_CHECK_EQUAL(invOp.getOutputBitDepth(), OCIO::BIT_DEPTH_F16);

        // Ensure id, name and descriptions are empty
        OIIO_CHECK_EQUAL(invOp.getId(), "");
        OIIO_CHECK_EQUAL(invOp.getName(), "");
        OIIO_CHECK_EQUAL(invOp.getDescriptions().getList().size(), 0u);

        // Ensure style is inverted
        OIIO_CHECK_EQUAL(invOp.getCDLStyle(), OCIO::OpData::CDL::CDL_NO_CLAMP_FWD);

        OIIO_CHECK_ASSERT(!invOp.isReverse());

        // Ensure CDL parameters are unchanged
        OIIO_CHECK_ASSERT(invOp.getSlopeParams()
                          == OCIO::OpData::CDL::ChannelParams(1.35, 1.1, 0.71));

        OIIO_CHECK_ASSERT(invOp.getOffsetParams()
                          == OCIO::OpData::CDL::ChannelParams(0.05, -0.23, 0.11));

        OIIO_CHECK_ASSERT(invOp.getPowerParams()
                          == OCIO::OpData::CDL::ChannelParams(0.93, 0.81, 1.27));

        OIIO_CHECK_EQUAL(invOp.getSaturation(), 1.23);
    }
}


OIIO_ADD_TEST(OpDataCDL, Style)
{
    // Check default constructor
    OCIO::OpData::CDL cdlOp;

    // Check CDL_V1_2_FWD
    cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_V1_2_FWD);
    OIIO_CHECK_EQUAL(cdlOp.getCDLStyle(), OCIO::OpData::CDL::CDL_V1_2_FWD);
    OIIO_CHECK_ASSERT(!cdlOp.isReverse());

    // Check CDL_V1_2_REV
    cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_V1_2_REV);
    OIIO_CHECK_EQUAL(cdlOp.getCDLStyle(), OCIO::OpData::CDL::CDL_V1_2_REV);
    OIIO_CHECK_ASSERT(cdlOp.isReverse());

    // Check CDL_NO_CLAMP_FWD
    cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_NO_CLAMP_FWD);
    OIIO_CHECK_EQUAL(cdlOp.getCDLStyle(), OCIO::OpData::CDL::CDL_NO_CLAMP_FWD);
    OIIO_CHECK_ASSERT(!cdlOp.isReverse());

    // Check CDL_NO_CLAMP_REV
    cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_NO_CLAMP_REV);
    OIIO_CHECK_EQUAL(cdlOp.getCDLStyle(), OCIO::OpData::CDL::CDL_NO_CLAMP_REV);
    OIIO_CHECK_ASSERT(cdlOp.isReverse());

    // Check unknown style
    OIIO_CHECK_THROW_WHAT(OCIO::OpData::CDL::GetCDLStyle("unknown_style"),
                          OCIO::Exception, 
                          "Unknown style for CDL");
}


OIIO_ADD_TEST(OpDataCDL, ValidationSuccess)
{
    OCIO::OpData::CDL cdlOp;

    // Set valid parameters
    OCIO::OpData::CDL::ChannelParams slopeParams(1.15);
    OCIO::OpData::CDL::ChannelParams offsetParams(-0.02);
    OCIO::OpData::CDL::ChannelParams powerParams(0.97);

    cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_V1_2_FWD);

    cdlOp.setSlopeParams(slopeParams);
    cdlOp.setOffsetParams(offsetParams);
    cdlOp.setPowerParams(powerParams);
    cdlOp.setSaturation(1.22);

    OIIO_CHECK_ASSERT(!cdlOp.isIdentity());
    OIIO_CHECK_ASSERT(!cdlOp.isNoOp());

    OIIO_CHECK_NO_THROW(cdlOp.validate());

    // Set an identity operation
    OCIO::OpData::CDL::ChannelParams identitySlopeParams(1.0);
    OCIO::OpData::CDL::ChannelParams identityOffsetParams(0.0);
    OCIO::OpData::CDL::ChannelParams identityPowerParams(1.0);

    cdlOp.setSlopeParams(identitySlopeParams);
    cdlOp.setOffsetParams(identityOffsetParams);
    cdlOp.setPowerParams(identityPowerParams);
    cdlOp.setSaturation(1.0);

    OIIO_CHECK_ASSERT(cdlOp.isIdentity());
    OIIO_CHECK_ASSERT(!cdlOp.isNoOp());
    // Set to non clamping
    cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_NO_CLAMP_FWD);
    OIIO_CHECK_ASSERT(cdlOp.isNoOp());

    OIIO_CHECK_NO_THROW(cdlOp.validate());

    // Check for slope = 0
    cdlOp.setSlopeParams(OCIO::OpData::CDL::ChannelParams(0.0));
    cdlOp.setOffsetParams(offsetParams);
    cdlOp.setPowerParams(powerParams);
    cdlOp.setSaturation(1.0);

    cdlOp.setCDLStyle(OCIO::OpData::CDL::CDL_V1_2_FWD);

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

OIIO_ADD_TEST(OpDataCDL, ValidationFailure)
{
    OCIO::OpData::CDL cdlOp;

    // Fail: invalid scale
    cdlOp.setSlopeParams(OCIO::OpData::CDL::ChannelParams(-0.9));
    cdlOp.setOffsetParams(OCIO::OpData::CDL::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::OpData::CDL::ChannelParams(1.2));
    cdlOp.setSaturation(1.17);

    OIIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");

    // Fail: invalid power
    cdlOp.setSlopeParams(OCIO::OpData::CDL::ChannelParams(0.9));
    cdlOp.setOffsetParams(OCIO::OpData::CDL::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::OpData::CDL::ChannelParams(-1.2));
    cdlOp.setSaturation(1.17);

    OIIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");

    // Fail: invalid saturation
    cdlOp.setSlopeParams(OCIO::OpData::CDL::ChannelParams(0.9));
    cdlOp.setOffsetParams(OCIO::OpData::CDL::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::OpData::CDL::ChannelParams(1.2));
    cdlOp.setSaturation(-1.17);

    OIIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");

    // Check for power = 0
    cdlOp.setSlopeParams(OCIO::OpData::CDL::ChannelParams(0.7));
    cdlOp.setOffsetParams(OCIO::OpData::CDL::ChannelParams(0.2));
    cdlOp.setPowerParams(OCIO::OpData::CDL::ChannelParams(0.0));
    cdlOp.setSaturation(1.4);

    OIIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");
}

// TODO: CDLOp_inverse_bypass_test is missing

OIIO_ADD_TEST(OpDataCDL, Channel)
{
  {
    OCIO::OpData::CDL cdlOp;

    // False: identity
    OIIO_CHECK_ASSERT(!cdlOp.hasChannelCrosstalk());
  }

  {
    OCIO::OpData::CDL cdlOp;
    cdlOp.setSlopeParams(OCIO::OpData::CDL::ChannelParams(-0.9));
    cdlOp.setOffsetParams(OCIO::OpData::CDL::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::OpData::CDL::ChannelParams(1.2));

    // False: slope, offset, and power 
    OIIO_CHECK_ASSERT(!cdlOp.hasChannelCrosstalk());
  }

  {
    OCIO::OpData::CDL cdlOp;
    cdlOp.setSaturation(1.17);

    // True: saturation
    OIIO_CHECK_ASSERT(cdlOp.hasChannelCrosstalk());
  }
}

#endif

