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
inline constexpr float degrees_to_radians(float d)
{
    return d / 180.0f * PI;
}

inline constexpr float radians_to_degrees(float r)
{
    return r / PI * 180.f;
}

inline float wrap_to_hue_limit(float hue) // TODO: track this and strip out unneeded calls
{
    float y = std::fmod(hue, hue_limit);
    if ( y < 0.f)
    {
        y = y + hue_limit;
    }
    return y;
}

float base_hue_for_position(int i_lo, int table_size) 
{
    const float result = i_lo * hue_limit / table_size;
    return result;
}

int hue_position_in_uniform_table(float hue, int table_size)
{
    const float wrapped_hue = wrap_to_hue_limit(hue);
    return int(wrapped_hue / hue_limit * (float) table_size);
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
        i = (i_lo + i_hi) / 2, gt.total_size;
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

    const float t = wrap_to_hue_limit(h) - base_hue;

    return lerpf(gt.table[i_lo], gt.table[i_hi], t);
}

//
// CAM
//

inline float _post_adaptation_cone_response_compression_fwd(float Rc, const float F_L_n)
{
    const float F_L_Y = powf(Rc * F_L_n, 0.42f);
    const float Ra    = (400.f * F_L_Y) / (27.13f + F_L_Y);
    return Ra;
}

inline float _post_adaptation_cone_response_compression_inv(float Ra, const float F_L_n)
{
    const float F_L_Y = (27.13f * Ra) / (400.f - Ra); // TODO: what happens when Ra >= 400.0f
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
    float h = wrap_to_hue_limit(radians_to_degrees(h_rad));

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

    const float h_rad = degrees_to_radians(h);

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

float chroma_compress_norm(float h, float chroma_compress_scale)
{
    const float h_rad = degrees_to_radians(h);
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

    const float minus_b = k3 * x - k1;
    const float minus_ac = k2 * k3 * x; // a is 1.0
    return 0.5f * (minus_b + sqrt(minus_b * minus_b + 4.f * minus_ac)); // a is 1.0, mins_b squared == b^2
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

float tonescale_fwd(const float J, const JMhParams &p, const ToneScaleParams &pt)
{
    // Tonescale applied in Y (convert to and from J)
    const float J_abs = std::abs(J);
    const float Y     = _J_to_Y(J_abs, p);
    
    const float f    = pt.m_2 * powf(Y / (Y + pt.s_2), pt.g);
    const float Y_ts = std::max(0.f, f * f / (f + pt.t_1)) * pt.n_r;  // max prevents -ve values being output also handles division by zero possibility

    const float J_ts  = _Y_to_J(Y_ts, p);
    return std::copysign(J_ts, Y_ts);
}

float tonescale_inv(const float J_ts, const JMhParams &p, const ToneScaleParams &pt)
{
    // Inverse Tonescale applied in Y (convert to and from J)
    const float J_abs = std::abs(J_ts);
    const float Y_ts  = _J_to_Y(J_abs, p);

    const float Y_ts_norm = Y_ts / reference_luminance; // TODO
    const float Z = std::max(0.f, std::min(pt.n / (pt.u_2 * pt.n_r), Y_ts_norm));  //TODO
    const float f = (Z + sqrt(Z * (4.f * pt.t_1 + Z))) / 2.f;
    const float Y = pt.s_2 / (powf((pt.m_2 / f), (1.f / pt.g)) - 1.f);

    const float J     = _Y_to_J(Y, p);
    return std::copysign(J, Y_ts);
}

f3 tonescale_chroma_compress_fwd(const f3 &JMh, const JMhParams &p, const ToneScaleParams &pt, const SharedCompressionParameters &ps, const ChromaCompressParams &pc)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];

    const float J_ts = tonescale_fwd(J, p, pt);

    // ChromaCompress
    float M_cp = M;

    if (M != 0.0)
    {
        const float nJ = J_ts / ps.limit_J_max;
        const float snJ = std::max(0.f, 1.f - nJ);
        const float Mnorm = chroma_compress_norm(h, pc.chroma_compress_scale);
        const float limit = powf(nJ, ps.model_gamma_inv) * reach_m_from_table(h, ps.reach_m_table) / Mnorm;

        M_cp = M * powf(J_ts / J, ps.model_gamma_inv);
        M_cp = M_cp / Mnorm;
        M_cp = limit - toe_fwd(limit - M_cp, limit - 0.001f, snJ * pc.sat, sqrt(nJ * nJ + pc.sat_thr));
        M_cp = toe_fwd(M_cp, limit, nJ * pc.compr, snJ);
        M_cp = M_cp * Mnorm;
    }

    return {J_ts, M_cp, h};
}

