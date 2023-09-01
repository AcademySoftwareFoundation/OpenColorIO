// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#if OCIO_USE_SSE2


#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "SSE.h"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(SSE, sse2_log2_test)
{
    const float values[8] = { 1e-010f, .1f, .5f, 1.f,
                              11.f, 112.f, 2425.f, 2e015f };

    // The sse approx should have about 15 good digits of mantissa
    const float rtol = powf(2.f, -14.f);

    float cpuResult;
    float sseResult[4];
    __m128 mm_sseResult;

    for (unsigned i = 0; i < 8; ++i)
    {
        cpuResult = logf(values[i]) / logf(2.f);

        mm_sseResult = OCIO::sseLog2(_mm_set1_ps(values[i]));
        _mm_storeu_ps(sseResult, mm_sseResult);

        OCIO_CHECK_CLOSE(cpuResult, sseResult[0], rtol);
    }
}

namespace
{

std::string GetErrorMessage(const std::string & operation, float expected, float actual)
{
    std::ostringstream oss;
    oss << "Output differs on " << operation << " : "
        << "expected: " << expected << " != " << "actual: " << actual;
    return oss.str();
}

bool IsInfinity(float floatToTest)
{
    const float posinf = std::numeric_limits<float>::infinity();
    const float neginf = -std::numeric_limits<float>::infinity();
    return (posinf == floatToTest
        || neginf == floatToTest);
}

void CheckFloat(const std::string& operation,
                const float expected,
                const float actual,
                const unsigned precision)
{

    if ((IsInfinity(expected) && IsInfinity(actual)) ||
        (OCIO::IsNan(expected) && OCIO::IsNan(actual)))
    {
        return;
    }

    const float rtol = powf(2.f, -((float)precision));
    OCIO_CHECK_ASSERT_MESSAGE(OCIO::EqualWithAbsError(expected, actual, rtol),
                              GetErrorMessage(operation, expected, actual));
}

void CheckSSE(const std::string & operation,
              float expected,
              float sseResult[4],
              unsigned precision)
{
    CheckFloat(operation, expected, sseResult[0], precision);
    CheckFloat(operation, expected, sseResult[1], precision);
    CheckFloat(operation, expected, sseResult[2], precision);
    CheckFloat(operation, expected, sseResult[3], precision);
}

void CheckPower(const float base, const float exponent)
{
    const float cpuResult = powf(base, exponent);

    __m128 mm_sseResult = OCIO::ssePower(_mm_set1_ps(base), _mm_set1_ps(exponent));

    float sseResult[4];
    _mm_storeu_ps(sseResult, mm_sseResult);

    std::ostringstream oss;
    oss << "power(" << base << " , " << exponent << ")";
    std::string operation = oss.str();

    CheckSSE(operation, cpuResult, sseResult, 12);
}

} // anon.

OCIO_ADD_TEST(SSE, sse2_power_test)
{
    const float values[] = {
        1e-010f, .1f, .5f, 1.f,
        .7f, .112f, .2425f, .3f
    };
    const unsigned num_values = sizeof(values) / sizeof(float);

    for (unsigned i = 0; i < num_values; ++i)
    {
        CheckPower(values[i], 10.f);
    }
}

