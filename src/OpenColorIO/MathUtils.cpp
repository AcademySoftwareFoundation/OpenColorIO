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

#include <sstream>
#include <string.h>
#include <type_traits>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"


OCIO_NAMESPACE_ENTER
{

template<typename T>
bool IsScalarEqualToZero(T v)
{
    static_assert(std::is_floating_point<T>::value, 
                  "Only single and double precision floats are supported");

    return !FloatsDiffer(0.0f, (float)v, 2, false);
}

template bool IsScalarEqualToZero(float v);
template bool IsScalarEqualToZero(double v);

template<typename T>
bool IsScalarEqualToOne(T v)
{
    static_assert(std::is_floating_point<T>::value, 
                  "Only single and double precision floats are supported");

    return !FloatsDiffer(1.0f, (float)v, 2, false);
}

template bool IsScalarEqualToOne(float v);
template bool IsScalarEqualToOne(double v);

template<typename T>
bool IsVecEqualToZero(const T * v, unsigned int size)
{
    static_assert(std::is_floating_point<T>::value, 
                  "Only single and double precision floats are supported");

    for(unsigned int i=0; i<size; ++i)
    {
        if(!IsScalarEqualToZero(v[i])) return false;
    }
    return true;
}

template bool IsVecEqualToZero(const float * v, unsigned int size);
template bool IsVecEqualToZero(const double * v, unsigned int size);

template<typename T>
bool IsVecEqualToOne(const T * v, unsigned int size)
{
    static_assert(std::is_floating_point<T>::value, 
                  "Only single and double precision floats are supported");

    for(unsigned int i=0; i<size; ++i)
    {
        if(!IsScalarEqualToOne(v[i])) return false;
    }
    return true;
}

template bool IsVecEqualToOne(const float * v, unsigned int size);
template bool IsVecEqualToOne(const double * v, unsigned int size);

template<typename T>
bool VecsEqualWithRelError(const T * v1, unsigned int size1,
                           const T * v2, unsigned int size2,
                           T e)
{
    static_assert(std::is_floating_point<T>::value, 
                  "Only single and double precision floats are supported");

    if (size1 != size2) return false;

    for (unsigned int i = 0; i<size1; ++i)
    {
        if (!EqualWithRelError(v1[i], v2[i], e)) return false;
    }

    return true;
}

template bool VecsEqualWithRelError(const float * v1, unsigned int size1, const float * v2, unsigned int size2, float e);
template bool VecsEqualWithRelError(const double * v1, unsigned int size1, const double * v2, unsigned int size2, double e);

float GetSafeScalarInverse(float v, float defaultValue)
{
    if(IsScalarEqualToZero(v)) return defaultValue;
    return 1.0f / v;
}

bool VecContainsZero(const float* v, int size)
{
    for(int i=0; i<size; ++i)
    {
        if(IsScalarEqualToZero(v[i])) return true;
    }
    return false;
}

bool VecContainsOne(const float* v, int size)
{
    for(int i=0; i<size; ++i)
    {
        if(IsScalarEqualToOne(v[i])) return true;
    }
    return false;
}

double ClampToNormHalf(double val)
{
    if(val < -GetHalfMax())
    {
        return -GetHalfMax();
    }

    if(val > -GetHalfNormMin() && val<GetHalfNormMin())
    {
        return 0.0;
    }

    if(val > GetHalfMax())
    {
        return GetHalfMax();
    }

    return val;
}

float ConvertHalfBitsToFloat(unsigned short val)
{
    half hVal;
    hVal.setBits(val);
    return (float)hVal;
}

float SanitizeFloat(float f)
{
    if (f == -std::numeric_limits<float>::infinity())
    {
        return -std::numeric_limits<float>::max();
    }
    else if (f == std::numeric_limits<float>::infinity())
    {
        return std::numeric_limits<float>::max();
    }
    else if (IsNan(f))
    {
        return 0.0f;
    }
    return f;
}

template<typename T>
bool IsM44Identity(const T * m44)
{
    static_assert(std::is_floating_point<T>::value, 
                  "Only single and double precision floats are supported");

    unsigned int index=0;

    for(unsigned int j=0; j<4; ++j)
    {
        for(unsigned int i=0; i<4; ++i)
        {
            index = 4*j+i;

            if(i==j)
            {
                if(!IsScalarEqualToOne(m44[index])) return false;
            }
            else
            {
                if(!IsScalarEqualToZero(m44[index])) return false;
            }
        }
    }
    
    return true;
}

template bool IsM44Identity(const float * m44);
template bool IsM44Identity(const double * m44);


bool IsM44Diagonal(const float* m44)
{
    for(int i=0; i<16; ++i)
    {
        if((i%5)==0) continue; // If we're on the diagonal, skip it
        if(!IsScalarEqualToZero(m44[i])) return false;
    }
    
    return true;
}

void GetM44Diagonal(float* out4, const float* m44)
{
    for(int i=0; i<4; ++i)
    {
        out4[i] = m44[i*5];
    }
}

// We use an intermediate double representation to make sure
// there is minimal float precision error on the determinant's computation
// (We have seen IsScalarEqualToZero sensitivities here on 32-bit
// virtual machines)

bool GetM44Inverse(float* inverse_out, const float* m_)
{
    double m[16];
    for(unsigned int i=0; i<16; ++i) m[i] = (double)m_[i];
    
    double d10_21 = m[4]*m[9] - m[5]*m[8];
    double d10_22 = m[4]*m[10] - m[6]*m[8];
    double d10_23 = m[4]*m[11] - m[7]*m[8];
    double d11_22 = m[5]*m[10] - m[6]*m[9];
    double d11_23 = m[5]*m[11] - m[7]*m[9];
    double d12_23 = m[6]*m[11] - m[7]*m[10];
    
    double a00 = m[13]*d12_23 - m[14]*d11_23 + m[15]*d11_22;
    double a10 = m[14]*d10_23 - m[15]*d10_22 - m[12]*d12_23;
    double a20 = m[12]*d11_23 - m[13]*d10_23 + m[15]*d10_21;
    double a30 = m[13]*d10_22 - m[14]*d10_21 - m[12]*d11_22;
    
    double det = a00*m[0] + a10*m[1] + a20*m[2] + a30*m[3];
    
    if(IsScalarEqualToZero((float)det)) return false;
    
    det = 1.0/det;
    
    double d00_31 = m[0]*m[13] - m[1]*m[12];
    double d00_32 = m[0]*m[14] - m[2]*m[12];
    double d00_33 = m[0]*m[15] - m[3]*m[12];
    double d01_32 = m[1]*m[14] - m[2]*m[13];
    double d01_33 = m[1]*m[15] - m[3]*m[13];
    double d02_33 = m[2]*m[15] - m[3]*m[14];
    
    double a01 = m[9]*d02_33 - m[10]*d01_33 + m[11]*d01_32;
    double a11 = m[10]*d00_33 - m[11]*d00_32 - m[8]*d02_33;
    double a21 = m[8]*d01_33 - m[9]*d00_33 + m[11]*d00_31;
    double a31 = m[9]*d00_32 - m[10]*d00_31 - m[8]*d01_32;
    
    double a02 = m[6]*d01_33 - m[7]*d01_32 - m[5]*d02_33;
    double a12 = m[4]*d02_33 - m[6]*d00_33 + m[7]*d00_32;
    double a22 = m[5]*d00_33 - m[7]*d00_31 - m[4]*d01_33;
    double a32 = m[4]*d01_32 - m[5]*d00_32 + m[6]*d00_31;
    
    double a03 = m[2]*d11_23 - m[3]*d11_22 - m[1]*d12_23;
    double a13 = m[0]*d12_23 - m[2]*d10_23 + m[3]*d10_22;
    double a23 = m[1]*d10_23 - m[3]*d10_21 - m[0]*d11_23;
    double a33 = m[0]*d11_22 - m[1]*d10_22 + m[2]*d10_21;
    
    inverse_out[0] = (float) (a00*det);
    inverse_out[1] = (float) (a01*det);
    inverse_out[2] = (float) (a02*det);
    inverse_out[3] = (float) (a03*det);
    inverse_out[4] = (float) (a10*det);
    inverse_out[5] = (float) (a11*det);
    inverse_out[6] = (float) (a12*det);
    inverse_out[7] = (float) (a13*det);
    inverse_out[8] = (float) (a20*det);
    inverse_out[9] = (float) (a21*det);
    inverse_out[10] = (float) (a22*det);
    inverse_out[11] = (float) (a23*det);
    inverse_out[12] = (float) (a30*det);
    inverse_out[13] = (float) (a31*det);
    inverse_out[14] = (float) (a32*det);
    inverse_out[15] = (float) (a33*det);
    
    return true;
}

void GetM44M44Product(float* mout, const float* m1_, const float* m2_)
{
    float m1[16];
    float m2[16];
    memcpy(m1, m1_, 16*sizeof(float));
    memcpy(m2, m2_, 16*sizeof(float));
    
    mout[ 0] = m1[ 0]*m2[0] + m1[ 1]*m2[4] + m1[ 2]*m2[ 8] + m1[ 3]*m2[12];
    mout[ 1] = m1[ 0]*m2[1] + m1[ 1]*m2[5] + m1[ 2]*m2[ 9] + m1[ 3]*m2[13];
    mout[ 2] = m1[ 0]*m2[2] + m1[ 1]*m2[6] + m1[ 2]*m2[10] + m1[ 3]*m2[14];
    mout[ 3] = m1[ 0]*m2[3] + m1[ 1]*m2[7] + m1[ 2]*m2[11] + m1[ 3]*m2[15];
    mout[ 4] = m1[ 4]*m2[0] + m1[ 5]*m2[4] + m1[ 6]*m2[ 8] + m1[ 7]*m2[12];
    mout[ 5] = m1[ 4]*m2[1] + m1[ 5]*m2[5] + m1[ 6]*m2[ 9] + m1[ 7]*m2[13];
    mout[ 6] = m1[ 4]*m2[2] + m1[ 5]*m2[6] + m1[ 6]*m2[10] + m1[ 7]*m2[14];
    mout[ 7] = m1[ 4]*m2[3] + m1[ 5]*m2[7] + m1[ 6]*m2[11] + m1[ 7]*m2[15];   
    mout[ 8] = m1[ 8]*m2[0] + m1[ 9]*m2[4] + m1[10]*m2[ 8] + m1[11]*m2[12];
    mout[ 9] = m1[ 8]*m2[1] + m1[ 9]*m2[5] + m1[10]*m2[ 9] + m1[11]*m2[13];
    mout[10] = m1[ 8]*m2[2] + m1[ 9]*m2[6] + m1[10]*m2[10] + m1[11]*m2[14];
    mout[11] = m1[ 8]*m2[3] + m1[ 9]*m2[7] + m1[10]*m2[11] + m1[11]*m2[15];
    mout[12] = m1[12]*m2[0] + m1[13]*m2[4] + m1[14]*m2[ 8] + m1[15]*m2[12];
    mout[13] = m1[12]*m2[1] + m1[13]*m2[5] + m1[14]*m2[ 9] + m1[15]*m2[13];
    mout[14] = m1[12]*m2[2] + m1[13]*m2[6] + m1[14]*m2[10] + m1[15]*m2[14];
    mout[15] = m1[12]*m2[3] + m1[13]*m2[7] + m1[14]*m2[11] + m1[15]*m2[15];
}

namespace
{

void GetM44V4Product(float* vout, const float* m, const float* v_)
{
    float v[4];
    memcpy(v, v_, 4*sizeof(float));
    
    vout[0] = m[ 0]*v[0] + m[ 1]*v[1] + m[ 2]*v[2] + m[ 3]*v[3];
    vout[1] = m[ 4]*v[0] + m[ 5]*v[1] + m[ 6]*v[2] + m[ 7]*v[3];
    vout[2] = m[ 8]*v[0] + m[ 9]*v[1] + m[10]*v[2] + m[11]*v[3];
    vout[3] = m[12]*v[0] + m[13]*v[1] + m[14]*v[2] + m[15]*v[3];
}

void GetV4Sum(float* vout, const float* v1, const float* v2)
{
    for(int i=0; i<4; ++i)
    {
        vout[i] = v1[i] + v2[i];
    }
}

} // anon namespace

// All m(s) are 4x4.  All v(s) are size 4 vectors.
// Return mout, vout, where mout*x+vout == m2*(m1*x+v1)+v2
// mout = m2*m1
// vout = m2*v1 + v2
void GetMxbCombine(float* mout, float* vout,
                   const float* m1_, const float* v1_,
                   const float* m2_, const float* v2_)
{
    float m1[16];
    float v1[4];
    float m2[16];
    float v2[4];
    memcpy(m1, m1_, 16*sizeof(float));
    memcpy(v1, v1_, 4*sizeof(float));
    memcpy(m2, m2_, 16*sizeof(float));
    memcpy(v2, v2_, 4*sizeof(float));
    
    GetM44M44Product(mout, m2, m1);
    GetM44V4Product(vout, m2, v1);
    GetV4Sum(vout, vout, v2);
}

bool GetMxbInverse(float* mout, float* vout,
                   const float* m_, const float* v_)
{
    float m[16];
    float v[4];
    memcpy(m, m_, 16*sizeof(float));
    memcpy(v, v_, 4*sizeof(float));

    if(!GetM44Inverse(mout, m)) return false;
    
    for(int i=0; i<4; ++i)
    {
        v[i] = -v[i];
    }
    GetM44V4Product(vout, mout, v);
    
    return true;
}

//------------------------------------------------------------------------------
//
//  Map a floating-point number (already represented as an integer) to an ordered
//  integer representation, allowing for a tolerance-based comparison.
//
//  Floating-point numbers have their magnitude stored in the bits 0-30 as a pair of
//  exponent and mantissa, while the sign is stored on bit 31. That makes the positive
//  floating-point representations to be in the range [0x00000000, 0x7FFFFFFF]
//  (including Infinity and NaNs) while the negative floating-point representations are
//  in the range [0x80000000, 0xFFFFFFFF]. That fact that the sign is independent from the
//  magnitude in the floating-point representation has the side-effect that, when the
//  interpreted as an integer, the floating-point representations are in reverse ordering.
//
//  In order to keep the set of floating-point representations ordered when interpreted as
//  integers, we need to shift the positive representations to 0x80000000, as well as flip
//  the negative representations and shift them to 0x00000000. As a last adjustment,
//  since we don't want to have distinct representations for zero and negative zero, we
//  discard negative zero and shift the negative representations to 0x00000001.
//  
//  As a reference, some interesting values and their corresponding mappings are:
//
// +--------------------------------------------+---------------------------+---------------------------+
// |               Value/Range                  |       Source domain       |       Mapped domain       |
// +--------------------------------------------+---------------------------+---------------------------+
// |  Negative NaN                              |  0xFF800001 - 0xFFFFFFFF  |  0x00000001 - 0x007FFFFF  |
// |  Negative infinity                         |  0xFF800000               |  0x00800000               |
// |  Negative floats [-MAX_FLOAT, -MIN_FLOAT]  |  0x80800000 - 0xFF7FFFFF  |  0x00800001 - 0x7F800000  |
// |  Negative denorms                          |  0x80000001 - 0x807FFFFF  |  0x7F800001 - 0x7FFFFFFF  |
// |  Negatve zero                              |  0x80000000               |  0x80000000               |
// |  Zero                                      |  0x00000000               |  0x80000000               |
// |  Positive denorms                          |  0x00000001 - 0x007FFFFF  |  0x80000001 - 0x807FFFFF  |
// |  Positive floats [MIN_FLOAT, MAX_FLOAT]    |  0x00800000 - 0x7F7FFFFF  |  0x80800000 - 0xFF7FFFFF  |
// |  Positive infinity                         |  0x7F800000               |  0xFF800000               |
// |  Positive NaN                              |  0x7F800001 - 0x7FFFFFFF  |  0xFF800001 - 0xFFFFFFFF  |
// +--------------------------------------------+---------------------------+---------------------------+
//
//  The distribution of the floating-point values over the ordered/mapped domain can be summarized as:
//
//  0x00000001  0x00800000        0x7F800000         0x80000000         0x80800000        0xFF800000  0xFFFFFFFF
//      |            |                 |                  |                  |                 |           |
//      +------------+-----------------+------------------+------------------+-----------------+-----------+
//      |    -NaN    | Negative floats | Negative denorms | Positive denorms | Positive floats |    NaN    |
//      +------------+-----------------+------------------+------------------+-----------------+-----------+
//
inline int FloatForCompare(const unsigned floatBits)
{
    return floatBits < 0x80000000 ? (0x80000000 + floatBits) : (0x80000000 - (floatBits & 0x7FFFFFFF));
}

//------------------------------------------------------------------------------
//
//  Map a floating-point number (already represented as an integer) to an ordered
//  integer representation, compressing the denormalized values. Denormalized values
//  are interpreted as being equivalent to zero over the mapped domain.
//
//  As a reference, some interesting values and their corresponding mappings are:
//
// +--------------------------------------------+---------------------------+---------------------------+
// |               Value/Range                  |       Source domain       |       Mapped domain       |
// +--------------------------------------------+---------------------------+---------------------------+
// |  Negative NaN                              |  0xFF800001 - 0xFFFFFFFF  |  0x00800000 - 0x00FFFFFE  |
// |  Negative infinity                         |  0xFF800000               |  0x00FFFFFF               |
// |  Negative floats [-MAX_FLOAT, -MIN_FLOAT]  |  0x80800000 - 0xFF7FFFFF  |  0x01000000 - 0x7FFFFFFF  |
// |  Negative denorms                          |  0x80000001 - 0x807FFFFF  |  0x80000000               |
// |  Negatve zero                              |  0x80000000               |  0x80000000               |
// |  Zero                                      |  0x00000000               |  0x80000000               |
// |  Positive denorms                          |  0x00000001 - 0x007FFFFF  |  0x80000000               |
// |  Positive floats [MIN_FLOAT, MAX_FLOAT]    |  0x00800000 - 0x7F7FFFFF  |  0x80000001 - 0xFF000000  |
// |  Positive infinity                         |  0x7F800000               |  0xFF000001               |
// |  Positive NaN                              |  0x7F800001 - 0x7FFFFFFF  |  0xFF000002 - 0xFF800000  |
// +--------------------------------------------+---------------------------+---------------------------+
//
//  The distribution of the floating-point values over the ordered/mapped domain can be summarized as:
//
//  0x00800000  0x00FFFFFF        0x80000000        0xFF000001  0xFF800000
//      |            |                 |                 |           |
//      +------------+-----------------+-----------------+-----------+
//      |    -NaN    | Negative floats | Positive floats |    NaN    |
//      +------------+-----------------+-----------------+-----------+
//
inline int FloatForCompareCompressDenorms(const unsigned floatBits)
{
    const int absi = (floatBits & 0x7FFFFFFF);
    if (absi < 0x00800000)
    {
        return 0x80000000;
    }
    else
    {
        return floatBits < 0x80000000 ? (0x7F800001 + floatBits) : (0x807FFFFF - absi);
    }
}

inline void ExtractFloatComponents(const unsigned floatBits, unsigned& sign,
                                   unsigned& exponent, unsigned& mantissa)
{
    mantissa = floatBits & 0x007FFFFF;
    const unsigned signExp = floatBits >> 23;
    exponent = signExp & 0xFF;
    sign = signExp >> 8;
}

bool FloatsDiffer(const float expected, const float actual,
                  const int tolerance, const bool compressDenorms)
{
    const unsigned expectedBits = FloatAsInt(expected);
    const unsigned actualBits = FloatAsInt(actual);

    unsigned es, ee, em, as, ae, am;
    ExtractFloatComponents(expectedBits, es, ee, em);
    ExtractFloatComponents(actualBits, as, ae, am);

    const bool isExpectedSpecial = (ee == 0xFF);
    const bool isActualSpecial = (ae == 0xFF);
    if (isExpectedSpecial)  // expected is a special float (-/+Inf or NaN)
    {
       if (isActualSpecial)  // actual is a special float (-/+Inf or NaN)
        {
            // Comparing special floats
            const bool isExpectedInf = (em == 0);
            const bool isActualInf = (am == 0);
            if (isExpectedInf)  // expected is -/+Inf
            {
                if (isActualInf)  // actual is -/+Inf
                {
                    return (es != as);  // Comparing -/+Inf with -/+Inf
                }
                else  // actual is NaN
                {
                    return true;  // Comparing -/+Inf with NaN
                }
            }
            else  // expected is NaN
            {
                return isActualInf;  // Comparing NaN with special float
            }
        }
        else  // actual is regular float
        {
            return true;  // Comparing NaN with regular float
        }
    }
    else  // expected is regular float
    {
        if (isActualSpecial) // actual is special float (-/+Inf or NaN)
        {
            return true;  // Comparing regular float with special float
        }
    }

    // Comparing regular floats
    int expectedBitsComp, actualBitsComp;
    if (compressDenorms)
    {
        expectedBitsComp = FloatForCompareCompressDenorms(expectedBits);
        actualBitsComp = FloatForCompareCompressDenorms(actualBits);
    }
    else
    {
        expectedBitsComp = FloatForCompare(expectedBits);
        actualBitsComp = FloatForCompare(actualBits);
    }

    return abs(expectedBitsComp - actualBitsComp) > tolerance;
}

inline int HalfForCompare(const half h)
{
    // Map neg 0 and pos 0 to 32768, allowing tolerance-based comparison
    // of small numbers of mixed sign.
    int rawHalf = h.bits();
    return rawHalf < 32767 ? rawHalf + 32768 : 2 * 32768 - rawHalf;
}

bool HalfsDiffer(const half expected, const half actual, const int tolerance)
{
    // TODO: Make this faster

    // (These are ints rather than shorts to allow subtraction below.)
    const int aimBits = HalfForCompare(expected);
    const int valBits = HalfForCompare(actual);

    if (expected.isNan())
    {
        return !actual.isNan();
    }
    else if (actual.isNan())
    {
        return !expected.isNan();
    }
    else if (expected.isInfinity())
    {
        return aimBits != valBits;
    }
    else if (actual.isInfinity())
    {
        return aimBits != valBits;
    }
    else
    {
        if (abs(valBits - aimBits) > tolerance)
        {
            return true;
        }
    }
    return false;
}

}

