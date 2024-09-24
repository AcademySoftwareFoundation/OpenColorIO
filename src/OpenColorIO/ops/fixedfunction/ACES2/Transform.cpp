// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "Transform.h"


namespace OCIO_NAMESPACE
{

namespace ACES2
{

//
// Table lookups
//

float wrap_to_360(float hue)
{
    float y = std::fmod(hue, 360.f);
    if ( y < 0.f)
    {
        y = y + 360.f;
    }
    return y;
}

float base_hue_for_position(int i_lo, int table_size) 
{
    const float result = i_lo * 360.f / table_size;
    return result;
}

int hue_position_in_uniform_table(float hue, int table_size)
{
    const float wrapped_hue = wrap_to_360(hue);
    return int(wrapped_hue / 360.f * (float) table_size);
}

int next_position_in_table(int entry, int table_size)
{
    return (entry + 1) % table_size;
}

int clamp_to_table_bounds(int entry, int table_size)
{
    return std::min(table_size - 1, std::max(0, entry));
}

f2 cusp_from_table(float h, const Table3D &gt)
{
    int i_lo = 0;
    int i_hi = gt.base_index + gt.size; // allowed as we have an extra entry in the table
    int i = clamp_to_table_bounds(hue_position_in_uniform_table(h, gt.size) + gt.base_index, gt.total_size);

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
        i = clamp_to_table_bounds((i_lo + i_hi) / 2, gt.total_size);
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
    const int i_lo = clamp_to_table_bounds(hue_position_in_uniform_table(h, gt.size), gt.total_size);
    const int i_hi = clamp_to_table_bounds(next_position_in_table(i_lo, gt.size), gt.total_size);

    const float t = (h - i_lo) / (i_hi - i_lo);
    return lerpf(gt.table[i_lo], gt.table[i_hi], t);
}

float hue_dependent_upper_hull_gamma(float h, const ACES2::Table1D &gt)
{
    const int i_lo = clamp_to_table_bounds(hue_position_in_uniform_table(h, gt.size) + gt.base_index, gt.total_size);
    const int i_hi = clamp_to_table_bounds(next_position_in_table(i_lo, gt.size), gt.total_size);

    const float base_hue = (float) (i_lo - gt.base_index);

    const float t = wrap_to_360(h) - base_hue;

    return lerpf(gt.table[i_lo], gt.table[i_hi], t);
}

//
// CAM
//

// Post adaptation non linear response compression
float panlrc_forward(float v, float F_L)
{
    const float F_L_v = powf(F_L * std::abs(v) / reference_luminance, 0.42f);
    // Note that std::copysign(1.f, 0.f) returns 1 but the CTL copysign(1.,0.) returns 0.
    // TODO: Should we change the behaviour?
    return (400.f * std::copysign(1.f, v) * F_L_v) / (27.13f + F_L_v);
}

float panlrc_inverse(float v, float F_L)
{
    return std::copysign(1.f, v) * reference_luminance / F_L * powf((27.13f * std::abs(v) / (400.f - std::abs(v))), 1.f / 0.42f);
}

// Optimization used during initialization
float Y_to_J(float Y, const JMhParams &params)
{
    float F_L_Y = powf(params.F_L * std::abs(Y) / reference_luminance, 0.42f);
    return std::copysign(1.f, Y) * reference_luminance * powf(((400.f * F_L_Y) / (27.13f + F_L_Y)) / params.A_w_J, surround[1] * params.z);
}

f3 RGB_to_JMh(const f3 &RGB, const JMhParams &p)
{
    const float red = RGB[0];
    const float grn = RGB[1];
    const float blu = RGB[2];

    const float red_m = red * p.MATRIX_RGB_to_CAM16[0] + grn * p.MATRIX_RGB_to_CAM16[1] + blu * p.MATRIX_RGB_to_CAM16[2];
    const float grn_m = red * p.MATRIX_RGB_to_CAM16[3] + grn * p.MATRIX_RGB_to_CAM16[4] + blu * p.MATRIX_RGB_to_CAM16[5];
    const float blu_m = red * p.MATRIX_RGB_to_CAM16[6] + grn * p.MATRIX_RGB_to_CAM16[7] + blu * p.MATRIX_RGB_to_CAM16[8];

    const float red_a =  panlrc_forward(red_m * p.D_RGB[0], p.F_L);
    const float grn_a =  panlrc_forward(grn_m * p.D_RGB[1], p.F_L);
    const float blu_a =  panlrc_forward(blu_m * p.D_RGB[2], p.F_L);

    const float A = 2.f * red_a + grn_a + 0.05f * blu_a;
    const float a = red_a - 12.f * grn_a / 11.f + blu_a / 11.f;
    const float b = (red_a + grn_a - 2.f * blu_a) / 9.f;

    const float J = 100.f * powf(A / p.A_w, surround[1] * p.z);

    const float M = J == 0.f ? 0.f : 43.f * surround[2] * sqrt(a * a + b * b);

    const float PI = 3.14159265358979f;
    const float h_rad = std::atan2(b, a);
    float h = std::fmod(h_rad * 180.f / PI, 360.f);
    if (h < 0.f)
    {
        h += 360.f;
    }

    return {J, M, h};
}

f3 JMh_to_RGB(const f3 &JMh, const JMhParams &p)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];

