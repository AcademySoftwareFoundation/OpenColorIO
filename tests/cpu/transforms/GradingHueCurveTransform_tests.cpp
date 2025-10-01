// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradingrgbcurve/GradingBSplineCurve.h"
#include "transforms/GradingHueCurveTransform.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingHueCurveTransform, basic)
{
    // Create transform and validate default values for all styles.

    auto gctLin = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gctLin->getStyle(), OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gctLin->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    gctLin->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(gctLin->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(gctLin->getRGBToHSY(), OCIO::HSY_TRANSFORM_1);
    OCIO_CHECK_ASSERT(!gctLin->isDynamic());
    auto crv = gctLin->getValue()->getCurve(OCIO::LUM_SAT);
    OCIO_CHECK_EQUAL(crv->getNumControlPoints(), 3);
    OCIO_CHECK_EQUAL(crv->getControlPoint(0), OCIO::GradingControlPoint(-7.f, 1.f));
    OCIO_CHECK_EQUAL(crv->getControlPoint(1), OCIO::GradingControlPoint(0.f, 1.f));
    OCIO_CHECK_EQUAL(crv->getControlPoint(2), OCIO::GradingControlPoint(7.f, 1.f));
    crv = gctLin->getValue()->getCurve(OCIO::HUE_LUM);
    OCIO_CHECK_EQUAL(*gctLin->getValue()->getCurve(OCIO::HUE_SAT), *crv);
    OCIO_CHECK_ASSERT(gctLin.get());
    OCIO_CHECK_NO_THROW(gctLin->validate());

    auto gctLog = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gctLog->getStyle(), OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gctLog->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(gctLog->getRGBToHSY(), OCIO::HSY_TRANSFORM_1);
    OCIO_CHECK_ASSERT(!gctLog->isDynamic());
    crv = gctLog->getValue()->getCurve(OCIO::LUM_SAT);
    OCIO_CHECK_EQUAL(crv->getNumControlPoints(), 3);
    OCIO_CHECK_EQUAL(crv->getControlPoint(0), OCIO::GradingControlPoint(0.f, 1.f));
    OCIO_CHECK_EQUAL(crv->getControlPoint(1), OCIO::GradingControlPoint(0.5f, 1.f));
    OCIO_CHECK_EQUAL(crv->getControlPoint(2), OCIO::GradingControlPoint(1.f, 1.f));
    crv = gctLog->getValue()->getCurve(OCIO::HUE_LUM);
    OCIO_CHECK_EQUAL(*gctLog->getValue()->getCurve(OCIO::HUE_SAT), *crv);
    crv = gctLog->getValue()->getCurve(OCIO::LUM_LUM);
    OCIO_CHECK_EQUAL(*gctLog->getValue()->getCurve(OCIO::SAT_SAT), *crv);
    OCIO_CHECK_NO_THROW(gctLog->validate());

    auto gctVid = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_VIDEO);
    OCIO_CHECK_EQUAL(gctVid->getStyle(), OCIO::GRADING_VIDEO);
    OCIO_CHECK_EQUAL(gctVid->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(gctVid->getRGBToHSY(), OCIO::HSY_TRANSFORM_1);
    OCIO_CHECK_ASSERT(!gctVid->isDynamic());
    crv = gctVid->getValue()->getCurve(OCIO::LUM_SAT);
    OCIO_CHECK_EQUAL(crv->getNumControlPoints(), 3);
    OCIO_CHECK_EQUAL(crv->getControlPoint(0), OCIO::GradingControlPoint(0.f, 1.f));
    OCIO_CHECK_EQUAL(crv->getControlPoint(1), OCIO::GradingControlPoint(0.5f, 1.f));
    OCIO_CHECK_EQUAL(crv->getControlPoint(2), OCIO::GradingControlPoint(1.f, 1.f));
    crv = gctLog->getValue()->getCurve(OCIO::HUE_LUM);
    OCIO_CHECK_EQUAL(*gctLog->getValue()->getCurve(OCIO::HUE_SAT), *crv);
    crv = gctLog->getValue()->getCurve(OCIO::LUM_LUM);
    OCIO_CHECK_EQUAL(*gctLog->getValue()->getCurve(OCIO::SAT_SAT), *crv);
    OCIO_CHECK_NO_THROW(gctVid->validate());

    // Change values.
    auto t = gctVid->createEditableCopy();
    auto gct = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingHueCurveTransform>(t);
    gct->setStyle(OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gct->getStyle(), OCIO::GRADING_LIN);
    gct->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(gct->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    gct->setRGBToHSY(OCIO::HSY_TRANSFORM_NONE);
    OCIO_CHECK_EQUAL(gct->getRGBToHSY(), OCIO::HSY_TRANSFORM_NONE);
    gct->makeDynamic();
    OCIO_CHECK_ASSERT(gct->isDynamic());
    gct->setValue(gctLin->getValue());
    crv = gct->getValue()->getCurve(OCIO::LUM_LUM);
    OCIO_CHECK_EQUAL(crv->getControlPoint(0), OCIO::GradingControlPoint(-7.f, -7.f));
    OCIO_CHECK_NO_THROW(gct->validate());

    // Access out of range point.
    OCIO_CHECK_THROW_WHAT(crv->getControlPoint(4), OCIO::Exception,
                         "There are '3' control points. '4' is out of bounds.");

    // X-coordinate has to be increasing.
    {
        OCIO::GradingHueCurveTransformRcPtr hct = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_VIDEO);
        OCIO::GradingHueCurveRcPtr hueCurve = hct->getValue()->createEditableCopy();
        OCIO::GradingBSplineCurveRcPtr lumsat = hueCurve->getCurve(OCIO::LUM_SAT);
        OCIO::GradingControlPoint & cp = lumsat->getControlPoint(0);
        cp = OCIO::GradingControlPoint(0.7f, 1.f);
        OCIO_CHECK_THROW_WHAT(hct->setValue(hueCurve), OCIO::Exception,
                             "has a x coordinate '0.5' that is less than previous control "
                             "point x coordinate '0.7'.");
    }

    // Y-coordinate has to be increasing, for diagonal curves.
    {
        OCIO::GradingHueCurveTransformRcPtr hct = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_VIDEO);
        OCIO::GradingHueCurveRcPtr hueCurve = hct->getValue()->createEditableCopy();
        OCIO::GradingBSplineCurveRcPtr lumlum = hueCurve->getCurve(OCIO::LUM_LUM);
        OCIO::GradingControlPoint & cp = lumlum->getControlPoint(0);
        cp = OCIO::GradingControlPoint(0.f, 0.6f);
        OCIO_CHECK_THROW_WHAT(hct->setValue(hueCurve), OCIO::Exception,
                             "has a y coordinate '0.5' that is less than previous control "
                             "point y coordinate '0.6'.");
    }

    // Check slopes.
    gct->setSlope(OCIO::LUM_LUM, 2, 0.9f);
    OCIO_CHECK_NO_THROW(gct->validate());
    OCIO_CHECK_EQUAL(gct->getSlope(OCIO::LUM_LUM, 2), 0.9f);
    OCIO_CHECK_THROW_WHAT(gct->setSlope(OCIO::LUM_LUM, 4, 2.f), OCIO::Exception,
                         "There are '3' control points. '4' is out of bounds.");
    OCIO_CHECK_ASSERT(gct->slopesAreDefault(OCIO::LUM_SAT));
    OCIO_CHECK_ASSERT(!gct->slopesAreDefault(OCIO::LUM_LUM));
}

