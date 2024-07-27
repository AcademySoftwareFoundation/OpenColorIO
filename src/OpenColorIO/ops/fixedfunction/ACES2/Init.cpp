// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "Init.h"


namespace OCIO_NAMESPACE
{

namespace ACES2
{

inline float smin(float a, float b, float s)
{
    const float h = std::max(s - std::abs(a - b), 0.f) / s;
    return std::min(a, b) - h * h * h * s * (1.f / 6.f);
}

inline float wrap_to_360(float hue)
{
    float y = std::fmod(hue, 360.f);
    if ( y < 0.f)
    {
        y = y + 360.f;
    }
    return y;
}

inline float radians_to_degrees(float radians)
{
    return radians * 180.f / M_PI;
}

inline float degrees_to_radians(float degrees)
{
    return degrees / 180.f * M_PI;
}

inline float base_hue_for_position(int i_lo, int table_size) 
{
    const float result = i_lo * 360. / table_size;
    return result;
}

inline int hue_position_in_uniform_table(float hue, int table_size) 
{
    const float wrapped_hue = wrap_to_360( hue);
    const int result = (wrapped_hue / 360.f * table_size);
    return result;
}

inline int next_position_in_table(int entry, int table_size)
{
    const int result = (entry + 1) % table_size;
    return result;
}

inline f2 cuspFromTable(float h, const Table3D &gt)
{
    float lo[3]{};
    float hi[3]{};

    int low_i = 0;
    int high_i = gt.base_index + gt.size; // allowed as we have an extra entry in the table
    int i = hue_position_in_uniform_table(h, gt.size) + gt.base_index;

    while (low_i + 1 < high_i)
    {
        if (h > gt.table[i][2])
        {
            low_i = i;
        }
        else
        {
            high_i = i;
        }
        i = (low_i + high_i) / 2.f;
    }

    lo[0] = gt.table[high_i-1][0];
    lo[1] = gt.table[high_i-1][1];
    lo[2] = gt.table[high_i-1][2];

    hi[0] = gt.table[high_i][0];
    hi[1] = gt.table[high_i][1];
    hi[2] = gt.table[high_i][2];
    
    float t = (h - lo[2]) / (hi[2] - lo[2]);
    float cuspJ = lerpf( lo[0], hi[0], t);
    float cuspM = lerpf( lo[1], hi[1], t);
    
    return { cuspJ, cuspM };
}

inline float reachMFromTable(float h, const Table1D &gt)
{
    int i_lo = hue_position_in_uniform_table( h, gt.size);
    int i_hi = next_position_in_table( i_lo, gt.size);
    
    const float t = (h - i_lo) / (i_hi - i_lo);

    return lerpf(gt.table[i_lo], gt.table[i_hi], t);
}

// Post adaptation non linear response compression
float panlrc_forward(float v, float F_L)
{
    float F_L_v = pow(F_L * std::abs(v) / reference_luminance, 0.42f);
    float c = (400.f * std::copysign(1., v) * F_L_v) / (27.13f + F_L_v);
    return c;
}

float panlrc_inverse(float v, float F_L)
{
    float p = std::copysign(1., v) * reference_luminance / F_L * pow((27.13f * std::abs(v) / (400.f - std::abs(v))), 1.f / 0.42f);
    return p;
}

float Hellwig_J_to_Y(float J, const JMhParams &params)
{
    const float A = params.A_w_J * pow(std::abs(J) / reference_luminance, 1.f / (surround[1] * params.z));
    return std::copysign(1.f, J) * reference_luminance / params.F_L * pow((27.13f * A) / (400.f - A), 1.f / 0.42f);
}

float Y_to_Hellwig_J(float Y, const JMhParams &params)
{
    float F_L_Y = pow(params.F_L * std::abs(Y) / 100., 0.42);
    return std::copysign(1.f, Y) * reference_luminance * pow(((400.f * F_L_Y) / (27.13f + F_L_Y)) / params.A_w_J, surround[1] * params.z);
}

f3 XYZ_to_Hellwig2022_JMh(const f3 &XYZ, const JMhParams &params)
{
    // Step 1 - Converting CIE XYZ tristimulus values to sharpened RGB values
    const f3 RGB = mult_f3_f33(XYZ, params.MATRIX_16);

    // Step 2
    const f3 RGB_c = {
        params.D_RGB[0] * RGB[0],
        params.D_RGB[1] * RGB[1],
        params.D_RGB[2] * RGB[2]
    };

    // Step 3 - apply  forward post-adaptation non-linear response compression
    const f3 RGB_a = {
        panlrc_forward(RGB_c[0], params.F_L),
        panlrc_forward(RGB_c[1], params.F_L),
        panlrc_forward(RGB_c[2], params.F_L)
    };

    // Converting to preliminary cartesian coordinates
    const float A = ra * RGB_a[0] + RGB_a[1] + ba * RGB_a[2];
    const float a = RGB_a[0] - 12.f * RGB_a[1] / 11.f + RGB_a[2] / 11.f;
    const float b = (RGB_a[0] + RGB_a[1] - 2.f * RGB_a[2]) / 9.f;

    // Computing the hue angle math h
    const float hr = atan2(b, a);
    const float h = wrap_to_360(radians_to_degrees(hr));

    // Computing the correlate of lightness, J
    const float J = reference_luminance * pow( A / params.A_w, surround[1] * params.z);

    // Computing the correlate of colourfulness, M
    const float M = J == 0.f ? 0.f : 43.f * surround[2] * sqrt(a * a + b * b);

    return {J, M, h};
}

f3 Hellwig2022_JMh_to_XYZ(const f3 &JMh, const JMhParams &params)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];

