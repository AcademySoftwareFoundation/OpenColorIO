// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_AVX2_H
#define INCLUDED_OCIO_AVX2_H

#include "CPUInfo.h"
#if OCIO_USE_AVX2

#include <immintrin.h>
#include <stdio.h>

#include <OpenColorIO/OpenColorIO.h>
#include "BitDepthUtils.h"

// Macros for alignment declarations
#define AVX2_SIMD_BYTES 32
#define AVX2_ALIGN(decl) alignas(AVX2_SIMD_BYTES) decl

namespace OCIO_NAMESPACE
{

inline __m256 avx2_movelh_ps(__m256 a, __m256 b)
{
    return _mm256_castpd_ps(_mm256_unpacklo_pd(_mm256_castps_pd(a), _mm256_castps_pd(b)));
}

inline __m256 avx2_movehl_ps(__m256 a, __m256 b)
{
    // NOTE: this is a and b are reversed to match sse2 movhlps which is different than unpckhpd
    return _mm256_castpd_ps(_mm256_unpackhi_pd(_mm256_castps_pd(b), _mm256_castps_pd(a)));
}

inline __m256 avx2_clamp(__m256 value, const __m256& maxValue)
{
    value = _mm256_max_ps(value, _mm256_setzero_ps());
    return _mm256_min_ps(value, maxValue);
}

inline void avx2RGBATranspose_4x4_4x4(__m256 row0, __m256 row1, __m256 row2, __m256 row3,
            
                                      __m256 &out_r, __m256 &out_g, __m256 &out_b, __m256 &out_a )
{
    // the rgba transpose result will look this
    //
    //  0   1   2   3    0   1   2   3         0   1   2   3    0   1   2   3
    // r0, g0, b0, a0 | r1, g1, b1, a1        r0, r2, r4, r6 | r1, r3, r5, r7
    // r2, g2, b2, a2 | r3, g3, b3, a3  <==>  g0, g2, g4, g6 | g1, g3, g5, g7
    // r4, g4, b4, a4 | r5, g5, b5, a5  <==>  b0, b2, b4, b6 | b1, b3, b5, b7
    // r6, g6, b6, a6 | r7, g7, b7, a7        a0, a2, a4, a6 | a1, a4, a5, a7

    // each 128 lane is transposed independently,
    // the channel values end up with a even/odd shuffled order because of this.
    // if exact order is important more cross lane shuffling is needed

    __m256 tmp0 = _mm256_unpacklo_ps(row0, row1);
    __m256 tmp2 = _mm256_unpacklo_ps(row2, row3);
    __m256 tmp1 = _mm256_unpackhi_ps(row0, row1);
    __m256 tmp3 = _mm256_unpackhi_ps(row2, row3);

    out_r = avx2_movelh_ps(tmp0, tmp2);
    out_g = avx2_movehl_ps(tmp2, tmp0);
    out_b = avx2_movelh_ps(tmp1, tmp3);
    out_a = avx2_movehl_ps(tmp3, tmp1);

}

// Note Packing functions perform no 0.0 - 1.0 normalization
// but perform 0 - max value clamping for integer formats
template<BitDepth BD> struct AVX2RGBAPack {};

template <>
struct AVX2RGBAPack<BIT_DEPTH_UINT8>
{
    static inline void Load(const uint8_t *in, __m256& r, __m256& g, __m256& b, __m256& a)
    {
        __m256i rgba_00_07 = _mm256_loadu_si256((const __m256i*)in);

        __m128i rgba_00_03 =_mm256_castsi256_si128(rgba_00_07);
        __m128i rgba_04_07 =_mm256_extractf128_si256(rgba_00_07, 1);

        //          :  0,  1,  2,  3 |  4,  5,  6,  7 |  8,  9, 10, 11 | 12, 13, 14, 15
        // rgba_x03 : r0, g0, b0, a0 | r1, g1, b1, a1 | r2, g2, b2, a2 | r3, g3, b3, a3
        // rgba_x47 : r4, g4, b4, a4 | r5, g5, b5, a5 | r6, g6, b6, a6 | r7, g7, b7, a7

        __m256 rgba0 = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(rgba_00_03));
        __m256 rgba1 = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_shuffle_epi32(rgba_00_03, _MM_SHUFFLE(3, 2, 3, 2))));

        __m256 rgba2 = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(rgba_04_07));
        __m256 rgba3 = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_shuffle_epi32(rgba_04_07, _MM_SHUFFLE(3, 2, 3, 2))));

        avx2RGBATranspose_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }
    static inline void Store(uint8_t *out, __m256 r, __m256 g, __m256 b, __m256 a)
    {
        __m256 rgba0, rgba1, rgba2, rgba3;
        const __m256 maxValue = _mm256_set1_ps(255.0f);

        avx2RGBATranspose_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        rgba0 = avx2_clamp(rgba0, maxValue);
        rgba1 = avx2_clamp(rgba1, maxValue);
        rgba2 = avx2_clamp(rgba2, maxValue);
        rgba3 = avx2_clamp(rgba3, maxValue);

        // NOTE note using cvtps which will round based on MXCSR register defaults to _MM_ROUND_NEAREST
        __m256i rgba01 = _mm256_cvtps_epi32(rgba0);
        __m256i rgba23 = _mm256_cvtps_epi32(rgba1);
        __m256i rgba45 = _mm256_cvtps_epi32(rgba2);
        __m256i rgba67 = _mm256_cvtps_epi32(rgba3);

        const __m256i rgba_shuf_a = _mm256_setr_epi8( 0, 4, 8, 12,  -1,-1,-1, -1,  -1,-1,-1,-1, -1,-1,-1,-1,
                                                    -1,-1,-1, -1,   0, 4, 8, 12, -1,-1,-1,-1, -1,-1,-1,-1);

        const __m256i rgba_shuf_b = _mm256_setr_epi8(-1,-1,-1,-1, -1,-1,-1,-1,  0, 4, 8, 12,  -1,-1,-1,-1,
                                                    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1, -1,   0, 4, 8, 12);

        rgba01 = _mm256_shuffle_epi8(rgba01, rgba_shuf_a);
        rgba23 = _mm256_shuffle_epi8(rgba23, rgba_shuf_b);
        rgba01 = _mm256_or_si256(rgba01, rgba23);

        __m128i lo = _mm_or_si128(_mm256_castsi256_si128(rgba01), _mm256_extractf128_si256(rgba01, 1));

        rgba45 = _mm256_shuffle_epi8(rgba45, rgba_shuf_a);
        rgba67 = _mm256_shuffle_epi8(rgba67, rgba_shuf_b);
        rgba45 = _mm256_or_si256(rgba45, rgba67);

        __m128i hi = _mm_or_si128(_mm256_castsi256_si128(rgba45), _mm256_extractf128_si256(rgba45, 1));

        __m256i rgba = _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);

        _mm256_storeu_si256((__m256i*)out, rgba);
    }
};

