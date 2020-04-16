// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <algorithm>

#include "transforms/RangeTransform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(RangeTransform, basic)
{
    OCIO::RangeTransformRcPtr range = OCIO::RangeTransform::Create();
    OCIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(range->getStyle(), OCIO::RANGE_CLAMP);
    OCIO_CHECK_ASSERT(!range->hasMinInValue());
    OCIO_CHECK_ASSERT(!range->hasMaxInValue());
    OCIO_CHECK_ASSERT(!range->hasMinOutValue());
    OCIO_CHECK_ASSERT(!range->hasMaxOutValue());

    range->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(range->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

    range->setStyle(OCIO::RANGE_NO_CLAMP);
    OCIO_CHECK_EQUAL(range->getStyle(), OCIO::RANGE_NO_CLAMP);

    range->setMinInValue(-0.5);
    OCIO_CHECK_EQUAL(range->getMinInValue(), -0.5);
    OCIO_CHECK_ASSERT(range->hasMinInValue());

    OCIO::RangeTransformRcPtr range2 = OCIO::RangeTransform::Create();
    range2->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
    range2->setMinInValue(-0.5);
    range2->setStyle(OCIO::RANGE_NO_CLAMP);
    OCIO_CHECK_ASSERT(range2->equals(*range));

    range2->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    range2->setMinInValue(-1.5);
    range2->setMaxInValue(-0.5);
    range2->setMinOutValue(1.5);
    range2->setMaxOutValue(4.5);

    OCIO_CHECK_EQUAL(range2->getFileInputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);
    OCIO_CHECK_EQUAL(range2->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UNKNOWN);

    range2->setFileInputBitDepth(OCIO::BIT_DEPTH_UINT8);
    range2->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    OCIO_CHECK_EQUAL(range2->getFileInputBitDepth(), OCIO::BIT_DEPTH_UINT8);
    OCIO_CHECK_EQUAL(range2->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    OCIO_CHECK_EQUAL(range2->getMinInValue(), -1.5);
    OCIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5);
    OCIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5);
    OCIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5);

    range2->unsetMinInValue();

    // (Note that the transform would not validate at this point.)

    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5);
    OCIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5);
    OCIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5);

    range2->setMinInValue(-1.5f);
    OCIO_CHECK_EQUAL(range2->getMinInValue(), -1.5);
    OCIO_CHECK_EQUAL(range2->getMaxInValue(), -0.5);
    OCIO_CHECK_EQUAL(range2->getMinOutValue(), 1.5);
    OCIO_CHECK_EQUAL(range2->getMaxOutValue(), 4.5);

    OCIO_CHECK_ASSERT(range2->hasMinInValue());
    OCIO_CHECK_ASSERT(range2->hasMaxInValue());
    OCIO_CHECK_ASSERT(range2->hasMinOutValue());
    OCIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMinInValue();
    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_ASSERT(range2->hasMaxInValue());
    OCIO_CHECK_ASSERT(range2->hasMinOutValue());
    OCIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMaxInValue();
    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OCIO_CHECK_ASSERT(range2->hasMinOutValue());
    OCIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMinOutValue();
    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OCIO_CHECK_ASSERT(!range2->hasMinOutValue());
    OCIO_CHECK_ASSERT(range2->hasMaxOutValue());

    range2->unsetMaxOutValue();
    OCIO_CHECK_ASSERT(!range2->hasMinInValue());
    OCIO_CHECK_ASSERT(!range2->hasMaxInValue());
    OCIO_CHECK_ASSERT(!range2->hasMinOutValue());
    OCIO_CHECK_ASSERT(!range2->hasMaxOutValue());
}
