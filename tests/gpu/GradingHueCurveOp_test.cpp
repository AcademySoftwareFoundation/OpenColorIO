// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{

void GenerateIdentityLut3D(OCIOGPUTest::CustomValues & values, int edgeLen, float min, float max)
{
    const int numChannels = 4;
    int num_samples = edgeLen * edgeLen * edgeLen;
    std::vector<float> img(num_samples * numChannels, 0.f);

    const float scale = max - min;
    float c = 1.0f / ((float)edgeLen - 1.0f);
    for (int i = 0; i < edgeLen*edgeLen*edgeLen; i++)
    {
        img[numChannels*i + 0] = scale * (float)(i%edgeLen) * c + min;
        img[numChannels*i + 1] = scale * (float)((i / edgeLen) % edgeLen) * c + min;
        img[numChannels*i + 2] = scale * (float)((i / edgeLen / edgeLen) % edgeLen) * c + min;
    }
    values.m_inputValues = img;
}

} // anon.

void GradingHueCurveLog(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    // All curves are non-identities.
    auto hh = OCIO::GradingBSplineCurve::Create({ {0.05f, 0.15f}, {0.2f, 0.3f}, {0.35f, 0.4f}, 
        {0.45f, 0.45f}, {0.6f, 0.7f}, {0.8f, 0.85f} }, OCIO::HUE_HUE);
    auto hs = OCIO::GradingBSplineCurve::Create({ {-0.1f, 1.2f}, {0.2f, 0.7f}, {0.4f, 1.5f}, 
        {0.5f, 0.5f}, {0.6f, 1.4f}, {0.8f, 0.7f} }, OCIO::HUE_SAT);
    auto hl = OCIO::GradingBSplineCurve::Create({ {0.1f, 1.5f}, {0.2f, 0.7f}, {0.4f, 1.4f}, 
        {0.5f, 0.8f}, {0.8f, 0.5f} }, OCIO::HUE_LUM);
    auto ls = OCIO::GradingBSplineCurve::Create({ {0.05f, 1.5f}, {0.5f, 0.9f}, {1.1f, 1.4f}, 
        }, OCIO::LUM_SAT);
    auto ss = OCIO::GradingBSplineCurve::Create({ {0.f, 0.1f}, {0.5f, 0.45f}, {1.f, 1.1f} },
        OCIO::SAT_SAT);
    auto ll = OCIO::GradingBSplineCurve::Create({ {-0.02f, -0.04f}, {0.2f, 0.1f}, {0.8f, 0.95f}, {1.1f, 1.2f} },
        OCIO::LUM_LUM);
    auto sl = OCIO::GradingBSplineCurve::Create({ {0.f, 1.2f}, {0.6f, 0.8f}, {0.9f, 1.1f} },
        OCIO::SAT_LUM);
    auto hfx = OCIO::GradingBSplineCurve::Create({ {0.2f, 0.05f}, {0.4f, -0.09f}, {0.6f, -0.2f}, 
        { 0.8f, 0.05f}, {0.99f, -0.02f} }, OCIO::HUE_FX);

    auto curve = OCIO::GradingHueCurve::Create(hh, hs, hl, ls, ss, ll, sl, hfx);
    auto hc = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LOG);
    if(!hc.get())
    {
        throw OCIO::Exception("Cannot create GradingHueCurveTransform.");
    }
    hc->setValue(curve);
    hc->setDirection(dir);
    if (dynamic)
    {
        hc->makeDynamic();
    }

    test.setProcessor(hc);

    // Set up a grid of RGBA custom values.
    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    GenerateIdentityLut3D(values, lut_size, -0.1f, 1.5f);
    test.setCustomValues(values);

    test.setErrorThreshold(2e-5f);
    test.setExpectedMinimalValue(1.0f);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, style_log_fwd)
{
    GradingHueCurveLog(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, style_log_fwd_dynamic)
{
    GradingHueCurveLog(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, style_log_rev)
{
    GradingHueCurveLog(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, style_log_rev_dynamic)
{
    GradingHueCurveLog(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

void HueCurveLin(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    // All curves are non-identities.
    auto hh = OCIO::GradingBSplineCurve::Create({ {0.05f, 0.15f}, {0.2f, 0.3f}, {0.35f, 0.4f}, 
        {0.45f, 0.45f}, {0.6f, 0.7f}, {0.8f, 0.85f} }, OCIO::HUE_HUE);
    auto hs = OCIO::GradingBSplineCurve::Create({ {-0.1f, 1.2f}, {0.2f, 0.7f}, {0.4f, 1.5f}, 
        {0.5f, 0.5f}, {0.6f, 1.4f}, {0.8f, 0.7f} }, OCIO::HUE_SAT);
    auto hl = OCIO::GradingBSplineCurve::Create({ {0.1f, 1.5f}, {0.2f, 0.7f}, {0.4f, 1.4f}, 
        {0.5f, 0.8f}, {0.8f, 0.5f} }, OCIO::HUE_LUM);
    auto ss = OCIO::GradingBSplineCurve::Create({ {0.f, 0.1f}, {0.5f, 0.45f}, {1.f, 1.1f} },
        OCIO::SAT_SAT);
    auto sl = OCIO::GradingBSplineCurve::Create({ {0.f, 1.2f}, {0.6f, 0.8f}, {0.9f, 1.1f} },
        OCIO::SAT_LUM);
    auto hfx = OCIO::GradingBSplineCurve::Create({ {0.2f, 0.05f}, {0.4f, -0.09f}, {0.6f, -0.2f}, 
        { 0.8f, 0.05f}, {0.99f, -0.02f} }, OCIO::HUE_FX);
    // Adjust these two, relative to previous test, to work in f-stops.
    auto ls = OCIO::GradingBSplineCurve::Create({ {-6.f, 0.9f}, {-3.f, 0.95f}, {0.f, 1.2f}, 
        {2.f, 1.f}, {4.f, 0.6f}, {6.f, 0.55f} }, OCIO::LUM_SAT);
    auto ll = OCIO::GradingBSplineCurve::Create({ {-8.f, -7.f}, {-2.f, -3.f}, {2.f, 3.5f}, {8.f, 7.f} },
        OCIO::LUM_LUM);

    auto curve = OCIO::GradingHueCurve::Create(hh, hs, hl, ls, ss, ll, sl, hfx);
    auto hc = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LIN);
    if(!hc.get())
    {
     throw OCIO::Exception("Cannot create GradingHueCurveTransform.");
    }
    hc->setValue(curve);
    hc->setDirection(dir);
    if (dynamic)
    {
       hc->makeDynamic();
    }

    test.setProcessor(hc);

    // Set up a grid of RGBA custom values.
    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    GenerateIdentityLut3D(values, lut_size, -0.1f, 1.5f);
    test.setCustomValues(values);

    test.setErrorThreshold(2e-4f);
    test.setExpectedMinimalValue(1.0f);
    test.setRelativeComparison(true);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, style_lin_fwd)
{
    HueCurveLin(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, style_lin_fwd_dynamic)
{
    HueCurveLin(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, style_lin_rev)
{
    HueCurveLin(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, style_lin_rev_dynamic)
{
    HueCurveLin(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}
