// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include "DynamicProperty.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(DynamicPropertyImpl, basic)
{
    OCIO::DynamicPropertyDoubleImplRcPtr dp =
        std::make_shared<OCIO::DynamicPropertyDoubleImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO_REQUIRE_ASSERT(dp);
    OCIO_REQUIRE_EQUAL(dp->getValue(), 1.0);
    dp->setValue(2.0);
    OCIO_CHECK_EQUAL(dp->getValue(), 2.0);

    OCIO::DynamicPropertyDoubleImplRcPtr dpImpl =
        std::make_shared<OCIO::DynamicPropertyDoubleImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO_REQUIRE_ASSERT(dpImpl);
    OCIO_CHECK_ASSERT(!dpImpl->isDynamic());
    OCIO_CHECK_EQUAL(dpImpl->getValue(), 1.0);

    dpImpl->makeDynamic();
    OCIO_CHECK_ASSERT(dpImpl->isDynamic());
    dpImpl->setValue(2.0);
    OCIO_CHECK_EQUAL(dpImpl->getValue(), 2.0);
}

OCIO_ADD_TEST(DynamicPropertyImpl, equal_double)
{
    OCIO::DynamicPropertyDoubleImplRcPtr dpImpl0 =
        std::make_shared<OCIO::DynamicPropertyDoubleImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO::DynamicPropertyRcPtr dp0 = dpImpl0;

    OCIO::DynamicPropertyDoubleImplRcPtr dpImpl1 =
        std::make_shared<OCIO::DynamicPropertyDoubleImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, false);
    OCIO::DynamicPropertyRcPtr dp1 = dpImpl1;

    // Both not dynamic, same value.
    OCIO_CHECK_ASSERT(*dp0 == *dp1);

    // Both not dynamic, diff values.
    dpImpl0->setValue(2.0);
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Same value.
    dpImpl1->setValue(2.0);
    OCIO_CHECK_ASSERT(*dp0 == *dp1);

    // One dynamic, not the other, same value.
    dpImpl0->makeDynamic();
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Both dynamic, same value. Equality is used to optimized, so if values are dynamic they
    // might or not be the same, but they are considered different so that they are not optimized.
    dpImpl1->makeDynamic();
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Both dynamic, different values.
    dpImpl1->setValue(3.0);
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));
}

namespace
{
OCIO::ConstProcessorRcPtr LoadTransformFile(const std::string & fileName)
{
    const std::string filePath(OCIO::GetTestFilesDir() + "/" + fileName);

    // Create a FileTransform.
    OCIO::FileTransformRcPtr pFileTransform
        = OCIO::FileTransform::Create();
    pFileTransform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    pFileTransform->setSrc(filePath.c_str());

    // Create empty Config to use.
    OCIO::ConfigRcPtr pConfig = OCIO::Config::Create();

    // Get the processor corresponding to the transform.
    return pConfig->getProcessor(pFileTransform);
}
}

// Test several aspects of dynamic properties, especially the ability to set
// values via the processor.
OCIO_ADD_TEST(DynamicProperty, get_dynamic_via_cpu_processor)
{
    const std::string ctfFile("exposure_contrast_video_dp.ctf");

    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = LoadTransformFile(ctfFile));

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = processor->getDefaultCPUProcessor());

    float pixel[3] = { 0.5f, 0.4f, 0.2f };
    cpuProcessor->applyRGB(pixel);

    float error = 1e-5f;
    OCIO_CHECK_CLOSE(pixel[0], 0.57495f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.43988f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.19147f, error);

    const OCIO::DynamicPropertyType dpt = OCIO::DYNAMIC_PROPERTY_EXPOSURE;
    OCIO::DynamicPropertyRcPtr dp;
    OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(dpt));
    auto dpDouble = OCIO_DYNAMIC_POINTER_CAST<OCIO::DynamicPropertyDouble>(dp);
    const double fileValue = dpDouble->getValue();
    dpDouble->setValue(0.4);

    pixel[0] = 0.5f;
    pixel[1] = 0.4f;
    pixel[2] = 0.2f;
    cpuProcessor->applyRGB(pixel);

    // Adjust error for SSE approximation.
    OCIO_CHECK_CLOSE(pixel[0], 0.62966f, error*2.0f);
    OCIO_CHECK_CLOSE(pixel[1], 0.48175f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.20969f, error);

    // Restore file value.
    dpDouble->setValue(fileValue);

    pixel[0] = 0.5f;
    pixel[1] = 0.4f;
    pixel[2] = 0.2f;
    cpuProcessor->applyRGB(pixel);

    OCIO_CHECK_CLOSE(pixel[0], 0.57495f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.43988f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.19147f, error);

    // Note: The CTF does not define gamma as being dynamic.
    OCIO_CHECK_THROW_WHAT(cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GAMMA),
                          OCIO::Exception,
                          "Cannot find dynamic property");

    // Get optimized CPU processor without dynamic properties.
    OCIO_CHECK_NO_THROW(cpuProcessor = processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_ALL));

    // Now the dynamic property can't be found.
    OCIO_CHECK_THROW_WHAT(cpuProcessor->getDynamicProperty(dpt),
                          OCIO::Exception,
                          "Cannot find dynamic property");
}

