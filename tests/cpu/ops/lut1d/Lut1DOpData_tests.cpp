// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/lut1d/Lut1DOpData.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Lut1DOpData, get_lut_ideal_size)
{
    OCIO_CHECK_EQUAL(OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_UINT8), 256);
    OCIO_CHECK_EQUAL(OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_UINT16), 65536);

    OCIO_CHECK_EQUAL(OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_F16), 65536);
    OCIO_CHECK_EQUAL(OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_F32), 65536);
}

OCIO_ADD_TEST(Lut1DOpData, constructor)
{
    OCIO::Lut1DOpData lut(2);

    OCIO_CHECK_ASSERT(lut.getType() == OCIO::OpData::Lut1DType);
    OCIO_CHECK_ASSERT(!lut.isNoOp());
    OCIO_CHECK_ASSERT(lut.isIdentity());
    OCIO_CHECK_EQUAL(lut.getArray().getLength(), 2);
    OCIO_CHECK_EQUAL(lut.getInterpolation(), OCIO::INTERP_DEFAULT);
    OCIO_CHECK_NO_THROW(lut.validate());

    OCIO_CHECK_THROW_WHAT(new OCIO::Lut1DOpData(0), OCIO::Exception, "at least 2");
    OCIO_CHECK_THROW_WHAT(new OCIO::Lut1DOpData(1), OCIO::Exception, "at least 2");
}

OCIO_ADD_TEST(Lut1DOpData, accessors)
{
    OCIO::Lut1DOpData l(17);
    l.setInterpolation(OCIO::INTERP_LINEAR);

    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_ASSERT(!l.isNoOp());
    OCIO_CHECK_ASSERT(l.isIdentity());
    OCIO_CHECK_NO_THROW(l.validate());

    OCIO_CHECK_EQUAL(l.getHueAdjust(), OCIO::HUE_NONE);
    l.setHueAdjust(OCIO::HUE_DW3);
    OCIO_CHECK_EQUAL(l.getHueAdjust(), OCIO::HUE_DW3);

    // Note: Hue adjust does not affect identity status.
    OCIO_CHECK_ASSERT(l.isIdentity());
    OCIO_CHECK_NO_THROW(l.finalize());
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), 1);

    // Restore the number of components
    l.getArray().setNumColorComponents(3);
    l.getArray()[1] = 1.0f;
    OCIO_CHECK_ASSERT(!l.isNoOp());
    OCIO_CHECK_ASSERT(!l.isIdentity());
    OCIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_BEST);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_BEST);

    OCIO_CHECK_EQUAL(l.getArray().getLength(), 17);
    OCIO_CHECK_EQUAL(l.getArray().getNumValues(), 17 * 3);
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), 3);

    OCIO_CHECK_THROW_WHAT(l.getArray().resize(0, 3), OCIO::Exception, "at least 2");
    OCIO_CHECK_THROW_WHAT(l.getArray().resize(1, 3), OCIO::Exception, "at least 2");

    l.getArray().resize(65, 3);

    OCIO_CHECK_EQUAL(l.getArray().getLength(), 65);
    OCIO_CHECK_EQUAL(l.getArray().getNumValues(), 65 * 3);
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), 3);
    OCIO_CHECK_NO_THROW(l.validate());

    OCIO_CHECK_NO_THROW(l.finalize());
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), 3);

    // Restore value.
    l.getArray()[1] = 0.0f;

    OCIO_CHECK_NO_THROW(l.finalize());
    // Finalize sets numColorComponents to 1 if the three channels are equal.
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), 1);

    //
    // Number of components using NAN.
    //

    // Reset number of components.
    l.getArray().setNumColorComponents(3);

    l.getArray()[0] = std::numeric_limits<float>::quiet_NaN();
    l.getArray()[1] = std::numeric_limits<float>::quiet_NaN();
    l.getArray()[2] = 0;

    OCIO_CHECK_NO_THROW(l.finalize());
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), 3);

    l.getArray()[2] = std::numeric_limits<float>::quiet_NaN();
    OCIO_CHECK_NO_THROW(l.finalize());
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), 1);
}

OCIO_ADD_TEST(Lut1DOpData, is_identity)
{
    OCIO::Lut1DOpData l1(OCIO::Lut1DOpData::LUT_STANDARD, 1024, false);

    OCIO_CHECK_ASSERT(l1.isIdentity());

    // The tolerance will be 1e-5.
    const unsigned long lastId = (unsigned long)l1.getArray().getValues().size() - 1;
    const float first = l1.getArray()[0];
    const float last = l1.getArray()[lastId];

    l1.getArray()[0] = first + 0.9e-5f;
    l1.getArray()[lastId] = last + 0.9e-5f;
    OCIO_CHECK_ASSERT(l1.isIdentity());

    l1.getArray()[0] = first + 1.1e-5f;
    l1.getArray()[lastId] = last;
    OCIO_CHECK_ASSERT(!l1.isIdentity());

    l1.getArray()[0] = first;
    l1.getArray()[lastId] = last + 1.1e-5f;
    OCIO_CHECK_ASSERT(!l1.isIdentity());

    OCIO::Lut1DOpData l2(OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE, 65536, false);

    unsigned long id2 = 31700 * 3;
    const float first2 = l2.getArray()[0];
    const float last2 = l2.getArray()[id2];

    // (float)half(1) - (float)half(0) = 5.96046448e-08f
    const float ERROR_0 = 5.96046448e-08f;
    // (float)half(31701) - (float)half(31700) = 32.0f
    const float ERROR_31700 = 32.0f;

    OCIO_CHECK_ASSERT(l2.isIdentity());

    l2.getArray()[0] = first2 + ERROR_0;
    l2.getArray()[id2] = last2 + ERROR_31700;

    OCIO_CHECK_ASSERT(l2.isIdentity());

    l2.getArray()[0] = first2 + 2 * ERROR_0;
    l2.getArray()[id2] = last2;

    OCIO_CHECK_ASSERT(!l2.isIdentity());

    l2.getArray()[0] = first2;
    l2.getArray()[id2] = last2 + 2 * ERROR_31700;

    OCIO_CHECK_ASSERT(!l2.isIdentity());
}

