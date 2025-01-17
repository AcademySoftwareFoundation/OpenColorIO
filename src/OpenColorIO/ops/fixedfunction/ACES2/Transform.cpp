// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "Transform.h"

#include <array>
#include <algorithm>
#include <cmath>

namespace OCIO_NAMESPACE
{

namespace ACES2
{

//
// Table lookups
//

inline float base_hue_for_position(int i_lo, int table_size) 
{
    const float result = i_lo * hue_limit / table_size;
    return result;
}

inline int hue_position_in_uniform_table(float wrapped_hue, int table_size)
{
    return int(wrapped_hue / hue_limit * float(table_size)); // TODO: can we use the 'lost' fraction for the lerps?
}

inline int next_position_in_table(int entry, int table_size)
{
    return (entry + 1) % table_size;
}

inline int clamp_to_table_bounds(int entry, int table_size) // TODO: this should be removed if we can constrain the hue range properly
{
    return std::min(table_size - 1, std::max(0, entry));
}

f2 cusp_from_table(float h, const Table3D &gt, const std::array<int, 2> & hue_linearity_search_range)
{
    int i = hue_position_in_uniform_table(h, gt.size) + gt.base_index;
    int i_lo = std::max(0, i + hue_linearity_search_range[0]);
    int i_hi = std::min(gt.base_index + gt.size, i + hue_linearity_search_range[1]);

    while (i_lo + 1 < i_hi)
    {
        if (h > gt.table[i][2])
        {
            i_lo = i;
        }
        else
        {
            i_hi = i;
        }
        i = (i_lo + i_hi) / 2;
    }

    i_hi = std::max(1, i_hi);

    const f3 lo {
        gt.table[i_hi-1][0],
        gt.table[i_hi-1][1],
        gt.table[i_hi-1][2]
    };

    const f3 hi {
        gt.table[i_hi][0],
        gt.table[i_hi][1],
        gt.table[i_hi][2]
    };

    const float t = (h - lo[2]) / (hi[2] - lo[2]);
    const float cuspJ = lerpf(lo[0], hi[0], t);
    const float cuspM = lerpf(lo[1], hi[1], t);

    return { cuspJ, cuspM };
}

float reach_m_from_table(float h, const ACES2::Table1D &gt)
{
    const int i_lo = clamp_to_table_bounds(hue_position_in_uniform_table(h, gt.size), gt.total_size);  // TODO: this should be removed if we can constrain the hue range properly
    const int i_hi = next_position_in_table(i_lo, gt.size);

    const float t = (h - i_lo) / (i_hi - i_lo);
    return lerpf(gt.table[i_lo], gt.table[i_hi], t);
}

float hue_dependent_upper_hull_gamma(float h, const ACES2::Table1D &gt)
{
    const int i_lo = clamp_to_table_bounds(hue_position_in_uniform_table(h, gt.size) + gt.base_index, gt.total_size);  // TODO: this should be removed if we can constrain the hue range properly
    const int i_hi = next_position_in_table(i_lo, gt.size);

    const float base_hue = base_hue_for_position(i_lo - gt.base_index, gt.size);

    const float t = h - base_hue;

    return lerpf(gt.table[i_lo], gt.table[i_hi], t);
}

//
// CAM
//

inline float _post_adaptation_cone_response_compression_fwd(float Rc, const float F_L_n)
{
    const float F_L_Y = powf(Rc * F_L_n, 0.42f);
    const float Ra    = (cam_nl_scale * F_L_Y) / (cam_nl_offset + F_L_Y);
    return Ra;
}

inline float _post_adaptation_cone_response_compression_inv(float Ra, const float F_L_n)
{
    const float F_L_Y = (cam_nl_offset * Ra) / (cam_nl_scale - Ra); // TODO: what happens when Ra >= cam_nl_scale (400.0f)
    const float Rc    = powf(F_L_Y, 1.f / 0.42f) / F_L_n;
    return Rc;
}


float post_adaptation_cone_response_compression_fwd(float v, float F_L)
{
    const float abs_v = std::abs(v);
    const float Ra =  _post_adaptation_cone_response_compression_fwd(abs_v, F_L);
    // Note that std::copysign(1.f, 0.f) returns 1 but the CTL copysign(1.,0.) returns 0.
    // TODO: Should we change the behaviour?
    return std::copysign(Ra, v);
}

float post_adaptation_cone_response_compression_inv(float v, float F_L)
{
    const float abs_v = std::abs(v);
    const float Rc    = _post_adaptation_cone_response_compression_inv(abs_v, F_L);
    return std::copysign(Rc, v);
}

inline float Achromatic_n_to_J(float A, const float cz)
{
    return J_scale * powf(A, cz);
}

inline float J_to_Achromatic_n(float J, const float cz)
{
    return powf(J / J_scale, 1.f / cz);
}

// Optimization for achromatic values

inline float _J_to_Y(float abs_J, const JMhParams &p)
{
    const float Ra = p.A_w_J * J_to_Achromatic_n(abs_J, p.cz);
    const float Y  = _post_adaptation_cone_response_compression_inv(Ra, p.F_L_n);
    return Y;
}
inline float _Y_to_J(float abs_Y, const JMhParams &p)
{
    const float Ra    = _post_adaptation_cone_response_compression_fwd(abs_Y, p.F_L_n);
    const float J     = Achromatic_n_to_J(Ra / p.A_w_J, p.cz);
    return J;
}

float Y_to_J(float Y, const JMhParams &p)
{
    const float abs_Y = std::abs(Y);
    const float J     = _Y_to_J(abs_Y, p);
    return std::copysign(J, Y);
}

inline f3 RGB_to_Aab(const f3 &RGB, const JMhParams &p)
{
    const f3 rgb_m = mult_f3_f33(RGB, p.MATRIX_RGB_to_CAM16_c);

    const f3 rgb_a = {
        post_adaptation_cone_response_compression_fwd(rgb_m[0], p.F_L_n),
        post_adaptation_cone_response_compression_fwd(rgb_m[1], p.F_L_n),
        post_adaptation_cone_response_compression_fwd(rgb_m[2], p.F_L_n)
    };

    const f3 Aab = mult_f3_f33(rgb_a, p.MATRIX_cone_response_to_Aab);
    return Aab;
}

inline f3 Aab_to_JMh(const f3 &Aab, const JMhParams &p)
{
    const float J = Achromatic_n_to_J(Aab[0], p.cz);

    const float M = J == 0.f ? 0.f : sqrt(Aab[1] * Aab[1] + Aab[2] * Aab[2]);

    const float h_rad = std::atan2(Aab[2], Aab[1]);
    float h = from_radians(h_rad);

    return {J, M, h};
}

f3 RGB_to_JMh(const f3 &RGB, const JMhParams &p)
{
    const f3 Aab = RGB_to_Aab(RGB, p);
    const f3 JMh = Aab_to_JMh(Aab, p);
    return JMh;
}

inline f3 JMh_to_Aab(const f3 &JMh, const JMhParams &p)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];