OCIO_ADD_TEST(DynamicPropertyImpl, equal_grading_primary)
{
    OCIO::GradingPrimary gplog{ OCIO::GRADING_LOG };
    gplog.m_brightness = OCIO::GradingRGBM{ -10., 45., -5.,  50. };
    gplog.m_contrast = OCIO::GradingRGBM{ 0.9, 1.4,  0.7,  0.75 };
    gplog.m_gamma = OCIO::GradingRGBM{ 1.1, 0.7,  1.05, 1.15 };
    gplog.m_saturation = 1.21;
    gplog.m_pivot = -0.3;
    gplog.m_pivotBlack = 0.05;
    gplog.m_pivotWhite = 0.9;
    gplog.m_clampBlack = -0.05;
    gplog.m_clampWhite = 1.5;

    OCIO::DynamicPropertyGradingPrimaryImplRcPtr dpImpl0 =
        std::make_shared<OCIO::DynamicPropertyGradingPrimaryImpl>(OCIO::GRADING_LOG,
                                                                  OCIO::TRANSFORM_DIR_FORWARD,
                                                                  gplog, false);

    OCIO::DynamicPropertyRcPtr dp0 = dpImpl0;

    OCIO::DynamicPropertyGradingPrimaryImplRcPtr dpImpl1 =
        std::make_shared<OCIO::DynamicPropertyGradingPrimaryImpl>(OCIO::GRADING_LOG,
                                                                  OCIO::TRANSFORM_DIR_FORWARD,
                                                                  gplog, false);
    OCIO::DynamicPropertyRcPtr dp1 = dpImpl1;

    // Both not dynamic, same value.
    OCIO_CHECK_ASSERT(*dp0 == *dp1);

    // Both not dynamic, diff values.
    gplog.m_clampWhite = 1.4;

    dpImpl0->setValue(gplog);
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Same value.
    dpImpl1->setValue(gplog);
    OCIO_CHECK_ASSERT(*dp0 == *dp1);

    // One dynamic, not the other, same value.
    dpImpl0->makeDynamic();
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Both dynamic, same value.
    dpImpl1->makeDynamic();
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Both dynamic, different values.
    gplog.m_clampWhite = 1.3;
    dpImpl1->setValue(gplog);
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Different value types.
    OCIO::DynamicPropertyDoubleImplRcPtr dpImplDouble =
        std::make_shared<OCIO::DynamicPropertyDoubleImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, true);

    OCIO_CHECK_ASSERT(!(*dp0 == *dpImplDouble));
}

OCIO_ADD_TEST(DynamicPropertyImpl, equal_grading_rgb_curve)
{
    auto curveEdit = OCIO::GradingBSplineCurve::Create(2);
    OCIO::ConstGradingBSplineCurveRcPtr curve = curveEdit;
    OCIO::ConstGradingRGBCurveRcPtr rgbCurve = OCIO::GradingRGBCurve::Create(curve, curve, curve, curve);

    OCIO::DynamicPropertyGradingRGBCurveImplRcPtr dpImpl0 =
        std::make_shared<OCIO::DynamicPropertyGradingRGBCurveImpl>(rgbCurve, false);
    OCIO::DynamicPropertyRcPtr dp0 = dpImpl0;

    OCIO::DynamicPropertyGradingRGBCurveImplRcPtr dpImpl1 =
        std::make_shared<OCIO::DynamicPropertyGradingRGBCurveImpl>(rgbCurve, false);
    OCIO::DynamicPropertyRcPtr dp1 = dpImpl1;

    // Both not dynamic, same value.
    OCIO_CHECK_ASSERT(*dp0 == *dp1);

    // Both not dynamic, diff values.
    curveEdit->setNumControlPoints(3);
    OCIO::ConstGradingRGBCurveRcPtr rgbCurve1 = OCIO::GradingRGBCurve::Create(curve, curve, curve, curve);

    dpImpl0->setValue(rgbCurve1);
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Same value.
    dpImpl1->setValue(rgbCurve1);
    OCIO_CHECK_ASSERT(*dp0 == *dp1);

    // One dynamic, not the other, same value.
    dpImpl0->makeDynamic();
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Both dynamic, same value.
    dpImpl1->makeDynamic();
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Both dynamic, different values.
    dpImpl1->setValue(rgbCurve);
    OCIO_CHECK_ASSERT(!(*dp0 == *dp1));

    // Different value types.
    OCIO::DynamicPropertyDoubleImplRcPtr dpImplDouble =
        std::make_shared<OCIO::DynamicPropertyDoubleImpl>(OCIO::DYNAMIC_PROPERTY_EXPOSURE, 1.0, true);

    OCIO_CHECK_ASSERT(!(*dp0 == *dpImplDouble));
}

