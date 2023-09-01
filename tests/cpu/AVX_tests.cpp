// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "CPUInfo.h"
#if OCIO_USE_AVX

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include <immintrin.h>
#include "MathUtils.h"
#include "BitDepthUtils.h"
#include "AVX.h"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

#define HAS_F16C() \
    OCIO::CPUInfo::instance().hasF16C()

#define DEFINE_SIMD_TEST(name) \
void avx_test_##name()

namespace
{

std::string GetFormatName(OCIO::BitDepth BD)
{
    switch(BD)
    {
        case OCIO::BIT_DEPTH_UINT8:
            return "BIT_DEPTH_UINT8";
        case OCIO::BIT_DEPTH_UINT10:
            return "BIT_DEPTH_UINT10";
        case OCIO::BIT_DEPTH_UINT12:
            return "BIT_DEPTH_UINT12";
        case OCIO::BIT_DEPTH_UINT16:
            return "BIT_DEPTH_UINT16";
        case OCIO::BIT_DEPTH_F16:
            return "BIT_DEPTH_F16";
        case OCIO::BIT_DEPTH_F32:
            return "BIT_DEPTH_F32";
        case OCIO::BIT_DEPTH_UINT14:
        case OCIO::BIT_DEPTH_UINT32:
        case OCIO::BIT_DEPTH_UNKNOWN:
        default:
            break;
    }

    return "BIT_DEPTH_UNKNOWN";
}

std::string GetErrorMessage(float expected, float actual, OCIO::BitDepth inBD, OCIO::BitDepth outBD)
{
    std::ostringstream oss;
    oss << "expected: " << expected << " != " << "actual: " << actual << " : " << GetFormatName(inBD) << " -> " <<  GetFormatName(outBD);
    return oss.str();
}

template<OCIO::BitDepth BD>
typename OCIO::BitDepthInfo<BD>::Type scale_unsigned(unsigned i)
{
    return i;
}

template <>
float scale_unsigned<OCIO::BIT_DEPTH_F32>(unsigned i)
{
    return static_cast<float>(i) * 1.0f/65535.0f;
}

#if OCIO_USE_F16C

template <>
half scale_unsigned<OCIO::BIT_DEPTH_F16>(unsigned i)
{
    return static_cast<half>(1.0f/65535.0f * static_cast<float>(i));
}

#endif

template<OCIO::BitDepth inBD, OCIO::BitDepth outBD>
void testConvert_OutBitDepth()
{
    typedef typename OCIO::BitDepthInfo<inBD>::Type InType;
    typedef typename OCIO::BitDepthInfo<outBD>::Type OutType;

    size_t maxValue = OCIO::BitDepthInfo<inBD>::maxValue + 1;

    if (OCIO::BitDepthInfo<inBD>::isFloat)
        maxValue = 65536;

    std::vector<InType> inImage(maxValue);
    std::vector<OutType> outImage(maxValue);

    for (unsigned i = 0; i < maxValue; i++)
    {
        inImage[i] = scale_unsigned<inBD>(i);
    }

    float scale = (float)OCIO::BitDepthInfo<outBD>::maxValue / (float)OCIO::BitDepthInfo<inBD>::maxValue;
    __m256 s = _mm256_set1_ps(scale);

    for (unsigned i = 0; i < inImage.size(); i += 32)
    {
        __m256 r, g, b, a;
        OCIO::AVXRGBAPack<inBD>::Load(&inImage[i], r, g, b, a);
        r = _mm256_mul_ps(r, s);
        g = _mm256_mul_ps(g, s);
        b = _mm256_mul_ps(b, s);
        a = _mm256_mul_ps(a, s);
        OCIO::AVXRGBAPack<outBD>::Store(&outImage[i], r, g, b, a);
    }
    for (unsigned i = 0; i < outImage.size(); i++)
    {
        float v = (float)inImage[i] * scale;

        if (OCIO::BitDepthInfo<outBD>::isFloat)
            v = (OutType)v; // casts to half if format is half
        else
            v = rintf(v);

        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(v, (float)outImage[i], 0, false),
                                  GetErrorMessage(v, (float)outImage[i], inBD, outBD));
    }
}