    const float h_rad = to_radians(h);

    const float A = J_to_Achromatic_n(J, p.cz);
    const float a = M * cos(h_rad);
    const float b = M * sin(h_rad);
    return {A, a, b};
}

inline f3 Aab_to_RGB(const f3 &Aab, const JMhParams &p)
{
    const f3 rgb_a = mult_f3_f33(Aab, p.MATRIX_Aab_to_cone_response);

    const f3 rgb_m = {
        post_adaptation_cone_response_compression_inv(rgb_a[0], p.F_L_n),
        post_adaptation_cone_response_compression_inv(rgb_a[1], p.F_L_n),
        post_adaptation_cone_response_compression_inv(rgb_a[2], p.F_L_n)
    };

    const f3 rgb = mult_f3_f33(rgb_m, p.MATRIX_CAM16_c_to_RGB);
    return rgb;
}

f3 JMh_to_RGB(const f3 &JMh, const JMhParams &p)
{
    const f3 Aab = JMh_to_Aab(JMh, p);
    const f3 rgb = Aab_to_RGB(Aab, p);
    return rgb;
}

//
// Tonescale / Chroma compress
//

inline float chroma_compress_norm(float h, float chroma_compress_scale)
{
    const float h_rad = to_radians(h);
    const float a = cos(h_rad);
    const float b = sin(h_rad);
    const float cos_hr2 = a * a - b * b;
    const float sin_hr2 = 2.0f * a * b;
    const float cos_hr3 = 4.0f * a * a * a - 3.0f * a;
    const float sin_hr3 = 3.0f * b - 4.0f * b * b * b;

    const float M = 11.34072f * a +
              16.46899f * cos_hr2 +
               7.88380f * cos_hr3 +
              14.66441f * b +
              -6.37224f * sin_hr2 +
               9.19364f * sin_hr3 +
              77.12896f;

    return M * chroma_compress_scale; // TODO: is it worth prescaling the above M constants ?
}

inline float toe_fwd( float x, float limit, float k1_in, float k2_in)
{
    if (x > limit)
    {
        return x;
    }

    const float k2 = std::max(k2_in, 0.001f);
    const float k1 = sqrt(k1_in * k1_in + k2 * k2);
    const float k3 = (limit + k1) / (limit + k2);

    const float minus_b = k3 * x - k1;
    const float minus_ac = k2 * k3 * x; // a is 1.0
    return 0.5f * (minus_b + sqrt(minus_b * minus_b + 4.f * minus_ac)); // a is 1.0, mins_b squared == b^2
}

inline float toe_inv( float x, float limit, float k1_in, float k2_in)
{
    if (x > limit)
    {
        return x;
    }

    const float k2 = std::max(k2_in, 0.001f);
    const float k1 = sqrt(k1_in * k1_in + k2 * k2);
    const float k3 = (limit + k1) / (limit + k2);
    return (x * x + k1 * x) / (k3 * (x + k2));
}

template <bool inverse>
inline float aces_tonescale(const float Y_in, const JMhParams &p, const ToneScaleParams &pt)
{
    if (inverse)
    {
        const float Y_ts_norm = Y_in / reference_luminance; // TODO
        const float Z = std::max(0.f, std::min(pt.inverse_limit, Y_ts_norm)); // TODO: could eliminate max in the context of the full tonescale
        const float f = (Z + sqrt(Z * (4.f * pt.t_1 + Z))) / 2.f;
        const float Y = pt.s_2 / (powf((pt.m_2 / f), (1.f / pt.g)) - 1.f); // TODO: investigate precomputing reciprocal m_2  and a negative power? may swap a division for a multiply? powf(pt.m_2_recip * f, (-1.f / pt.g))
        return Y;
    }

    const float f    = pt.m_2 * powf(Y_in / (Y_in + pt.s_2), pt.g);
    const float Y_ts = std::max(0.f, f * f / (f + pt.t_1)) * pt.n_r;  // max prevents -ve values being output also handles division by zero possibility // TODO: can we eliminate the n_r scaling? should it be reference_luminance
    return Y_ts;
}

template <bool inverse>
float tonescale(const float J, const JMhParams &p, const ToneScaleParams &pt) // TODO: consider computing tonescale from and to A rather than J to avoid extra pow() calls
{
    // Tonescale applied in Y (convert to and from J)
    const float J_abs = std::abs(J);
    const float Y_in  = _J_to_Y(J_abs, p);
    const float Y_out = aces_tonescale<inverse>(Y_in, p, pt);
    const float J_out = _Y_to_J(Y_out, p);
    return std::copysign(J_out, J);
}

