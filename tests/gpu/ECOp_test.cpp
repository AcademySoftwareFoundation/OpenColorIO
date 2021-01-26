// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>


#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_GPU_TEST(ExposureContrast, style_linear_fwd)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LINEAR);
    ec->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    ec->setExposure(1.1);
    ec->setContrast(0.8);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    test.setProcessor(ec);

    test.setErrorThreshold(2e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(ExposureContrast, style_linear_rev)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LINEAR);
    ec->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    ec->setExposure(1.1);
    ec->setContrast(0.7);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    test.setProcessor(ec);

    test.setErrorThreshold(2e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
}

OCIO_ADD_GPU_TEST(ExposureContrast, style_video_fwd)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_VIDEO);
    ec->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    ec->setExposure(1.1);
    ec->setContrast(0.8);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    test.setProcessor(ec);

    test.setErrorThreshold(2e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(ExposureContrast, style_video_rev)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_VIDEO);
    ec->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    ec->setExposure(1.1);
    ec->setContrast(0.7);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    test.setProcessor(ec);

    test.setErrorThreshold(2e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
}

OCIO_ADD_GPU_TEST(ExposureContrast, style_log_fwd)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);
    ec->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    ec->setExposure(1.1);
    ec->setContrast(0.8);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    test.setProcessor(ec);

    test.setErrorThreshold(1e-6f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
}

OCIO_ADD_GPU_TEST(ExposureContrast, style_log_rev)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);
    ec->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    ec->setExposure(1.1);
    ec->setContrast(0.7);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    test.setProcessor(ec);

    test.setErrorThreshold(1e-6f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
}

namespace
{

class ECRetest
{
public:
    ECRetest() = delete;

    ECRetest(OCIOGPUTest & test)
        : m_test(test)
    {
        // Testing infrastructure creates a new cpu processor for each retest (but keeps the
        // shader). Changing the dynamic property on the processor will be reflected in each
        // cpu processor. Initialize the dynamic properties for CPU. Shader is has not been
        // created yet.
        OCIO::ConstProcessorRcPtr & processor = test.getProcessor();

        if(processor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE))
        {
            auto dp = processor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);
            m_exposureCPU = OCIO::DynamicPropertyValue::AsDouble(dp);
        }

        if(processor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST))
        {
            auto dp = processor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST);
            m_contrastCPU = OCIO::DynamicPropertyValue::AsDouble(dp);
        }

        if(processor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA))
        {
            auto dp = processor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA);
            m_gammaCPU = OCIO::DynamicPropertyValue::AsDouble(dp);
        }
    }

protected:

    void initializeGPUDynamicProperties()
    {
        // Wait for the shader to be created to call this (first retest).
        // Shader is created once, thus updating the dynamic property on the processor will not
        // be reflected on the shader because dynamic properties are decoupled between processor
        // and shader.
        auto & shaderDesc = m_test.getShaderDesc();
        if (shaderDesc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE))
        {
            auto dp = shaderDesc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);
            m_exposureGPU = OCIO::DynamicPropertyValue::AsDouble(dp);
        }

        if (shaderDesc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST))
        {
            auto dp = shaderDesc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST);
            m_contrastGPU = OCIO::DynamicPropertyValue::AsDouble(dp);
        }

        if (shaderDesc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA))
        {
            auto dp = shaderDesc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA);
            m_gammaGPU = OCIO::DynamicPropertyValue::AsDouble(dp);
        }
    }

    OCIOGPUTest & m_test;
    // Keep dynamic property values for tests modifying their current value.
    OCIO::DynamicPropertyDoubleRcPtr m_exposureCPU;
    OCIO::DynamicPropertyDoubleRcPtr m_contrastCPU;
    OCIO::DynamicPropertyDoubleRcPtr m_gammaCPU;
    OCIO::DynamicPropertyDoubleRcPtr m_exposureGPU;
    OCIO::DynamicPropertyDoubleRcPtr m_contrastGPU;
    OCIO::DynamicPropertyDoubleRcPtr m_gammaGPU;
};

}

