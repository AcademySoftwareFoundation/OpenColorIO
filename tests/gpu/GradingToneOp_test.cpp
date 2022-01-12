// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

namespace GTTest1
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
// These are {R, G, B, master, center, width};
static const OCIO::GradingRGBMSW midtones{ 0.3, 1.0, 1.8,  1.2, 0.47, 0.6 };
}

void GradingToneLogMidtones(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest1::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest1::style);
    tone.m_midtones = GTTest1::midtones;
    gt->setValue(tone);
    test.setProcessor(gt);

    test.setErrorThreshold(2e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_midtones_fwd)
{
    GradingToneLogMidtones(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_midtones_fwd_dynamic)
{
    GradingToneLogMidtones(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_midtones_rev)
{
    GradingToneLogMidtones(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_midtones_rev_dynamic)
{
    GradingToneLogMidtones(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GTTest2
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
// These are {R, G, B, master, start, pivot};
static const OCIO::GradingRGBMSW highlights{ 0.3, 1.0, 1.8, 1.4, -0.1, 0.9 };
}

void GradingToneLogHighlights(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest2::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest2::style);
    tone.m_highlights = GTTest2::highlights;
    gt->setValue(tone);
    test.setProcessor(gt);

    test.setErrorThreshold(2e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_highlights_fwd)
{
    GradingToneLogHighlights(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_highlights_fwd_dynamic)
{
    GradingToneLogHighlights(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_highlights_rev)
{
    GradingToneLogHighlights(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_highlights_rev_dynamic)
{
    GradingToneLogHighlights(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GTTest3
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_VIDEO;
// These are {R, G, B, master, start, pivot};
static const OCIO::GradingRGBMSW shadows{ 0.3, 1., 1.79, 0.6, 0.8, -0.1 };
}

void GradingToneVideoShadows(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest3::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest3::style);
    tone.m_shadows = GTTest3::shadows;
    gt->setValue(tone);
    test.setProcessor(gt);

    test.setErrorThreshold(3e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_video_shadows_fwd)
{
    GradingToneVideoShadows(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_video_shadows_fwd_dynamic)
{
    GradingToneVideoShadows(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingTone, style_video_shadows_rev)
{
    GradingToneVideoShadows(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_video_shadows_rev_dynamic)
{
    GradingToneVideoShadows(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GTTest4
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_VIDEO;
// These are {R, G, B, master, start, width};
static const OCIO::GradingRGBMSW whites{ 0.3, 1., 1.9, 0.6, -0.2, 1.4 };
}

void GradingToneVideoWhites(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest4::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest4::style);
    tone.m_whites = GTTest4::whites;
    gt->setValue(tone);
    test.setProcessor(gt);

    test.setErrorThreshold(3e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_video_white_detail_fwd)
{
    GradingToneVideoWhites(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_video_white_detail_fwd_dynamic)
{
    GradingToneVideoWhites(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingTone, style_video_white_detail_rev)
{
    GradingToneVideoWhites(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_video_white_detail_rev_dynamic)
{
    GradingToneVideoWhites(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GTTest5
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
// These are {R, G, B, master, start, width};
static const OCIO::GradingRGBMSW blacks{ 0.3, 1., 1.9, 0.6, 0.8, 0.9 };
}

void GradingToneLogBlacks(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest5::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest5::style);
    tone.m_blacks = GTTest5::blacks;
    gt->setValue(tone);
    test.setProcessor(gt);

    test.setErrorThreshold(3e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_black_detail_fwd)
{
    GradingToneLogBlacks(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_black_detail_fwd_dynamic)
{
    GradingToneLogBlacks(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_black_detail_rev)
{
    GradingToneLogBlacks(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_black_detail_rev_dynamic)
{
    GradingToneLogBlacks(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace GTTest6
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LOG;
static constexpr double scontrast = 1.8;
static constexpr double scontrast2 = 0.3;
}

void GradingToneLogSContrast(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic,
                             double scontrast)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest6::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest6::style);
    tone.m_scontrast = scontrast;
    gt->setValue(tone);
    test.setProcessor(gt);

    test.setErrorThreshold(3e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_scontrast_fwd)
{
    GradingToneLogSContrast(test, OCIO::TRANSFORM_DIR_FORWARD, false,
                            GTTest6::scontrast);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_scontrast_fwd_dynamic)
{
    GradingToneLogSContrast(test, OCIO::TRANSFORM_DIR_FORWARD, true,
                            GTTest6::scontrast);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_scontrast2_fwd)
{
    GradingToneLogSContrast(test, OCIO::TRANSFORM_DIR_FORWARD, false,
                            GTTest6::scontrast2);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_scontrast2_fwd_dynamic)
{
    GradingToneLogSContrast(test, OCIO::TRANSFORM_DIR_FORWARD, true,
                            GTTest6::scontrast2);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_scontrast_rev)
{
    GradingToneLogSContrast(test, OCIO::TRANSFORM_DIR_INVERSE, false,
                            GTTest6::scontrast);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_scontrast_rev_dynamic)
{
    GradingToneLogSContrast(test, OCIO::TRANSFORM_DIR_INVERSE, true,
                            GTTest6::scontrast);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_scontrast2_rev)
{
    GradingToneLogSContrast(test, OCIO::TRANSFORM_DIR_INVERSE, false,
                            GTTest6::scontrast2);
}

OCIO_ADD_GPU_TEST(GradingTone, style_log_scontrast2_rev_dynamic)
{
    GradingToneLogSContrast(test, OCIO::TRANSFORM_DIR_INVERSE, true,
                            GTTest6::scontrast2);
}

namespace GTTest7
{
static constexpr OCIO::GradingStyle style = OCIO::GRADING_LIN;
// These are {R, G, B, master, center, width};
const OCIO::GradingRGBMSW midtones{ 0.3, 1.4, 1.8, 1., 1., 8. };
}

void GradingToneLinMidtones(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto gt = OCIO::GradingToneTransform::Create(GTTest7::style);
    gt->setDirection(dir);
    if (dynamic)
    {
        gt->makeDynamic();
    }

    OCIO::GradingTone tone(GTTest7::style);
    tone.m_midtones = GTTest7::midtones;
    gt->setValue(tone);
    test.setProcessor(gt);

    test.setErrorThreshold(5e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_lin_midtones_fwd)
{
    GradingToneLinMidtones(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_lin_midtones_fwd_dynamic)
{
    GradingToneLinMidtones(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingTone, style_lin_midtones_rev)
{
    GradingToneLinMidtones(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingTone, style_lin_midtones_rev_dynamic)
{
    GradingToneLinMidtones(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

// TODO: add inverse tests.

namespace
{

class GTRetest
{
public:
    GTRetest() = delete;

    explicit GTRetest(OCIOGPUTest & test)
        : m_test(test)
    {
        OCIO::ConstProcessorRcPtr & processor = test.getProcessor();

        if (processor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_TONE))
        {
            auto dp = processor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_TONE);
            m_dynProp = OCIO::DynamicPropertyValue::AsGradingTone(dp);
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
        if (shaderDesc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_TONE))
        {
            auto dp = shaderDesc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_TONE);
            m_dynPropGPU = OCIO::DynamicPropertyValue::AsGradingTone(dp);
        }
    }

    OCIOGPUTest & m_test;
    // Keep dynamic property values for tests modifying their current value.
    OCIO::DynamicPropertyGradingToneRcPtr m_dynProp;
    OCIO::DynamicPropertyGradingToneRcPtr m_dynPropGPU;
};

}

OCIO_ADD_GPU_TEST(GradingTone, style_log_dynamic_retests)
{
    OCIO::GradingToneTransformRcPtr gt = OCIO::GradingToneTransform::Create(OCIO::GRADING_LOG);
    gt->makeDynamic();

    OCIO::GradingTone gtlog{ OCIO::GRADING_LOG };
    gt->setValue(gtlog);
    test.setProcessor(gt);

    class MyGTRetest : public GTRetest
    {
    public:
        explicit MyGTRetest(OCIOGPUTest & test) : GTRetest(test)
        {
        }

        void retest1()
        {
            initializeGPUDynamicProperties();
            auto vals = m_dynProp->getValue();
            vals.m_midtones = GTTest1::midtones;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest2()
        {
            auto vals = m_dynProp->getValue();
            vals.m_highlights = GTTest2::highlights;
            vals.m_whites = GTTest4::whites;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest3()
        {
            auto vals = m_dynProp->getValue();
            vals.m_blacks = GTTest5::blacks;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest4()
        {
            OCIO::GradingTone identity{ OCIO::GRADING_LOG };
            m_dynProp->setValue(identity);
            m_dynPropGPU->setValue(identity);
        }
    };

    // Use shared_ptr so that object would stay until test is deleted.
    std::shared_ptr<MyGTRetest> gtretest = std::make_shared<MyGTRetest>(test);

    // This adds a reference count to the shared_ptr.
    OCIOGPUTest::RetestSetupCallback f1 = std::bind(&MyGTRetest::retest1, gtretest);
    OCIOGPUTest::RetestSetupCallback f2 = std::bind(&MyGTRetest::retest2, gtretest);
    OCIOGPUTest::RetestSetupCallback f3 = std::bind(&MyGTRetest::retest3, gtretest);
    OCIOGPUTest::RetestSetupCallback f4 = std::bind(&MyGTRetest::retest4, gtretest);

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

OCIO_ADD_GPU_TEST(GradingTone, two_transforms_retests)
{
    OCIO::GradingToneTransformRcPtr gtDyn = OCIO::GradingToneTransform::Create(OCIO::GRADING_LOG);
    gtDyn->makeDynamic();

    OCIO::GradingTone gtlog{ OCIO::GRADING_LOG };
    gtDyn->setValue(gtlog);

    auto gtNonDyn = OCIO::GradingToneTransform::Create(GTTest6::style);

    OCIO::GradingTone tone{ OCIO::GRADING_LIN };
    tone.m_scontrast = 1.8;
    tone.m_midtones = OCIO::GradingRGBMSW(0.3, 1.4, 1.8, 1., 1., 8.);

    gtNonDyn->setValue(tone);

    auto group = OCIO::GroupTransform::Create();
    group->appendTransform(gtDyn);
    group->appendTransform(gtNonDyn);
    test.setProcessor(group);

    class MyGTRetest : public GTRetest
    {
    public:
        explicit MyGTRetest(OCIOGPUTest & test) : GTRetest(test)
        {
        }

        void retest1()
        {
            initializeGPUDynamicProperties();
            auto vals = m_dynProp->getValue();
            vals.m_midtones = GTTest1::midtones;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest2()
        {
            auto vals = m_dynProp->getValue();
            vals.m_highlights = GTTest2::highlights;
            vals.m_whites = GTTest4::whites;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest3()
        {
            auto vals = m_dynProp->getValue();
            vals.m_blacks = GTTest5::blacks;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest4()
        {
            OCIO::GradingTone identity{ OCIO::GRADING_LOG };
            m_dynProp->setValue(identity);
            m_dynPropGPU->setValue(identity);
        }
    };

    // Use shared_ptr so that object would stay until test is deleted.
    std::shared_ptr<MyGTRetest> gtretest = std::make_shared<MyGTRetest>(test);

    // This adds a reference count to the shared_ptr.
    OCIOGPUTest::RetestSetupCallback f1 = std::bind(&MyGTRetest::retest1, gtretest);
    OCIOGPUTest::RetestSetupCallback f2 = std::bind(&MyGTRetest::retest2, gtretest);
    OCIOGPUTest::RetestSetupCallback f3 = std::bind(&MyGTRetest::retest3, gtretest);
    OCIOGPUTest::RetestSetupCallback f4 = std::bind(&MyGTRetest::retest4, gtretest);

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
