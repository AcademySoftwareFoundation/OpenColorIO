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
    auto hh = OCIO::GradingBSplineCurve::Create(
        { {0.05f, 0.15f}, {0.2f, 0.3f}, {0.35f, 0.4f}, {0.45f, 0.45f}, {0.6f, 0.7f}, {0.8f, 0.85f} },
        OCIO::HUE_HUE);
    auto hs = OCIO::GradingBSplineCurve::Create(
        { {-0.1f, 1.2f}, {0.2f, 0.7f}, {0.4f, 1.5f}, {0.5f, 0.5f}, {0.6f, 1.4f}, {0.8f, 0.7f} },
        OCIO::HUE_SAT);
    auto hl = OCIO::GradingBSplineCurve::Create(
        { {0.1f, 1.5f}, {0.2f, 0.7f}, {0.4f, 1.4f}, {0.5f, 0.8f}, {0.8f, 0.5f} },
        OCIO::HUE_LUM);
    auto ls = OCIO::GradingBSplineCurve::Create(
        { {0.05f, 1.5f}, {0.5f, 0.9f}, {1.1f, 1.4f} },
        OCIO::LUM_SAT);
    auto ss = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.1f}, {0.5f, 0.45f}, {1.f, 1.1f} },
        OCIO::SAT_SAT);
    auto ll = OCIO::GradingBSplineCurve::Create(
        { {-0.02f, -0.04f}, {0.2f, 0.1f}, {0.8f, 0.95f}, {1.1f, 1.2f} },
        OCIO::LUM_LUM);
    auto sl = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.2f}, {0.6f, 0.8f}, {0.9f, 1.1f} },
        OCIO::SAT_LUM);
    auto hfx = OCIO::GradingBSplineCurve::Create(
        { {0.2f, 0.05f}, {0.4f, -0.09f}, {0.6f, -0.2f}, { 0.8f, 0.05f}, {0.99f, -0.02f} },
        OCIO::HUE_FX);

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
    const int lut_size = 21;
    OCIOGPUTest::CustomValues values;
    // Choose values so that there is a grid point at 0.
    GenerateIdentityLut3D(values, lut_size, -0.075f, 1.425f);
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
    auto hh = OCIO::GradingBSplineCurve::Create(
        { {0.05f, 0.15f}, {0.2f, 0.3f}, {0.35f, 0.4f}, {0.45f, 0.45f}, {0.6f, 0.7f}, {0.8f, 0.85f} },
        OCIO::HUE_HUE);
    auto hs = OCIO::GradingBSplineCurve::Create(
        { {-0.1f, 1.2f}, {0.2f, 0.7f}, {0.4f, 1.5f}, {0.5f, 0.5f}, {0.6f, 1.4f}, {0.8f, 0.7f} },
        OCIO::HUE_SAT);
    auto hl = OCIO::GradingBSplineCurve::Create(
        { {0.1f, 1.5f}, {0.2f, 0.7f}, {0.4f, 1.4f}, {0.5f, 0.8f}, {0.8f, 0.5f} },
        OCIO::HUE_LUM);
    auto ss = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.1f}, {0.5f, 0.45f}, {1.f, 1.1f} },
        OCIO::SAT_SAT);
    auto hfx = OCIO::GradingBSplineCurve::Create(
        { {0.2f, 0.05f}, {0.4f, -0.09f}, {0.6f, -0.2f}, { 0.8f, 0.05f}, {0.99f, -0.02f} },
        OCIO::HUE_FX);
    // Adjust these two, relative to previous test, to work in f-stops.
    auto ll = OCIO::GradingBSplineCurve::Create(
        { {-8.f, -7.f}, {-2.f, -3.f}, {2.f, 3.5f}, {8.f, 7.f} },
        OCIO::LUM_LUM);
    auto ls = OCIO::GradingBSplineCurve::Create(
        { {-6.f, 0.9f}, {-3.f, 0.95f}, {0.f, 1.1f}, {2.f, 1.f}, {4.f, 0.6f}, {6.f, 0.55f} },
        OCIO::LUM_SAT);
    // Adjusted this one, relative to above, to avoid some artifacts due to high sat boost.
    auto sl = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.2f}, {0.6f, 0.8f}, {0.9f, 1.05f}, {1.f, 1.1f} },
        OCIO::SAT_LUM);

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
    const int lut_size = 21;
    OCIOGPUTest::CustomValues values;
    // Choose values so that there is a grid point at 0.
    GenerateIdentityLut3D(values, lut_size, -0.075f, 1.425f);
    test.setCustomValues(values);

    // This test produces some large linear values due to the sat boost, needs a large tolerance.
    // Metal is worse than GLSL.
    test.setErrorThreshold(4e-4f);
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

void DrawCurve(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    OCIO::GradingHueCurveTransformRcPtr hc = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LIN);
    OCIO::GradingHueCurveRcPtr hueCurve = hc->getValue()->createEditableCopy();
    OCIO::GradingBSplineCurveRcPtr huesat = hueCurve->getCurve(OCIO::HUE_SAT);

    // Enable drawCurveOnly mode.  This should only evaluate the HUE-SAT spline for use
    // in a user interface.
    hueCurve->setDrawCurveOnly(true);

    huesat->setSplineType( OCIO::DIAGONAL_B_SPLINE );

    huesat->setNumControlPoints(3);
    huesat->getControlPoint(0) = OCIO::GradingControlPoint(0.0f, 0.0f);
    huesat->getControlPoint(1) = OCIO::GradingControlPoint(0.5f, 0.7f);
    huesat->getControlPoint(2) = OCIO::GradingControlPoint(1.0f, 1.0f);

    hc->setValue(hueCurve);
    hc->setDirection(dir);
    if (dynamic)
    {
        hc->makeDynamic();
    }

    test.setProcessor(hc);

    test.setErrorThreshold(1e-5f);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, draw_lin_fwd)
{
    DrawCurve(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, draw_lin_fwd_dynamic)
{
    DrawCurve(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, draw_lin_rev)
{
    DrawCurve(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, draw_lin_rev_dynamic)
{
    DrawCurve(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

void BypassRGBtoHSY(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    OCIO::GradingHueCurveTransformRcPtr hc = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LIN);
    OCIO::GradingHueCurveRcPtr hueCurve = hc->getValue()->createEditableCopy();
    OCIO::GradingBSplineCurveRcPtr satsat = hueCurve->getCurve(OCIO::SAT_SAT);
    satsat->getControlPoint(1) = OCIO::GradingControlPoint(0.4f, 0.6f);

    hc->setRGBToHSY(OCIO::HSY_TRANSFORM_NONE);

    hc->setValue(hueCurve);
    hc->setDirection(dir);
    if (dynamic)
    {
        hc->makeDynamic();
    }

    test.setProcessor(hc);

    test.setErrorThreshold(1e-5f);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, bypass_rgbtohsy_fwd)
{
    BypassRGBtoHSY(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, bypass_rgbtohsy_fwd_dynamic)
{
    BypassRGBtoHSY(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, bypass_rgbtohsy_rev)
{
    BypassRGBtoHSY(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(GradingHueCurve, bypass_rgbtohsy_rev_dynamic)
{
    BypassRGBtoHSY(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}
