// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// NOTE:
// The file is a copy and paste from the corresponding GPU unitest i.e. []/tests/gpu/ECOp_test.cpp.
// Keep the two files in sync. to increase the test coverage.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


OCIO_OSL_TEST(ExposureContrast, style_linear_fwd)
{
    auto ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LINEAR);
    ec->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    ec->setExposure(1.1);
    ec->setContrast(0.8);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    m_data->m_transform = ec;

    m_data->m_threshold = 2e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(ExposureContrast, style_linear_rev)
{
    auto ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LINEAR);
    ec->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    ec->setExposure(1.1);
    ec->setContrast(0.7);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    m_data->m_transform = ec;

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 2e-5f
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(ExposureContrast, style_video_fwd)
{
    auto ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_VIDEO);
    ec->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    ec->setExposure(1.1);
    ec->setContrast(0.8);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    m_data->m_transform = ec;

    m_data->m_threshold = 2e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(ExposureContrast, style_video_rev)
{
    auto ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_VIDEO);
    ec->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    ec->setExposure(1.1);
    ec->setContrast(0.7);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    m_data->m_transform = ec;

    m_data->m_threshold = 5e-5f; // Slight difference with the GLSL unit test i.e. 2e-5f
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(ExposureContrast, style_log_fwd)
{
    auto ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);
    ec->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    ec->setExposure(1.1);
    ec->setContrast(0.8);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    m_data->m_transform = ec;

    m_data->m_threshold = 1e-6f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(ExposureContrast, style_log_rev)
{
    auto ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);
    ec->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    ec->setExposure(1.1);
    ec->setContrast(0.7);
    ec->setGamma(0.9);
    ec->setPivot(0.22);

    m_data->m_transform = ec;

    m_data->m_threshold = 1e-6f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

OCIO_OSL_TEST(ExposureContrast, style_linear_dynamic_parameter)
{
    auto ec = OCIO::ExposureContrastTransform::Create();
    ec->setStyle(OCIO::EXPOSURE_CONTRAST_LINEAR);
    ec->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    ec->setExposure(1.1);
    ec->setContrast(0.8);
    ec->setGamma(0.9);
    ec->setPivot(0.22);
    ec->makeExposureDynamic();
    ec->makeContrastDynamic();
    ec->makeGammaDynamic();

    m_data->m_transform = ec;

    m_data->m_threshold = 5e-5f;
    m_data->m_expectedMinimalValue = 1.0f;
    m_data->m_relativeComparison = true;
}

void Prepare2ECDynamic(OSLDataRcPtr & data, bool firstDyn, bool secondDyn)
{
    // See also OCIO_ADD_TEST(ExposureContrastTransform, processor_several_ec).
    OCIO::ExposureContrastTransformRcPtr ec1 = OCIO::ExposureContrastTransform::Create();
    ec1->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);

    ec1->setExposure(0.8);
    ec1->setContrast(0.5);
    ec1->setGamma(1.5);

    OCIO::ExposureContrastTransformRcPtr ec2 = OCIO::ExposureContrastTransform::Create();
    ec2->setStyle(OCIO::EXPOSURE_CONTRAST_LOGARITHMIC);

    ec2->setExposure(0.8);
    ec2->setContrast(0.5);
    ec2->setGamma(1.5);

    if (firstDyn) ec1->makeContrastDynamic();
    if (secondDyn) ec2->makeExposureDynamic();

    OCIO::GroupTransformRcPtr grp = OCIO::GroupTransform::Create();
    grp->appendTransform(ec1);
    grp->appendTransform(ec2);

    data->m_transform = grp;

    data->m_threshold = 5e-5f;
    data->m_expectedMinimalValue = 1.0f;
    data->m_relativeComparison = true;
}

OCIO_OSL_TEST(ExposureContrast, dp_several_one_dynamic)
{
    // 2 EC, first not dynamic, second dynamic.
    Prepare2ECDynamic(m_data, false, true);
}

OCIO_OSL_TEST(ExposureContrast, dp_several_both_dynamic)
{
    // 2 EC, both dynamic.
    Prepare2ECDynamic(m_data, true, true);
}