OCIO_NAMESPACE_EXIT




///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

OCIO_NAMESPACE_USING

#include <limits>
#include "unittest.h"

namespace
{
    
    void GetMxbResult(float* vout, float* m, float* x, float* v)
    {
        GetM44V4Product(vout, m, x);
        GetV4Sum(vout, vout, v);
    }
    
}

   
OIIO_ADD_TEST(MathUtils, M44_is_diagonal)
{
    {
        float m44[] = { 1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f };
        bool isdiag = IsM44Diagonal(m44);
        OIIO_CHECK_EQUAL(isdiag, true);

        m44[1] += 1e-8f;
        isdiag = IsM44Diagonal(m44);
        OIIO_CHECK_EQUAL(isdiag, false);
    }
}


OIIO_ADD_TEST(MathUtils, IsScalarEqualToZero)
{
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(0.0f), true);
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(-0.0f), true);
        
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(-1.072883670794056e-09f), false);
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(1.072883670794056e-09f), false);
        
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(-1.072883670794056e-03f), false);
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(1.072883670794056e-03f), false);
        
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(-1.072883670794056e-01f), false);
        OIIO_CHECK_EQUAL(IsScalarEqualToZero(1.072883670794056e-01f), false);
}

OIIO_ADD_TEST(MathUtils, GetM44Inverse)
{
        // This is a degenerate matrix, and shouldnt be invertible.
        float m[] = { 0.3f, 0.3f, 0.3f, 0.0f,
                      0.3f, 0.3f, 0.3f, 0.0f,
                      0.3f, 0.3f, 0.3f, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f };
        
        float mout[16];
        bool invertsuccess = GetM44Inverse(mout, m);
        OIIO_CHECK_EQUAL(invertsuccess, false);
}


