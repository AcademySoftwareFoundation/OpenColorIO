// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_SSE_H
#define INCLUDED_OCIO_SSE_H


#ifdef USE_SSE


#include <emmintrin.h>
#include <stdio.h>


#include <OpenColorIO/OpenColorIO.h>



namespace OCIO_NAMESPACE
{

// Macros for alignment declarations
#define OCIO_SIMD_BYTES 16
#if defined( _MSC_VER )
#define OCIO_ALIGN(decl) __declspec(align(OCIO_SIMD_BYTES)) decl
#elif ( __APPLE__ )
// TODO: verify if this is good for clang
#define OCIO_ALIGN(decl) decl
#else
#define OCIO_ALIGN(decl) decl __attribute__((aligned(OCIO_SIMD_BYTES)))
#endif


#include <limits>

static constexpr int EXP_MASK   = 0x7F800000;
static constexpr int EXP_BIAS   = 127;
static constexpr int EXP_SHIFT  = 23;
static constexpr int SIGN_SHIFT = 31;

static const __m128i EMASK = _mm_set1_epi32(EXP_MASK);
static const __m128i EBIAS = _mm_set1_epi32(EXP_BIAS);

static const __m128 EONE    = _mm_set1_ps(1.0f);
static const __m128 EZERO   = _mm_set1_ps(0.0f);
static const __m128 ENEG126 = _mm_set1_ps(-126.0f);
static const __m128 EPOS127 = _mm_set1_ps(127.0f);

static const __m128 EPOSINF = _mm_set1_ps(std::numeric_limits<float>::infinity());

// Debug function to print out the contents of a floating-point SSE register
inline void ssePrintRegister(const char* msg, __m128& reg)
{
    float r[4];
    _mm_storeu_ps(r, reg);
    printf("%s : %f %f %f %f\n", msg, r[0], r[1], r[2], r[3]);
}

// Debug function to print out the contents of an integer SSE register
inline void ssePrintRegister(const char* msg, __m128i& reg)
{
    int *r = (int*) &reg;
    printf("%s : %d %d %d %d\n", msg, r[0], r[1], r[2], r[3]);
}

// Determine whether a floating-point value is negative based on its sign bit.
// This function will treat special values, like -0, -NaN, -Inf, as they were indeed
// negative values.
inline __m128 isNegativeSpecial(const __m128 x)
{
  return _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(x), SIGN_SHIFT));
}

// Select function in SSE version 2
//
// Return the parameter arg_false when the parameter mask is 0x0,
// or the parameter arg_true when the mask is 0xffffffff.
//
// Algorithm Explanation:
//
// Uses the identities:
//   x XOR 0 = x
//   x XOR x = 0
//
// select = ((arg_true XOR arg_false) AND mask) XOR arg_false
//
// When mask = 0, the expression evaluates to:
//   = ((arg_true XOR arg_false) AND 0x0) XOR arg_false
//   = 0x0 XOR arg_false
//   = arg_false
//
// When mask = 0xffffffff, the expression evaluates to:
//   = ((arg_true XOR arg_false) AND 0xffffffff) XOR arg_false
//   = arg_true XOR a XOR arg_false
//   = arg_true XOR 0
//   = arg_true
//
// Note:
//   This version is better than the common alternative:
//     (arg_true & mask) | (arg_false & ~mask)
//
//   because it requires only one SEE register while the common
//   solution requires two registers.
//
inline __m128 sseSelect(const __m128& mask, const __m128& arg_true, const __m128& arg_false)
{
    return _mm_xor_ps( arg_false, _mm_and_ps( mask, _mm_xor_ps( arg_true, arg_false ) ) );
}

// Coefficients of Chebyshev (minimax) degree 5 polynomial
// approximation to log2() over the range [1.0, 2.0[.
static const __m128 PNLOG5 = _mm_set1_ps((float)+4.487361286440374006195e-2);
static const __m128 PNLOG4 = _mm_set1_ps((float)-4.165637071209677112635e-1);
static const __m128 PNLOG3 = _mm_set1_ps((float)+1.631148826119436277100);
static const __m128 PNLOG2 = _mm_set1_ps((float)-3.550793018041176193407);
static const __m128 PNLOG1 = _mm_set1_ps((float)+5.091710879305474367557);
static const __m128 PNLOG0 = _mm_set1_ps((float)-2.800364054395965731506);