OCIO_ADD_TEST(Lut1DOpData, clone)
{
    OCIO::Lut1DOpData ref(20);
    ref.getArray()[1] = 0.5f;
    ref.setHueAdjust(OCIO::HUE_DW3);

    OCIO::Lut1DOpDataRcPtr pClone = ref.clone();

    OCIO_CHECK_ASSERT(!pClone->isNoOp());
    OCIO_CHECK_ASSERT(!pClone->isIdentity());
    OCIO_CHECK_NO_THROW(pClone->validate());
    OCIO_CHECK_ASSERT(pClone->getArray() == ref.getArray());
    OCIO_CHECK_EQUAL(pClone->getHueAdjust(), OCIO::HUE_DW3);
}

OCIO_ADD_TEST(Lut1DOpData, equality_test)
{
    OCIO::Lut1DOpData l1(OCIO::Lut1DOpData::LUT_STANDARD, 1024, false);
    OCIO::Lut1DOpData l2(OCIO::Lut1DOpData::LUT_STANDARD, 1024, false);
    l2.setInterpolation(OCIO::INTERP_NEAREST);

    // LUT 1D only implements 1 style of interpolation.
    OCIO_CHECK_ASSERT(l1 == l2);

    OCIO::Lut1DOpData l3(OCIO::Lut1DOpData::LUT_STANDARD, 65536, false);

    OCIO_CHECK_ASSERT(!(l1 == l3) && !(l3 == l2));

    OCIO::Lut1DOpData l4(OCIO::Lut1DOpData::LUT_STANDARD, 1024, false);

    OCIO_CHECK_ASSERT(l1 == l4);

    l1.setHueAdjust(OCIO::HUE_DW3);

    OCIO_CHECK_ASSERT(!(l1 == l4));

    // Inversion quality does not affect forward ops equality.
    l4.setHueAdjust(OCIO::HUE_DW3);

    OCIO_CHECK_ASSERT(l1 == l4);

    // Inversion quality does not affect inverse ops equality.
    // Even so applying the ops could lead to small differences.
    auto l5 = l1.inverse();
    auto l6 = l4.inverse();

    OCIO_CHECK_ASSERT(*l5 == *l6);
}

OCIO_ADD_TEST(Lut1DOpData, channel)
{
    auto L1 = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 17, false);

    OCIO::ConstLut1DOpDataRcPtr L2 = std::make_shared<OCIO::Lut1DOpData>(
        OCIO::Lut1DOpData::LUT_STANDARD,
        20, false);

    // False: identity.
    OCIO_CHECK_ASSERT(!L1->hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(L1->mayCompose(L2));

    L1->setHueAdjust(OCIO::HUE_DW3);

    // True: hue restore is on, it's an identity LUT, but this is not
    // tested for efficiency.
    OCIO_CHECK_ASSERT(L1->hasChannelCrosstalk());

    OCIO_CHECK_ASSERT(!L1->mayCompose(L2));

    OCIO::ConstLut1DOpDataRcPtr L1C = L1;
    OCIO_CHECK_ASSERT(!L2->mayCompose(L1C));

    L1->setHueAdjust(OCIO::HUE_NONE);
    L1->getArray()[1] = 3.0f;
    // False: non-identity.
    OCIO_CHECK_ASSERT(!L1->hasChannelCrosstalk());

    L1->setHueAdjust(OCIO::HUE_DW3);
    // True: non-identity w/hue restore.
    OCIO_CHECK_ASSERT(L1->hasChannelCrosstalk());
}

OCIO_ADD_TEST(Lut1DOpData, interpolation)
{
    OCIO::Lut1DOpData l(17);

    l.setInterpolation(OCIO::INTERP_LINEAR);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_BEST);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_BEST);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_CUBIC);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_CUBIC);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "does not support interpolation algorithm");

    l.setInterpolation(OCIO::INTERP_DEFAULT);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_DEFAULT);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(l.validate());

    // TODO: INTERP_NEAREST is currently implemented as INTERP_LINEAR.
    l.setInterpolation(OCIO::INTERP_NEAREST);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_NEAREST);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(l.validate());

    // Invalid interpolation type do not get translated
    // by getConcreteInterpolation.
    l.setInterpolation(OCIO::INTERP_UNKNOWN);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_UNKNOWN);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "does not support interpolation algorithm");

    l.setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, " does not support interpolation algorithm");
}

