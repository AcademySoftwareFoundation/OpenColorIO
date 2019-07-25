/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef INCLUDED_OCIO_BIT_DEPTH_UTILS_H
#define INCLUDED_OCIO_BIT_DEPTH_UTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include "ilmbase/half.h"


OCIO_NAMESPACE_ENTER
{

// This is used for normalizing various integer bit-depths relative to float.
float GetBitDepthMaxValue(BitDepth in);

// True if the bit depth is a float
bool IsFloatBitDepth(BitDepth in);


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


// Clamp helper method
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


}
OCIO_NAMESPACE_EXIT

#endif // INCLUDED_OCIO_BIT_DEPTH_UTILS_H
