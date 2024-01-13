// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "Lut3DOpCPU_AVX512.h"
#if OCIO_USE_AVX512

#include <immintrin.h>
#include <string.h>

#include "AVX512.h"

namespace OCIO_NAMESPACE
{
namespace {

struct Lut3DContextAVX512 {
    const float *lut;
    __m512 lutmax;
    __m512 lutsize;
    __m512 lutsize2;
};

struct rgbavec_avx512 {
    __m512 r, g, b, a;
};

#define gather_rgb_avx512(src, idx)                            \
    sample_r = _mm512_i32gather_ps(idx, (void * )(src+0), 4);  \
    sample_g = _mm512_i32gather_ps(idx, (void * )(src+1), 4);  \
    sample_b = _mm512_i32gather_ps(idx, (void * )(src+2), 4)

static inline rgbavec_avx512 interp_tetrahedral_avx512(const Lut3DContextAVX512 &ctx, __m512& r, __m512& g, __m512& b, __m512& a)
{
    __m512 x0, x1, x2;
    __m512 cxxxa;
    __m512 cxxxb;
    __mmask16  mask;
    __m512 sample_r, sample_g, sample_b;

    rgbavec_avx512 result;

    __m512 lut_max  = ctx.lutmax;
    __m512 lutsize  = ctx.lutsize;
    __m512 lutsize2 = ctx.lutsize2;

    __m512 one_f   = _mm512_set1_ps(1.0f);
    __m512 four_f  = _mm512_set1_ps(4.0f);

    __m512 prev_r = _mm512_floor_ps(r);
    __m512 prev_g = _mm512_floor_ps(g);
    __m512 prev_b = _mm512_floor_ps(b);

    // rgb delta values
    __m512 d_r = _mm512_sub_ps(r, prev_r);
    __m512 d_g = _mm512_sub_ps(g, prev_g);
    __m512 d_b = _mm512_sub_ps(b, prev_b);

    __m512 next_r = _mm512_min_ps(lut_max, _mm512_add_ps(prev_r, one_f));
    __m512 next_g = _mm512_min_ps(lut_max, _mm512_add_ps(prev_g, one_f));
    __m512 next_b = _mm512_min_ps(lut_max, _mm512_add_ps(prev_b, one_f));

    // prescale indices
    prev_r = _mm512_mul_ps(prev_r, lutsize2);
    next_r = _mm512_mul_ps(next_r, lutsize2);

    prev_g = _mm512_mul_ps(prev_g, lutsize);
    next_g = _mm512_mul_ps(next_g, lutsize);

    prev_b = _mm512_mul_ps(prev_b, four_f);
    next_b = _mm512_mul_ps(next_b, four_f);

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

    __mmask16 gt_r = _mm512_cmp_ps_mask(d_r, d_g, _CMP_GT_OQ); // r>g
    __mmask16 gt_g = _mm512_cmp_ps_mask(d_g, d_b, _CMP_GT_OQ); // g>b
    __mmask16 gt_b = _mm512_cmp_ps_mask(d_b, d_r, _CMP_GT_OQ); // b>r

    // r> !b>r && r>g
    mask = _mm512_kandn(gt_b, gt_r);
    cxxxa = _mm512_mask_blend_ps(mask, prev_r, next_r);

    // r< !r>g && b>r
    mask = _mm512_kandn(gt_r, gt_b);
    cxxxb = _mm512_mask_blend_ps(mask, next_r, prev_r);

    // g> !r>g && g>b
    mask = _mm512_kandn(gt_r, gt_g);
    cxxxa = _mm512_add_ps(cxxxa, _mm512_mask_blend_ps(mask, prev_g, next_g));

    // g< !g>b && r>g
    mask = _mm512_kandn(gt_g, gt_r);
    cxxxb = _mm512_add_ps(cxxxb, _mm512_mask_blend_ps(mask, next_g, prev_g));

    // b> !g>b && b>r
    mask = _mm512_kandn(gt_g, gt_b);
    cxxxa = _mm512_add_ps(cxxxa, _mm512_mask_blend_ps(mask, prev_b, next_b));

    // b< !b>r && g>b
    mask = _mm512_kandn(gt_b, gt_g);
    cxxxb = _mm512_add_ps(cxxxb, _mm512_mask_blend_ps(mask, next_b, prev_b));

    __m512 c000 = _mm512_add_ps(_mm512_add_ps(prev_r, prev_g), prev_b);
    __m512 c111 = _mm512_add_ps(_mm512_add_ps(next_r, next_g), next_b);

    // sort delta r,g,b x0 >= x1 >= x2
    __m512 rg_min = _mm512_min_ps(d_r, d_g);
    __m512 rg_max = _mm512_max_ps(d_r, d_g);

    x2         = _mm512_min_ps(rg_min, d_b);
    __m512 mid = _mm512_max_ps(rg_min, d_b);

    x0 = _mm512_max_ps(rg_max, d_b);
    x1 = _mm512_min_ps(rg_max, mid);

    // convert indices to int
    __m512i c000_idx  = _mm512_cvttps_epi32(c000);
    __m512i cxxxa_idx = _mm512_cvttps_epi32(cxxxa);
    __m512i cxxxb_idx = _mm512_cvttps_epi32(cxxxb);
    __m512i c111_idx  = _mm512_cvttps_epi32(c111);

    gather_rgb_avx512(ctx.lut, c000_idx);

    // (1-x0) * c000
    __m512 v = _mm512_sub_ps(one_f, x0);
    result.r = _mm512_mul_ps(sample_r, v);
    result.g = _mm512_mul_ps(sample_g, v);
    result.b = _mm512_mul_ps(sample_b, v);

    gather_rgb_avx512(ctx.lut, cxxxa_idx);

    // (x0-x1) * cxxxa
    v = _mm512_sub_ps(x0, x1);
    result.r = _mm512_fmadd_ps(v, sample_r, result.r);
    result.g = _mm512_fmadd_ps(v, sample_g, result.g);
    result.b = _mm512_fmadd_ps(v, sample_b, result.b);

    gather_rgb_avx512(ctx.lut, cxxxb_idx);

    // (x1-x2) * cxxxb
    v = _mm512_sub_ps(x1, x2);
    result.r = _mm512_fmadd_ps(v, sample_r, result.r);
    result.g = _mm512_fmadd_ps(v, sample_g, result.g);
    result.b = _mm512_fmadd_ps(v, sample_b, result.b);

    gather_rgb_avx512(ctx.lut, c111_idx);

    // x2 * c111
    result.r = _mm512_fmadd_ps(x2, sample_r, result.r);
    result.g = _mm512_fmadd_ps(x2, sample_g, result.g);
    result.b = _mm512_fmadd_ps(x2, sample_b, result.b);

    result.a = a;

    return result;
}

template<BitDepth inBD, BitDepth outBD>
inline void applyTetrahedralAVX512Func(const float *lut3d, int dim, const void *inImg, void *outImg, int numPixels)
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType * src = (InType *)inImg;
    OutType * dst = (OutType *)outImg;
    __m512 r,g,b,a;
    rgbavec_avx512 c;

