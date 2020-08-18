// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <limits>

#include "MathUtils.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
void GetMxbResult(float* vout, float* m, float* x, float* v)
{
    OCIO::GetM44V4Product(vout, m, x);
    OCIO::GetV4Sum(vout, vout, v);
}
}

OCIO_ADD_TEST(MathUtils, is_scalar_equal_to_zero)
{
    OCIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(0.0f), true);
    OCIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(-0.0f), true);

    OCIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(-1.072883670794056e-09f), false);
    OCIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(1.072883670794056e-09f), false);

    OCIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(-1.072883670794056e-03f), false);
    OCIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(1.072883670794056e-03f), false);

    OCIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(-1.072883670794056e-01f), false);
    OCIO_CHECK_EQUAL(OCIO::IsScalarEqualToZero(1.072883670794056e-01f), false);
}

OCIO_ADD_TEST(MathUtils, get_m44_inverse)
{
    // This is a degenerate matrix, and shouldn't be invertible.
    float m[] = { 0.3f, 0.3f, 0.3f, 0.0f,
                  0.3f, 0.3f, 0.3f, 0.0f,
                  0.3f, 0.3f, 0.3f, 0.0f,
                  0.0f, 0.0f, 0.0f, 1.0f };

    float mout[16];
    bool invertsuccess = OCIO::GetM44Inverse(mout, m);
    OCIO_CHECK_EQUAL(invertsuccess, false);
}


OCIO_ADD_TEST(MathUtils, m44_m44_product)
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
    OCIO::GetM44M44Product(mout, m1, m2);

    float mcorrect[] = { 1.0f, 3.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 1.0f, 0.0f,
                   1.0f, 1.0f, 1.0f, 0.0f,
                   2.0f, 1.0f, 3.0f, 1.0f };

    for(int i=0; i<16; ++i)
    {
        OCIO_CHECK_EQUAL(mout[i], mcorrect[i]);
    }
}

OCIO_ADD_TEST(MathUtils, m44_v4_product)
{
    float vout[4];
    float m[] = { 1.0f, 2.0f, 0.0f, 0.0f,
                  0.0f, 1.0f, 1.0f, 0.0f,
                  1.0f, 0.0f, 1.0f, 0.0f,
                  0.0f, 1.0f, 3.0f, 1.0f };
    float v[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    OCIO::GetM44V4Product(vout, m, v);

    float vcorrect[] = { 5.0f, 5.0f, 4.0f, 15.0f };

    for(int i=0; i<4; ++i)
    {
        OCIO_CHECK_EQUAL(vout[i], vcorrect[i]);
    }
}

OCIO_ADD_TEST(MathUtils, v4_add)
{
    float vout[4];
    float v1[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    float v2[] = { 3.0f, 1.0f, 4.0f, 1.0f };
    OCIO::GetV4Sum(vout, v1, v2);

    float vcorrect[] = { 4.0f, 3.0f, 7.0f, 5.0f };

    for(int i=0; i<4; ++i)
    {
        OCIO_CHECK_EQUAL(vout[i], vcorrect[i]);
    }
}

OCIO_ADD_TEST(MathUtils, mxb_eval)
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
        OCIO_CHECK_EQUAL(vout[i], vcorrect[i]);
    }
}

