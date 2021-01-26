// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/ExposureContrastTransform.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(ExposureContrastTransform, basic)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    OCIO_CHECK_EQUAL(ec->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(ec->getStyle(), OCIO::EXPOSURE_CONTRAST_LINEAR);
    OCIO_CHECK_EQUAL(ec->getExposure(), 0.0);
    OCIO_CHECK_EQUAL(ec->getContrast(), 1.0);
    OCIO_CHECK_EQUAL(ec->getGamma(), 1.0);
    OCIO_CHECK_EQUAL(ec->getPivot(), 0.18);
    OCIO_CHECK_EQUAL(ec->getLogExposureStep(),
                     OCIO::ExposureContrastOpData::LOGEXPOSURESTEP_DEFAULT);
    OCIO_CHECK_EQUAL(ec->getLogMidGray(),
                     OCIO::ExposureContrastOpData::LOGMIDGRAY_DEFAULT);
    OCIO_CHECK_NO_THROW(ec->validate());

    OCIO_CHECK_NO_THROW(ec->setDirection(OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_EQUAL(ec->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_NO_THROW(ec->validate());

    OCIO_CHECK_NO_THROW(ec->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC));
    OCIO_CHECK_EQUAL(ec->getStyle(), OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);
    OCIO_CHECK_NO_THROW(ec->validate());

    OCIO_CHECK_NO_THROW(ec->setStyle(OCIO::EXPOSURE_CONTRAST_VIDEO));
    OCIO_CHECK_EQUAL(ec->getStyle(), OCIO::EXPOSURE_CONTRAST_VIDEO);
    OCIO_CHECK_NO_THROW(ec->validate());
}

OCIO_ADD_TEST(ExposureContrastTransform, processor)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    OCIO_CHECK_NO_THROW(ec->setStyle(OCIO::EXPOSURE_CONTRAST_VIDEO));
    ec->setExposure(1.1);
    ec->makeExposureDynamic();
    ec->setContrast(0.5);
    ec->setGamma(1.5);

    OCIO::ConstProcessorRcPtr processor = config->getProcessor(ec);
    OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();

    float pixel[3] = { 0.2f, 0.3f, 0.4f };
    cpuProcessor->applyRGB(pixel);

    const float error = 1e-5f;
    OCIO_CHECK_CLOSE(pixel[0], 0.32340f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.43834f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.54389f, error);

    // Changing the original transform does not change the processor.
    ec->setExposure(2.1);

    pixel[0] = 0.2f;
    pixel[1] = 0.3f;
    pixel[2] = 0.4f;
    cpuProcessor->applyRGB(pixel);
    OCIO_CHECK_CLOSE(pixel[0], 0.32340f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.43834f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.54389f, error);

    OCIO::DynamicPropertyRcPtr dpExposure;
    OCIO_CHECK_NO_THROW(dpExposure = cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
    auto dpVal = OCIO_DYNAMIC_POINTER_CAST<OCIO::DynamicPropertyDouble>(dpExposure);
    OCIO_REQUIRE_ASSERT(dpVal);
    dpVal->setValue(2.1);

    // Gamma is a property of ExposureContrast but here it is not defined as dynamic.
    OCIO_CHECK_THROW(cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA), OCIO::Exception);

    // Processor has been changed by dpExposure.
    pixel[0] = 0.2f;
    pixel[1] = 0.3f;
    pixel[2] = 0.4f;
    cpuProcessor->applyRGB(pixel);
    OCIO_CHECK_CLOSE(pixel[0], 0.42965f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.58235f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.72258f, error);

    // dpExposure can be used to change the processor.
    dpVal->setValue(0.8);
    pixel[0] = 0.2f;
    pixel[1] = 0.3f;
    pixel[2] = 0.4f;
    cpuProcessor->applyRGB(pixel);
    OCIO_CHECK_CLOSE(pixel[0], 0.29698f, error);
    // Adjust error for SSE approximation.
    OCIO_CHECK_CLOSE(pixel[1], 0.40252f, error*2.0f);
    OCIO_CHECK_CLOSE(pixel[2], 0.49946f, error);
}

