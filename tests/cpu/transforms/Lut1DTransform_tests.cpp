// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/Lut1DTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Lut1DTransform, basic)
{
    const OCIO::Lut1DTransformRcPtr lut = OCIO::Lut1DTransform::Create();

    OCIO_CHECK_EQUAL(lut->getLength(), 2);
    OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(lut->getHueAdjust(), OCIO::HUE_NONE);
    OCIO_CHECK_EQUAL(lut->getInputHalfDomain(), false);
    OCIO_CHECK_EQUAL(lut->getOutputRawHalfs(), false);
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    lut->getValue(0, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 0.f);
    lut->getValue(1, r, g, b);
    OCIO_CHECK_EQUAL(r, 1.f);
    OCIO_CHECK_EQUAL(g, 1.f);
    OCIO_CHECK_EQUAL(b, 1.f);

    lut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(lut->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    lut->setLength(3);
    OCIO_CHECK_EQUAL(lut->getLength(), 3);
    lut->getValue(0, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.f);
    OCIO_CHECK_EQUAL(g, 0.f);
    OCIO_CHECK_EQUAL(b, 0.f);
    lut->getValue(1, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.5f);
    OCIO_CHECK_EQUAL(g, 0.5f);
    OCIO_CHECK_EQUAL(b, 0.5f);
    lut->getValue(2, r, g, b);
    OCIO_CHECK_EQUAL(r, 1.f);
    OCIO_CHECK_EQUAL(g, 1.f);
    OCIO_CHECK_EQUAL(b, 1.f);

    r = 0.51f;
    g = 0.52f;
    b = 0.53f;
    lut->setValue(1, r, g, b);

    r = 0.f;
    g = 0.f;
    b = 0.f;
    lut->getValue(1, r, g, b);

    OCIO_CHECK_EQUAL(r, 0.51f);
    OCIO_CHECK_EQUAL(g, 0.52f);
    OCIO_CHECK_EQUAL(b, 0.53f);

    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);

    lut->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(lut->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT8);

    // File out bit-depth does not affect values.
    r = 0.f;
    g = 0.f;
    b = 0.f;
    lut->getValue(1, r, g, b);

    OCIO_CHECK_EQUAL(r, 0.51f);
    OCIO_CHECK_EQUAL(g, 0.52f);
    OCIO_CHECK_EQUAL(b, 0.53f);

    OCIO_CHECK_NO_THROW(lut->validate());

    OCIO_CHECK_THROW_WHAT(lut->setValue(3, 0.f, 0.f, 0.f), OCIO::Exception,
                          "should be less than the length");
    OCIO_CHECK_THROW_WHAT(lut->getValue(3, r, g, b), OCIO::Exception,
                          "should be less than the length");

    lut->setInputHalfDomain(true);
    OCIO_CHECK_THROW_WHAT(lut->validate(), OCIO::Exception,
                          "65536 required for halfDomain 1D LUT");

    OCIO_CHECK_THROW_WHAT(lut->setLength(1024*1024+1), OCIO::Exception,
                          "must not be greater than");

    lut->setInputHalfDomain(false);
    lut->setValue(0, -0.2f, 0.1f, -0.3f);
    lut->setValue(2, 1.2f, 1.3f, 0.8f);

    std::ostringstream oss;
    oss << *lut;
    OCIO_CHECK_EQUAL(oss.str(), "<Lut1DTransform direction=inverse, fileoutdepth=8ui,"
        " interpolation=default, inputhalf=0, outputrawhalf=0, hueadjust=0,"
        " length=3, minrgb=[-0.2 0.1 -0.3], maxrgb=[1.2 1.3 0.8]>");

    auto lut2 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::Lut1DTransform>(lut->createEditableCopy());
    std::ostringstream oss2;
    oss2 << *lut2;
    OCIO_CHECK_EQUAL(oss2.str(), oss.str());

    OCIO_CHECK_ASSERT(lut->equals(*lut2));
}