OIIO_ADD_TEST(MathUtils, M44_M44_product)
{
    {
        float mout[16];
        float m1[] = { 1.0f, 2.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 1.0f, 0.0f,
                       1.0f, 0.0f, 1.0f, 0.0f,
                       0.0f, 1.0f, 3.0f, 1.0f };
        float m2[] = { 1.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 1.0f, 0.0f,
                       2.0f, 0.0f, 0.0f, 1.0f };
        GetM44M44Product(mout, m1, m2);
        
        float mcorrect[] = { 1.0f, 3.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 1.0f, 0.0f,
                       1.0f, 1.0f, 1.0f, 0.0f,
                       2.0f, 1.0f, 3.0f, 1.0f };
        
        for(int i=0; i<16; ++i)
        {
            OIIO_CHECK_EQUAL(mout[i], mcorrect[i]);
        }
    }
}

OIIO_ADD_TEST(MathUtils, M44_V4_product)
{
    {
        float vout[4];
        float m[] = { 1.0f, 2.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 1.0f, 0.0f,
                      1.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 1.0f, 3.0f, 1.0f };
        float v[] = { 1.0f, 2.0f, 3.0f, 4.0f };
        GetM44V4Product(vout, m, v);
        
        float vcorrect[] = { 5.0f, 5.0f, 4.0f, 15.0f };
        
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_EQUAL(vout[i], vcorrect[i]);
        }
    }
}

