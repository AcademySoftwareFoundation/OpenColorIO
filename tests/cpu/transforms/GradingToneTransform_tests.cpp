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
