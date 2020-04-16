// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include "DynamicProperty.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(DynamicPropertyImpl, basic)
{
    OCIO::DynamicPropertyRcPtr dp =
        std::make_shared<OCIO::DynamicPropertyImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO_REQUIRE_ASSERT(dp);
    OCIO_REQUIRE_EQUAL(dp->getDoubleValue(), 1.0);
    dp->setValue(2.0);
    OCIO_CHECK_EQUAL(dp->getDoubleValue(), 2.0);

    OCIO::DynamicPropertyImplRcPtr dpImpl =
        std::make_shared<OCIO::DynamicPropertyImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO_REQUIRE_ASSERT(dpImpl);
    OCIO_CHECK_ASSERT(!dpImpl->isDynamic());
    OCIO_CHECK_EQUAL(dpImpl->getDoubleValue(), 1.0);

    dpImpl->makeDynamic();
    OCIO_CHECK_ASSERT(dpImpl->isDynamic());
    dpImpl->setValue(2.0);
    OCIO_CHECK_EQUAL(dpImpl->getDoubleValue(), 2.0);
}

OCIO_ADD_TEST(DynamicPropertyImpl, equal)
{
    OCIO::DynamicPropertyImplRcPtr dpImpl0 =
        std::make_shared<OCIO::DynamicPropertyImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO::DynamicPropertyRcPtr dp0 = dpImpl0;

    OCIO::DynamicPropertyImplRcPtr dpImpl1 =
        std::make_shared<OCIO::DynamicPropertyImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO::DynamicPropertyRcPtr dp1 = dpImpl1;

    // Both not dynamic, same value.
    OCIO_CHECK_ASSERT(*dp0 == *dp1);

    // Both not dynamic, diff values.
    dp0->setValue(2.0);
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Same value.
    dp1->setValue(2.0);
    OCIO_CHECK_ASSERT(*dp0 == *dp1);

    // One dynamic, not the other, same value.
    dpImpl0->makeDynamic();
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Both dynamic, same value.
    dpImpl1->makeDynamic();
    OCIO_CHECK_ASSERT(*dp0 == *dp1);

    // Both dynamic, diffent values.
    dp1->setValue(3.0);
    OCIO_CHECK_ASSERT(*dp0 == *dp1);
}

namespace
{
OCIO::ConstProcessorRcPtr LoadTransformFile(const std::string & fileName)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir()) + "/"
        + fileName);
    // Create a FileTransform.
    OCIO::FileTransformRcPtr pFileTransform
        = OCIO::FileTransform::Create();
    // A transform file does not define any interpolation (contrary to config
    // file), this is to avoid exception when creating the operation.
    pFileTransform->setInterpolation(OCIO::INTERP_LINEAR);
    pFileTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    pFileTransform->setSrc(filePath.c_str());

    // Create empty Config to use.
    OCIO::ConfigRcPtr pConfig = OCIO::Config::Create();

    // Get the processor corresponding to the transform.
    return pConfig->getProcessor(pFileTransform);
}
}

// Test several aspects of dynamic properties, especially the ability to set
// values via the processor.
OCIO_ADD_TEST(DynamicProperty, get_dynamic_via_processor)
{
    const std::string ctfFile("exposure_contrast_video_dp.ctf");

    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = LoadTransformFile(ctfFile));

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = processor->getDefaultCPUProcessor());

    float pixel[3] = { 0.5f, 0.4f, 0.2f };
    cpuProcessor->applyRGB(pixel);

    float error = 1e-5f;
    OCIO_CHECK_CLOSE(pixel[0], 0.57495f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.43988f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.19147f, error);

    const OCIO::DynamicPropertyType dpt = OCIO::DYNAMIC_PROPERTY_EXPOSURE;
    OCIO::DynamicPropertyRcPtr dp;
    OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(dpt));
    const double fileValue = dp->getDoubleValue();
    dp->setValue(0.4);

    pixel[0] = 0.5f;
    pixel[1] = 0.4f;
    pixel[2] = 0.2f;
    cpuProcessor->applyRGB(pixel);

    // Adjust error for SSE approximation.
    OCIO_CHECK_CLOSE(pixel[0], 0.62966f, error*2.0f);
    OCIO_CHECK_CLOSE(pixel[1], 0.48175f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.20969f, error);

    // Restore file value.
    dp->setValue(fileValue);

    pixel[0] = 0.5f;
    pixel[1] = 0.4f;
    pixel[2] = 0.2f;
    cpuProcessor->applyRGB(pixel);

    OCIO_CHECK_CLOSE(pixel[0], 0.57495f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.43988f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.19147f, error);

    // Note: The CTF does not define gamma as being dynamic.
    OCIO_CHECK_THROW_WHAT(cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA),
                          OCIO::Exception,
                          "Cannot find dynamic property");

    // Get optimized CPU processor without dynamic properties.
    OCIO_CHECK_NO_THROW(cpuProcessor = processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_ALL));

    // Now the dynamic property can't be found.
    OCIO_CHECK_THROW_WHAT(cpuProcessor->getDynamicProperty(dpt),
                          OCIO::Exception,
                          "Cannot find dynamic property");

    // Using GPUProcessor.
    OCIO::ConstGPUProcessorRcPtr gpuProcessor;

    // Default optimization keeps dynamic properties.
    OCIO_CHECK_NO_THROW(gpuProcessor = processor->getDefaultGPUProcessor());
    OCIO_CHECK_NO_THROW(dp = gpuProcessor->getDynamicProperty(dpt));
    OCIO_CHECK_ASSERT(dp);

    // Get optimized GPU processor without dynamic properties.
    OCIO_CHECK_NO_THROW(gpuProcessor = processor->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_ALL));
    OCIO_CHECK_THROW_WHAT(gpuProcessor->getDynamicProperty(dpt),
                          OCIO::Exception,
                          "Cannot find dynamic property");

}