f3 tonescale_chroma_compress_fwd(const f3 &JMh, const JMhParams &p, const ToneScaleParams &pt, const ResolvedSharedCompressionParameters &pr, const ChromaCompressParams &pc)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];

    constexpr bool inverse = false;
    const float J_ts = tonescale<inverse>(J, p, pt);

    // ChromaCompress
    float M_cp = M;

    if (M != 0.0)
    {
        const float nJ = J_ts / pr.limit_J_max;
        const float snJ = std::max(0.f, 1.f - nJ);
        const float Mnorm = chroma_compress_norm(h, pc.chroma_compress_scale);
        const float limit = powf(nJ, pr.model_gamma_inv) * pr.reachMaxM / Mnorm;

        M_cp = M * powf(J_ts / J, pr.model_gamma_inv);
        M_cp = M_cp / Mnorm;
        M_cp = limit - toe_fwd(limit - M_cp, limit - 0.001f, snJ * pc.sat, sqrt(nJ * nJ + pc.sat_thr));
        M_cp = toe_fwd(M_cp, limit, nJ * pc.compr, snJ);
        M_cp = M_cp * Mnorm;
    }

    return {J_ts, M_cp, h};
}

f3 tonescale_chroma_compress_inv(const f3 &JMh, const JMhParams &p, const ToneScaleParams &pt, const ResolvedSharedCompressionParameters &pr, const ChromaCompressParams &pc)
{
    const float J_ts = JMh[0];
    const float M_cp = JMh[1];
    const float h    = JMh[2];

    constexpr bool inverse = true;
    const float J = tonescale<inverse>(J_ts, p, pt);

    // Inverse ChromaCompress
    float M = M_cp;

    if (M_cp != 0.0)
    {
        const float nJ = J_ts / pr.limit_J_max;
        const float snJ = std::max(0.f, 1.f - nJ);
        const float Mnorm = chroma_compress_norm(h, pc.chroma_compress_scale);
        const float limit = powf(nJ, pr.model_gamma_inv) * pr.reachMaxM / Mnorm;

        M = M_cp / Mnorm;
        M = toe_inv(M, limit, nJ * pc.compr, snJ);
        M = limit - toe_inv(limit - M, limit - 0.001f, snJ * pc.sat, sqrt(nJ * nJ + pc.sat_thr));
        M = M * Mnorm;
        M = M * powf(J_ts / J, -pr.model_gamma_inv);
    }

    return {J, M, h};
}

inline float model_gamma(void)
{
    // c * z nonlinearity
    return surround[1] * (1.48f + sqrt(Y_b / reference_luminance));
}

JMhParams init_JMhParams(const Primaries &prims)
{
    const m33f cone_response_to_Aab = {
        2.0f, 1.0f, 1.0f / 20.0f,
        1.0f, -12.0f / 11.0f, 1.0f / 11.0f,
        1.0f / 9.0f, 1.0f / 9.0f, -2.0f / 9.0f
    };

    const m33f MATRIX_16 = XYZtoRGB_f33(CAM16::primaries);
    const m33f RGB_to_XYZ = RGBtoXYZ_f33(prims);
    const f3 XYZ_w = mult_f3_f33(f3_from_f(reference_luminance), RGB_to_XYZ);

    const float Y_W = XYZ_w[1];

    const f3 RGB_w = mult_f3_f33(XYZ_w, MATRIX_16);

    // Viewing condition dependent parameters
    constexpr float K = 1.f / (5.f * L_A + 1.f);
    constexpr float K4 = K * K * K * K;
    const float F_L = 0.2f * K4 * (5.f * L_A) + 0.1f * powf((1.f - K4), 2.f) * powf(5.f * L_A, 1.f/3.f);

    const float F_L_n = F_L / reference_luminance;
    const float cz    = model_gamma();

    const f3 D_RGB = {
        Y_W / RGB_w[0],
        Y_W / RGB_w[1],
        Y_W / RGB_w[2]
    };

    const f3 RGB_WC {
        D_RGB[0] * RGB_w[0],
        D_RGB[1] * RGB_w[1],
        D_RGB[2] * RGB_w[2]
    };

    const f3 RGB_AW = {
        post_adaptation_cone_response_compression_fwd(RGB_WC[0], F_L_n),
        post_adaptation_cone_response_compression_fwd(RGB_WC[1], F_L_n),
        post_adaptation_cone_response_compression_fwd(RGB_WC[2], F_L_n)
    };

    const float A_w   = cone_response_to_Aab[0] * RGB_AW[0] + cone_response_to_Aab[1] * RGB_AW[1] + cone_response_to_Aab[2] * RGB_AW[2];
    const float A_w_J = _post_adaptation_cone_response_compression_fwd(reference_luminance, F_L_n);

    // TODO: evaluate the condition number of the below matrices and consider extracting a power of 2 scale out of them may improve noise behaviour
    //       power of 2 to make cost of extra scaling limited vs generalised multiply/divide ?

    // Note we are prescaling the CAM16 LMS responses to directly provide for chromatic adaptation.
    const m33f MATRIX_RGB_to_CAM16 = mult_f33_f33(RGBtoRGB_f33(prims, CAM16::primaries), scale_f33(Identity_M33, f3_from_f(reference_luminance)));
    const m33f MATRIX_RGB_to_CAM16_c = mult_f33_f33(scale_f33(Identity_M33, D_RGB), MATRIX_RGB_to_CAM16);
    const m33f MATRIX_CAM16_c_to_RGB = invert_f33(MATRIX_RGB_to_CAM16_c);

    const m33f MATRIX_cone_response_to_Aab = {
        cone_response_to_Aab[0] / A_w,                cone_response_to_Aab[1] / A_w,                cone_response_to_Aab[2] / A_w,
        cone_response_to_Aab[3] * 43.f * surround[2], cone_response_to_Aab[4] * 43.f * surround[2], cone_response_to_Aab[5] * 43.f * surround[2],
        cone_response_to_Aab[6] * 43.f * surround[2], cone_response_to_Aab[7] * 43.f * surround[2], cone_response_to_Aab[8] * 43.f * surround[2],
    };
    const m33f MATRIX_Aab_to_cone_response = invert_f33(MATRIX_cone_response_to_Aab);

    JMhParams p = {
        F_L_n,
        cz,
        A_w,
        A_w_J,
        MATRIX_RGB_to_CAM16_c,
        MATRIX_CAM16_c_to_RGB,
        MATRIX_cone_response_to_Aab,
        MATRIX_Aab_to_cone_response
    };
   return p;
}

