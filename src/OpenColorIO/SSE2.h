// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_SSE2_H
#define INCLUDED_OCIO_SSE2_H

#include "CPUInfo.h"
#if OCIO_USE_SSE2

// Include the appropriate SIMD intrinsics header based on the architecture (Intel vs. ARM).
#if !defined(__aarch64__)
    #include <emmintrin.h>
#elif defined(__aarch64__)
    // ARM architecture A64 (ARM64)
    #if OCIO_USE_SSE2NEON
        #include <sse2neon.h>
    #endif
#endif

#include <stdio.h>

#include <OpenColorIO/OpenColorIO.h>
#include "BitDepthUtils.h"

// Macros for alignment declarations
#define SSE2_SIMD_BYTES 16
#define SSE2_ALIGN(decl) alignas(SSE2_SIMD_BYTES) decl

namespace OCIO_NAMESPACE
{
// Note that it is important for the code below this ifdef stays in the OCIO_NAMESPACE since
// it is redefining two of the functions from sse2neon.

#if defined(__aarch64__)
    #if OCIO_USE_SSE2NEON
        // Using vmaxnmq_f32 and vminnmq_f32 rather than sse2neon's vmaxq_f32 and vminq_f32 due to 
        // NaN handling. This doesn't seem to be significantly slower than the default sse2neon behavior.

