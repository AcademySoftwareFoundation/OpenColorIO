// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"

namespace
{

static const std::string errBDNotSupported("Bit depth is not supported: ");

};

OCIO_NAMESPACE_ENTER
{
    double GetBitDepthMaxValue(BitDepth in)
    {
        switch(in)
        {
            case BIT_DEPTH_UINT8:
                return (double)BitDepthInfo<BIT_DEPTH_UINT8>::maxValue;
            case BIT_DEPTH_UINT10:
                return (double)BitDepthInfo<BIT_DEPTH_UINT10>::maxValue;
            case BIT_DEPTH_UINT12:
                return (double)BitDepthInfo<BIT_DEPTH_UINT12>::maxValue;
            case BIT_DEPTH_UINT16:
                return (double)BitDepthInfo<BIT_DEPTH_UINT16>::maxValue;
            case BIT_DEPTH_F16:
                return (double)BitDepthInfo<BIT_DEPTH_F16>::maxValue;
            case BIT_DEPTH_F32:
                return (double)BitDepthInfo<BIT_DEPTH_F32>::maxValue;

            case BIT_DEPTH_UNKNOWN:
            case BIT_DEPTH_UINT14:
            case BIT_DEPTH_UINT32:
            default:
            {
                std::string err(errBDNotSupported);
                err += BitDepthToString(in);
                throw Exception(err.c_str());
            }
        }
    }


    bool IsFloatBitDepth(BitDepth in)
    {
        switch(in)
        {
            case BIT_DEPTH_UINT8:
                return BitDepthInfo<BIT_DEPTH_UINT8>::isFloat;
            case BIT_DEPTH_UINT10:
                return BitDepthInfo<BIT_DEPTH_UINT10>::isFloat;
            case BIT_DEPTH_UINT12:
                return BitDepthInfo<BIT_DEPTH_UINT12>::isFloat;
            case BIT_DEPTH_UINT16:
                return BitDepthInfo<BIT_DEPTH_UINT16>::isFloat;
            case BIT_DEPTH_F16:
                return BitDepthInfo<BIT_DEPTH_F16>::isFloat;
            case BIT_DEPTH_F32:
                return BitDepthInfo<BIT_DEPTH_F32>::isFloat;

            case BIT_DEPTH_UNKNOWN:
            case BIT_DEPTH_UINT14:
            case BIT_DEPTH_UINT32:
            default:
            {
                std::string err(errBDNotSupported);
                err += BitDepthToString(in);
                throw Exception(err.c_str());
            }
        }
    }
}
OCIO_NAMESPACE_EXIT



///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "UnitTest.h"

OCIO_ADD_TEST(BitDepthUtils, GetBitDepthMaxValue)
{
    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT8), 255.0);
    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT16), 65535.0);

    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F16), 1.0);
    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32), 1.0);

    OCIO_CHECK_THROW_WHAT(
        OCIO::GetBitDepthMaxValue((OCIO::BitDepth)42), OCIO::Exception, "not supported");
}

OCIO_ADD_TEST(BitDepthUtils, IsFloatBitDepth)
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

#endif