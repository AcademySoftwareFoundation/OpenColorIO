// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "Lut1DOpCPU_AVX2.h"
#if OCIO_USE_AVX2

#include <immintrin.h>
#include <string.h>

#include "AVX2.h"

namespace OCIO_NAMESPACE
{

namespace {


static inline __m256 apply_lut_avx2(const float *lut, __m256 v, const __m256& scale, const __m256& lut_max)
{
    __m256 zero   = _mm256_setzero_ps();
    __m256 one_f  = _mm256_set1_ps(1);

    __m256 scaled = _mm256_mul_ps(v, scale);

    // clamp, max first, NAN set to zero
    __m256 x      = _mm256_min_ps(_mm256_max_ps(scaled, zero), lut_max);
    __m256 prev_f = _mm256_floor_ps(x);
    __m256 d      = _mm256_sub_ps(x, prev_f);
    __m256 next_f = _mm256_min_ps(_mm256_add_ps(prev_f, one_f), lut_max);

    __m256i prev_i = _mm256_cvttps_epi32(prev_f);
    __m256i next_i = _mm256_cvttps_epi32(next_f);

    __m256 p = _mm256_i32gather_ps(lut, prev_i, sizeof(float));
    __m256 n = _mm256_i32gather_ps(lut, next_i, sizeof(float));

    // lerp: a + (b - a) * t;
    v = _mm256_fmadd_ps(_mm256_sub_ps(n, p), d, p);

    return v;
}

template <BitDepth inBD, BitDepth outBD>
static inline void linear1D(const float *lutR, const float *lutG,const float *lutB, int dim, const void *inImg, void *outImg, long numPixels)
{

    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType *src = (const InType*)inImg;
    OutType *dst = (OutType*)outImg;
    __m256 r,g,b,a, alpha_scale;

    float rgb_scale = 1.0f / (float)BitDepthInfo<inBD>::maxValue  * ((float)dim -1);
    const __m256 lut_scale = _mm256_set1_ps(rgb_scale);
    const __m256 lut_max   = _mm256_set1_ps((float)dim -1);

    if (inBD != outBD)
        alpha_scale = _mm256_set1_ps((float)BitDepthInfo<outBD>::maxValue / (float)BitDepthInfo<inBD>::maxValue);

    int pixel_count = numPixels / 8 * 8;
    int remainder = numPixels - pixel_count;

    for (int i = 0; i < pixel_count; i += 8 ) {
        AVX2RGBAPack<inBD>::Load(src, r, g, b, a);

        r = apply_lut_avx2(lutR, r, lut_scale, lut_max);
        g = apply_lut_avx2(lutG, g, lut_scale, lut_max);
        b = apply_lut_avx2(lutB, b, lut_scale, lut_max);

        if (inBD != outBD)
            a = _mm256_mul_ps(a, alpha_scale);

        AVX2RGBAPack<outBD>::Store(dst, r, g, b, a);

        src += 32;
        dst += 32;
    }

     // handler leftovers pixels
    if (remainder) {
        InType in_buf[32] = {};
        OutType out_buf[32];

        // memcpy(in_buf, src, remainder * 4 * sizeof(InType));
        for (int i = 0; i < remainder*4; i+=4)
        {
            in_buf[i + 0] = src[0];
            in_buf[i + 1] = src[1];
            in_buf[i + 2] = src[2];
            in_buf[i + 3] = src[3];
            src+=4;
        }

        AVX2RGBAPack<inBD>::Load(in_buf, r, g, b, a);

        r = apply_lut_avx2(lutR, r, lut_scale, lut_max);
        g = apply_lut_avx2(lutG, g, lut_scale, lut_max);
        b = apply_lut_avx2(lutB, b, lut_scale, lut_max);

        if (inBD != outBD)
            a = _mm256_mul_ps(a, alpha_scale);

        AVX2RGBAPack<outBD>::Store(out_buf, r, g, b, a);
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
#if OCIO_USE_F16C
            if (CPUInfo::instance().hasF16C())
                return linear1D<inBD, BIT_DEPTH_F16>;
            break;
#endif
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

Lut1DOpCPUApplyFunc * AVX2GetLut1DApplyFunc(BitDepth inBD, BitDepth outBD)
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

#endif // OCIO_USE_AVX2