template<BitDepth BD>
struct AVX2RGBAPack16
{
    typedef typename BitDepthInfo<BD>::Type Type;

    static inline void Load(const Type *in, __m256& r, __m256& g, __m256& b, __m256& a)
    {
        // const __m256 scale = _mm256_set1_ps(1.0f / (float)BitDepthInfo<BD>::maxValue);
        __m256i rgba_00_03 = _mm256_loadu_si256((const __m256i*)(in +  0));
        __m256i rgba_04_07 = _mm256_loadu_si256((const __m256i*)(in + 16));

        __m256 rgba0 = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(_mm256_castsi256_si128(rgba_00_03)));
        __m256 rgba1 = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(_mm256_extractf128_si256(rgba_00_03, 1)));
        __m256 rgba2 = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(_mm256_castsi256_si128(rgba_04_07)));
        __m256 rgba3 = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(_mm256_extractf128_si256(rgba_04_07, 1)));

        avx2RGBATranspose_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }

    static inline void Store(Type *out, __m256 r, __m256 g, __m256 b, __m256 a)
    {
        __m256 rgba0, rgba1, rgba2, rgba3;
        __m128i lo, hi;
        __m256i rgba;
        const __m256 maxValue = _mm256_set1_ps((float)BitDepthInfo<BD>::maxValue);

        avx2RGBATranspose_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        rgba0 = avx2_clamp(rgba0, maxValue);
        rgba1 = avx2_clamp(rgba1, maxValue);
        rgba2 = avx2_clamp(rgba2, maxValue);
        rgba3 = avx2_clamp(rgba3, maxValue);

        // NOTE note using cvtps which will round based on MXCSR register defaults to _MM_ROUND_NEAREST
        __m256i rgba01 = _mm256_cvtps_epi32(rgba0);
        __m256i rgba23 = _mm256_cvtps_epi32(rgba1);
        __m256i rgba45 = _mm256_cvtps_epi32(rgba2);
        __m256i rgba67 = _mm256_cvtps_epi32(rgba3);

        const __m256i rgba_shuf = _mm256_setr_epi8( 0, 1, 4, 5,   8,  9, 12, 13,  -1,-1,-1,-1, -1,-1, -1 ,-1,
                                                -1,-1,-1,-1,  -1, -1, -1, -1,   0, 1, 4, 5,  8, 9, 12, 13);

        rgba01 = _mm256_shuffle_epi8(rgba01, rgba_shuf);
        lo = _mm_or_si128(_mm256_castsi256_si128(rgba01), _mm256_extractf128_si256(rgba01, 1));

        rgba23 = _mm256_shuffle_epi8(rgba23, rgba_shuf);
        hi = _mm_or_si128(_mm256_castsi256_si128(rgba23), _mm256_extractf128_si256(rgba23, 1));

        rgba = _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
        _mm256_storeu_si256((__m256i*)(out+0), rgba);

        rgba45 = _mm256_shuffle_epi8(rgba45, rgba_shuf);
        lo = _mm_or_si128(_mm256_castsi256_si128(rgba45), _mm256_extractf128_si256(rgba45, 1));

        rgba67 = _mm256_shuffle_epi8(rgba67, rgba_shuf);
        hi = _mm_or_si128(_mm256_castsi256_si128(rgba67), _mm256_extractf128_si256(rgba67, 1));

        rgba = _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
        _mm256_storeu_si256((__m256i*)(out+16), rgba);
    }
};

