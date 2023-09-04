// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "Lut3DOpCPU_AVX2.h"
#if OCIO_USE_AVX2

#include <immintrin.h>
#include <string.h>

#include "AVX2.h"

namespace OCIO_NAMESPACE
{
namespace {

struct Lut3DContextAVX2 {
    const float *lut;
    __m256 lutmax;
    __m256 lutsize;
    __m256 lutsize2;
};

struct rgbavec_avx2 {
    __m256 r, g, b, a;
};

#define gather_rgb_avx2(src, idx)                               \
    sample_r = _mm256_i32gather_ps(src+0, idx, 4);              \
    sample_g = _mm256_i32gather_ps(src+1, idx, 4);              \
    sample_b = _mm256_i32gather_ps(src+2, idx, 4)

static inline rgbavec_avx2 interp_tetrahedral_avx2(const Lut3DContextAVX2 &ctx, __m256& r, __m256& g, __m256& b, __m256& a)
{
    __m256 x0, x1, x2;
    __m256 cxxxa;
    __m256 cxxxb;
    __m256 mask;
    __m256 sample_r, sample_g, sample_b;

    rgbavec_avx2 result;

    __m256 lut_max  = ctx.lutmax;
    __m256 lutsize  = ctx.lutsize;
    __m256 lutsize2 = ctx.lutsize2;

    __m256 one_f    = _mm256_set1_ps(1.0f);
    __m256 four_f  = _mm256_set1_ps(4.0f);

    __m256 prev_r = _mm256_floor_ps(r);
    __m256 prev_g = _mm256_floor_ps(g);
    __m256 prev_b = _mm256_floor_ps(b);

    // rgb delta values
    __m256 d_r = _mm256_sub_ps(r, prev_r);
    __m256 d_g = _mm256_sub_ps(g, prev_g);
    __m256 d_b = _mm256_sub_ps(b, prev_b);

    __m256 next_r = _mm256_min_ps(lut_max, _mm256_add_ps(prev_r, one_f));
    __m256 next_g = _mm256_min_ps(lut_max, _mm256_add_ps(prev_g, one_f));
    __m256 next_b = _mm256_min_ps(lut_max, _mm256_add_ps(prev_b, one_f));

    // prescale indices
    prev_r = _mm256_mul_ps(prev_r, lutsize2);
    next_r = _mm256_mul_ps(next_r, lutsize2);

    prev_g = _mm256_mul_ps(prev_g, lutsize);
    next_g = _mm256_mul_ps(next_g, lutsize);

    prev_b = _mm256_mul_ps(prev_b, four_f);
    next_b = _mm256_mul_ps(next_b, four_f);

    // This is the tetrahedral blend equation
    // red = (1-x0) * c000.r + (x0-x1) * cxxxa.r + (x1-x2) * cxxxb.r + x2 * c111.r;
    // The x values are the rgb delta values sorted, x0 >= x1 >= x2
    // c### are samples from the lut, which are indices made with prev_(r,g,b) and next_(r,g,b) values
    // 0 = use prev, 1 = use next
    // c### = (prev_r or next_r) * (lutsize * lutsize) + (prev_g or next_g) * lutsize + (prev_b or next_b)

    // cxxxa
    // always uses 1 next and 2 prev and next is largest delta
    // r> == c100 == (r>g && r>b) == (!b>r && r>g)
    // g> == c010 == (g>r && g>b) == (!r>g && g>b)
    // b> == c001 == (b>r && b>g) == (!g>b && b>r)

    // cxxxb
    // always uses 2 next and 1 prev and prev is smallest delta
    // r< == c011 == (r<=g && r<=b) == (!r>g && b>r)
    // g< == c101 == (g<=r && g<=b) == (!g>b && r>g)
    // b< == c110 == (b<=r && b<=g) == (!b>r && g>b)

    // c000 and c111 are const (prev,prev,prev) and (next,next,next)

    __m256 gt_r = _mm256_cmp_ps(d_r, d_g, _CMP_GT_OQ); // r>g
    __m256 gt_g = _mm256_cmp_ps(d_g, d_b, _CMP_GT_OQ); // g>b
    __m256 gt_b = _mm256_cmp_ps(d_b, d_r, _CMP_GT_OQ); // b>r

    // r> !b>r && r>g
    mask = _mm256_andnot_ps(gt_b, gt_r);
    cxxxa = _mm256_blendv_ps(prev_r, next_r, mask);

    // r< !r>g && b>r
    mask = _mm256_andnot_ps(gt_r, gt_b);
    cxxxb = _mm256_blendv_ps(next_r, prev_r, mask);

    // g> !r>g && g>b
    mask = _mm256_andnot_ps(gt_r, gt_g);
    cxxxa = _mm256_add_ps(cxxxa, _mm256_blendv_ps(prev_g, next_g, mask));

    // g< !g>b && r>g
    mask = _mm256_andnot_ps(gt_g, gt_r);
    cxxxb = _mm256_add_ps(cxxxb, _mm256_blendv_ps(next_g, prev_g, mask));

    // b> !g>b && b>r
    mask = _mm256_andnot_ps(gt_g, gt_b);
    cxxxa = _mm256_add_ps(cxxxa, _mm256_blendv_ps(prev_b, next_b, mask));

    // b< !b>r && g>b
    mask = _mm256_andnot_ps(gt_b, gt_g);
    cxxxb = _mm256_add_ps(cxxxb, _mm256_blendv_ps(next_b, prev_b, mask));

    __m256 c000 = _mm256_add_ps(_mm256_add_ps(prev_r, prev_g), prev_b);
    __m256 c111 = _mm256_add_ps(_mm256_add_ps(next_r, next_g), next_b);

    // sort delta r,g,b x0 >= x1 >= x2
    __m256 rg_min = _mm256_min_ps(d_r, d_g);
    __m256 rg_max = _mm256_max_ps(d_r, d_g);

    x2         = _mm256_min_ps(rg_min, d_b);
    __m256 mid = _mm256_max_ps(rg_min, d_b);

    x0 = _mm256_max_ps(rg_max, d_b);
    x1 = _mm256_min_ps(rg_max, mid);

    // convert indices to int
    __m256i c000_idx  = _mm256_cvttps_epi32(c000);
    __m256i cxxxa_idx = _mm256_cvttps_epi32(cxxxa);
    __m256i cxxxb_idx = _mm256_cvttps_epi32(cxxxb);
    __m256i c111_idx  = _mm256_cvttps_epi32(c111);

    gather_rgb_avx2(ctx.lut, c000_idx);

    // (1-x0) * c000
    __m256 v = _mm256_sub_ps(one_f, x0);
    result.r = _mm256_mul_ps(sample_r, v);
    result.g = _mm256_mul_ps(sample_g, v);
    result.b = _mm256_mul_ps(sample_b, v);

    gather_rgb_avx2(ctx.lut, cxxxa_idx);

    // (x0-x1) * cxxxa
    v = _mm256_sub_ps(x0, x1);
    result.r = _mm256_fmadd_ps(v, sample_r, result.r);
    result.g = _mm256_fmadd_ps(v, sample_g, result.g);
    result.b = _mm256_fmadd_ps(v, sample_b, result.b);

    gather_rgb_avx2(ctx.lut, cxxxb_idx);

    // (x1-x2) * cxxxb
    v = _mm256_sub_ps(x1, x2);
    result.r = _mm256_fmadd_ps(v, sample_r, result.r);
    result.g = _mm256_fmadd_ps(v, sample_g, result.g);
    result.b = _mm256_fmadd_ps(v, sample_b, result.b);

    gather_rgb_avx2(ctx.lut, c111_idx);

    // x2 * c111
    result.r = _mm256_fmadd_ps(x2, sample_r, result.r);
    result.g = _mm256_fmadd_ps(x2, sample_g, result.g);
    result.b = _mm256_fmadd_ps(x2, sample_b, result.b);

    result.a = a;

    return result;
}

template<BitDepth inBD, BitDepth outBD>
inline void applyTetrahedralAVX2Func(const float *lut3d, int dim, const void *inImg, void *outImg, int numPixels)
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType * src = (InType *)inImg;
    OutType * dst = (OutType *)outImg;
    __m256 r,g,b,a;
    rgbavec_avx2 c;