OCIO_ADD_TEST(DynamicPropertyImpl, setter_validation)
{
    // Make an identity dynamic transform.
    OCIO::GradingHueCurveTransformRcPtr gct = OCIO::GradingHueCurveTransform::Create(OCIO::GRADING_LOG);
    gct->makeDynamic();

    // Apply it on CPU.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    OCIO::ConstProcessorRcPtr processor = config->getProcessor(gct);
    OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();

    float pixel[3] = { 0.4f, 0.3f, 0.2f };
    cpuProcessor->applyRGB(pixel);

    const float error = 1e-5f;
    OCIO_CHECK_CLOSE(pixel[0], pixel[0], error);
    OCIO_CHECK_CLOSE(pixel[1], pixel[1], error);
    OCIO_CHECK_CLOSE(pixel[2], pixel[2], error);

    // Get a handle to the dynamic property.
    OCIO::DynamicPropertyRcPtr dp;
    OCIO_CHECK_NO_THROW(dp = cpuProcessor->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_HUECURVE));
    auto dpVal = OCIO::DynamicPropertyValue::AsGradingHueCurve(dp);
    OCIO_REQUIRE_ASSERT(dpVal);

    // Set a non-identity value.
    OCIO::GradingHueCurveRcPtr hueCurve = dpVal->getValue()->createEditableCopy();
    OCIO::GradingBSplineCurveRcPtr huehue = hueCurve->getCurve(OCIO::HUE_HUE);
    huehue->setNumControlPoints(3);
    huehue->getControlPoint(0) = OCIO::GradingControlPoint(0.f, -0.1f);
    huehue->getControlPoint(1) = OCIO::GradingControlPoint(0.5f, 0.5f);
    huehue->getControlPoint(2) = OCIO::GradingControlPoint(0.8f, 0.8f);
    dpVal->setValue(hueCurve);
    cpuProcessor->applyRGB(pixel);

    OCIO_CHECK_CLOSE(pixel[0], 0.4385873675f, error);
    OCIO_CHECK_CLOSE(pixel[1], 0.2829087377f, error);
    OCIO_CHECK_CLOSE(pixel[2], 0.2556785941f, error);

    // Ensure that validation of control points is happening as expected. Set the last point
    // so that it is no longer monotonic with respect to the first point. Because it is periodic,
    // the last point Y value becomes -0.05 when wrapped around to an X value of -0.2.
    huehue->getControlPoint(2) = OCIO::GradingControlPoint(0.8f, 0.95f);
    OCIO_CHECK_THROW_WHAT(dpVal->setValue(hueCurve),
                          OCIO::Exception,
                          "GradingHueCurve validation failed for 'hue_hue' curve with: Control point at index 0 "
                          "has a y coordinate '-0.1' that is less than previous control point y coordinate '-0.05'.");
}

