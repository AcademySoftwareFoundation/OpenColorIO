// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/cdl/CDLOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(CDLOpData, accessors)
{
    OCIO::CDLOpData::ChannelParams slopeParams(1.35, 1.1, 0.71);
    OCIO::CDLOpData::ChannelParams offsetParams(0.05, -0.23, 0.11);
    OCIO::CDLOpData::ChannelParams powerParams(0.93, 0.81, 1.27);

    OCIO::CDLOpData cdlOp(OCIO::CDLOpData::CDL_V1_2_FWD,
                          slopeParams, offsetParams, powerParams, 1.23);

    // Update slope parameters with the same value.
    OCIO::CDLOpData::ChannelParams newSlopeParams(0.66);
    cdlOp.setSlopeParams(newSlopeParams);

    OCIO_CHECK_ASSERT(cdlOp.getSlopeParams() == newSlopeParams);
    OCIO_CHECK_ASSERT(cdlOp.getOffsetParams() == offsetParams);
    OCIO_CHECK_ASSERT(cdlOp.getPowerParams() == powerParams);
    OCIO_CHECK_EQUAL(cdlOp.getSaturation(), 1.23);

    // Update offset parameters with the same value.
    OCIO::CDLOpData::ChannelParams newOffsetParams(0.09);
    cdlOp.setOffsetParams(newOffsetParams);

    OCIO_CHECK_ASSERT(cdlOp.getSlopeParams() == newSlopeParams);
    OCIO_CHECK_ASSERT(cdlOp.getOffsetParams() == newOffsetParams);
    OCIO_CHECK_ASSERT(cdlOp.getPowerParams() == powerParams);
    OCIO_CHECK_EQUAL(cdlOp.getSaturation(), 1.23);

    // Update power parameters with the same value.
    OCIO::CDLOpData::ChannelParams newPowerParams(1.1);
    cdlOp.setPowerParams(newPowerParams);

    OCIO_CHECK_ASSERT(cdlOp.getSlopeParams() == newSlopeParams);
    OCIO_CHECK_ASSERT(cdlOp.getOffsetParams() == newOffsetParams);
    OCIO_CHECK_ASSERT(cdlOp.getPowerParams() == newPowerParams);
    OCIO_CHECK_EQUAL(cdlOp.getSaturation(), 1.23);

    // Update the saturation parameter.
    cdlOp.setSaturation(0.99);

    OCIO_CHECK_ASSERT(cdlOp.getSlopeParams() == newSlopeParams);
    OCIO_CHECK_ASSERT(cdlOp.getOffsetParams() == newOffsetParams);
    OCIO_CHECK_ASSERT(cdlOp.getPowerParams() == newPowerParams);
    OCIO_CHECK_EQUAL(cdlOp.getSaturation(), 0.99);

}

OCIO_ADD_TEST(CDLOpData, constructors)
{
    // Check default constructor.
    OCIO::CDLOpData cdlOpDefault;

    OCIO_CHECK_EQUAL(cdlOpDefault.getType(), OCIO::CDLOpData::CDLType);

    OCIO_CHECK_EQUAL(cdlOpDefault.getID(), "");
    OCIO_CHECK_ASSERT(cdlOpDefault.getFormatMetadata().getChildrenElements().empty());

    OCIO_CHECK_EQUAL(cdlOpDefault.getStyle(),
                     OCIO::CDLOpData::CDL_NO_CLAMP_FWD);

    OCIO_CHECK_ASSERT(!cdlOpDefault.isReverse());

    OCIO_CHECK_ASSERT(cdlOpDefault.getSlopeParams()
        == OCIO::CDLOpData::ChannelParams(1.0));
    OCIO_CHECK_ASSERT(cdlOpDefault.getOffsetParams() 
        == OCIO::CDLOpData::ChannelParams(0.0));
    OCIO_CHECK_ASSERT(cdlOpDefault.getPowerParams()
        == OCIO::CDLOpData::ChannelParams(1.0));
    OCIO_CHECK_EQUAL(cdlOpDefault.getSaturation(), 1.0);

    // Check complete constructor.
    OCIO::CDLOpData cdlOpComplete(OCIO::CDLOpData::CDL_NO_CLAMP_REV,
                                  OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71),
                                  OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11),
                                  OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27),
                                  1.23);

    auto & metadata = cdlOpComplete.getFormatMetadata();
    metadata.addAttribute(OCIO::METADATA_NAME, "cdl-name");
    metadata.addAttribute(OCIO::METADATA_ID, "cdl-id");

    OCIO_CHECK_EQUAL(cdlOpComplete.getName(), "cdl-name");
    OCIO_CHECK_EQUAL(cdlOpComplete.getID(), "cdl-id");

    OCIO_CHECK_EQUAL(cdlOpComplete.getType(), OCIO::OpData::CDLType);

    OCIO_CHECK_EQUAL(cdlOpComplete.getStyle(),
                     OCIO::CDLOpData::CDL_NO_CLAMP_REV);

    OCIO_CHECK_ASSERT(cdlOpComplete.isReverse());

    OCIO_CHECK_ASSERT(cdlOpComplete.getSlopeParams()
        == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));
    OCIO_CHECK_ASSERT(cdlOpComplete.getOffsetParams()
        == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));
    OCIO_CHECK_ASSERT(cdlOpComplete.getPowerParams()
        == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));
    OCIO_CHECK_EQUAL(cdlOpComplete.getSaturation(), 1.23);
}

