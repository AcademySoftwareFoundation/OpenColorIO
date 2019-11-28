// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "BitDepthUtils.cpp"

#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(BitDepthUtils, get_bitdepth_max_value)
{
    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT8), 255.0);
    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT16), 65535.0);

    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F16), 1.0);
    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32), 1.0);

    OCIO_CHECK_THROW_WHAT(
        OCIO::GetBitDepthMaxValue((OCIO::BitDepth)42), OCIO::Exception, "not supported");
}

OCIO_ADD_TEST(BitDepthUtils, is_float_bitdepth)
{
    OCIO_CHECK_ASSERT(!OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT8));
    OCIO_CHECK_ASSERT(!OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT10));
    OCIO_CHECK_ASSERT(!OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT12));
    OCIO_CHECK_ASSERT(!OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT16));

    OCIO_CHECK_ASSERT(OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_F16));
    OCIO_CHECK_ASSERT(OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_F32));

    OCIO_CHECK_THROW_WHAT(
        OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT14), OCIO::Exception, "not supported");

    OCIO_CHECK_THROW_WHAT(
        OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT32), OCIO::Exception, "not supported");

    OCIO_CHECK_THROW_WHAT(
        OCIO::IsFloatBitDepth((OCIO::BitDepth)42), OCIO::Exception, "not supported");
}