OCIO_ADD_TEST(DynamicPropertyImpl, grading_rgb_curve_knots_coefs)
{
    auto curve11 = OCIO::GradingBSplineCurve::Create({ { 0.f, 10.f },{ 2.f, 10.f },{ 3.f, 10.f },
    { 5.f, 10.f },{ 6.f, 10.f },{ 8.f, 10.f },{ 9.f, 10.5f },{ 11.f, 15.f },{ 12.f, 50.f },
    { 14.f, 60.f },{ 15.f, 85.f } });
    // Identity curve.
    auto curve = OCIO::GradingBSplineCurve::Create({ { 0.f, 0.f },{ 1.f, 1.f } });

    // 1 curve with 11 control points used for green.
    auto curves = OCIO::GradingRGBCurve::Create(curve, curve11, curve, curve);

    OCIO::DynamicPropertyGradingRGBCurveImplRcPtr dp =
        std::make_shared<OCIO::DynamicPropertyGradingRGBCurveImpl>(curves, false);
    const int * coefsOffsets = dp->getCoefsOffsetsArray();
    const int * knotsOffsets = dp->getKnotsOffsetsArray();
    OCIO_CHECK_EQUAL(-1, coefsOffsets[0]); // Offset for red
    OCIO_CHECK_EQUAL(0,  coefsOffsets[1]); // Count for red
    OCIO_CHECK_EQUAL(0,  coefsOffsets[2]); // Offset for green
    OCIO_CHECK_EQUAL(45, coefsOffsets[3]); // Count for green
    OCIO_CHECK_EQUAL(-1, coefsOffsets[4]); // Offset for blue
    OCIO_CHECK_EQUAL(0,  coefsOffsets[5]); // Count for blue
    OCIO_CHECK_EQUAL(-1, coefsOffsets[6]); // Offset for master
    OCIO_CHECK_EQUAL(0,  coefsOffsets[7]); // Count for master
    OCIO_CHECK_EQUAL(-1, knotsOffsets[0]); // Offset for red
    OCIO_CHECK_EQUAL(0,  knotsOffsets[1]); // Count for red
    OCIO_CHECK_EQUAL(0,  knotsOffsets[2]); // Offset for green
    OCIO_CHECK_EQUAL(16, knotsOffsets[3]); // Count for green
    OCIO_CHECK_EQUAL(-1, knotsOffsets[4]); // Offset for blue
    OCIO_CHECK_EQUAL(0,  knotsOffsets[5]); // Count for blue
    OCIO_CHECK_EQUAL(-1, knotsOffsets[6]); // Offset for master
    OCIO_CHECK_EQUAL(0,  knotsOffsets[7]); // Count for master
    OCIO_CHECK_EQUAL(45, dp->getNumCoefs());
    OCIO_CHECK_EQUAL(16, dp->getNumKnots());

    const float * coefs = dp->getCoefsArray();
    const float * knots = dp->getKnotsArray();

    static constexpr float error = 1e-6f;
    OCIO_CHECK_CLOSE(0.f, coefs[0], error);
    OCIO_CHECK_CLOSE(0.f, coefs[1], error);
    OCIO_CHECK_CLOSE(0.f, coefs[2], error);
    OCIO_CHECK_CLOSE(0.f, coefs[3], error);
    OCIO_CHECK_CLOSE(0.f, coefs[4], error);
    OCIO_CHECK_CLOSE(0.337645531f, coefs[5], error);
    OCIO_CHECK_CLOSE(2.74714088f, coefs[6], error);
    OCIO_CHECK_CLOSE(0.081863299f, coefs[7], error);
    OCIO_CHECK_CLOSE(643.661987f, coefs[8], error);
    OCIO_CHECK_CLOSE(17.7471409f, coefs[9], error);
    OCIO_CHECK_CLOSE(-37.0891609f, coefs[10], error);
    OCIO_CHECK_CLOSE(-5.69135284f, coefs[11], error);
    OCIO_CHECK_CLOSE(3.83422971f, coefs[12], error);
    OCIO_CHECK_CLOSE(59.0043716f, coefs[13], error);
    OCIO_CHECK_CLOSE(1.69310224f, coefs[14], error);
    OCIO_CHECK_CLOSE(0.f, coefs[15], error);
    OCIO_CHECK_CLOSE(0.f, coefs[16], error);
    OCIO_CHECK_CLOSE(0.f, coefs[17], error);
    OCIO_CHECK_CLOSE(0.f, coefs[18], error);
    OCIO_CHECK_CLOSE(0.f, coefs[19], error);
    OCIO_CHECK_CLOSE(0.f, coefs[20], error);
    OCIO_CHECK_CLOSE(0.499999881f, coefs[21], error);
    OCIO_CHECK_CLOSE(1.92619848f, coefs[22], error);
    OCIO_CHECK_CLOSE(2.25f, coefs[23], error);
    OCIO_CHECK_CLOSE(30.9619350f, coefs[24], error);
    OCIO_CHECK_CLOSE(48.7090759f, coefs[25], error);
    OCIO_CHECK_CLOSE(11.6199141f, coefs[26], error);
    OCIO_CHECK_CLOSE(0.237208843f, coefs[27], error);
    OCIO_CHECK_CLOSE(7.90566826f, coefs[28], error);
    OCIO_CHECK_CLOSE(24.9999962f, coefs[29], error);
    OCIO_CHECK_CLOSE(10.f, coefs[30], error);
    OCIO_CHECK_CLOSE(10.f, coefs[31], error);
    OCIO_CHECK_CLOSE(10.f, coefs[32], error);
    OCIO_CHECK_CLOSE(10.f, coefs[33], error);
    OCIO_CHECK_CLOSE(10.f, coefs[34], error);
    OCIO_CHECK_CLOSE(10.f, coefs[35], error);
    OCIO_CHECK_CLOSE(10.1851053f, coefs[36], error);
    OCIO_CHECK_CLOSE(10.5f, coefs[37], error);
    OCIO_CHECK_CLOSE(14.6296263f, coefs[38], error);
    OCIO_CHECK_CLOSE(15.f, coefs[39], error);
    OCIO_CHECK_CLOSE(34.9177551f, coefs[40], error);
    OCIO_CHECK_CLOSE(50.f, coefs[41], error);
    OCIO_CHECK_CLOSE(55.9285622f, coefs[42], error);
    OCIO_CHECK_CLOSE(60.f, coefs[43], error);
    OCIO_CHECK_CLOSE(62.3833008f, coefs[44], error);

    OCIO_CHECK_CLOSE(0.f, knots[0], error);
    OCIO_CHECK_CLOSE(2.f, knots[1], error);
    OCIO_CHECK_CLOSE(3.f, knots[2], error);
    OCIO_CHECK_CLOSE(5.f, knots[3], error);
    OCIO_CHECK_CLOSE(6.f, knots[4], error);
    OCIO_CHECK_CLOSE(8.f, knots[5], error);
    OCIO_CHECK_CLOSE(8.74042130f, knots[6], error);
    OCIO_CHECK_CLOSE(9.f, knots[7], error);
    OCIO_CHECK_CLOSE(10.9776964f, knots[8], error);
    OCIO_CHECK_CLOSE(11.f, knots[9], error);
    OCIO_CHECK_CLOSE(11.5f, knots[10], error);
    OCIO_CHECK_CLOSE(12.f, knots[11], error);
    OCIO_CHECK_CLOSE(13.f, knots[12], error);
    OCIO_CHECK_CLOSE(14.f, knots[13], error);
    OCIO_CHECK_CLOSE(14.1448565f, knots[14], error);
    OCIO_CHECK_CLOSE(15.f, knots[15], error);

    // Using the 11 control points curve twice.
    curves = OCIO::GradingRGBCurve::Create(curve11, curve, curve11, curve);

    auto dp2 = std::make_shared<OCIO::DynamicPropertyGradingRGBCurveImpl>(curves, false);
    coefsOffsets = dp2->getCoefsOffsetsArray();
    knotsOffsets = dp2->getKnotsOffsetsArray();
    OCIO_CHECK_EQUAL(0,  coefsOffsets[0]); // Offset for red
    OCIO_CHECK_EQUAL(45, coefsOffsets[1]); // Count for red
    OCIO_CHECK_EQUAL(-1, coefsOffsets[2]); // Offset for green
    OCIO_CHECK_EQUAL(0,  coefsOffsets[3]); // Count for green
    OCIO_CHECK_EQUAL(45, coefsOffsets[4]); // Offset for blue
    OCIO_CHECK_EQUAL(45, coefsOffsets[5]); // Count for blue
    OCIO_CHECK_EQUAL(-1, coefsOffsets[6]); // Offset for master
    OCIO_CHECK_EQUAL(0,  coefsOffsets[7]); // Count for master
    OCIO_CHECK_EQUAL(90, dp2->getNumCoefs());
    OCIO_CHECK_EQUAL(32, dp2->getNumKnots());

    const float * coefs2 = dp2->getCoefsArray();
    const float * knots2 = dp2->getKnotsArray();

    for (int c = 0; c < 45; ++c)
    {
        OCIO_CHECK_EQUAL(coefs[c], coefs2[c]);
        OCIO_CHECK_EQUAL(coefs[c], coefs2[45 + c]);
    }

    for (int k = 0; k < 16; ++k)
    {
        OCIO_CHECK_EQUAL(knots[k], knots2[k]);
        OCIO_CHECK_EQUAL(knots[k], knots2[16 + k]);
    }

    // Verify that pointer does not change when setting data.
    auto dpPointer = dp.get();
    dp->setValue(curves);
    OCIO_CHECK_EQUAL(dp2->getNumCoefs(), dp->getNumCoefs());
    OCIO_CHECK_EQUAL(dp2->getNumKnots(), dp->getNumKnots());
    auto dpPointerAfterSet = dp.get();
    OCIO_CHECK_EQUAL(dpPointer, dpPointerAfterSet);
}