OCIO_ADD_TEST(Lut1DOpData, lut_1d_compose)
{
    OCIO::Lut1DOpDataRcPtr lut1 = std::make_shared<OCIO::Lut1DOpData>(10);

    lut1->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "lut1");
    lut1->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "description of 'lut1'");
    lut1->getArray().resize(8, 3);
    float * values = &lut1->getArray().getValues()[0];

    values[0]  = 0.0f;      values[1]  = 0.0f;      values[2]  = 0.002333f;
    values[3]  = 0.0f;      values[4]  = 0.291341f; values[5]  = 0.015624f;
    values[6]  = 0.106521f; values[7]  = 0.334331f; values[8]  = 0.462431f;
    values[9]  = 0.515851f; values[10] = 0.474151f; values[11] = 0.624611f;
    values[12] = 0.658791f; values[13] = 0.527381f; values[14] = 0.685071f;
    values[15] = 0.908501f; values[16] = 0.707951f; values[17] = 0.886331f;
    values[18] = 0.926671f; values[19] = 0.846431f; values[20] = 1.0f;
    values[21] = 1.0f;      values[22] = 1.0f;      values[23] = 1.0f;

    OCIO::Lut1DOpDataRcPtr lut2 = std::make_shared<OCIO::Lut1DOpData>(10);

    lut2->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "lut2");
    lut2->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "description of 'lut2'");
    lut2->getArray().resize(8, 3);
    values = &lut2->getArray().getValues()[0];

    values[0]  = 0.0f;        values[1]  = 0.0f;       values[2]  = 0.0023303f;
    values[3]  = 0.0f;        values[4]  = 0.0029134f; values[5]  = 0.015624f;
    values[6]  = 0.00010081f; values[7]  = 0.0059806f; values[8]  = 0.023362f;
    values[9]  = 0.0045628f;  values[10] = 0.024229f;  values[11] = 0.05822f;
    values[12] = 0.0082598f;  values[13] = 0.033831f;  values[14] = 0.074063f;
    values[15] = 0.028595f;   values[16] = 0.075003f;  values[17] = 0.13552f;
    values[18] = 0.69154f;    values[19] = 0.9213f;    values[20] = 1.0f;
    values[21] = 0.76038f;    values[22] = 1.0f;       values[23] = 1.0f;

    OCIO::ConstLut1DOpDataRcPtr lut1C = lut1;
    OCIO::ConstLut1DOpDataRcPtr lut2C = lut2;

    {
        OCIO::Lut1DOpDataRcPtr result;
        OCIO_CHECK_NO_THROW(result =
            OCIO::Lut1DOpData::Compose(lut1C, lut2C, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO));

        OCIO_REQUIRE_EQUAL(result->getFormatMetadata().getNumAttributes(), 1);
        OCIO_CHECK_EQUAL(std::string(result->getFormatMetadata().getAttributeName(0)), OCIO::METADATA_ID);
        OCIO_CHECK_EQUAL(std::string(result->getFormatMetadata().getAttributeValue(0)), "lut1 + lut2");
        OCIO_REQUIRE_EQUAL(result->getFormatMetadata().getNumChildrenElements(), 2);
        const auto & desc1 = result->getFormatMetadata().getChildElement(0);
        OCIO_CHECK_EQUAL(std::string(desc1.getElementName()), OCIO::METADATA_DESCRIPTION);
        OCIO_CHECK_EQUAL(std::string(desc1.getElementValue()), "description of 'lut1'");
        const auto & desc2 = result->getFormatMetadata().getChildElement(1);
        OCIO_CHECK_EQUAL(std::string(desc2.getElementName()), OCIO::METADATA_DESCRIPTION);
        OCIO_CHECK_EQUAL(std::string(desc2.getElementValue()), "description of 'lut2'");

        values = &result->getArray().getValues()[0];

        OCIO_CHECK_EQUAL(result->getArray().getLength(), 8);

        OCIO_CHECK_CLOSE(values[0], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[1], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[2], 0.00254739914f, 1e-6f);

        OCIO_CHECK_CLOSE(values[3], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[4], 0.00669934973f, 1e-6f);
        OCIO_CHECK_CLOSE(values[5], 0.00378420483f, 1e-6f);

        OCIO_CHECK_CLOSE(values[6], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[7], 0.0121908365f, 1e-6f);
        OCIO_CHECK_CLOSE(values[8], 0.0619750582f, 1e-6f);

        OCIO_CHECK_CLOSE(values[9], 0.00682150759f, 1e-6f);
        OCIO_CHECK_CLOSE(values[10], 0.0272925831f, 1e-6f);
        OCIO_CHECK_CLOSE(values[11], 0.096942015f, 1e-6f);

        OCIO_CHECK_CLOSE(values[12], 0.0206955168f, 1e-6f);
        OCIO_CHECK_CLOSE(values[13], 0.0308703855f, 1e-6f);
        OCIO_CHECK_CLOSE(values[14], 0.12295182f, 1e-6f);

        OCIO_CHECK_CLOSE(values[15], 0.716288447f, 1e-6f);
        OCIO_CHECK_CLOSE(values[16], 0.0731772855f, 1e-6f);
        OCIO_CHECK_CLOSE(values[17], 1.0f, 1e-6f);

        OCIO_CHECK_CLOSE(values[18], 0.725044191f, 1e-6f);
        OCIO_CHECK_CLOSE(values[19], 0.857842028f, 1e-6f);
        OCIO_CHECK_CLOSE(values[20], 1.0f, 1e-6f);
    }

    {
        OCIO::Lut1DOpDataRcPtr result;
        OCIO_CHECK_NO_THROW(result =
            OCIO::Lut1DOpData::Compose(lut1C, lut2C, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_BIG));

        values = &result->getArray().getValues()[0];

        OCIO_CHECK_EQUAL(result->getArray().getLength(), 65536);

        OCIO_CHECK_CLOSE(values[0], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[1], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[2], 0.00254739914f, 1e-6f);

        OCIO_CHECK_CLOSE(values[3], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[4], 6.34463504e-07f, 1e-6f);
        OCIO_CHECK_CLOSE(values[5], 0.00254753046f, 1e-6f);

        OCIO_CHECK_CLOSE(values[6], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[7], 1.26915984e-06f, 1e-6f);
        OCIO_CHECK_CLOSE(values[8], 0.00254766271f, 1e-6f);

        OCIO_CHECK_CLOSE(values[9], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[10], 1.90362334e-06f, 1e-6f);
        OCIO_CHECK_CLOSE(values[11], 0.00254779495f, 1e-6f);

        OCIO_CHECK_CLOSE(values[12], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[13], 2.53855251e-06f, 1e-6f);
        OCIO_CHECK_CLOSE(values[14], 0.0025479272f, 1e-6f);

        OCIO_CHECK_CLOSE(values[15], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[16], 3.17324884e-06f, 1e-6f);
        OCIO_CHECK_CLOSE(values[17], 0.00254805945f, 1e-6f);

        OCIO_CHECK_CLOSE(values[300], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[301], 6.3463347e-05f, 1e-6f);
        OCIO_CHECK_CLOSE(values[302], 0.00256060902f, 1e-6f);

        OCIO_CHECK_CLOSE(values[900], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[901], 0.000190390972f, 1e-6f);
        OCIO_CHECK_CLOSE(values[902], 0.00258703064f, 1e-6f);

        OCIO_CHECK_CLOSE(values[2700], 0.0f, 1e-6f);
        OCIO_CHECK_CLOSE(values[2701], 0.000571172219f, 1e-6f);
        OCIO_CHECK_CLOSE(values[2702], 0.00266629551f, 1e-6f);
    }
}