OCIO_ADD_TEST(GradingHueCurveTransform, processor_several_transforms)
{
    OCIO::GradingHueCurveTransformRcPtr gcta = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LOG);
    OCIO::GradingHueCurveRcPtr hueCurveA = gcta->getValue()->createEditableCopy();
    OCIO::GradingBSplineCurveRcPtr huefx = hueCurveA->getCurve(OCIO::HUE_FX);
    // Shift all hues up by 0.1.
    huefx->setNumControlPoints(2);
    huefx->getControlPoint(0) = OCIO::GradingControlPoint(0.f, 0.1f);
    huefx->getControlPoint(1) = OCIO::GradingControlPoint(0.9f, 0.1f);
    gcta->setValue(hueCurveA);

    OCIO_CHECK_NO_THROW(gcta->validate());
    OCIO_CHECK_ASSERT(!hueCurveA->isIdentity());

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    const float srcPixel[3] = { 0.2f, 0.3f, 0.4f };

    // Will hold results for hueCurveA.
    float pixel_a[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
    // Will hold results for hueCurveA applied twice.
    float pixel_aa[3];
    {
       OCIO::ConstProcessorRcPtr processor = config->getProcessor(gcta);
       OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
       cpuProcessor->applyRGB(pixel_a);

       pixel_aa[0] = pixel_a[0];
       pixel_aa[1] = pixel_a[1];
       pixel_aa[2] = pixel_a[2];
       cpuProcessor->applyRGB(pixel_aa);
    }

    // NB: This must be GRADING_LOG like above because the test will be changing the curves
    // as dynamic parameters but that does not change the base style.
    OCIO::GradingHueCurveTransformRcPtr gctb = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LOG);
    OCIO::GradingHueCurveRcPtr hueCurveB = gctb->getValue()->createEditableCopy();
    OCIO::GradingBSplineCurveRcPtr lumsat = hueCurveB->getCurve(OCIO::LUM_SAT);
    // Increase sat at all luminances by 1.5.
    lumsat->setNumControlPoints(2);
    lumsat->getControlPoint(0) = OCIO::GradingControlPoint(0.f, 1.5f);
    lumsat->getControlPoint(1) = OCIO::GradingControlPoint(1.f, 1.5f);
    gctb->setValue(hueCurveB);

    OCIO_CHECK_ASSERT(!hueCurveB->isIdentity());

    // Will hold results for hueCurveA applied then hueCurveB applied.
    float pixel_ab[3] = { pixel_a[0], pixel_a[1], pixel_a[2] };
    {
       OCIO::ConstProcessorRcPtr processor = config->getProcessor(gctb);
       OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
       cpuProcessor->applyRGB(pixel_ab);
    }

    // Make second transform dynamic.
    gctb->makeDynamic();
    const float error = 1e-6f;

    //
    // Test with two grading hue curve transforms where only the second one is dynamic.
    //

    OCIO::GroupTransformRcPtr grp1 = OCIO::GroupTransform::Create();
    gctb->setValue(hueCurveA);
    grp1->appendTransform(gcta);  // gpta values are hueCurveA
    grp1->appendTransform(gctb);  // gptb values are hueCurveA

    {
       OCIO::ConstProcessorRcPtr processor = config->getProcessor(grp1);
       OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();

       // Second transform is dynamic. Value is still hueCurveA.
       OCIO::DynamicPropertyRcPtr dp;
       OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_HUECURVE));
       auto dpVal = OCIO::DynamicPropertyValue::AsGradingHueCurve(dp);
       OCIO_REQUIRE_ASSERT(dpVal);

       float pixel[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };

       // Apply hueCurveA then hueCurveA.
       cpuProcessor->applyRGB(pixel);

       OCIO_CHECK_CLOSE(pixel[0], pixel_aa[0], error);
       OCIO_CHECK_CLOSE(pixel[1], pixel_aa[1], error);
       OCIO_CHECK_CLOSE(pixel[2], pixel_aa[2], error);

       // Change the 2nd values.
       dpVal->setValue(hueCurveB);

       pixel[0] = srcPixel[0];
       pixel[1] = srcPixel[1];
       pixel[2] = srcPixel[2];

       // Apply hueCurveA then hueCurveB.
       cpuProcessor->applyRGB(pixel);

       OCIO_CHECK_CLOSE(pixel[0], pixel_ab[0], error);
       OCIO_CHECK_CLOSE(pixel[1], pixel_ab[1], error);
       OCIO_CHECK_CLOSE(pixel[2], pixel_ab[2], error);
    }

    //
    // Test two grading hue curve transforms can't be both dynamic.
    //

    // Make first dynamic (second already is).
    gcta->makeDynamic();

    OCIO::GroupTransformRcPtr grp2 = OCIO::GroupTransform::Create();
    grp2->appendTransform(gcta);
    grp2->appendTransform(gctb);

    {
       OCIO::LogGuard log;
       OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);
       OCIO_CHECK_NO_THROW(config->getProcessor(grp2));
       OCIO_CHECK_EQUAL(log.output(), "[OpenColorIO Warning]: Grading hue curve dynamic property "
                                      "can only be there once.\n");
    }
}

