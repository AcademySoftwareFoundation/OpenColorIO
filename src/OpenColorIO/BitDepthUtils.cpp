// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"

namespace
{

constexpr char errBDNotSupported[] = "Bit depth is not supported: ";

};

namespace OCIO_NAMESPACE
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
            err += ".";
            throw Exception(err.c_str());
        }
    }
}

namespace
{

template<BitDepth A, BitDepth B>
constexpr unsigned MiddleMaxValue()
{
    return (BitDepthInfo<A>::maxValue + BitDepthInfo<B>::maxValue) / 2;
}

}

// For formats that do not explicitly identify the intended bit-depth scaling,
// we must infer it based on the LUT values. However LUTs sometimes contain
// values that extend outside the nominal ranges. For example, a LUT that
// started out in a floating point format with values going up to 1.09 may get
// converted to another format that uses 10-bit values and those extend up to
// 1.09 * 1023 = 1115. In this case we want the "auto detection" to return
// 10-bit rather than 12-bit. Hence rather than using breakpoints of 1024,
// 2048, 4096, etc., we use breakpoints that are midway between in order to
// better handle LUTs with occasional over-range values.
BitDepth GetBitdepthFromMaxValue(unsigned maxValue)
{
    if (maxValue < MiddleMaxValue<BIT_DEPTH_F32, BIT_DEPTH_UINT8>()) // 128
    {
        return BIT_DEPTH_F32;
    }
    else if (maxValue < MiddleMaxValue<BIT_DEPTH_UINT8, BIT_DEPTH_UINT10>()) // 639
    {
        return BIT_DEPTH_UINT8;
    }
    else if (maxValue < MiddleMaxValue<BIT_DEPTH_UINT10, BIT_DEPTH_UINT12>()) // 2559
    {
        return BIT_DEPTH_UINT10;
    }
    else if (maxValue < MiddleMaxValue<BIT_DEPTH_UINT12, BIT_DEPTH_UINT16>()) // 34815
    {
        return BIT_DEPTH_UINT12;
    }
    return BIT_DEPTH_UINT16;
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
            err += ".";
            throw Exception(err.c_str());
        }
    }
}


unsigned GetChannelSizeInBytes(BitDepth in)
{
    switch(in)
    {
        case BIT_DEPTH_UINT8:
            return sizeof(BitDepthInfo<BIT_DEPTH_UINT8>::Type);
        case BIT_DEPTH_UINT10:
            return sizeof(BitDepthInfo<BIT_DEPTH_UINT10>::Type);
        case BIT_DEPTH_UINT12:
            return sizeof(BitDepthInfo<BIT_DEPTH_UINT12>::Type);
        case BIT_DEPTH_UINT16:
            return sizeof(BitDepthInfo<BIT_DEPTH_UINT16>::Type);
        case BIT_DEPTH_F16:
            return sizeof(BitDepthInfo<BIT_DEPTH_F16>::Type);
        case BIT_DEPTH_F32:
            return sizeof(BitDepthInfo<BIT_DEPTH_F32>::Type);
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
        {
            std::string err(errBDNotSupported);
            err += BitDepthToString(in);
            err += ".";
            throw Exception(err.c_str());
        }
    }
}

} // namespace OCIO_NAMESPACE

