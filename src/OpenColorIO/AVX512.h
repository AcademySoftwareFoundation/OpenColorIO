// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_AVX512_H
#define INCLUDED_OCIO_AVX512_H

#include "CPUInfo.h"
#if OCIO_USE_AVX512

#include <immintrin.h>

#include <OpenColorIO/OpenColorIO.h>
#include "BitDepthUtils.h"

// Macros for alignment declarations
#define AVX512_SIMD_BYTES 64
#define AVX512_ALIGN(decl) alignas(AVX512_SIMD_BYTES) decl

namespace OCIO_NAMESPACE
{

inline __m512 av512_clamp(__m512 value, const __m512& maxValue)
{
    value = _mm512_max_ps(value, _mm512_setzero_ps());
    return _mm512_min_ps(value, maxValue);
}

inline __m512 avx512_movelh_ps(__m512 a, __m512 b)
{
    return _mm512_castpd_ps(_mm512_unpacklo_pd(_mm512_castps_pd(a), _mm512_castps_pd(b)));
}

inline __m512 avx512_movehl_ps(__m512 a, __m512 b)
{
    // NOTE: this is a and b are reversed to match sse2 movhlps which is different than unpckhpd
    return _mm512_castpd_ps(_mm512_unpackhi_pd(_mm512_castps_pd(b), _mm512_castps_pd(a)));
}


inline void avx512RGBATranspose_4x4_4x4_4x4_4x4(__m512 row0,   __m512 row1,   __m512 row2,   __m512 row3,   
                                                __m512 &out_r, __m512 &out_g, __m512 &out_b, __m512 &out_a )
{
    // the rgba transpose result will look this
    //
    //   0    1    2    3 |   4    5    6    7     8    9   10   11    12   13   14   15           
    //  r0,  g0,  b0,  a0 |  r1,  g1,  b1,  a1 |  r2,  g2,  b2,  a2 |  r3,  g3,  b3,  a3          
    //  r4,  g4,  b4,  a4 |  r5,  g5,  b5,  a5 |  r6,  g6,  b6,  a6 |  r7,  g7,  b7,  a7    
    //  r8   g8,  b8,  a8 |  r9,  g9,  b9,  a9 | r10, g10, b10, a10 | r11, g11, b11, a11 
    // r12, g12, b12, a12 | r13, g13, b13, a13 | r14, g14, b14, a14 | r15, g15, b15, a15          
    //                    |                    |                    |
    //         |          |          |         |          |         |          |        
    //         V          |          V         |          V         |          V
    //                    |                    |                    | 
    //  r0,  r4,  r8, r12 |  r1,  r5,  r9, r13 |  r2,  r6, r10, r14 |  r3,  r7, r11, r15
    //  g0,  g4,  g8, g12 |  g1,  g5,  g9, g13 |  g2,  g6, g10, g14 |  g3,  g7, g11, g15
    //  b0,  b4,  b9, b12 |  b1,  b5,  b9, b13 |  b2,  b6, b10, b14 |  b3,  b7, b11, b15
    //  a0,  a4,  a8, a12 |  a1,  a5,  a9, a13 |  a2,  a6, a10, a14 |  a3,  a7, a11, a15


    // each 128 lane is transposed independently,
    // the channel values end up with a even/odd shuffled order because of this.
    // if exact order is important more cross lane shuffling is needed

    __m512 tmp0 = _mm512_unpacklo_ps(row0, row1);
    __m512 tmp2 = _mm512_unpacklo_ps(row2, row3);
    __m512 tmp1 = _mm512_unpackhi_ps(row0, row1);
    __m512 tmp3 = _mm512_unpackhi_ps(row2, row3);

    out_r = avx512_movelh_ps(tmp0, tmp2);
    out_g = avx512_movehl_ps(tmp2, tmp0);
    out_b = avx512_movelh_ps(tmp1, tmp3);
    out_a = avx512_movehl_ps(tmp3, tmp1);

}


// Note Packing functions perform no 0.0 - 1.0 normalization
// but perform 0 - max value clamping for integer formats
template<BitDepth BD> struct AVX512RGBAPack {};

template <>
struct AVX512RGBAPack<BIT_DEPTH_UINT8>
{
    static inline void Load(const uint8_t *in, __m512& r, __m512& g, __m512& b, __m512& a)
    {
        __m512i rgba = _mm512_loadu_si512((const __m512i*)in);

        __m512 rgba0 = _mm512_cvtepi32_ps(_mm512_cvtepu8_epi32(_mm512_castsi512_si128(rgba)));
        __m512 rgba1 = _mm512_cvtepi32_ps(_mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(rgba, 1)));
        __m512 rgba2 = _mm512_cvtepi32_ps(_mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(rgba, 2)));
        __m512 rgba3 = _mm512_cvtepi32_ps(_mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(rgba, 3)));

        avx512RGBATranspose_4x4_4x4_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }

    static inline void LoadMasked(const uint8_t *in, __m512& r, __m512& g, __m512& b, __m512& a, uint32_t pixel_count)
    {
        __mmask16 k;
        uint16_t mask = 0;
        for (uint32_t i = 0; i < pixel_count; i++) {
            mask = (mask << 1) | 1;
        }

        k = _mm512_int2mask(mask);
        __m512i rgba = _mm512_maskz_loadu_epi32(k, (const __m512i*)in);

        __m512 rgba0 = _mm512_cvtepi32_ps(_mm512_cvtepu8_epi32(_mm512_castsi512_si128(rgba)));
        __m512 rgba1 = _mm512_cvtepi32_ps(_mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(rgba, 1)));
        __m512 rgba2 = _mm512_cvtepi32_ps(_mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(rgba, 2)));
        __m512 rgba3 = _mm512_cvtepi32_ps(_mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(rgba, 3)));

        avx512RGBATranspose_4x4_4x4_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }

    static inline void Store(uint8_t *out, __m512 r, __m512 g, __m512 b, __m512 a)
    {
        const __m512 maxValue = _mm512_set1_ps(255.0f);
        __m512 rgba0, rgba1,rgba2, rgba3;

        r = av512_clamp(r, maxValue);
        g = av512_clamp(g, maxValue);
        b = av512_clamp(b, maxValue);
        a = av512_clamp(a, maxValue);

        avx512RGBATranspose_4x4_4x4_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        __mmask16 k = _mm512_int2mask(0xFFFF);
        _mm512_mask_cvtepi32_storeu_epi8(out+0,  k, _mm512_cvtps_epi32(rgba0));
        _mm512_mask_cvtepi32_storeu_epi8(out+16, k, _mm512_cvtps_epi32(rgba1));
        _mm512_mask_cvtepi32_storeu_epi8(out+32, k, _mm512_cvtps_epi32(rgba2));
        _mm512_mask_cvtepi32_storeu_epi8(out+48, k, _mm512_cvtps_epi32(rgba3));
    }

    static inline void StoreMasked(uint8_t *out, __m512 r, __m512 g, __m512 b, __m512 a, uint32_t pixel_count)
    {
        const __m512 maxValue = _mm512_set1_ps(255.0f);
        __m512 rgba0, rgba1,rgba2, rgba3;

        __mmask16 k;
        uint64_t mask = 0;
        for (uint32_t i = 0; i < pixel_count; i++) {
            mask = (mask << 4) | 0b1111;
        }

        r = av512_clamp(r, maxValue);
        g = av512_clamp(g, maxValue);
        b = av512_clamp(b, maxValue);
        a = av512_clamp(a, maxValue);

        avx512RGBATranspose_4x4_4x4_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        k = _mm512_int2mask((mask >> 0) & 0xFFFF);
        _mm512_mask_cvtepi32_storeu_epi8(out+0,  k, _mm512_cvtps_epi32(rgba0));
        k = _mm512_int2mask((mask >> 16) & 0xFFFF);
        _mm512_mask_cvtepi32_storeu_epi8(out+16, k, _mm512_cvtps_epi32(rgba1));
        k = _mm512_int2mask((mask >> 32) & 0xFFFF);
        _mm512_mask_cvtepi32_storeu_epi8(out+32, k, _mm512_cvtps_epi32(rgba2));
        k = _mm512_int2mask((mask >> 48) & 0xFFFF);
        _mm512_mask_cvtepi32_storeu_epi8(out+48, k, _mm512_cvtps_epi32(rgba3));
    }
};