    const float PI = 3.14159265358979f;
    const float h_rad = h * PI / 180.f;

    const float scale = M / (43.f * surround[2]);
    const float A = p.A_w * powf(J / 100.f, 1.f / (surround[1] * p.z));
    const float a = scale * cos(h_rad);
    const float b = scale * sin(h_rad);

    const float red_a = (460.f * A + 451.f * a + 288.f * b) / 1403.f;
    const float grn_a = (460.f * A - 891.f * a - 261.f * b) / 1403.f;
    const float blu_a = (460.f * A - 220.f * a - 6300.f * b) / 1403.f;

    float red_m = panlrc_inverse(red_a, p.F_L) / p.D_RGB[0];
    float grn_m = panlrc_inverse(grn_a, p.F_L) / p.D_RGB[1];
    float blu_m = panlrc_inverse(blu_a, p.F_L) / p.D_RGB[2];

    const float red = red_m * p.MATRIX_CAM16_to_RGB[0] + grn_m * p.MATRIX_CAM16_to_RGB[1] + blu_m * p.MATRIX_CAM16_to_RGB[2];
    const float grn = red_m * p.MATRIX_CAM16_to_RGB[3] + grn_m * p.MATRIX_CAM16_to_RGB[4] + blu_m * p.MATRIX_CAM16_to_RGB[5];
    const float blu = red_m * p.MATRIX_CAM16_to_RGB[6] + grn_m * p.MATRIX_CAM16_to_RGB[7] + blu_m * p.MATRIX_CAM16_to_RGB[8];

    return {red, grn, blu};
}

//
// Tonescale / Chroma compress
//

float chroma_compress_norm(float h, float chroma_compress_scale)
{
    const float PI = 3.14159265358979f;
    const float h_rad = h / 180.f * PI;
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

    return M * chroma_compress_scale;
}

float toe_fwd( float x, float limit, float k1_in, float k2_in)
{
    if (x > limit)
    {
        return x;
    }

    const float k2 = std::max(k2_in, 0.001f);
    const float k1 = sqrt(k1_in * k1_in + k2 * k2);
    const float k3 = (limit + k1) / (limit + k2);
    return 0.5f * (k3 * x - k1 + sqrt((k3 * x - k1) * (k3 * x - k1) + 4.f * k2 * k3 * x));
}

float toe_inv( float x, float limit, float k1_in, float k2_in)
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