// Coefficients of Chebyshev (minimax) degree 4 polynomial
// approximation to exp2() over the range [0.0, 1.0[.
static const __m128 PNEXP4 = _mm_set1_ps((float)1.353416792833547468620e-2);
static const __m128 PNEXP3 = _mm_set1_ps((float)5.201146058412685018921e-2);
static const __m128 PNEXP2 = _mm_set1_ps((float)2.414427569091865207710e-1);
static const __m128 PNEXP1 = _mm_set1_ps((float)6.930038344665415134202e-1);
static const __m128 PNEXP0 = _mm_set1_ps((float)1.000002593370603213644);

// log2 function in SSE version 2
//
// The function log2() is evaluated by performing argument
// reduction and then using Chebyshev polynomials to evaluate the function 
// over a restricted range.
inline __m128 sseLog2(__m128 x)
{
    // y = log2( x ) = log2( 2^exposant * mantissa ) 
    //               = exposant + log2( mantissa )

    __m128 mantissa
        = _mm_or_ps(
            _mm_andnot_ps(_mm_castsi128_ps(EMASK), x), EONE);

    __m128 log2
        = _mm_add_ps(
            _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_add_ps(
                            _mm_mul_ps(
                                _mm_add_ps(
                                    _mm_mul_ps(
                                        _mm_add_ps(
                                            _mm_mul_ps(PNLOG5, mantissa),
                                            PNLOG4),
                                        mantissa),
                                    PNLOG3),
                                mantissa),
                            PNLOG2),
                        mantissa),
                    PNLOG1),
                mantissa),
            PNLOG0);

    __m128i exponent
        = _mm_sub_epi32(
            _mm_srli_epi32(
                _mm_and_si128(_mm_castps_si128(x),
                    EMASK),
                EXP_SHIFT),
            EBIAS);

    log2 = _mm_add_ps(log2, _mm_cvtepi32_ps(exponent));

    return log2;
}

// exp2 function in SSE version 2
//
// The function exp2() is evaluated by performing argument
// reduction and then using Chebyshev polynomials to evaluate the function 
// over a restricted range.
inline __m128 sseExp2(__m128 x)
{
    // y = exp2( x ) = exp2(integer + fraction) 
    //               = exp2(integer) * exp2(fraction) 
    //               = zf * mexp

    // Compute the largest integer not greater than x, i.e., floor(x)
    // Note: cvttps_epi32 simply cast the float value to int. That means cvttps_epi32(-2.7) = -2
    // rather than -3, hence for negative numbers we need to add -1. This ensures that "fraction"
    // is always in the range [0, 1).
    __m128i floor_x
        = _mm_add_epi32(
            _mm_cvttps_epi32(x),
            _mm_castps_si128(
                _mm_cmpnle_ps(EZERO, x)));

    // Compute exp2(floor_x) by moving floor_x to the exponent bits of the floating-point number.
    __m128 zf
        = _mm_castsi128_ps(
            _mm_slli_epi32(
                _mm_add_epi32(floor_x, EBIAS),
                EXP_SHIFT));

    __m128 iexp = _mm_cvtepi32_ps(floor_x);
    __m128 fraction = _mm_sub_ps(x, iexp);

    // Compute exp2(fraction) using a polynomial approximation
    __m128 mexp
        = _mm_add_ps(
            _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_add_ps(
                            _mm_mul_ps(
                                _mm_add_ps(
                                    _mm_mul_ps(PNEXP4, fraction),
                                    PNEXP3),
                                fraction),
                            PNEXP2),
                        fraction),
                    PNEXP1),
                fraction),
            PNEXP0);

    __m128 exp2 = _mm_mul_ps(zf, mexp);

    // Handle underflow:
    // If the (unbiased) exponent of zf is less than -126, the result is smaller than
    // the smallest representable floating-point number and an underflow computation is
    // potentially happening. When this happens, force the result to zero.
    exp2 = _mm_andnot_ps(_mm_cmplt_ps(iexp, ENEG126), exp2);

    // Handle overflow:
    // If the (unbiased) exponent of zf is greater than 127, the result is larger than
    // the largest representable floating-point number and an overflow computation is
    // potentially happening. When this happens, force the result to positive infinity.
    exp2 = sseSelect(_mm_cmpgt_ps(iexp, EPOS127), EPOSINF, exp2);

    return exp2;
}

// Power function in SSE version 2
//
// Algorithm Explanation:
//
// The pow() function is evaluated using the following mathematical 
//   identity:
//
// pow( x, exp ) = exp2( exp * log2( x ) )
//
// The functions exp2() and log2() are evaluated by performing argument
// reduction and then using Chebyshev polynomials to evaluate the function 
// over a restricted range. The polynomials have been chosen to achieve 
// a precision of approximately 15 bits of mantissa.
//
// Results from base values smaller than zero are mapped to zero.
// TODO: The toxik module photoLabProc.cpp has some interesting comments related to SSE and a
//       version that is more highly optimized than what we use here. Should investigate if we
//       can speed things up beyond just using the polynomial approximation.
inline __m128 ssePower(__m128 x, __m128 exp)
{
    __m128 values = sseLog2(x);

    values = _mm_mul_ps(exp, values);

    values = sseExp2(values);

    // Handle values where base is smaller or equal than zero
    values = _mm_and_ps(values, _mm_cmpgt_ps(x, EZERO));

    return values;
}

