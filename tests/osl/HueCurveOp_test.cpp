// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

void GradingHueCurveLog(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    //auto r = OCIO::GradingBSplineCurve::Create({ { 0.0f,   0.0f },  { 0.785f, 0.231f },
    //                                             { 0.809f, 0.631f },{ 0.948f, 0.704f },
    //                                             { 1.0f,   1.0f } });
    //auto g = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.15f }, { 0.55f, 0.35f },{ 0.9f, 1.1f } });
    //auto b = OCIO::GradingBSplineCurve::Create({ { -6.f, -8.f }, { -2.f, -5.f },
    //                                             {  2.f,  4.f }, {  5.f,  6.f } });
    //auto m = OCIO::GradingBSplineCurve::Create({ { -0.1f, 0.1f },{ 1.1f, 1.3f } });
    //OCIO::ConstGradingRGBCurveRcPtr curves = OCIO::GradingRGBCurve::Create(r, g, b, m);

    //auto gc = OCIO::HueCurveTransform::Create();
    //gc->setValue(curves);
    //if(!gc.get())
    //{
    //  throw OCIO::Exception("Cannot create HueCurveTransform.");
    //}

    //gc->setDirection(dir);
    //if (dynamic)
    //{
    //    gc->makeDynamic();
    //}

    //test.setProcessor(gc);
//
    //test.setErrorThreshold(2e-5f);
    //test.setExpectedMinimalValue(1.0f);
    //test.setRelativeComparison(true);
    //test.setTestWideRange(true);
    //test.setTestInfinity(true);
    //test.setTestNaN(true);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, style_log_fwd)
{
    GradingHueCurveLog(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, style_log_fwd_dynamic)
{
    GradingHueCurveLog(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, style_log_rev)
{
    GradingHueCurveLog(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, style_log_rev_dynamic)
{
    GradingHueCurveLog(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

void HueCurveLin(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    //auto r = OCIO::GradingBSplineCurve::Create({ { 0.0f,   0.0f },{ 0.785f, 0.231f },
    //                                             { 0.809f, 0.631f },{ 0.948f, 0.704f },
    //                                             { 1.0f,   1.0f } });
    //auto g = OCIO::GradingBSplineCurve::Create({ { 0.1f, 0.15f },{ 0.55f, 0.35f },{ 0.9f, 0.8f } });
    //auto b = OCIO::GradingBSplineCurve::Create({ { -6.f, -4.f },{ -2.f, -1.f },
    //                                             { 2.f,  2.f },{ 5.f,  4.f } });
    //auto m = OCIO::GradingBSplineCurve::Create({ { -0.1f, 0.1f },{ 1.1f, 0.9f } });
    //OCIO::ConstGradingRGBCurveRcPtr curves = OCIO::GradingRGBCurve::Create(r, g, b, m);

    //auto gc = OCIO::HueCurveTransform::Create();
    //gc->setValue(curves);
    //gc->setDirection(dir);
    //if (dynamic)
    //{
    //    gc->makeDynamic();
    //}

    //test.setProcessor(gc);
//
    //test.setErrorThreshold(1.5e-4f);
    //test.setExpectedMinimalValue(1.0f);
    //test.setRelativeComparison(true);
    //test.setTestWideRange(true);
    //test.setTestInfinity(false);
    //test.setTestNaN(true);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, style_lin_fwd)
{
    HueCurveLin(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, style_lin_fwd_dynamic)
{
    HueCurveLin(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, style_lin_rev)
{
    HueCurveLin(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, style_lin_rev_dynamic)
{
    HueCurveLin(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}

void HueSCurve(OCIOGPUTest & test, OCIO::TransformDirection dir, bool dynamic)
{
    // Create an S-curve with 0 slope at each end.
    //auto curve = OCIO::GradingBSplineCurve::Create({
    //        {-5.26017743f, -4.f},
    //        {-3.75502745f, -3.57868829f},
    //        {-2.24987747f, -1.82131329f},
    //        {-0.74472749f,  0.68124124f},
    //        { 1.06145248f,  2.87457742f},
    //        { 2.86763245f,  3.83406206f},
    //        { 4.67381243f,  4.f}
    //    });
    //float slopes[] = { 0.f,  0.55982688f,  1.77532247f,  1.55f,  0.8787017f,  0.18374463f,  0.f };
    //for (size_t i = 0; i < 7; ++i)
    //{
    //    curve->setSlope( i, slopes[i] );
    //}
//
    //OCIO::ConstGradingBSplineCurveRcPtr m = curve;
    //// Adjust scaling to ensure the test vector for the inverse hits the flat areas.
    //auto scaling = OCIO::GradingBSplineCurve::Create({ { -5.f, 0.f }, { 5.f, 1.f } });
    //OCIO::ConstGradingBSplineCurveRcPtr z = scaling;
    //OCIO::ConstGradingRGBCurveRcPtr curves = OCIO::GradingRGBCurve::Create(m, m, m, z);

    //auto gc = OCIO::HueCurveTransform::Create();
    //if(!gc.get())
    //{
    //  throw OCIO::Exception("Cannot create HueCurveTransform.");
    //}
    //gc->setValue(curves);
    //gc->setDirection(dir);
    //if (dynamic)
    //{
    //    gc->makeDynamic();
    //}

    //test.setProcessor(gc);
//
    //test.setErrorThreshold(1.5e-4f);
    //test.setExpectedMinimalValue(1.0f);
    //test.setRelativeComparison(true);
    //test.setTestWideRange(true);
    //test.setTestInfinity(false);
    //test.setTestNaN(true);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, scurve_fwd)
{
    HueSCurve(test, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, scurve_fwd_dynamic)
{
    HueSCurve(test, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, scurve_rev)
{
    HueSCurve(test, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_ADD_GPU_TEST(HueCurveTransform, scurve_rev_dynamic)
{
    HueSCurve(test, OCIO::TRANSFORM_DIR_INVERSE, true);
}
