// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

void GradingRGBCurveLog(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto r = OCIO::GradingBSplineCurve::Create({ { 0.0f,   0.0f },  { 0.785f, 0.231f },
                                                 { 0.809f, 0.631f },{ 0.948f, 0.704f },
                                                 { 1.0f,   1.0f } });
    auto g = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.15f }, { 0.55f, 0.35f },{ 0.9f, 1.1f } });
    auto b = OCIO::GradingBSplineCurve::Create({ { -6.f, -8.f }, { -2.f, -5.f },
                                                 {  2.f,  4.f }, {  5.f,  6.f } });
    auto m = OCIO::GradingBSplineCurve::Create({ { -0.1f, 0.1f },{ 1.1f, 1.3f } });
    OCIO::ConstGradingRGBCurveRcPtr curves = OCIO::GradingRGBCurve::Create(r, g, b, m);

    auto gc = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
    gc->setValue(curves);
    gc->setDirection(dir);
    if (dynamic)
    {
        gc->makeDynamic();
    }

    test.setProcessor(gc);

    test.setErrorThreshold(2e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(true);
    test.setTestNaN(true);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, style_log_fwd)
{
    GradingRGBCurveLog(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, style_log_fwd_dynamic)
{
    GradingRGBCurveLog(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, style_log_rev)
{
    GradingRGBCurveLog(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, style_log_rev_dynamic)
{
    GradingRGBCurveLog(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

void GradingRGBCurveLin(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    auto r = OCIO::GradingBSplineCurve::Create({ { 0.0f,   0.0f },{ 0.785f, 0.231f },
                                                 { 0.809f, 0.631f },{ 0.948f, 0.704f },
                                                 { 1.0f,   1.0f } });
    auto g = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.15f },{ 0.55f, 0.35f },{ 0.9f, 0.8f } });
    auto b = OCIO::GradingBSplineCurve::Create({ { -6.f, -4.f },{ -2.f, -1.f },
                                                 { 2.f,  2.f },{ 5.f,  4.f } });
    auto m = OCIO::GradingBSplineCurve::Create({ { -0.1f, 0.1f },{ 1.1f, 0.9f } });
    OCIO::ConstGradingRGBCurveRcPtr curves = OCIO::GradingRGBCurve::Create(r, g, b, m);

    auto gc = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LIN);
    gc->setValue(curves);
    gc->setDirection(dir);
    if (dynamic)
    {
        gc->makeDynamic();
    }

    test.setProcessor(gc);

    test.setErrorThreshold(1.5e-4f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(true);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, style_lin_fwd)
{
    GradingRGBCurveLin(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, style_lin_fwd_dynamic)
{
    GradingRGBCurveLin(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, style_lin_rev)
{
    GradingRGBCurveLin(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, style_lin_rev_dynamic)
{
    GradingRGBCurveLin(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

void GradingRGBSCurve(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    // Create an S-curve with 0 slope at each end.
    auto curve = OCIO::GradingBSplineCurve::Create({
            {-5.26017743f, -4.f},
            {-3.75502745f, -3.57868829f},
            {-2.24987747f, -1.82131329f},
            {-0.74472749f,  0.68124124f},
            { 1.06145248f,  2.87457742f},
            { 2.86763245f,  3.83406206f},
            { 4.67381243f,  4.f}
        });
    float slopes[] = { 0.f,  0.55982688f,  1.77532247f,  1.55f,  0.8787017f,  0.18374463f,  0.f };
    for (size_t i = 0; i < 7; ++i)
    {
        curve->setSlope( i, slopes[i] );
    }

    OCIO::ConstGradingBSplineCurveRcPtr m = curve;
    // Adjust scaling to ensure the test vector for the inverse hits the flat areas.
    auto scaling = OCIO::GradingBSplineCurve::Create({ { -5.f, 0.f }, { 5.f, 1.f } });
    OCIO::ConstGradingBSplineCurveRcPtr z = scaling;
    OCIO::ConstGradingRGBCurveRcPtr curves = OCIO::GradingRGBCurve::Create(m, m, m, z);

    auto gc = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
    gc->setValue(curves);
    gc->setDirection(dir);
    if (dynamic)
    {
        gc->makeDynamic();
    }

    test.setProcessor(gc);

    test.setErrorThreshold(1.5e-4f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(true);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, scurve_fwd)
{
    GradingRGBSCurve(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, scurve_fwd_dynamic)
{
    GradingRGBSCurve(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, scurve_rev)
{
    GradingRGBSCurve(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, scurve_rev_dynamic)
{
    GradingRGBSCurve(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

namespace
{

class GCRetest
{
public:
    GCRetest() = delete;

    GCRetest(OCIOGPUTest & test)
        : m_test(test)
    {
        // Testing infrastructure creates a new cpu processor for each retest (but keeps the
        // shader). Changing the dynamic property on the processor will be reflected in each
        // cpu processor. Initialize the dynamic properties for CPU. Shader is has not been
        // created yet.
        OCIO::ConstProcessorRcPtr & processor = test.getProcessor();

        if (processor->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_RGBCURVE))
        {
            auto dp = processor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_RGBCURVE);
            m_dynProp = OCIO::DynamicPropertyValue::AsGradingRGBCurve(dp);
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
        if (shaderDesc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_RGBCURVE))
        {
            auto dp = shaderDesc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_RGBCURVE);
            m_dynPropGPU = OCIO::DynamicPropertyValue::AsGradingRGBCurve(dp);
        }
    }

    OCIOGPUTest & m_test;
    // Keep dynamic property values for tests modifying their current value.
    OCIO::DynamicPropertyGradingRGBCurveRcPtr m_dynProp;
    OCIO::DynamicPropertyGradingRGBCurveRcPtr m_dynPropGPU;
};

}

OCIO_ADD_GPU_TEST(GradingRGBCurve, style_log_dynamic_retests)
{
    OCIO::GradingRGBCurveTransformRcPtr gc = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
    gc->makeDynamic();

    auto rnc = OCIO::GradingBSplineCurve::Create({ { 0.0f,   0.0f },{ 0.785f, 0.231f },
    { 0.809f, 0.631f },{ 0.948f, 0.704f },
    { 1.0f,   1.0f } });
    auto gnc = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.15f },{ 0.55f, 0.35f },{ 0.9f, 1.1f } });
    auto bnc = OCIO::GradingBSplineCurve::Create({ { -6.f, -8.f },{ -2.f, -5.f },
    { 2.f,  4.f },{ 5.f,  6.f } });
    auto mnc = OCIO::GradingBSplineCurve::Create({ { -0.1f, 0.1f },{ 1.1f, 1.3f } });

    OCIO::ConstGradingBSplineCurveRcPtr r = rnc;
    OCIO::ConstGradingBSplineCurveRcPtr g = gnc;
    OCIO::ConstGradingBSplineCurveRcPtr b = bnc;
    OCIO::ConstGradingBSplineCurveRcPtr m = mnc;

    OCIO::ConstGradingRGBCurveRcPtr curves = OCIO::GradingRGBCurve::Create(r, g, b, m);

    gc->setValue(curves);
    test.setProcessor(gc);

    class MyGPRetest : public GCRetest
    {
    public:
        MyGPRetest(OCIOGPUTest & test) : GCRetest(test)
        {
        }

        void retest1()
        {
            initializeGPUDynamicProperties();

            auto vals = m_dynProp->getValue()->createEditableCopy();
            vals->getCurve(OCIO::RGB_RED)->getControlPoint(1).m_y += 0.1f;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest2()
        {
            auto vals = m_dynProp->getValue()->createEditableCopy();
            vals->getCurve(OCIO::RGB_GREEN)->getControlPoint(1).m_y -= 0.1f;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest3()
        {
            auto vals = m_dynProp->getValue()->createEditableCopy();
            OCIO::GradingBSplineCurveRcPtr mc = vals->getCurve(OCIO::RGB_MASTER);
            mc->setNumControlPoints(3);
            mc->getControlPoint(1).m_x = 0.2f;
            mc->getControlPoint(1).m_y = 0.5f;
            mc->getControlPoint(2).m_x = 1.1f;
            mc->getControlPoint(2).m_y = 1.3f;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest4()
        {
            auto identity = OCIO::GradingRGBCurve::Create(OCIO::GRADING_LOG);
            m_dynProp->setValue(identity);
            m_dynPropGPU->setValue(identity);
        }
    };

    // Use shared_ptr so that object would stay until test is deleted.
    std::shared_ptr<MyGPRetest> gpRetest = std::make_shared<MyGPRetest>(test);

    // This adds a reference count to the shared_ptr.
    OCIOGPUTest::RetestSetupCallback f1 = std::bind(&MyGPRetest::retest1, gpRetest);
    OCIOGPUTest::RetestSetupCallback f2 = std::bind(&MyGPRetest::retest2, gpRetest);
    OCIOGPUTest::RetestSetupCallback f3 = std::bind(&MyGPRetest::retest3, gpRetest);
    OCIOGPUTest::RetestSetupCallback f4 = std::bind(&MyGPRetest::retest4, gpRetest);

    test.addRetest(f1);
    test.addRetest(f2);
    test.addRetest(f3);
    test.addRetest(f4);

    test.setErrorThreshold(5e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(true);
    test.setTestNaN(true);
}

OCIO_ADD_GPU_TEST(GradingRGBCurve, two_transforms_retests)
{
    OCIO::GradingRGBCurveTransformRcPtr gcDyn = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
    gcDyn->makeDynamic();
        
    auto rnc = OCIO::GradingBSplineCurve::Create({ { 0.0f,   0.0f },   { 0.785f, 0.231f },
                                                   { 0.809f, 0.631f }, { 0.948f, 0.704f },
                                                   { 1.0f,   1.0f } });
    auto gnc = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.15f }, { 0.55f, 0.35f }, { 0.9f, 1.1f } });
    auto bnc = OCIO::GradingBSplineCurve::Create({ { -6.f, -8.f }, { -2.f, -5.f },
                                                    { 2.f,  4.f }, { 5.f,  6.f } });
    auto mnc = OCIO::GradingBSplineCurve::Create({ { -0.1f, 0.1f }, { 1.1f, 1.3f } });

    OCIO::ConstGradingBSplineCurveRcPtr r = rnc;
    OCIO::ConstGradingBSplineCurveRcPtr g = gnc;
    OCIO::ConstGradingBSplineCurveRcPtr b = bnc;
    OCIO::ConstGradingBSplineCurveRcPtr m = mnc;

    OCIO::ConstGradingRGBCurveRcPtr curves = OCIO::GradingRGBCurve::Create(r, g, b, m);

    gcDyn->setValue(curves);

    auto c1 = OCIO::GradingBSplineCurve::Create({ { 0.0f,  0.0f },{ 0.2f,  0.2f },
                                                  { 0.5f,  0.7f },{ 1.0f,  1.0f } });
    auto c2 = OCIO::GradingBSplineCurve::Create({ { 0.0f,  0.5f },{ 0.3f,  0.7f },
                                                  { 0.5f,  1.1f },{ 1.0f,  1.5f } });
    auto c3 = OCIO::GradingBSplineCurve::Create({ { 0.0f, -0.5f },{ 0.2f, -0.4f },
                                                  { 0.3f,  0.1f },{ 0.5f,  0.4f },
                                                  { 0.7f,  0.9f },{ 1.0f,  1.1f } });
    auto c4 = OCIO::GradingBSplineCurve::Create({ { -1.0f,  0.0f },{ 0.2f,  0.2f },
                                                  { 0.8f,  0.8f },{ 2.0f,  1.0f } });
    OCIO::ConstGradingRGBCurveRcPtr curves2 = OCIO::GradingRGBCurve::Create(c1, c2, c3, c4);

    auto gcNonDyn = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LIN);
    gcNonDyn->setValue(curves2);

    auto group = OCIO::GroupTransform::Create();
    group->appendTransform(gcDyn);
    group->appendTransform(gcNonDyn);
    test.setProcessor(group);

    class MyGPRetest : public GCRetest
    {
    public:
        MyGPRetest(OCIOGPUTest & test) : GCRetest(test)
        {
        }

        void retest1()
        {
            initializeGPUDynamicProperties();

            auto vals = m_dynProp->getValue()->createEditableCopy();
            vals->getCurve(OCIO::RGB_RED)->getControlPoint(1).m_y += 0.1f;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest2()
        {
            auto vals = m_dynProp->getValue()->createEditableCopy();
            vals->getCurve(OCIO::RGB_GREEN)->getControlPoint(1).m_y -= 0.1f;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest3()
        {
            auto vals = m_dynProp->getValue()->createEditableCopy();
            OCIO::GradingBSplineCurveRcPtr mc = vals->getCurve(OCIO::RGB_MASTER);
            mc->setNumControlPoints(3);
            mc->getControlPoint(1).m_x = 0.2f;
            mc->getControlPoint(1).m_y = 0.5f;
            mc->getControlPoint(2).m_x = 1.1f;
            mc->getControlPoint(2).m_y = 1.3f;
            m_dynProp->setValue(vals);
            m_dynPropGPU->setValue(vals);
        }
        void retest4()
        {
            auto identity = OCIO::GradingRGBCurve::Create(OCIO::GRADING_LOG);
            m_dynProp->setValue(identity);
            m_dynPropGPU->setValue(identity);
        }
    };

    // Use shared_ptr so that object would stay until test is deleted.
    std::shared_ptr<MyGPRetest> gpRetest = std::make_shared<MyGPRetest>(test);

    // This adds a reference count to the shared_ptr.
    OCIOGPUTest::RetestSetupCallback f1 = std::bind(&MyGPRetest::retest1, gpRetest);
    OCIOGPUTest::RetestSetupCallback f2 = std::bind(&MyGPRetest::retest2, gpRetest);
    OCIOGPUTest::RetestSetupCallback f3 = std::bind(&MyGPRetest::retest3, gpRetest);
    OCIOGPUTest::RetestSetupCallback f4 = std::bind(&MyGPRetest::retest4, gpRetest);

    test.addRetest(f1);
    test.addRetest(f2);
    test.addRetest(f3);
    test.addRetest(f4);

    test.setErrorThreshold(5e-5f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
    test.setTestWideRange(true);
    test.setTestInfinity(false);
    test.setTestNaN(true);
}
