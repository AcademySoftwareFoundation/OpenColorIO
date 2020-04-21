// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <stdio.h>
#include <sstream>
#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "GPUHelpers.h"
#include "GPUUnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


const float g_epsilon = 1e-6f;

OCIO_ADD_GPU_TEST(RangeOp, scale_with_low_and_high_clippings)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.1f);
    range->setMaxInValue(1.1f);
    range->setMinOutValue(0.5f);
    range->setMaxOutValue(1.5f);

    test.setProcessor(range);

    test.setTestWideRange(true);
    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_low_clipping)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.2f);
    range->setMinOutValue(0.2f);

    test.setProcessor(range);

    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_high_clipping)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMaxInValue(0.9f);
    range->setMaxOutValue(0.9f);

    test.setProcessor(range);
    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, scale_with_low_and_high_clippings_2)
{
    // The similar test above is only an offset, this test is a scale & offset.
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(0.1f);
    range->setMaxInValue(1.1f);
    range->setMinOutValue(-0.5f);
    range->setMaxOutValue(1.5f);

    test.setProcessor(range);

    test.setTestWideRange(true);
    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, arbitrary_1)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue (0.4000202f);
    range->setMaxInValue (0.6000502f);
    range->setMinOutValue(0.4000601f);
    range->setMaxOutValue(0.6000801f);

    test.setProcessor(range);
    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, arbitrary_1_no_clamp)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setStyle(OCIO::RANGE_NO_CLAMP);
    range->setMinInValue (0.4000202f);
    range->setMaxInValue (0.6000502f);
    range->setMinOutValue(0.4000601f);
    range->setMaxOutValue(0.6000801f);

    test.setProcessor(range);
    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, arbitrary_2)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setMinInValue(-0.010201f);
    range->setMaxInValue (0.601102f);
    range->setMinOutValue(0.209803f);
    range->setMaxOutValue(1.600208f);

    test.setProcessor(range);

    test.setTestWideRange(true);
    test.setErrorThreshold(g_epsilon);
}

OCIO_ADD_GPU_TEST(RangeOp, arbitrary_2_no_clamp)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    range->setStyle(OCIO::RANGE_NO_CLAMP);
    range->setMinInValue(-0.010201f);
    range->setMaxInValue (0.601102f);
    range->setMinOutValue(0.209803f);
    range->setMaxOutValue(1.600208f);

    test.setProcessor(range);

    test.setTestWideRange(true);
    test.setErrorThreshold(g_epsilon);
}