OCIO_ADD_TEST(CDLOpData, inverse)
{
    OCIO::CDLOpData cdlOp(OCIO::CDLOpData::CDL_V1_2_FWD,
                          OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71),
                          OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11),
                          OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27),
                          1.23);
    cdlOp.getFormatMetadata().addAttribute(OCIO::METADATA_ID, "test_id");
    cdlOp.getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "Inverse op test description");

    // Test CDL_V1_2_FWD inverse
    {
        cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_FWD);
        const OCIO::CDLOpDataRcPtr invOp = cdlOp.inverse();

        // Ensure metadata is copied
        OCIO_CHECK_EQUAL(invOp->getID(), "test_id");
        OCIO_REQUIRE_EQUAL(invOp->getFormatMetadata().getChildrenElements().size(), 1);
        OCIO_CHECK_EQUAL(std::string(OCIO::METADATA_DESCRIPTION),
                         invOp->getFormatMetadata().getChildrenElements()[0].getElementName());
        OCIO_CHECK_EQUAL(std::string("Inverse op test description"),
                         invOp->getFormatMetadata().getChildrenElements()[0].getElementValue());

        // Ensure style is inverted
        OCIO_CHECK_EQUAL(invOp->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);

        OCIO_CHECK_ASSERT(invOp->isReverse());

        // Ensure CDL parameters are unchanged
        OCIO_CHECK_ASSERT(invOp->getSlopeParams()
                          == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));

        OCIO_CHECK_ASSERT(invOp->getOffsetParams()
                         == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));

        OCIO_CHECK_ASSERT(invOp->getPowerParams()
                          == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));

        OCIO_CHECK_EQUAL(invOp->getSaturation(), 1.23);
    }

    // Test CDL_V1_2_REV inverse
    {
        cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_REV);
        const OCIO::CDLOpDataRcPtr invOp = cdlOp.inverse();

        // Ensure metadata is copied
        OCIO_CHECK_EQUAL(invOp->getID(), "test_id");
        OCIO_CHECK_EQUAL(invOp->getFormatMetadata().getChildrenElements().size(), 1);

        // Ensure style is inverted
        OCIO_CHECK_EQUAL(invOp->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);

        OCIO_CHECK_EQUAL(invOp->isReverse(), false);

        // Ensure CDL parameters are unchanged
        OCIO_CHECK_ASSERT(invOp->getSlopeParams()
                          == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));

        OCIO_CHECK_ASSERT(invOp->getOffsetParams()
                          == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));

        OCIO_CHECK_ASSERT(invOp->getPowerParams()
                          == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));

        OCIO_CHECK_EQUAL(invOp->getSaturation(), 1.23);
    }

    // Test CDL_NO_CLAMP_FWD inverse
    {
        cdlOp.setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
        const OCIO::CDLOpDataRcPtr invOp = cdlOp.inverse();

        // Ensure metadata is copied
        OCIO_CHECK_EQUAL(invOp->getID(), "test_id");
        OCIO_CHECK_EQUAL(invOp->getFormatMetadata().getChildrenElements().size(), 1);

        // Ensure style is inverted
        OCIO_CHECK_EQUAL(invOp->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);
        OCIO_CHECK_ASSERT(invOp->isReverse());

        // Ensure CDL parameters are unchanged
        OCIO_CHECK_ASSERT(invOp->getSlopeParams()
                           == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));

        OCIO_CHECK_ASSERT(invOp->getOffsetParams()
                           == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));

        OCIO_CHECK_ASSERT(invOp->getPowerParams()
                           == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));

        OCIO_CHECK_EQUAL(invOp->getSaturation(), 1.23);
    }

    // Test CDL_NO_CLAMP_REV inverse
    {
        cdlOp.setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_REV);
        const OCIO::CDLOpDataRcPtr invOp = cdlOp.inverse();

        // Ensure metadata is copied
        OCIO_CHECK_EQUAL(invOp->getID(), "test_id");
        OCIO_CHECK_EQUAL(invOp->getFormatMetadata().getChildrenElements().size(), 1);

        // Ensure style is inverted
        OCIO_CHECK_EQUAL(invOp->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
        OCIO_CHECK_ASSERT(!invOp->isReverse());

        // Ensure CDL parameters are unchanged
        OCIO_CHECK_ASSERT(invOp->getSlopeParams()
                          == OCIO::CDLOpData::ChannelParams(1.35, 1.1, 0.71));

        OCIO_CHECK_ASSERT(invOp->getOffsetParams()
                          == OCIO::CDLOpData::ChannelParams(0.05, -0.23, 0.11));

        OCIO_CHECK_ASSERT(invOp->getPowerParams()
                          == OCIO::CDLOpData::ChannelParams(0.93, 0.81, 1.27));

        OCIO_CHECK_EQUAL(invOp->getSaturation(), 1.23);
    }
}


