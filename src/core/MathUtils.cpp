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

#include <OpenColorIO/OpenColorIO.h>

#include <cstring>

#include "MathUtils.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        const float FLTMIN = std::numeric_limits<float>::min();
    }
    
    bool IsScalarEqualToZero(float v)
    {
        return equalWithAbsError(v, 0.0f, FLTMIN);
    }

    bool IsScalarEqualToZeroFlt(double v)
    {
        return equalWithAbsError(float(v), 0.0f, FLTMIN);
    }

    bool IsScalarEqualToOne(float v)
    {
        return equalWithAbsError(v, 1.0f, FLTMIN);
    }
    
    bool IsScalarEqualToOneFlt(double v)
    {
        return equalWithAbsError(float(v), 1.0f, FLTMIN);
    }
    
    bool IsVecEqualToZero(const float* v, int size)
    {
        for(int i=0; i<size; ++i)
        {
            if(!IsScalarEqualToZero(v[i])) return false;
        }
        return true;
    }
    
    bool IsVecEqualToOne(const float* v, int size)
    {
        for(int i=0; i<size; ++i)
        {
            if(!IsScalarEqualToOne(v[i])) return false;
        }
        return true;
    }

    bool IsVecEqualToOneFlt(const double* v, int size)
    {
        for(int i=0; i<size; ++i)
        {
            if(!IsScalarEqualToOneFlt(v[i])) return false;
        }
        return true;
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

    template<typename T>
    bool VecsEqualWithRelErrorT(const T* v1, int size1,
                               const T* v2, int size2,
                               T e)
    {
        if(size1 != size2) return false;
        for(int i=0; i<size1; ++i)
        {
            if(!equalWithRelError(v1[i], v2[i], e)) return false;
        }
        
        return true;
    }
    
    bool VecsEqualWithRelError(const float* v1, int size1,
                               const float* v2, int size2,
                               float e)
    {
        return VecsEqualWithRelErrorT(v1, size1, v2, size2, e);
    }
    
    bool VecsEqualWithRelError(const double* v1, int size1,
                               const double* v2, int size2,
                               double e)
    {
        return VecsEqualWithRelErrorT(v1, size1, v2, size2, e);
    }

    double ClampToNormHalf(double val)
    {
        if(val < -HALF_MAX)
        {
            return -HALF_MAX;
        }

        if(val > -HALF_NRM_MIN && val<HALF_NRM_MIN)
        {
            return 0.0;
        }

        if(val > HALF_MAX)
        {
            return HALF_MAX;
        }

        return val;
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
        else if (isnan(f))
        {
            return 0.0f;
        }
        return f;
    }

    float ConvertHalfBitsToFloat(unsigned short val)
    {
        half hVal;
        hVal.setBits(val);
        return (float)hVal;
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

    // Extract the sign, exponent and mantissa components of a floating-point number.
    inline void ExtractFloatComponents(unsigned floatBits, unsigned & sign,
        unsigned & exponent, unsigned & mantissa)
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

    bool HalfsDiffer(half aimHalf, half valHalf, int tol)
    {
        // TODO: Make this faster

        // (These are ints rather than shorts to allow subtraction below.)
        const int aimBits = HalfForCompare(aimHalf);
        const int valBits = HalfForCompare(valHalf);

        if (aimHalf.isNan())
        {
            return !valHalf.isNan();
        }
        else if (valHalf.isNan())
        {
            return !aimHalf.isNan();
        }
        else if (aimHalf.isInfinity())
        {
            return aimBits != valBits;
        }
        else if (valHalf.isInfinity())
        {
            return aimBits != valBits;
        }
        else
        {
            if (abs(valBits - aimBits) > tol)
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

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(MathUtils, Clamp)
{
    OIIO_CHECK_EQUAL(OCIO::clamp(10.0f, 0.f, 100.f), 10.f);
    OIIO_CHECK_EQUAL(OCIO::clamp(10.0f, 100.f, 0.f), 0.f);
    OIIO_CHECK_EQUAL(OCIO::clamp(10.0f, 100.f, 20.f), 100.f);
    OIIO_CHECK_EQUAL(OCIO::clamp(10.0f, 0.f, 5.f), 5.f);
    OIIO_CHECK_EQUAL(OCIO::clamp(5.0f, 0.f, 5.f), 5.f);
    OIIO_CHECK_EQUAL(OCIO::clamp(5.0f, 5.f, 5.f), 5.f);
}

OIIO_ADD_TEST(MathUtils, IsScalarEqualToZero)
{
    OIIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(0.0f), true);
    OIIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(-0.0f), true);
    
    OIIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(-1.072883670794056e-09f), false);
    OIIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(1.072883670794056e-09f), false);
    
    OIIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(-1.072883670794056e-03f), false);
    OIIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(1.072883670794056e-03f), false);
    
    OIIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(-1.072883670794056e-01f), false);
    OIIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(1.072883670794056e-01f), false);
}