void checkKnotsAndCoefs(
    OCIO::DynamicPropertyGradingHueCurveImplRcPtr & dp,
    int set,
    const float * true_knots,
    const float * true_coefsA,
    const float * true_coefsB,
    const float * true_coefsC,
    unsigned line)
{
    const int * knotsOffsets = dp->getKnotsOffsetsArray();
    const int * coefsOffsets = dp->getCoefsOffsetsArray();
    const float * knots = dp->getKnotsArray();
    const float * coefs = dp->getCoefsArray();

    const int numKnots = knotsOffsets[set*2 + 1];
    for (int i = 0; i < numKnots; i++)
    {
        const int offset = knotsOffsets[set*2];
        OCIO_CHECK_CLOSE_FROM(knots[offset + i], true_knots[i], 1e-6, line);
    }
    const int numCoefSets = coefsOffsets[set*2 + 1] / 3;
    for (int i = 0; i < numCoefSets; i++)
    {
        const int offset = coefsOffsets[set*2];
        OCIO_CHECK_CLOSE_FROM(coefs[offset + i], true_coefsA[i], 3e-4, line);
        OCIO_CHECK_CLOSE_FROM(coefs[offset + numCoefSets + i], true_coefsB[i], 1e-5, line);
        OCIO_CHECK_CLOSE_FROM(coefs[offset + 2*numCoefSets + i], true_coefsC[i], 1e-5, line);
    }
}