OCIO_ADD_TEST(CDLOpData, style)
{
    // Check default constructor
    OCIO::CDLOpData cdlOp;

    // Check CDL_V1_2_FWD

    cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_FWD);
    OCIO_CHECK_EQUAL(cdlOp.getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);
    OCIO_CHECK_ASSERT(!cdlOp.isReverse());

    // Check the identity replacement.
    OCIO::OpDataRcPtr op = cdlOp.getIdentityReplacement();
    OCIO::RangeOpDataRcPtr rg = OCIO::DynamicPtrCast<OCIO::RangeOpData>(op);
    OCIO_REQUIRE_ASSERT(rg);
    OCIO_CHECK_ASSERT(rg->hasMinInValue()  && (rg->getMinInValue()  == 0.));
    OCIO_CHECK_ASSERT(rg->hasMaxInValue()  && (rg->getMaxInValue()  == 1.));
    OCIO_CHECK_ASSERT(rg->hasMinOutValue() && (rg->getMinOutValue() == 0.));
    OCIO_CHECK_ASSERT(rg->hasMaxOutValue() && (rg->getMaxOutValue() == 1.));
    OCIO_CHECK_ASSERT(!rg->scales());

    // Check CDL_V1_2_REV

    cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_REV);
    OCIO_CHECK_EQUAL(cdlOp.getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);
    OCIO_CHECK_ASSERT(cdlOp.isReverse());

    // Check the identity replacement.
    op = cdlOp.getIdentityReplacement();
    rg = OCIO::DynamicPtrCast<OCIO::RangeOpData>(op);
    OCIO_REQUIRE_ASSERT(rg);
    OCIO_CHECK_ASSERT(rg->hasMinInValue()  && (rg->getMinInValue()  == 0.));
    OCIO_CHECK_ASSERT(rg->hasMaxInValue()  && (rg->getMaxInValue()  == 1.));
    OCIO_CHECK_ASSERT(rg->hasMinOutValue() && (rg->getMinOutValue() == 0.));
    OCIO_CHECK_ASSERT(rg->hasMaxOutValue() && (rg->getMaxOutValue() == 1.));
    OCIO_CHECK_ASSERT(!rg->scales());

    // Check CDL_NO_CLAMP_FWD

    cdlOp.setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
    OCIO_CHECK_EQUAL(cdlOp.getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
    OCIO_CHECK_ASSERT(!cdlOp.isReverse());

    // Check the identity replacement.
    op = cdlOp.getIdentityReplacement();
    OCIO::MatrixOpDataRcPtr mtx = OCIO::DynamicPtrCast<OCIO::MatrixOpData>(op);
    OCIO_REQUIRE_ASSERT(mtx);
    OCIO_CHECK_ASSERT(mtx->isIdentity());

    // Check CDL_NO_CLAMP_REV

    cdlOp.setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_REV);
    OCIO_CHECK_EQUAL(cdlOp.getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);
    OCIO_CHECK_ASSERT(cdlOp.isReverse());

    // Check the identity replacement.
    op = cdlOp.getIdentityReplacement();
    mtx = OCIO::DynamicPtrCast<OCIO::MatrixOpData>(op);
    OCIO_REQUIRE_ASSERT(mtx);
    OCIO_CHECK_ASSERT(mtx->isIdentity());

    // Check unknown style

    OCIO_CHECK_THROW_WHAT(OCIO::CDLOpData::GetStyle("unknown_style"),
                          OCIO::Exception, 
                          "Unknown style for CDL");
}