// This test verifies that if there are several ops in a processor that contain
// a given dynamic property that
// 1) Only the op for which the dynamic property is enabled responds to changes in the property,
//    and
// 2) Ops where a given dynamic property is not enabled continue to use the initial value and do
//    not respond to changes to the property, and
// 3) If two ops enable the same dynamic property an exception is thrown.
OCIO_ADD_TEST(ExposureContrastTransform, processor_several_ec)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    //
    // Build expected values using 2 E/C with no dynamic parameters.
    //

    // 2 different exposure values.
    const double a = 1.1;
    const double b = 2.1;
    OCIO::ExposureContrastTransformRcPtr ec1 = OCIO::ExposureContrastTransform::Create();
    OCIO_CHECK_NO_THROW(ec1->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC));

    ec1->setExposure(a);
    ec1->setContrast(0.5);
    ec1->setGamma(1.5);

    const float srcPixel[3] = { 0.2f, 0.3f, 0.4f };

    // Will hold results for exposure a.
    float pixel_a[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
    // Will hold results for exposure a applied twice.
    float pixel_aa[3];
    {
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(ec1);
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
        cpuProcessor->applyRGB(pixel_a);

        pixel_aa[0] = pixel_a[0];
        pixel_aa[1] = pixel_a[1];
        pixel_aa[2] = pixel_a[2];
        cpuProcessor->applyRGB(pixel_aa);
    }

    OCIO::ExposureContrastTransformRcPtr ec2 = OCIO::ExposureContrastTransform::Create();
    OCIO_CHECK_NO_THROW(ec2->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC));

    ec2->setExposure(b);
    ec2->setContrast(0.5);
    ec2->setGamma(1.5);

    // Will hold results for exposure b.
    float pixel_b[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
    // Will hold results for exposure b applied twice.
    float pixel_bb[3];
    // Will hold results for exposure a applied then exposure b applied.
    float pixel_ab[3] = { pixel_a[0], pixel_a[1], pixel_a[2] };
    {
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(ec2);
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
        cpuProcessor->applyRGB(pixel_b);

        pixel_bb[0] = pixel_b[0];
        pixel_bb[1] = pixel_b[1];
        pixel_bb[2] = pixel_b[2];

        cpuProcessor->applyRGB(pixel_bb);
        cpuProcessor->applyRGB(pixel_ab);
    }

    // Make exposure of second E/C dynamic.
    ec2->makeExposureDynamic();
    const float error = 1e-6f;

    //
    // Test with two E/C where only the second one is dynamic.
    //
    OCIO::GroupTransformRcPtr grp1 = OCIO::GroupTransform::Create();
    ec2->setExposure(a);
    grp1->appendTransform(ec1);  // ec1 exposure is a
    grp1->appendTransform(ec2);  // ec2 exposure is a

    {
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(grp1);
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();

        // Second exposure is dynamic. Value is still a.
        OCIO::DynamicPropertyRcPtr dpExposure;
        OCIO_CHECK_NO_THROW(dpExposure = cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
        auto dpVal = OCIO_DYNAMIC_POINTER_CAST<OCIO::DynamicPropertyDouble>(dpExposure);
        OCIO_REQUIRE_ASSERT(dpVal);

        float pixel[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };

        // Apply a then a.
        cpuProcessor->applyRGB(pixel);

        OCIO_CHECK_CLOSE(pixel[0], pixel_aa[0], error);
        OCIO_CHECK_CLOSE(pixel[1], pixel_aa[1], error);
        OCIO_CHECK_CLOSE(pixel[2], pixel_aa[2], error);

        // Change the 2nd exposure.
        dpVal->setValue(b);
        pixel[0] = srcPixel[0];
        pixel[1] = srcPixel[1];
        pixel[2] = srcPixel[2];

        // Apply a then b.
        cpuProcessor->applyRGB(pixel);

        OCIO_CHECK_CLOSE(pixel[0], pixel_ab[0], error);
        OCIO_CHECK_CLOSE(pixel[1], pixel_ab[1], error);
        OCIO_CHECK_CLOSE(pixel[2], pixel_ab[2], error);
    }

    //
    // Test with two E/C where both are dynamic.
    //

    {
        // Make exposure of first E/C dynamic (second one already is).
        ec1->makeExposureDynamic();

        OCIO::GroupTransformRcPtr grp2 = OCIO::GroupTransform::Create();
        grp2->appendTransform(ec1);
        grp2->appendTransform(ec2);

        OCIO::LogGuard log;
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);
        OCIO_CHECK_NO_THROW(config->getProcessor(grp2));
        OCIO_CHECK_EQUAL(log.output(), "[OpenColorIO Warning]: Exposure dynamic property can "
                                       "only be there once.\n");
    }
}