OCIO_ADD_TEST(DynamicPropertyImpl, grading_hue_curve_knots_coefs)
{
     auto hh = OCIO::GradingBSplineCurve::Create(
        { {0.1f, 0.05f}, {0.2f, 0.3f}, {0.5f, 0.4f}, {0.8f, 0.7f}, {0.9f, 0.75f}, {1.0f, 0.9f} },
        OCIO::HUE_HUE);
    auto hs = OCIO::GradingBSplineCurve::Create(
        { {-0.15f, 1.25f}, {0.f, 0.8f}, {0.2f, 0.9f}, {0.4f, 1.8f}, {0.6f, 1.4f}, {0.8f, 1.3f}, {0.9f, 1.1f}, {1.1f, 0.7f} },
        OCIO::HUE_SAT);
    auto hl = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.f}, {0.22f, 0.077f}, {0.36f, 0.092f}, {0.51f, 0.27f}, {0.67f, 0.f}, {0.83f, 0.f} },
        OCIO::HUE_LUM);
    // The rest are identities, but not the default curves.
    auto ls = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.f}, {1.f, 1.f} }, 
        OCIO::LUM_SAT);
    auto ss = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.f}, {0.25f, 0.25f}, {1.f, 1.f} },
        OCIO::SAT_SAT);
    auto ll = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.f}, {0.25f, 0.25f}, {0.5f, 0.5f}, {1.f, 1.f} },
        OCIO::LUM_LUM);
    auto sl = OCIO::GradingBSplineCurve::Create(
        { {0.f, 1.f}, {0.25f, 1.f}, {0.5f, 1.f}, {1.f, 1.f} },
        OCIO::SAT_LUM);
    auto hfx = OCIO::GradingBSplineCurve::Create(
        { {0.f, 0.f}, {0.1f, 0.f}, {0.2f, 0.f}, {0.4f, 0.f}, {0.6f, 0.f}, {0.8f, 0.f} },
        OCIO::HUE_FX);

    auto curves = OCIO::GradingHueCurve::Create(hh, hs, hl, ls, ss, ll, sl, hfx);

    {
        // Fit the polynomials.
        OCIO::DynamicPropertyGradingHueCurveImplRcPtr dp =
            std::make_shared<OCIO::DynamicPropertyGradingHueCurveImpl>(curves, false);

        OCIO_CHECK_EQUAL(46, dp->getNumKnots());
        OCIO_CHECK_EQUAL(129, dp->getNumCoefs());

        const int * coefsOffsets = dp->getCoefsOffsetsArray();
        const int * knotsOffsets = dp->getKnotsOffsetsArray();

        // These are offset0, count0, offset1, count1, offset2, count2, ...
        const int true_knotsOffsets[] = {0, 15, 15, 19, 34, 12, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0};
        const int true_coefsOffsets[] = {0, 42, 42, 54, 96, 33, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0};
        OCIO_CHECK_EQUAL(16, dp->GetNumOffsetValues());
        for (int i=0; i < dp->GetNumOffsetValues(); i++)
        {
            OCIO_CHECK_EQUAL(knotsOffsets[i], true_knotsOffsets[i]);
            OCIO_CHECK_EQUAL(coefsOffsets[i], true_coefsOffsets[i]);
        }
    }

    // Repeat the test in DrawCurveOnly mode. This will yield identity knots and coefs for the
    // curves that are identities.

    curves->setDrawCurveOnly(true);

    OCIO::DynamicPropertyGradingHueCurveImplRcPtr dp =
        std::make_shared<OCIO::DynamicPropertyGradingHueCurveImpl>(curves, false);

    OCIO_CHECK_EQUAL(56, dp->getNumKnots());
    OCIO_CHECK_EQUAL(144, dp->getNumCoefs());

    const int * coefsOffsets = dp->getCoefsOffsetsArray();
    const int * knotsOffsets = dp->getKnotsOffsetsArray();

    const int true_knotsOffsets[] = {0, 15, 15, 19, 34, 12, 46, 2, 48, 2, 50, 2, 52, 2, 54, 2};
    const int true_coefsOffsets[] = {0, 42, 42, 54, 96, 33, 129, 3, 132, 3, 135, 3, 138, 3, 141, 3};
    for (int i=0; i < dp->GetNumOffsetValues(); i++)
    {
        OCIO_CHECK_EQUAL(knotsOffsets[i], true_knotsOffsets[i]);
        OCIO_CHECK_EQUAL(coefsOffsets[i], true_coefsOffsets[i]);
    }

    {
        // Hue-Hue
        const float true_knots[15] = {-0.1f, -0.06928571f, 0.0f, 0.05642857f, 0.1f, 0.17549634f, 0.2f, 0.33714286f,
                                       0.5f,  0.62499860f, 0.8f, 0.85261905f, 0.9f, 0.93071429f, 1.f };

        // Quadratic coefs.
        const float true_coefsA[14] = { 15.95930233f, -1.66237113f, -1.44778481f, 6.17827869f, 10.39930009f,
                                       -58.70626575f, -1.54375789f,  1.03834397f, 3.7077401f,  -2.12344738f,
                                        -3.54260935f,  4.81365159f, 15.95930233f,-1.66237113f };
        // Linear coefs.
        const float true_coefsB[14] = { 0.75f, 1.73035714f, 1.5f, 1.33660714f, 1.875f, 3.44521825f, 0.56818182f,
                                        0.14475108f, 0.48295455f, 1.40987919f, 0.66666667f, 0.29384921f, 0.75f, 1.73035714f };

        // Constant coefs.
        const float true_coefsC[14] = { -0.25f, -0.2119088f, -0.1f, -0.01996716f, 0.05f, 0.25082851f, 0.3f, 
                                         0.34888683f, 0.4f, 0.51830078f, 0.7f, 0.72527072f, 0.75f, 0.7880912f };

        checkKnotsAndCoefs(dp, 0, true_knots, true_coefsA, true_coefsB, true_coefsC, __LINE__);
    }
    {
        // Hue-Sat

        const float true_knots[19] = { -0.1f, -0.03071429f, 0.f, 0.0625f, 0.1f, 0.13333333f, 0.2f, 0.34913793f, 0.4f, 
                                        0.46896552f, 0.6f, 0.69f, 0.8f, 0.82770833f,  0.85f, 0.86535714f, 0.9f, 0.96928571f, 1.f };
        const float true_coefsA[18] = { -3.32474227f, 31.91860465f, 3.5f, 14.16666667f, 32.30769231f, 4.61538462f,
                                        13.9662072f, -68.17470665f, -25.2f, 10.21052632f, 2.92592593f, -1.78787879f,
                                        -5.32581454f, -12.07165109f, -63.8372093f, 6.64948454f, -3.32474227f, 31.91860465f };
        const float true_coefsB[18] = { -3.f, -3.46071429f, -1.5f, -1.0625f, 0.f, 2.15384615f, 2.76923077f, 6.93501326f, 0.f, -3.47586207f, 
                                        -0.8f, -0.27333333f, -0.66666667f, -0.96180556f, -1.5f, -3.46071429f, -3.f, -3.46071429f };
        const float true_coefsC[18] = { 1.1f, 0.8761824f, 0.8f, 0.71992187f, 0.7f, 0.73589744f, 0.9f, 1.62363544f, 1.8f, 
                                        1.68014269f, 1.4f, 1.3517f, 1.3f, 1.27743887f, 1.25f, 1.2119088f, 1.1f, 0.8761824f };

        checkKnotsAndCoefs(dp, 1, true_knots, true_coefsA, true_coefsB, true_coefsC, __LINE__);
    }
    {
        // Hue-Lum
        // Test for the "Adjust slopes that are not shape-preserving" path in EstimateHueSlopes.

        const float true_knots[12] = { -0.17f, 0.f, 0.07049104f, 0.22f, 0.29691485f, 0.36f, 0.435f, 0.51f, 0.59f, 0.67f, 0.83f, 1.f };

        const float true_coefsA[11] = { 0.f, 4.21997107f, -1.47264319f, -0.70657119f, 1.10402357f, 13.97025263f, 
                                       -15.20489902f, -21.09375f, 21.09375f, 0.f, 0.f };
        const float true_coefsB[11] = { 0.f, 0.f, 0.59494032f, 0.15459362f, 0.04590198f, 0.18519696f, 2.28073485f, 
                                        0.f, -3.375f, 0.f, 0.f };
        const float true_coefsC[11] = { 0.f, 0.f, 0.02096898f, 0.077f, 0.08471054f, 0.092f, 0.18447244f, 0.27f, 
                                        0.135f, 0.f, 0.f };

        checkKnotsAndCoefs(dp, 2, true_knots, true_coefsA, true_coefsB, true_coefsC, __LINE__);
    }
    {
        // Horizontal identities

        const float true_knots[2] = { 0.f, 1.f };
        const float true_coefsA[1] = { 0.f };
        const float true_coefsB[1] = { 0.f };
        const float true_coefsC[1] = { 1.f };
        const float true_coefsCfx[1] = { 0.f };

        // Lum-Sat
        checkKnotsAndCoefs(dp, 3, true_knots, true_coefsA, true_coefsB, true_coefsC, __LINE__);
        // Sat-Lum
        checkKnotsAndCoefs(dp, 6, true_knots, true_coefsA, true_coefsB, true_coefsC, __LINE__);
        // Hue-Fx
        checkKnotsAndCoefs(dp, 7, true_knots, true_coefsA, true_coefsB, true_coefsCfx, __LINE__);
    }
    {
        // Diagonal identities

        const float true_knots[2] = { 0.f, 1.f };
        const float true_coefsA[1] = { 0.f };
        const float true_coefsB[1] = { 1.f };
        const float true_coefsC[1] = { 0.f };

        // Sat-Sat
        checkKnotsAndCoefs(dp, 4, true_knots, true_coefsA, true_coefsB, true_coefsC, __LINE__);
        // Lum-Lum
        checkKnotsAndCoefs(dp, 5, true_knots, true_coefsA, true_coefsB, true_coefsC, __LINE__);
    }
}