template<BitDepth BD>
struct AVX512RGBAPack16
{
    typedef typename BitDepthInfo<BD>::Type Type;

    static inline void Load(const Type *in, __m512& r, __m512& g, __m512& b, __m512& a)
    {
        __m512i rgba_00_07 = _mm512_loadu_si512((const __m512i*)(in +  0));
        __m512i rgba_08_15 = _mm512_loadu_si512((const __m512i*)(in + 32));

        __m512 rgba0 = _mm512_cvtepi32_ps(_mm512_cvtepu16_epi32(_mm512_castsi512_si256(rgba_00_07)));
        __m512 rgba1 = _mm512_cvtepi32_ps(_mm512_cvtepu16_epi32(_mm512_extracti64x4_epi64 (rgba_00_07, 1)));
        __m512 rgba2 = _mm512_cvtepi32_ps(_mm512_cvtepu16_epi32(_mm512_castsi512_si256(rgba_08_15)));
        __m512 rgba3 = _mm512_cvtepi32_ps(_mm512_cvtepu16_epi32(_mm512_extracti64x4_epi64 (rgba_08_15, 1)));

        avx512RGBATranspose_4x4_4x4_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);

    }

    static inline void LoadMasked(const Type *in, __m512& r, __m512& g, __m512& b, __m512& a, uint32_t pixel_count)
    {
        __mmask16 k;
        uint32_t mask = 0;
        for (uint32_t i = 0; i < pixel_count; i++) {
            mask = (mask << 2) | 0b11;
        }

        k = _mm512_int2mask((mask >> 0) & 0xFFFF);
        __m512i rgba_00_07 = _mm512_maskz_loadu_epi32(k, (const __m512i*)(in +  0));
        k = _mm512_int2mask((mask >> 16) & 0xFFFF);
        __m512i rgba_08_15 = _mm512_maskz_loadu_epi32(k, (const __m512i*)(in + 32));

        __m512 rgba0 = _mm512_cvtepi32_ps(_mm512_cvtepu16_epi32(_mm512_castsi512_si256(rgba_00_07)));
        __m512 rgba1 = _mm512_cvtepi32_ps(_mm512_cvtepu16_epi32(_mm512_extracti64x4_epi64 (rgba_00_07, 1)));
        __m512 rgba2 = _mm512_cvtepi32_ps(_mm512_cvtepu16_epi32(_mm512_castsi512_si256(rgba_08_15)));
        __m512 rgba3 = _mm512_cvtepi32_ps(_mm512_cvtepu16_epi32(_mm512_extracti64x4_epi64 (rgba_08_15, 1)));

        avx512RGBATranspose_4x4_4x4_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);

    }

    static inline void Store(Type *out, __m512 r, __m512 g, __m512 b, __m512 a)
    {
        const __m512 maxValue = _mm512_set1_ps((float)BitDepthInfo<BD>::maxValue);
        __m512 rgba0, rgba1,rgba2, rgba3;

        r = av512_clamp(r, maxValue);
        g = av512_clamp(g, maxValue);
        b = av512_clamp(b, maxValue);
        a = av512_clamp(a, maxValue);

        avx512RGBATranspose_4x4_4x4_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        __mmask16 k = _mm512_int2mask(0xFFFF);
        _mm512_mask_cvtepi32_storeu_epi16(out+0,  k, _mm512_cvtps_epi32(rgba0));
        _mm512_mask_cvtepi32_storeu_epi16(out+16, k, _mm512_cvtps_epi32(rgba1));
        _mm512_mask_cvtepi32_storeu_epi16(out+32, k, _mm512_cvtps_epi32(rgba2));
        _mm512_mask_cvtepi32_storeu_epi16(out+48, k, _mm512_cvtps_epi32(rgba3));

    }

    static inline void StoreMasked(Type *out, __m512 r, __m512 g, __m512 b, __m512 a, uint32_t pixel_count)
    {
        const __m512 maxValue = _mm512_set1_ps((float)BitDepthInfo<BD>::maxValue);
        __m512 rgba0, rgba1,rgba2, rgba3;

        __mmask16 k;
        uint64_t mask = 0;
        for (uint32_t i = 0; i < pixel_count; i++) {
            mask = (mask << 4) | 0b1111;
        }

        r = av512_clamp(r, maxValue);
        g = av512_clamp(g, maxValue);
        b = av512_clamp(b, maxValue);
        a = av512_clamp(a, maxValue);

        avx512RGBATranspose_4x4_4x4_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        k = _mm512_int2mask((mask >> 0) & 0xFFFF);
        _mm512_mask_cvtepi32_storeu_epi16(out+0,  k, _mm512_cvtps_epi32(rgba0));
        k = _mm512_int2mask((mask >> 16) & 0xFFFF);
        _mm512_mask_cvtepi32_storeu_epi16(out+16, k, _mm512_cvtps_epi32(rgba1));
        k = _mm512_int2mask((mask >> 32) & 0xFFFF);
        _mm512_mask_cvtepi32_storeu_epi16(out+32, k, _mm512_cvtps_epi32(rgba2));
        k = _mm512_int2mask((mask >> 48) & 0xFFFF);
        _mm512_mask_cvtepi32_storeu_epi16(out+48, k, _mm512_cvtps_epi32(rgba3));

    }
};