OIIO_ADD_TEST(MathUtils, half_bits_test)
{
    // Sanity check.
    OIIO_CHECK_EQUAL(0.5f, OCIO::ConvertHalfBitsToFloat(0x3800));

    // Preserve negatives.
    OIIO_CHECK_EQUAL(-1.f, OCIO::ConvertHalfBitsToFloat(0xbc00));

    // Preserve values > 1.
    OIIO_CHECK_EQUAL(1024.f, OCIO::ConvertHalfBitsToFloat(0x6400));
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

    OIIO_CHECK_ASSERT(OCIO::HalfsDiffer(pos_inf, neg_inf, tol));
    OIIO_CHECK_ASSERT(OCIO::HalfsDiffer(pos_inf, pos_nan, tol));
    OIIO_CHECK_ASSERT(OCIO::HalfsDiffer(neg_inf, neg_nan, tol));
    OIIO_CHECK_ASSERT(OCIO::HalfsDiffer(pos_max, pos_inf, tol));
    OIIO_CHECK_ASSERT(OCIO::HalfsDiffer(neg_max, neg_inf, tol));
    OIIO_CHECK_ASSERT(OCIO::HalfsDiffer(pos_1, neg_1, tol));
    OIIO_CHECK_ASSERT(OCIO::HalfsDiffer(pos_2, pos_1, 0));
    OIIO_CHECK_ASSERT(OCIO::HalfsDiffer(neg_2, neg_1, 0));

    OIIO_CHECK_ASSERT(!OCIO::HalfsDiffer(pos_zero, neg_zero, 0));
    OIIO_CHECK_ASSERT(!OCIO::HalfsDiffer(pos_small, neg_small, tol));
    OIIO_CHECK_ASSERT(!OCIO::HalfsDiffer(pos_2, pos_1, tol));
    OIIO_CHECK_ASSERT(!OCIO::HalfsDiffer(neg_2, neg_1, tol));
}

OIIO_ADD_TEST(Numerics, check_abs_error_test)
{
    const float eps = 1e-5f;

    OIIO_CHECK_ASSERT(!OCIO::equalWithAbsError(2e-4f, 3e-4f, eps));
    OIIO_CHECK_ASSERT(OCIO::equalWithAbsError(2e-6f, 3e-6f, eps));

    OIIO_CHECK_ASSERT(!OCIO::equalWithAbsError(-2e-4f, -3e-4f, eps));
    OIIO_CHECK_ASSERT(OCIO::equalWithAbsError(-2e-6f, -3e-6f, eps));

    OIIO_CHECK_ASSERT(!OCIO::equalWithAbsError(-2e-4f, 3e-4f, eps));
    OIIO_CHECK_ASSERT(OCIO::equalWithAbsError(-2e-6f, 3e-6f, eps));
    OIIO_CHECK_ASSERT(!OCIO::equalWithAbsError(-5e-6f, 51e-7f, eps));
}

// Infrastructure for testing FloatsDiffer
#define KEEP_DENORMS      false
#define COMPRESS_DENORMS  true

