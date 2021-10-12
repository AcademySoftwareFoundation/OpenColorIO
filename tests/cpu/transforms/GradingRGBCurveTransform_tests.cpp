// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/gradingrgbcurve/GradingBSplineCurve.h"
#include "transforms/GradingRGBCurveTransform.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingRGBCurveTransform, basic)
{
    // Create transform and validate default values for all styles.

    auto gctLin = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gctLin->getStyle(), OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gctLin->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(!gctLin->getBypassLinToLog());
    OCIO_CHECK_ASSERT(!gctLin->isDynamic());
    auto red = gctLin->getValue()->getCurve(OCIO::RGB_RED);
    OCIO_CHECK_EQUAL(red->getNumControlPoints(), 3);
    OCIO_CHECK_EQUAL(red->getControlPoint(0), OCIO::GradingControlPoint(-7.f, -7.f));
    OCIO_CHECK_EQUAL(red->getControlPoint(1), OCIO::GradingControlPoint(0.f, 0.f));
    OCIO_CHECK_EQUAL(red->getControlPoint(2), OCIO::GradingControlPoint(7.f, 7.f));
    OCIO_CHECK_EQUAL(*gctLin->getValue()->getCurve(OCIO::RGB_GREEN), *red);
    OCIO_CHECK_EQUAL(*gctLin->getValue()->getCurve(OCIO::RGB_BLUE), *red);
    OCIO_CHECK_EQUAL(*gctLin->getValue()->getCurve(OCIO::RGB_MASTER), *red);
    OCIO_CHECK_NO_THROW(gctLin->validate());

    auto gctLog = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gctLog->getStyle(), OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gctLog->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(!gctLog->getBypassLinToLog());
    OCIO_CHECK_ASSERT(!gctLog->isDynamic());
    red = gctLog->getValue()->getCurve(OCIO::RGB_RED);
    OCIO_CHECK_EQUAL(red->getNumControlPoints(), 3);
    OCIO_CHECK_EQUAL(red->getControlPoint(0), OCIO::GradingControlPoint(0.f, 0.f));
    OCIO_CHECK_EQUAL(red->getControlPoint(1), OCIO::GradingControlPoint(0.5f, 0.5f));
    OCIO_CHECK_EQUAL(red->getControlPoint(2), OCIO::GradingControlPoint(1.f, 1.f));
    OCIO_CHECK_EQUAL(*gctLog->getValue()->getCurve(OCIO::RGB_GREEN), *red);
    OCIO_CHECK_EQUAL(*gctLog->getValue()->getCurve(OCIO::RGB_BLUE), *red);
    OCIO_CHECK_EQUAL(*gctLog->getValue()->getCurve(OCIO::RGB_MASTER), *red);
    OCIO_CHECK_NO_THROW(gctLog->validate());

    auto gctVid = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_VIDEO);
    OCIO_CHECK_EQUAL(gctVid->getStyle(), OCIO::GRADING_VIDEO);
    OCIO_CHECK_EQUAL(gctVid->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(!gctVid->getBypassLinToLog());
    OCIO_CHECK_ASSERT(!gctVid->isDynamic());
    red = gctVid->getValue()->getCurve(OCIO::RGB_RED);
    OCIO_CHECK_EQUAL(red->getNumControlPoints(), 3);
    OCIO_CHECK_EQUAL(red->getControlPoint(0), OCIO::GradingControlPoint(0.f, 0.f));
    OCIO_CHECK_EQUAL(red->getControlPoint(1), OCIO::GradingControlPoint(0.5f, 0.5f));
    OCIO_CHECK_EQUAL(red->getControlPoint(2), OCIO::GradingControlPoint(1.f, 1.f));
    OCIO_CHECK_EQUAL(*gctVid->getValue()->getCurve(OCIO::RGB_GREEN), *red);
    OCIO_CHECK_EQUAL(*gctVid->getValue()->getCurve(OCIO::RGB_BLUE), *red);
    OCIO_CHECK_EQUAL(*gctVid->getValue()->getCurve(OCIO::RGB_MASTER), *red);
    OCIO_CHECK_NO_THROW(gctVid->validate());

    // Change values.
    auto t = gctVid->createEditableCopy();
    auto gct = OCIO_DYNAMIC_POINTER_CAST<OCIO::GradingRGBCurveTransform>(t);
    gct->setStyle(OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gct->getStyle(), OCIO::GRADING_LIN);
    gct->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(gct->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
    gct->setBypassLinToLog(true);
    OCIO_CHECK_ASSERT(gct->getBypassLinToLog());
    gct->makeDynamic();
    OCIO_CHECK_ASSERT(gct->isDynamic());
    gct->setValue(gctLin->getValue());
    red = gct->getValue()->getCurve(OCIO::RGB_RED);
    OCIO_CHECK_EQUAL(red->getControlPoint(0), OCIO::GradingControlPoint(-7.f, -7.f));
    OCIO_CHECK_NO_THROW(gct->validate());

    // Access out of range point.
    OCIO_CHECK_THROW_WHAT(red->getControlPoint(4), OCIO::Exception,
                          "There are '3' control points. '4' is invalid.");

    // X has to be increasing.
    auto invalidCurve = OCIO::GradingBSplineCurve::Create({ { 0.0f, 0.0f }, { 0.5f, 0.2f },
                                                            { 0.2f, 0.7f }, { 1.0f, 1.0f } });
    auto newCurve = OCIO::GradingRGBCurve::Create(red, red, invalidCurve, red);
    OCIO_CHECK_THROW_WHAT(gct->setValue(newCurve), OCIO::Exception,
                          "has a x coordinate '0.2' that is less from previous control "
                          "point x cooordinate '0.5'.");

    // Check slopes.
    gct->setSlope(OCIO::RGB_BLUE, 2, 0.9f);
    OCIO_CHECK_NO_THROW(gct->validate());
    OCIO_CHECK_EQUAL(gct->getSlope(OCIO::RGB_BLUE, 2), 0.9f);
    OCIO_CHECK_THROW_WHAT(gct->setSlope(OCIO::RGB_BLUE, 4, 2.f), OCIO::Exception,
                          "There are '3' control points. '4' is invalid.");
    OCIO_CHECK_ASSERT(gct->slopesAreDefault(OCIO::RGB_GREEN));
    OCIO_CHECK_ASSERT(!gct->slopesAreDefault(OCIO::RGB_BLUE));
}

OCIO_ADD_TEST(GradingRGBCurveTransform, processor_several_transforms)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    const float srcPixel[3] = { 0.2f, 0.3f, 0.4f };

    auto c1 = OCIO::GradingBSplineCurve::Create({ {  0.0f,  0.0f }, { 0.2f,  0.2f },
                                                  {  0.5f,  0.7f }, { 1.0f,  1.0f } });
    auto c2 = OCIO::GradingBSplineCurve::Create({ {  0.0f,  0.5f }, { 0.3f,  0.7f },
                                                  {  0.5f,  1.1f }, { 1.0f,  1.5f } });
    auto c3 = OCIO::GradingBSplineCurve::Create({ {  0.0f, -0.5f }, { 0.2f, -0.4f },
                                                  {  0.3f,  0.1f }, { 0.5f,  0.4f },
                                                  {  0.7f,  0.9f }, { 1.0f,  1.1f } });
    auto c4 = OCIO::GradingBSplineCurve::Create({ { -1.0f,  0.0f }, { 0.2f,  0.2f },
                                                  {  0.8f,  0.8f }, { 2.0f,  1.0f } });
    auto c5 = OCIO::GradingBSplineCurve::Create({ {  0.0f,  0.0f }, { 1.0f,  1.0f } });


    auto rgbCurveA = OCIO::GradingRGBCurve::Create(c1, c2, c3, c5);

    auto gcta = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
    gcta->setValue(rgbCurveA);

    // Will hold results for rgbCurveA.
    float pixel_a[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
    // Will hold results for rgbCurveA applied twice.
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

    auto rgbCurveB = OCIO::GradingRGBCurve::Create(c4, c1, c2, c5);
    auto gctb = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
    gctb->setValue(rgbCurveB);

    // Will hold results for rgbCurveB.
    float pixel_b[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };
    // Will hold results for rgbCurveB applied twice.
    float pixel_bb[3];
    // Will hold results for rgbCurveA applied then rgbCurveB applied.
    float pixel_ab[3] = { pixel_a[0], pixel_a[1], pixel_a[2] };
    {
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(gctb);
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
        cpuProcessor->applyRGB(pixel_b);

        pixel_bb[0] = pixel_b[0];
        pixel_bb[1] = pixel_b[1];
        pixel_bb[2] = pixel_b[2];

        cpuProcessor->applyRGB(pixel_bb);
        cpuProcessor->applyRGB(pixel_ab);
    }

    // Make second transform dynamic.
    gctb->makeDynamic();
    const float error = 1e-6f;

    //
    // Test with two grading rgb curve transforms where only the second one is dynamic.
    //
    OCIO::GroupTransformRcPtr grp1 = OCIO::GroupTransform::Create();
    gctb->setValue(rgbCurveA);
    grp1->appendTransform(gcta);  // gpta values are rgbCurveA
    grp1->appendTransform(gctb);  // gptb values are rgbCurveA

    {
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(grp1);
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();

        // Second transform is dynamic. Value is still rgbCurveA.
        OCIO::DynamicPropertyRcPtr dp;
        OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_RGBCURVE));
        auto dpVal = OCIO::DynamicPropertyValue::AsGradingRGBCurve(dp);
        OCIO_REQUIRE_ASSERT(dpVal);

        float pixel[3] = { srcPixel[0], srcPixel[1], srcPixel[2] };

        // Apply rgbCurveA then rgbCurveA.
        cpuProcessor->applyRGB(pixel);

        OCIO_CHECK_CLOSE(pixel[0], pixel_aa[0], error);
        OCIO_CHECK_CLOSE(pixel[1], pixel_aa[1], error);
        OCIO_CHECK_CLOSE(pixel[2], pixel_aa[2], error);

        // Change the 2nd values.
        dpVal->setValue(rgbCurveB);
        pixel[0] = srcPixel[0];
        pixel[1] = srcPixel[1];
        pixel[2] = srcPixel[2];

        // Apply rgbCurveA then rgbCurveB.
        cpuProcessor->applyRGB(pixel);

        OCIO_CHECK_CLOSE(pixel[0], pixel_ab[0], error);
        OCIO_CHECK_CLOSE(pixel[1], pixel_ab[1], error);
        OCIO_CHECK_CLOSE(pixel[2], pixel_ab[2], error);
    }

    //
    // Test two grading rgb curve transforms can't be both dynamic.
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
        OCIO_CHECK_EQUAL(log.output(), "[OpenColorIO Warning]: Grading RGB curve dynamic property "
                                       "can only be there once.\n");
    }
}