template <>
struct AVX512RGBAPack<BIT_DEPTH_UINT10>
{
    static inline void Load(const uint16_t *in, __m512& r, __m512& g, __m512& b, __m512& a)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT10>::Load(in, r, g, b, a);
    }
    static inline void LoadMasked(const uint16_t *in, __m512& r, __m512& g, __m512& b, __m512& a, uint32_t pixel_count)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT10>::LoadMasked(in, r, g, b, a, pixel_count);
    }
    static inline void Store(uint16_t *out, __m512 r, __m512 g, __m512 b, __m512 a)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT10>::Store(out, r, g, b, a);
    }
    static inline void StoreMasked(uint16_t *out, __m512 r, __m512 g, __m512 b, __m512 a, uint32_t pixel_count)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT10>::StoreMasked(out, r, g, b, a, pixel_count);
    }
    
};

template <>
struct AVX512RGBAPack<BIT_DEPTH_UINT12>
{
    static inline void Load(const uint16_t *in, __m512& r, __m512& g, __m512& b, __m512& a)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT12>::Load(in, r, g, b, a);
    }
    static inline void LoadMasked(const uint16_t *in, __m512& r, __m512& g, __m512& b, __m512& a,  uint32_t pixel_count)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT12>::LoadMasked(in, r, g, b, a, pixel_count);
    }
    static inline  void Store(uint16_t *out, __m512 r, __m512 g, __m512 b, __m512 a)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT12>::Store(out, r, g, b, a);
    }
    static inline void StoreMasked(uint16_t *out, __m512 r, __m512 g, __m512 b, __m512 a, uint32_t pixel_count)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT12>::StoreMasked(out, r, g, b, a, pixel_count);
    }
};

