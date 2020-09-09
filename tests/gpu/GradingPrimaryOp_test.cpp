// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

namespace GPTest1
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
static const OCIO::GradingRGBM brightness{ -10., 45., -5.,  50. };
static const OCIO::GradingRGBM contrast{ 0.9, 1.4,  0.7,  0.75 };
static const OCIO::GradingRGBM gamma{ 1.1, 0.7,  1.05, 1.15 };
static constexpr double saturation = 1.21;
static constexpr double pivot      = -0.3;
static constexpr double pivotBlack = 0.05;
static constexpr double pivotWhite = 0.9;
static constexpr double clampBlack = -0.05;
static constexpr double clampWhite = 1.50;
}

void GradingPrimaryLog(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto gp = OCIO::GradingPrimaryTransform::Create(GPTest1::style);
    gp->setDirection(dir);
    if (dynamic)
    {
        gp->makeDynamic();
    }

    OCIO::GradingPrimary gplog{ GPTest1::style };
    gplog.m_brightness = GPTest1::brightness;
    gplog.m_contrast   = GPTest1::contrast;
    gplog.m_gamma      = GPTest1::gamma;
    gplog.m_saturation = GPTest1::saturation;
    gplog.m_pivot      = GPTest1::pivot;
    gplog.m_pivotBlack = GPTest1::pivotBlack;
    gplog.m_pivotWhite = GPTest1::pivotWhite;
    gplog.m_clampBlack = GPTest1::clampBlack;
    gplog.m_clampWhite = GPTest1::clampWhite;
    gp->setValue(gplog);
    test.setProcessor(gp);

    test.setErrorThreshold(2e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_log_fwd)
{
    GradingPrimaryLog(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_log_fwd_dynamic)
{
    GradingPrimaryLog(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_log_rev)
{
    GradingPrimaryLog(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_log_rev_dynamic)
{
    GradingPrimaryLog(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GPTest2
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LIN;
static const OCIO::GradingRGBM exposure{ 0.5,  -0.2,  0.4, -0.25 };
static const OCIO::GradingRGBM offset{ -0.03,  0.02, 0.1, -0.1 };
static const OCIO::GradingRGBM contrast{ 0.9,   1.4,  0.7,  0.75 };
static constexpr double saturation = 1.33;
static constexpr double pivot      = 0.5;
static constexpr double clampBlack = -0.40;
static constexpr double clampWhite = 1.05;
}

void GradingPrimaryLin(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto gp = OCIO::GradingPrimaryTransform::Create(GPTest2::style);
    gp->setDirection(dir);
    if (dynamic)
    {
        gp->makeDynamic();
    }

    OCIO::GradingPrimary gplin{ GPTest2::style };
    gplin.m_exposure   = GPTest2::exposure;
    gplin.m_contrast   = GPTest2::contrast;
    gplin.m_offset     = GPTest2::offset;
    gplin.m_pivot      = GPTest2::pivot;
    gplin.m_saturation = GPTest2::saturation;
    gplin.m_clampBlack = GPTest2::clampBlack;
    gplin.m_clampWhite = GPTest2::clampWhite;
    gp->setValue(gplin);
    test.setProcessor(gp);

    test.setErrorThreshold(2e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_lin_fwd)
{
    GradingPrimaryLin(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_lin_fwd_dynamic)
{
    GradingPrimaryLin(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_lin_rev)
{
    GradingPrimaryLin(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_lin_rev_dynamic)
{
    GradingPrimaryLin(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GPTest3
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LIN;
static const OCIO::GradingRGBM lift{ 0.05, -0.04, 0.02, 0.05 };
static const OCIO::GradingRGBM gamma{ 0.9,   1.4,  0.7,  0.75 };
static const OCIO::GradingRGBM gain{ 1.2,   1.1,  1.25, 0.8 };
static const OCIO::GradingRGBM offset{ -0.03,  0.02, 0.1, -0.1 };
static constexpr double saturation = 1.2;
static constexpr double pivotBlack = 0.05;
static constexpr double pivotWhite = 0.9;
static constexpr double clampBlack = -0.15;
static constexpr double clampWhite = 1.50;
}

void GradingPrimaryVideo(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto gp = OCIO::GradingPrimaryTransform::Create(GPTest3::style);
    gp->setDirection(dir);
    if (dynamic)
    {
        gp->makeDynamic();
    }

    OCIO::GradingPrimary gpvideo{ GPTest3::style };
    gpvideo.m_lift       = GPTest3::lift;
    gpvideo.m_gamma      = GPTest3::gamma;
    gpvideo.m_gain       = GPTest3::gain;
    gpvideo.m_offset     = GPTest3::offset;
    gpvideo.m_saturation = GPTest3::saturation;
    gpvideo.m_clampBlack = GPTest3::clampBlack;
    gpvideo.m_clampWhite = GPTest3::clampWhite;
    gpvideo.m_pivotBlack = GPTest3::pivotBlack;
    gpvideo.m_pivotWhite = GPTest3::pivotWhite;
    gp->setValue(gpvideo);
    test.setProcessor(gp);

    test.setErrorThreshold(3e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_video_fwd)
{
    GradingPrimaryVideo(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_video_fwd_dynamic)
{
    GradingPrimaryVideo(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_video_rev)
{
    GradingPrimaryVideo(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingPrimary, style_video_rev_dynamic)
{
    GradingPrimaryVideo(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace
{

class GPRetest
{
public:
    GPRetest() = delete;

    GPRetest(OCIOGPUTest & test)
        : m_test(test)
    {
        // Testing infrastructure creates a new cpu processor for each retest (but keeps the
        // shader). Changing the dynamic property on the processor will be reflected in each
        // cpu processor. Initialize the dynamic properties for CPU. Shader is has not been
        // created yet.
        OCIO::ConstProcessorRcPtr & processor = test.getProcessor();

        if (processor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY))
        {
            auto dp = processor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY);
            m_dynProp = OCIO::DynamicPropertyValue::AsGradingPrimary(dp);
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
        if (shaderDesc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY))
        {
            auto dp = shaderDesc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY);
            m_dynPropGPU = OCIO::DynamicPropertyValue::AsGradingPrimary(dp);
        }
    }

    OCIOGPUTest & m_test;
    // Keep dynamic property values for tests modifying their current value.
    OCIO::DynamicPropertyGradingPrimaryRcPtr m_dynProp;
    OCIO::DynamicPropertyGradingPrimaryRcPtr m_dynPropGPU;
};

}

// Test that a dynamic property may be updated and that the result still matches the CPU.
OCIO_ADD_GPU_TEST(GradingPrimary, style_log_dynamic_retests)
{
    OCIO::GradingPrimaryTransformRcPtr gp = OCIO::GradingPrimaryTransform::Create(GPTest1::style);
    gp->makeDynamic();

    OCIO::GradingPrimary gplog{ GPTest1::style };
    gplog.m_brightness = GPTest1::brightness;
    gplog.m_contrast   = GPTest1::contrast;
    gplog.m_gamma      = GPTest1::gamma;
    gplog.m_saturation = GPTest1::saturation;
    gplog.m_pivot      = GPTest1::pivot;
    gp->setValue(gplog);
    test.setProcessor(gp);

    class MyGPRetest : public GPRetest
    {
    public:
        MyGPRetest(OCIOGPUTest & test) : GPRetest(test)
        {
        }

        void retest_clamp()
        {
            initializeGPUDynamicProperties();
            auto vals = m_dynProp->getValue();
            vals.m_clampBlack = GPTest1::clampBlack;
            vals.m_clampWhite = GPTest1::clampWhite;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }

        void retest_pivot()
        {
            auto vals = m_dynProp->getValue();
            vals.m_clampBlack = -100;
            vals.m_clampWhite = 100;
            vals.m_pivotBlack = GPTest1::pivotBlack;
            vals.m_pivotWhite = GPTest1::pivotWhite;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);

        }
        void retest1()
        {
            auto vals = m_dynProp->getValue();
            vals.m_gamma.m_red += 0.1;
            vals.m_gamma.m_master += 0.1;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest2()
        {
            auto vals = m_dynProp->getValue();
            vals.m_saturation += 0.1;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest3()
        {
            auto vals = m_dynProp->getValue();
            vals.m_clampWhite += 1.;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest4()
        {
            OCIO::GradingPrimary identity{ GPTest1::style };
            m_dynProp->setValue(identity);
            m_dynPropGPU->setValue(identity);
        }
    };

    // Use shared_ptr so that object would stay until test is deleted.
    std::shared_ptr<MyGPRetest> gpRetest = std::make_shared<MyGPRetest>(test);

    // This adds a reference count to the shared_ptr.
    OCIOGPUTest::RetestSetupCallback f_clamp = std::bind(&MyGPRetest::retest_clamp, gpRetest);
    OCIOGPUTest::RetestSetupCallback f_pivot = std::bind(&MyGPRetest::retest_pivot, gpRetest);
    OCIOGPUTest::RetestSetupCallback f1 = std::bind(&MyGPRetest::retest1, gpRetest);
    OCIOGPUTest::RetestSetupCallback f2 = std::bind(&MyGPRetest::retest2, gpRetest);
    OCIOGPUTest::RetestSetupCallback f3 = std::bind(&MyGPRetest::retest3, gpRetest);
    OCIOGPUTest::RetestSetupCallback f4 = std::bind(&MyGPRetest::retest4, gpRetest);

    test.addRetest(f_clamp);
    test.addRetest(f_pivot);
    test.addRetest(f1);
    test.addRetest(f2);
    test.addRetest(f3);
    test.addRetest(f4);

    test.setErrorThreshold(5e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);

    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingPrimary, two_transforms_retests)
{
    OCIO::GradingPrimaryTransformRcPtr gpDyn = OCIO::GradingPrimaryTransform::Create(GPTest1::style);
    gpDyn->makeDynamic();

    OCIO::GradingPrimary gplog{ GPTest1::style };
    gplog.m_brightness = GPTest1::brightness;
    gplog.m_contrast = GPTest1::contrast;
    gplog.m_gamma = GPTest1::gamma;
    gplog.m_saturation = GPTest1::saturation;
    gplog.m_pivot = GPTest1::pivot;
    gpDyn->setValue(gplog);

    auto gpNonDyn = OCIO::GradingPrimaryTransform::Create(GPTest2::style);
    OCIO::GradingPrimary gplin{ GPTest2::style };
    gplin.m_exposure = GPTest2::exposure;
    gplin.m_contrast = GPTest2::contrast;
    gplin.m_offset = GPTest2::offset;
    gplin.m_pivot = GPTest2::pivot;
    gplin.m_saturation = GPTest2::saturation;
    gplin.m_clampBlack = GPTest2::clampBlack;
    gplin.m_clampWhite = GPTest2::clampWhite;
    gpNonDyn->setValue(gplin);

    auto group = OCIO::GroupTransform::Create();
    group->appendTransform(gpDyn);
    group->appendTransform(gpNonDyn);
    test.setProcessor(group);

    class MyGPRetest : public GPRetest
    {
    public:
        MyGPRetest(OCIOGPUTest & test) : GPRetest(test)
        {
        }

        void retest_clamp()
        {
            initializeGPUDynamicProperties();
            auto vals = m_dynProp->getValue();
            vals.m_clampBlack = GPTest1::clampBlack;
            vals.m_clampWhite = GPTest1::clampWhite;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }

        void retest_pivot()
        {
            auto vals = m_dynProp->getValue();
            vals.m_clampBlack = -100;
            vals.m_clampWhite = 100;
            vals.m_pivotBlack = GPTest1::pivotBlack;
            vals.m_pivotWhite = GPTest1::pivotWhite;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);

        }
        void retest1()
        {
            auto vals = m_dynProp->getValue();
            vals.m_gamma.m_red += 0.1;
            vals.m_gamma.m_master += 0.1;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest2()
        {
            auto vals = m_dynProp->getValue();
            vals.m_saturation += 0.1;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest3()
        {
            auto vals = m_dynProp->getValue();
            vals.m_clampWhite += 1.;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest4()
        {
            OCIO::GradingPrimary identity{ GPTest1::style };
            m_dynProp->setValue(identity);
            m_dynPropGPU->setValue(identity);
        }
    };

    // Use shared_ptr so that object would stay until test is deleted.
    std::shared_ptr<MyGPRetest> gpRetest = std::make_shared<MyGPRetest>(test);

    // This adds a reference count to the shared_ptr.
    OCIOGPUTest::RetestSetupCallback f_clamp = std::bind(&MyGPRetest::retest_clamp, gpRetest);
    OCIOGPUTest::RetestSetupCallback f_pivot = std::bind(&MyGPRetest::retest_pivot, gpRetest);
    OCIOGPUTest::RetestSetupCallback f1 = std::bind(&MyGPRetest::retest1, gpRetest);
    OCIOGPUTest::RetestSetupCallback f2 = std::bind(&MyGPRetest::retest2, gpRetest);
    OCIOGPUTest::RetestSetupCallback f3 = std::bind(&MyGPRetest::retest3, gpRetest);
    OCIOGPUTest::RetestSetupCallback f4 = std::bind(&MyGPRetest::retest4, gpRetest);

    test.addRetest(f_clamp);
    test.addRetest(f_pivot);
    test.addRetest(f1);
    test.addRetest(f2);
    test.addRetest(f3);
    test.addRetest(f4);

    test.setErrorThreshold(1e-4f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);

    test.setTestInfinity(false);
    test.setTestNaN(false);
}