    const float hr = degrees_to_radians(h);

    // Computing achromatic respons A for the stimulus
    const float A = params.A_w * pow(J / reference_luminance, 1. / (surround[1] * params.z));

    // Computing opponent colour dimensions a and b
    const float scale = M / (43.f * surround[2]);
    const float a = scale * cos(hr);
    const float b = scale * sin(hr);

    // Step 4 - Applying post-adaptation non-linear response compression matrix
    const f3 RGB_a {
        (460.f * A + 451.f * a + 288.f * b) / 1403.f,
        (460.f * A - 891.f * a - 261.f * b) / 1403.f,
        (460.f * A - 220.f * a - 6300.f * b) / 1403.f
    };

    // Step 5 - Applying the inverse post-adaptation non-linear respnose compression
    const f3 RGB_c = {
        panlrc_inverse(RGB_a[0], params.F_L),
        panlrc_inverse(RGB_a[1], params.F_L),
        panlrc_inverse(RGB_a[2], params.F_L)
    };

    // Step 6
    const f3 RGB = {
        RGB_c[0] / params.D_RGB[0],
        RGB_c[1] / params.D_RGB[1],
        RGB_c[2] / params.D_RGB[2]
    };

    // Step 7
    f3 XYZ = mult_f3_f33(RGB, params.MATRIX_16_INV);

    return XYZ;
}

f3 RGB_to_JMh(const f3 &RGB, const m33f &RGB_TO_XYZ, float luminance, const JMhParams &params)
{
    const f3 luminanceRGB = mult_f_f3(luminance, RGB);
    const f3 XYZ = mult_f3_f33(luminanceRGB, RGB_TO_XYZ);
    const f3 JMh = XYZ_to_Hellwig2022_JMh(XYZ, params);
    return JMh;
}

f3 JMh_to_RGB(const f3 &JMh, const m33f &XYZ_TO_RGB, float luminance, const JMhParams &params)
{
    const f3 luminanceXYZ = Hellwig2022_JMh_to_XYZ(JMh, params);
    const f3 luminanceRGB = mult_f3_f33(luminanceXYZ, XYZ_TO_RGB);
    const f3 RGB = mult_f_f3(1.f / luminance, luminanceRGB);
    return RGB;
}

