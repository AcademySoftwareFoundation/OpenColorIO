// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_SSE2_H
#define INCLUDED_OCIO_SSE2_H

#include "CPUInfo.h"
#ifdef OCIO_USE_SSE2

#include <immintrin.h>
#include <stdio.h>

#include <OpenColorIO/OpenColorIO.h>
#include "BitDepthUtils.h"

// Macros for alignment declarations
#define SSE2_SIMD_BYTES 16
#if defined( _MSC_VER )
#define SSE2_ALIGN(decl) __declspec(align(SSE2_SIMD_BYTES)) decl
#elif ( __APPLE__ )

#define SSE2_ALIGN(decl) decl
#else
#define SSE2_ALIGN(decl) decl __attribute__((aligned(SSE2_SIMD_BYTES)))
#endif

namespace OCIO_NAMESPACE
{

inline __m128 sse2_clamp(__m128 value, const __m128& maxValue)
{
    value = _mm_max_ps(value, _mm_setzero_ps());
    return _mm_min_ps(value, maxValue);
}

static inline void sse2RGBATranspose_4x4(__m128 row0, __m128 row1, __m128 row2, __m128 row3,
                                         __m128 &out_r, __m128 &out_g, __m128 &out_b, __m128 &out_a )
{
    __m128 tmp0 = _mm_unpacklo_ps(row0, row1);
    __m128 tmp2 = _mm_unpacklo_ps(row2, row3);
    __m128 tmp1 = _mm_unpackhi_ps(row0, row1);
    __m128 tmp3 = _mm_unpackhi_ps(row2, row3);
    out_r = _mm_movelh_ps(tmp0, tmp2);
    out_g = _mm_movehl_ps(tmp2, tmp0); // Note movhlps swaps b with a which is different than unpckhpd
    out_b = _mm_movelh_ps(tmp1, tmp3);
    out_a = _mm_movehl_ps(tmp3, tmp1);
}

// Note Packing functions preform no 0.0 - 1.0 normalization
// but preform 0 - max value clamping for integer formats
template<BitDepth BD> struct SSE2RGBAPack {};
template <>
struct SSE2RGBAPack<BIT_DEPTH_UINT8>
{
    static inline void Load(const uint8_t *in, __m128& r, __m128& g, __m128& b, __m128& a)
    {
        const __m128i zero = _mm_setzero_si128();
        __m128i rgba_00_03 = _mm_loadu_si128((const __m128i*)in);

        __m128i rgba_00_01 = _mm_unpacklo_epi8(rgba_00_03, zero);
        __m128i rgba_02_03 = _mm_unpackhi_epi8(rgba_00_03, zero);

        __m128 rgba0 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(rgba_00_01, zero));
        __m128 rgba1 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(rgba_00_01, zero));

        __m128 rgba2 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(rgba_02_03, zero));
        __m128 rgba3 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(rgba_02_03, zero));

        sse2RGBATranspose_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }
    static inline void Store(uint8_t *out, __m128 r_in, __m128 g_in, __m128 b_in, __m128 a_in)
    {
        const __m128 maxValue = _mm_set1_ps(255.0f);

        // NOTE note using cvtps which will round based on MXCSR register defaults to _MM_ROUND_NEAREST
        __m128i r = _mm_cvtps_epi32(sse2_clamp(r_in, maxValue));
        __m128i g = _mm_cvtps_epi32(sse2_clamp(g_in, maxValue));
        __m128i b = _mm_cvtps_epi32(sse2_clamp(b_in, maxValue));
        __m128i a = _mm_cvtps_epi32(sse2_clamp(a_in, maxValue));

        __m128i rgba = _mm_or_si128(r, _mm_slli_si128(g, 1));
        rgba = _mm_or_si128(rgba, _mm_slli_si128(b, 2));
        rgba = _mm_or_si128(rgba, _mm_slli_si128(a, 3));
        _mm_storeu_si128((__m128i*)out, rgba);
    }
};

template<BitDepth BD>
struct SSE2RGBAPack16
{
    typedef typename BitDepthInfo<BD>::Type Type;

    static inline void Load(const Type *in, __m128& r, __m128& g, __m128& b, __m128& a)
    {
        const __m128i zero = _mm_setzero_si128();
        __m128i rgba_00_01 = _mm_loadu_si128((const __m128i*)(in + 0));
        __m128i rgba_02_03 = _mm_loadu_si128((const __m128i*)(in + 8));

        __m128 rgba0 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(rgba_00_01, zero));
        __m128 rgba1 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(rgba_00_01, zero));

        __m128 rgba2 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(rgba_02_03, zero));
        __m128 rgba3 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(rgba_02_03, zero));

        sse2RGBATranspose_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }

    static inline void Store(Type *out, __m128 r, __m128 g, __m128 b, __m128 a)
    {
        const __m128 maxValue = _mm_set1_ps((float)BitDepthInfo<BD>::maxValue);

        __m128i rrrr = _mm_cvtps_epi32(sse2_clamp(r, maxValue));
        __m128i gggg = _mm_cvtps_epi32(sse2_clamp(g, maxValue));
        __m128i bbbb = _mm_cvtps_epi32(sse2_clamp(b, maxValue));
        __m128i aaaa = _mm_cvtps_epi32(sse2_clamp(a, maxValue));

        __m128i rgrg_rgrg = _mm_or_si128(rrrr, _mm_slli_si128(gggg, 2));
        __m128i baba_baba = _mm_or_si128(bbbb, _mm_slli_si128(aaaa, 2));

        __m128i rgba_00_01 = _mm_unpacklo_epi32(rgrg_rgrg, baba_baba);
        __m128i rgba_02_03 = _mm_unpackhi_epi32(rgrg_rgrg, baba_baba);

        _mm_storeu_si128((__m128i*)(out + 0), rgba_00_01);
        _mm_storeu_si128((__m128i*)(out + 8), rgba_02_03);
    }
};