OIIO_ADD_TEST(MathUtils, V4_add)
{
    {
        float vout[4];
        float v1[] = { 1.0f, 2.0f, 3.0f, 4.0f };
        float v2[] = { 3.0f, 1.0f, 4.0f, 1.0f };
        GetV4Sum(vout, v1, v2);
        
        float vcorrect[] = { 4.0f, 3.0f, 7.0f, 5.0f };
        
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_EQUAL(vout[i], vcorrect[i]);
        }
    }
}

OIIO_ADD_TEST(MathUtils, mxb_eval)
{
    {
        float vout[4];
        float m[] = { 1.0f, 2.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 1.0f, 0.0f,
                      1.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 1.0f, 3.0f, 1.0f };
        float x[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float v[] = { 1.0f, 2.0f, 3.0f, 4.0f };
        GetMxbResult(vout, m, x, v);
        
        float vcorrect[] = { 4.0f, 4.0f, 5.0f, 9.0f };
        
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_EQUAL(vout[i], vcorrect[i]);
        }
    }
}

OIIO_ADD_TEST(MathUtils, Combine_two_mxb)
{
    float m1[] = { 1.0f, 0.0f, 2.0f, 0.0f,
                   2.0f, 1.0f, 0.0f, 1.0f,
                   0.0f, 1.0f, 2.0f, 0.0f,
                   1.0f, 0.0f, 0.0f, 1.0f };
    float v1[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    float m2[] = { 2.0f, 1.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f, 0.0f,
                   1.0f, 0.0f, 3.0f, 0.0f,
                   1.0f,1.0f, 1.0f, 1.0f };
    float v2[] = { 0.0f, 2.0f, 1.0f, 0.0f };
    float tolerance = 1e-9f;

    {
        float x[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float vout[4];

        // Combine two mx+b operations, and apply to test point
        float mout[16];
        float vcombined[4];      
        GetMxbCombine(mout, vout, m1, v1, m2, v2);
        GetMxbResult(vcombined, mout, x, vout);
        
        // Sequentially apply the two mx+b operations.
        GetMxbResult(vout, m1, x, v1);
        GetMxbResult(vout, m2, vout, v2);
        
        // Compare outputs
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_CLOSE(vcombined[i], vout[i], tolerance);
        }
    }
    
    {
        float x[] = { 6.0f, 0.5f, -2.0f, -0.1f };
        float vout[4];

        float mout[16];
        float vcombined[4];
        GetMxbCombine(mout, vout, m1, v1, m2, v2);
        GetMxbResult(vcombined, mout, x, vout);
        
        GetMxbResult(vout, m1, x, v1);
        GetMxbResult(vout, m2, vout, v2);
        
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_CLOSE(vcombined[i], vout[i], tolerance);
        }
    }
    
    {
        float x[] = { 26.0f, -0.5f, 0.005f, 12.1f };
        float vout[4];

        float mout[16];
        float vcombined[4];
        GetMxbCombine(mout, vout, m1, v1, m2, v2);
        GetMxbResult(vcombined, mout, x, vout);
        
        GetMxbResult(vout, m1, x, v1);
        GetMxbResult(vout, m2, vout, v2);
        
        // We pick a not so small tolerance, as we're dealing with
        // large numbers, and the error for CHECK_CLOSE is absolute.
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_CLOSE(vcombined[i], vout[i], 1e-3);
        }
    }
}