template<OCIO::BitDepth inBD>
void testConvert_InBitDepth(OCIO::BitDepth outBD)
{
    switch(outBD)
    {
        case OCIO::BIT_DEPTH_UINT8:
            return testConvert_OutBitDepth<inBD, OCIO::BIT_DEPTH_UINT8>();
        case OCIO::BIT_DEPTH_UINT10:
            return testConvert_OutBitDepth<inBD, OCIO::BIT_DEPTH_UINT10>();
        case OCIO::BIT_DEPTH_UINT12:
            return testConvert_OutBitDepth<inBD, OCIO::BIT_DEPTH_UINT12>();
        case OCIO::BIT_DEPTH_UINT16:
            return testConvert_OutBitDepth<inBD, OCIO::BIT_DEPTH_UINT16>();
        case OCIO::BIT_DEPTH_F16:
#if OCIO_USE_F16C
            if (HAS_F16C())
                return testConvert_OutBitDepth<inBD, OCIO::BIT_DEPTH_F16>();
#endif
            break;
        case OCIO::BIT_DEPTH_F32:
            return testConvert_OutBitDepth<inBD, OCIO::BIT_DEPTH_F32>();

        case OCIO::BIT_DEPTH_UINT14:
        case OCIO::BIT_DEPTH_UINT32:
        case OCIO::BIT_DEPTH_UNKNOWN:
        default:
            break;
    }
}

}

DEFINE_SIMD_TEST(packed_uint8_to_float_test)
{
    std::vector<uint8_t> inImage(256);
    std::vector<float> outImage(256);

    for (unsigned i = 0; i < inImage.size(); i++)
    {
        inImage[i] = i;
    }

    for (unsigned i = 0; i < inImage.size(); i += 32)
    {
        __m256 r, g, b, a;
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_UINT8>::Load(&inImage[i], r, g, b, a);
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F32>::Store(&outImage[i], r, g, b, a);
    }

    for (unsigned i = 0; i < outImage.size(); i++)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer((float)inImage[i], (float)outImage[i], 0, false),
                                  GetErrorMessage((float)inImage[i], (float)outImage[i],
                                                  OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_F32));
    }
}

DEFINE_SIMD_TEST(packed_uint10_to_f32_test)
{
    size_t maxValue = OCIO::BitDepthInfo<OCIO::BIT_DEPTH_UINT10>::maxValue + 1;
    std::vector<uint16_t> inImage(maxValue);
    std::vector<float> outImage(maxValue);

    for (unsigned i = 0; i < inImage.size(); i++)
    {
        inImage[i] = i;
    }

    for (unsigned i = 0; i < inImage.size(); i += 32)
    {
        __m256 r, g, b, a;
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_UINT10>::Load(&inImage[i], r, g, b, a);
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F32>::Store(&outImage[i], r, g, b, a);
    }

    for (unsigned i = 0; i < outImage.size(); i++)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer((float)inImage[i], (float)outImage[i], 0, false),
                                  GetErrorMessage((float)inImage[i], (float)outImage[i],
                                                   OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_F32));
    }
}

DEFINE_SIMD_TEST(packed_uint12_to_f32_test)
{
    size_t maxValue = OCIO::BitDepthInfo<OCIO::BIT_DEPTH_UINT12>::maxValue + 1;
    std::vector<uint16_t> inImage(maxValue);
    std::vector<float> outImage(maxValue);

    for (unsigned i = 0; i < inImage.size(); i++)
    {
        inImage[i] = i;
    }

    for (unsigned i = 0; i < inImage.size(); i += 32)
    {
        __m256 r, g, b, a;
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_UINT12>::Load(&inImage[i], r, g, b, a);
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F32>::Store(&outImage[i], r, g, b, a);
    }

    for (unsigned i = 0; i < outImage.size(); i++)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer((float)inImage[i], (float)outImage[i], 0, false),
                                  GetErrorMessage((float)inImage[i], (float)outImage[i],
                                                  OCIO::BIT_DEPTH_UINT12, OCIO::BIT_DEPTH_F32));
    }
}