OCIO_ADD_TEST(Lut1DOpData, lut_1d_compose_sc)
{
    OCIO::Lut1DOpDataRcPtr lut1 = std::make_shared<OCIO::Lut1DOpData>(2);

    lut1->getArray().resize(2, 3);
    float * values = &lut1->getArray().getValues()[0];
    values[0] = 64.f;  values[1] = 64.f;   values[2] = 64.f;
    values[3] = 196.f; values[4] = 196.f;  values[5] = 196.f;
    lut1->scale(1.0f / 255.0f);

    OCIO::Lut1DOpDataRcPtr lut2 = std::make_shared<OCIO::Lut1DOpData>(2);

    lut2->getArray().resize(32, 3);
    values = &lut2->getArray().getValues()[0];

    values[0] = 0.0000000f;   values[1] = 0.0000000f;  values[2] = 0.0023303f;
    values[3] = 0.0000000f;   values[4] = 0.0001869f;  values[5] = 0.0052544f;
    values[6] = 0.0000000f;   values[7] = 0.0010572f;  values[8] = 0.0096338f;
    values[9] = 0.0000000f;  values[10] = 0.0029134f; values[11] = 0.0156240f;
    values[12] = 0.0001008f; values[13] = 0.0059806f; values[14] = 0.0233620f;
    values[15] = 0.0007034f; values[16] = 0.0104480f; values[17] = 0.0329680f;
    values[18] = 0.0021120f; values[19] = 0.0164810f; values[20] = 0.0445540f;
    values[21] = 0.0045628f; values[22] = 0.0242290f; values[23] = 0.0582200f;
    values[24] = 0.0082598f; values[25] = 0.0338310f; values[26] = 0.0740630f;
    values[27] = 0.0133870f; values[28] = 0.0454150f; values[29] = 0.0921710f;
    values[30] = 0.0201130f; values[31] = 0.0591010f; values[32] = 0.1126300f;
    values[33] = 0.0285950f; values[34] = 0.0750030f; values[35] = 0.1355200f;
    values[36] = 0.0389830f; values[37] = 0.0932290f; values[38] = 0.1609100f;
    values[39] = 0.0514180f; values[40] = 0.1138800f; values[41] = 0.1888800f;
    values[42] = 0.0660340f; values[43] = 0.1370600f; values[44] = 0.2195000f;
    values[45] = 0.0829620f; values[46] = 0.1628600f; values[47] = 0.2528300f;
    values[48] = 0.1023300f; values[49] = 0.1913800f; values[50] = 0.2889500f;
    values[51] = 0.1242500f; values[52] = 0.2227000f; values[53] = 0.3279000f;
    values[54] = 0.1488500f; values[55] = 0.2569100f; values[56] = 0.3697600f;
    values[57] = 0.1762300f; values[58] = 0.2940900f; values[59] = 0.4145900f;
    values[60] = 0.2065200f; values[61] = 0.3343300f; values[62] = 0.4624300f;
    values[63] = 0.2398200f; values[64] = 0.3777000f; values[65] = 0.5133400f;
    values[66] = 0.2762200f; values[67] = 0.4242800f; values[68] = 0.5673900f;
    values[69] = 0.3158500f; values[70] = 0.4741500f; values[71] = 0.6246100f;
    values[72] = 0.3587900f; values[73] = 0.5273800f; values[74] = 0.6850700f;
    values[75] = 0.4051500f; values[76] = 0.5840400f; values[77] = 0.7488100f;
    values[78] = 0.4550200f; values[79] = 0.6442100f; values[80] = 0.8158800f;
    values[81] = 0.5085000f; values[82] = 0.7079500f; values[83] = 0.8863300f;
    values[84] = 0.5656900f; values[85] = 0.7753400f; values[86] = 0.9602100f;
    values[87] = 0.6266700f; values[88] = 0.8464300f; values[89] = 1.0000000f;
    values[90] = 0.6915400f; values[91] = 0.9213000f; values[92] = 1.0000000f;
    values[93] = 0.7603800f; values[94] = 1.0000000f; values[95] = 1.0000000f;

    OCIO::ConstLut1DOpDataRcPtr lut1C = lut1;
    OCIO::ConstLut1DOpDataRcPtr lut2C = lut2;

    {
        OCIO::Lut1DOpDataRcPtr lComp;
        OCIO_CHECK_NO_THROW(lComp = OCIO::Lut1DOpData::Compose(lut1C, lut2C,
                                                               OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO));

        OCIO_CHECK_EQUAL(lComp->getArray().getLength(), 2);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[0], 0.00744791f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[1], 0.03172233f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[2], 0.07058375f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[3], 0.3513808f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[4], 0.51819527f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[5], 0.67463773f, 1e-6f);
    }

    {
        OCIO::Lut1DOpDataRcPtr lComp;
        OCIO_CHECK_NO_THROW(lComp = OCIO::Lut1DOpData::Compose(lut1C, lut2C,
                                                               OCIO::Lut1DOpData::COMPOSE_RESAMPLE_BIG));

        OCIO_CHECK_EQUAL(lComp->getArray().getLength(), 65536);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[0], 0.00744791f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[1], 0.03172233f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[2], 0.07058375f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[98688], 0.0991418f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[98689], 0.1866853f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[98690], 0.2830042f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[196605], 0.3513808f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[196606], 0.51819527f, 1e-6f);
        OCIO_CHECK_CLOSE(lComp->getArray().getValues()[196607], 0.67463773f, 1e-6f);
    }
}