namespace
{

unsigned GetULPDifference(const float a, const float b)
{
    return abs((int)(OCIO::FloatAsInt(a) - OCIO::FloatAsInt(b)));
}

void EvaluateExp2(const float x, float* result)
{
    __m128 mm_sseResult = OCIO::sseExp2(_mm_set1_ps(x));
    _mm_storeu_ps(result, mm_sseResult);
}

bool AreAllInfinity(const float* sseResult)
{
    return IsInfinity(sseResult[0]) &&
           IsInfinity(sseResult[1]) &&
           IsInfinity(sseResult[2]) &&
           IsInfinity(sseResult[3]);
}

bool AreAllZero(const float* sseResult)
{
    return (sseResult[0] == 0.0f) &&
           (sseResult[1] == 0.0f) &&
           (sseResult[2] == 0.0f) &&
           (sseResult[3] == 0.0f);
}

bool AreAllInRange(const float* sseResult, const float lower_bound, const float upper_bound)
{
    return ((sseResult[0] > lower_bound) && (sseResult[0] < upper_bound)) &&
           ((sseResult[1] > lower_bound) && (sseResult[1] < upper_bound)) &&
           ((sseResult[2] > lower_bound) && (sseResult[2] < upper_bound)) &&
           ((sseResult[3] > lower_bound) && (sseResult[3] < upper_bound));
}

bool AreAllClose(const float* sseResult, const float reference, const unsigned ulp_tolerance)
{
    for (unsigned i = 0; i < 4; ++i)
    {
        if (GetULPDifference(sseResult[i], reference) > ulp_tolerance)
        {
            return false;
        }
    }
    return true;
}

std::string GetOperation(const char* fct, const float arg1)
{
    std::ostringstream oss;
    oss << fct << "(" << arg1 << ")";
    return oss.str();
}

std::string GetOperation(const char* fct, const float arg1, const float arg2)
{
    std::ostringstream oss;
    oss << fct << "(" << arg1 << " , " << arg2 << ")";
    return oss.str();
}

std::string GetErrorMessage(const std::string& operation, const float expected, const float* actual)
{
    std::ostringstream oss;
    oss << "Output differs on " << operation << " : " << "result: [ "
        << actual[0] << " , " << actual[1] << " , " << actual[2] << " , " << actual[3]
        << " ], expected: " << expected;
    return oss.str();
}

} // anon.

OCIO_ADD_TEST(SSE, sse2_exp2_test)
{
    const unsigned ulp_tolerance = 50;

    const float values[] = {
           1e-5f,  1e-10f,  1e-15f,   1e-20f,
          0.005f,    0.1f,    0.5f,     1.0f,
           0.67f,  0.112f, 0.2425f,    0.33f,
            1.5f,    3.2f,   7.11f,   13.23f,
         27.001f, 32.513f, 44.999f,  56.191f,
        61.0019f,   77.7f, 83.654f,  98.989f
    };
    const unsigned num_values = sizeof(values) / sizeof(float);

    float sseResult[4];

    // Check positive test values
    for (unsigned i = 0; i < num_values; ++i)
    {
        const float expected = powf(2.0f, values[i]);

        EvaluateExp2(values[i], sseResult);
        OCIO_CHECK_ASSERT_MESSAGE(AreAllClose(sseResult, expected, ulp_tolerance),
                                  GetErrorMessage(GetOperation("exp2", values[i]), expected, sseResult));
    }

    // Check negative test values
    for (unsigned i = 0; i < num_values; ++i)
    {
        const float expected = powf(2.0f, -values[i]);

        EvaluateExp2(-values[i], sseResult);
        OCIO_CHECK_ASSERT_MESSAGE(AreAllClose(sseResult, expected, ulp_tolerance),
                                  GetErrorMessage(GetOperation("exp2", -values[i]), expected, sseResult));
    }

    //
    // Check for edge cases
    //

    // log2_max_float should be exactly 128.0f
    const float log2_max_float = (float)(log((double)std::numeric_limits<float>::max()) / log(2.0));

    // log2_min_float should be exactly -126.0f
    const float log2_min_float = (float)(log((double)std::numeric_limits<float>::min()) / log(2.0));

    // Check the log2_max_float and log2_min_float limits
    {
        EvaluateExp2(log2_max_float, sseResult);
        OCIO_CHECK_ASSERT(AreAllInfinity(sseResult));

        EvaluateExp2(log2_min_float, sseResult);
        OCIO_CHECK_ASSERT(AreAllZero(sseResult));
    }

    // The valid domain of exp2 is actually reduced by one ULP
    // Verify that the log2_max_float and log2_min_float limits, contracted by one ULP,
    // return valid representable floating-point numbers.
    //
    // Note: We want log2_min_float_inside_one_ulp to be -125.9999..., but since addULP ignores the sign and
    // just modifies the mantissa, we actually need to subtract one.
    const float log2_max_float_inside_one_ulp = OCIO::AddULP(log2_max_float, -1);
    const float log2_min_float_inside_one_ulp = OCIO::AddULP(log2_min_float, -1);
    {
        // The result should be a large number, but not infinity
        // Create a tight bound for the large number based on the log2_max_float limit
        const float large_threshold = (float)pow(2.0, (double)OCIO::AddULP(log2_max_float, -2));

        EvaluateExp2(log2_max_float_inside_one_ulp, sseResult);
        OCIO_CHECK_ASSERT(AreAllInRange(sseResult, large_threshold,
                                        std::numeric_limits<float>::infinity()));

        // The result should be a small number, but not zero
        // Create a tight bound for the small number based on the log2_min_float limit
        const float small_threshold = (float)pow(2.0, (double)OCIO::AddULP(log2_min_float, -2));

        EvaluateExp2(log2_min_float_inside_one_ulp, sseResult);
        OCIO_CHECK_ASSERT(AreAllInRange(sseResult, 0.0f, small_threshold));
    }

    // Verify that the log2_max_float and log2_min_float limits, expanded by one ULP,
    // still return Infinity and zero, respectively.
    //
    // Note: As above, it is perhaps counter-intuitive, but we want to make
    // log2_min_float_outside_one_ulp just slightly more negative than -126 and
    // so need to increment the mantissa.
    const float log2_max_float_outside_one_ulp = OCIO::AddULP(log2_max_float, 1);
    const float log2_min_float_outside_one_ulp = OCIO::AddULP(log2_min_float, 1);
    {
        EvaluateExp2(log2_max_float_outside_one_ulp, sseResult);
        OCIO_CHECK_ASSERT(AreAllInfinity(sseResult));

        EvaluateExp2(log2_min_float_outside_one_ulp, sseResult);
        OCIO_CHECK_ASSERT(AreAllZero(sseResult));
    }
}

