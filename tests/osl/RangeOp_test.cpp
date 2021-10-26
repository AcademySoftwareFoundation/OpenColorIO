// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// NOTE:
// The file is a copy and paste from the corresponding GPU unitest i.e. []/tests/gpu/RangeOp_test.cpp.
// Keep the two files in sync. to increase the test coverage.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


OCIO_OSL_TEST(RangeOp, scale_with_low_and_high_clippings)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.1f);
    range->setMaxInValue(1.1f);
    range->setMinOutValue(0.5f);
    range->setMaxOutValue(1.5f);

    m_data->m_transform = range;
}

OCIO_OSL_TEST(RangeOp, scale_with_low_clipping)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.2f);
    range->setMinOutValue(0.2f);

    m_data->m_transform = range;
}

OCIO_OSL_TEST(RangeOp, scale_with_high_clipping)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMaxInValue(0.9f);
    range->setMaxOutValue(0.9f);

    m_data->m_transform = range;
}

OCIO_OSL_TEST(RangeOp, scale_with_low_and_high_clippings_2)
{
    // The similar test above is only an offset, this test is a scale & offset.
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.1f);
    range->setMaxInValue(1.1f);
    range->setMinOutValue(-0.5f);
    range->setMaxOutValue(1.5f);

    m_data->m_transform = range;
}

OCIO_OSL_TEST(RangeOp, arbitrary_1)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue (0.4000202f);
    range->setMaxInValue (0.6000502f);
    range->setMinOutValue(0.4000601f);
    range->setMaxOutValue(0.6000801f);

    m_data->m_transform = range;
}

OCIO_OSL_TEST(RangeOp, arbitrary_1_no_clamp)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setStyle(OCIO::RANGE_NO_CLAMP);
    range->setMinInValue (0.4000202f);
    range->setMaxInValue (0.6000502f);
    range->setMinOutValue(0.4000601f);
    range->setMaxOutValue(0.6000801f);

    m_data->m_transform = range;
}

OCIO_OSL_TEST(RangeOp, arbitrary_2)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(-0.010201f);
    range->setMaxInValue (0.601102f);
    range->setMinOutValue(0.209803f);
    range->setMaxOutValue(1.600208f);

    m_data->m_transform = range;
}

OCIO_OSL_TEST(RangeOp, arbitrary_2_no_clamp)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setStyle(OCIO::RANGE_NO_CLAMP);
    range->setMinInValue(-0.010201f);
    range->setMaxInValue (0.601102f);
    range->setMinOutValue(0.209803f);
    range->setMaxOutValue(1.600208f);

    m_data->m_transform = range;
}