Table3D make_gamut_table(const JMhParams &params, float peakLuminance)
{
    Table3D gamutCuspTableUnsorted{};
    int minhIndex = 0;
    for (int i = 0; i < gamutCuspTableUnsorted.size; i++)
    {
        const float hNorm = float(i) / float(gamutCuspTableUnsorted.size);
        const f3 HSV = {hNorm, 1., 1.};
        const f3 RGB = HSV_to_RGB(HSV);
        const f3 scaledRGB = mult_f_f3(peakLuminance / reference_luminance, RGB);
        const f3 JMh = RGB_to_JMh(scaledRGB, params);

        gamutCuspTableUnsorted.table[i][0] = JMh[0];
        gamutCuspTableUnsorted.table[i][1] = JMh[1]  * (1.f + smooth_m * smooth_cusps);
        gamutCuspTableUnsorted.table[i][2] = JMh[2];
        if ( gamutCuspTableUnsorted.table[i][2] < gamutCuspTableUnsorted.table[minhIndex][2])
            minhIndex = i;
    }

    Table3D gamutCuspTable{};
    for (int i = 0; i < gamutCuspTableUnsorted.size; i++)
    {
        gamutCuspTable.table[i + gamutCuspTable.base_index][0] = gamutCuspTableUnsorted.table[(minhIndex+i) % gamutCuspTableUnsorted.size][0];
        gamutCuspTable.table[i + gamutCuspTable.base_index][1] = gamutCuspTableUnsorted.table[(minhIndex+i) % gamutCuspTableUnsorted.size][1];
        gamutCuspTable.table[i + gamutCuspTable.base_index][2] = gamutCuspTableUnsorted.table[(minhIndex+i) % gamutCuspTableUnsorted.size][2];
    }

    // Copy last populated entry to first empty spot
    gamutCuspTable.table[0][0] = gamutCuspTable.table[gamutCuspTable.base_index + gamutCuspTable.size-1][0];
    gamutCuspTable.table[0][1] = gamutCuspTable.table[gamutCuspTable.base_index + gamutCuspTable.size-1][1];
    gamutCuspTable.table[0][2] = gamutCuspTable.table[gamutCuspTable.base_index + gamutCuspTable.size-1][2];

    // Copy first populated entry to last empty spot
    gamutCuspTable.table[gamutCuspTable.base_index + gamutCuspTable.size][0] = gamutCuspTable.table[gamutCuspTable.base_index][0];
    gamutCuspTable.table[gamutCuspTable.base_index + gamutCuspTable.size][1] = gamutCuspTable.table[gamutCuspTable.base_index][1];
    gamutCuspTable.table[gamutCuspTable.base_index + gamutCuspTable.size][2] = gamutCuspTable.table[gamutCuspTable.base_index][2];

    // Wrap the hues, to maintain monotonicity. These entries will fall outside [0.0, hue_limit)
    gamutCuspTable.table[0][2] = gamutCuspTable.table[0][2] - hue_limit;
    gamutCuspTable.table[gamutCuspTable.size+1][2] = gamutCuspTable.table[gamutCuspTable.size+1][2] + hue_limit;
    return gamutCuspTable;
}

bool any_below_zero(const f3 &rgb)
{
    return (rgb[0] < 0. || rgb[1] < 0. || rgb[2] < 0.);
}

Table1D make_reach_m_table(const JMhParams &params, const float limit_J_max)
{
    Table1D gamutReachTable{};

    for (int i = 0; i < gamutReachTable.size; i++) {
        const float hue = base_hue_for_position(i, gamutReachTable.size);

        const float search_range = 50.f;
        float low = 0.f;
        float high = low + search_range;
        bool outside = false;

        while ((outside != true) & (high < 1300.f))
        {
            const f3 searchJMh = {limit_J_max, high, hue};
            const f3 newLimitRGB = JMh_to_RGB(searchJMh, params);
            outside = any_below_zero(newLimitRGB);
            if (outside == false)
            {
                low = high;
                high = high + search_range;
            }
        }

        while (high-low > 1e-2)
        {
            const float sampleM = (high + low) / 2.f;
            const f3 searchJMh = {limit_J_max, sampleM, hue};
            const f3 newLimitRGB = JMh_to_RGB(searchJMh, params);
            outside = any_below_zero(newLimitRGB);
            if (outside)
            {
                high = sampleM;
            }
            else
            {
                low = sampleM;
            }
        }

        gamutReachTable.table[i] = high;
    }

    return gamutReachTable;
}