void EvaluateAtan(const float x, float* result)
{
    __m128 mm_sseResult = OCIO::sseAtan(_mm_set1_ps(x));
    _mm_storeu_ps(result, mm_sseResult);
}

OCIO_ADD_TEST(SSE, sse2_atan_test)
{
    const float sign_values[] = { -1.0f, 1.0f };

    const float tan_pi_3  = (float) 1.7320508075688772935274463415059;
    const float tan_pi_4  = (float) 1.0;
    const float tan_pi_6  = (float) 0.57735026918962576450914878050196;
    const float tan_pi_12 = (float) 0.26794919243112270647255365849413;

    const float values[] = {
            0.0f,   1e-20f,   1e-10f,     1e-5f,
          0.005f,     0.1f,     0.5f,      1.0f,
        tan_pi_3, tan_pi_4, tan_pi_6, tan_pi_12,
            1.5f,     3.2f,    7.11f,    13.23f,
         27.001f,  32.513f,  44.999f,   56.191f,
        61.0019f,    77.7f,  83.654f,   98.989f
    };
    const unsigned num_values = sizeof(values) / sizeof(float);

    float sseResult[4];

    for (unsigned si = 0; si < 2; ++si)
    {
        for (unsigned i = 0; i < num_values; ++i)
        {
            const float x = sign_values[si] * values[i];

            const float expected = atanf(x);
            EvaluateAtan(x, sseResult);

            const std::string operation = GetOperation("atan", x);

            CheckSSE(operation, expected, sseResult, 14);
        }
    }
}

OCIO_ADD_TEST(SSE, scalar_atan_test)
{
    const float sign_values[] = { -1.0f, 1.0f };

    const float tan_pi_3  = (float) 1.7320508075688772935274463415059;
    const float tan_pi_4  = (float) 1.0;
    const float tan_pi_6  = (float) 0.57735026918962576450914878050196;
    const float tan_pi_12 = (float) 0.26794919243112270647255365849413;

    const float values[] = {
            0.0f,   1e-20f,   1e-10f,     1e-5f,
          0.005f,     0.1f,     0.5f,      1.0f,
        tan_pi_3, tan_pi_4, tan_pi_6, tan_pi_12,
            1.5f,     3.2f,    7.11f,    13.23f,
         27.001f,  32.513f,  44.999f,   56.191f,
        61.0019f,    77.7f,  83.654f,   98.989f
    };
    const unsigned num_values = sizeof(values) / sizeof(float);

    for (unsigned si = 0; si < 2; ++si)
    {
        for (unsigned i = 0; i < num_values; ++i)
        {
            const float x = sign_values[si] * values[i];

            const float expected = atanf(x);
            const float result = OCIO::sseAtan(x);

            std::string operation = GetOperation("atan", x);

            CheckFloat(operation, expected, result, 14);
        }
    }
}

