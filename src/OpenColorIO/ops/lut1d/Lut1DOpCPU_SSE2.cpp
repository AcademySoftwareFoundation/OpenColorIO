// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "Lut1DOpCPU_SSE2.h"

#if OCIO_USE_SSE2

#include <string.h>

#include "SSE2.h"

namespace OCIO_NAMESPACE
{

namespace {

#define i32gather_ps_sse2(src, dst, idx, indices, buffer)  \
    _mm_store_si128((__m128i *)indices, idx); \
    buffer[0] = (src)[indices[0]];               \
    buffer[1] = (src)[indices[1]];               \
    buffer[2] = (src)[indices[2]];               \
    buffer[3] = (src)[indices[3]];               \
    dst = _mm_load_ps(buffer)

static inline __m128 fmadd_ps_sse2(__m128 a, __m128 b, __m128 c)
{
    return  _mm_add_ps(_mm_mul_ps(a, b), c);
}

static inline __m128 floor_ps_sse2(__m128 v)
{
    // NOTE: using truncate cvtt
    return _mm_cvtepi32_ps(_mm_cvttps_epi32(v));
}


static inline __m128 apply_lut_sse2(const float *lut, __m128 v, const __m128& scale, const __m128& lut_max)
{
    SSE2_ALIGN(uint32_t indices_p[4]);
    SSE2_ALIGN(uint32_t indices_n[4]);
    SSE2_ALIGN(float buffer_p[4]);
    SSE2_ALIGN(float buffer_n[4]);

    __m128 zero   = _mm_setzero_ps();
    __m128 one_f  = _mm_set1_ps(1);

    __m128 scaled = _mm_mul_ps(v, scale);

    // clamp, max first, NAN set to zero
    __m128 x      = _mm_min_ps(_mm_max_ps(scaled, zero), lut_max);
    __m128 prev_f = floor_ps_sse2(x);
    __m128 d      = _mm_sub_ps(x, prev_f);
    __m128 next_f = _mm_min_ps(_mm_add_ps(prev_f, one_f), lut_max);

    __m128i prev_i = _mm_cvttps_epi32(prev_f);
    __m128i next_i = _mm_cvttps_epi32(next_f);

    __m128 p, n;
    i32gather_ps_sse2(lut, p, prev_i, indices_p, buffer_p);
    i32gather_ps_sse2(lut, n, next_i, indices_n, buffer_n);

    // lerp: a + (b - a) * t;
    v = fmadd_ps_sse2(_mm_sub_ps(n, p), d, p);

    return v;
}

template <BitDepth inBD, BitDepth outBD>
static inline void linear1D(const float *lutR, const float *lutG,const float *lutB, int dim, const void *inImg, void *outImg, long numPixels)
{

    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType *src = (const InType*)inImg;
    OutType *dst = (OutType*)outImg;
    __m128 r,g,b,a, alpha_scale;

    float rgb_scale = 1.0f / (float)BitDepthInfo<inBD>::maxValue  * ((float)dim -1);
    const __m128 lut_scale = _mm_set1_ps(rgb_scale);
    const __m128 lut_max   = _mm_set1_ps((float)dim -1);

    if (inBD != outBD)
        alpha_scale = _mm_set1_ps((float)BitDepthInfo<outBD>::maxValue / (float)BitDepthInfo<inBD>::maxValue);

    int pixel_count = numPixels / 4 * 4;
    int remainder = numPixels - pixel_count;

    for (int i = 0; i < pixel_count; i += 4 ) {
        SSE2RGBAPack<inBD>::Load(src, r, g, b, a);

        r = apply_lut_sse2(lutR, r, lut_scale, lut_max);
        g = apply_lut_sse2(lutG, g, lut_scale, lut_max);
        b = apply_lut_sse2(lutB, b, lut_scale, lut_max);

        if (inBD != outBD)
            a = _mm_mul_ps(a, alpha_scale);

        SSE2RGBAPack<outBD>::Store(dst, r, g, b, a);

        src += 16;
        dst += 16;
    }

     // handler leftovers pixels
    if (remainder) {
        InType in_buf[16] = {};
        OutType out_buf[16];

        // memcpy(in_buf, src, remainder * 4 * sizeof(InType));
        for (int i = 0; i < remainder*4; i+=4)
        {
            in_buf[i + 0] = src[0];
            in_buf[i + 1] = src[1];
            in_buf[i + 2] = src[2];
            in_buf[i + 3] = src[3];
            src+=4;
        }

        SSE2RGBAPack<inBD>::Load(in_buf, r, g, b, a);

        r = apply_lut_sse2(lutR, r, lut_scale, lut_max);
        g = apply_lut_sse2(lutG, g, lut_scale, lut_max);
        b = apply_lut_sse2(lutB, b, lut_scale, lut_max);

        if (inBD != outBD)
            a = _mm_mul_ps(a, alpha_scale);

        SSE2RGBAPack<outBD>::Store(out_buf, r, g, b, a);
        // memcpy(dst, out_buf, remainder * 4 * sizeof(OutType));
        for (int i = 0; i < remainder*4; i+=4)
        {
            dst[0] = out_buf[i + 0];
            dst[1] = out_buf[i + 1];
            dst[2] = out_buf[i + 2];
            dst[3] = out_buf[i + 3];
            dst+=4;
        }

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

Lut1DOpCPUApplyFunc * SSE2GetLut1DApplyFunc(BitDepth inBD, BitDepth outBD)
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

#endif // OCIO_USE_SSE2