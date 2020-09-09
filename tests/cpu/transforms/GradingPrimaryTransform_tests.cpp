// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "MathUtils.h"

#include "transforms/GradingPrimaryTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingPrimaryTransform, basic)
{
    OCIO_CHECK_EQUAL(OCIO::GradingPrimary::NoClampWhite(), std::numeric_limits<double>::max());
    OCIO_CHECK_EQUAL(OCIO::GradingPrimary::NoClampBlack(), -std::numeric_limits<double>::max());

    // Verify default data.
    const OCIO::GradingPrimary gdpLin(OCIO::GRADING_LIN);

    OCIO_CHECK_EQUAL(gdpLin.m_brightness, OCIO::GradingRGBM(0.0, 0.0, 0.0, 0.0));
    OCIO_CHECK_EQUAL(gdpLin.m_contrast  , OCIO::GradingRGBM(1.0, 1.0, 1.0, 1.0));
    OCIO_CHECK_EQUAL(gdpLin.m_gamma     , OCIO::GradingRGBM(1.0, 1.0, 1.0, 1.0));
    OCIO_CHECK_EQUAL(gdpLin.m_offset    , OCIO::GradingRGBM(0.0, 0.0, 0.0, 0.0));
    OCIO_CHECK_EQUAL(gdpLin.m_exposure  , OCIO::GradingRGBM(0.0, 0.0, 0.0, 0.0));
    OCIO_CHECK_EQUAL(gdpLin.m_lift      , OCIO::GradingRGBM(0.0, 0.0, 0.0, 0.0));
    OCIO_CHECK_EQUAL(gdpLin.m_gain      , OCIO::GradingRGBM(1.0, 1.0, 1.0, 1.0));
    OCIO_CHECK_EQUAL(gdpLin.m_pivot     , 0.18);
    OCIO_CHECK_EQUAL(gdpLin.m_saturation, 1.0);
    OCIO_CHECK_EQUAL(gdpLin.m_clampWhite, OCIO::GradingPrimary::NoClampWhite());
    OCIO_CHECK_EQUAL(gdpLin.m_clampBlack, OCIO::GradingPrimary::NoClampBlack());
    OCIO_CHECK_EQUAL(gdpLin.m_pivotWhite, 1.0);
    OCIO_CHECK_EQUAL(gdpLin.m_pivotBlack, 0.0);

    // Log defaults only differ lin defaults by pivot value.
    const OCIO::GradingPrimary gdpLog(OCIO::GRADING_LOG);
    OCIO_CHECK_NE(gdpLog, gdpLin);
    OCIO_CHECK_EQUAL(gdpLog.m_pivot, -0.2);
    OCIO::GradingPrimary gdpLogEdit{ gdpLog };
    OCIO_CHECK_EQUAL(gdpLog, gdpLogEdit);
    gdpLogEdit.m_pivot = gdpLin.m_pivot;
    OCIO_CHECK_EQUAL(gdpLogEdit, gdpLin);

    // Video defaults are the same as lin defaults.
    const OCIO::GradingPrimary gdpVid(OCIO::GRADING_VIDEO);
    OCIO_CHECK_EQUAL(gdpVid, gdpLin);

    // Create transform and validate default values for all styles.
    auto gptLin = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gptLin->getStyle(), OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gptLin->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(gptLin->getValue(), gdpLin);
    OCIO_CHECK_NO_THROW(gptLin->validate());

    auto gptLog = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gptLog->getStyle(), OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gptLog->getValue(), gdpLog);
    OCIO_CHECK_NO_THROW(gptLog->validate());

    auto gptVid = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_VIDEO);
    OCIO_CHECK_EQUAL(gptVid->getStyle(), OCIO::GRADING_VIDEO);
    OCIO_CHECK_EQUAL(gptVid->getValue(), gdpVid);
    OCIO_CHECK_NO_THROW(gptVid->validate());

    // Create editable copy and change values.
    auto t = gptLin->createEditableCopy();
    auto gpt = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingPrimaryTransform>(t);
    OCIO_CHECK_EQUAL(gpt->getStyle(), OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gpt->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(gpt->getValue(), gdpLin);

    gpt->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(gpt->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    gpt->setStyle(OCIO::GRADING_VIDEO);
    OCIO_CHECK_EQUAL(gpt->getStyle(), OCIO::GRADING_VIDEO);

    auto v = gpt->getValue();
    v.m_pivot = 0.24;
    gpt->setValue(v);
    OCIO_CHECK_EQUAL(gpt->getValue().m_pivot, 0.24);

    gpt->setStyle(OCIO::GRADING_LOG);
    gpt->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    const OCIO::GradingRGBM gammaInvalid{ 0.00001, 1.0, 1.0, 1.0 };
    v.m_gamma = gammaInvalid;
    OCIO_CHECK_THROW_WHAT(gpt->setValue(v), OCIO::Exception,
                          "GradingPrimary gamma '<r=1e-05, g=1, b=1, m=1>' "
                          "are below lower bound (0.01)");
}

OCIO_ADD_TEST(GradingPrimaryTransform, dynamic)
{
    auto gpt = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LOG);

    OCIO_CHECK_ASSERT(!gpt->isDynamic());
    gpt->makeDynamic();
    OCIO_CHECK_ASSERT(gpt->isDynamic());
}