template <>
struct SSE2RGBAPack<BIT_DEPTH_UINT10>
{
    static inline void Load(const uint16_t *in, __m128& r, __m128& g, __m128& b, __m128& a)
    {
        SSE2RGBAPack16<BIT_DEPTH_UINT10>::Load(in, r, g, b, a);
    }
    static inline void Store(uint16_t *out, __m128 r, __m128 g, __m128 b, __m128 a)
    {
        SSE2RGBAPack16<BIT_DEPTH_UINT10>::Store(out, r, g, b, a);
    }
};

template <>
struct SSE2RGBAPack<BIT_DEPTH_UINT12>
{
    static inline void Load(const uint16_t *in, __m128& r, __m128& g, __m128& b, __m128& a)
    {
        SSE2RGBAPack16<BIT_DEPTH_UINT12>::Load(in, r, g, b, a);
    }
    static inline  void Store(uint16_t *out, __m128 r, __m128 g, __m128 b, __m128 a)
    {
        SSE2RGBAPack16<BIT_DEPTH_UINT12>::Store(out, r, g, b, a);
    }
};

template <>
struct SSE2RGBAPack<BIT_DEPTH_UINT16>
{
    static inline void Load(const uint16_t *in, __m128& r, __m128& g, __m128& b, __m128& a)
    {
        SSE2RGBAPack16<BIT_DEPTH_UINT16>::Load(in, r, g, b, a);
    }
    static inline  void Store(uint16_t *out, __m128 r, __m128 g, __m128 b, __m128 a)
    {
        SSE2RGBAPack16<BIT_DEPTH_UINT16>::Store(out, r, g, b, a);
    }
};

#if OCIO_USE_F16C

template <>
struct SSE2RGBAPack<BIT_DEPTH_F16>
{
    static inline void Load(const half *in, __m128& r, __m128& g, __m128& b, __m128& a)
    {
        __m128i rgba_00_01 = _mm_loadu_si128((const __m128i*)(in + 0));
        __m128i rgba_02_03 = _mm_loadu_si128((const __m128i*)(in + 8));

        __m128 rgba0 = _mm_cvtph_ps(rgba_00_01);
        __m128 rgba1 = _mm_cvtph_ps(_mm_shuffle_epi32(rgba_00_01, _MM_SHUFFLE(1,0,3,2)));
        __m128 rgba2 = _mm_cvtph_ps(rgba_02_03);
        __m128 rgba3 = _mm_cvtph_ps(_mm_shuffle_epi32(rgba_02_03, _MM_SHUFFLE(1,0,3,2)));

        sse2RGBATranspose_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }

    static inline  void Store(half *out, __m128 r, __m128 g, __m128 b, __m128 a)
    {
        __m128 rgba0, rgba1, rgba2, rgba3;
        __m128i rgba;

        sse2RGBATranspose_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        __m128i rgba00_01 = _mm_cvtps_ph(rgba0, 0);
        __m128i rgba02_03 = _mm_cvtps_ph(rgba1, 0);
        __m128i rgba04_05 = _mm_cvtps_ph(rgba2, 0);
        __m128i rgba06_07 = _mm_cvtps_ph(rgba3, 0);

        rgba = _mm_xor_si128(rgba00_01, _mm_shuffle_epi32(rgba02_03, _MM_SHUFFLE(1,0,3,2)));
        _mm_storeu_si128((__m128i*)(out+0), rgba);

        rgba = _mm_xor_si128(rgba04_05, _mm_shuffle_epi32(rgba06_07, _MM_SHUFFLE(1,0,3,2)));
        _mm_storeu_si128((__m128i*)(out+8), rgba);
    }
};

#endif

template <>
struct SSE2RGBAPack<BIT_DEPTH_F32>
{
    static inline void Load(const float *in, __m128& r, __m128& g, __m128& b, __m128& a)
    {
        __m128 rgba0 = _mm_loadu_ps(in +  0);
        __m128 rgba1 = _mm_loadu_ps(in +  4);
        __m128 rgba2 = _mm_loadu_ps(in +  8);
        __m128 rgba3 = _mm_loadu_ps(in + 12);

        sse2RGBATranspose_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }

    static inline void Store(float *out, __m128 r, __m128 g, __m128 b, __m128 a)
    {
        __m128 rgba0, rgba1, rgba2, rgba3;
        sse2RGBATranspose_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        _mm_storeu_ps(out +  0, rgba0);
        _mm_storeu_ps(out +  4, rgba1);
        _mm_storeu_ps(out +  8, rgba2);
        _mm_storeu_ps(out + 12, rgba3);
    }
};


} // namespace OCIO_NAMESPACE

#endif // OCIO_USE_SSE2
#endif // INCLUDED_OCIO_SSE2_H