    Lut3DContextAVX2 ctx;

    float lutmax = (float)dim - 1;
    __m256 scale   = _mm256_set1_ps(lutmax);
    __m256 zero    = _mm256_setzero_ps();

    ctx.lut      = lut3d;
    ctx.lutmax   = _mm256_set1_ps(lutmax);
    ctx.lutsize  = _mm256_set1_ps((float)dim * 4);
    ctx.lutsize2 = _mm256_set1_ps((float)dim * dim * 4);

    int pixel_count = numPixels / 8 * 8;
    int remainder = numPixels - pixel_count;

    for (int i = 0; i < pixel_count; i += 8 )
    {
        AVX2RGBAPack<inBD>::Load(src, r, g, b, a);

        // scale and clamp values
        r = _mm256_mul_ps(r, scale);
        g = _mm256_mul_ps(g, scale);
        b = _mm256_mul_ps(b, scale);

        r = _mm256_max_ps(r, zero);
        g = _mm256_max_ps(g, zero);
        b = _mm256_max_ps(b, zero);

        r = _mm256_min_ps(r, ctx.lutmax);
        g = _mm256_min_ps(g, ctx.lutmax);
        b = _mm256_min_ps(b, ctx.lutmax);

        c = interp_tetrahedral_avx2(ctx, r, g, b, a);

        AVX2RGBAPack<outBD>::Store(dst, c.r, c.g, c.b, c.a);

        src += 32;
        dst += 32;
    }

     // handler leftovers pixels
    if (remainder)
    {
        InType in_buf[32] = {};
        OutType out_buf[32];

        for (int i = 0; i < remainder*4; i+=4)
        {
            in_buf[i + 0] = src[0];
            in_buf[i + 1] = src[1];
            in_buf[i + 2] = src[2];
            in_buf[i + 3] = src[3];
            src+=4;
        }

        AVX2RGBAPack<inBD>::Load(in_buf, r, g, b, a);

        // scale and clamp values
        r = _mm256_mul_ps(r, scale);
        g = _mm256_mul_ps(g, scale);
        b = _mm256_mul_ps(b, scale);

        r = _mm256_max_ps(r, zero);
        g = _mm256_max_ps(g, zero);
        b = _mm256_max_ps(b, zero);

        r = _mm256_min_ps(r, ctx.lutmax);
        g = _mm256_min_ps(g, ctx.lutmax);
        b = _mm256_min_ps(b, ctx.lutmax);

        c = interp_tetrahedral_avx2(ctx, r, g, b, a);

        AVX2RGBAPack<outBD>::Store(out_buf, c.r, c.g, c.b, c.a);

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

} // anonymous namespace

void applyTetrahedralAVX2(const float *lut3d, int dim, const float *src, float *dst, int total_pixel_count)
{
    applyTetrahedralAVX2Func<BIT_DEPTH_F32, BIT_DEPTH_F32>(lut3d, dim, src, dst, total_pixel_count);
}

} // OCIO_NAMESPACE

#endif // OCIO_USE_AVX2