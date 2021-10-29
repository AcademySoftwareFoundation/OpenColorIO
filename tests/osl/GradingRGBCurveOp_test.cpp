// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// NOTE:
// The file is a copy and paste from the corresponding GPU unitest i.e. []/tests/gpu/GradingRGBCurveOp_test.cpp.
// Keep the two files in sync. to increase the test coverage.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


void GradingRGBCurveLog(OSLDataRcPtr & data, OCIO::TransformDirection dir, bool dynamic)
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

    data->m_transform = gc;

    data->m_threshold = 2e-5f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingRGBCurve, style_log_fwd)
{
    GradingRGBCurveLog(m_data, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_OSL_TEST(GradingRGBCurve, style_log_fwd_dynamic)
{
    GradingRGBCurveLog(m_data, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_OSL_TEST(GradingRGBCurve, style_log_rev)
{
    GradingRGBCurveLog(m_data, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_OSL_TEST(GradingRGBCurve, style_log_rev_dynamic)
{
    GradingRGBCurveLog(m_data, OCIO::TRANSFORM_DIR_INVERSE, true);
}

void GradingRGBCurveLin(OSLDataRcPtr & data, OCIO::TransformDirection dir, bool dynamic)
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

    data->m_transform = gc;

    data->m_threshold = 1.5e-4f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingRGBCurve, style_lin_fwd)
{
    GradingRGBCurveLin(m_data, OCIO::TRANSFORM_DIR_FORWARD, false);
}

OCIO_OSL_TEST(GradingRGBCurve, style_lin_fwd_dynamic)
{
    GradingRGBCurveLin(m_data, OCIO::TRANSFORM_DIR_FORWARD, true);
}

OCIO_OSL_TEST(GradingRGBCurve, style_lin_rev)
{
    GradingRGBCurveLin(m_data, OCIO::TRANSFORM_DIR_INVERSE, false);
}

OCIO_OSL_TEST(GradingRGBCurve, style_lin_rev_dynamic)
{
    GradingRGBCurveLin(m_data, OCIO::TRANSFORM_DIR_INVERSE, true);
}

OCIO_OSL_TEST(GradingRGBCurve, style_log_dynamic_retests)
{
    auto gc = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
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

    m_data->m_transform = gc;

    m_data->m_threshold = 5e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(GradingRGBCurve, two_transforms_retests)
{
    auto gcDyn = OCIO::GradingRGBCurveTransform::Create(OCIO::GRADING_LOG);
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

    m_data->m_transform = group;

    m_data->m_threshold = 5e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}