static const __m128 ESIGN_MASK = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
static const __m128 EABS_MASK  = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));

static const __m128 E_PI        = _mm_set1_ps( (float) 3.14159265358979323846 );
static const __m128 E_PI_2      = _mm_set1_ps( (float) 1.57079632679489661923 );

// Arc tangent function in SSE version 2
//
// Algorithm Explanation:
//
// The function atan() is evaluated by reducing the argument domain to the
// range [0,1] using the identities:
//   atan(x) = PI/2 - atan(1/x)
//   atan(x) = -atan(-x)
//
// and then using a rational polynomial approximation to evaluate the function 
// over the reduced domain:
//
//                 a1*x + a2*x^3 + a3*x^5
//   atan(x)  ~  --------------------------
//               b1 + b2*x^2 + b3*x^4 + x^6
//
// The result is then adjusted according to the identities applied during the
// argument reduction. This approximation is accurate to 14 bits of mantissa.
// TODO: Investigate the need for higher accuracy for atan.
// It is possible to further reduce the argument from [0,1] to [0, pi/12]
// which would provide accurate results up to 20 bits of mantissa.
inline __m128 sseAtan(const __m128 x)
{
    // Rational polynomial coefficients for the arc tangent approximation.
    // Original source: http://www.ganssle.com/approx/approx.pdf
    static const __m128 PN_ATAN_A1 = _mm_set1_ps((float) 48.70107004404898384);
    static const __m128 PN_ATAN_A2 = _mm_set1_ps((float) 49.5326263772254345);
    static const __m128 PN_ATAN_A3 = _mm_set1_ps((float)  9.40604244231624);
    static const __m128 PN_ATAN_B1 = _mm_set1_ps((float) 48.70107004404996166);
    static const __m128 PN_ATAN_B2 = _mm_set1_ps((float) 65.7663163908956299);
    static const __m128 PN_ATAN_B3 = _mm_set1_ps((float) 21.587934067020262);

    // Apply identity atan(x) = -atan(-x) to reduce domain to [0, Inf)
    __m128 sign_x = _mm_and_ps(x, ESIGN_MASK);
    __m128 abs_x = _mm_and_ps(x, EABS_MASK);

    // Apply identity atan(x) = PI/2 - atan(1/x) to reduce domain to [0,1]
    __m128 inv_mask = _mm_cmpgt_ps(abs_x, EONE);
    __m128 inv_abs_x = _mm_div_ps(EONE, abs_x);
    __m128 norm_x = sseSelect(inv_mask, inv_abs_x, abs_x);

    // compute atan using a normalized input
    __m128 norm_x2 = _mm_mul_ps(norm_x, norm_x);

    __m128 num =
        _mm_mul_ps(
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_add_ps(
                        _mm_mul_ps(norm_x2, PN_ATAN_A3),
                        PN_ATAN_A2),
                    norm_x2),
                PN_ATAN_A1),
            norm_x);

    __m128 denom =
        _mm_add_ps(
            _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_add_ps(norm_x2, PN_ATAN_B3),
                        norm_x2),
                    PN_ATAN_B2),
                norm_x2),
            PN_ATAN_B1);

    __m128 res = _mm_div_ps(num, denom);

    // If the input was inverted during domain reduction,
    // correct the result by subtracting it from PI/2.
    res = sseSelect(inv_mask, _mm_sub_ps(E_PI_2, res), res);

    // If the input was negated during domain reduction,
    // correct the result by negating it again.
    return _mm_or_ps(sign_x, res);
}