OCIO_ADD_TEST(Lut1DTransform, create_with_parameters)
{
    const auto lut0 = OCIO::Lut1DTransform::Create(65536, true);

    OCIO_CHECK_EQUAL(lut0->getLength(), 65536);
    OCIO_CHECK_EQUAL(lut0->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(lut0->getHueAdjust(), OCIO::HUE_NONE);
    OCIO_CHECK_EQUAL(lut0->getInputHalfDomain(), true);
    OCIO_CHECK_NO_THROW(lut0->validate());

    const auto lut1 = OCIO::Lut1DTransform::Create(10, true);

    OCIO_CHECK_EQUAL(lut1->getLength(), 10);
    OCIO_CHECK_EQUAL(lut1->getInputHalfDomain(), true);
    OCIO_CHECK_THROW_WHAT(lut1->validate(), OCIO::Exception,
                          "65536 required for halfDomain 1D LUT");

    const auto lut2 = OCIO::Lut1DTransform::Create(8, false);

    OCIO_CHECK_EQUAL(lut2->getLength(), 8);
    OCIO_CHECK_EQUAL(lut2->getInputHalfDomain(), false);
    OCIO_CHECK_NO_THROW(lut2->validate());
}

OCIO_ADD_TEST(Lut1DTransform, non_monotonic)
{
    auto lut = OCIO::Lut1DTransform::Create();

    // Make a non-monotonic LUT.
    lut->setLength(5);
    float r = 0.1f;
    float g = 0.1f;
    float b = 0.1f;
    lut->setValue(2, r, g, b);

    OCIO_CHECK_NO_THROW(lut->validate());
    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateRaw());

    // Processor from forward LUT.
    auto proc = config->getProcessor(lut);

    // Make a transform from the processor.
    auto transformFromProc = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(transformFromProc->getNumTransforms(), 1);
    auto lutFromTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut1DTransform>(transformFromProc->getTransform(0));
    OCIO_REQUIRE_ASSERT(lutFromTransform);

    // Transform is still a non-montonic LUT.
    lutFromTransform->getValue(2, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.1f);
    OCIO_CHECK_EQUAL(g, 0.1f);
    OCIO_CHECK_EQUAL(b, 0.1f);

    // Now with inverse LUT.
    lut->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    proc = config->getProcessor(lut);

    transformFromProc = proc->createGroupTransform();
    OCIO_REQUIRE_EQUAL(transformFromProc->getNumTransforms(), 1);
    lutFromTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::Lut1DTransform>(transformFromProc->getTransform(0));
    OCIO_REQUIRE_ASSERT(lutFromTransform);

    // LUT has been made monotonic.
    lutFromTransform->getValue(2, r, g, b);
    OCIO_CHECK_EQUAL(r, 0.25f);
    OCIO_CHECK_EQUAL(g, 0.25f);
    OCIO_CHECK_EQUAL(b, 0.25f);
}

OCIO_ADD_TEST(Lut1DTransform, hue_adjust)
{
    auto lut = OCIO::Lut1DTransform::Create();
    OCIO_CHECK_EQUAL(lut->getHueAdjust(), OCIO::HUE_NONE);
    OCIO_CHECK_NO_THROW(lut->setHueAdjust(OCIO::HUE_DW3));
    OCIO_CHECK_EQUAL(lut->getHueAdjust(), OCIO::HUE_DW3);
    OCIO_CHECK_THROW_WHAT(lut->setHueAdjust(OCIO::HUE_WYPN), OCIO::Exception,
                          "1D LUT HUE_WYPN hue adjust style is not implemented.");
}

OCIO_ADD_TEST(Lut1DTransform, format_metadata)
{
    auto lut = OCIO::Lut1DTransform::Create();
    OCIO::FormatMetadata & fmd = lut->getFormatMetadata();
    fmd.setName("test LUT");
    fmd.setID("LUTID");
    const OCIO::FormatMetadata & cfmd = lut->getFormatMetadata();
    OCIO_CHECK_EQUAL(std::string(cfmd.getName()), "test LUT");
    OCIO_CHECK_EQUAL(std::string(cfmd.getID()), "LUTID");
}