OCIO_ADD_TEST(MathUtils, combine_two_mxb)
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
        OCIO::GetMxbCombine(mout, vout, m1, v1, m2, v2);
        GetMxbResult(vcombined, mout, x, vout);

        // Sequentially apply the two mx+b operations.
        GetMxbResult(vout, m1, x, v1);
        GetMxbResult(vout, m2, vout, v2);

        // Compare outputs
        for(int i=0; i<4; ++i)
        {
            OCIO_CHECK_CLOSE(vcombined[i], vout[i], tolerance);
        }
    }

    {
        float x[] = { 6.0f, 0.5f, -2.0f, -0.1f };
        float vout[4];

        float mout[16];
        float vcombined[4];
        OCIO::GetMxbCombine(mout, vout, m1, v1, m2, v2);
        GetMxbResult(vcombined, mout, x, vout);

        GetMxbResult(vout, m1, x, v1);
        GetMxbResult(vout, m2, vout, v2);

        for(int i=0; i<4; ++i)
        {
            OCIO_CHECK_CLOSE(vcombined[i], vout[i], tolerance);
        }
    }

    {
        float x[] = { 26.0f, -0.5f, 0.005f, 12.1f };
        float vout[4];

        float mout[16];
        float vcombined[4];
        OCIO::GetMxbCombine(mout, vout, m1, v1, m2, v2);
        GetMxbResult(vcombined, mout, x, vout);

        GetMxbResult(vout, m1, x, v1);
        GetMxbResult(vout, m2, vout, v2);

        // We pick a not so small tolerance, as we're dealing with
        // large numbers, and the error for CHECK_CLOSE is absolute.
        for(int i=0; i<4; ++i)
        {
            OCIO_CHECK_CLOSE(vcombined[i], vout[i], 1e-3);
        }
    }
}

OCIO_ADD_TEST(MathUtils, mxb_invert)
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
        bool invertsuccess = OCIO::GetMxbInverse(mout, vout, m, v);
        OCIO_CHECK_EQUAL(invertsuccess, true);

        GetMxbResult(vresult, mout, vresult, vout);

        float tolerance = 1e-9f;
        for(int i=0; i<4; ++i)
        {
            OCIO_CHECK_CLOSE(vresult[i], x[i], tolerance);
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

        bool invertsuccess = OCIO::GetMxbInverse(mout, vout, m, v);
        OCIO_CHECK_EQUAL(invertsuccess, false);
    }
}


// Infrastructure for testing FloatsDiffer()
//
#define KEEP_DENORMS      false
#define COMPRESS_DENORMS  true

#define TEST_CHECK_MESSAGE(a, msg) if(!(a)) throw OCIO::Exception(msg.c_str())


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
  const float VAR_NAME = OCIO::AddULP(REFERENCE, ULP)

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
        << "(" << std::hex << std::showbase << OCIO::FloatAsInt(a) << std::dec << ") "
        << "and " << b << " "
        << "(" << std::hex << std::showbase << OCIO::FloatAsInt(b) << std::dec << ") "
        << "are expected to be DIFFERENT within a tolerance of " << tolerance << " ULPs "
        << ( compressDenorms ? "(when compressing denormalized numbers)."
                             : "(when keeping denormalized numbers)." );

    return oss.str();
}