f3 tonescale_chroma_compress_fwd(const f3 &JMh, const JMhParams &p, const ToneScaleParams &pt, const ChromaCompressParams &pc)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];

    // Tonescale applied in Y (convert to and from J)
    const float A = p.A_w_J * powf(std::abs(J) / 100.f, 1.f / (surround[1] * p.z));
    const float Y = std::copysign(1.f, J) * 100.f / p.F_L * powf((27.13f * A) / (400.f - A), 1.f / 0.42f) / 100.f;

    const float f = pt.m_2 * powf(std::max(0.f, Y) / (Y + pt.s_2), pt.g);
    const float Y_ts = std::max(0.f, f * f / (f + pt.t_1)) * pt.n_r;

    const float F_L_Y = powf(p.F_L * std::abs(Y_ts) / 100.f, 0.42f);
    const float J_ts = std::copysign(1.f, Y_ts) * 100.f * powf(((400.f * F_L_Y) / (27.13f + F_L_Y)) / p.A_w_J, surround[1] * p.z);

    // ChromaCompress
    float M_cp = M;

    if (M != 0.0)
    {
        const float nJ = J_ts / pc.limit_J_max;
        const float snJ = std::max(0.f, 1.f - nJ);
        const float Mnorm = chroma_compress_norm(h, pc.chroma_compress_scale);
        const float limit = powf(nJ, pc.model_gamma) * reach_m_from_table(h, pc.reach_m_table) / Mnorm;

        M_cp = M * powf(J_ts / J, pc.model_gamma);
        M_cp = M_cp / Mnorm;
        M_cp = limit - toe_fwd(limit - M_cp, limit - 0.001f, snJ * pc.sat, sqrt(nJ * nJ + pc.sat_thr));
        M_cp = toe_fwd(M_cp, limit, nJ * pc.compr, snJ);
        M_cp = M_cp * Mnorm;
    }

    return {J_ts, M_cp, h};
}

f3 tonescale_chroma_compress_inv(const f3 &JMh, const JMhParams &p, const ToneScaleParams &pt, const ChromaCompressParams &pc)
{
    const float J_ts = JMh[0];
    const float M_cp = JMh[1];
    const float h    = JMh[2];

    // Inverse Tonescale applied in Y (convert to and from J)
    const float A = p.A_w_J * powf(std::abs(J_ts) / 100.f, 1.f / (surround[1] * p.z));
    const float Y_ts = std::copysign(1.f, J_ts) * 100.f / p.F_L * powf((27.13f * A) / (400.f - A), 1.f / 0.42f) / 100.f;

    const float Z = std::max(0.f, std::min(pt.n / (pt.u_2 * pt.n_r), Y_ts));
    const float ht = (Z + sqrt(Z * (4.f * pt.t_1 + Z))) / 2.f;
    const float Y = pt.s_2 / (powf((pt.m_2 / ht), (1.f / pt.g)) - 1.f) * pt.n_r;

    const float F_L_Y = powf(p.F_L * std::abs(Y) / 100.f, 0.42f);
    const float J = std::copysign(1.f, Y) * 100.f * powf(((400.f * F_L_Y) / (27.13f + F_L_Y)) / p.A_w_J, surround[1] * p.z);

    // Inverse ChromaCompress
    float M = M_cp;

    if (M_cp != 0.0)
    {
        const float nJ = J_ts / pc.limit_J_max;
        const float snJ = std::max(0.f, 1.f - nJ);
        const float Mnorm = chroma_compress_norm(h, pc.chroma_compress_scale);
        const float limit = powf(nJ, pc.model_gamma) * reach_m_from_table(h, pc.reach_m_table) / Mnorm;

        M = M_cp / Mnorm;
        M = toe_inv(M, limit, nJ * pc.compr, snJ);
        M = limit - toe_inv(limit - M, limit - 0.001f, snJ * pc.sat, sqrt(nJ * nJ + pc.sat_thr));
        M = M * Mnorm;
        M = M * powf(J_ts / J, -pc.model_gamma);
    }

    return {J, M, h};
}