OCIO_ADD_TEST(GradingPrimaryTransform, processor_several_transforms)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    const float srcPixel[3] = { 0.2f, 0.3f, 0.4f };

    OCIO::GradingPrimary gpa(OCIO::GRADING_LOG);
    gpa.m_gamma = OCIO::GradingRGBM(1.1, 1.2, 1.3, 1.0);
    auto gpta = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LOG);
    gpta->setValue(gpa);

    // Will hold results for gpa.
    float pixel_a[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
    // Will hold results for gpa applied twice.
    float pixel_aa[3];
    {
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(gpta);
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
        cpuProcessor->applyRGB(pixel_a);

        pixel_aa[0] = pixel_a[0];
        pixel_aa[1] = pixel_a[1];
        pixel_aa[2] = pixel_a[2];
        cpuProcessor->applyRGB(pixel_aa);
    }

    OCIO::GradingPrimary gpb(OCIO::GRADING_LOG);
    gpb.m_gamma = OCIO::GradingRGBM(1.2, 1.4, 1.1, 1.0);
    gpb.m_saturation = 1.5;
    auto gptb = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LOG);
    gptb->setValue(gpb);

    // Will hold results for gpb.
    float pixel_b[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
    // Will hold results for gpb applied twice.
    float pixel_bb[3];
    // Will hold results for gpa applied then gpb applied.
    float pixel_ab[3] = { pixel_a[0], pixel_a[1], pixel_a[2] };
    {
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(gptb);
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
        cpuProcessor->applyRGB(pixel_b);

        pixel_bb[0] = pixel_b[0];
        pixel_bb[1] = pixel_b[1];
        pixel_bb[2] = pixel_b[2];

        cpuProcessor->applyRGB(pixel_bb);
        cpuProcessor->applyRGB(pixel_ab);
    }

    // Make second transform dynamic.
    gptb->makeDynamic();
    static constexpr float error = 1e-6f;

    //
    // Test with two grading primary transforms where only the second one is dynamic.
    //
    OCIO::GroupTransformRcPtr grp1 = OCIO::GroupTransform::Create();
    gptb->setValue(gpa);
    grp1->appendTransform(gpta);  // gpta values are gpa
    grp1->appendTransform(gptb);  // gptb values are gpa

    {
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(grp1);
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();

        // Second transform is dynamic. Value is still gpa.
        OCIO::DynamicPropertyRcPtr dp;
        OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY));
        auto dpVal = OCIO::DynamicPropertyValue::AsGradingPrimary(dp);
        OCIO_REQUIRE_ASSERT(dpVal);

        float pixel[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };

        // Apply gpa then gpa.
        cpuProcessor->applyRGB(pixel);

        OCIO_CHECK_CLOSE(pixel[0], pixel_aa[0], error);
        OCIO_CHECK_CLOSE(pixel[1], pixel_aa[1], error);
        OCIO_CHECK_CLOSE(pixel[2], pixel_aa[2], error);

        // Change the 2nd values to gpb.
        dpVal->setValue(gpb);
        pixel[0] = srcPixel[0];
        pixel[1] = srcPixel[1];
        pixel[2] = srcPixel[2];

        // Apply gpa then gpb.
        cpuProcessor->applyRGB(pixel);

        OCIO_CHECK_CLOSE(pixel[0], pixel_ab[0], error);
        OCIO_CHECK_CLOSE(pixel[1], pixel_ab[1], error);
        OCIO_CHECK_CLOSE(pixel[2], pixel_ab[2], error);
    }

    // To start, make only the first transform dynamic.
    gpta->makeDynamic();

    //
    // Test with two grading primary transforms where both are dynamic.
    //
    OCIO::GroupTransformRcPtr grp2 = OCIO::GroupTransform::Create();
    grp2->appendTransform(gpta);
    grp2->appendTransform(gptb);

    // Change both values (will not be used).
    gpta->setValue(OCIO::GradingPrimary(OCIO::GRADING_LIN));
    gptb->setValue(OCIO::GradingPrimary(OCIO::GRADING_LIN));
    {
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(grp2);
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();

        // The dynamic property is common to both ops.
        OCIO::DynamicPropertyRcPtr dp;
        OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY));
        auto dpVal = OCIO::DynamicPropertyValue::AsGradingPrimary(dp);
        OCIO_REQUIRE_ASSERT(dpVal);

        float pixel[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };

        // Change both grading primary to gpa.
        dpVal->setValue(gpa);

        // Apply gpa twice.
        cpuProcessor->applyRGB(pixel);

        OCIO_CHECK_CLOSE(pixel[0], pixel_aa[0], error);
        OCIO_CHECK_CLOSE(pixel[1], pixel_aa[1], error);
        OCIO_CHECK_CLOSE(pixel[2], pixel_aa[2], error);

        // Changing the dynamic property is changing both values to gpb.
        dpVal->setValue(gpb);
        pixel[0] = srcPixel[0];
        pixel[1] = srcPixel[1];
        pixel[2] = srcPixel[2];

        // Apply gpb twice.
        cpuProcessor->applyRGB(pixel);

        OCIO_CHECK_CLOSE(pixel[0], pixel_bb[0], error);
        OCIO_CHECK_CLOSE(pixel[1], pixel_bb[1], error);
        OCIO_CHECK_CLOSE(pixel[2], pixel_bb[2], error);
    }
}