template <>
struct AVX512RGBAPack<BIT_DEPTH_UINT16>
{
    static inline void Load(const uint16_t *in, __m512& r, __m512& g, __m512& b, __m512& a)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT16>::Load(in, r, g, b, a);
    }
    static inline void LoadMasked(const uint16_t *in, __m512& r, __m512& g, __m512& b, __m512& a,  uint32_t pixel_count)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT16>::LoadMasked(in, r, g, b, a, pixel_count);
    }
    static inline  void Store(uint16_t *out, __m512 r, __m512 g, __m512 b, __m512 a)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT16>::Store(out, r, g, b, a);
    }
    static inline void StoreMasked(uint16_t *out, __m512 r, __m512 g, __m512 b, __m512 a, uint32_t pixel_count)
    {
        AVX512RGBAPack16<BIT_DEPTH_UINT16>::StoreMasked(out, r, g, b, a, pixel_count);
    }
};

template <>
struct AVX512RGBAPack<BIT_DEPTH_F16>
{
    static inline void Load(const half *in, __m512& r, __m512& g, __m512& b, __m512& a)
    {
        __m512i rgba_00_07 = _mm512_loadu_si512((const __m512i*)(in +  0));
        __m512i rgba_08_15 = _mm512_loadu_si512((const __m512i*)(in + 32));

        __m512 rgba0 = _mm512_cvtph_ps(_mm512_castsi512_si256(rgba_00_07));
        __m512 rgba1 = _mm512_cvtph_ps(_mm512_extracti64x4_epi64(rgba_00_07, 1));

        __m512 rgba2 = _mm512_cvtph_ps(_mm512_castsi512_si256(rgba_08_15));
        __m512 rgba3 = _mm512_cvtph_ps(_mm512_extracti64x4_epi64(rgba_08_15, 1));

        avx512RGBATranspose_4x4_4x4_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);

    }

    static inline void LoadMasked(const half *in, __m512& r, __m512& g, __m512& b, __m512& a, uint32_t pixel_count)
    {
        __mmask16 k;
        uint32_t mask = 0;
        for (uint32_t i = 0; i < pixel_count; i++) {
            mask = (mask << 2) | 0b11;
        }

        k = _mm512_int2mask((mask >> 0) & 0xFFFF);
        __m512i rgba_00_07 = _mm512_maskz_loadu_epi32(k, (const __m512i*)(in +  0));
        k = _mm512_int2mask((mask >> 16) & 0xFFFF);
        __m512i rgba_08_15 = _mm512_maskz_loadu_epi32(k, (const __m512i*)(in + 32));

        __m512 rgba0 = _mm512_cvtph_ps(_mm512_castsi512_si256(rgba_00_07));
        __m512 rgba1 = _mm512_cvtph_ps(_mm512_extracti64x4_epi64(rgba_00_07, 1));

        __m512 rgba2 = _mm512_cvtph_ps(_mm512_castsi512_si256(rgba_08_15));
        __m512 rgba3 = _mm512_cvtph_ps(_mm512_extracti64x4_epi64(rgba_08_15, 1));

        avx512RGBATranspose_4x4_4x4_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);

    }

    static inline  void Store(half *out, __m512 r, __m512 g, __m512 b, __m512 a)
    {
        __m512 rgba0, rgba1,rgba2, rgba3;

        avx512RGBATranspose_4x4_4x4_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        __m512i rgba0i = _mm512_inserti64x4(_mm512_castsi256_si512(_mm512_cvtps_ph(rgba0, 0)), _mm512_cvtps_ph(rgba1, 0), 1);
        __m512i rgba1i = _mm512_inserti64x4(_mm512_castsi256_si512(_mm512_cvtps_ph(rgba2, 0)), _mm512_cvtps_ph(rgba3, 0), 1);

        _mm512_storeu_si512((__m512i*)(out +  0), rgba0i);
        _mm512_storeu_si512((__m512i*)(out + 32), rgba1i);
    }

    static inline  void StoreMasked(half *out, __m512 r, __m512 g, __m512 b, __m512 a, uint32_t pixel_count)
    {
        __m512 rgba0, rgba1,rgba2, rgba3;

        __mmask16 k;
        uint64_t mask = 0;
        for (uint32_t i = 0; i < pixel_count; i++) {
            mask = (mask << 2) | 0b11;
        }

        avx512RGBATranspose_4x4_4x4_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        __m512i rgba0i = _mm512_inserti64x4(_mm512_castsi256_si512(_mm512_cvtps_ph(rgba0, 0)), _mm512_cvtps_ph(rgba1, 0), 1);
        __m512i rgba1i = _mm512_inserti64x4(_mm512_castsi256_si512(_mm512_cvtps_ph(rgba2, 0)), _mm512_cvtps_ph(rgba3, 0), 1);

        k = _mm512_int2mask((mask >> 0) & 0xFFFF);
        _mm512_mask_storeu_epi32((__m512i*)(out +  0), k, rgba0i);
        k = _mm512_int2mask((mask >> 16) & 0xFFFF);
        _mm512_mask_storeu_epi32((__m512i*)(out + 32), k, rgba1i);
    }
};