namespace
{
    // Add a number of ULPs (Unit of Least Precision) to a given
    // floating-point number.
    // - f original floating-point number
    // - ulp the number of ULPs to be added to the floating-point number
    //! \return the original floating-point number added by the number of ULPs.
    inline float AddULP(const float f, const int ulp)
    {
        return OCIO::IntAsFloat(OCIO::FloatAsInt(f) + ulp);
    }

    const float posinf = std::numeric_limits<float>::infinity();
    const float neginf = -std::numeric_limits<float>::infinity();
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float snan = std::numeric_limits<float>::signaling_NaN();

    const float posmaxfloat = std::numeric_limits<float>::max();
    const float negmaxfloat = -std::numeric_limits<float>::max();

    const float posminfloat = std::numeric_limits<float>::min();
    const float negminfloat = -std::numeric_limits<float>::min();

    const float zero = 0.0f;
    const float negzero = -0.0f;

    const float posone = 1.0f;
    const float negone = -1.0f;

    const float posrandom = 12.345f;
    const float negrandom = -12.345f;

    // Helper macros to declare float variables close to a reference value.
    // We use these variables to validate the threshold comparison of floatsDiffer.
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

    // Helper functions to validate if (1 to 6) floating-point values are DIFFERENT
    // from a reference value, within a given tolerance threshold, and considering
    // that denormalized values are being compressed or kept.
    std::string getErrorMessageDifferent(const float a,
        const float b,
        const int tolerance,
        const bool compressDenorms)
    {
        std::ostringstream oss;
        oss << "The values " << a << " "
            << "(" << std::hex << std::showbase << OCIO::FloatAsInt(a) << std::dec << ") "
            << "and " << b << " "
            << "(" << std::hex << std::showbase << OCIO::FloatAsInt(b) << std::dec << ") "
            << "are expected to be DIFFERENT within a tolerance of " << tolerance << " ULPs "
            << (compressDenorms ? "(when compressing denormalized numbers)."
                : "(when keeping denormalized numbers).");

        return oss.str();
    }
	
    void checkFloatsAreDifferent(const float ref, const int tolerance, const bool compressDenorms,
        const float a)
    {
        OIIO_CHECK_ASSERT_MESSAGE(OCIO::FloatsDiffer(ref, a, tolerance, compressDenorms),
            getErrorMessageDifferent(ref, a, tolerance, compressDenorms));

        OIIO_CHECK_ASSERT_MESSAGE(OCIO::FloatsDiffer(a, ref, tolerance, compressDenorms),
            getErrorMessageDifferent(a, ref, tolerance, compressDenorms));
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

    // Helper functions to validate if (1 to 6) floating-point values are CLOSE
    // to a reference value, within a given tolerance threshold, and considering
    // that denormalized values are being compressed or kept.
    std::string getErrorMessageClose(const float a, const float b, const int tolerance,
        const bool compressDenorms)
    {
        std::ostringstream oss;
        oss << "The values " << a << " "
            << "(" << std::hex << std::showbase << OCIO::FloatAsInt(a) << std::dec << ") "
            << "and " << b << " "
            << "(" << std::hex << std::showbase << OCIO::FloatAsInt(b) << std::dec << ") "
            << "are expected to be CLOSE within a tolerance of " << tolerance << " ULPs "
            << (compressDenorms ? "(when compressing denormalized numbers)."
                : "(when keeping denormalized numbers).");

        return oss.str();
    }

    void checkFloatsAreClose(const float ref, const int tolerance, const bool compressDenorms,
        const float a)
    {
        OIIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(ref, a, tolerance, compressDenorms),
            getErrorMessageClose(ref, a, tolerance, compressDenorms));

        OIIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(a, ref, tolerance, compressDenorms),
            getErrorMessageClose(a, ref, tolerance, compressDenorms));
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

    // Helper functions to validate if 1 or 2 floating-point values are EQUAL
    // to a reference value, considering that denormalized values are being
    // compressed or kept.
    std::string getErrorMessageEqual(const float a, const float b, const bool compressDenorms)
    {
        std::ostringstream oss;
        oss << "The values " << a << " "
            << "(" << std::hex << std::showbase << OCIO::FloatAsInt(a) << std::dec << ") "
            << "and " << b << " "
            << "(" << std::hex << std::showbase << OCIO::FloatAsInt(b) << std::dec << ") "
            << "are expected to be EQUAL "
            << (compressDenorms ? "(when compressing denormalized numbers)."
                : "(when keeping denormalized numbers).");

        return oss.str();
    }