void EvaluateAtan2(const float y, const float x, float* result)
{
    __m128 mm_sseResult = OCIO::sseAtan2(_mm_set1_ps(y), _mm_set1_ps(x));
    _mm_storeu_ps(result, mm_sseResult);
}

OCIO_ADD_TEST(SSE, sse2_atan2_test)
{
    const float sign_values[] = { -1.0f, 1.0f };

    const float tan_pi_3  = (float) 1.7320508075688772935274463415059;
    const float tan_pi_4  = (float) 1.0;
    const float tan_pi_6  = (float) 0.57735026918962576450914878050196;
    const float tan_pi_12 = (float) 0.26794919243112270647255365849413;

    const float values_x[] = {
            0.0f,   1e-20f,   1e-15f,    1e-10f,
          0.005f,     0.1f,     0.5f,      1.0f,
        tan_pi_3, tan_pi_4, tan_pi_6, tan_pi_12,
            1.5f,     3.2f,    7.11f,    13.23f,
         27.001f,  32.513f,  44.999f,   56.191f,
        61.0019f,    77.7f,  83.654f,   98.989f
    };
    const unsigned num_values_x = sizeof(values_x) / sizeof(float);

    const float values_y[] = {
            0.0f,   1e-20f,   1e-15f,    1e-10f,
          0.005f,     0.1f,     0.5f,      1.0f,
        tan_pi_3, tan_pi_4, tan_pi_6, tan_pi_12,
            1.5f,     3.2f,    7.11f,    13.23f,
         27.001f,  32.513f,  44.999f,   56.191f,
        61.0019f,    77.7f,  83.654f,   98.989f
    };
    const unsigned num_values_y = sizeof(values_y) / sizeof(float);

    float sseResult[4];

    for (unsigned si = 0; si < 2; ++si)
    {
        for (unsigned i = 0; i < num_values_x; ++i)
        {
            const float x = sign_values[si] * values_x[i];

            for (unsigned sj = 0; sj < 2; ++sj)
            {
                for (unsigned j = 0; j < num_values_y; ++j)
                {
                    const float y = sign_values[sj] * values_y[j];

                    const float expected = atan2f(y, x);
                    EvaluateAtan2(y, x, sseResult);

                    const std::string operation = GetOperation("atan2", y, x);

                    CheckSSE(operation, expected, sseResult, 14);
                }
            }
        }
    }
}

OCIO_ADD_TEST(SSE, scalar_atan2_test)
{
    const float sign_values[] = { -1.0f, 1.0f };

    const float tan_pi_3  = (float) 1.7320508075688772935274463415059;
    const float tan_pi_4  = (float) 1.0;
    const float tan_pi_6  = (float) 0.57735026918962576450914878050196;
    const float tan_pi_12 = (float) 0.26794919243112270647255365849413;

    const float values_x[] = {
            0.0f,   1e-20f,   1e-15f,    1e-10f,
          0.005f,     0.1f,     0.5f,      1.0f,
        tan_pi_3, tan_pi_4, tan_pi_6, tan_pi_12,
            1.5f,     3.2f,    7.11f,    13.23f,
         27.001f,  32.513f,  44.999f,   56.191f,
        61.0019f,    77.7f,  83.654f,   98.989f
    };
    const unsigned num_values_x = sizeof(values_x) / sizeof(float);

    const float values_y[] = {
            0.0f,   1e-20f,   1e-15f,    1e-10f,
          0.005f,     0.1f,     0.5f,      1.0f,
        tan_pi_3, tan_pi_4, tan_pi_6, tan_pi_12,
            1.5f,     3.2f,    7.11f,    13.23f,
         27.001f,  32.513f,  44.999f,   56.191f,
        61.0019f,    77.7f,  83.654f,   98.989f
    };
    const unsigned num_values_y = sizeof(values_y) / sizeof(float);

    for (unsigned si = 0; si < 2; ++si)
    {
        for (unsigned i = 0; i < num_values_x; ++i)
        {
            const float x = sign_values[si] * values_x[i];

            for (unsigned sj = 0; sj < 2; ++sj)
            {
                for (unsigned j = 0; j < num_values_y; ++j)
                {
                    const float y = sign_values[sj] * values_y[j];

                    const float expected = atan2f(y, x);
                    const float result = OCIO::sseAtan2(y, x);

                    const std::string operation = GetOperation("atan2", y, x);

                    CheckFloat(operation, expected, result, 14);
                }
            }
        }
    }
}