f3 tonescale_chroma_compress_inv(const f3 &JMh, const JMhParams &p, const ToneScaleParams &pt, const SharedCompressionParameters &ps, const ChromaCompressParams &pc)
{
    const float J_ts = JMh[0];
    const float M_cp = JMh[1];
    const float h    = JMh[2];

    const float J = tonescale_inv(J_ts, p, pt);

    // Inverse ChromaCompress
    float M = M_cp;

    if (M_cp != 0.0)
    {
        const float nJ = J_ts / ps.limit_J_max;
        const float snJ = std::max(0.f, 1.f - nJ);
        const float Mnorm = chroma_compress_norm(h, pc.chroma_compress_scale);
        const float limit = powf(nJ, ps.model_gamma_inv) * reach_m_from_table(h, ps.reach_m_table) / Mnorm;

        M = M_cp / Mnorm;
        M = toe_inv(M, limit, nJ * pc.compr, snJ);
        M = limit - toe_inv(limit - M, limit - 0.001f, snJ * pc.sat, sqrt(nJ * nJ + pc.sat_thr));
        M = M * Mnorm;
        M = M * powf(J_ts / J, -ps.model_gamma_inv);
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

    // Wrap the hues, to maintain monotonicity. These entries will fall outside [0.0, 360.0)
    gamutCuspTable.table[0][2] = gamutCuspTable.table[0][2] - hue_limit;
    gamutCuspTable.table[gamutCuspTable.size+1][2] = gamutCuspTable.table[gamutCuspTable.size+1][2] + hue_limit;

    return gamutCuspTable;
}

bool any_below_zero(const f3 &rgb)
{
    return (rgb[0] < 0. || rgb[1] < 0. || rgb[2] < 0.);
}

Table1D make_reach_m_table(const JMhParams &params, float peakLuminance)
{
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

f2 find_gamut_boundary_intersection(const f2 &JM_cusp_in, float J_focus, float J_max, float slope_gain, float gamma_top, float gamma_bottom, const float J_intersect_source)
{
    const float s = std::max(0.000001f, smooth_cusps); // TODO: pre smooth the cusp
    const f2 JM_cusp = {
        JM_cusp_in[0],
        JM_cusp_in[1] * (1.f + smooth_m * s)
    };

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
    const float J_boundary = J_intersect_source + slope * M_boundary; // TODO don't recalculate this

    return {J_boundary, M_boundary};
}

f3 get_reach_boundary(
    float J,
    float M,
    float h,
    const f2 &JMcusp,
    float focusJ,
    float limit_J_max,
    float model_gamma_inv,
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

    const float boundary = limit_J_max * powf(intersectJ / limit_J_max, model_gamma_inv) * reachMaxM / (limit_J_max - slope * reachMaxM);
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

f3 compressGamut(const f3 &JMh, float Jx, const ACES2::SharedCompressionParameters& sp, const ACES2::GamutCompressParams& p, const HueDependantGamutParams hdp, bool invert)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];

    if (M < 0.0001f || J > sp.limit_J_max)
    {
        return {J, 0.f, h};
    }
    else
    {
        const f2 project_from = {J, M};
        const float slope_gain = sp.limit_J_max * p.focus_dist * get_focus_gain(Jx, hdp.JMcusp[0], sp.limit_J_max);

        const float J_intersect_source = solve_J_intersect(J, M, hdp.focusJ, sp.limit_J_max, slope_gain);

        const f2 boundaryReturn = find_gamut_boundary_intersection(hdp.JMcusp, hdp.focusJ, sp.limit_J_max, slope_gain, hdp.gamma_top, hdp.gamma_bottom, J_intersect_source);
        const f2 JMboundary = {boundaryReturn[0], boundaryReturn[1]};
        const f2 project_to = {J_intersect_source, 0.f};

        if (JMboundary[1] <= 0.0f)
        {
            return {J, 0.f, h};
        }

        const f3 reachBoundary = get_reach_boundary(JMboundary[0], JMboundary[1], h, hdp.JMcusp, hdp.focusJ, sp.limit_J_max, sp.model_gamma_inv, p.focus_dist, sp.reach_m_table); // TODO

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

inline float compute_focusJ(float cusp_J, float mid_J, float limit_J_max)
{
    return lerpf(cusp_J, mid_J, std::min(1.f, cusp_mid_blend - (cusp_J / limit_J_max)));
}

f3 gamut_compress_fwd(const f3 &JMh, const SharedCompressionParameters &sp, const GamutCompressParams &p)
{
    HueDependantGamutParams hdp;
    hdp.gamma_top = hue_dependent_upper_hull_gamma(JMh[2], p.upper_hull_gamma_table);
    hdp.gamma_bottom = p.lower_hull_gamma;
    hdp.JMcusp = cusp_from_table(JMh[2], p.gamut_cusp_table);
    hdp.focusJ = compute_focusJ(hdp.JMcusp[0], p.mid_J, sp.limit_J_max);
    
    return compressGamut(JMh, JMh[0], sp, p, hdp, false);
}

f3 gamut_compress_inv(const f3 &JMh, const SharedCompressionParameters &sp, const GamutCompressParams &p)
{
    HueDependantGamutParams hdp;
    hdp.gamma_top = hue_dependent_upper_hull_gamma(JMh[2], p.upper_hull_gamma_table);
    hdp.gamma_bottom = p.lower_hull_gamma;
    hdp.JMcusp = cusp_from_table(JMh[2], p.gamut_cusp_table);
    hdp.focusJ = compute_focusJ(hdp.JMcusp[0], p.mid_J, sp.limit_J_max);

    float Jx = JMh[0];

    // Analytic inverse below threshold
    if (Jx > lerpf(hdp.JMcusp[0], sp.limit_J_max, focus_gain_blend))
    {
        // Approximation above threshold
        Jx = compressGamut(JMh, Jx, sp, p, hdp, true)[0];
    }
    return compressGamut(JMh, Jx, sp, p, hdp, true);
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
    const float focusJ = compute_focusJ(JMcusp[0], mid_J, limit_J_max);

    for (size_t testIndex = 0; testIndex < 3; testIndex++)
    {
        const float slope_gain = limit_J_max * focus_dist * get_focus_gain(testJMh[testIndex][0], JMcusp[0], limit_J_max);
        const float J_intersect_source = solve_J_intersect(testJMh[testIndex][0], testJMh[testIndex][1], focusJ, limit_J_max, slope_gain);
        const f2 approxLimit = find_gamut_boundary_intersection(JMcusp, focusJ, limit_J_max, slope_gain, topGamma, lower_hull_gamma, J_intersect_source);
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
    const float s_2 = w_2 * m_1 * reference_luminance;
    const float u_2 = powf((r_hit/m_1)/((r_hit/m_1) + w_2), g);
    const float m_2 = m_1 / u_2;
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
        make_reach_m_table(compressionGamut, peakLuminance)
    };
    return params;
}

ChromaCompressParams init_ChromaCompressParams(float peakLuminance, const ToneScaleParams &tsParams)
{
    // Calculated chroma compress variables
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

GamutCompressParams init_GamutCompressParams(float peakLuminance, const JMhParams &inputJMhParams, const JMhParams &limitJMhParams,
                                             const ToneScaleParams &tsParams, const SharedCompressionParameters &shParams)
{
    float mid_J = Y_to_J(tsParams.c_t * reference_luminance, inputJMhParams); // TODO

    // Calculated chroma compress variables
    const float focus_dist = focus_distance + focus_distance * focus_distance_scaling * tsParams.log_peak;
    const float lower_hull_gamma =  1.14f + 0.07f * tsParams.log_peak;

    GamutCompressParams params = {
        mid_J,
        focus_dist,
        lower_hull_gamma,
        make_gamut_table(limitJMhParams, peakLuminance),
        make_upper_hull_gamma(params.gamut_cusp_table, peakLuminance, shParams.limit_J_max,
                              mid_J, focus_dist, lower_hull_gamma, limitJMhParams)
    };
    return params;
}

} // namespace ACES2

} // OCIO namespace