template <>
struct AVX512RGBAPack<BIT_DEPTH_F32>
{
    static inline void Load(const float *in, __m512& r, __m512& g, __m512& b, __m512& a)
    {
        __m512 rgba0 = _mm512_loadu_ps(in +  0);
        __m512 rgba1 = _mm512_loadu_ps(in + 16);
        __m512 rgba2 = _mm512_loadu_ps(in + 32);
        __m512 rgba3 = _mm512_loadu_ps(in + 48);

        avx512RGBATranspose_4x4_4x4_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }

    static inline void LoadMasked(const float *in, __m512& r, __m512& g, __m512& b, __m512& a, uint32_t pixel_count)
    {
        __mmask16 k;
        uint64_t mask = 0;
        for (uint32_t i = 0; i < pixel_count; i++) {
            mask = (mask << 4) | 0b1111;
        }

        k = _mm512_int2mask((mask >> 0) & 0xFFFF);
        __m512 rgba0 = _mm512_maskz_loadu_ps(k, in +  0);
        k = _mm512_int2mask((mask >> 16) & 0xFFFF);
        __m512 rgba1 = _mm512_maskz_loadu_ps(k, in + 16);
        k = _mm512_int2mask((mask >> 32) & 0xFFFF);
        __m512 rgba2 = _mm512_maskz_loadu_ps(k, in + 32);
        k = _mm512_int2mask((mask >> 48) & 0xFFFF);
        __m512 rgba3 = _mm512_maskz_loadu_ps(k, in + 48);

        avx512RGBATranspose_4x4_4x4_4x4_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }

    static inline void Store(float *out, __m512 r, __m512 g, __m512 b, __m512 a)
    {
        __m512 rgba0, rgba1,rgba2, rgba3;

        avx512RGBATranspose_4x4_4x4_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        _mm512_storeu_ps((__m512*)(out+0),  rgba0);
        _mm512_storeu_ps((__m512*)(out+16), rgba1);
        _mm512_storeu_ps((__m512*)(out+32), rgba2);
        _mm512_storeu_ps((__m512*)(out+48), rgba3);
    }

    static inline void StoreMasked(float *out, __m512 r, __m512 g, __m512 b, __m512 a, uint32_t pixel_count)
    {
        __m512 rgba0, rgba1,rgba2, rgba3;

        __mmask16 k;
        uint64_t mask = 0;
        for (uint32_t i = 0; i < pixel_count; i++) {
            mask = (mask << 4) | 0b1111;
        }

        avx512RGBATranspose_4x4_4x4_4x4_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        k = _mm512_int2mask((mask >> 0) & 0xFFFF);
        _mm512_mask_storeu_ps((__m512*)(out+0), k, rgba0);
        k = _mm512_int2mask((mask >> 16) & 0xFFFF);
        _mm512_mask_storeu_ps((__m512*)(out+16), k, rgba1);
        k = _mm512_int2mask((mask >> 32) & 0xFFFF);
        _mm512_mask_storeu_ps((__m512*)(out+32), k, rgba2);
        k = _mm512_int2mask((mask >> 48) & 0xFFFF);
        _mm512_mask_storeu_ps((__m512*)(out+48), k, rgba3);
    }
};

} // namespace OCIO_NAMESPACE

#endif // OCIO_USE_AVX512
#endif // INCLUDED_OCIO_AVX512_H