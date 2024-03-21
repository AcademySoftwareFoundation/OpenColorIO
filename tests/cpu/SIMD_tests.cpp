#include "testutils/UnitTest.h"
#include "CPUInfo.h"

namespace OCIO = OCIO_NAMESPACE;

#if OCIO_USE_SSE2

#define SSE2_CHECK() \
    if (!OCIO::CPUInfo::instance().hasSSE2()) throw SkipException()

#define OCIO_ADD_TEST_SSE2(name) \
void sse2_test_##name();         \
OCIO_ADD_TEST(SSE2, name)        \
{                                \
   SSE2_CHECK();                 \
   sse2_test_##name();           \
}

OCIO_ADD_TEST_SSE2(packed_uint8_to_float_test)
OCIO_ADD_TEST_SSE2(packed_uint10_to_f32_test)
OCIO_ADD_TEST_SSE2(packed_uint12_to_f32_test)
OCIO_ADD_TEST_SSE2(packed_uint16_to_f32_test)
OCIO_ADD_TEST_SSE2(packed_f16_to_f32_test)
OCIO_ADD_TEST_SSE2(packed_nan_inf_test)
OCIO_ADD_TEST_SSE2(packed_all_test)

#endif

#if OCIO_USE_AVX

#define AVX_CHECK() \
    if (!OCIO::CPUInfo::instance().hasAVX()) throw SkipException()

#define OCIO_ADD_TEST_AVX(name) \
void avx_test_##name();         \
OCIO_ADD_TEST(AVX, name)        \
{                               \
   AVX_CHECK();                 \
   avx_test_##name();           \
}

OCIO_ADD_TEST_AVX(packed_uint8_to_float_test)
OCIO_ADD_TEST_AVX(packed_uint10_to_f32_test)
OCIO_ADD_TEST_AVX(packed_uint12_to_f32_test)
OCIO_ADD_TEST_AVX(packed_uint16_to_f32_test)
#if OCIO_USE_F16C
    OCIO_ADD_TEST_AVX(packed_f16_to_f32_test)
#endif
OCIO_ADD_TEST_AVX(packed_nan_inf_test)
OCIO_ADD_TEST_AVX(packed_all_test)

#endif

#if OCIO_USE_AVX2

#define AVX2_CHECK() \
    if (!OCIO::CPUInfo::instance().hasAVX2()) throw SkipException()

#define OCIO_ADD_TEST_AVX2(name) \
void avx2_test_##name();         \
OCIO_ADD_TEST(AVX2, name)        \
{                                \
   AVX2_CHECK();                 \
   avx2_test_##name();           \
}

OCIO_ADD_TEST_AVX2(packed_uint8_to_float_test)
OCIO_ADD_TEST_AVX2(packed_uint10_to_f32_test)
OCIO_ADD_TEST_AVX2(packed_uint12_to_f32_test)
OCIO_ADD_TEST_AVX2(packed_uint16_to_f32_test)
#if OCIO_USE_F16C
    OCIO_ADD_TEST_AVX2(packed_f16_to_f32_test)
#endif
OCIO_ADD_TEST_AVX2(packed_nan_inf_test)
OCIO_ADD_TEST_AVX2(packed_all_test)

#endif

#if OCIO_USE_AVX512

#define AVX512_CHECK() \
    if (!OCIO::CPUInfo::instance().hasAVX512()) throw SkipException()

#define OCIO_ADD_TEST_AVX512(name) \
void avx512_test_##name();         \
OCIO_ADD_TEST(AVX512, name)        \
{                                \
   AVX512_CHECK();                 \
   avx512_test_##name();           \
}

OCIO_ADD_TEST_AVX512(packed_uint8_to_float_test)
OCIO_ADD_TEST_AVX512(packed_uint10_to_f32_test)
OCIO_ADD_TEST_AVX512(packed_uint12_to_f32_test)
OCIO_ADD_TEST_AVX512(packed_uint16_to_f32_test)
OCIO_ADD_TEST_AVX512(packed_f16_to_f32_test)
OCIO_ADD_TEST_AVX512(packed_nan_inf_test)
OCIO_ADD_TEST_AVX512(packed_all_test)

#endif