OIIO_ADD_TEST(MathUtils, mxb_invert)
{
    {
        float m[] = { 1.0f, 2.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 1.0f, 0.0f,
                      1.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 1.0f, 3.0f, 1.0f };
        float x[] = { 1.0f, 0.5f, -1.0f, 60.0f };
        float v[] = { 1.0f, 2.0f, 3.0f, 4.0f };
        
        float vresult[4];
        float mout[16];
        float vout[4];
        
        GetMxbResult(vresult, m, x, v);
        bool invertsuccess = GetMxbInverse(mout, vout, m, v);
        OIIO_CHECK_EQUAL(invertsuccess, true);
        
        GetMxbResult(vresult, mout, vresult, vout);
        
        float tolerance = 1e-9f;
        for(int i=0; i<4; ++i)
        {
            OIIO_CHECK_CLOSE(vresult[i], x[i], tolerance);
        }
    }
    
    {
        float m[] = { 0.3f, 0.3f, 0.3f, 0.0f,
                      0.3f, 0.3f, 0.3f, 0.0f,
                      0.3f, 0.3f, 0.3f, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f };
        float v[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        
        float mout[16];
        float vout[4];
        
        bool invertsuccess = GetMxbInverse(mout, vout, m, v);
        OIIO_CHECK_EQUAL(invertsuccess, false);
    }
}


// Infrastructure for testing FloatsDiffer()
//
#define KEEP_DENORMS      false
#define COMPRESS_DENORMS  true

#define TEST_CHECK_MESSAGE(a, msg) if(!(a)) throw Exception(msg.c_str())


namespace
{

const float posinf =  std::numeric_limits<float>::infinity();
const float neginf = -std::numeric_limits<float>::infinity();
const float qnan = std::numeric_limits<float>::quiet_NaN();
const float snan = std::numeric_limits<float>::signaling_NaN();

const float posmaxfloat =  std::numeric_limits<float>::max();
const float negmaxfloat = -std::numeric_limits<float>::max();

const float posminfloat =  std::numeric_limits<float>::min();
const float negminfloat = -std::numeric_limits<float>::min();

const float zero = 0.0f;
const float negzero = -0.0f;

const float posone = 1.0f;
const float negone = -1.0f;

const float posrandom =  12.345f;
const float negrandom = -12.345f;

// Helper macros to declare float variables close to a reference value.
// We use these variables to validate the threshold comparison of FloatsDiffer.
//
#define DECLARE_FLOAT(VAR_NAME, REFERENCE, ULP) \
  const float VAR_NAME = AddULP(REFERENCE, ULP)

#define DECLARE_FLOAT_PLUS_ULP(VAR_PREFIX, REFERENCE, ULP1, ULP2, ULP3, ULP4, ULP5, ULP6) \
  DECLARE_FLOAT(VAR_PREFIX##ULP1, REFERENCE, ULP1); \
  DECLARE_FLOAT(VAR_PREFIX##ULP2, REFERENCE, ULP2); \
  DECLARE_FLOAT(VAR_PREFIX##ULP3, REFERENCE, ULP3); \
  DECLARE_FLOAT(VAR_PREFIX##ULP4, REFERENCE, ULP4); \
  DECLARE_FLOAT(VAR_PREFIX##ULP5, REFERENCE, ULP5); \
  DECLARE_FLOAT(VAR_PREFIX##ULP6, REFERENCE, ULP6)

#define DECLARE_FLOAT_MINUS_ULP(VAR_PREFIX, REFERENCE, ULP1, ULP2, ULP3, ULP4, ULP5, ULP6) \
  DECLARE_FLOAT(VAR_PREFIX##ULP1, REFERENCE, -ULP1); \
  DECLARE_FLOAT(VAR_PREFIX##ULP2, REFERENCE, -ULP2); \
  DECLARE_FLOAT(VAR_PREFIX##ULP3, REFERENCE, -ULP3); \
  DECLARE_FLOAT(VAR_PREFIX##ULP4, REFERENCE, -ULP4); \
  DECLARE_FLOAT(VAR_PREFIX##ULP5, REFERENCE, -ULP5); \
  DECLARE_FLOAT(VAR_PREFIX##ULP6, REFERENCE, -ULP6)

#define DECLARE_FLOAT_RANGE_ULP(VAR_PLUS_PREFIX, VAR_MINUS_PREFIX, REFERENCE, ULP1, ULP2, ULP3, ULP4, ULP5, ULP6) \
  DECLARE_FLOAT_PLUS_ULP(VAR_PLUS_PREFIX, REFERENCE, ULP1, ULP2, ULP3, ULP4, ULP5, ULP6); \
  DECLARE_FLOAT_MINUS_ULP(VAR_MINUS_PREFIX, REFERENCE, ULP1, ULP2, ULP3, ULP4, ULP5, ULP6)

  // Create close float representations for comparison:
  // Create new floats for (-/+) 1, tol/2, tol-1, tol, tol+1 and 2*tol ULPs.
  const int tol = 8;

  // posinf_p*  =>  (NaN, NaN, NaN, NaN, NaN, NaN)
  DECLARE_FLOAT_RANGE_ULP(posinf_p, posinf_m, posinf, 1, 4, 7, 8, 9, 16);

  // neginf_p*  =>  (-NaN, -NaN, -NaN, -NaN, -NaN, -NaN)
  DECLARE_FLOAT_RANGE_ULP(neginf_p, neginf_m, neginf, 1, 4, 7, 8, 9, 16);

  // posmaxfloat_p*  =>  (+Inf, NaN, NaN, NaN, NaN, NaN)
  DECLARE_FLOAT_RANGE_ULP(posmaxfloat_p, posmaxfloat_m, posmaxfloat, 1, 4, 7, 8, 9, 16);

  // negmaxfloat_p*  =>  (-Inf, -NaN, -NaN, -NaN, -NaN, -NaN)
  DECLARE_FLOAT_RANGE_ULP(negmaxfloat_p, negmaxfloat_m, negmaxfloat, 1, 4, 7, 8, 9, 16);

  // posminfloat_m*  =>  denorms
  DECLARE_FLOAT_RANGE_ULP(posminfloat_p, posminfloat_m, posminfloat, 1, 4, 7, 8, 9, 16);

  // negminfloat_m*  =>  -denorms
  DECLARE_FLOAT_RANGE_ULP(negminfloat_p, negminfloat_m, negminfloat, 1, 4, 7, 8, 9, 16);

  // zero_p*  =>  denorms
  DECLARE_FLOAT_PLUS_ULP(zero_p, zero, 1, 4, 7, 8, 9, 16);

  // negzero_p*  =>  -denorms
  DECLARE_FLOAT_PLUS_ULP(negzero_p, negzero, 1, 4, 7, 8, 9, 16);

  DECLARE_FLOAT_RANGE_ULP(posone_p, posone_m, posone, 1, 4, 7, 8, 9, 16);
  DECLARE_FLOAT_RANGE_ULP(negone_p, negone_m, negone, 1, 4, 7, 8, 9, 16);

  DECLARE_FLOAT_RANGE_ULP(posrandom_p, posrandom_m, posrandom, 1, 4, 7, 8, 9, 16);
  DECLARE_FLOAT_RANGE_ULP(negrandom_p, negrandom_m, negrandom, 1, 4, 7, 8, 9, 16);

#undef DECLARE_FLOAT
#undef DECLARE_FLOAT_PLUS_ULP
#undef DECLARE_FLOAT_MINUS_ULP
#undef DECLARE_FLOAT_RANGE_ULP

//
// Helper functions to validate if (1 to 6) floating-point values are DIFFERENT
// from a reference value, within a given tolerance threshold, and considering
// that denormalized values are being compressed or kept.
//
std::string getErrorMessageDifferent(const float a, 
                                     const float b, 
                                     const int tolerance,
                                     const bool compressDenorms)
{
    std::ostringstream oss;
    oss << "The values " << a << " "
        << "(" << std::hex << std::showbase << FloatAsInt(a) << std::dec << ") "
        << "and " << b << " "
        << "(" << std::hex << std::showbase << FloatAsInt(b) << std::dec << ") "
        << "are expected to be DIFFERENT within a tolerance of " << tolerance << " ULPs "
        << ( compressDenorms ? "(when compressing denormalized numbers)."
                             : "(when keeping denormalized numbers)." );

    return oss.str();
}
                           
void checkFloatsAreDifferent(const float ref, const int tolerance, const bool compressDenorms,
                             const float a)
{
    TEST_CHECK_MESSAGE(FloatsDiffer(ref, a, tolerance, compressDenorms),
                       getErrorMessageDifferent(ref, a, tolerance, compressDenorms) );

    TEST_CHECK_MESSAGE(FloatsDiffer(a, ref, tolerance, compressDenorms),
                       getErrorMessageDifferent(a, ref, tolerance, compressDenorms) );
}

void checkFloatsAreDifferent(const float ref, const int tolerance, const bool compressDenorms,
                             const float a, const float b)
{
    checkFloatsAreDifferent(ref, tolerance, compressDenorms, a);
    checkFloatsAreDifferent(ref, tolerance, compressDenorms, b);
}

void checkFloatsAreDifferent(const float ref, const int tolerance, const bool compressDenorms,
                             const float a, const float b, const float c)
{
    checkFloatsAreDifferent(ref, tolerance, compressDenorms, a, b);
    checkFloatsAreDifferent(ref, tolerance, compressDenorms, c);
}

void checkFloatsAreDifferent(const float ref, const int tolerance, const bool compressDenorms,
                             const float a, const float b, const float c, const float d)
{
    checkFloatsAreDifferent(ref, tolerance, compressDenorms, a, b, c);
    checkFloatsAreDifferent(ref, tolerance, compressDenorms, d);
}

void checkFloatsAreDifferent(const float ref, const int tolerance, const bool compressDenorms,
                             const float a, const float b, const float c, const float d,
                             const float e)
{
    checkFloatsAreDifferent(ref, tolerance, compressDenorms, a, b, c, d);
    checkFloatsAreDifferent(ref, tolerance, compressDenorms, e);
}

void checkFloatsAreDifferent(const float ref, const int tolerance, const bool compressDenorms,
                             const float a, const float b, const float c, const float d,
                             const float e, const float f)
{
    checkFloatsAreDifferent(ref, tolerance, compressDenorms, a, b, c, d, e);
    checkFloatsAreDifferent(ref, tolerance, compressDenorms, f);
}

//
// Helper functions to validate if (1 to 6) floating-point values are CLOSE
// to a reference value, within a given tolerance threshold, and considering
// that denormalized values are being compressed or kept.
//
std::string getErrorMessageClose(const float a, const float b, const int tolerance,
                                 const bool compressDenorms)
{
    std::ostringstream oss;
    oss << "The values " << a << " "
        << "(" << std::hex << std::showbase << FloatAsInt(a) << std::dec << ") "
        << "and " << b << " "
        << "(" << std::hex << std::showbase << FloatAsInt(b) << std::dec << ") "
        << "are expected to be CLOSE within a tolerance of " << tolerance << " ULPs "
        << ( compressDenorms ? "(when compressing denormalized numbers)."
                             : "(when keeping denormalized numbers)." );

    return oss.str();
}
                               
void checkFloatsAreClose(const float ref, const int tolerance, const bool compressDenorms,
                         const float a)
{
    TEST_CHECK_MESSAGE(!FloatsDiffer(ref, a, tolerance, compressDenorms),
                       getErrorMessageClose(ref, a, tolerance, compressDenorms) );

    TEST_CHECK_MESSAGE(!FloatsDiffer(a, ref, tolerance, compressDenorms),
                       getErrorMessageClose(a, ref, tolerance, compressDenorms) );
}

void checkFloatsAreClose(const float ref, const int tolerance, const bool compressDenorms,
                         const float a, const float b)
{
    checkFloatsAreClose(ref, tolerance, compressDenorms, a);
    checkFloatsAreClose(ref, tolerance, compressDenorms, b);
}

void checkFloatsAreClose(const float ref, const int tolerance, const bool compressDenorms,
                         const float a, const float b, const float c)
{
    checkFloatsAreClose(ref, tolerance, compressDenorms, a, b);
    checkFloatsAreClose(ref, tolerance, compressDenorms, c);
}

void checkFloatsAreClose(const float ref, const int tolerance, const bool compressDenorms,
                         const float a, const float b, const float c, const float d)
{
    checkFloatsAreClose(ref, tolerance, compressDenorms, a, b, c);
    checkFloatsAreClose(ref, tolerance, compressDenorms, d);
}

void checkFloatsAreClose(const float ref, const int tolerance, const bool compressDenorms,
                         const float a, const float b, const float c, const float d,
                         const float e)
{
    checkFloatsAreClose(ref, tolerance, compressDenorms, a, b, c, d);
    checkFloatsAreClose(ref, tolerance, compressDenorms, e);
}

void checkFloatsAreClose(const float ref, const int tolerance, const bool compressDenorms,
                         const float a, const float b, const float c, const float d,
                         const float e, const float f)
{
    checkFloatsAreClose(ref, tolerance, compressDenorms, a, b, c, d, e);
    checkFloatsAreClose(ref, tolerance, compressDenorms, f);
}

//
// Helper functions to validate if 1 or 2 floating-point values are EQUAL
// to a reference value, considering that denormalized values are being
// compressed or kept.
//
std::string getErrorMessageEqual(const float a, const float b, const bool compressDenorms)
{
    std::ostringstream oss;
    oss << "The values " << a << " "
        << "(" << std::hex << std::showbase << FloatAsInt(a) << std::dec << ") "
        << "and " << b << " "
        << "(" << std::hex << std::showbase << FloatAsInt(b) << std::dec << ") "
        << "are expected to be EQUAL "
        << ( compressDenorms ? "(when compressing denormalized numbers)."
                             : "(when keeping denormalized numbers)." );

    return oss.str();
}

void checkFloatsAreEqual(const float ref, const bool compressDenorms, const float a)
{
    TEST_CHECK_MESSAGE(!FloatsDiffer(ref, a, 0, compressDenorms),
                       getErrorMessageEqual(ref, a, compressDenorms) );

    TEST_CHECK_MESSAGE(!FloatsDiffer(a, ref, 0, compressDenorms),
                       getErrorMessageEqual(a, ref, compressDenorms) );
}

void checkFloatsAreEqual(const float ref, const bool compressDenorms, 
                         const float a, const float b)
{
    checkFloatsAreEqual(ref, compressDenorms, a);
    checkFloatsAreEqual(ref, compressDenorms, b);
}

// Validate a set of floating-point comparison that are expected to be
// unaffected by the "compress denormalized floats" flag.
void checkFloatsDenormInvariant(const bool compressDenorms)
{
    checkFloatsAreEqual(posinf, compressDenorms, posinf);
    checkFloatsAreDifferent(posinf, compressDenorms, tol, neginf, qnan, snan);

    checkFloatsAreEqual(neginf, compressDenorms, neginf);
    checkFloatsAreDifferent(neginf, compressDenorms, tol, qnan, snan);

    checkFloatsAreEqual(qnan, compressDenorms, qnan, snan);
    checkFloatsAreEqual(snan, compressDenorms, snan);

    // Check positive infinity limits
    //
    checkFloatsAreDifferent(posinf, tol, compressDenorms, 
                            posinf_p1, posinf_p4, posinf_p7,
                            posinf_p8, posinf_p9, posinf_p16);

    checkFloatsAreDifferent(posinf, tol, compressDenorms,
                            posinf_m1, posinf_m4, posinf_m7,
                            posinf_m8, posinf_m9, posinf_m16);

    // Check negative infinity limits
    //
    checkFloatsAreDifferent(neginf, tol, compressDenorms, 
                            neginf_p1, neginf_p4, neginf_p7,
                            neginf_p8, neginf_p9, neginf_p16);

    checkFloatsAreDifferent(neginf, tol, compressDenorms, 
                            neginf_m1, neginf_m4, neginf_m7,
                            neginf_m8, neginf_m9, neginf_m16);

    // Check positive maximum float
    //
    checkFloatsAreEqual(posmaxfloat, compressDenorms, posinf_m1);
    checkFloatsAreEqual(posmaxfloat_p1, compressDenorms, posinf);

    checkFloatsAreDifferent(posmaxfloat, tol, compressDenorms,
                            posmaxfloat_p1, posmaxfloat_p4, posmaxfloat_p7,
                            posmaxfloat_p8, posmaxfloat_p9, posmaxfloat_p16 );

    checkFloatsAreClose(posmaxfloat, tol, compressDenorms, 
                        posmaxfloat_m1, posmaxfloat_m4,
                        posmaxfloat_m7, posmaxfloat_m8);

    checkFloatsAreDifferent(posmaxfloat, compressDenorms, tol, posmaxfloat_m9, posmaxfloat_m16 );

    // Check negative maximum float
    //
    checkFloatsAreEqual(negmaxfloat, compressDenorms, neginf_m1);
    checkFloatsAreEqual(negmaxfloat_p1, compressDenorms, neginf);

    checkFloatsAreDifferent(negmaxfloat, tol, compressDenorms, 
                            negmaxfloat_p1, negmaxfloat_p4, negmaxfloat_p7,
                            negmaxfloat_p8, negmaxfloat_p9, negmaxfloat_p16 );

    checkFloatsAreClose(negmaxfloat, tol, compressDenorms, 
                        negmaxfloat_m1, negmaxfloat_m4,
                        negmaxfloat_m7, negmaxfloat_m8);

    checkFloatsAreDifferent(negmaxfloat, compressDenorms, tol, negmaxfloat_m9, negmaxfloat_m16 );

    // Check zero and negative zero equality
    checkFloatsAreEqual(zero, compressDenorms, negzero);

    // Check positive and negative one
    checkFloatsAreDifferent(posone, tol, compressDenorms, posone_m16, posone_m9);
    checkFloatsAreClose(posone, tol, compressDenorms, posone_m8, posone_m7, posone_m4, posone_m1);
    checkFloatsAreClose(posone, tol, compressDenorms, posone_p1, posone_p4, posone_p7, posone_p8);
    checkFloatsAreDifferent(posone, tol, compressDenorms, posone_p9, posone_p16);

    checkFloatsAreDifferent(negone, tol, compressDenorms, negone_m16, negone_m9);
    checkFloatsAreClose(negone, tol, compressDenorms, negone_m8, negone_m7, negone_m4, negone_m1);
    checkFloatsAreClose(negone, tol, compressDenorms, negone_p1, negone_p4, negone_p7, negone_p8);
    checkFloatsAreDifferent(negone, tol, compressDenorms, negone_p9, negone_p16);

    // Check positive and negative random value
    checkFloatsAreDifferent(posrandom, tol, compressDenorms, posrandom_m16, posrandom_m9);
    checkFloatsAreClose(posrandom, tol, compressDenorms, 
                        posrandom_m8, posrandom_m7, posrandom_m4, posrandom_m1);
    checkFloatsAreClose(posrandom, tol, compressDenorms, 
                        posrandom_p1, posrandom_p4, posrandom_p7, posrandom_p8);
    checkFloatsAreDifferent(posrandom, tol, compressDenorms, posrandom_p9, posrandom_p16);

    checkFloatsAreDifferent(negrandom, tol, compressDenorms, negrandom_m16, negrandom_m9);
    checkFloatsAreClose(negrandom, tol, compressDenorms, 
                        negrandom_m8, negrandom_m7, negrandom_m4, negrandom_m1);
    checkFloatsAreClose(negrandom, tol, compressDenorms, negrandom_p1, 
                        negrandom_p4, negrandom_p7, negrandom_p8);
    checkFloatsAreDifferent(negrandom, tol, compressDenorms, negrandom_p9, negrandom_p16);
}

} // anon

OIIO_ADD_TEST(MathUtils, float_diff_keep_denorms_test)
{
    checkFloatsDenormInvariant(KEEP_DENORMS);

    // Check positive minimum float
    //
    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS, posminfloat_m16, posminfloat_m9);
    checkFloatsAreClose(posminfloat, tol, KEEP_DENORMS, 
                        posminfloat_m8, posminfloat_m7,
                        posminfloat_m4, posminfloat_m1);
    checkFloatsAreClose(posminfloat, tol, KEEP_DENORMS,
                        posminfloat_p1, posminfloat_p4,
                        posminfloat_p7, posminfloat_p8);
    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS, posminfloat_p9, posminfloat_p16);

    // Check negative minimum float
    //
    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, negminfloat_m16, negminfloat_m9);
    checkFloatsAreClose(negminfloat, tol, KEEP_DENORMS, 
                        negminfloat_m8, negminfloat_m7,
                        negminfloat_m4, negminfloat_m1);
    checkFloatsAreClose(negminfloat, tol, KEEP_DENORMS, 
                        negminfloat_p1, negminfloat_p4,
                        negminfloat_p7, negminfloat_p8);
    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, negminfloat_p9, negminfloat_p16);

    // Compare zero and positive denorms
    checkFloatsAreClose(zero, tol, KEEP_DENORMS, zero_p1, zero_p4, zero_p7, zero_p8);
    checkFloatsAreDifferent(zero, tol, KEEP_DENORMS, zero_p9, zero_p16);

    // Compare zero and negative denorms
    checkFloatsAreClose(zero, tol, KEEP_DENORMS, negzero_p1, negzero_p4, negzero_p7, negzero_p8);
    checkFloatsAreDifferent(zero, tol, KEEP_DENORMS, negzero_p9, negzero_p16);

    // Compare negative zero and positive denorms
    checkFloatsAreClose(negzero, tol, KEEP_DENORMS, zero_p1, zero_p4, zero_p7, zero_p8);
    checkFloatsAreDifferent(negzero, tol, KEEP_DENORMS, zero_p9, zero_p16);

    // Compare negative zero and negative denorms
    checkFloatsAreClose(negzero, tol, KEEP_DENORMS, 
                        negzero_p1, negzero_p4, negzero_p7, negzero_p8);
    checkFloatsAreDifferent(negzero, tol, KEEP_DENORMS, negzero_p9, negzero_p16);

    // Compare positive denorms and negative denorms
    checkFloatsAreClose(zero_p1, tol, KEEP_DENORMS, negzero_p1, negzero_p4, negzero_p7);
    checkFloatsAreDifferent(zero_p1, tol, KEEP_DENORMS, negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(zero_p4, tol, KEEP_DENORMS, negzero_p1, negzero_p4);
    checkFloatsAreDifferent(zero_p4, tol, KEEP_DENORMS, 
                            negzero_p7, negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreDifferent(zero_p9, tol, KEEP_DENORMS, 
                            negzero_p1, negzero_p4, negzero_p7,
                            negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(negzero_p1, tol, KEEP_DENORMS, zero_p1, zero_p4, zero_p7);
    checkFloatsAreDifferent(negzero_p1, tol, KEEP_DENORMS, zero_p8, zero_p9, zero_p16);

    checkFloatsAreClose(negzero_p4, tol, KEEP_DENORMS, zero_p1, zero_p4);
    checkFloatsAreDifferent(negzero_p4, tol, KEEP_DENORMS, zero_p7, zero_p8, zero_p9, zero_p16);

    checkFloatsAreDifferent(negzero_p9, tol, KEEP_DENORMS,
                            zero_p1, zero_p4, zero_p7,
                            zero_p8, zero_p9, zero_p16);

    // Compare negative and positive minimum floats
    //
    // Note: The float-point values being compared are expected to be different because there is
    //       the full set of denormalized values between zero and -/+MIN_FLOAT when denormalized
    //       values are kept.
    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS, zero_p1, zero_p4, zero_p7,
                                                          zero_p8, zero_p9, zero_p16);

    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS, 
                            negzero_p1, negzero_p4, negzero_p7,
                            negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS,
                            negminfloat_p1, negminfloat_p4, negminfloat_p7,
                            negminfloat_p8, negminfloat_p9, negminfloat_p16);

    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS,
                            negminfloat_m1, negminfloat_m4, negminfloat_m7,
                            negminfloat_m8, negminfloat_m9, negminfloat_m16);

    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, zero_p1, zero_p4, zero_p7,
                                                          zero_p8, zero_p9, zero_p16);

    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                                                          negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, 
                            posminfloat_p1, posminfloat_p4, posminfloat_p7,
                            posminfloat_p8, posminfloat_p9, posminfloat_p16);

    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, 
                            posminfloat_m1, posminfloat_m4, posminfloat_m7,
                            posminfloat_m8, posminfloat_m9, posminfloat_m16);
}

OIIO_ADD_TEST(MathUtils, float_diff_compress_denorms_test )
{
    checkFloatsDenormInvariant(COMPRESS_DENORMS);

    // Check positive minimum float
    //
    // Note: posminfloat_m* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(posminfloat, tol, COMPRESS_DENORMS, 
                        posminfloat_m16, posminfloat_m9, posminfloat_m8,
                        posminfloat_m7, posminfloat_m4, posminfloat_m1);
    checkFloatsAreClose(posminfloat, tol, COMPRESS_DENORMS, 
                        posminfloat_p1, posminfloat_p4,
                        posminfloat_p7, posminfloat_p8);
    checkFloatsAreDifferent(posminfloat, tol, COMPRESS_DENORMS, posminfloat_p9, posminfloat_p16);

    // Check negative minimum float
    //
    // Note: negminfloat_m* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, 
                        negminfloat_m16, negminfloat_m9, negminfloat_m8,
                        negminfloat_m7, negminfloat_m4, negminfloat_m1);
    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS,
                        negminfloat_p1, negminfloat_p4,
                        negminfloat_p7, negminfloat_p8);
    checkFloatsAreDifferent(negminfloat, tol, COMPRESS_DENORMS, negminfloat_p9, negminfloat_p16);

    // Compare zero and positive denorms
    //
    // Note: zero_p* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(zero, tol, COMPRESS_DENORMS,
                        zero_p1, zero_p4, zero_p7, zero_p8, zero_p9, zero_p16);

    // Compare zero and negative denorms
    //
    // Note: negzero_p* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(zero, tol, COMPRESS_DENORMS,
                        negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    // Compare negative zero and positive denorms
    //
    // Note: zero_p* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(negzero, tol, COMPRESS_DENORMS, 
                        zero_p1, zero_p4, zero_p7, zero_p8, zero_p9, zero_p16);

    // Compare negative zero and negative denorms
    //
    // Note: negzero_p* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(negzero, tol, COMPRESS_DENORMS, 
                        negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    // Compare positive denorms and negative denorms
    //
    // Note: negzero_p* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(zero_p1, tol, COMPRESS_DENORMS, 
                        negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(zero_p4, tol, COMPRESS_DENORMS,
                        negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(zero_p9, tol, COMPRESS_DENORMS,
                        negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(negzero_p1, tol, COMPRESS_DENORMS, 
                        zero_p1, zero_p4, zero_p7, zero_p8, zero_p9, zero_p16);

    checkFloatsAreClose(negzero_p4, tol, COMPRESS_DENORMS, 
                        zero_p1, zero_p4, zero_p7, zero_p8, zero_p9, zero_p16);

    checkFloatsAreClose(negzero_p9, tol, COMPRESS_DENORMS, 
                        zero_p1, zero_p4, zero_p7, zero_p8, zero_p9, zero_p16);

    // Compare negative and positive minimum floats
    //
    // Note: When compressing denorms, the mapped floating-point values ordering used for
    //       comparison becomes: ... , negminfloat , zero , posminfloat , ..., so the
    //       difference between negminfloat and posminfloat actually becomes 2 ULPs.
    //       Denormalized values like, zero_p*, negzero_p*, posminfloat_m*, negminfloat_m*
    //       are all mapped to zero.
    checkFloatsAreClose(zero, 1, COMPRESS_DENORMS, negminfloat);
    checkFloatsAreClose(zero, 1, COMPRESS_DENORMS, posminfloat);
    checkFloatsAreClose(posminfloat, 2, COMPRESS_DENORMS, negminfloat);

    checkFloatsAreClose(posminfloat, tol, COMPRESS_DENORMS, 
                        negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(posminfloat, tol, COMPRESS_DENORMS, negminfloat_p1, negminfloat_p4);
    checkFloatsAreDifferent(posminfloat, tol, COMPRESS_DENORMS, 
                            negminfloat_p7, negminfloat_p8,
                            negminfloat_p9, negminfloat_p16);

    checkFloatsAreClose(posminfloat, tol, COMPRESS_DENORMS, 
                        negminfloat_m1, negminfloat_m4, negminfloat_m7,
                        negminfloat_m8, negminfloat_m9, negminfloat_m16);

    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, 
                        zero_p1, zero_p4, zero_p7, zero_p8, zero_p9, zero_p16);

    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, 
                        negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, posminfloat_p1, posminfloat_p4);
    checkFloatsAreDifferent(negminfloat, tol, COMPRESS_DENORMS, 
                            posminfloat_p7, posminfloat_p8,
                            posminfloat_p9, posminfloat_p16);

    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, 
                        posminfloat_m1, posminfloat_m4, posminfloat_m7,
                        posminfloat_m8, posminfloat_m9, posminfloat_m16);
}

OIIO_ADD_TEST(MathUtils, half_bits_test)
{
    // Sanity check.
    OIIO_CHECK_EQUAL(0.5f, ConvertHalfBitsToFloat(0x3800));

    // Preserve negatives.
    OIIO_CHECK_EQUAL(-1.f, ConvertHalfBitsToFloat(0xbc00));

    // Preserve values > 1.
    OIIO_CHECK_EQUAL(1024.f, ConvertHalfBitsToFloat(0x6400));
}

OIIO_ADD_TEST(MathUtils, halfs_differ_test)
{
    half pos_inf;   pos_inf.setBits(31744);   // +inf
    half neg_inf;   neg_inf.setBits(64512);   // -inf
    half pos_nan;   pos_nan.setBits(31745);   // +nan
    half neg_nan;   neg_nan.setBits(64513);   // -nan
    half pos_max;   pos_max.setBits(31743);   // +HALF_MAX
    half neg_max;   neg_max.setBits(64511);   // -HALF_MAX
    half pos_zero;  pos_zero.setBits(0);      // +0
    half neg_zero;  neg_zero.setBits(32768);  // -0
    half pos_small; pos_small.setBits(4);     // +small
    half neg_small; neg_small.setBits(32772); // -small
    half pos_1;     pos_1.setBits(15360);     // 
    half pos_2;     pos_2.setBits(15365);     // 
    half neg_1;     neg_1.setBits(50000);     // 
    half neg_2;     neg_2.setBits(50005);     // 

    int tol = 10;

    OIIO_CHECK_ASSERT(HalfsDiffer(pos_inf, neg_inf, tol));
    OIIO_CHECK_ASSERT(HalfsDiffer(pos_inf, pos_nan, tol));
    OIIO_CHECK_ASSERT(HalfsDiffer(neg_inf, neg_nan, tol));
    OIIO_CHECK_ASSERT(HalfsDiffer(pos_max, pos_inf, tol));
    OIIO_CHECK_ASSERT(HalfsDiffer(neg_max, neg_inf, tol));
    OIIO_CHECK_ASSERT(HalfsDiffer(pos_1, neg_1, tol));
    OIIO_CHECK_ASSERT(HalfsDiffer(pos_2, pos_1, 0));
    OIIO_CHECK_ASSERT(HalfsDiffer(neg_2, neg_1, 0));

    OIIO_CHECK_ASSERT(!HalfsDiffer(pos_zero, neg_zero, 0));
    OIIO_CHECK_ASSERT(!HalfsDiffer(pos_small, neg_small, tol));
    OIIO_CHECK_ASSERT(!HalfsDiffer(pos_2, pos_1, tol));
    OIIO_CHECK_ASSERT(!HalfsDiffer(neg_2, neg_1, tol));
}

OIIO_ADD_TEST(MathUtils, clamp)
{
    OIIO_CHECK_EQUAL(-1.0f, Clamp(std::numeric_limits<float>::quiet_NaN(), -1.0f, 1.0f));

    OIIO_CHECK_EQUAL(10.0f, Clamp( std::numeric_limits<float>::infinity(),  5.0f, 10.0f));
    OIIO_CHECK_EQUAL(5.0f, Clamp(-std::numeric_limits<float>::infinity(),  5.0f, 10.0f));

    OIIO_CHECK_EQUAL(0.0000005f, Clamp( 0.0000005f, 0.0f, 1.0f));
    OIIO_CHECK_EQUAL(0.0f,       Clamp(-0.0000005f, 0.0f, 1.0f));
    OIIO_CHECK_EQUAL(1.0f,       Clamp( 1.0000005f, 0.0f, 1.0f));
}

#endif