DEFINE_SIMD_TEST(packed_uint16_to_f32_test)
{
    size_t maxValue = OCIO::BitDepthInfo<OCIO::BIT_DEPTH_UINT16>::maxValue + 1;
    std::vector<uint16_t> inImage(maxValue);
    std::vector<float> outImage(maxValue);

    for (unsigned i = 0; i < inImage.size(); i++)
    {
        inImage[i] = i;
    }

    for (unsigned i = 0; i < inImage.size(); i += 32)
    {
        __m256 r, g, b, a;
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_UINT16>::Load(&inImage[i], r, g, b, a);
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F32>::Store(&outImage[i], r, g, b, a);
    }

    for (unsigned i = 0; i < outImage.size(); i++)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer((float)inImage[i], (float)outImage[i], 0, false),
                                  GetErrorMessage((float)inImage[i], (float)outImage[i],
                                                  OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_F32));
    }
}

#if OCIO_USE_F16C

DEFINE_SIMD_TEST(packed_f16_to_f32_test)
{
    if(!HAS_F16C()) throw SkipException();

    size_t maxValue = OCIO::BitDepthInfo<OCIO::BIT_DEPTH_UINT16>::maxValue + 1;
    std::vector<half> inImage(maxValue);
    std::vector<float> outImage(maxValue);

    uint16_t *u16Image =(uint16_t*)&inImage[0];
    for (unsigned i = 0; i < inImage.size(); i++)
    {
        u16Image[i] = i;
    }

    for (unsigned i = 0; i < inImage.size(); i += 32)
    {
        __m256 r, g, b, a;
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F16>::Load(&inImage[i], r, g, b, a);
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F32>::Store(&outImage[i], r, g, b, a);
    }

    for (unsigned i = 0; i < outImage.size(); i++)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer((float)inImage[i], (float)outImage[i], 0, false),
                                  GetErrorMessage((float)inImage[i], (float)outImage[i],
                                                  OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32));
    }
}

#endif

DEFINE_SIMD_TEST(packed_nan_inf_test)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float maxf = std::numeric_limits<float>::max();

    __m256 r, g, b, a;
    std::vector<half> outImageHalf(32);
    std::vector<uint8_t> outImageU8(32);
    std::vector<uint16_t> outImageU16(32);

    const float pixels[32] = {     qnan,      qnan,       qnan,     0.25f,
                                   maxf,     -maxf,       3.2f,      qnan,
                                    inf,       inf,        inf,       inf,
                                   -inf,      -inf,       -inf,      -inf,
                                   0.0f,    270.0f,     500.0f,      2.0f,
                                  -0.0f,     -1.0f,     - 2.0f,     -5.0f,
                              100000.0f, 200000.0f,     -10.0f,  -2000.0f,
                               65535.0f,  65537.0f,  -65536.0f, -65537.0f };
#if OCIO_USE_F16C
    if(HAS_F16C())
    {
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F32>::Load(&pixels[0], r, g, b, a);
        OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F16>::Store(&outImageHalf[0], r, g, b, a);

        for (unsigned i = 0; i < outImageHalf.size(); i++)
        {
            OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer((half)pixels[i], (float)outImageHalf[i], 0, false),
                                    GetErrorMessage((half)pixels[i], (float)outImageHalf[i],
                                                    OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F16));
        }
    }

