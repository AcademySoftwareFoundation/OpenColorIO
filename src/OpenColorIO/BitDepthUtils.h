// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_BIT_DEPTH_UTILS_H
#define INCLUDED_OCIO_BIT_DEPTH_UTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include "OpenEXR/half.h"


namespace OCIO_NAMESPACE
{

// Returns a double since often times a ratio of calls to this function is used
// to calculate a scale factor and this ratio needs to be done at double
// precision to avoid slight errors in the scaled values.
double GetBitDepthMaxValue(BitDepth in);

// True if the bit depth is a float.
bool IsFloatBitDepth(BitDepth in);

// Return the size in bytes of one channel.
unsigned GetChannelSizeInBytes(BitDepth in);

// Metaprogramming requires templated structures to access
// some bit depth information at compile time.

// Incomplete structure to have a compile time build break for unsupported bit depths.
template<BitDepth BD>
struct BitDepthInfo {};

template<> struct BitDepthInfo<BIT_DEPTH_UINT8>
{
    typedef uint8_t Type;
    static const bool isFloat = false;
    static const unsigned maxValue = 255;
};

template<> struct BitDepthInfo<BIT_DEPTH_UINT10>
{
    typedef uint16_t Type;
    static const bool isFloat = false;
    static const unsigned maxValue = 1023;
};

template<> struct BitDepthInfo<BIT_DEPTH_UINT12>
{
    typedef uint16_t Type;
    static const bool isFloat = false;
    static const unsigned maxValue = 4095;
};

template<> struct BitDepthInfo<BIT_DEPTH_UINT16>
{
    typedef uint16_t Type;
    static const bool isFloat = false;
    static const unsigned maxValue = 65535;
};

template<> struct BitDepthInfo<BIT_DEPTH_F16>
{
    typedef half Type;
    static const bool isFloat = true;
    static const unsigned maxValue = 1;
};

template<> struct BitDepthInfo<BIT_DEPTH_F32>
{
    typedef float Type;
    static const bool isFloat = true;
    static const unsigned maxValue = 1;
};

// Infer the bit-depth scaling based on the largest value contained in the LUT.
BitDepth GetBitdepthFromMaxValue(unsigned maxValue);

// Clamp helper method.
#define CLAMP(a, min, max) \
  ((a)>(max) ? (max) : ((min)>(a) ? (min) : (a)))


// Converting from float to any integer type require to first correctly round
// the float value before casting.

// Incomplete structure to have a compile time build break for unsupported bit depths.
template<BitDepth bd> struct Converter { };

template<>
struct Converter<BIT_DEPTH_UINT8>
{
    typedef typename BitDepthInfo<BIT_DEPTH_UINT8>::Type Type;

    static Type CastValue(float value)
    {
        // Compute once here instead of several times in the macro.
        const float v = value + 0.5f;
        return (Type)CLAMP(v, 0.0f, BitDepthInfo<BIT_DEPTH_UINT8>::maxValue);
    }
};

template<>
struct Converter<BIT_DEPTH_UINT10>
{
    typedef typename BitDepthInfo<BIT_DEPTH_UINT10>::Type Type;

    static Type CastValue(float value)
    {
        // Compute once here instead of several times in the macro.
        const float v = value + 0.5f;
        return (Type)CLAMP(v, 0.0f, BitDepthInfo<BIT_DEPTH_UINT10>::maxValue);
    }
};

template<>
struct Converter<BIT_DEPTH_UINT12>
{
    typedef typename BitDepthInfo<BIT_DEPTH_UINT12>::Type Type;

    static Type CastValue(float value)
    {
        // Compute once here instead of several times in the macro.
        const float v = value + 0.5f;
        return (Type)CLAMP(v, 0.0f, BitDepthInfo<BIT_DEPTH_UINT12>::maxValue);
    }
};

template<>
struct Converter<BIT_DEPTH_UINT16>
{
    typedef typename BitDepthInfo<BIT_DEPTH_UINT16>::Type Type;

    static Type CastValue(float value)
    {
        // Compute once here instead of several times in the macro.
        const float v = value + 0.5f;
        return (Type)CLAMP(v, 0.0f, BitDepthInfo<BIT_DEPTH_UINT16>::maxValue);
    }
};

template<>
struct Converter<BIT_DEPTH_F16>
{
    typedef typename BitDepthInfo<BIT_DEPTH_F16>::Type Type;

    static Type CastValue(float value)
    { 
        return Type(value);
    }
};

template<>
struct Converter<BIT_DEPTH_F32>
{
    typedef typename BitDepthInfo<BIT_DEPTH_F32>::Type Type;

    static Type CastValue(float value)
    { 
        return Type(value);
    }
};


} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_BIT_DEPTH_UTILS_H