void EvaluateCos(const float x, float* result)
{
    __m128 mm_sseResult = OCIO::sseCos(_mm_set1_ps(x));
    _mm_storeu_ps(result, mm_sseResult);
}

OCIO_ADD_TEST(SSE, sse2_cos_test)
{
    const float sign_values[] = { -1.0f, 1.0f };

    const float three_pi = (float) 9.4247779607693797153879301498385;
    const float two_pi   = (float) 6.283185307179586476925286766559;
    const float pi       = (float) 3.1415926535897932384626433832795;
    const float pi_2     = (float) 1.5707963267948966192313216916398;
    const float pi_3     = (float) 1.0471975511965977461542144610932;
    const float pi_4     = (float) 0.78539816339744830961566084581988;
    const float pi_6     = (float) 0.52359877559829887307710723054658;
    const float pi_12    = (float) 0.26179938779914943653855361527329;

    const float values[] = {
            0.0f,   1e-20f,   1e-10f,     1e-5f,
          0.005f,     0.1f,     0.5f,      1.0f,
           0.67f,   0.112f,  0.2425f,     0.33f,
              pi,     pi_2,     pi_3,      pi_4,
            pi_6,    pi_12,   two_pi,  three_pi,
         27.001f,  32.513f,  44.999f,   56.191f,
        61.0019f,    77.7f,  83.654f,   98.989f
    };
    const unsigned num_values = sizeof(values) / sizeof(float);

    float sseResult[4];

    for (unsigned si = 0; si < 2; ++si)
    {
        for (unsigned i = 0; i < num_values; ++i)
        {
            const float x = sign_values[si] * values[i];

            const float expected = cosf(x);
            EvaluateCos(x, sseResult);

            const std::string operation = GetOperation("cos", x);

            CheckSSE(operation, expected, sseResult, 16);
        }
    }
}

void EvaluateSin(const float x, float* result)
{
    __m128 mm_sseResult = OCIO::sseSin(_mm_set1_ps(x));
    _mm_storeu_ps(result, mm_sseResult);
}

OCIO_ADD_TEST(SSE, sse2_sin_test)
{
    const float sign_values[] = { -1.0f, 1.0f };

    const float three_pi = (float) 9.4247779607693797153879301498385;
    const float two_pi   = (float) 6.283185307179586476925286766559;
    const float pi       = (float) 3.1415926535897932384626433832795;
    const float pi_2     = (float) 1.5707963267948966192313216916398;
    const float pi_3     = (float) 1.0471975511965977461542144610932;
    const float pi_4     = (float) 0.78539816339744830961566084581988;
    const float pi_6     = (float) 0.52359877559829887307710723054658;
    const float pi_12    = (float) 0.26179938779914943653855361527329;

    const float values[] = {
            0.0f,   1e-20f,   1e-10f,     1e-5f,
          0.005f,     0.1f,     0.5f,      1.0f,
           0.67f,   0.112f,  0.2425f,     0.33f,
              pi,     pi_2,     pi_3,      pi_4,
            pi_6,    pi_12,   two_pi,  three_pi,
         27.001f,  32.513f,  44.999f,   56.191f,
        61.0019f,    77.7f,  83.654f,   98.989f
    };
    const unsigned num_values = sizeof(values) / sizeof(float);

    float sseResult[4];

    for (unsigned si = 0; si < 2; ++si)
    {
        for (unsigned i = 0; i < num_values; ++i)
        {
            const float x = sign_values[si] * values[i];

            const float expected = sinf(x);
            EvaluateSin(x, sseResult);

            const std::string operation = GetOperation("sin", x);

            CheckSSE(operation, expected, sseResult, 16);
        }
    }
}