namespace
{
const char uid[] = "uid";
};

OCIO_ADD_TEST(Lut1DOpData, inverse_hueadjust)
{
    OCIO::Lut1DOpData refLut1d(OCIO::Lut1DOpData::LUT_STANDARD, 65536, false);
    refLut1d.getFormatMetadata().addAttribute(OCIO::METADATA_ID, uid);

    refLut1d.setHueAdjust(OCIO::HUE_DW3);

    // Get inverse of reference lut1d operation.
    OCIO::Lut1DOpDataRcPtr invLut1d = refLut1d.inverse();

    OCIO_CHECK_EQUAL(invLut1d->getHueAdjust(), OCIO::HUE_DW3);
}

OCIO_ADD_TEST(Lut1DOpData, is_inverse)
{
    // Create forward LUT.
    auto L1 = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 5, false);
    L1->getFormatMetadata().addAttribute(OCIO::METADATA_ID, uid);

    // Make it not an identity.
    OCIO::Array & array = L1->getArray();
    OCIO::Array::Values & values = array.getValues();
    values[0] = 20.f;
    OCIO_CHECK_ASSERT(!L1->isIdentity());

    // Create an inverse LUT with same basics.
    OCIO::Lut1DOpDataRcPtr L2 = L1->inverse();
    OCIO_REQUIRE_ASSERT(L2);

    OCIO_CHECK_ASSERT(!(*L1 == *L2));

    OCIO::ConstLut1DOpDataRcPtr L1C = L1;
    OCIO::ConstLut1DOpDataRcPtr L2C = L2;

    // Check isInverse.
    OCIO_CHECK_ASSERT(L1->isInverse(L2C));
    OCIO_CHECK_ASSERT(L2->isInverse(L1C));
}

void SetLutArray(OCIO::Lut1DOpData & op,
                 unsigned long dimension,
                 unsigned long channels,
                 const float * data)
{
    OCIO::Array & refArray = op.getArray();
    refArray.resize(dimension, channels);
    // The data allocated for the array is dimension * getMaxColorComponents(),
    // not dimension * channels.

    const unsigned long maxChannels = refArray.getMaxColorComponents();
    OCIO::Array::Values & values = refArray.getValues();
    if (channels == maxChannels)
    {
        memcpy(&(values[0]), data, dimension * channels * sizeof(float));
    }
    else
    {
        // Set the red component, fill other with zero values.
        for (unsigned long i = 0; i < dimension; ++i)
        {
            values[i*maxChannels + 0] = data[i];
            values[i*maxChannels + 1] = 0.f;
            values[i*maxChannels + 2] = 0.f;
        }
    }
}

// Validate the overall increase/decrease and effective domain.
void CheckInverse_IncreasingEffectiveDomain(
    unsigned long dimension, unsigned long channels, const float* fwdArrayData,
    bool expIncreasingR, unsigned long expStartDomainR, unsigned long expEndDomainR,
    bool expIncreasingG, unsigned long expStartDomainG, unsigned long expEndDomainG,
    bool expIncreasingB, unsigned long expStartDomainB, unsigned long expEndDomainB)
{
    OCIO::Lut1DOpData refLut1dOp(OCIO::Lut1DOpData::LUT_STANDARD, 1024, false);
    refLut1dOp.getFormatMetadata().addAttribute(OCIO::METADATA_ID, uid);

    SetLutArray(refLut1dOp, dimension, channels, fwdArrayData);

    refLut1dOp.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(refLut1dOp.validate());
    OCIO_CHECK_NO_THROW(refLut1dOp.finalize());

    const OCIO::Lut1DOpData::ComponentProperties &
        redProperties = refLut1dOp.getRedProperties();

    const OCIO::Lut1DOpData::ComponentProperties &
        greenProperties = refLut1dOp.getGreenProperties();

    const OCIO::Lut1DOpData::ComponentProperties &
        blueProperties = refLut1dOp.getBlueProperties();

    OCIO_CHECK_EQUAL(redProperties.isIncreasing, expIncreasingR);
    OCIO_CHECK_EQUAL(redProperties.startDomain, expStartDomainR);
    OCIO_CHECK_EQUAL(redProperties.endDomain, expEndDomainR);

    OCIO_CHECK_EQUAL(greenProperties.isIncreasing, expIncreasingG);
    OCIO_CHECK_EQUAL(greenProperties.startDomain, expStartDomainG);
    OCIO_CHECK_EQUAL(greenProperties.endDomain, expEndDomainG);

    OCIO_CHECK_EQUAL(blueProperties.isIncreasing, expIncreasingB);
    OCIO_CHECK_EQUAL(blueProperties.startDomain, expStartDomainB);
    OCIO_CHECK_EQUAL(blueProperties.endDomain, expEndDomainB);
}