OCIO_ADD_TEST(GradingPrimaryTransform, several_transforms_switch)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    const float srcPixel[3] = { 0.2f, 0.3f, 0.4f };

    OCIO::GradingPrimary gpa(OCIO::GRADING_LOG);
    gpa.m_gamma = OCIO::GradingRGBM(1.1, 1.2, 1.3, 1.0);
    auto gpta = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LOG);
    gpta->setValue(gpa);

    OCIO::GradingPrimary gpb(OCIO::GRADING_LOG);
    gpb.m_gamma = OCIO::GradingRGBM(1.2, 1.4, 1.1, 1.0);
    gpb.m_saturation = 1.5;
    auto gptb = OCIO::GradingPrimaryTransform::Create(OCIO::GRADING_LOG);
    gptb->setValue(gpb);

    // To start, make only the first transform dynamic.
    gpta->makeDynamic();

    //
    // Test with two grading primary transforms where first is dynamic.
    //
    OCIO::GroupTransformRcPtr grp = OCIO::GroupTransform::Create();
    grp->appendTransform(gpta);
    grp->appendTransform(gptb);

    OCIO::ConstProcessorRcPtr processor = config->getProcessor(grp);
    OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();

    static constexpr float error = 1e-6f;
    float pixel[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
    {
        // The dynamic property is for the first op.
        OCIO::DynamicPropertyRcPtr dp;
        OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY));
        auto dpVal = OCIO::DynamicPropertyValue::AsGradingPrimary(dp);
        OCIO_REQUIRE_ASSERT(dpVal);

        // Change one property and apply to the source pixel.
        auto val = dpVal->getValue();
        val.m_saturation += 0.15;
        dpVal->setValue(val);

        float temp[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
        cpuProcessor->applyRGB(temp);

        // Change additional properties.
        val.m_brightness.m_red += 0.11;
        val.m_gamma.m_master += 0.123456;
        dpVal->setValue(val);

        // Apply to the same source pixel (and keep results).
        cpuProcessor->applyRGB(pixel);

        // Additional properties were modified so the temp and pixel results are different.
        OCIO_CHECK_ASSERT(!OCIO::VecsEqualWithRelError(temp, 3, pixel, 3, error));
    }

    // Now make the first transform non-dynamic and the second transform dynamic.  This is what
    // an application would need to do in order to edit multiple different dynamic transforms.

    {
        // Get dynamic property from the current cpuProcessor, to retrieve the current value.
        OCIO::DynamicPropertyRcPtr dp;
        OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY));
        auto dpVal = OCIO::DynamicPropertyValue::AsGradingPrimary(dp);
        OCIO_REQUIRE_ASSERT(dpVal);
        auto val = dpVal->getValue();

        // Copy the newly edited property values back to the first transform.
        gpta->setValue(val);

        // Swap which transform is dynamic.
        gpta->makeNonDynamic();
        gptb->makeDynamic();
    }

    // Create a new processor (since it is not possible to make a transform dynamic after the
    // processor is created).
    grp = OCIO::GroupTransform::Create();
    grp->appendTransform(gpta);
    grp->appendTransform(gptb);

    processor = config->getProcessor(grp);
    cpuProcessor = processor->getDefaultCPUProcessor();

    float pixel2[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
    {
        // The application is now able to edit the second transform.  The first transform has been
        // updated to reflect the editing done above.

        cpuProcessor->applyRGB(pixel2);

        // Check that the new processor is getting the same result as the latest edits on the
        // previous processor.
        OCIO_CHECK_CLOSE(pixel[0], pixel2[0], error);
        OCIO_CHECK_CLOSE(pixel[1], pixel2[1], error);
        OCIO_CHECK_CLOSE(pixel[2], pixel2[2], error);

        // Get a pointer to the dynamic property in the processor, this time it is on the second
        // transform.
        OCIO::DynamicPropertyRcPtr dp;
        OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY));
        auto dpVal = OCIO::DynamicPropertyValue::AsGradingPrimary(dp);
        OCIO_REQUIRE_ASSERT(dpVal);

        // Change properties.
        auto val = dpVal->getValue();
        val.m_saturation += 0.15;
        dpVal->setValue(val);

        float temp[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
        cpuProcessor->applyRGB(temp);

        // Additional properties were modified so the temp and pixel results are different.
        OCIO_CHECK_ASSERT(!OCIO::VecsEqualWithRelError(temp, 3, pixel, 3, error));
    }
}
