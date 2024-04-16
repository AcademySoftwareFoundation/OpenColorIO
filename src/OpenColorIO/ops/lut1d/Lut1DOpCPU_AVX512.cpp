// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "Lut1DOpCPU_AVX512.h"
#if OCIO_USE_AVX512

#include <immintrin.h>
#include <string.h>

#include "AVX512.h"

namespace OCIO_NAMESPACE
{

namespace {


static inline __m512 apply_lut_avx512(const float *lut, __m512 v, const __m512& scale, const __m512& lut_max)
{
    __m512 zero   = _mm512_setzero_ps();
    __m512 one_f  = _mm512_set1_ps(1);

    __m512 scaled = _mm512_mul_ps(v, scale);

    // clamp, max first, NAN set to zero
    __m512 x      = _mm512_min_ps(_mm512_max_ps(scaled, zero), lut_max);
    __m512 prev_f = _mm512_floor_ps(x);
    __m512 d      = _mm512_sub_ps(x, prev_f);
    __m512 next_f = _mm512_min_ps(_mm512_add_ps(prev_f, one_f), lut_max);

    __m512i prev_i = _mm512_cvttps_epi32(prev_f);
    __m512i next_i = _mm512_cvttps_epi32(next_f);

    __m512 p = _mm512_i32gather_ps(prev_i, lut, sizeof(float));
    __m512 n = _mm512_i32gather_ps(next_i, lut, sizeof(float));

    // lerp: a + (b - a) * t;
    v = _mm512_fmadd_ps(_mm512_sub_ps(n, p), d, p);

    return v;
}

template <BitDepth inBD, BitDepth outBD>
static inline void linear1D(const float *lutR, const float *lutG,const float *lutB, int dim, const void *inImg, void *outImg, long numPixels)
{

    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType *src = (const InType*)inImg;
    OutType *dst = (OutType*)outImg;
    __m512 r,g,b,a, alpha_scale;

    float rgb_scale = 1.0f / (float)BitDepthInfo<inBD>::maxValue  * ((float)dim -1);
    const __m512 lut_scale = _mm512_set1_ps(rgb_scale);
    const __m512 lut_max   = _mm512_set1_ps((float)dim -1);

    if (inBD != outBD)
        alpha_scale = _mm512_set1_ps((float)BitDepthInfo<outBD>::maxValue / (float)BitDepthInfo<inBD>::maxValue);

    int pixel_count = numPixels / 16 * 16;
    int remainder = numPixels - pixel_count;

    for (int i = 0; i < pixel_count; i += 16 ) {
        AVX512RGBAPack<inBD>::Load(src, r, g, b, a);

        r = apply_lut_avx512(lutR, r, lut_scale, lut_max);
        g = apply_lut_avx512(lutG, g, lut_scale, lut_max);
        b = apply_lut_avx512(lutB, b, lut_scale, lut_max);

        if (inBD != outBD)
            a = _mm512_mul_ps(a, alpha_scale);

        AVX512RGBAPack<outBD>::Store(dst, r, g, b, a);

        src += 64;
        dst += 64;
    }

     // handler leftovers pixels
    if (remainder) {
        AVX512RGBAPack<inBD>::LoadMasked(src, r, g, b, a, remainder);

        r = apply_lut_avx512(lutR, r, lut_scale, lut_max);
        g = apply_lut_avx512(lutG, g, lut_scale, lut_max);
        b = apply_lut_avx512(lutB, b, lut_scale, lut_max);

        if (inBD != outBD)
            a = _mm512_mul_ps(a, alpha_scale);

        AVX512RGBAPack<outBD>::StoreMasked(dst, r, g, b, a, remainder);
    }
}

template<BitDepth inBD>
inline Lut1DOpCPUApplyFunc * GetConvertInBitDepth(BitDepth outBD)
{
    switch(outBD)
    {
        case BIT_DEPTH_UINT8:
            return linear1D<inBD, BIT_DEPTH_UINT8>;
        case BIT_DEPTH_UINT10:
            return linear1D<inBD, BIT_DEPTH_UINT10>;
        case BIT_DEPTH_UINT12:
            return linear1D<inBD, BIT_DEPTH_UINT12>;
        case BIT_DEPTH_UINT16:
            return linear1D<inBD, BIT_DEPTH_UINT16>;
        case BIT_DEPTH_F16:
            return linear1D<inBD, BIT_DEPTH_F16>;
        case BIT_DEPTH_F32:
            return linear1D<inBD, BIT_DEPTH_F32>;
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
            break;
    }

    return nullptr;
}

} // anonymous namespace

Lut1DOpCPUApplyFunc * AVX512GetLut1DApplyFunc(BitDepth inBD, BitDepth outBD)
{

    // Lut1DOp only uses interpolation for in float in formats
    switch(inBD)
    {
        case BIT_DEPTH_UINT8:
        case BIT_DEPTH_UINT10:
        case BIT_DEPTH_UINT12:
        case BIT_DEPTH_UINT16:
        case BIT_DEPTH_F16:
            break;
        case BIT_DEPTH_F32:
            return GetConvertInBitDepth<BIT_DEPTH_F32>(outBD);
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
            break;
    }

    return nullptr;
}

} // OCIO_NAMESPACE

#endif // OCIO_USE_AVX512