#endif

    const uint8_t resultU8[32] = {   0,   0,   0,   0,
                                   255,   0,   3,   0,
                                   255, 255, 255, 255,
                                     0,   0,   0,   0,
                                     0, 255, 255,   2,
                                     0,   0,   0,   0,
                                   255, 255,   0,   0,
                                   255, 255,   0,   0 };

    OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F32>::Load(&pixels[0], r, g, b, a);
    OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_UINT8>::Store(&outImageU8[0], r, g, b, a);

    for (unsigned i = 0; i < outImageU8.size(); i++)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(resultU8[i], outImageU8[i], 0, false),
                                  GetErrorMessage(resultU8[i], outImageU8[i],
                                                  OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT8));
    }

    const uint16_t resultU10[32] = {    0,    0,    0,    0,
                                     1023,    0,    3,    0,
                                     1023, 1023, 1023, 1023,
                                        0,    0,    0,    0,
                                        0,  270,  500,    2,
                                        0,    0,    0,    0,
                                     1023, 1023,    0,    0,
                                     1023, 1023,    0,    0};

    OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F32>::Load(&pixels[0], r, g, b, a);
    OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_UINT10>::Store(&outImageU16[0], r, g, b, a);

    for (unsigned i = 0; i < outImageU8.size(); i++)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(resultU10[i], outImageU16[i], 0, false),
                                  GetErrorMessage(resultU10[i], outImageU16[i],
                                                  OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT10));
    }

    const uint16_t resultU12[32] = {    0,    0,    0,    0,
                                     4095,    0,    3,    0,
                                     4095, 4095, 4095, 4095,
                                        0,    0,    0,    0,
                                        0,  270,  500,    2,
                                        0,    0,    0,    0,
                                     4095, 4095,    0,    0,
                                     4095, 4095,    0,    0};

    OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F32>::Load(&pixels[0], r, g, b, a);
    OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_UINT12>::Store(&outImageU16[0], r, g, b, a);

    for (unsigned i = 0; i < outImageU8.size(); i++)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(resultU12[i], outImageU16[i], 0, false),
                                  GetErrorMessage(resultU12[i], outImageU16[i],
                                                  OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT12));
    }

    const uint16_t resultU16[32] = {    0,     0,     0,     0,
                                    65535,     0,     3,     0,
                                    65535, 65535, 65535, 65535,
                                        0,     0,     0,     0,
                                        0,   270,   500,     2,
                                        0,     0,     0,     0,
                                    65535, 65535,     0,     0,
                                    65535, 65535,     0,     0};

    OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_F32>::Load(&pixels[0], r, g, b, a);
    OCIO::AVXRGBAPack<OCIO::BIT_DEPTH_UINT16>::Store(&outImageU16[0], r, g, b, a);

    for (unsigned i = 0; i < outImageU8.size(); i++)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(resultU16[i], outImageU16[i], 0, false),
                                  GetErrorMessage(resultU16[i], outImageU16[i],
                                                  OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT16));
    }
}

DEFINE_SIMD_TEST(packed_all_test)
{
    const std::vector<  OCIO::BitDepth> formats = {
                                                   OCIO::BIT_DEPTH_UINT8,
                                                   OCIO::BIT_DEPTH_UINT10,
                                                   OCIO::BIT_DEPTH_UINT12,
                                                   OCIO::BIT_DEPTH_UINT16,
                                                   OCIO::BIT_DEPTH_F16,
                                                   OCIO::BIT_DEPTH_F32,
                                                  };

    for(unsigned i = 0; i < formats.size(); i++)
    {
        OCIO::BitDepth inBD = formats[i];
        for(unsigned j = 0; j < formats.size(); j++)
        {
            OCIO::BitDepth outBD = formats[j];
            switch(inBD)
            {
            case OCIO::BIT_DEPTH_UINT8:
                testConvert_InBitDepth<OCIO::BIT_DEPTH_UINT8>(outBD);
                break;
            case OCIO::BIT_DEPTH_UINT10:
                testConvert_InBitDepth<OCIO::BIT_DEPTH_UINT10>(outBD);
                break;
            case OCIO::BIT_DEPTH_UINT12:
                testConvert_InBitDepth<OCIO::BIT_DEPTH_UINT12>(outBD);
                break;
            case OCIO::BIT_DEPTH_UINT16:
                testConvert_InBitDepth<OCIO::BIT_DEPTH_UINT16>(outBD);
                break;
            case OCIO::BIT_DEPTH_F16:
#if OCIO_USE_F16C
                if(HAS_F16C())
                    testConvert_InBitDepth<OCIO::BIT_DEPTH_F16>(outBD);
#endif
                break;
            case OCIO::BIT_DEPTH_F32:
                testConvert_InBitDepth<OCIO::BIT_DEPTH_F32>(outBD);
                break;
            case OCIO::BIT_DEPTH_UINT14:
            case OCIO::BIT_DEPTH_UINT32:
            case OCIO::BIT_DEPTH_UNKNOWN:
                break;
            default:
                break;
            }
        }
    }
}

#endif // OCIO_USE_AVX