OCIO_ADD_TEST(GradingRGBCurveTransform, serialization)
{
    // Test the serialization of the transform.

    auto c1 = OCIO::GradingBSplineCurve::Create({ {  0.0f,  0.0f }, { 0.2f,  0.2f },
                                                  {  0.5f,  0.7f }, { 1.0f,  1.0f } });
    auto c2 = OCIO::GradingBSplineCurve::Create({ {  0.0f,  0.5f }, { 0.3f,  0.7f },
                                                  {  0.5f,  1.1f }, { 1.0f,  1.5f } });
    auto c3 = OCIO::GradingBSplineCurve::Create({ {  0.0f, -0.5f }, { 0.2f, -0.4f },
                                                  {  0.3f,  0.1f }, { 0.5f,  0.4f },
                                                  {  0.7f,  0.9f }, { 1.0f,  1.1f } });
    auto c4 = OCIO::GradingBSplineCurve::Create({ {  0.0f,  0.0f }, { 1.0f,  1.0f } });


    auto data = OCIO::GradingRGBCurve::Create(c1, c2, c3, c4);

    auto curve = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
    curve->setValue(data);

    static constexpr char CURVE_STR[]
        = "<GradingRGBCurveTransform direction=forward, style=log, "\
          "values=<red=<control_points=[<x=0, y=0><x=0.2, y=0.2><x=0.5, y=0.7><x=1, y=1>]>, "\
          "green=<control_points=[<x=0, y=0.5><x=0.3, y=0.7><x=0.5, y=1.1><x=1, y=1.5>]>, "\
          "blue=<control_points=[<x=0, y=-0.5><x=0.2, y=-0.4><x=0.3, y=0.1><x=0.5, y=0.4><x=0.7, y=0.9><x=1, y=1.1>]>, "\
          "master=<control_points=[<x=0, y=0><x=1, y=1>]>>>";

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


OCIO_ADD_TEST(GradingRGBCurveTransform, local_bypass)
{
    // Test that the GPU is empty for an identity transform.

    OCIO::GradingRGBCurveTransformRcPtr transform
        = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);

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