OCIO_ADD_GPU_TEST(ExposureContrast, style_linear_dynamic_parameter)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LINEAR);
    ec->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    ec->setExposure(1.1);
    ec->setContrast(0.8);
    ec->setGamma(0.9);
    ec->setPivot(0.22);
    ec->makeExposureDynamic();
    ec->makeContrastDynamic();
    ec->makeGammaDynamic();

    test.setProcessor(ec);

    class MyECRetest : public ECRetest
    {
    public:
        MyECRetest(OCIOGPUTest & test) : ECRetest(test) {}

        void retest1()
        {
            initializeGPUDynamicProperties();

            m_exposureCPU->setValue(m_exposureCPU->getValue() + 0.1);
            m_exposureGPU->setValue(m_exposureCPU->getValue());
        }
        void retest2()
        {
            m_contrastCPU->setValue(m_contrastCPU->getValue() + 0.1);
            m_contrastGPU->setValue(m_contrastCPU->getValue());
        }
        void retest3()
        {
            m_gammaCPU->setValue(m_gammaCPU->getValue() + 0.1);
            m_gammaGPU->setValue(m_gammaCPU->getValue());
        }
    };

    // Use shared_ptr so that object would stay until test is deleted.
    std::shared_ptr<MyECRetest> ecRetest = std::make_shared<MyECRetest>(test);

    // This adds a reference count to the shared_ptr.
    OCIOGPUTest::RetestSetupCallback f1 = std::bind(&MyECRetest::retest1, ecRetest);
    OCIOGPUTest::RetestSetupCallback f2 = std::bind(&MyECRetest::retest2, ecRetest);
    OCIOGPUTest::RetestSetupCallback f3 = std::bind(&MyECRetest::retest3, ecRetest);

    test.addRetest(f1);
    test.addRetest(f2);
    test.addRetest(f3);

    test.setErrorThreshold(5e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);

    test.setTestInfinity(false);
}

void Prepare2ECDynamic(OCIOGPUTest & test, bool firstDyn, bool secondDyn)
{
    // See also OCIO_ADD_TEST(ExposureContrastTransform, processor_several_ec).
    OCIO::ExposureContrastTransformRcPtr ec1 = OCIO::ExposureContrastTransform::Create();
    ec1->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);

    ec1->setExposure(0.8);
    ec1->setContrast(0.5);
    ec1->setGamma(1.5);

    OCIO::ExposureContrastTransformRcPtr ec2 = OCIO::ExposureContrastTransform::Create();
    ec2->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);

    ec2->setExposure(0.8);
    ec2->setContrast(0.5);
    ec2->setGamma(1.5);

    if (firstDyn) ec1->makeContrastDynamic();
    if (secondDyn) ec2->makeExposureDynamic();

    OCIO::GroupTransformRcPtr grp = OCIO::GroupTransform::Create();
    grp->appendTransform(ec1);
    grp->appendTransform(ec2);

    test.setProcessor(grp);

    class MyECRetest : public ECRetest
    {
    public:
        MyECRetest(OCIOGPUTest & test) : ECRetest(test) {}

        void retest1()
        {
            initializeGPUDynamicProperties();

            if (m_contrastCPU)
            {
                m_contrastCPU->setValue(0.5);
                m_contrastGPU->setValue(m_contrastCPU->getValue());
            }
            if (m_exposureCPU)
            {
                m_exposureCPU->setValue(1.1);
                m_exposureGPU->setValue(m_exposureCPU->getValue());
            }
        }
        void retest2()
        {
            if (m_contrastCPU)
            {
                m_contrastCPU->setValue(0.7);
                m_contrastGPU->setValue(m_contrastCPU->getValue());
            }
            if (m_exposureCPU)
            {
                m_exposureCPU->setValue(2.1);
                m_exposureGPU->setValue(m_exposureCPU->getValue());
            }
        }
    };

    // Use shared_ptr so that object would stay until test is deleted.
    std::shared_ptr<MyECRetest> ecRetest = std::make_shared<MyECRetest>(test);

    // This adds a reference count to the shared_ptr.
    OCIOGPUTest::RetestSetupCallback f1 = std::bind(&MyECRetest::retest1, ecRetest);
    OCIOGPUTest::RetestSetupCallback f2 = std::bind(&MyECRetest::retest2, ecRetest);

    test.addRetest(f1);
    test.addRetest(f2);

    test.setErrorThreshold(5e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(ExposureContrast, dp_several_one_dynamic)
{
    // 2 EC, first not dynamic, second dynamic.
    Prepare2ECDynamic(test, false, true);
}

OCIO_ADD_GPU_TEST(ExposureContrast, dp_several_both_dynamic)
{
    // 2 EC, both dynamic.
    Prepare2ECDynamic(test, true, true);
}
