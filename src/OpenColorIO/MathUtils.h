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


#ifndef INCLUDED_OCIO_MATHUTILS_H
#define INCLUDED_OCIO_MATHUTILS_H


#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ilmbase/half.h"


OCIO_NAMESPACE_ENTER
{

template<typename T>
bool IsNan(T val) { return std::isnan(val); }


// From Imath
//--------------------------------------------------------------------------
// Compare two numbers and test if they are "approximately equal":
//
// EqualWithAbsError (x1, x2, e)
//
//  Returns true if x1 is the same as x2 with an absolute error of
//  no more than e,
//  
//  abs (x1 - x2) <= e
//
// EqualWithRelError (x1, x2, e)
//
//  Returns true if x1 is the same as x2 with an relative error of
//  no more than e,
//  
//  abs (x1 - x2) <= e * x1
//
//--------------------------------------------------------------------------
    
template<typename T>
inline bool EqualWithAbsError (T x1, T x2, T e)
{
    return ((x1 > x2)? x1 - x2: x2 - x1) <= e;
}

template<typename T>
inline bool EqualWithRelError (T x1, T x2, T e)
{
    return ((x1 > x2)? x1 - x2: x2 - x1) <= e * ((x1 > 0)? x1: -x1);
}

#ifdef OCIO_UNIT_TEST
// Relative comparison: check if the difference between value and expected
// relative to (divided by) expected does not exceed the eps.  A minimum
// expected value is used to limit the scaling of the difference and
// avoid large relative differences for small numbers.
template<typename T>
inline bool EqualWithSafeRelError(T value,
                                  T expected,
                                  T eps,
                                  T minExpected)
{
    // If value and expected are infinity, return true.
    if (value == expected) return true;
    if (IsNan(value) && IsNan(expected)) return true;
    const float div = (expected > 0) ?
        ((expected < minExpected) ? minExpected : expected) :
        ((-expected < minExpected) ? minExpected : -expected);

    return (
        ((value > expected) ? value - expected : expected - value)
        / div) <= eps;
}
#endif

inline float lerpf(float a, float b, float z)
{
    return (b - a) * z + a;
}
    
    
// Clamp value a to[min, max]
// First compare with max, then with min.
// 
// Note: Does not validate max >= min.
// Note: NaN values become 0.
template<typename T>
inline T Clamp(T a, T min, T max)
{
    return std::min(std::max(min, a), max);
}

// Remove/map special float values to values inside the floating-point domain.
//        Especifically, maps:
//          -Inf to -MAX_FLOAT
//           Inf to  MAX_FLOAT
//           NaN to  0
// - f is the float to sanitize
// Return the sanitized float
float SanitizeFloat(float f);

// Checks within fltmin tolerance
template<typename T>
bool IsScalarEqualToZero(T v);
template<typename T>
bool IsScalarEqualToOne(T v);

// Are all the vector components the specified value?
template<typename T>
bool IsVecEqualToZero(const T * v, unsigned int size);
template<typename T>
bool IsVecEqualToOne(const T * v, unsigned int size);

// Is at least one of the specified components equal to 0?
bool VecContainsZero(const float* v, int size);
bool VecContainsOne(const float* v, int size);

// Are two vectors equal? (Same size, same values?)
template<typename T>
bool VecsEqualWithRelError(const T * v1, unsigned int size1,
                           const T * v2, unsigned int size2,
                           T e);

inline double GetHalfMax()
{
    return 65504.0;         // Largest positive half
}

inline double GetHalfMin()
{
    return 5.96046448e-08;  // Smallest positive half
}

inline double GetHalfNormMin()
{
    return 6.10351562e-05;  // Smallest positive normalized half
}

// Clamp the specified value to the valid range of normalized half.
// (can be either positive or negative though).
double ClampToNormHalf(double val);

// Convert an half representation to the corresponding float.
float ConvertHalfBitsToFloat(unsigned short val);

float GetSafeScalarInverse(float v, float defaultValue = 1.0);


// All matrix / vector operations use the following sizing...
//
// m : 4x4 matrix
// v : 4 column vector

// Return the 4x4 inverse, and whether the inverse has succeeded.
// Supports in-place operations.
bool GetM44Inverse(float* mout, const float* m);

// Is an identity matrix? (with fltmin tolerance)
template<typename T>
bool IsM44Identity(const T * m);

// Is this a purely diagonal matrix?
bool IsM44Diagonal(const float* m);

// Extract the diagonal
void GetM44Diagonal(float* vout, const float* m);

// Combine two transforms in the mx+b form, into a single transform.
// mout*x+vout == m2*(m1*x+v1)+v2
// Supports in-place operations.
void GetMxbCombine(float* mout, float* vout,
                   const float* m1, const float* v1,
                   const float* m2, const float* v2);

// Supports in-place operations
bool GetMxbInverse(float* mout, float* vout,
                   const float* m, const float* v);

// Reinterpret the binary representation of a single-precision floating-point number
//   as a 32-bit integer.
//
// x : floating-point number
//
// Return reinterpreted float bit representation as an integer.
inline unsigned FloatAsInt(const float x)
{
    union {
        float f;
        unsigned i;
    } v;

    v.f = x;
    return v.i;
}

// Reinterpret the binary representation of a 32-bit integer as a
//   single-precision floating-point number.
//
// x : integer number
//
// Return reinterpreted integer bit representation as a float.
inline float IntAsFloat(const unsigned x)
{
    union {
        float f;
        unsigned i;
    } v;

    v.i = x;
    return v.f;
}

// Add a number of ULPs (Unit of Least Precision) to a given
//   floating-point number.
//
// f : original floating-point number
// ulp : the number of ULPs to be added to the floating-point number
//
// Return the original floating-point number added by the number of ULPs.
inline float AddULP(const float f, const int ulp)
{
    return IntAsFloat( FloatAsInt(f) + ulp );
}

// Verify if two floating-point numbers are within a tolerance given in ULPs.
//   Special values like NaN, Inf and -Inf are checked for equivalence. When the
//   compressDenorms flag is true, denormalized floating-point numbers are interpreted
//   as being equivalent to zero for comparison purposes. This is a form of relative
//   comparison where 1 ULP is equivalent to 2^(exponent - 23) for normalized values
//   and to 2^(-149) for denormalized numbers.
//   One ULP on the domain [1.0, 2.0] is 2^(0-23)=1.19e-7. A correctly rounded float value 
//   is always within 0.5 ULPs of the exact value.
//
// expected : reference floating-point value
// actual   : floating-point value to be compared to the reference
// tolerance: threshold (inclusive) for comparison, given in ULP (unit of Least Precision)
// compressDenorms: flag indicating whether or not denormalized values are going to be
//                        interpreted as zero
//
// Return true if the floating-point number are different, that is, their difference is not
//              within the acceptable tolerance, under the conditions imposed by the
//              compressDenorms flag.
bool FloatsDiffer(const float expected, const float actual, 
                  const int tolerance, const bool compressDenorms);

// Compares half-floats as raw integers with a tolerance (essentially in ULPs).
// Returns true if the integer difference is strictly greater than the tolerance.
// If aimHalf is a NaN, valHalf must also be one of the NaNs.
// Inf is treated like any other value (diff from HALFMAX is 1).
bool HalfsDiffer(const half expected, const half actual, const int tolerance);

}
OCIO_NAMESPACE_EXIT

#endif
