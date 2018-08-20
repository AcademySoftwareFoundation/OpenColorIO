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

#include <OpenColorIO/OpenColorIO.h>

#include "half.h"

#include <cmath>
#include <vector>

#include "Op.h"
#include "Platform.h"

#ifdef WINDOWS
#include <float.h>
#endif

OCIO_NAMESPACE_ENTER
{
    // From Imath
    //--------------------------------------------------------------------------
    // Compare two numbers and test if they are "approximately equal":
    //
    // equalWithAbsError (x1, x2, e)
    //
    //	Returns true if x1 is the same as x2 with an absolute error of
    //	no more than e,
    //	
    //	abs (x1 - x2) <= e
    //
    // equalWithRelError (x1, x2, e)
    //
    //	Returns true if x1 is the same as x2 with an relative error of
    //	no more than e,
    //	
    //	abs (x1 - x2) <= e * x1
    //
    //--------------------------------------------------------------------------
    
    template<typename T>
    inline bool equalWithAbsError (T x1, T x2, T e)
    {
        return ((x1 > x2)? x1 - x2: x2 - x1) <= e;
    }
    
    template<typename T>
    inline bool equalWithRelError (T x1, T x2, T e)
    {
        return ((x1 > x2)? x1 - x2: x2 - x1) <= e * ((x1 > 0)? x1: -x1);
    }
    
    // Relative comparison: check if the difference between value and expected
    // relative to (divided by) expected does not exceed the eps.  A minimum
    // expected value is used to limit the scaling of the difference and
    // avoid large relative differences for small numbers.
    template<typename T>
    inline bool equalWithSafeRelError(T value,
                                      T expected,
                                      T eps,
                                      T minExpected)
    {
        const float div = (expected > 0) ?
            ((expected < minExpected) ? minExpected : expected) :
            ((-expected < minExpected) ? minExpected : -expected);

        return (
            ((value > expected) ? value - expected : expected - value)
            / div) <= eps;
    }


    inline float lerpf(float a, float b, float z)
    {
        return (b - a) * z + a;
    }
    
#ifdef WINDOWS
    inline int isnan (float val)
    {
        // Windows uses a non-standard version of 'isnan'
        return _isnan (val);
    }
    inline int isnan(double val)
    {
        // Windows uses a non-standard version of 'isnan'
        return _isnan(val);
    }
#else

#ifdef ANDROID
// support std::isnan - needs to be tested as it might not be part of the NDK
#define _GLIBCXX_USE_C99_MATH 1
#endif

    // This lets all platforms just use isnan, within the OCIO namespace,
    // across all platforms. (Windows defines the function above).
    using std::isnan;
#endif
    
    // Clamp value a to[min, max]
    // First compare with max, then with min.
    // Does not validate max >= min
    template<typename T>
    inline T clamp(T a, T min, T max)
    {
        return ((a) > (max) ? (max) : ((min) > (a) ? (min) : (a)));
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
    bool IsScalarEqualToZero(float v);
    bool IsScalarEqualToOne(float v);
    bool IsScalarEqualToZeroFlt(double v);
    bool IsScalarEqualToOneFlt(double v);

    // Are all the vector components the specified value?
    bool IsVecEqualToZero(const float* v, int size);
    bool IsVecEqualToOne(const float* v, int size);
    bool IsVecEqualToOneFlt(const double* v, int size);

    // Is at least one of the specified components equal to 0?
    bool VecContainsZero(const float* v, int size);
    bool VecContainsOne(const float* v, int size);
    
    // Are two vectors equal? (Same size, same values?)
    bool VecsEqualWithRelError(const float* v1, int size1,
                               const float* v2, int size2,
                               float e);
    
    // Are two vectors equal? (Same size, same values?)
    bool VecsEqualWithRelError(const double* v1, int size1,
                               const double* v2, int size2,
                               double e);

    // Clamp the specified value to the valid range of normalized half.
    // (can be either positive or negative though
    double ClampToNormHalf(double val);
    
    // Convert an half representation to the corresponding float
    float ConvertHalfBitsToFloat(unsigned short val);

    // Reinterpret the binary representation of a single-precision floating-point 
    // number as a 32-bit integer.
    // Returns reinterpreted float bit representation as an integer 
    inline unsigned FloatAsInt(const float x)
    {
        union
        {
            float f;
            unsigned i;
        } v;

        v.f = x;
        return v.i;
    }

    // Reinterpret the binary representation of a 32-bit integer as a
    // single-precision floating-point number.
    // Returns reinterpreted integer bit representation as a float
    inline float IntAsFloat(const unsigned x)
    {
        union
        {
            float f;
            unsigned i;
        } v;

        v.i = x;
        return v.f;
    }

    // Add a number of ULPs (Unit of Least Precision) to a given
    // floating-point number.
    // - f original floating-point number
    // - ulp the number of ULPs to be added to the floating-point number
    // Returns the original floating-point number added by the number of ULPs.
    inline float AddULP(const float f, const int ulp)
    {
        return IntAsFloat(FloatAsInt(f) + ulp);
    }
    
    // Clamp the specified value to the valid range of normalized half.
    // (can be either positive or negative though
    double ClampToNormHalf(double val);
    
    // Verify if two floating-point numbers are within a tolerance given in
    // ULPs. Special values like NaN, Inf and -Inf are checked for equivalence.
    // When the compressDenorms flag is true, denormalized floating-point
    // numbers are interpreted as being equivalent to zero for comparison
    // purposes. This is a form of relative comparison where 1 ULP is
    // equivalent to 2^(exponent - 23) for normalized values and to
    // 2^(-151) for denormalized numbers.
    // - expected reference floating-point value
    // - actual floating-point value to be compared to the reference
    // - tolerance threshold (inclusive) for comparison,
    //             given in ULP (unit of Least Precision)
    // - compressDenorms flag indicating whether or not denormalized values are
    //                   going to be interpreted as zero
    // Return true if the floating-point number are different, that is, their
    //        difference is not within the acceptable tolerance, under the
    //        conditions imposed by the compressDenorms flag.
    bool FloatsDiffer(float expected,
                      float actual,
                      int tolerance,
                      bool compressDenorms);

    // Compares halfs as raw integers for equality within tol.
    // If aimHalf is a NaN, valHalf must also be one of the NaNs.
    // Inf is treated like any other value (diff from HALFMAX is 1).
    // - aimHalf reference value
    // - valHalf value being tested
    // - tol the difference between raw half ints must be <= tol
    bool HalfsDiffer(half aimHalf, half valHalf, int tol);

}
OCIO_NAMESPACE_EXIT

#endif
