// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradingtone/GradingToneOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingToneOpData, accessors)
{
    // Create GradingToneOpData and check values. More tests are done using
    // GradingToneTransform.
    OCIO::GradingToneOpData gToneOpData{ OCIO::GRADING_LIN };

    OCIO_CHECK_EQUAL(gToneOpData.getStyle(), OCIO::GRADING_LIN);
    OCIO::GradingTone gToneVal{ OCIO::GRADING_LIN };
    OCIO_CHECK_EQUAL(gToneOpData.getValue(), gToneVal);
    OCIO_CHECK_EQUAL(gToneOpData.getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

    gToneOpData.setStyle(OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gToneOpData.getStyle(), OCIO::GRADING_LOG);
    // Setting the style reset values to default.
    const OCIO::GradingTone defLog{ OCIO::GRADING_LOG };
    OCIO_CHECK_EQUAL(gToneOpData.getValue(), defLog);
    gToneVal.m_scontrast += 0.1;
    gToneOpData.setValue(gToneVal);
    OCIO_CHECK_EQUAL(gToneOpData.getValue(), gToneVal);

    // ... except if it is the current style.
    gToneOpData.setStyle(OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gToneOpData.getValue(), gToneVal);

    gToneOpData.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(gToneOpData.getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_EQUAL(gToneOpData.getType(), OCIO::OpData::GradingToneType);
    gToneVal.m_scontrast -= 0.1;
    gToneOpData.setValue(gToneVal);
    OCIO_CHECK_EQUAL(gToneOpData.isNoOp(), true);
    OCIO_CHECK_EQUAL(gToneOpData.isIdentity(), true);
    OCIO_CHECK_EQUAL(gToneOpData.hasChannelCrosstalk(), false);

    const std::string expected{ "log inverse "
                                "<blacks=<red=1 green=1 blue=1 master=1 start=0 width=4> "
                                "shadows=<red=1 green=1 blue=1 master=1 start=2 width=-7> "
                                "midtones=<red=1 green=1 blue=1 master=1 start=0 width=8> "
                                "highlights=<red=1 green=1 blue=1 master=1 start=-2 width=9> "
                                "whites=<red=1 green=1 blue=1 master=1 start=0 width=8> "
                                "s_contrast=1>" };
    OCIO_CHECK_EQUAL(expected, gToneOpData.getCacheID());

    // Test operator==.
    OCIO::GradingToneOpData gt1{ OCIO::GRADING_LIN };
    OCIO::GradingToneOpData gt2{ OCIO::GRADING_LIN };

    OCIO_CHECK_ASSERT(gt1 == gt2);
    gt1.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(!(gt1 == gt2));
    gt2.setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_ASSERT(gt1 == gt2);

    gt1.setStyle(OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(!(gt1 == gt2));
    gt2.setStyle(OCIO::GRADING_LOG);
    OCIO_CHECK_ASSERT(gt1 == gt2);

    auto v1 = gt1.getValue();
    v1.m_midtones.m_red += 0.1;
    gt1.setValue(v1);
    OCIO_CHECK_ASSERT(!(gt1 == gt2));
    auto v2 = gt2.getValue();
    v2.m_midtones.m_red += 0.1;
    gt2.setValue(v2);
    OCIO_CHECK_ASSERT(gt1 == gt2);

    v1.m_scontrast += 0.1;
    gt1.setValue(v1);
    OCIO_CHECK_ASSERT(!(gt1 == gt2));
    v2.m_scontrast += 0.1;
    gt2.setValue(v2);
    OCIO_CHECK_ASSERT(gt1 == gt2);

    OCIO_CHECK_EQUAL(gt1.isIdentity(), false);

    // Check inverse.
    OCIO::ConstGradingToneOpDataRcPtr gt1Inv = gt1.inverse();
    OCIO_CHECK_ASSERT(gt1.isInverse(gt1Inv));

    OCIO_CHECK_EQUAL(gt1.getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(gt1Inv->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    auto v1Inv = gt1Inv->getValue();
    OCIO_CHECK_EQUAL(v1, v1Inv);
}

OCIO_ADD_TEST(GradingToneOpData, validate)
{
    OCIO::GradingToneOpData gt{ OCIO::GRADING_LOG };
    OCIO_CHECK_NO_THROW(gt.validate());

    auto v = gt.getValue();

    // Test invalid black detail.
    v.m_blacks.m_red = 2.0;
    OCIO_CHECK_THROW_WHAT(gt.setValue(v), OCIO::Exception,
                          "GradingTone blacks '<red=2 green=1 blue=1 master=1 start=0.4 width=0.4>' "
                          "are above upper bound (1.9)");

    v.m_blacks.m_red = 1.5;
    gt.setValue(v);
    OCIO_CHECK_NO_THROW(gt.validate());
}

OCIO_ADD_TEST(GradingToneOpData, dynamic)
{
    // Create GradingToneOpData and check values. More tests are done using
    // GradingToneTransform.
    OCIO::GradingToneOpData gt{ OCIO::GRADING_LIN };

    OCIO_CHECK_ASSERT(!gt.isDynamic());
    OCIO::DynamicPropertyRcPtr dp;
    OCIO_CHECK_NO_THROW(dp = gt.getDynamicProperty());
    OCIO_REQUIRE_ASSERT(dp);

    auto dgt = gt.getDynamicPropertyInternal();
    OCIO_REQUIRE_ASSERT(dgt);
    OCIO_CHECK_ASSERT(!dgt->isDynamic());
    dgt->makeDynamic();

    OCIO_CHECK_ASSERT(gt.isDynamic());
    OCIO_CHECK_EQUAL(dp->getType(), OCIO::DYNAMIC_PROPERTY_GRADING_TONE);

    OCIO::GradingTone val{ OCIO::GRADING_LIN };
    val.m_scontrast = 1.1;
    OCIO_CHECK_NO_THROW(dgt->setValue(val));

    OCIO_CHECK_EQUAL(gt.getValue().m_scontrast, 1.1);

    dgt->makeNonDynamic();

    OCIO_CHECK_ASSERT(!gt.isDynamic());
}