// Scalar version of sseAtan
inline float sseAtan(const float v)
{
    // Rational polynomial coefficients for the arc tangent approximation.
    // Original source: http://www.ganssle.com/approx/approx.pdf
    static const float PN_ATAN_A1 = (float) 48.70107004404898384;
    static const float PN_ATAN_A2 = (float) 49.5326263772254345;
    static const float PN_ATAN_A3 = (float)  9.40604244231624;
    static const float PN_ATAN_B1 = (float) 48.70107004404996166;
    static const float PN_ATAN_B2 = (float) 65.7663163908956299;
    static const float PN_ATAN_B3 = (float) 21.587934067020262;

    static const float F_PI_2 = (float) 1.57079632679489661923;

    int inv = false;
    int neg = false;
    float x = v;

    // Apply identity atan(x) = -atan(-x) to reduce domain to [0, Inf)
    if (x < 0.0f)
    {
        x = -x;
        neg = true;
    }

    // Apply identity atan(x) = PI/2 - atan(1/x) to reduce domain to [0,1]
    if (x > 1.0f)
    {
        x = 1.0f / x;
        inv = true;
    }

    // compute atan using a normalized input
    float x2 = x*x;

    float num = x*(PN_ATAN_A1 + x2*(PN_ATAN_A2 + x2*PN_ATAN_A3));
    float denom = PN_ATAN_B1 + x2*(PN_ATAN_B2 + x2*(x2 + PN_ATAN_B3));
    float res = num / denom;

    // If the input was inverted during domain reduction,
    // correct the result by subtracting it from PI/2.
    if (inv)
    {
        res = F_PI_2 - res;
    }

    // If the input was negated during domain reduction,
    // correct the result by negating it again.
    if (neg)
    {
        res = -res;
    }

    return res;
}

// Arc tangent function of two variables in SSE version 2
//
// Algorithm Explanation:
//
// The function atan2() is evaluated by first calling atan(), which
// yields the arc tangent in [-pi/2, pi/2] radians, and then adjusting
// the result based on the signs of the arguments to return the arc tangent
// in the range [-pi,pi].
//
// Considering the quadrants defined as:
//   Quadrant 1 : positive y, positive x
//   Quadrant 2 : positive y, negative x
//   Quadrant 3 : negative y, negative x
//   Quadrant 4 : negative y, positive x
//
// The atan() function implicitly maps the results from quadrants 2 and 3 to
// quadrants 4 and 1, respectively. The results from these quadrants need to be
// adjusted.
inline __m128 sseAtan2(const __m128 y,  const __m128 x)
{
    __m128 res = sseAtan(_mm_div_ps(y, x));

    // Fix for x=0 and y=0
    __m128 zero_mask = _mm_or_ps(_mm_cmpneq_ps(x, EZERO), _mm_cmpneq_ps(y, EZERO));
    res = _mm_and_ps(res, zero_mask);

    // Adjust quadrants 2 and 3 based on the sign of the arguments
    __m128 neg_x = isNegativeSpecial(x);
    __m128 sign_y = _mm_and_ps(y, ESIGN_MASK);

    return _mm_add_ps(res, _mm_and_ps(_mm_or_ps(sign_y, E_PI), neg_x));
}

// Scalar version of sseAtan2
inline float sseAtan2(const float y, const float x)
{
    static const unsigned SIGN_MASK = 0x80000000;
    static const float F_PI = (float) 3.14159265358979323846;

    // Fix for x=0 and y=0
    float res = ((x == 0.0f) && (y == 0.0f)) ? 0.0f : sseAtan(y / x);

    const unsigned* piy = (const unsigned*)&y;
    const unsigned* pix = (const unsigned*)&x;
    // Adjust quadrants 2 and 3 based on the sign of the arguments
    const unsigned iy = *piy;
    const unsigned ix = *pix;
    const bool neg_x = (ix & SIGN_MASK) != 0;
    const bool neg_y = (iy & SIGN_MASK) != 0;

    if (neg_x)
    {
        res = res + (neg_y ? -F_PI : F_PI);
    }
    return res;
}

static const __m128 E_1_PI = _mm_set1_ps( (float) 0.31830988618379067153776752674503 );