bool outside_hull(const f3 &rgb)
{
    // limit value, once we cross this value, we are outside of the top gamut shell
    const float maxRGBtestVal = 1.0;
    return rgb[0] > maxRGBtestVal || rgb[1] > maxRGBtestVal || rgb[2] > maxRGBtestVal;
}

inline float get_focus_gain(float J, float cuspJ, float limit_J_max, float focus_dist)
{
    const float thr = lerpf(cuspJ, limit_J_max, focus_gain_blend);

    /* Note from Pekka

    If we’d be willing to make a tiny change in the pixel values, we can eliminate one pow() from getFocusGain() function by changing the focusAdjustGain value to 0.5 from the current 0.55. It then becomes:

      float gain = log10((limitJmax - thr) / max(0.0001f, (limitJmax - min(limitJmax, J))));
      return gain * gain + 1.0f;
    from the current:

      float gain = (limitJmax - thr) / max(0.0001f, (limitJmax - min(limitJmax, J)));
      return pow(log10(gain), 1.0f / focusAdjustGain) + 1.0f;
    
    */
    float gain = 1.0f;
    if (J > thr)
    {
        // Approximate inverse required above threshold
        gain = (limit_J_max - thr) / std::max(0.0001f, (limit_J_max - std::min(limit_J_max, J)));
        gain = powf(log10(gain), focus_adjust_gain_inv) + 1.f;
    }
    return limit_J_max * focus_dist * gain;
}

float solve_J_intersect(float J, float M, float focusJ, float maxJ, float slope_gain)
{
    const float a = M / (focusJ * slope_gain);
    float b = 0.f;
    float c = 0.f;
    float intersectJ = 0.f;

    if (J < focusJ)
    {
        b = 1.f - M / slope_gain;
    }
    else
    {
        b = - (1.f + M / slope_gain + maxJ * M / (focusJ * slope_gain));
    }

    if (J < focusJ)
    {
        c = -J;
    }
    else
    {
        c = maxJ * M / slope_gain + J;
    }

    const float root = sqrt(b * b - 4.f * a * c);

    if (J < focusJ)
    {
        intersectJ = 2.f * c / (-b - root);
    }
    else
    {
        intersectJ = 2.f * c / (-b + root);
    }

    return intersectJ;
}

inline float smin(float a, float b, float s)
{
    const float h = std::max(s - std::abs(a - b), 0.f) / s;
    return std::min(a, b) - h * h * h * s * (1.f / 6.f);
}

// Redefined smin function to handle scaling by adjusting k
inline float smin_scaled(float a, float b, float s, float scale) {
    float s_scaled = s * scale;
    float h = std::max(s_scaled - std::abs(a - b), 0.0f) / s_scaled;
    return std::min(a, b) - h * h * h * s_scaled * (1.f / 6.f);
}

inline float compute_compression_vector_slope(const float intersectJ, const float focusJ, const float limitJmax, const float slope_gain)
{
    const float direction_scaler = (intersectJ < focusJ) ? intersectJ : (limitJmax - intersectJ); // TODO < vs <=
    return direction_scaler * (intersectJ - focusJ) / (focusJ * slope_gain);
}

inline float estimate_line_and_boundary_intersection_M(const float J_axis_intersect, const float slope, const float inv_gamma,
                                                       const float J_max, const float M_max, const float J_intersection_reference)
{
    // Line defined by     J = slope * x + J_axis_intersect
    // Boundary defined by J = J_max * (x / M_max) ^ (1/inv_gamma)
    // Approximate as we do not want to iteratively solve intersection of a straight line and an exponential

    // We calculate a shifted intersection from the original intersection using the inverse of the exponential
    // and the provided reference
    const float normalised_J         = J_axis_intersect / J_intersection_reference;
    const float shifted_intersection = J_intersection_reference * powf(normalised_J, inv_gamma);

    // Now we find the M intersection of two lines
    // line from origin to J,M Max       l1(x) = J/M * x
    // line from J Intersect' with slope l2(x) = slope * x + Intersect'

    return shifted_intersection / ((J_max / M_max) - slope);
    //return shifted_intersection * M_max / (J_max - slope * M_max);
}

float find_gamut_boundary_intersection(const f2 &JM_cusp, float J_max, float gamma_top_inv, float gamma_bottom_inv, const float J_intersect_source, const float slope, const float J_intersect_cusp)
{
    const float M_boundary_lower = estimate_line_and_boundary_intersection_M(J_intersect_source, slope, gamma_bottom_inv, JM_cusp[0], JM_cusp[1], J_intersect_cusp);

    // The upper hull is flipped and thus 'zeroed' at J_max
    // Also note we negate the slope
    const float f_J_intersect_cusp   = J_max - J_intersect_cusp;
    const float f_J_intersect_source = J_max - J_intersect_source;
    const float f_JM_cusp_J          = J_max - JM_cusp[0];
    const float M_boundary_upper =
      estimate_line_and_boundary_intersection_M(f_J_intersect_source, -slope, gamma_top_inv, f_JM_cusp_J, JM_cusp[1], f_J_intersect_cusp);

    // Smooth minimum between the two calculated values for the M component
    // TODO: do we need to normalise based on JM_cusp[1]
    //const float M_boundary = JM_cusp[1] * smin(M_boundary_lower / JM_cusp[1], M_boundary_upper / JM_cusp[1], smooth_cusps);
    const float M_boundary = smin_scaled(M_boundary_lower, M_boundary_upper, smooth_cusps, JM_cusp[1]);
    return M_boundary;
}

template <bool invert>
inline float reinhard_remap(const float scale, const float nd)
{
    if (invert)
    {
        if (nd >= 1.0f) // TODO: given remap_M already tests against proportion do we need this asymptote test
            return scale;
        else
            return scale * -(nd / (nd - 1.0f));
    }
    return scale * nd / (1.0f + nd);
}

