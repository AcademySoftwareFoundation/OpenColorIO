// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>
#include <sstream>
#include <string.h>
#include <type_traits>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"

namespace OCIO_NAMESPACE
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
// |  Negative zero                             |  0x80000000               |  0x80000000               |
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
// |  Negative zero                             |  0x80000000               |  0x80000000               |
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
    static_assert(sizeof(int)==4,
                  "Mathematical operations based on 32-bits in-memory integer representation.");

    static_assert(sizeof(float)==4,
                  "Mathematical operations based on 32-bits in-memory float representation.");

    const unsigned expectedBits = FloatAsInt(expected);
    const unsigned actualBits   = FloatAsInt(actual);

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

    // TODO: Revisit the ULP comparison to only use unsigned integers. 

    // Comparing regular floats.
    unsigned expectedBitsComp, actualBitsComp;
    if (compressDenorms)
    {
        expectedBitsComp = FloatForCompareCompressDenorms(expectedBits);
        actualBitsComp   = FloatForCompareCompressDenorms(actualBits);
    }
    else
    {
        expectedBitsComp = FloatForCompare(expectedBits);
        actualBitsComp   = FloatForCompare(actualBits);
    }

    const unsigned diff = (expectedBitsComp > actualBitsComp)
                        ? (expectedBitsComp - actualBitsComp)
                        : (actualBitsComp - expectedBitsComp);

    return diff > (unsigned)tolerance;
}

inline int HalfForCompare(const half h)
{
    // Map neg 0 and pos 0 to 32768, allowing tolerance-based comparison
    // of small numbers of mixed sign.
    const int rawHalf = h.bits();
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

} // namespace OCIO_NAMESPACE