OCIO_ADD_TEST(CDLOpData, validation_success)
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

    OCIO_CHECK_ASSERT(!cdlOp.isIdentity());
    OCIO_CHECK_ASSERT(!cdlOp.isNoOp());

    OCIO_CHECK_NO_THROW(cdlOp.validate());

    // Set an identity operation
    cdlOp.setSlopeParams(OCIO::kOneParams);
    cdlOp.setOffsetParams(OCIO::kZeroParams);
    cdlOp.setPowerParams(OCIO::kOneParams);
    cdlOp.setSaturation(1.0);

    OCIO_CHECK_ASSERT(cdlOp.isIdentity());
    OCIO_CHECK_ASSERT(!cdlOp.isNoOp());
    // Set to non clamping
    cdlOp.setStyle(OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
    OCIO_CHECK_ASSERT(cdlOp.isIdentity());
    OCIO_CHECK_ASSERT(cdlOp.isNoOp());

    OCIO_CHECK_NO_THROW(cdlOp.validate());

    // Check for slope = 0
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(0.0));
    cdlOp.setOffsetParams(offsetParams);
    cdlOp.setPowerParams(powerParams);
    cdlOp.setSaturation(1.0);

    cdlOp.setStyle(OCIO::CDLOpData::CDL_V1_2_FWD);

    OCIO_CHECK_ASSERT(!cdlOp.isIdentity());
    OCIO_CHECK_ASSERT(!cdlOp.isNoOp());

    OCIO_CHECK_NO_THROW(cdlOp.validate());

    // Check for saturation = 0
    cdlOp.setSlopeParams(slopeParams);
    cdlOp.setOffsetParams(offsetParams);
    cdlOp.setPowerParams(powerParams);
    cdlOp.setSaturation(0.0);

    OCIO_CHECK_ASSERT(!cdlOp.isIdentity());
    OCIO_CHECK_ASSERT(!cdlOp.isNoOp());

    OCIO_CHECK_NO_THROW(cdlOp.validate());
}

OCIO_ADD_TEST(CDLOpData, validation_failure)
{
    OCIO::CDLOpData cdlOp;

    // Fail: invalid scale
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(-0.9));
    cdlOp.setOffsetParams(OCIO::CDLOpData::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::CDLOpData::ChannelParams(1.2));
    cdlOp.setSaturation(1.17);

    OCIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");

    // Fail: invalid power
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(0.9));
    cdlOp.setOffsetParams(OCIO::CDLOpData::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::CDLOpData::ChannelParams(-1.2));
    cdlOp.setSaturation(1.17);

    OCIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");

    // Fail: invalid saturation
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(0.9));
    cdlOp.setOffsetParams(OCIO::CDLOpData::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::CDLOpData::ChannelParams(1.2));
    cdlOp.setSaturation(-1.17);

    OCIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");

    // Check for power = 0
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(0.7));
    cdlOp.setOffsetParams(OCIO::CDLOpData::ChannelParams(0.2));
    cdlOp.setPowerParams(OCIO::CDLOpData::ChannelParams(0.0));
    cdlOp.setSaturation(1.4);

    OCIO_CHECK_THROW_WHAT(cdlOp.validate(), OCIO::Exception, "should be greater than 0");
}

// TODO: CDLOp_inverse_bypass_test is missing

OCIO_ADD_TEST(CDLOpData, channel)
{
  {
    OCIO::CDLOpData cdlOp;

    // False: identity
    OCIO_CHECK_ASSERT(!cdlOp.hasChannelCrosstalk());
  }

  {
    OCIO::CDLOpData cdlOp;
    cdlOp.setSlopeParams(OCIO::CDLOpData::ChannelParams(-0.9));
    cdlOp.setOffsetParams(OCIO::CDLOpData::ChannelParams(0.01));
    cdlOp.setPowerParams(OCIO::CDLOpData::ChannelParams(1.2));

    // False: slope, offset, and power 
    OCIO_CHECK_ASSERT(!cdlOp.hasChannelCrosstalk());
  }

  {
    OCIO::CDLOpData cdlOp;
    cdlOp.setSaturation(1.17);

    // True: saturation
    OCIO_CHECK_ASSERT(cdlOp.hasChannelCrosstalk());
  }
}