JMhParams init_JMhParams(const Primaries &P)
{
    JMhParams p;

    const m33f MATRIX_16 = XYZtoRGB_f33(CAM16::primaries);
    const m33f RGB_to_XYZ = RGBtoXYZ_f33(P);
    const f3 XYZ_w = mult_f3_f33(f3_from_f(reference_luminance), RGB_to_XYZ);

    const float Y_W = XYZ_w[1];

    const f3 RGB_w = mult_f3_f33(XYZ_w, MATRIX_16);

    // Viewing condition dependent parameters
    const float K = 1.f / (5.f * L_A + 1.f);
    const float K4 = powf(K, 4.f);
    const float N = Y_b / Y_W;
    const float F_L = 0.2f * K4 * (5.f * L_A) + 0.1f * powf((1.f - K4), 2.f) * powf(5.f * L_A, 1.f/3.f);
    const float z = 1.48f + sqrt(N);

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
        panlrc_forward(RGB_WC[0], F_L),
        panlrc_forward(RGB_WC[1], F_L),
        panlrc_forward(RGB_WC[2], F_L)
    };

    const float A_w = ra * RGB_AW[0] + RGB_AW[1] + ba * RGB_AW[2];

    const float F_L_W = powf(F_L, 0.42f);
    const float A_w_J   = (400.f * F_L_W) / (27.13f + F_L_W);

    p.XYZ_w = XYZ_w;
    p.F_L = F_L;
    p.z = z;
    p.D_RGB = D_RGB;
    p.A_w = A_w;
    p.A_w_J = A_w_J;

    p.MATRIX_RGB_to_CAM16 = mult_f33_f33(RGBtoRGB_f33(P, CAM16::primaries), scale_f33(Identity_M33, f3_from_f(100.f)));
    p.MATRIX_CAM16_to_RGB = invert_f33(p.MATRIX_RGB_to_CAM16);

    return p;
}