OCIO_ADD_TEST(GradingHueCurveTransform, serialization)
{
    // Test the serialization of the transform.

    auto hh = OCIO::GradingBSplineCurve::Create(
        { {0.1f, -0.05f}, {0.2f, 0.23f}, {0.5f, 0.25f}, {0.8f, 0.7f}, {0.85f, 0.8f}, {0.95f, 0.9f} },
        OCIO::HUE_HUE);
    auto hs = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.2f}, {0.1f, 1.2f}, {0.4f, 0.7f}, {0.6f, 0.3f}, {0.8f, 0.5f}, {0.9f, 0.8f} },
        OCIO::HUE_SAT);
    auto hl = OCIO::GradingBSplineCurve::Create(
        { {0.1f, 1.4f}, {0.2f, 1.4f}, {0.4f, 0.7f}, {0.6f, 0.5f}, {0.8f, 0.8f} },
        OCIO::HUE_LUM);
    auto ls = OCIO::GradingBSplineCurve::Create(
        { {0.0f, 1.0f}, {0.5f, 1.5f}, {1.0f, 0.9f}, {1.1f, 1.1f} },
        OCIO::LUM_SAT);
    auto ss = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.05f}, {0.5f, 0.8f}, {1.f, 1.05f} },
        OCIO::SAT_SAT);
    auto ll = OCIO::GradingBSplineCurve::Create(
        { {0.f, -0.0005f}, {0.5f, 0.3f}, {1.f, 0.9f} },
        OCIO::LUM_LUM);
    auto sl = OCIO::GradingBSplineCurve::Create(
        { {0.05f, 1.1f}, {0.3f, 1.f}, {1.2f, 0.9f} },
        OCIO::SAT_LUM);
    auto hfx = OCIO::GradingBSplineCurve::Create(
        { {-0.15f, 0.1f}, {0.f, -0.05f}, {0.2f, -0.1f}, {0.4f, 0.3f}, {0.6f, 0.25f}, { 0.8f, 0.2f}, {0.9f, 0.05f}, {1.1f, -0.07f} },
        OCIO::HUE_FX);

    ss->setSlope(0, 1.2f);
    ss->setSlope(1, 0.8f);
    ss->setSlope(2, 0.4f);

    auto data = OCIO::GradingHueCurve::Create(hh, hs, hl, ls, ss, ll, sl, hfx);

    auto curve = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_VIDEO);
    OCIO_CHECK_ASSERT(curve.get());
    OCIO_CHECK_NO_THROW(curve->validate());

    curve->setValue(data);

    static constexpr char CURVE_STR[]
      = "<GradingHueCurveTransform direction=forward, style=video, values="
        "<hue_hue=<control_points=[<x=0.1, y=-0.05><x=0.2, y=0.23><x=0.5, y=0.25><x=0.8, y=0.7>"
        "<x=0.85, y=0.8><x=0.95, y=0.9>]>, hue_sat=<control_points=[<x=0, y=1.2><x=0.1, y=1.2>"
        "<x=0.4, y=0.7><x=0.6, y=0.3><x=0.8, y=0.5><x=0.9, y=0.8>]>, hue_lum=<control_points="
        "[<x=0.1, y=1.4><x=0.2, y=1.4><x=0.4, y=0.7><x=0.6, y=0.5><x=0.8, y=0.8>]>, lum_sat="
        "<control_points=[<x=0, y=1><x=0.5, y=1.5><x=1, y=0.9><x=1.1, y=1.1>]>, sat_sat=<control_points="
        "[<x=0, y=0.05, slp=1.2><x=0.5, y=0.8, slp=0.8><x=1, y=1.05, slp=0.4>]>, lum_lum=<control_points="
        "[<x=0, y=-0.0005><x=0.5, y=0.3><x=1, y=0.9>]>, sat_lum=<control_points=[<x=0.05, y=1.1>"
        "<x=0.3, y=1><x=1.2, y=0.9>]>, hue_fx=<control_points=[<x=-0.15, y=0.1><x=0, y=-0.05>"
        "<x=0.2, y=-0.1><x=0.4, y=0.3><x=0.6, y=0.25><x=0.8, y=0.2><x=0.9, y=0.05><x=1.1, y=-0.07>]>>>";

    {
        std::ostringstream oss;
        oss << *curve;

        OCIO_CHECK_EQUAL(oss.str(), CURVE_STR);
    }

    OCIO::GroupTransformRcPtr grp = OCIO::GroupTransform::Create();
    grp->appendTransform(OCIO::DynamicPtrCast<OCIO::Transform>(curve));

    {
        std::ostringstream oss;
        oss << *grp;

        std::string GROUP_STR("<GroupTransform direction=forward, transforms=\n");
        GROUP_STR += "        ";
        GROUP_STR += CURVE_STR;
        GROUP_STR += ">";

        OCIO_CHECK_EQUAL(oss.str(), GROUP_STR);
    }
}

OCIO_ADD_TEST(GradingHueCurveTransform, local_bypass)
{
    // Test that the GPU shader is empty for an identity transform.

    OCIO::GradingHueCurveTransformRcPtr transform
        = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LOG);

    OCIO_CHECK_NO_THROW(transform->validate());

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();

    OCIO::ConstProcessorRcPtr proc = config->getProcessor(transform);
    OCIO::ConstGPUProcessorRcPtr gpu = proc->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_NONE);

    OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();

    OCIO_CHECK_NO_THROW(gpu->extractGpuShaderInfo(shaderDesc));

    static const std::string gpuStr("\n"
                                   "// Declaration of the OCIO shader function\n"
                                   "\n"
                                   "vec4 OCIOMain(vec4 inPixel)\n"
                                   "{\n"
                                   "  vec4 outColor = inPixel;\n"
                                   "\n"
                                   "  return outColor;\n"
                                   "}\n");

    OCIO_CHECK_EQUAL(gpuStr, std::string(shaderDesc->getShaderText()));
}