JMhParams init_JMhParams(const Primaries &P)
{
    JMhParams p;

    const m33f MATRIX_16 = XYZtoRGB_f33(CAM16::primaries);
    const m33f MATRIX_16_INV = RGBtoXYZ_f33(CAM16::primaries);

    const m33f RGB_to_XYZ = RGBtoXYZ_f33(P);
    const f3 XYZ_w = mult_f3_f33(f3_from_f(reference_luminance), RGB_to_XYZ);

    const float Y_W = XYZ_w[1];

    const f3 RGB_w = mult_f3_f33(XYZ_w, MATRIX_16);

    // Viewing condition dependent parameters
    const float K = 1.f / (5.f * L_A + 1.f);
    const float K4 = pow(K, 4.f);
    const float N = Y_b / Y_W;
    const float F_L = 0.2f * K4 * (5.f * L_A) + 0.1f * pow((1.f - K4), 2.f) * pow(5.f * L_A, 1.f/3.f);
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

    const float F_L_W = pow(F_L, 0.42f);
    const float A_w_J   = (400. * F_L_W) / (27.13 + F_L_W);

    p.XYZ_w = XYZ_w;
    p.F_L = F_L;
    p.z = z;
    p.D_RGB = D_RGB;
    p.A_w = A_w;
    p.A_w_J = A_w_J;

    p.MATRIX_16 = MATRIX_16;
    p.MATRIX_16_INV = MATRIX_16_INV;

    p.MATRIX_RGB_to_CAM16 = mult_f33_f33(RGBtoRGB_f33(P, CAM16::primaries), scale_f33(Identity_M33, f3_from_f(100.f)));
    p.MATRIX_CAM16_to_RGB = invert_f33(p.MATRIX_RGB_to_CAM16);

    return p;
}