void EvaluateSinCos(const float x, float* resultSin, float* resultCos)
{
    __m128 sseResultSin, sseResultCos;
    OCIO::sseSinCos(_mm_set1_ps(x), sseResultSin, sseResultCos);
    _mm_storeu_ps(resultSin, sseResultSin);
    _mm_storeu_ps(resultCos, sseResultCos);
}

OCIO_ADD_TEST(SSE, sse2_sin_cos_test)
{
    const float sign_values[] = { -1.0f, 1.0f };

    const float three_pi = (float) 9.4247779607693797153879301498385;
    const float two_pi   = (float) 6.283185307179586476925286766559;
    const float pi       = (float) 3.1415926535897932384626433832795;
    const float pi_2     = (float) 1.5707963267948966192313216916398;
    const float pi_3     = (float) 1.0471975511965977461542144610932;
    const float pi_4     = (float) 0.78539816339744830961566084581988;
    const float pi_6     = (float) 0.52359877559829887307710723054658;
    const float pi_12    = (float) 0.26179938779914943653855361527329;

    const float values[] = {
            0.0f,   1e-20f,   1e-10f,     1e-5f,
          0.005f,     0.1f,     0.5f,      1.0f,
           0.67f,   0.112f,  0.2425f,     0.33f,
              pi,     pi_2,     pi_3,      pi_4,
            pi_6,    pi_12,   two_pi,  three_pi,
         27.001f,  32.513f,  44.999f,   56.191f,
        61.0019f,    77.7f,  83.654f,   98.989f
    };
    const unsigned num_values = sizeof(values) / sizeof(float);

    float sseResultSin[4];
    float sseResultCos[4];

    for (unsigned si = 0; si < 2; ++si)
    {
        for (unsigned i = 0; i < num_values; ++i)
        {
            const float x = sign_values[si] * values[i];

            const float expectedSin = sinf(x);
            const float expectedCos = cosf(x);
            EvaluateSinCos(x, sseResultSin, sseResultCos);

            const std::string operation = GetOperation("sincos", x);

            CheckSSE(operation, expectedSin, sseResultSin, 16);
            CheckSSE(operation, expectedCos, sseResultCos, 16);
        }
    }
}

OCIO_ADD_TEST(SSE, scalar_sin_cos_test)
{
    const float sign_values[] = { -1.0f, 1.0f };

    const float three_pi = (float) 9.4247779607693797153879301498385;
    const float two_pi   = (float) 6.283185307179586476925286766559;
    const float pi       = (float) 3.1415926535897932384626433832795;
    const float pi_2     = (float) 1.5707963267948966192313216916398;
    const float pi_3     = (float) 1.0471975511965977461542144610932;
    const float pi_4     = (float) 0.78539816339744830961566084581988;
    const float pi_6     = (float) 0.52359877559829887307710723054658;
    const float pi_12    = (float) 0.26179938779914943653855361527329;

    const float values[] = {
            0.0f,   1e-20f,   1e-10f,     1e-5f,
          0.005f,     0.1f,     0.5f,      1.0f,
           0.67f,   0.112f,  0.2425f,     0.33f,
              pi,     pi_2,     pi_3,      pi_4,
            pi_6,    pi_12,   two_pi,  three_pi,
         27.001f,  32.513f,  44.999f,   56.191f,
        61.0019f,    77.7f,  83.654f,   98.989f
    };
    const unsigned num_values = sizeof(values) / sizeof(float);

    float resultSin, resultCos;

    for (unsigned si = 0; si < 2; ++si)
    {
        for (unsigned i = 0; i < num_values; ++i)
        {
            const float x = sign_values[si] * values[i];

            const float expectedSin = sinf(x);
            const float expectedCos = cosf(x);
            OCIO::sseSinCos(x, resultSin, resultCos);

            const std::string operation = GetOperation("sincos", x);

            CheckFloat(operation, expectedSin, resultSin, 16);
            CheckFloat(operation, expectedCos, resultCos, 16);
        }
    }
}

#endif