OCIO_ADD_TEST(Lut1DOpData, inverse_increasing_effective_domain)
{
    {
        const float fwdData[] = { 0.1f, 0.8f, 0.1f,    // 0
                                  0.1f, 0.7f, 0.1f,
                                  0.1f, 0.6f, 0.1f,    // 2
                                  0.2f, 0.5f, 0.1f,    // 3
                                  0.3f, 0.4f, 0.2f,
                                  0.4f, 0.3f, 0.3f,
                                  0.5f, 0.1f, 0.4f,    // 6
                                  0.6f, 0.1f, 0.5f,    // 7
                                  0.7f, 0.1f, 0.5f,
                                  0.8f, 0.1f, 0.5f };  // 9

        CheckInverse_IncreasingEffectiveDomain(
            10, 3, fwdData,
            true,  2, 9,   // increasing, flat [0, 2]
            false, 0, 6,   // decreasing, flat [6, 9]
            true,  3, 7);  // increasing, flat [0, 3] and [7, 9]
    }

    {
        const float fwdData[] = { 0.3f,    // 0
                                  0.3f,
                                  0.3f,    // 2
                                  0.4f,
                                  0.5f,
                                  0.6f,
                                  0.7f,
                                  0.8f,    // 7
                                  0.8f,
                                  0.8f };  // 9

        CheckInverse_IncreasingEffectiveDomain(
            10, 1, fwdData,
            true, 2, 7,   // increasing, flat [0->2] and [7->9]
            true, 2, 7,
            true, 2, 7);
    }

    {
        const float fwdData[] = { 0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f };

        CheckInverse_IncreasingEffectiveDomain(
            10, 1, fwdData,
            false, 0, 0,
            false, 0, 0,
            false, 0, 0);
    }

    {
        const float fwdData[] = { 0.8f,     // 0
                                  0.9f,     // reversal
                                  0.8f,     // 2
                                  0.5f,
                                  0.4f,
                                  0.3f,
                                  0.2f,
                                  0.1f,     // 7
                                  0.1f,
                                  0.2f };   // reversal

        CheckInverse_IncreasingEffectiveDomain(
            10, 1, fwdData,
            false, 2, 7,
            false, 2, 7,
            false, 2, 7);
    }
}

// Validate the flatten algorithms.
void CheckInverse_Flatten(unsigned long dimension,
                          unsigned long channels,
                          const float * fwdArrayData,
                          const float * expInvArrayData)
{
    OCIO::Lut1DOpData refLut1dOp(OCIO::Lut1DOpData::LUT_STANDARD, 65536, false);
    refLut1dOp.getFormatMetadata().addAttribute(OCIO::METADATA_ID, uid);

    SetLutArray(refLut1dOp, dimension, channels, fwdArrayData);

    refLut1dOp.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(refLut1dOp.validate());
    OCIO_CHECK_NO_THROW(refLut1dOp.finalize());

    const OCIO::Array::Values & invValues = refLut1dOp.getArray().getValues();

    for (unsigned long i = 0; i < dimension * channels; ++i)
    {
        OCIO_CHECK_EQUAL(invValues[i], expInvArrayData[i]);
    }
}

OCIO_ADD_TEST(Lut1DOpData, inverse_flatten_test)
{
    {
        const float fwdData[] = { 0.10f, 0.90f, 0.25f,    // 0
                                  0.20f, 0.80f, 0.30f,
                                  0.30f, 0.70f, 0.40f,
                                  0.40f, 0.60f, 0.50f,
                                  0.35f, 0.50f, 0.60f,    // 4
                                  0.30f, 0.55f, 0.50f,    // 5
                                  0.45f, 0.60f, 0.40f,    // 6
                                  0.50f, 0.65f, 0.30f,    // 7
                                  0.60f, 0.45f, 0.20f,    // 8
                                  0.70f, 0.50f, 0.10f };  // 9
        // red is increasing, with a reversal [4, 5]
        // green is decreasing, with reversals [4, 5] and [9]
        // blue is decreasing, with reversals [0, 8]

        const float expInvData[] = { 0.10f, 0.90f, 0.25f,
                                     0.20f, 0.80f, 0.25f,
                                     0.30f, 0.70f, 0.25f,
                                     0.40f, 0.60f, 0.25f,
                                     0.40f, 0.50f, 0.25f,
                                     0.40f, 0.50f, 0.25f,
                                     0.45f, 0.50f, 0.25f,
                                     0.50f, 0.50f, 0.25f,
                                     0.60f, 0.45f, 0.20f,
                                     0.70f, 0.45f, 0.10f };

        CheckInverse_Flatten(10, 3, fwdData, expInvData);
    }
}