template <bool invert>
inline float remap_M(const float M, const float gamut_boundary_M, const float reach_boundary_M)
{
    const float boundary_ratio = gamut_boundary_M / reach_boundary_M;
    const float proportion = std::max(boundary_ratio, compression_threshold);
    const float threshold  = proportion * gamut_boundary_M;

    if (proportion >= 1.0f || M <= threshold)
        return M;

    // Translate to place threshold at zero
    const float m_offset     = M - threshold;
    const float gamut_offset = gamut_boundary_M - threshold;
    const float reach_offset = reach_boundary_M - threshold;

    const float scale = reach_offset / ((reach_offset / gamut_offset) - 1.0f);
    const float nd = m_offset / scale;
 
    // shift back to absolute
    return threshold + reinhard_remap<invert>(scale, nd);
}

template <bool invert>
f3 compressGamut(const f3 &JMh, float Jx, const ACES2::ResolvedSharedCompressionParameters& sr, const ACES2::GamutCompressParams& p, const HueDependantGamutParams hdp)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];
    
    // TODO: test migrating these to calling function so they are only tested once for the inverse?
    if (J <= 0.0f) // Limit to +ve J values // TODO test this is needed
    {
        return {0.0f, 0.f, h};
    }
    if (M <= 0.0f || J > sr.limit_J_max) // We compress M only so avoid mapping zero
                                         // Above the expected maximum we explicitly map to 0 M
    {
        return {J, 0.f, h};
    }

    const float slope_gain = get_focus_gain(Jx, hdp.JMcusp[0], sr.limit_J_max, p.focus_dist);
    const float J_intersect_source = solve_J_intersect(J, M, hdp.focusJ, sr.limit_J_max, slope_gain);
    const float gamut_slope = compute_compression_vector_slope(J_intersect_source, hdp.focusJ, sr.limit_J_max, slope_gain);
 

    const float J_intersect_cusp = solve_J_intersect(hdp.JMcusp[0], hdp.JMcusp[1], hdp.focusJ, sr.limit_J_max, slope_gain);
    const float gamut_boundary_M = find_gamut_boundary_intersection(hdp.JMcusp, sr.limit_J_max, hdp.gamma_top_inv, hdp.gamma_bottom_inv, J_intersect_source, gamut_slope, J_intersect_cusp);

    if (gamut_boundary_M <= 0.0f) // TODO: when/why does this happen?
    {
        return {J, 0.f, h};
    }

    //const float reach_slope_gain = get_focus_gain(gamut_boundary[0], hdp.JMcusp[0], sr.limit_J_max, p.focus_dist);
    //const float reach_intersectJ = solve_J_intersect(gamut_boundary[0], gamut_boundary[1], hdp.focusJ, sr.limit_J_max, reach_slope_gain);
    //const float reach_slope      = compute_compression_vector_slope(reach_intersectJ, hdp.focusJ, sr.limit_J_max, reach_slope_gain);
    //const float reachBoundaryM   = estimate_line_and_boundary_intersection_M(reach_intersectJ, reach_slope, sr.model_gamma_inv, sr.limit_J_max, sr.reachMaxM, sr.limit_J_max);
    const float reachBoundaryM   = estimate_line_and_boundary_intersection_M(J_intersect_source, gamut_slope, sr.model_gamma_inv, sr.limit_J_max, sr.reachMaxM, sr.limit_J_max);

    const float remapped_M = remap_M<invert>(M, gamut_boundary_M, reachBoundaryM);

    const f3 JMcompressed {
        J_intersect_source + remapped_M * gamut_slope,
        remapped_M,
        h
    };
    return JMcompressed;
}

inline float compute_focusJ(float cusp_J, float mid_J, float limit_J_max)
{
    return lerpf(cusp_J, mid_J, std::min(1.f, cusp_mid_blend - (cusp_J / limit_J_max)));
}

HueDependantGamutParams init_HueDependantGamutParams(const f3 &JMh, const ResolvedSharedCompressionParameters &sr, const GamutCompressParams &p)
{
    HueDependantGamutParams hdp;
    hdp.gamma_top_inv = hue_dependent_upper_hull_gamma(JMh[2], p.upper_hull_gamma_inv_table);
    hdp.gamma_bottom_inv = p.lower_hull_gamma_inv;
    hdp.JMcusp = cusp_from_table(JMh[2], p.gamut_cusp_table, p.hue_linearity_search_range);
    hdp.focusJ = compute_focusJ(hdp.JMcusp[0], p.mid_J, sr.limit_J_max);
    hdp.analytical_threshold = lerpf(hdp.JMcusp[0], sr.limit_J_max, focus_gain_blend);
    return hdp;
}

f3 gamut_compress_fwd(const f3 &JMh, const ResolvedSharedCompressionParameters &sr, const GamutCompressParams &p)
{
    const HueDependantGamutParams hdp = init_HueDependantGamutParams(JMh, sr, p);
    
    return compressGamut<false>(JMh, JMh[0], sr, p, hdp);
}

f3 gamut_compress_inv(const f3 &JMh, const ResolvedSharedCompressionParameters &sr, const GamutCompressParams &p)
{
    const HueDependantGamutParams hdp = init_HueDependantGamutParams(JMh, sr, p);

    float Jx = JMh[0];
    if (Jx > hdp.analytical_threshold)
    {
        // Approximation above threshold
        Jx = compressGamut<true>(JMh, Jx, sr, p, hdp)[0];
    }
    return compressGamut<true>(JMh, Jx, sr, p, hdp);
}

static constexpr int gamma_test_count = 5;