void checkFloatsAreDifferent(const float ref, const int tolerance, const bool compressDenorms,
                             const float a)
{
    TEST_CHECK_MESSAGE(OCIO::FloatsDiffer(ref, a, tolerance, compressDenorms),
                       getErrorMessageDifferent(ref, a, tolerance, compressDenorms) );

    TEST_CHECK_MESSAGE(OCIO::FloatsDiffer(a, ref, tolerance, compressDenorms),
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
        << "(" << std::hex << std::showbase << OCIO::FloatAsInt(a) << std::dec << ") "
        << "and " << b << " "
        << "(" << std::hex << std::showbase << OCIO::FloatAsInt(b) << std::dec << ") "
        << "are expected to be CLOSE within a tolerance of " << tolerance << " ULPs "
        << ( compressDenorms ? "(when compressing denormalized numbers)."
                             : "(when keeping denormalized numbers)." );

    return oss.str();
}

void checkFloatsAreClose(const float ref, const int tolerance, const bool compressDenorms,
                         const float a)
{
    TEST_CHECK_MESSAGE(!OCIO::FloatsDiffer(ref, a, tolerance, compressDenorms),
                       getErrorMessageClose(ref, a, tolerance, compressDenorms) );

    TEST_CHECK_MESSAGE(!OCIO::FloatsDiffer(a, ref, tolerance, compressDenorms),
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
        << "(" << std::hex << std::showbase << OCIO::FloatAsInt(a) << std::dec << ") "
        << "and " << b << " "
        << "(" << std::hex << std::showbase << OCIO::FloatAsInt(b) << std::dec << ") "
        << "are expected to be EQUAL "
        << ( compressDenorms ? "(when compressing denormalized numbers)."
                             : "(when keeping denormalized numbers)." );

    return oss.str();
}

void checkFloatsAreEqual(const float ref, const bool compressDenorms, const float a)
{
    TEST_CHECK_MESSAGE(!OCIO::FloatsDiffer(ref, a, 0, compressDenorms),
                       getErrorMessageEqual(ref, a, compressDenorms) );

    TEST_CHECK_MESSAGE(!OCIO::FloatsDiffer(a, ref, 0, compressDenorms),
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

OCIO_ADD_TEST(MathUtils, float_diff_keep_denorms_test)
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

OCIO_ADD_TEST(MathUtils, float_diff_compress_denorms_test )
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

OCIO_ADD_TEST(MathUtils, half_bits_test)
{
    // Validation.
    OCIO_CHECK_EQUAL(0.5f, OCIO::ConvertHalfBitsToFloat(0x3800));

    // Preserve negatives.
    OCIO_CHECK_EQUAL(-1.f, OCIO::ConvertHalfBitsToFloat(0xbc00));

    // Preserve values > 1.
    OCIO_CHECK_EQUAL(1024.f, OCIO::ConvertHalfBitsToFloat(0x6400));
}

OCIO_ADD_TEST(MathUtils, halfs_differ_test)
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

    OCIO_CHECK_ASSERT(OCIO::HalfsDiffer(pos_inf, neg_inf, tol));
    OCIO_CHECK_ASSERT(OCIO::HalfsDiffer(pos_inf, pos_nan, tol));
    OCIO_CHECK_ASSERT(OCIO::HalfsDiffer(neg_inf, neg_nan, tol));
    OCIO_CHECK_ASSERT(OCIO::HalfsDiffer(pos_max, pos_inf, tol));
    OCIO_CHECK_ASSERT(OCIO::HalfsDiffer(neg_max, neg_inf, tol));
    OCIO_CHECK_ASSERT(OCIO::HalfsDiffer(pos_1, neg_1, tol));
    OCIO_CHECK_ASSERT(OCIO::HalfsDiffer(pos_2, pos_1, 0));
    OCIO_CHECK_ASSERT(OCIO::HalfsDiffer(neg_2, neg_1, 0));

    OCIO_CHECK_ASSERT(!OCIO::HalfsDiffer(pos_zero, neg_zero, 0));
    OCIO_CHECK_ASSERT(!OCIO::HalfsDiffer(pos_small, neg_small, tol));
    OCIO_CHECK_ASSERT(!OCIO::HalfsDiffer(pos_2, pos_1, tol));
    OCIO_CHECK_ASSERT(!OCIO::HalfsDiffer(neg_2, neg_1, tol));
}

OCIO_ADD_TEST(MathUtils, clamp)
{
    OCIO_CHECK_EQUAL(-1.0f, OCIO::Clamp(std::numeric_limits<float>::quiet_NaN(), -1.0f, 1.0f));

    OCIO_CHECK_EQUAL(10.0f, OCIO::Clamp( std::numeric_limits<float>::infinity(),  5.0f, 10.0f));
    OCIO_CHECK_EQUAL(5.0f, OCIO::Clamp(-std::numeric_limits<float>::infinity(),  5.0f, 10.0f));

    OCIO_CHECK_EQUAL(0.0000005f, OCIO::Clamp( 0.0000005f, 0.0f, 1.0f));
    OCIO_CHECK_EQUAL(0.0f,       OCIO::Clamp(-0.0000005f, 0.0f, 1.0f));
    OCIO_CHECK_EQUAL(1.0f,       OCIO::Clamp( 1.0000005f, 0.0f, 1.0f));
}