    void checkFloatsAreEqual(const float ref, const bool compressDenorms, const float a)
    {
        OIIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(ref, a, 0, compressDenorms),
            getErrorMessageEqual(ref, a, compressDenorms));

        OIIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(a, ref, 0, compressDenorms),
            getErrorMessageEqual(a, ref, compressDenorms));
    }

    void checkFloatsAreEqual(const float ref, const bool compressDenorms, const float a, const float b)
    {
        checkFloatsAreEqual(ref, compressDenorms, a);
        checkFloatsAreEqual(ref, compressDenorms, b);
    }
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
    checkFloatsAreDifferent(posinf, tol, compressDenorms, posinf_p1, posinf_p4, posinf_p7,
                            posinf_p8, posinf_p9, posinf_p16);

    checkFloatsAreDifferent(posinf, tol, compressDenorms, posinf_m1, posinf_m4, posinf_m7,
                            posinf_m8, posinf_m9, posinf_m16);

    // Check negative infinity limits
    //
    checkFloatsAreDifferent(neginf, tol, compressDenorms, neginf_p1, neginf_p4, neginf_p7,
                            neginf_p8, neginf_p9, neginf_p16);

    checkFloatsAreDifferent(neginf, tol, compressDenorms, neginf_m1, neginf_m4, neginf_m7,
                            neginf_m8, neginf_m9, neginf_m16);

    // Check positive maximum float
    //
    checkFloatsAreEqual(posmaxfloat, compressDenorms, posinf_m1);
    checkFloatsAreEqual(posmaxfloat_p1, compressDenorms, posinf);

    checkFloatsAreDifferent(posmaxfloat, tol, compressDenorms, posmaxfloat_p1, posmaxfloat_p4, posmaxfloat_p7,
                            posmaxfloat_p8, posmaxfloat_p9, posmaxfloat_p16);

    checkFloatsAreClose(posmaxfloat, tol, compressDenorms, posmaxfloat_m1, posmaxfloat_m4,
                        posmaxfloat_m7, posmaxfloat_m8);

    checkFloatsAreDifferent(posmaxfloat, compressDenorms, tol, posmaxfloat_m9, posmaxfloat_m16);

    // Check negative maximum float
    //
    checkFloatsAreEqual(negmaxfloat, compressDenorms, neginf_m1);
    checkFloatsAreEqual(negmaxfloat_p1, compressDenorms, neginf);

    checkFloatsAreDifferent(negmaxfloat, tol, compressDenorms, negmaxfloat_p1, negmaxfloat_p4, negmaxfloat_p7,
                            negmaxfloat_p8, negmaxfloat_p9, negmaxfloat_p16);

    checkFloatsAreClose(negmaxfloat, tol, compressDenorms, negmaxfloat_m1, negmaxfloat_m4,
                        negmaxfloat_m7, negmaxfloat_m8);

    checkFloatsAreDifferent(negmaxfloat, compressDenorms, tol, negmaxfloat_m9, negmaxfloat_m16);

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
    checkFloatsAreClose(posrandom, tol, compressDenorms, posrandom_m8, posrandom_m7, posrandom_m4, posrandom_m1);
    checkFloatsAreClose(posrandom, tol, compressDenorms, posrandom_p1, posrandom_p4, posrandom_p7, posrandom_p8);
    checkFloatsAreDifferent(posrandom, tol, compressDenorms, posrandom_p9, posrandom_p16);

    checkFloatsAreDifferent(negrandom, tol, compressDenorms, negrandom_m16, negrandom_m9);
    checkFloatsAreClose(negrandom, tol, compressDenorms, negrandom_m8, negrandom_m7, negrandom_m4, negrandom_m1);
    checkFloatsAreClose(negrandom, tol, compressDenorms, negrandom_p1, negrandom_p4, negrandom_p7, negrandom_p8);
    checkFloatsAreDifferent(negrandom, tol, compressDenorms, negrandom_p9, negrandom_p16);
}