void SetLutArrayHalf(const OCIO::Lut1DOpDataRcPtr & op, unsigned long channels)
{
    const unsigned long dimension = 65536u;
    OCIO::Array & refArray = op->getArray();
    refArray.resize(dimension, channels);
    // The data allocated for the array is dimension * getMaxColorComponents(),
    // not dimension * channels.

    const unsigned long maxChannels = refArray.getMaxColorComponents();
    OCIO::Array::Values & values = refArray.getValues();
    for (unsigned long j = 0u; j < channels; j++)
    {
        for (unsigned long i = 0u; i < 65536u; i++)
        {
            float f = OCIO::ConvertHalfBitsToFloat((unsigned short)i);
            if (j == 0)
            {
                if (i < 32768u)
                    f = 2.f * f - 0.1f;
                else                            // neg domain overlaps pos with reversal
                    f = 3.f * f + 0.1f;
                if (i >= 25000u && i < 32760u)  // flat spot at positive end
                    f = 10000.f;
                if (i >= 60000u)                // flat spot at neg end
                    f = -10000.f;
                if (i > 15000u && i < 20000u)   // reversal in positive side
                    f = 0.5f;
                if (i > 50000u && i < 55000u)   // reversal in negative side
                    f = -2.f;
            }
            else if (j == 1)
            {
                if (i < 32768u)                 // decreasing function
                    f = -0.5f * f + 0.02f;
                else                            // gap between pos & neg at zero
                    f = -0.4f * f + 0.05f;
                if (i >= 25000u && i < 32760u)  // flat spot at positive end
                    f = -400.f;
                if (i >= 60000u)                // flat spot at neg end
                    f = 2000.f;
                if (i > 15000u && i < 20000u)   // reversal in positive side
                    f = -0.1f;
                if (i > 50000u && i < 55000u)   // reversal in negative side
                    f = 1.4f;
            }
            else if (j == 2)
            {
                if (i < 32768u)
                    f = std::pow(f, 1.5f);
                else
                    f = -std::pow(-f, 0.9f);
                if (i <= 11878u || (i >= 32768u && i <= 44646u))  // flat spot around zero
                    f = -0.01f;
            }
            values[i * maxChannels + j] = f;
        }
    }
}

OCIO_ADD_TEST(Lut1DOpData, inverse_half_domain)
{
    const auto halfFlags = OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE;
    auto refLut1dOp = std::make_shared<OCIO::Lut1DOpData>(halfFlags, 65536, false);
    refLut1dOp->getFormatMetadata().addAttribute(OCIO::METADATA_ID, uid);

    SetLutArrayHalf(refLut1dOp, 3u);

    refLut1dOp->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(refLut1dOp->validate());
    OCIO_CHECK_NO_THROW(refLut1dOp->finalize());

    const OCIO::Lut1DOpData::ComponentProperties &
        redProperties = refLut1dOp->getRedProperties();

    const OCIO::Lut1DOpData::ComponentProperties &
        greenProperties = refLut1dOp->getGreenProperties();

    const OCIO::Lut1DOpData::ComponentProperties &
        blueProperties = refLut1dOp->getBlueProperties();

    const OCIO::Array::Values &
        invValues = refLut1dOp->getArray().getValues();

    // Check increasing/decreasing and start/end domain.
    OCIO_CHECK_EQUAL(redProperties.isIncreasing, true);
    OCIO_CHECK_EQUAL(redProperties.startDomain, 0u);
    OCIO_CHECK_EQUAL(redProperties.endDomain, 25000u);
    OCIO_CHECK_EQUAL(redProperties.negStartDomain, 44100u);  // -0.2/3 (flattened to remove overlap)
    OCIO_CHECK_EQUAL(redProperties.negEndDomain, 60000u);

    OCIO_CHECK_EQUAL(greenProperties.isIncreasing, false);
    OCIO_CHECK_EQUAL(greenProperties.startDomain, 0u);
    OCIO_CHECK_EQUAL(greenProperties.endDomain, 25000u);
    OCIO_CHECK_EQUAL(greenProperties.negStartDomain, 32768u);
    OCIO_CHECK_EQUAL(greenProperties.negEndDomain, 60000u);

    OCIO_CHECK_EQUAL(blueProperties.isIncreasing, true);
    OCIO_CHECK_EQUAL(blueProperties.startDomain, 11878u);
    OCIO_CHECK_EQUAL(blueProperties.endDomain, 31743u);      // see note in Lut1DOpData.cpp
    OCIO_CHECK_EQUAL(blueProperties.negStartDomain, 44646u);
    OCIO_CHECK_EQUAL(blueProperties.negEndDomain, 64511u);

    // Check reversals are removed.
    half act;
    act = invValues[16000 * 3];
    OCIO_CHECK_EQUAL(act.bits(), 15922);  // halfToFloat(15000) * 2 - 0.1
    act = invValues[52000 * 3];
    OCIO_CHECK_EQUAL(act.bits(), 51567);  // halfToFloat(50000) * 3 + 0.1
    act = invValues[16000 * 3 + 1];
    OCIO_CHECK_EQUAL(act.bits(), 46662);  // halfToFloat(15000) * -0.5 + 0.02
    act = invValues[52000 * 3 + 1];
    OCIO_CHECK_EQUAL(act.bits(), 15885);  // halfToFloat(50000) * -0.4 + 0.05

    bool reversal = false;
    for (unsigned long i = 1; i < 31745; i++)
    {
        if (invValues[i * 3] < invValues[(i - 1) * 3])           // increasing red
            reversal = true;
    }
    OCIO_CHECK_ASSERT(!reversal);
    // Check no overlap at +0 and -0.
    OCIO_CHECK_ASSERT(invValues[0] >= invValues[32768 * 3]);
    reversal = false;
    for (unsigned long i = 1; i < 31745; i++)
    {
        if (invValues[i * 3 + 1] > invValues[(i - 1) * 3 + 1])   // decreasing grn
            reversal = true;
    }
    OCIO_CHECK_ASSERT(!reversal);
    OCIO_CHECK_ASSERT(invValues[0 + 1] <= invValues[32768 * 3 + 1]);
    reversal = false;
    for (unsigned long i = 1; i < 31745; i++)
    {
        if (invValues[i * 3 + 2] < invValues[(i - 1) * 3 + 2])   // increasing blu
            reversal = true;
    }
    OCIO_CHECK_ASSERT(!reversal);
    OCIO_CHECK_ASSERT(invValues[0 + 2] >= invValues[32768 * 3 + 2]);
}