OCIO_ADD_TEST(DynamicPropertyImpl, get_as)
{
    OCIO::GradingPrimary gplog{ OCIO::GRADING_LOG };
    gplog.m_saturation = 1.21;

    OCIO::DynamicPropertyGradingPrimaryImplRcPtr dpImpl0 =
        std::make_shared<OCIO::DynamicPropertyGradingPrimaryImpl>(OCIO::GRADING_LOG,
                                                                  OCIO::TRANSFORM_DIR_FORWARD,
                                                                  gplog, false);

    OCIO::DynamicPropertyRcPtr dp0 = dpImpl0;
    OCIO_CHECK_THROW_WHAT(OCIO::DynamicPropertyValue::AsDouble(dp0), OCIO::Exception,
                          "Dynamic property value is not a double");
    OCIO_CHECK_THROW_WHAT(OCIO::DynamicPropertyValue::AsGradingTone(dp0), OCIO::Exception,
                          "Dynamic property value is not a grading tone");
    OCIO::DynamicPropertyGradingPrimaryRcPtr asPrimary;
    OCIO_CHECK_NO_THROW(asPrimary = OCIO::DynamicPropertyValue::AsGradingPrimary(dp0));
    OCIO_REQUIRE_ASSERT(asPrimary);

    OCIO_CHECK_EQUAL(asPrimary->getValue(), gplog);
    gplog.m_pivot = 0.12;
    asPrimary->setValue(gplog);
    OCIO_CHECK_EQUAL(dpImpl0->getValue(), gplog);
}