OIIO_ADD_TEST(MathUtils, float_diff_keep_denorms_test)
{
    checkFloatsDenormInvariant(KEEP_DENORMS);

    // Check positive minimum float
    //
    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS, posminfloat_m16, posminfloat_m9);
    checkFloatsAreClose(posminfloat, tol, KEEP_DENORMS, posminfloat_m8, posminfloat_m7,
                        posminfloat_m4, posminfloat_m1);
    checkFloatsAreClose(posminfloat, tol, KEEP_DENORMS, posminfloat_p1, posminfloat_p4,
                        posminfloat_p7, posminfloat_p8);
    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS, posminfloat_p9, posminfloat_p16);

    // Check negative minimum float
    //
    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, negminfloat_m16, negminfloat_m9);
    checkFloatsAreClose(negminfloat, tol, KEEP_DENORMS, negminfloat_m8, negminfloat_m7,
                        negminfloat_m4, negminfloat_m1);
    checkFloatsAreClose(negminfloat, tol, KEEP_DENORMS, negminfloat_p1, negminfloat_p4,
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
    checkFloatsAreClose(negzero, tol, KEEP_DENORMS, negzero_p1, negzero_p4, negzero_p7, negzero_p8);
    checkFloatsAreDifferent(negzero, tol, KEEP_DENORMS, negzero_p9, negzero_p16);

    // Compare positive denorms and negative denorms
    checkFloatsAreClose(zero_p1, tol, KEEP_DENORMS, negzero_p1, negzero_p4, negzero_p7);
    checkFloatsAreDifferent(zero_p1, tol, KEEP_DENORMS, negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(zero_p4, tol, KEEP_DENORMS, negzero_p1, negzero_p4);
    checkFloatsAreDifferent(zero_p4, tol, KEEP_DENORMS, negzero_p7, negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreDifferent(zero_p9, tol, KEEP_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                            negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(negzero_p1, tol, KEEP_DENORMS, zero_p1, zero_p4, zero_p7);
    checkFloatsAreDifferent(negzero_p1, tol, KEEP_DENORMS, zero_p8, zero_p9, zero_p16);

    checkFloatsAreClose(negzero_p4, tol, KEEP_DENORMS, zero_p1, zero_p4);
    checkFloatsAreDifferent(negzero_p4, tol, KEEP_DENORMS, zero_p7, zero_p8, zero_p9, zero_p16);

    checkFloatsAreDifferent(negzero_p9, tol, KEEP_DENORMS, zero_p1, zero_p4, zero_p7,
                            zero_p8, zero_p9, zero_p16);

    // Compare negative and positive minimum floats
    //
    // Note: The float-point values being compared are expected to be different because there is
    //       the full set of denormalized values between zero and -/+MIN_FLOAT when denormalized
    //       values are kept.
    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS, zero_p1, zero_p4, zero_p7,
                            zero_p8, zero_p9, zero_p16);

    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                            negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS, negminfloat_p1, negminfloat_p4, negminfloat_p7,
                            negminfloat_p8, negminfloat_p9, negminfloat_p16);

    checkFloatsAreDifferent(posminfloat, tol, KEEP_DENORMS, negminfloat_m1, negminfloat_m4, negminfloat_m7,
                            negminfloat_m8, negminfloat_m9, negminfloat_m16);

    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, zero_p1, zero_p4, zero_p7,
                            zero_p8, zero_p9, zero_p16);

    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                            negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, posminfloat_p1, posminfloat_p4, posminfloat_p7,
                            posminfloat_p8, posminfloat_p9, posminfloat_p16);

    checkFloatsAreDifferent(negminfloat, tol, KEEP_DENORMS, posminfloat_m1, posminfloat_m4, posminfloat_m7,
                            posminfloat_m8, posminfloat_m9, posminfloat_m16);
}

