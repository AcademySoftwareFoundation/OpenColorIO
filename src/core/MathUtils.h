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
    
    inline bool equalWithAbsError (float x1, float x2, float e)
    {
        return ((x1 > x2)? x1 - x2: x2 - x1) <= e;
    }
    
    inline bool equalWithRelError (float x1, float x2, float e)
    {
        return ((x1 > x2)? x1 - x2: x2 - x1) <= e * ((x1 > 0)? x1: -x1);
    }
    
    inline float lerpf(float a, float b, float z)
    {
        return (b - a) * z + a;
    }
    
#ifdef WINDOWS
    inline double
    round (float val) {
        return floor (val + 0.5);
    }

    inline float
    roundf (float val) {
        return static_cast<float>(round (val));
    }

    inline int
    isnan (float val) {
        // Windows uses a non-standard version of 'isnan'
        return _isnan (val);
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
    
    inline double GetHalfMax()
    {
        return 65504.0;         // Largest positive half
    }
    
    inline double GetHalfMin()
    {
        return 5.96046448e-08;  // Smallest positive half;
    }
    
    inline double GetHalfNormMin()
    {
        return 6.10351562e-05;  // Smallest positive normalized half
    }
    
    //! Clamp the specified value to the valid range of normalized half.
    // (can be either positive or negative though
    
    double ClampToNormHalf(double val);
    
    float GetSafeScalarInverse(float v, float defaultValue = 1.0);
    
    
    // All matrix / vector operations use the following sizing...
    //
    // m : 4x4 matrix
    // v : 4 column vector
    
    // Return the 4x4 inverse, and whether the inverse has succeeded.
    // Supports in-place operations
    bool GetM44Inverse(float* mout, const float* m);
    
    // Is an identity matrix? (with fltmin tolerance)
    bool IsM44Identity(const float* m);
    
    // Is this a purely diagonal matrix?
    bool IsM44Diagonal(const float* m);
    
    // Extract the diagonal
    void GetM44Diagonal(float* vout, const float* m);
    
    // Get the product, out = m1*m2
    // Supports in-place operations
    void GetM44Product(float* mout, const float* m1, const float* m2);
    
    // Combine two transforms in the mx+b form, into a single transform
    // mout*x+vout == m2*(m1*x+v1)+v2
    // Supports in-place operations
    void GetMxbCombine(float* mout, float* vout,
                       const float* m1, const float* v1,
                       const float* m2, const float* v2);
    
    // Supports in-place operations
    bool GetMxbInverse(float* mout, float* vout,
                       const float* m, const float* v);

}
OCIO_NAMESPACE_EXIT

#endif
