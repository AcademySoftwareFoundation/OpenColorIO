/*
Copyright (c) 2019 Autodesk Inc., et al.
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


namespace OCIO = OCIO_NAMESPACE;
#include "GPUUnitTest.h"

OCIO_ADD_GPU_TEST(ExposureContrast, style_linear_fwd)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LINEAR);
    ec->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    ec->setExposure(1.1);
    ec->setContrast(0.8);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(ec->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(ec->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(ec->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(ec->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(ec->createEditableCopy(), shaderDesc);

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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(ec->createEditableCopy(), shaderDesc);

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
    {
        OCIO::ConstProcessorRcPtr & processor = test.getProcessor();

        if(processor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE))
        {
            m_exposure = processor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE);
        }

        if(processor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST))
        {
            m_contrast = processor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST);
        }

        if(processor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA))
        {
            m_gamma = processor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA);
        }

        m_shaderDesc = test.getShaderDesc();
    }

protected:

    template<typename T>
    void updateUniform(OCIO::DynamicPropertyType type, T value)
    {
        // Update all the GPU dynamic properties.

        for(unsigned idx=0; idx<m_shaderDesc->getNumUniforms(); ++idx)
        {
            const char * name = nullptr;
            OCIO::DynamicPropertyRcPtr prop;

            m_shaderDesc->getUniform(idx, name, prop);
            if(prop->getType()==type && prop->isDynamic())
            {
                prop->setValue(value);
                return;
            }
        }
    }

    // Holder of all GPU information including dynamic properties.
    OCIO::GpuShaderDescRcPtr m_shaderDesc;

    // Keep dynamic property values for tests modifying their current value.
    OCIO::DynamicPropertyRcPtr m_exposure;
    OCIO::DynamicPropertyRcPtr m_contrast;
    OCIO::DynamicPropertyRcPtr m_gamma;
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

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(ec->createEditableCopy(), shaderDesc);

    class MyECRetest : public ECRetest
    {
    public:
        MyECRetest(OCIOGPUTest & test) : ECRetest(test) {}

        void retest1()
        {
            m_exposure->setValue(m_exposure->getDoubleValue() + 0.1);
            updateUniform(m_exposure->getType(), m_exposure->getDoubleValue());
        }
        void retest2()
        {
            m_contrast->setValue(m_contrast->getDoubleValue() + 0.1);
            updateUniform(m_contrast->getType(), m_contrast->getDoubleValue());
        }
        void retest3()
        {
            m_gamma->setValue(m_gamma->getDoubleValue() + 0.1);
            updateUniform(m_gamma->getType(), m_gamma->getDoubleValue());
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

    if (firstDyn) ec1->makeExposureDynamic();
    if (secondDyn) ec2->makeExposureDynamic();

    OCIO::GroupTransformRcPtr grp = OCIO::GroupTransform::Create();
    grp->push_back(ec1);
    grp->push_back(ec2);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
    test.setContext(grp->createEditableCopy(), shaderDesc);

    class MyECRetest : public ECRetest
    {
    public:
        MyECRetest(OCIOGPUTest & test) : ECRetest(test) {}

        void retest1()
        {
            m_exposure->setValue(1.1);
            updateUniform(m_exposure->getType(), m_exposure->getDoubleValue());
        }
        void retest2()
        {
            m_exposure->setValue(2.1);
            updateUniform(m_exposure->getType(), m_exposure->getDoubleValue());
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
