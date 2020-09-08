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