Table3D make_gamut_table(const Primaries &P, float peakLuminance)
{
    const m33f RGB_TO_XYZ = RGBtoXYZ_f33(P);
    const JMhParams params = init_JMhParams(P);

    Table3D gamutCuspTableUnsorted{};
    for (int i = 0; i < gamutCuspTableUnsorted.size; i++)
    {
        const float hNorm = (float) i / gamutCuspTableUnsorted.size;
        const f3 HSV = {hNorm, 1., 1.};
        const f3 RGB = HSV_to_RGB(HSV);
        const f3 JMh = RGB_to_JMh(RGB, RGB_TO_XYZ, peakLuminance, params);

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
    gamutCuspTable.table[0][2] = gamutCuspTable.table[0][2] - 360.0;
    gamutCuspTable.table[gamutCuspTable.size+1][2] = gamutCuspTable.table[gamutCuspTable.size+1][2] + 360.0;

    return gamutCuspTable;
}

bool any_below_zero(const f3 &rgb)
{
    return (rgb[0] < 0. || rgb[1] < 0. || rgb[2] < 0.);
}

Table1D make_reach_m_table(const Primaries &P, float peakLuminance)
{
    const m33f XYZ_TO_RGB = XYZtoRGB_f33(P);
    const JMhParams params = init_JMhParams(P);
    const float limit_J_max = Y_to_Hellwig_J(peakLuminance, params);

    Table1D gamutReachTable{};

    for (int i = 0; i < gamutReachTable.size; i++) {
        const float hue = i;

        const float search_range = 50.f;
        float low = 0.;
        float high = low + search_range;
        bool outside = false;

        while ((outside != true) & (high < 1300.f))
        {
            const f3 searchJMh = {limit_J_max, high, hue};
            const f3 newLimitRGB = JMh_to_RGB(searchJMh, XYZ_TO_RGB, peakLuminance, params);
            outside = any_below_zero(newLimitRGB);
            if (outside == false)
            {
                low = high;
                high = high + search_range;
            }
        }

        while (high-low > 1e-4)
        {
            const float sampleM = (high + low) / 2.f;
            const f3 searchJMh = {limit_J_max, sampleM, hue};
            const f3 newLimitRGB = JMh_to_RGB(searchJMh, XYZ_TO_RGB, peakLuminance, params);
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
        return pow(log10(gain), 1. / focus_adjust_gain) + 1.f;
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

f3 find_gamut_boundary_intersection(const f3 &JMh_s, const f2 &JM_cusp_in, float J_focus, float J_max, float slope_gain, float gamma_top, float gamma_bottom)
{
    float slope = 0.f;

    const float s = std::max(0.000001f, smooth_cusps);
    const f2 JM_cusp = {
        JM_cusp_in[0],
        JM_cusp_in[1] * (1.f + smooth_m * s)
    };

    const float J_intersect_source = solve_J_intersect(JMh_s[0], JMh_s[1], J_focus, J_max, slope_gain);
    const float J_intersect_cusp = solve_J_intersect(JM_cusp[0], JM_cusp[1], J_focus, J_max, slope_gain);

    if (J_intersect_source < J_focus)
    {
        slope = J_intersect_source * (J_intersect_source - J_focus) / (J_focus * slope_gain);
    }
    else
    {
        slope = (J_max - J_intersect_source) * (J_intersect_source - J_focus) / (J_focus * slope_gain);
    }

    const float M_boundary_lower = J_intersect_cusp * pow(J_intersect_source / J_intersect_cusp, 1. / gamma_bottom) / (JM_cusp[0] / JM_cusp[1] - slope);
    const float M_boundary_upper = JM_cusp[1] * (J_max - J_intersect_cusp) * pow((J_max - J_intersect_source) / (J_max - J_intersect_cusp), 1. / gamma_top) / (slope * JM_cusp[1] + J_max - JM_cusp[0]);
    const float M_boundary = JM_cusp[1] * smin(M_boundary_lower / JM_cusp[1], M_boundary_upper / JM_cusp[1], s);
    const float J_boundary = J_intersect_source + slope * M_boundary;

    return {J_boundary, M_boundary, J_intersect_source};
}

bool evaluate_gamma_fit(
    const f2 &JMcusp,
    const f3 testJMh[3],
    float topGamma,
    float peakLuminance,
    float limit_J_max,
    float mid_J,
    float focus_dist,
    const m33f &limit_XYZ_to_RGB,
    const JMhParams &limitJMhParams)
{
    const float focusJ = lerpf(JMcusp[0], mid_J, std::min(1.f, cusp_mid_blend - (JMcusp[0] / limit_J_max)));

    for (size_t testIndex = 0; testIndex < 3; testIndex++)
    {
        const float slope_gain = limit_J_max * focus_dist * get_focus_gain(testJMh[testIndex][0], JMcusp[0], limit_J_max);
        const f3 approxLimit = find_gamut_boundary_intersection(testJMh[testIndex], JMcusp, focusJ, limit_J_max, slope_gain, topGamma, lower_hull_gamma);
        const f3 approximate_JMh = {approxLimit[0], approxLimit[1], testJMh[testIndex][2]};
        const f3 newLimitRGB = JMh_to_RGB(approximate_JMh, limit_XYZ_to_RGB, peakLuminance, limitJMhParams);

        if (!outside_hull(newLimitRGB))
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
    const m33f &limit_XYZ_to_RGB,
    const JMhParams &limitJMhParams)
{
    const int test_count = 3;
    const float testPositions[test_count] = {0.01f, 0.5f, 0.99f};

    Table1D gammaTable{};
    Table1D gamutTopGamma{};

    for (int i = 0; i < gammaTable.size; i++)
    {
        gammaTable.table[i] = -1.f;

        const float hue = i;
        const f2 JMcusp = cuspFromTable(hue, gamutCuspTable);

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
            const bool gammaFound = evaluate_gamma_fit(JMcusp, testJMh, high, peakLuminance, limit_J_max, mid_J, focus_dist, limit_XYZ_to_RGB, limitJMhParams);
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
            const bool gammaFound = evaluate_gamma_fit(JMcusp, testJMh, testGamma, peakLuminance, limit_J_max, mid_J, focus_dist, limit_XYZ_to_RGB, limitJMhParams);
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

    const float n_r = 100.0;    // normalized white in nits (what 1.0 should be)
    const float g = 1.15;       // surround / contrast
    const float c = 0.18;       // anchor for 18% grey
    const float c_d = 10.013;   // output luminance of 18% grey (in nits)
    const float w_g = 0.14;     // change in grey between different peak luminance
    const float t_1 = 0.04;     // shadow toe or flare/glare compensation
    const float r_hit_min = 128.;   // scene-referred value "hitting the roof"
    const float r_hit_max = 896.;   // scene-referred value "hitting the roof"

    // Calculate output constants
    const float r_hit = r_hit_min + (r_hit_max - r_hit_min) * (log(n/n_r)/log(10000.f/100.f));
    const float m_0 = (n / n_r);
    const float m_1 = 0.5f * (m_0 + sqrt(m_0 * (m_0 + 4.f * t_1)));
    const float u = pow((r_hit/m_1)/((r_hit/m_1)+1),g);
    const float m = m_1 / u;
    const float w_i = log(n/100.f)/log(2.f);
    const float c_t = c_d/n_r * (1. + w_i * w_g);
    const float g_ip = 0.5f * (c_t + sqrt(c_t * (c_t + 4.f * t_1)));
    const float g_ipp2 = -(m_1 * pow((g_ip/m),(1.f/g))) / (pow(g_ip/m , 1.f/g)-1.f);
    const float w_2 = c / g_ipp2;
    const float s_2 = w_2 * m_1;
    const float u_2 = pow((r_hit/m_1)/((r_hit/m_1) + w_2), g);
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

    float limit_J_max = Y_to_Hellwig_J(peakLuminance, inputJMhParams);

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
    params.chroma_compress_scale = pow(0.03379f * peakLuminance, 0.30596f) - 0.45135f;
    params.reach_m_table = make_reach_m_table(ACES_AP1::primaries, peakLuminance);
    return params;
}

GamutCompressParams init_GamutCompressParams(float peakLuminance, const Primaries &limitingPrimaries)
{
    const ToneScaleParams tsParams = init_ToneScaleParams(peakLuminance);
    const JMhParams inputJMhParams = init_JMhParams(ACES_AP0::primaries);

    float limit_J_max = Y_to_Hellwig_J(peakLuminance, inputJMhParams);
    float mid_J = Y_to_Hellwig_J(tsParams.c_t * 100.f, inputJMhParams);

    // Calculated chroma compress variables
    const float log_peak = log10( tsParams.n / tsParams.n_r);
    const float model_gamma = 1.f / (surround[1] * (1.48f + sqrt(Y_b / L_A)));
    const float focus_dist = focus_distance + focus_distance * focus_distance_scaling * log_peak;

    const m33f LIMIT_XYZ_TO_RGB = XYZtoRGB_f33(limitingPrimaries);
    const JMhParams limitJMhParams = init_JMhParams(limitingPrimaries);

    GamutCompressParams params{};

    params.limit_J_max = limit_J_max;
    params.mid_J = mid_J;
    params.model_gamma = model_gamma;
    params.focus_dist = focus_dist;
    params.reach_m_table = make_reach_m_table(ACES_AP1::primaries, peakLuminance);
    params.gamut_cusp_table = make_gamut_table(limitingPrimaries, peakLuminance);
    params.upper_hull_gamma_table = make_upper_hull_gamma(
        params.gamut_cusp_table,
        peakLuminance,
        limit_J_max,
        mid_J,
        focus_dist,
        LIMIT_XYZ_TO_RGB,
        limitJMhParams);

    return params;
}

} // namespace ACES2

} // OCIO namespace