    Lut3DContextAVX512 ctx;

    float lutmax = (float)dim - 1;
    __m512 scale   = _mm512_set1_ps(lutmax);
    __m512 zero    = _mm512_setzero_ps();

    ctx.lut      = lut3d;
    ctx.lutmax   = _mm512_set1_ps(lutmax);
    ctx.lutsize  = _mm512_set1_ps((float)dim * 4);
    ctx.lutsize2 = _mm512_set1_ps((float)dim * dim * 4);

    int pixel_count = numPixels / 16 * 16;
    int remainder = numPixels - pixel_count;

    for (int i = 0; i < pixel_count; i += 16 )
    {
        AVX512RGBAPack<inBD>::Load(src, r, g, b, a);

        // scale and clamp values
        r = _mm512_mul_ps(r, scale);
        g = _mm512_mul_ps(g, scale);
        b = _mm512_mul_ps(b, scale);

        r = _mm512_max_ps(r, zero);
        g = _mm512_max_ps(g, zero);
        b = _mm512_max_ps(b, zero);

        r = _mm512_min_ps(r, ctx.lutmax);
        g = _mm512_min_ps(g, ctx.lutmax);
        b = _mm512_min_ps(b, ctx.lutmax);

        c = interp_tetrahedral_avx512(ctx, r, g, b, a);

        AVX512RGBAPack<outBD>::Store(dst, c.r, c.g, c.b, c.a);

        src += 64;
        dst += 64;
    }

     // handler leftovers pixels
    if (remainder)
    {
        AVX512RGBAPack<inBD>::LoadMasked(src, r, g, b, a, remainder);

        // scale and clamp values
        r = _mm512_mul_ps(r, scale);
        g = _mm512_mul_ps(g, scale);
        b = _mm512_mul_ps(b, scale);

        r = _mm512_max_ps(r, zero);
        g = _mm512_max_ps(g, zero);
        b = _mm512_max_ps(b, zero);

        r = _mm512_min_ps(r, ctx.lutmax);
        g = _mm512_min_ps(g, ctx.lutmax);
        b = _mm512_min_ps(b, ctx.lutmax);

        c = interp_tetrahedral_avx512(ctx, r, g, b, a);

        AVX512RGBAPack<outBD>::StoreMasked(dst, c.r, c.g, c.b, c.a, remainder);
    }
}

} // anonymous namespace

void applyTetrahedralAVX512(const float *lut3d, int dim, const float *src, float *dst, int total_pixel_count)
{
    applyTetrahedralAVX512Func<BIT_DEPTH_F32, BIT_DEPTH_F32>(lut3d, dim, src, dst, total_pixel_count);
}

} // OCIO_NAMESPACE

#endif // OCIO_USE_AVX512