        // With the Intel intrinsics, if one value is a NaN, the second argument is output, as if it were 
        // a simple (a>b) ? a:b. OCIO sometimes uses this behavior to filter out a possible NaN in the 
        // first argument. The vmaxq/vminq will return a NaN if either input is a NaN, which omits the 
        // filtering behavior. The vmaxnmq/vminnmq (similar to std::fmax/fmin) are not quite the same as 
        // the Intel _mm_max_ps / _mm_min_ps since they always return the non-NaN argument 
        // (for quiet NaNs, signaling NaNs always get returned), but that's fine for OCIO since a NaN in 
        // the first argument continues to be filtered out.
        static inline __m128 _mm_max_ps(__m128 a, __m128 b)
        {
            return vreinterpretq_m128_f32(
                vmaxnmq_f32(vreinterpretq_f32_m128(a), vreinterpretq_f32_m128(b)));
        }
        static inline __m128 _mm_min_ps(__m128 a, __m128 b)
        {
            return vreinterpretq_m128_f32(
                vminnmq_f32(vreinterpretq_f32_m128(a), vreinterpretq_f32_m128(b)));
        }
    #endif
#endif

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

static inline __m128i sse2_blendv(__m128i a, __m128i b, __m128i mask)
{
    return _mm_xor_si128(_mm_and_si128(_mm_xor_si128(a, b), mask), a);
}

static inline __m128i sse2_cvtps_ph(__m128 a)
{
    __m128i x = _mm_castps_si128(a);

    __m128i x_sgn = _mm_and_si128(x, _mm_set1_epi32(0x80000000u));
    __m128i x_exp = _mm_and_si128(x, _mm_set1_epi32(0x7f800000u));

    __m128 magic1 = _mm_castsi128_ps(_mm_set1_epi32(0x77800000u)); // 0x1.0p+112f
    __m128 magic2 = _mm_castsi128_ps(_mm_set1_epi32(0x08800000u)); // 0x1.0p-110f

    // sse2 doesn't have _mm_max_epu32, but _mm_max_ps works
    __m128i exp_max = _mm_set1_epi32(0x38800000u);
    x_exp = _mm_castps_si128(_mm_max_ps(_mm_castsi128_ps(x_exp), _mm_castsi128_ps(exp_max))); // max(e, -14)
    x_exp = _mm_add_epi32(x_exp, _mm_set1_epi32(15u << 23)); // e += 15
    x = _mm_and_si128(x, _mm_set1_epi32(0x7fffffffu)); // Discard sign

    __m128 f = _mm_castsi128_ps(x);
    __m128 magicf = _mm_castsi128_ps(x_exp);

    // If 15 < e then inf, otherwise e += 2
    f = _mm_mul_ps(_mm_mul_ps(f, magic1), magic2);
    f = _mm_add_ps(f, magicf);

    __m128i u = _mm_castps_si128(f);

    __m128i h_exp = _mm_and_si128(_mm_srli_epi32(u, 13), _mm_set1_epi32(0x7c00u));
    __m128i h_sig = _mm_and_si128(u, _mm_set1_epi32(0x0fffu));

    // blend in nan values only if present
    __m128i nan_mask = _mm_cmpgt_epi32(x, _mm_set1_epi32(0x7f800000u));
    if (_mm_movemask_epi8(nan_mask)) {
        __m128i nan = _mm_and_si128(_mm_srli_epi32(x, 13), _mm_set1_epi32(0x03FFu));
        nan = _mm_or_si128(_mm_set1_epi32(0x0200u), nan);
        h_sig = sse2_blendv(h_sig, nan, nan_mask);
    }

    __m128i ph = _mm_add_epi32(_mm_srli_epi32(x_sgn, 16),_mm_add_epi32(h_exp, h_sig));

    // pack u16 values into lower 64 bits
    ph = _mm_shufflehi_epi16(ph, (1 << 6 | 1 << 4 | 2 << 2 | 0 << 0));
    ph = _mm_shufflelo_epi16(ph, (1 << 6 | 1 << 4 | 2 << 2 | 0 << 0));
    return _mm_shuffle_epi32(ph, (3 << 6 | 3 << 4 | 2 << 2 | 0 << 0));
}

static inline __m128 sse2_cvtph_ps(__m128i a)
{
    __m128 magic      = _mm_castsi128_ps(_mm_set1_epi32((254 - 15) << 23));
    __m128 was_infnan = _mm_castsi128_ps(_mm_set1_epi32((127 + 16) << 23));
    __m128 sign;
    __m128 o;

    // the values to unpack are in the lower 64 bits
    // | 0 1 | 2 3 | 4 5 | 6 7 | 8 9 | 10 11 | 12 13 | 14 15
    // | 0 1 | 0 1 | 2 3 | 2 3 | 4 5 |  4  5 | 6   7 | 6   7
    a = _mm_unpacklo_epi16(a, a);

    // extract sign
    sign = _mm_castsi128_ps(_mm_slli_epi32(_mm_and_si128(a, _mm_set1_epi32(0x8000)), 16));

    // extract exponent/mantissa bits
    o = _mm_castsi128_ps(_mm_slli_epi32(_mm_and_si128(a, _mm_set1_epi32(0x7fff)), 13));

    // magic multiply
    o = _mm_mul_ps(o, magic);

    // blend in inf/nan values only if present
    __m128i mask = _mm_castps_si128(_mm_cmpge_ps(o, was_infnan));
    if (_mm_movemask_epi8(mask)) {
        __m128i ou =  _mm_castps_si128(o);
        __m128i ou_nan = _mm_or_si128(ou, _mm_set1_epi32( 0x01FF << 22));
        __m128i ou_inf = _mm_or_si128(ou, _mm_set1_epi32( 0x00FF << 23));

        // blend in nans
        ou = sse2_blendv(ou, ou_nan, mask);

        // blend in infinities
        mask = _mm_cmpeq_epi32( _mm_castps_si128(o), _mm_castps_si128(was_infnan));
        o  = _mm_castsi128_ps(sse2_blendv(ou, ou_inf, mask));
    }

    return  _mm_or_ps(o, sign);
}

// Note Packing functions perform no 0.0 - 1.0 normalization
// but perform 0 - max value clamping for integer formats
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

template <>
struct SSE2RGBAPack<BIT_DEPTH_F16>
{
    static inline void Load(const half *in, __m128& r, __m128& g, __m128& b, __m128& a)
    {
        __m128i rgba_00_01 = _mm_loadu_si128((const __m128i*)(in + 0));
        __m128i rgba_02_03 = _mm_loadu_si128((const __m128i*)(in + 8));

        __m128 rgba0 = sse2_cvtph_ps(rgba_00_01);
        __m128 rgba1 = sse2_cvtph_ps(_mm_shuffle_epi32(rgba_00_01, _MM_SHUFFLE(1,0,3,2)));
        __m128 rgba2 = sse2_cvtph_ps(rgba_02_03);
        __m128 rgba3 = sse2_cvtph_ps(_mm_shuffle_epi32(rgba_02_03, _MM_SHUFFLE(1,0,3,2)));

        sse2RGBATranspose_4x4(rgba0, rgba1, rgba2, rgba3, r, g, b, a);
    }

    static inline  void Store(half *out, __m128 r, __m128 g, __m128 b, __m128 a)
    {
        __m128 rgba0, rgba1, rgba2, rgba3;
        __m128i rgba;

        sse2RGBATranspose_4x4(r, g, b, a, rgba0, rgba1, rgba2, rgba3);

        __m128i rgba00_01 = sse2_cvtps_ph(rgba0);
        __m128i rgba02_03 = sse2_cvtps_ph(rgba1);
        __m128i rgba04_05 = sse2_cvtps_ph(rgba2);
        __m128i rgba06_07 = sse2_cvtps_ph(rgba3);

        rgba = _mm_xor_si128(rgba00_01, _mm_shuffle_epi32(rgba02_03, _MM_SHUFFLE(1,0,3,2)));
        _mm_storeu_si128((__m128i*)(out+0), rgba);

        rgba = _mm_xor_si128(rgba04_05, _mm_shuffle_epi32(rgba06_07, _MM_SHUFFLE(1,0,3,2)));
        _mm_storeu_si128((__m128i*)(out+8), rgba);
    }
};

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