template <>
struct AVX2RGBAPack<BIT_DEPTH_UINT10>
{
    static inline void Load(const uint16_t *in, __m256& r, __m256& g, __m256& b, __m256& a)
    {
        AVX2RGBAPack16<BIT_DEPTH_UINT10>::Load(in, r, g, b, a);
    }
    static inline void Store(uint16_t *out, __m256 r, __m256 g, __m256 b, __m256 a)
    {
        AVX2RGBAPack16<BIT_DEPTH_UINT10>::Store(out, r, g, b, a);
    }
};

template <>
struct AVX2RGBAPack<BIT_DEPTH_UINT12>
{
    static inline void Load(const uint16_t *in, __m256& r, __m256& g, __m256& b, __m256& a)
    {
        AVX2RGBAPack16<BIT_DEPTH_UINT12>::Load(in, r, g, b, a);
    }
    static inline  void Store(uint16_t *out, __m256 r, __m256 g, __m256 b, __m256 a)
    {
        AVX2RGBAPack16<BIT_DEPTH_UINT12>::Store(out, r, g, b, a);
    }
};

template <>
struct AVX2RGBAPack<BIT_DEPTH_UINT16>
{
    static inline void Load(const uint16_t *in, __m256& r, __m256& g, __m256& b, __m256& a)
    {
        AVX2RGBAPack16<BIT_DEPTH_UINT16>::Load(in, r, g, b, a);
    }
    static inline  void Store(uint16_t *out, __m256 r, __m256 g, __m256 b, __m256 a)
    {
        AVX2RGBAPack16<BIT_DEPTH_UINT16>::Store(out, r, g, b, a);
    }
};

#if OCIO_USE_F16C

template <>
struct AVX2RGBAPack<BIT_DEPTH_F16>
{
    static inline void Load(const half *in, __m256& r, __m256& g, __m256& b, __m256& a)
    {

        __m256i rgba_00_03 = _mm256_loadu_si256((const __m256i*)(in +  0));
        __m256i rgba_04_07 = _mm256_loadu_si256((const __m256i*)(in + 16));

        __m256 rgba0 = _mm256_cvtph_ps(_mm256_castsi256_si128(rgba_00_03));
        __m256 rgba1 = _mm256_cvtph_ps(_mm256_extractf128_si256(rgba_00_03, 1));
        __m256 rgba2 = _mm256_cvtph_ps(_mm256_castsi256_si128(rgba_04_07));
        __m256 rgba3 = _mm256_cvtph_ps(_mm256_extractf128_si256(rgba_04_07, 1));

        avx2RGBATranspose_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }

    static inline  void Store(half *out, __m256 r, __m256 g, __m256 b, __m256 a)
    {
        __m256 rgba0, rgba1, rgba2, rgba3;
        __m256i rgba;

        avx2RGBATranspose_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        __m128i rgba00_03 = _mm256_cvtps_ph(rgba0, 0);
        __m128i rgba04_07 = _mm256_cvtps_ph(rgba1, 0);
        __m128i rgba08_11 = _mm256_cvtps_ph(rgba2, 0);
        __m128i rgba12_16 = _mm256_cvtps_ph(rgba3, 0);

        rgba = _mm256_inserti128_si256(_mm256_castsi128_si256(rgba00_03), rgba04_07, 1);
        _mm256_storeu_si256((__m256i*)(out+0), rgba);

        rgba = _mm256_inserti128_si256(_mm256_castsi128_si256(rgba08_11), rgba12_16, 1);
        _mm256_storeu_si256((__m256i*)(out+16), rgba);
    }
};

#endif

template <>
struct AVX2RGBAPack<BIT_DEPTH_F32>
{
    static inline void Load(const float *in, __m256& r, __m256& g, __m256& b, __m256& a)
    {
        const __m256i rgba_idx = _mm256_setr_epi32(0, 8, 16, 24, 4, 12, 20, 28);
        r = _mm256_i32gather_ps(in + 0, rgba_idx, 4);
        g = _mm256_i32gather_ps(in + 1, rgba_idx, 4);
        b = _mm256_i32gather_ps(in + 2, rgba_idx, 4);
        a = _mm256_i32gather_ps(in + 3, rgba_idx, 4);
    }

    static inline void Store(float *out, __m256 r, __m256 g, __m256 b, __m256 a)
    {
        __m256 rgba0, rgba1, rgba2, rgba3;
        avx2RGBATranspose_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        _mm256_storeu_ps(out +  0, rgba0);
        _mm256_storeu_ps(out +  8, rgba1);
        _mm256_storeu_ps(out + 16, rgba2);
        _mm256_storeu_ps(out + 24, rgba3);
    }
};

} // namespace OCIO_NAMESPACE

#endif // OCIO_USE_AVX2
#endif // INCLUDED_OCIO_AVX2_H