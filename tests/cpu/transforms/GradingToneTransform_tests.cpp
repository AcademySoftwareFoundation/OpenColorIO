// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "transforms/GradingToneTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(GradingToneTransform, basic)
{
    // Create transform and validate default values for all styles.
    auto gttLin = OCIO::GradingToneTransform::Create(OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gttLin->getStyle(), OCIO::GRADING_LIN);

    const OCIO::GradingTone toneDefaultsLin(OCIO::GRADING_LIN);
    OCIO_CHECK_EQUAL(gttLin->getValue(), toneDefaultsLin);
    OCIO::GradingTone tone(OCIO::GRADING_LIN);
    tone.m_scontrast += 0.123;
    tone.m_blacks.m_red += 0.321;
    tone.m_blacks.m_start += 0.1;
    gttLin->setValue(tone);
    OCIO_CHECK_EQUAL(gttLin->getValue(), tone);

    OCIO_CHECK_EQUAL(gttLin->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    gttLin->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(gttLin->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_ASSERT(!gttLin->isDynamic());
    gttLin->makeDynamic();
    OCIO_CHECK_ASSERT(gttLin->isDynamic());
    gttLin->makeNonDynamic();
    OCIO_CHECK_ASSERT(!gttLin->isDynamic());

    OCIO_CHECK_NO_THROW(gttLin->validate());
    tone.m_blacks.m_width = 0.0001;
    OCIO_CHECK_THROW_WHAT(gttLin->setValue(tone), OCIO::Exception, "is below lower bound (0.01)");
    tone.m_blacks.m_width = 1.;
    tone.m_blacks.m_red = 2.1;
    OCIO_CHECK_THROW_WHAT(gttLin->setValue(tone), OCIO::Exception, "are above upper bound (1.9)");

    auto gttLog = OCIO::GradingToneTransform::Create(OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gttLog->getStyle(), OCIO::GRADING_LOG);
    const OCIO::GradingTone toneDefaultsLog(OCIO::GRADING_LOG);
    OCIO_CHECK_EQUAL(gttLog->getValue(), toneDefaultsLog);
    auto gttVid = OCIO::GradingToneTransform::Create(OCIO::GRADING_VIDEO);
    OCIO_CHECK_EQUAL(gttVid->getStyle(), OCIO::GRADING_VIDEO);
    const OCIO::GradingTone toneDefaultsVid(OCIO::GRADING_VIDEO);
    OCIO_CHECK_EQUAL(gttVid->getValue(), toneDefaultsVid);
}

OCIO_ADD_TEST(GradingToneTransform, serialization)
{
    // Test the serialization of the transform.

    OCIO::GradingTone data(OCIO::GRADING_LIN);
    data.m_scontrast      += 0.123;
    data.m_blacks.m_red   += 0.321;
    data.m_blacks.m_start += 0.1;

    auto tone = OCIO::GradingToneTransform::Create(OCIO::GRADING_LIN);
    tone->setValue(data);

    static constexpr char TONE_STR[]
        = "<GradingToneTransform direction=forward, style=linear, values=<"\
          "blacks=<red=1.321 green=1 blue=1 master=1 start=0.1 width=4> "\
          "shadows=<red=1 green=1 blue=1 master=1 start=2 width=-7> "\
          "midtones=<red=1 green=1 blue=1 master=1 start=0 width=8> "\
          "highlights=<red=1 green=1 blue=1 master=1 start=-2 width=9> "\
          "whites=<red=1 green=1 blue=1 master=1 start=0 width=8> s_contrast=1.123>>";

    {
        std::ostringstream oss;
        oss << *tone;

        OCIO_CHECK_EQUAL(oss.str(), TONE_STR);
    }

    OCIO::GroupTransformRcPtr grp = OCIO::GroupTransform::Create();
    grp->appendTransform(OCIO::DynamicPtrCast<OCIO::Transform>(tone));

    {
        std::ostringstream oss;
        oss << *grp;

        std::string GROUP_STR("<GroupTransform direction=forward, transforms=\n");
        GROUP_STR += "        ";
        GROUP_STR += TONE_STR;
        GROUP_STR += ">";

        OCIO_CHECK_EQUAL(oss.str(), GROUP_STR);
    }
}

OCIO_ADD_TEST(GradingToneTransform, local_bypass)
{
    // Test the local bypass behavior.

    OCIO::GradingToneTransformRcPtr transform
        = OCIO::GradingToneTransform::Create(OCIO::GRADING_LOG);

    OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();

    {
        // Test that the GPU is empty for an identity transform.

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

    {
        transform->makeDynamic();
    
        const auto proc = config->getProcessor(transform);
        const auto cpu = proc->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_NONE);

        // Values are unchanged (to within 32f precision).

        float v1[3] {0.3f, 0.4f, 0.5f};
        cpu->applyRGB(v1);
        OCIO_CHECK_CLOSE_FROM(v1[0], 0.30000001192092896f, std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_CLOSE_FROM(v1[1], 0.4000000059604645f,  std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_EQUAL(v1[2], 0.5f);

        // Try a value > HalfMax = 65504, note it is not clamped.  Therefore localBypass is being used.

        float v2[3] {0.3f, 0.4f, 65550.0f};
        cpu->applyRGB(v2);
        OCIO_CHECK_CLOSE_FROM(v2[0], 0.30000001192092896f, std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_CLOSE_FROM(v2[1], 0.4000000059604645f,  std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_EQUAL(v2[2], 65550.0f);

        // Set the midtones control so it is no longer an identity.

        auto valst = OCIO::GradingTone(OCIO::GRADING_LOG);
        valst.m_midtones = OCIO::GradingRGBMSW(0.3, 1.0, 1.8,  1.2, 0.37, 0.6);
        auto dp = cpu->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_TONE);
        auto propGT = OCIO::DynamicPropertyValue::AsGradingTone(dp);
        OCIO_CHECK_ASSERT(propGT);
        OCIO_CHECK_NO_THROW(propGT->setValue(valst));

        // It is affecting the midtones.

        float v3[3] {0.3f, 0.4f, 0.5f};
        cpu->applyRGB(v3);
        OCIO_CHECK_CLOSE_FROM(v3[0], 0.20410963892936707f, std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_CLOSE_FROM(v3[1], 0.4343799948692322f,  std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_CLOSE_FROM(v3[2], 0.6093657612800598f,  std::numeric_limits<float>::max_digits10, __LINE__);

        // The max value is now clamped, so localBypass is not being used.

        float v4[3] {0.3f, 0.4f, 65550.0f};
        cpu->applyRGB(v4);
        OCIO_CHECK_CLOSE_FROM(v4[0], 0.20410963892936707f, std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_CLOSE_FROM(v4[1], 0.4343799948692322f,  std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_EQUAL(v4[2], 65504.0f);

        // The midtones control does not affect large values, so just clarify that large values
        // below 65504 are not affected.

        float v5[3] {0.3f, 0.4f, 65500.0f};
        cpu->applyRGB(v5);
        OCIO_CHECK_CLOSE_FROM(v5[0], 0.20410963892936707f, std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_CLOSE_FROM(v5[1], 0.4343799948692322f,  std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_EQUAL(v5[2], 65500.0f);

        // Set the midtones values back to its default.

        valst.m_midtones = OCIO::GradingRGBMSW(1., 1.0, 1.,  1., 0.4, 0.6);
        OCIO_CHECK_NO_THROW(propGT->setValue(valst));

        // The behavior is now the same as originally.

        float v6[3] {0.3f, 0.4f, 0.5f};
        cpu->applyRGB(v6);
        OCIO_CHECK_CLOSE_FROM(v6[0], 0.30000001192092896f, std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_CLOSE_FROM(v6[1], 0.4000000059604645f,  std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_EQUAL(v6[2], 0.5f);

        float v7[3] {0.3f, 0.4f, 65550.0f};
        cpu->applyRGB(v7);
        OCIO_CHECK_CLOSE_FROM(v7[0], 0.30000001192092896f, std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_CLOSE_FROM(v7[1], 0.4000000059604645f,  std::numeric_limits<float>::max_digits10, __LINE__);
        OCIO_CHECK_EQUAL(v7[2], 65550.0f);
    }
}