// Helper function that computes the cosine of an angle, and that also returns some
//        intermediate results. This function is shared between sseCos and sseSinCos functions.
// [in] x                the input angle
// [out] cos_x           the cosine of the angle
// [out] xr              input angle reduced to the domain [-pi/2, pi/2]
// [out] xr2             reduced input angle squared
// [out] flip_sign_cos_x sign mask indicating that the cosine was flipped because the
//                       original angle was on quadrants 2 or 3.
inline void __sse_cos__(const __m128 x, __m128& cos_x, __m128& xr, __m128& xr2, __m128& flip_sign_cos_x)
{
    // Chebyshev polynomial coefficients for the cosine approximation.
    // Original source: http://www.ganssle.com/approx/approx.pdf
    static const __m128 PN_COS_C1 = _mm_set1_ps((float)  0.999999953464);
    static const __m128 PN_COS_C2 = _mm_set1_ps((float)-0.499999053455);
    static const __m128 PN_COS_C3 = _mm_set1_ps((float)  0.0416635846769);
    static const __m128 PN_COS_C4 = _mm_set1_ps((float)-0.0013853704264);
    static const __m128 PN_COS_C5 = _mm_set1_ps((float)  0.00002315393167);

    // Reduce to [-pi/2, pi/2]
    __m128i cycles = _mm_cvtps_epi32(_mm_mul_ps(x, E_1_PI));

    xr = _mm_sub_ps(x, _mm_mul_ps(_mm_cvtepi32_ps(cycles), E_PI));
    xr2 = _mm_mul_ps(xr, xr);

    cos_x =
        _mm_add_ps(
            _mm_mul_ps(
                _mm_add_ps(
                    _mm_mul_ps(
                        _mm_add_ps(
                            _mm_mul_ps(
                                _mm_add_ps(
                                    _mm_mul_ps(PN_COS_C5, xr2),
                                    PN_COS_C4),
                                xr2),
                            PN_COS_C3),
                        xr2),
                    PN_COS_C2),
                xr2),
            PN_COS_C1);

    // If cycles is odd, then the angle is in either quadrant 2 or 3.
    // In these case we need to invert the sign of the result.
    flip_sign_cos_x = _mm_castsi128_ps(_mm_slli_epi32(cycles, 31));
    cos_x = _mm_xor_ps(cos_x, flip_sign_cos_x);
}

// Cosine function
//
// Algorithm Explanation:
// The function cos() is evaluated by reducing the argument to the domain [-pi/2,pi/2]
// and then using Chebyshev polynomials to evaluate the function over the restricted
// range. The sign of the result is then adjusted if the (reduced) input value is on
// quadrants 2 or 3. This approximation is accurate to 17 bits of mantissa.
// - x the input angle
// Return the cosine of the angle
inline __m128 sseCos(const __m128 x)
{
    __m128 cos_x, xr, xr2, flip_sign_cos_x;
    __sse_cos__(x, cos_x, xr, xr2, flip_sign_cos_x);
    return cos_x;
}

// Sine function
//
// The function sin() is evaluated by phasing the argument, and then computing
// its cosine. It is based on the identity:
//   sin(x) = cos(pi/2-x)
// - x the input angle
// Return the sine of the angle
inline __m128 sseSin(const __m128 x)
{
    return sseCos(_mm_sub_ps(E_PI_2, x));
}

// Sine and Cosine function
//
// The function sincos() is evaluated by computing the cosine of the angle, then computing 
// the sine based on the cosine using a simplified formula. The sign of the sine as well as
// a better approximation for the sine when the input angle is close to zero is done using
// some of the temporary values determined during the cosine computation.
// [in] x the input angle
// [out] sin_x the sine of the angle
// [out] cos_x the cosine of the angle
inline void sseSinCos(const __m128 x, __m128& sin_x, __m128& cos_x)
{
    // Using a threshold of 2^-7 for the reduced angle seems to provide
    // a fairly decent precision (16 bits) to the final result.
    static const __m128 SINE_THRESHOLD_SQUARED = _mm_set1_ps( (float) 0.00006103515625 );

    __m128 xr, xr2, flip_sign_cos_x;
    __sse_cos__(x, cos_x, xr, xr2, flip_sign_cos_x);

    // When cos(x) becomes too close to 1, the sin(x) evaluation contains too
    // much error. However, in this case, sin(x) ~ x, and we can use xr to 
    // approximate sin(x) instead.
    __m128 sin_x2 = _mm_sub_ps(EONE, _mm_mul_ps(cos_x, cos_x));
    sin_x2 = sseSelect(_mm_cmpgt_ps(xr2, SINE_THRESHOLD_SQUARED), sin_x2, xr2);
    sin_x = _mm_sqrt_ps(sin_x2);

    // Flip the sign of sin(x) if the angle was in quadrants 3 or 4.
    __m128 xr_sign = _mm_and_ps(xr, ESIGN_MASK);
    __m128 flip_sign_sin_x = _mm_xor_ps(flip_sign_cos_x, xr_sign);
    sin_x = _mm_xor_ps(sin_x, flip_sign_sin_x);
}

// Scalar version of sseSinCos
inline void sseSinCos(const float x, float& sin_x, float& cos_x)
{
    static const float F_PI_2 = (float) 1.57079632679489661923;

    __m128 sc = _mm_setr_ps(x, F_PI_2-x, 0, 0);
    __m128 res = sseCos(sc);

    OCIO_ALIGN(float buf[4]);
    _mm_store_ps(buf, res);

    cos_x = buf[0];
    sin_x = buf[1];
}

} // namespace OCIO_NAMESPACE


#endif


#endif