Table3D make_gamut_table(const Primaries &P, float peakLuminance)
{
    const JMhParams params = init_JMhParams(P);

    Table3D gamutCuspTableUnsorted{};
    for (int i = 0; i < gamutCuspTableUnsorted.size; i++)
    {
        const float hNorm = (float) i / gamutCuspTableUnsorted.size;
        const f3 HSV = {hNorm, 1., 1.};
        const f3 RGB = HSV_to_RGB(HSV);
        const f3 scaledRGB = mult_f_f3(peakLuminance / reference_luminance, RGB);
        const f3 JMh = RGB_to_JMh(scaledRGB, params);

        gamutCuspTableUnsorted.table[i][0] = JMh[0];
        gamutCuspTableUnsorted.table[i][1] = JMh[1];
        gamutCuspTableUnsorted.table[i][2] = JMh[2];
    }

    int minhIndex = 0;
    for (int i = 0; i < gamutCuspTableUnsorted.size; i++)
    {
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

    // Wrap the hues, to maintain monotonicity. These entries will fall outside [0.0, 360.0]
    gamutCuspTable.table[0][2] = gamutCuspTable.table[0][2] - 360.f;
    gamutCuspTable.table[gamutCuspTable.size+1][2] = gamutCuspTable.table[gamutCuspTable.size+1][2] + 360.f;

    return gamutCuspTable;
}

bool any_below_zero(const f3 &rgb)
{
    return (rgb[0] < 0. || rgb[1] < 0. || rgb[2] < 0.);
}

Table1D make_reach_m_table(const Primaries &P, float peakLuminance)
{
    const JMhParams params = init_JMhParams(P);
    const float limit_J_max = Y_to_J(peakLuminance, params);

    Table1D gamutReachTable{};

    for (int i = 0; i < gamutReachTable.size; i++) {
        const float hue = (float) i;

        const float search_range = 50.f;
        float low = 0.;
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

float get_focus_gain(float J, float cuspJ, float limit_J_max)
{
    const float thr = lerpf(cuspJ, limit_J_max, focus_gain_blend);

    if (J > thr)
    {
        // Approximate inverse required above threshold
        float gain = (limit_J_max - thr) / std::max(0.0001f, (limit_J_max - std::min(limit_J_max, J)));
        return powf(log10(gain), 1.f / focus_adjust_gain) + 1.f;
    }
    else
    {
        // Analytic inverse possible below cusp
        return 1.f;
    }
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

float smin(float a, float b, float s)
{
    const float h = std::max(s - std::abs(a - b), 0.f) / s;
    return std::min(a, b) - h * h * h * s * (1.f / 6.f);
}

f3 find_gamut_boundary_intersection(const f3 &JMh_s, const f2 &JM_cusp_in, float J_focus, float J_max, float slope_gain, float gamma_top, float gamma_bottom)
{
    const float s = std::max(0.000001f, smooth_cusps);
    const f2 JM_cusp = {
        JM_cusp_in[0],
        JM_cusp_in[1] * (1.f + smooth_m * s)
    };

    const float J_intersect_source = solve_J_intersect(JMh_s[0], JMh_s[1], J_focus, J_max, slope_gain);
    const float J_intersect_cusp = solve_J_intersect(JM_cusp[0], JM_cusp[1], J_focus, J_max, slope_gain);

    float slope = 0.f;
    if (J_intersect_source < J_focus)
    {
        slope = J_intersect_source * (J_intersect_source - J_focus) / (J_focus * slope_gain);
    }
    else
    {
        slope = (J_max - J_intersect_source) * (J_intersect_source - J_focus) / (J_focus * slope_gain);
    }

    const float M_boundary_lower = J_intersect_cusp * powf(J_intersect_source / J_intersect_cusp, 1.f / gamma_bottom) / (JM_cusp[0] / JM_cusp[1] - slope);
    const float M_boundary_upper = JM_cusp[1] * (J_max - J_intersect_cusp) * powf((J_max - J_intersect_source) / (J_max - J_intersect_cusp), 1.f / gamma_top) / (slope * JM_cusp[1] + J_max - JM_cusp[0]);
    const float M_boundary = JM_cusp[1] * smin(M_boundary_lower / JM_cusp[1], M_boundary_upper / JM_cusp[1], s);
    const float J_boundary = J_intersect_source + slope * M_boundary;

    return {J_boundary, M_boundary, J_intersect_source};
}

f3 get_reach_boundary(
    float J,
    float M,
    float h,
    const f2 &JMcusp,
    float focusJ,
    float limit_J_max,
    float model_gamma,
    float focus_dist,
    const ACES2::Table1D & reach_m_table
)
{
    const float reachMaxM = reach_m_from_table(h, reach_m_table);

    const float slope_gain = limit_J_max * focus_dist * get_focus_gain(J, JMcusp[0], limit_J_max);

    const float intersectJ = solve_J_intersect(J, M, focusJ, limit_J_max, slope_gain);

    float slope;
    if (intersectJ < focusJ)
    {
        slope = intersectJ * (intersectJ - focusJ) / (focusJ * slope_gain);
    }
    else
    {
        slope = (limit_J_max - intersectJ) * (intersectJ - focusJ) / (focusJ * slope_gain);
    }

    const float boundary = limit_J_max * powf(intersectJ / limit_J_max, model_gamma) * reachMaxM / (limit_J_max - slope * reachMaxM);
    return {J, boundary, h};
}

float compression_function(
    float v,
    float thr,
    float lim,
    bool invert)
{
    float s = (lim - thr) * (1.f - thr) / (lim - 1.f);
    float nd = (v - thr) / s;

    float vCompressed;

    if (invert) {
        if (v < thr || lim <= 1.0001f || v > thr + s) {
            vCompressed = v;
        } else {
            vCompressed = thr + s * (-nd / (nd - 1.f));
        }
    } else {
        if (v < thr || lim <= 1.0001f) {
            vCompressed = v;
        } else {
            vCompressed = thr + s * nd / (1.f + nd);
        }
    }

    return vCompressed;
}

f3 compressGamut(const f3 &JMh, float Jx, const ACES2::GamutCompressParams& p, bool invert)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];

    if (M < 0.0001f || J > p.limit_J_max)
    {
        return {J, 0.f, h};
    }
    else
    {
        const f2 project_from = {J, M};
        const f2 JMcusp = cusp_from_table(h, p.gamut_cusp_table);
        const float focusJ = lerpf(JMcusp[0], p.mid_J, std::min(1.f, cusp_mid_blend - (JMcusp[0] / p.limit_J_max)));
        const float slope_gain = p.limit_J_max * p.focus_dist * get_focus_gain(Jx, JMcusp[0], p.limit_J_max);

        const float gamma_top = hue_dependent_upper_hull_gamma(h, p.upper_hull_gamma_table);
        const float gamma_bottom = p.lower_hull_gamma;

        const f3 boundaryReturn = find_gamut_boundary_intersection({J, M, h}, JMcusp, focusJ, p.limit_J_max, slope_gain, gamma_top, gamma_bottom);
        const f2 JMboundary = {boundaryReturn[0], boundaryReturn[1]};
        const f2 project_to = {boundaryReturn[2], 0.f};

        if (JMboundary[1] <= 0.0f)
        {
            return {J, 0.f, h};
        }

        const f3 reachBoundary = get_reach_boundary(JMboundary[0], JMboundary[1], h, JMcusp, focusJ, p.limit_J_max, p.model_gamma, p.focus_dist, p.reach_m_table);

        const float difference = std::max(1.0001f, reachBoundary[1] / JMboundary[1]);
        const float threshold = std::max(compression_threshold, 1.f / difference);

        float v = project_from[1] / JMboundary[1];
        v = compression_function(v, threshold, difference, invert);

        const f2 JMcompressed {
            project_to[0] + v * (JMboundary[0] - project_to[0]),
            project_to[1] + v * (JMboundary[1] - project_to[1])
        };

        return {JMcompressed[0], JMcompressed[1], h};
    }
}

f3 gamut_compress_fwd(const f3 &JMh, const GamutCompressParams &p)
{
    return compressGamut(JMh, JMh[0], p, false);
}

f3 gamut_compress_inv(const f3 &JMh, const GamutCompressParams &p)
{
    const f2 JMcusp = cusp_from_table(JMh[2], p.gamut_cusp_table);
    float Jx = JMh[0];

    f3 unCompressedJMh;

    // Analytic inverse below threshold
    if (Jx <= lerpf(JMcusp[0], p.limit_J_max, focus_gain_blend))
    {
        unCompressedJMh = compressGamut(JMh, Jx, p, true);
    }
    // Approximation above threshold
    else
    {
        Jx = compressGamut(JMh, Jx, p, true)[0];
        unCompressedJMh = compressGamut(JMh, Jx, p, true);
    }

    return unCompressedJMh;
}

bool evaluate_gamma_fit(
    const f2 &JMcusp,
    const f3 testJMh[3],
    float topGamma,
    float peakLuminance,
    float limit_J_max,
    float mid_J,
    float focus_dist,
    float lower_hull_gamma,
    const JMhParams &limitJMhParams)
{
    const float focusJ = lerpf(JMcusp[0], mid_J, std::min(1.f, cusp_mid_blend - (JMcusp[0] / limit_J_max)));

    for (size_t testIndex = 0; testIndex < 3; testIndex++)
    {
        const float slope_gain = limit_J_max * focus_dist * get_focus_gain(testJMh[testIndex][0], JMcusp[0], limit_J_max);
        const f3 approxLimit = find_gamut_boundary_intersection(testJMh[testIndex], JMcusp, focusJ, limit_J_max, slope_gain, topGamma, lower_hull_gamma);
        const f3 approximate_JMh = {approxLimit[0], approxLimit[1], testJMh[testIndex][2]};
        const f3 newLimitRGB = JMh_to_RGB(approximate_JMh, limitJMhParams);
        const f3 newLimitRGBScaled = mult_f_f3(reference_luminance / peakLuminance, newLimitRGB);

        if (!outside_hull(newLimitRGBScaled))
        {
            return false;
        }
    }

    return true;
}

Table1D make_upper_hull_gamma(
    const Table3D &gamutCuspTable,
    float peakLuminance,
    float limit_J_max,
    float mid_J,
    float focus_dist,
    float lower_hull_gamma,
    const JMhParams &limitJMhParams)
{
    const int test_count = 3;
    const float testPositions[test_count] = {0.01f, 0.5f, 0.99f};

    Table1D gammaTable{};
    Table1D gamutTopGamma{};

    for (int i = 0; i < gammaTable.size; i++)
    {
        gammaTable.table[i] = -1.f;

        const float hue = (float) i;
        const f2 JMcusp = cusp_from_table(hue, gamutCuspTable);

        f3 testJMh[test_count]{};
        for (int testIndex = 0; testIndex < test_count; testIndex++)
        {
            const float testJ = JMcusp[0] + ((limit_J_max - JMcusp[0]) * testPositions[testIndex]);
            testJMh[testIndex] = {
                testJ,
                JMcusp[1],
                hue
            };
        }

        const float search_range = gammaSearchStep;
        float low = gammaMinimum;
        float high = low + search_range;
        bool outside = false;

        while (!(outside) && (high < 5.f))
        {
            const bool gammaFound = evaluate_gamma_fit(JMcusp, testJMh, high, peakLuminance, limit_J_max, mid_J, focus_dist, lower_hull_gamma, limitJMhParams);
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
            const bool gammaFound = evaluate_gamma_fit(JMcusp, testJMh, testGamma, peakLuminance, limit_J_max, mid_J, focus_dist, lower_hull_gamma, limitJMhParams);
            if (gammaFound)
            {
                high = testGamma;
                gammaTable.table[i] = high;
            }
            else
            {
                low = testGamma;
            }
        }

        // Duplicate gamma value to array, leaving empty entries at first and last position
        gamutTopGamma.table[i+gamutTopGamma.base_index] = gammaTable.table[i];
    }

    // Copy last populated entry to first empty spot
    gamutTopGamma.table[0] = gammaTable.table[gammaTable.size-1];

    // Copy first populated entry to last empty spot
    gamutTopGamma.table[gamutTopGamma.total_size-1] = gammaTable.table[0];

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
    const float s_2 = w_2 * m_1;
    const float u_2 = powf((r_hit/m_1)/((r_hit/m_1) + w_2), g);
    const float m_2 = m_1 / u_2;

    ToneScaleParams TonescaleParams = {
        n,
        n_r,
        g,
        t_1,
        c_t,
        s_2,
        u_2,
        m_2
    };

    return TonescaleParams;
}

ChromaCompressParams init_ChromaCompressParams(float peakLuminance)
{
    const ToneScaleParams tsParams = init_ToneScaleParams(peakLuminance);
    const JMhParams inputJMhParams = init_JMhParams(ACES_AP0::primaries);

    float limit_J_max = Y_to_J(peakLuminance, inputJMhParams);

    // Calculated chroma compress variables
    const float log_peak = log10( tsParams.n / tsParams.n_r);
    const float compr = chroma_compress + (chroma_compress * chroma_compress_fact) * log_peak;
    const float sat = std::max(0.2f, chroma_expand - (chroma_expand * chroma_expand_fact) * log_peak);
    const float sat_thr = chroma_expand_thr / tsParams.n;
    const float model_gamma = 1.f / (surround[1] * (1.48f + sqrt(Y_b / L_A)));

    ChromaCompressParams params{};
    params.limit_J_max = limit_J_max;
    params.model_gamma = model_gamma;
    params.sat = sat;
    params.sat_thr = sat_thr;
    params.compr = compr;
    params.chroma_compress_scale = powf(0.03379f * peakLuminance, 0.30596f) - 0.45135f;
    params.reach_m_table = make_reach_m_table(ACES_AP1::primaries, peakLuminance);
    return params;
}

GamutCompressParams init_GamutCompressParams(float peakLuminance, const Primaries &limitingPrimaries)
{
    const ToneScaleParams tsParams = init_ToneScaleParams(peakLuminance);
    const JMhParams inputJMhParams = init_JMhParams(ACES_AP0::primaries);

    float limit_J_max = Y_to_J(peakLuminance, inputJMhParams);
    float mid_J = Y_to_J(tsParams.c_t * 100.f, inputJMhParams);

    // Calculated chroma compress variables
    const float log_peak = log10( tsParams.n / tsParams.n_r);
    const float model_gamma = 1.f / (surround[1] * (1.48f + sqrt(Y_b / L_A)));
    const float focus_dist = focus_distance + focus_distance * focus_distance_scaling * log_peak;
    const float lower_hull_gamma =  1.14f + 0.07f * log_peak;

    const JMhParams limitJMhParams = init_JMhParams(limitingPrimaries);

    GamutCompressParams params{};
    params.limit_J_max = limit_J_max;
    params.mid_J = mid_J;
    params.model_gamma = model_gamma;
    params.focus_dist = focus_dist;
    params.lower_hull_gamma = lower_hull_gamma;
    params.reach_m_table = make_reach_m_table(ACES_AP1::primaries, peakLuminance);
    params.gamut_cusp_table = make_gamut_table(limitingPrimaries, peakLuminance);
    params.upper_hull_gamma_table = make_upper_hull_gamma(
        params.gamut_cusp_table,
        peakLuminance,
        limit_J_max,
        mid_J,
        focus_dist,
        lower_hull_gamma,
        limitJMhParams);

    return params;
}

} // namespace ACES2

} // OCIO namespace