OIIO_ADD_TEST(MathUtils, float_diff_compress_denorms_test)
{
    checkFloatsDenormInvariant(COMPRESS_DENORMS);

    // Check positive minimum float
    //
    // Note: posminfloat_m* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(posminfloat, tol, COMPRESS_DENORMS, posminfloat_m16, posminfloat_m9, posminfloat_m8,
                        posminfloat_m7, posminfloat_m4, posminfloat_m1);
    checkFloatsAreClose(posminfloat, tol, COMPRESS_DENORMS, posminfloat_p1, posminfloat_p4,
                        posminfloat_p7, posminfloat_p8);
    checkFloatsAreDifferent(posminfloat, tol, COMPRESS_DENORMS, posminfloat_p9, posminfloat_p16);

    // Check negative minimum float
    //
    // Note: negminfloat_m* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, negminfloat_m16, negminfloat_m9, negminfloat_m8,
                        negminfloat_m7, negminfloat_m4, negminfloat_m1);
    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, negminfloat_p1, negminfloat_p4,
                        negminfloat_p7, negminfloat_p8);
    checkFloatsAreDifferent(negminfloat, tol, COMPRESS_DENORMS, negminfloat_p9, negminfloat_p16);

    // Compare zero and positive denorms
    //
    // Note: zero_p* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(zero, tol, COMPRESS_DENORMS, zero_p1, zero_p4, zero_p7, zero_p8, zero_p9, zero_p16);

    // Compare zero and negative denorms
    //
    // Note: negzero_p* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(zero, tol, COMPRESS_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    // Compare negative zero and positive denorms
    //
    // Note: zero_p* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(negzero, tol, COMPRESS_DENORMS, zero_p1, zero_p4, zero_p7, zero_p8, zero_p9, zero_p16);

    // Compare negative zero and negative denorms
    //
    // Note: negzero_p* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(negzero, tol, COMPRESS_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    // Compare positive denorms and negative denorms
    //
    // Note: negzero_p* are mapped to zero when compressing denormalized values.
    checkFloatsAreClose(zero_p1, tol, COMPRESS_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(zero_p4, tol, COMPRESS_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(zero_p9, tol, COMPRESS_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(negzero_p1, tol, COMPRESS_DENORMS, zero_p1, zero_p4, zero_p7,
                        zero_p8, zero_p9, zero_p16);

    checkFloatsAreClose(negzero_p4, tol, COMPRESS_DENORMS, zero_p1, zero_p4, zero_p7,
                        zero_p8, zero_p9, zero_p16);

    checkFloatsAreClose(negzero_p9, tol, COMPRESS_DENORMS, zero_p1, zero_p4, zero_p7,
                        zero_p8, zero_p9, zero_p16);

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

    checkFloatsAreClose(posminfloat, tol, COMPRESS_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(posminfloat, tol, COMPRESS_DENORMS, negminfloat_p1, negminfloat_p4);
    checkFloatsAreDifferent(posminfloat, tol, COMPRESS_DENORMS, negminfloat_p7, negminfloat_p8,
                            negminfloat_p9, negminfloat_p16);

    checkFloatsAreClose(posminfloat, tol, COMPRESS_DENORMS, negminfloat_m1, negminfloat_m4, negminfloat_m7,
                        negminfloat_m8, negminfloat_m9, negminfloat_m16);

    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, zero_p1, zero_p4, zero_p7,
                        zero_p8, zero_p9, zero_p16);

    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, negzero_p1, negzero_p4, negzero_p7,
                        negzero_p8, negzero_p9, negzero_p16);

    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, posminfloat_p1, posminfloat_p4);
    checkFloatsAreDifferent(negminfloat, tol, COMPRESS_DENORMS, posminfloat_p7, posminfloat_p8,
                            posminfloat_p9, posminfloat_p16);

    checkFloatsAreClose(negminfloat, tol, COMPRESS_DENORMS, posminfloat_m1, posminfloat_m4, posminfloat_m7,
                        posminfloat_m8, posminfloat_m9, posminfloat_m16);
}



#endif