bool evaluate_gamma_fit(
    const f2 &JMcusp,
    const std::array<f3, gamma_test_count> JMh_values,
    float topGamma_inv,
    float peakLuminance,
    float limit_J_max,
    float mid_J,
    float focus_dist,
    float lower_hull_gamma_inv,
    const JMhParams &limitJMhParams)
{

    const float focusJ = compute_focusJ(JMcusp[0], mid_J, limit_J_max);

    for (auto testJMh: JMh_values)
    {
        const float slope_gain = get_focus_gain(testJMh[0], JMcusp[0], limit_J_max, focus_dist);
        const float J_intersect_source = solve_J_intersect(testJMh[0], testJMh[1], focusJ, limit_J_max, slope_gain);
        const float slope = compute_compression_vector_slope(J_intersect_source, focusJ, limit_J_max, slope_gain);
        const float J_intersect_cusp = solve_J_intersect(JMcusp[0], JMcusp[1], focusJ, limit_J_max, slope_gain);

        const float approxLimit_M = find_gamut_boundary_intersection(JMcusp, limit_J_max, topGamma_inv, lower_hull_gamma_inv, J_intersect_source, slope, J_intersect_cusp);
        const float approxLimit_J = J_intersect_source + slope * approxLimit_M;

        const f3 approximate_JMh = {approxLimit_J, approxLimit_M, testJMh[2]};
        const f3 newLimitRGB = JMh_to_RGB(approximate_JMh, limitJMhParams);
        const f3 newLimitRGBScaled = mult_f_f3(reference_luminance / peakLuminance, newLimitRGB); // TODO: can this be avoided by amending outside_hull to account for peak?

        if (!outside_hull(newLimitRGBScaled))
        {
            return false;
        }
    }

    return true;
}

Table1D make_upper_hull_gamma(
    const Table3D &gamutCuspTable,
    const std::array<int, 2> & hue_linearity_search_range,
    float peakLuminance,
    float limit_J_max,
    float mid_J,
    float focus_dist,
    float lower_hull_gamma_inv,
    const JMhParams &limitJMhParams)
{
    const std::array<float, gamma_test_count> testPositions = {0.01f, 0.1f, 0.5f, 0.8f, 0.99f};

    Table1D gamutTopGamma{};

    for (int i = 0; i < gamutTopGamma.size; i++)
    {
        gamutTopGamma.table[i + gamutTopGamma.base_index] = -1.f;

        const float hue = base_hue_for_position(i, gamutTopGamma.size);
        const f2 JMcusp = cusp_from_table(hue, gamutCuspTable, hue_linearity_search_range);

        std::array<f3, gamma_test_count> testJMh;
        std::generate(testJMh.begin(), testJMh.end(), [JMcusp, limit_J_max, testPositions, hue, testIndex = 0]() mutable {
            const float testJ = lerpf(JMcusp[0], limit_J_max, testPositions[testIndex]);
            f3 result = { testJ, JMcusp[1], hue };
            testIndex++;
            return result;
        });

        // TODO: calculate best fitting inverse gamma directly rather than reciprocating it in the loop
        // TODO: adjacent found gamma values typically fall close to each other could initialise the search range
        //       with values near to speed up searching. Note that discrepancies do occur at/near corners of gamut
        const float search_range = gammaSearchStep;
        float low = gammaMinimum;
        float high = low + search_range;
        bool outside = false;

        auto gamma_fit_predicate = [JMcusp, &testJMh, peakLuminance, limit_J_max, mid_J, focus_dist, lower_hull_gamma_inv, limitJMhParams](float gamma)
        {
            return evaluate_gamma_fit(JMcusp, testJMh, 1.0f / gamma, peakLuminance, limit_J_max, mid_J, focus_dist, lower_hull_gamma_inv, limitJMhParams);
        };
        while (!(outside) && (high < gammaMaximum))
        {
            const bool gammaFound = gamma_fit_predicate(high);
            if (!gammaFound)
            {
                low = high;
                high = high + search_range;
            }
            else
            {
                outside = true;
            }
        }

        float testGamma = -1.f;
        while ( (high-low) > gammaAccuracy)
        {
            testGamma = (high + low) / 2.f;
            const bool gammaFound = gamma_fit_predicate(testGamma);
            if (gammaFound)
            {
                high = testGamma;
                gamutTopGamma.table[i + gamutTopGamma.base_index] = 1.0f / high;
            }
            else
            {
                low = testGamma;
            }
        }
    }

    // Copy last populated entries to empty spot 'wrapping' entries
    gamutTopGamma.table[0] = gamutTopGamma.table[gamutTopGamma.base_index + gamutTopGamma.size - 1];
    gamutTopGamma.table[gamutTopGamma.base_index + gamutTopGamma.size] = gamutTopGamma.table[gamutCuspTable.base_index];

    return gamutTopGamma;
}