OCIO_ADD_TEST(Lut1DOpData, make_fast_from_inverse_gpu_extented_domain)
{
    const std::string ctfFile("lut1d_inverse_gpu.ctf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, ctfFile, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OCIO_REQUIRE_ASSERT(lut);
    auto lutEdit = lut->clone();
    OCIO_REQUIRE_ASSERT(lutEdit);
    // Ordinarily the entries would be determined by the inDepth.
    // This is just to make sure the 32f depth of the above is not what gets the half domain.
    lutEdit->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    lut = lutEdit;

    OCIO::Lut1DOpDataRcPtr newLut;
    OCIO_CHECK_NO_THROW(newLut = OCIO::MakeFastLut1DFromInverse(lut));

    // This LUT has values outside [0,1], so the fastLut needs to have a half domain
    // even on GPU.
    OCIO_CHECK_EQUAL(newLut->getArray().getLength(), 65536u);
    OCIO_CHECK_ASSERT(newLut->isInputHalfDomain());
}

OCIO_ADD_TEST(Lut1DOpData, make_fast_from_inverse_f32_opt)
{
    const std::string ctfFile("lut1d_inv.ctf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, ctfFile, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 3);

    auto op = std::const_pointer_cast<const OCIO::Op>(ops[2]);
    auto opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OCIO_REQUIRE_ASSERT(lut);

    OCIO::Lut1DOpDataRcPtr newLut;
    OCIO_CHECK_NO_THROW(newLut = OCIO::MakeFastLut1DFromInverse(lut));

    // TODO: This LUT has all values in [0,1], so the fastLut should be compact for more efficient
    // evaluation and less texture usage on GPU.
    OCIO_CHECK_EQUAL(newLut->getArray().getLength(), 65536U);
    OCIO_CHECK_ASSERT(newLut->isInputHalfDomain());
}

OCIO_ADD_TEST(Lut1DOpData, make_fast_from_inverse_half_domain)
{
    const std::string ctfFile("lut1d_halfdom.ctf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, ctfFile, context,
                                           OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);
    OCIO_REQUIRE_ASSERT(lut);
    OCIO::Lut1DOpDataRcPtr lutInv = lut->inverse();

    OCIO::ConstLut1DOpDataRcPtr lutInvConst = lutInv;
    OCIO::Lut1DOpDataRcPtr newLut;
    OCIO_CHECK_NO_THROW(newLut = OCIO::MakeFastLut1DFromInverse(lutInvConst));

    // Source LUT has an extended domain, so fastLut should have a half domain.
    OCIO_CHECK_EQUAL(newLut->getArray().getLength(), 65536u);
    OCIO_CHECK_ASSERT(newLut->isInputHalfDomain());
}

OCIO_ADD_TEST(Lut1DOpData, compose_inverse_luts)
{
    OCIO::ConstLut1DOpDataRcPtr lutRef = std::make_shared<OCIO::Lut1DOpData>(17);
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(17);

    auto & lutValues = lut->getArray().getValues();
    for (auto & val : lutValues)
    {
        val *= val;
    }

    OCIO::ConstLut1DOpDataRcPtr lutFwd1 = lut->clone();
    OCIO::ConstLut1DOpDataRcPtr lutFwd2 = lutFwd1->clone();

    // Forward + forward.
    auto compLutFwdFwd = OCIO::Lut1DOpData::Compose(lutFwd1, lutFwd2,
                                                    OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO);
    OCIO_CHECK_EQUAL(compLutFwdFwd->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    // Inverse + inverse.
    OCIO::Lut1DOpDataRcPtr lutInv1NonConst = lut->inverse();
    lutInv1NonConst->finalize();
    OCIO::ConstLut1DOpDataRcPtr lutInv1 = lutInv1NonConst;
    OCIO::Lut1DOpDataRcPtr lutInv2NonConst = lut->inverse();
    lutInv2NonConst->finalize();
    OCIO::ConstLut1DOpDataRcPtr lutInv2 = lutInv2NonConst;
    auto compLutInvInv = OCIO::Lut1DOpData::Compose(lutInv1, lutInv2,
                                                    OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO);
    OCIO_CHECK_EQUAL(compLutInvInv->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_ASSERT(compLutFwdFwd->getArray().getValues() ==
                      compLutInvInv->getArray().getValues());

    // Forward + inverse.
    auto compLutFwdInv = OCIO::Lut1DOpData::Compose(lutFwd1, lutInv1,
                                                    OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO);
    OCIO_CHECK_EQUAL(compLutFwdInv->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_ASSERT(compLutFwdInv->getArray().getValues() == lutRef->getArray().getValues());

    // Inverse + forward.
    auto compLutInvFwd = OCIO::Lut1DOpData::Compose(lutInv1, lutFwd1,
                                                    OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO);
    OCIO_CHECK_EQUAL(compLutInvFwd->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_ASSERT(compLutInvFwd->isInputHalfDomain());
    OCIO_REQUIRE_EQUAL(compLutInvFwd->getArray().getLength(), 65536);
    OCIO_CHECK_EQUAL(compLutInvFwd->getArray()[14336 * 3], 0.5f);
}