// Tonescale pre-calculations
ToneScaleParams init_ToneScaleParams(float peakLuminance)
{
    // Preset constants that set the desired behavior for the curve
    const float n = peakLuminance;

    const float n_r = 100.0f;    // normalized white in nits (what 1.0 should be)
    const float g = 1.15f;       // surround / contrast
    const float c = 0.18f;       // anchor for 18% grey
    const float c_d = 10.013f;   // output luminance of 18% grey (in nits)
    const float w_g = 0.14f;     // change in grey between different peak luminance
    const float t_1 = 0.04f;     // shadow toe or flare/glare compensation
    const float r_hit_min = 128.f;   // scene-referred value "hitting the roof"
    const float r_hit_max = 896.f;   // scene-referred value "hitting the roof"

    // Calculate output constants
    // TODO: factor these calculations into well named functions
    // TODO: ensure correct use of n_r vs n vs reference_luminance
    const float r_hit = r_hit_min + (r_hit_max - r_hit_min) * (log(n/n_r)/log(10000.f/100.f));
    const float m_0 = (n / n_r);
    const float m_1 = 0.5f * (m_0 + sqrt(m_0 * (m_0 + 4.f * t_1)));
    const float u = powf((r_hit/m_1)/((r_hit/m_1)+1.f),g);
    const float m = m_1 / u;
    const float w_i = log(n/100.f)/log(2.f);
    const float c_t = c_d/n_r * (1.f + w_i * w_g);
    const float g_ip = 0.5f * (c_t + sqrt(c_t * (c_t + 4.f * t_1)));
    const float g_ipp2 = -(m_1 * powf((g_ip/m),(1.f/g))) / (powf(g_ip/m , 1.f/g)-1.f);
    const float w_2 = c / g_ipp2;
    const float s_2 = w_2 * m_1 * reference_luminance;
    const float u_2 = powf((r_hit/m_1)/((r_hit/m_1) + w_2), g);
    const float m_2 = m_1 / u_2;
    const float inverse_limit = n / (u_2 * n_r);
    const float log_peak = log10( n / n_r);

    ToneScaleParams TonescaleParams = {
        n,
        n_r,
        g,
        t_1,
        c_t,
        s_2,
        u_2,
        m_2,
        inverse_limit,
        log_peak
    };

    return TonescaleParams;
}

SharedCompressionParameters init_SharedCompressionParams(float peakLuminance, const JMhParams &inputJMhParams)
{
    const float limit_J_max = Y_to_J(peakLuminance, inputJMhParams);
    const float model_gamma_inv = 1.f / model_gamma();
    const JMhParams compressionGamut = init_JMhParams(ACES_AP1::primaries);

    SharedCompressionParameters params = {
        limit_J_max,
        model_gamma_inv,
        make_reach_m_table(compressionGamut, limit_J_max)
    };
    return params;
}

ResolvedSharedCompressionParameters resolve_CompressionParams(float hue, const SharedCompressionParameters &p)
{
    ResolvedSharedCompressionParameters params = {
        p.limit_J_max,
        p.model_gamma_inv,
        reach_m_from_table(hue, p.reach_m_table)
    };
    return params;
}

ChromaCompressParams init_ChromaCompressParams(float peakLuminance, const ToneScaleParams &tsParams)
{
    // Calculated chroma compress variables
    // TODO: name these magic constants
    const float compr = chroma_compress + (chroma_compress * chroma_compress_fact) * tsParams.log_peak;
    const float sat = std::max(0.2f, chroma_expand - (chroma_expand * chroma_expand_fact) * tsParams.log_peak);
    const float sat_thr = chroma_expand_thr / tsParams.n;
    const float chroma_compress_scale =  powf(0.03379f * peakLuminance, 0.30596f) - 0.45135f;

    ChromaCompressParams params = {
        sat,
        sat_thr,
        compr,
        chroma_compress_scale
    };
    return params;
}

std::array<int, 2> determine_hue_linearity_search_range(const Table3D &gamutCuspTable)
{
    // This function searches through the hues looking for the largest deviations from a linear distribution
    // We can then use this to initialise the binary search range to something smaller than the full one to
    // reduce the number of lookups per hue lookup from ~ceil(log2(table size)) to ~ceil(log2(range))
    // during image rendering.

    // TODO: Padding values are a quick hack to ensure the range encloses the needed range, left as an exersize
    // for the reader to fully reason if these values could be smaller, probably best done the closer the hue
    // distribution comes to linear as the overhead becomes a potentially greater issue
    constexpr int lower_padding = -2;
    constexpr int upper_padding = 1;
    std::array<int, 2> hue_linearity_search_range = {lower_padding, upper_padding};
    for (int i = gamutCuspTable.base_index; i < gamutCuspTable.base_index + gamutCuspTable.size; i++)
    {
      const int pos = hue_position_in_uniform_table(gamutCuspTable.table[i][2], gamutCuspTable.size) + gamutCuspTable.base_index;
      const int delta = i - pos;
      hue_linearity_search_range[0] = std::min(hue_linearity_search_range[0], delta + lower_padding);
      hue_linearity_search_range[1] = std::max(hue_linearity_search_range[1], delta + upper_padding);
    }
    return hue_linearity_search_range;
}

GamutCompressParams init_GamutCompressParams(float peakLuminance, const JMhParams &inputJMhParams, const JMhParams &limitJMhParams,
                                             const ToneScaleParams &tsParams, const SharedCompressionParameters &shParams)
{
    float mid_J = Y_to_J(tsParams.c_t * reference_luminance, inputJMhParams);

    // Calculated chroma compress variables
    const float focus_dist = focus_distance + focus_distance * focus_distance_scaling * tsParams.log_peak;
    const float lower_hull_gamma_inv =  1.0f / (1.14f + 0.07f * tsParams.log_peak); // TODO: name these magic constants

    GamutCompressParams params;
    params.mid_J = mid_J;
    params.focus_dist = focus_dist;
    params.lower_hull_gamma_inv = lower_hull_gamma_inv;
    params.gamut_cusp_table = make_gamut_table(limitJMhParams, peakLuminance);
    params.hue_linearity_search_range = determine_hue_linearity_search_range(params.gamut_cusp_table);
    params.upper_hull_gamma_inv_table = make_upper_hull_gamma(params.gamut_cusp_table, params.hue_linearity_search_range,
                                                              peakLuminance, shParams.limit_J_max, mid_J, focus_dist,
                                                              lower_hull_gamma_inv, limitJMhParams); //TODO: mess of parameters
    return params;
}

} // namespace ACES2

} // OCIO namespace
