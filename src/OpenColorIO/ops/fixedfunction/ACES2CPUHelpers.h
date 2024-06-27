// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_ACES2CPUHELPERS_H
#define INCLUDED_OCIO_ACES2CPUHELPERS_H

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>

#include <Imath/ImathMatrix.h>
#include <Imath/ImathMatrixAlgo.h>


namespace OCIO_NAMESPACE
{

namespace ACES2CPUHelpers
{

//////////////////////////////////////////////////////////////////////////
// Generic types
//////////////////////////////////////////////////////////////////////////

// Redefine standard array as our primary vector / matrix data type
// Imath V3d / M33f would have been a good candidate but don't quite
// work in GCC 9.x constexpr context from quick testing.

// Annoyingly, std::array indexing operator non-const overload is not
// constexpr until c++17.
using f2 = std::array<float, 2>;
using f3 = std::array<float, 3>;
using f4 = std::array<float, 4>;
using m33f = std::array<float, 9>;

struct Chromaticities
{
    f2 red;
    f2 green;
    f2 blue;
    f2 white;

    bool operator==(const Chromaticities &o)
    {
        return red == o.red && green == o.green && blue == o.blue && white == o.white;
    }
};


//////////////////////////////////////////////////////////////////////////
// Logging
//////////////////////////////////////////////////////////////////////////

void print_v(const std::string &name, float v)
{
    std::cerr << name << "\n";
    std::cerr << std::fixed << std::setprecision(9) << "\t" << v << "\n";
}

void print_v2(const std::string &name, const f2 &v)
{
    std::cerr << name << "\n";
    std::cerr << std::fixed << std::setprecision(9) << "\t" << v[0] << "\t" << v[1] << "\n";
}

void print_v3(const std::string &name, const f3 &v)
{
    std::cerr << name << "\n";
    std::cerr << std::fixed << std::setprecision(9) << "\t" << v[0] << "\t" << v[1] << "\t" << v[2] << "\n";
}

void print_m33(const std::string &name, const m33f &m)
{
    std::cerr << name << "\n";
    std::cerr << std::fixed << std::setprecision(9) << "\t" << m[0] << "\t" << m[1] << "\t" << m[2] << "\n";
    std::cerr << std::fixed << std::setprecision(9) << "\t" << m[3] << "\t" << m[4] << "\t" << m[5] << "\n";
    std::cerr << std::fixed << std::setprecision(9) << "\t" << m[6] << "\t" << m[7] << "\t" << m[8] << "\n";
}


//////////////////////////////////////////////////////////////////////////
// Utilities
//////////////////////////////////////////////////////////////////////////

// smooth minimum of a and b
constexpr float smin(float a, float b, float s)
{
    const float h = std::max(s - fabs(a - b), 0.f) / s;
    return std::min(a, b) - h * h * h * s * (1.f / 6.f);
}

constexpr float sign(float val)
{
    return (0.f < val) - (val < 0.f);
}

constexpr float spow(float base, float exponent)
{
    if (base < 0.f && exponent != floor(exponent))
    {
        return 0.f;
    } else {
        return pow(base, exponent);
    }
}

// fmod is not constexpr before C++23, roll our own in the meanwhile
constexpr float cfmod(float x, float y)
{
    return x - trunc(x / y) * y;
}

constexpr float wrap_to_360(float hue)
{
    float y = cfmod( hue, 360.f);
    if ( y < 0.f)
    {
        y = y + 360.f;
    }
    return y;
}

constexpr float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

constexpr float radians_to_degrees(float radians)
{
    return radians * 180.f / M_PI;
}

constexpr float degrees_to_radians(float degrees)
{
    return degrees / 180.f * M_PI;
}


//////////////////////////////////////////////////////////////////////////
// Vector / matrix utilities
//////////////////////////////////////////////////////////////////////////

constexpr f3 clamp_f3(const f3 &f3, float clampMin, float clampMax)
{
    return {
        std::max(std::min(f3[0], clampMax), clampMin),
        std::max(std::min(f3[1], clampMax), clampMin),
        std::max(std::min(f3[2], clampMax), clampMin)
    };
}

constexpr f3 f3_from_f(float v)
{
    return f3 {v, v, v};
}

constexpr f3 add_f_f3(float v, const f3 &f3)
{
    return {
        v + f3[0],
        v + f3[1],
        v + f3[2]
    };
}

constexpr f3 mult_f_f3(float v, const f3 &f3)
{
    return {
        v * f3[0],
        v * f3[1],
        v * f3[2]
    };
}

constexpr f3 mult_f3_f33(const f3 &f3, const m33f &mat33)
{
    return {
        f3[0] * mat33[0] + f3[1] * mat33[1] + f3[2] * mat33[2],
        f3[0] * mat33[3] + f3[1] * mat33[4] + f3[2] * mat33[5],
        f3[0] * mat33[6] + f3[1] * mat33[7] + f3[2] * mat33[8]
    };
}

constexpr m33f transpose_f33(const m33f &mat33)
{
    return {
        mat33[0], mat33[3], mat33[6],
        mat33[1], mat33[4], mat33[7],
        mat33[2], mat33[5], mat33[8]
    };
}

// Implementation from Imath::M33f::inverse()
constexpr m33f invert_f33(const m33f &mat33)
{
    float x[3][3] = {
        {mat33[0], mat33[1], mat33[2]},
        {mat33[3], mat33[4], mat33[5]},
        {mat33[6], mat33[7], mat33[8]},
    };

    if (x[0][2] != 0 || x[1][2] != 0 || x[2][2] != 1)
    {
        float s[3][3] = {
            x[1][1] * x[2][2] - x[2][1] * x[1][2],
            x[2][1] * x[0][2] - x[0][1] * x[2][2],
            x[0][1] * x[1][2] - x[1][1] * x[0][2],

            x[2][0] * x[1][2] - x[1][0] * x[2][2],
            x[0][0] * x[2][2] - x[2][0] * x[0][2],
            x[1][0] * x[0][2] - x[0][0] * x[1][2],

            x[1][0] * x[2][1] - x[2][0] * x[1][1],
            x[2][0] * x[0][1] - x[0][0] * x[2][1],
            x[0][0] * x[1][1] - x[1][0] * x[0][1]};

        float r = x[0][0] * s[0][0] + x[0][1] * s[1][0] + x[0][2] * s[2][0];

        if (abs (r) >= 1)
        {
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    s[i][j] /= r;
                }
            }
        }
        else
        {
            float mr = abs (r) / std::numeric_limits<float>::min();

            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    if (mr > abs (s[i][j]))
                    {
                        s[i][j] /= r;
                    }
                    else
                    {
                        throw std::invalid_argument ("Cannot invert singular matrix.");
                        return m33f();
                    }
                }
            }
        }

        return {
            s[0][0], s[0][1], s[0][2],
            s[1][0], s[1][1], s[1][2],
            s[2][0], s[2][1], s[2][2],
        };
    }
    else
    {
        float s[3][3] = {
            x[1][1],
            -x[0][1],
            0,

            -x[1][0],
            x[0][0],
            0,

            0,
            0,
            1};

        float r = x[0][0] * x[1][1] - x[1][0] * x[0][1];

        if (abs (r) >= 1)
        {
            for (int i = 0; i < 2; ++i)
            {
                for (int j = 0; j < 2; ++j)
                {
                    s[i][j] /= r;
                }
            }
        }
        else
        {
            float mr = abs (r) / std::numeric_limits<float>::min();

            for (int i = 0; i < 2; ++i)
            {
                for (int j = 0; j < 2; ++j)
                {
                    if (mr > abs (s[i][j]))
                    {
                        s[i][j] /= r;
                    }
                    else
                    {
                        throw std::invalid_argument ("Cannot invert singular matrix.");
                        return m33f();
                    }
                }
            }
        }

        s[2][0] = -x[2][0] * s[0][0] - x[2][1] * s[1][0];
        s[2][1] = -x[2][0] * s[0][1] - x[2][1] * s[1][1];

        return {
            s[0][0], s[0][1], s[0][2],
            s[1][0], s[1][1], s[1][2],
            s[2][0], s[2][1], s[2][2],
        };
    }
}


//////////////////////////////////////////////////////////////////////////
// Color math
//////////////////////////////////////////////////////////////////////////

constexpr f3 HSV_to_RGB(const f3 &HSV)
{
    const float C = HSV[2] * HSV[1];
    const float X = C * (1.f - fabs(cfmod(HSV[0] * 6.f, 2.f) - 1.f));
    const float m = HSV[2] - C;

    f3 RGB{};
    if (HSV[0] < 1.f/6.f) {
        RGB = {C, X, 0.f};
    } else if (HSV[0] < 2./6.) {
        RGB = {X, C, 0.f};
    } else if (HSV[0] < 3./6.) {
        RGB = {0.f, C, X};
    } else if (HSV[0] < 4./6.) {
        RGB = {0.f, X, C};
    } else if (HSV[0] < 5./6.) {
        RGB = {X, 0.f, C};
    } else {
        RGB = {C, 0.f, X};
    }
    RGB = add_f_f3(m, RGB);
    return RGB;
}

constexpr m33f RGBtoXYZ_f33(const Chromaticities &C, float Y=1.f)
{
    // X and Z values of RGB value (1, 1, 1), or "white"
    float X = C.white[0] * Y / C.white[1];
    float Z = (1. - C.white[0] - C.white[1]) * Y / C.white[1];

    // Scale factors for matrix rows
    float d = C.red[0] * (C.blue[1]  - C.green[1]) +
              C.blue[0] * (C.green[1] - C.red[1]) +
              C.green[0] * (C.red[1]   - C.blue[1]);

    float Sr = (          X * (C.blue[1] - C.green[1]) -
                 C.green[0] * (Y * (C.blue[1] - 1.f) + C.blue[1] * (X + Z)) +
                  C.blue[0] * (Y * (C.green[1] - 1.f) + C.green[1] * (X + Z))
               ) / d;

    float Sg = (          X * (C.red[1] - C.blue[1]) +
                   C.red[0] * (Y * (C.blue[1] - 1.f) + C.blue[1] * (X + Z)) -
                  C.blue[0] * (Y * (C.red[1] - 1.f) + C.red[1] * (X + Z))
               ) / d;

    float Sb = (          X * (C.green[1] - C.red[1]) -
                   C.red[0] * (Y * (C.green[1] - 1.f) + C.green[1] * (X + Z)) +
                 C.green[0] * (Y * (C.red[1] - 1.f) + C.red[1] * (X + Z))
               ) / d;

    // Assemble the matrix
    m33f M = {
        Sr * C.red[0],
        Sr * C.red[1],
        Sr * (1.f - C.red[0] - C.red[1]),

        Sg * C.green[0],
        Sg * C.green[1],
        Sg * (1.f - C.green[0] - C.green[1]),

        Sb * C.blue[0],
        Sb * C.blue[1],
        Sb * (1.f - C.blue[0] - C.blue[1])
    };

    return transpose_f33(M);
}

constexpr m33f XYZtoRGB_f33(const Chromaticities &C, float Y=1.f)
{
    return invert_f33(RGBtoXYZ_f33(C, Y));
}

constexpr m33f Identity_M33 = {
    1.f, 0.f, 0.f,
    0.f, 1.f, 0.f,
    0.f, 0.f, 1.f
};

constexpr Chromaticities AP0_PRI =
{
    { 0.73470,  0.26530},
    { 0.00000,  1.00000},
    { 0.00010, -0.07700},
    { 0.32168,  0.33767}
};

constexpr Chromaticities AP1_PRI =
{
    { 0.713,    0.293},
    { 0.165,    0.830},
    { 0.128,    0.044},
    { 0.32168,  0.33767}
};

constexpr m33f AP1_TO_XYZ = RGBtoXYZ_f33(AP1_PRI);
constexpr m33f XYZ_TO_AP1 = XYZtoRGB_f33(AP1_PRI);

constexpr Chromaticities REC709_PRI =
{
    { 0.6400,  0.3300},
    { 0.3000,  0.6000},
    { 0.1500,  0.0600},
    { 0.3127,  0.3290}
};

constexpr Chromaticities P3D65_PRI =
{
    { 0.6800,  0.3200},
    { 0.2650,  0.6900},
    { 0.1500,  0.0600},
    { 0.3127,  0.3290}
};

constexpr Chromaticities REC2020_PRI =
{
    { 0.7080,  0.2920},
    { 0.1700,  0.7970},
    { 0.1310,  0.0460},
    { 0.3127,  0.3290}
};

constexpr Chromaticities CAM16_PRI
{
    { 0.8336,  0.1735},
    { 2.3854, -1.4659},
    { 0.087 , -0.125 },
    { 0.333 ,  0.333 }
};

constexpr m33f MATRIX_16 = XYZtoRGB_f33(CAM16_PRI);
constexpr m33f MATRIX_16_INV = invert_f33(MATRIX_16);


//////////////////////////////////////////////////////////////////////////
// CAM utilities
//////////////////////////////////////////////////////////////////////////

constexpr m33f generate_panlrcm(float ra, float ba)
{
    m33f panlrcm_data = {
        ra,   1.f      ,  1.f/9.f,
        1.f, -12.f/11.f,  1.f/9.f,
        ba,   1.f/11.f , -2.f/9.f
    };
    const m33f panlrcm = invert_f33(panlrcm_data);

    // Normalize columns so that first row is 460
    const float s1 = 460.f / panlrcm[0];
    const float s2 = 460.f / panlrcm[1];
    const float s3 = 460.f / panlrcm[2];

    const m33f scaled_panlrcm = {
        panlrcm[0] * s1, panlrcm[1] * s2, panlrcm[2] * s3,
        panlrcm[3] * s1, panlrcm[4] * s2, panlrcm[5] * s3,
        panlrcm[6] * s1, panlrcm[7] * s2, panlrcm[8] * s3
    };

    return transpose_f33(scaled_panlrcm);
}


//////////////////////////////////////////////////////////////////////////
// Gamut lookup table utilities
//////////////////////////////////////////////////////////////////////////

constexpr int GAMUT_TABLE_SIZE = 360;

struct gamutTable
{
    static constexpr int size = GAMUT_TABLE_SIZE;
    float table[size][3];
};

struct gammaTable
{
    static constexpr int size = GAMUT_TABLE_SIZE;
    float table[size];
};

constexpr int midpoint(int low, int high)
{
    return (low + high) / 2;
}

constexpr float base_hue_for_position(int i_lo, int table_size)
{
    float result = i_lo * 360.f / table_size;
    return result;
}

constexpr int next_position_in_table(int entry, int table_size)
{
    int result = (entry + 1) % table_size;
    return result;
}

constexpr int hue_position_in_uniform_table(float hue, int table_size)
{
    const float wrapped_hue = wrap_to_360(hue);
    int result = (wrapped_hue / 360.f * table_size);
    return result;
}

constexpr f2 cuspFromTable(float h, const gamutTable &gt)
{
    float lo[3]{};
    float hi[3]{};

    if (h <= gt.table[0][2])
    {
        lo[0] = gt.table[gt.size-1][0];
        lo[1] = gt.table[gt.size-1][1];
        lo[2] = gt.table[gt.size-1][2] - 360.f;

        hi[0] = gt.table[0][0];
        hi[1] = gt.table[0][1];
        hi[2] = gt.table[0][2];
    }
    else if (h >= gt.table[gt.size-1][2])
    {
        lo[0] = gt.table[gt.size-1][0];
        lo[1] = gt.table[gt.size-1][1];
        lo[2] = gt.table[gt.size-1][2];

        hi[0] = gt.table[0][0];
        hi[1] = gt.table[0][1];
        hi[2] = gt.table[0][2] + 360.f;
    }
    else
    {
        int low_i = 0;
        int high_i = gt.size; // allowed as we have an extra entry in the table
        int i = hue_position_in_uniform_table(h, gt.size);

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
            i = midpoint(low_i, high_i);
        }

        lo[0] = gt.table[high_i-1][0];
        lo[1] = gt.table[high_i-1][1];
        lo[2] = gt.table[high_i-1][2];

        hi[0] = gt.table[high_i][0];
        hi[1] = gt.table[high_i][1];
        hi[2] = gt.table[high_i][2];
    }

    float t = (h - lo[2]) / (hi[2] - lo[2]);
    float cuspJ = lerp(lo[0], hi[0], t);
    float cuspM = lerp(lo[1], hi[1], t);

    return {cuspJ, cuspM};
}

constexpr float reachFromTable(float h, const gamutTable &gt)
{
    int i_lo = hue_position_in_uniform_table( h, gt.size);
    int i_hi = next_position_in_table( i_lo, gt.size);

    const f3 lo {
        gt.table[i_lo][0],
        gt.table[i_lo][1],
        gt.table[i_lo][2]
    };
    const f3 hi {
        gt.table[i_hi][0],
        gt.table[i_hi][1],
        gt.table[i_hi][2]
    };

    const float t = (h - lo[2]) / (hi[2] - lo[2]);

    return lerp(lo[1], hi[1], t);
}


//////////////////////////////////////////////////////////////////////////
// DRT types and constants
//////////////////////////////////////////////////////////////////////////

struct JMhParams
{
    float F_L;
    float z;
    f3 D_RGB;
    float A_w;
    float A_w_J;
};

struct TSParams
{
    float n;
    float n_r;
    float g;
    float t_1;
    float c_t;
    float s_2;
    float u_2;
    float m_2;
};

struct ODTParams
{
    bool clamp_ap1;

    float peakLuminance;

    // Tonescale
    float n_r;
    float g;
    float t_1;
    float c_t;
    float s_2;
    float u_2;
    float m_2;

    // Chroma Compression
    float limit_J_max;
    float mid_J;
    float model_gamma;
    float sat;
    float sat_thr;
    float compr;
    float focus_dist;
    gamutTable reach_gamut_table;
    gamutTable reach_cusp_table;

    // Gamut Compression
    gamutTable gamut_cusp_table;
    gammaTable upperHullGammaTable;

    // Input gamut
    m33f INPUT_RGB_TO_XYZ;
    m33f INPUT_XYZ_TO_RGB;
    JMhParams inputJMhParams;

    // Limiting gamut
    m33f LIMIT_RGB_TO_XYZ;
    m33f LIMIT_XYZ_TO_RGB;
    JMhParams limitJMhParams;
};

// CAM
constexpr float reference_luminance = 100.f;
constexpr float L_A = 100.f;
constexpr float Y_b = 20.f;
constexpr float ac_resp = 1.f;
constexpr float ra = 2.f * ac_resp;
constexpr float ba = 0.05f + (2.f - ra);
constexpr m33f panlrcm = generate_panlrcm(ra, ba);
constexpr f3 surround = {0.9f, 0.59f, 0.9f}; // Dim surround

// Chroma compression
constexpr float chroma_compress = 2.4f;
constexpr float chroma_compress_fact = 3.3f;
constexpr float chroma_expand = 1.3f;
constexpr float chroma_expand_fact = 0.69f;
constexpr float chroma_expand_thr = 0.5f;

// Gamut compression
constexpr float smooth_cusps = 0.12f;
constexpr float smooth_m = 0.27f;
constexpr float cusp_mid_blend = 1.3f;
constexpr float focus_gain_blend = 0.3f;
constexpr float focus_adjust_gain = 0.55f;
constexpr float focus_distance = 1.35f;
constexpr float focus_distance_scaling = 1.75f;
constexpr float lower_hull_gamma = 1.14f;
constexpr f4 compr_func_params = {0.75f, 1.1f, 1.3f, 1.f};

//////////////////////////////////////////////////////////////////////////
// RGB to and from JMh
//////////////////////////////////////////////////////////////////////////

// Post adaptation non linear response compression
constexpr float panlrc_forward(float v, float F_L)
{
    float F_L_v = spow(F_L * fabs(v) / 100.f, 0.42f);
    float c = (400.f * std::copysign(1., v) * F_L_v) / (27.13f + F_L_v);
    return c;
}

constexpr float panlrc_inverse(float v, float F_L)
{
    float p = sign(v) * 100.f / F_L * spow((27.13f * fabs(v) / (400.f - fabs(v))), 1.f / 0.42f);
    return p;
}

constexpr JMhParams initJMhParams(const Chromaticities &C)
{
    JMhParams p{};

    const m33f RGB_to_XYZ = RGBtoXYZ_f33(C);
    // const m33f XYZ_to_RGB = XYZtoRGB_f33(C);
    const f3 XYZ_w = mult_f3_f33(f3_from_f(reference_luminance), RGB_to_XYZ);

    const float Y_W = XYZ_w[1];

    const f3 RGB_w = mult_f3_f33(XYZ_w, MATRIX_16);

    // Viewing condition dependent parameters
    const float K = 1.f / (5.f * L_A + 1.f);
    const float K4 = pow(K, 4.f);
    const float N = Y_b / Y_W;
    const float F_L = 0.2f * K4 * (5.f * L_A) + 0.1f * pow((1.f - K4), 2.f) * spow(5.f * L_A, 1.f/3.f);
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

    p.F_L = F_L;
    p.z = z;
    p.D_RGB = D_RGB;
    p.A_w = A_w;
    p.A_w_J = A_w_J;

    return p;
}

constexpr JMhParams inputJMhParams = initJMhParams(AP0_PRI);

constexpr float Hellwig_J_to_Y(float J, const JMhParams &params)
{
    const float A = params.A_w_J * pow(fabs(J) / 100.f, 1.f / (surround[1] * params.z));
    return sign(J) * 100.f / params.F_L * pow((27.13f * A) / (400.f - A), 1.f / 0.42f);
}

constexpr float Y_to_Hellwig_J(float Y, const JMhParams &params)
{
    float F_L_Y = pow(params.F_L * fabs(Y) / 100., 0.42);
    return sign(Y) * 100.f * pow(((400.f * F_L_Y) / (27.13f + F_L_Y)) / params.A_w_J, surround[1] * params.z);
}

constexpr f3 XYZ_to_Hellwig2022_JMh(const f3 &XYZ, const JMhParams &params)
{
    // Step 1 - Converting CIE XYZ tristimulus values to sharpened RGB values
    const f3 RGB = mult_f3_f33(XYZ, MATRIX_16);

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

    // Step 4 - Converting to preliminary cartesian  coordinates
    const float a = RGB_a[0] - 12.f * RGB_a[1] / 11.f + RGB_a[2] / 11.f;
    const float b = (RGB_a[0] + RGB_a[1] - 2.f * RGB_a[2]) / 9.f;

    // Computing the hue angle math h
    const float hr = atan2(b, a);
    const float h = wrap_to_360(radians_to_degrees(hr));

    // Step 6 - Computing achromatic responses for the stimulus
    const float A = ra * RGB_a[0] + RGB_a[1] + ba * RGB_a[2];

    // Step 7 - Computing the correlate of lightness, J
    const float J = 100.f * pow( A / params.A_w, surround[1] * params.z);

    // Step 9 - Computing the correlate of colourfulness, M
    const float M = J == 0.f ? 0.f : 43.f * surround[2] * sqrt(a * a + b * b);

    return {J, M, h};
}

constexpr f3 Hellwig2022_JMh_to_XYZ(const f3 &JMh, const JMhParams &params)
{
    const float J = JMh[0];
    const float M = JMh[1];
    const float h = JMh[2];

    const float hr = degrees_to_radians(h);

    // Computing achromatic respons A for the stimulus
    const float A = params.A_w * spow(J / 100.f, 1. / (surround[1] * params.z));

    // Computing P_p_1 to P_p_2
    const float P_p_1 = 43.f * surround[2];
    const float P_p_2 = A;

    // Step 3 - Computing opponent colour dimensions a and b
    const float gamma = M / P_p_1;
    const float a = gamma * cos(hr);
    const float b = gamma * sin(hr);

    // Step 4 - Applying post-adaptation non-linear response compression matrix
    const f3 vec = {P_p_2, a, b};
    const f3 RGB_a = mult_f_f3( 1.0/1403.0, mult_f3_f33(vec, panlrcm));

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
    f3 XYZ = mult_f3_f33(RGB, MATRIX_16_INV);

    return XYZ;
}

constexpr f3 RGB_to_JMh(const f3 &RGB, const m33f &RGB_TO_XYZ, float luminance, const JMhParams &params)
{
    const f3 luminanceRGB = mult_f_f3(luminance, RGB);
    const f3 XYZ = mult_f3_f33(luminanceRGB, RGB_TO_XYZ);
    const f3 JMh = XYZ_to_Hellwig2022_JMh(XYZ, params);
    return JMh;
}

constexpr f3 JMh_to_RGB(const f3 &JMh, const m33f &XYZ_TO_RGB, float luminance, const JMhParams &params)
{
    const f3 luminanceXYZ = Hellwig2022_JMh_to_XYZ(JMh, params);
    const f3 luminanceRGB = mult_f3_f33(luminanceXYZ, XYZ_TO_RGB);
    const f3 RGB = mult_f_f3(1.f / luminance, luminanceRGB);
    return RGB;
}


//////////////////////////////////////////////////////////////////////////
// Tonescale and Chroma compression
//////////////////////////////////////////////////////////////////////////

constexpr float tonescale_fwd(float x, const ODTParams &params)
{
    float f = params.m_2 * pow(std::max(0.f, x) / (x + params.s_2), params.g);
    float h = std::max(0.f, f * f / (f + params.t_1));

    return h * params.n_r;
}

constexpr float tonescale_inv(float Y, const ODTParams &params)
{
    float Z = std::max(0.f, std::min(params.peakLuminance / (params.u_2 * params.n_r), Y));
    float h = (Z + sqrt(Z * (4.f * params.t_1 + Z))) / 2.f;
    float f = params.s_2 / (pow((params.m_2 / h), (1.f / params.g)) - 1.f);

    return f;
}

constexpr float toe( float x, float limit, float k1_in, float k2_in, bool invert = false)
{
    if (x > limit)
    {
        return x;
    }

    const float k2 = std::max(k2_in, 0.001f);
    const float k1 = sqrt(k1_in * k1_in + k2 * k2);
    const float k3 = (limit + k1) / (limit + k2);

    if (invert)
    {
        return (x * x + k1 * x) / (k3 * (x + k2));
    }
    else
    {
        return 0.5f * (k3 * x - k1 + sqrt((k3 * x - k1) * (k3 * x - k1) + 4.f * k2 * k3 * x));
    }
}

constexpr float chromaCompression_fwd(const f3 &JMh, float origJ, const ODTParams &params)
{
    const float J = JMh[0];
    float M = JMh[1];
    const float h = JMh[2];

    if (M == 0.0) {
        return M;
    }

    const float nJ = J / params.limit_J_max;
    const float snJ = std::max(0.f, 1.f - nJ);
    const float Mnorm = cuspFromTable(h, params.reach_gamut_table)[1];
    const float limit = pow(nJ, params.model_gamma) * reachFromTable(h, params.reach_cusp_table) / Mnorm;

    M = M * pow(J / origJ, params.model_gamma);
    M = M / Mnorm;
    M = limit - toe(limit - M, limit - 0.001f, snJ * params.sat, sqrt(nJ * nJ + params.sat_thr), false);
    M = toe(M, limit, nJ * params.compr, snJ, false);

    M = M * Mnorm;

    return M;
}

constexpr float chromaCompression_inv(const f3 &JMh, float origJ, const ODTParams &params)
{
    const float J = JMh[0];
    float M = JMh[1];
    const float h = JMh[2];

    if (M == 0.0) {
        return M;
    }

    const float nJ = J / params.limit_J_max;
    const float snJ = std::max(0.f, 1.f - nJ);
    const float Mnorm = cuspFromTable(h, params.reach_gamut_table)[1];
    const float limit = pow(nJ, params.model_gamma) * reachFromTable(h, params.reach_cusp_table) / Mnorm;

    M = M / Mnorm;
    M = toe(M, limit, nJ * params.compr, snJ, true);
    M = limit - toe(limit - M, limit - 0.001f, snJ * params.sat, sqrt(nJ * nJ + params.sat_thr), true);
    M = M * Mnorm;
    M = M * pow( J / origJ, -params.model_gamma);

    return M;
}

f3 tonemapAndCompress_fwd(const f3 &inputJMh, const ODTParams &params)
{
    const float linear = Hellwig_J_to_Y(inputJMh[0], inputJMhParams) / reference_luminance;
    const float luminanceTS = tonescale_fwd(linear, params);
    const float tonemappedJ = Y_to_Hellwig_J(luminanceTS, inputJMhParams);
    const f3 tonemappedJMh = {tonemappedJ, inputJMh[1], inputJMh[2]};

    const f3 outputJMh = tonemappedJMh;
    const float M = chromaCompression_fwd(outputJMh, inputJMh[0], params);

    return {outputJMh[0], M, outputJMh[2]};
}

constexpr f3 tonemapAndCompress_inv(const f3 &JMh, const ODTParams &params)
{
    float luminance = Hellwig_J_to_Y(JMh[0], inputJMhParams);
    float linear = tonescale_inv(luminance / reference_luminance, params);

    const f3 tonemappedJMh = JMh;
    const float untonemappedJ = Y_to_Hellwig_J(linear * reference_luminance, inputJMhParams);
    const f3 untonemappedColorJMh = {untonemappedJ, tonemappedJMh[1], tonemappedJMh[2]};

    const float M = chromaCompression_inv(tonemappedJMh, untonemappedColorJMh[0], params);

    return {untonemappedColorJMh[0], M, untonemappedColorJMh[2]};
}


//////////////////////////////////////////////////////////////////////////
// Gamut Compression
//////////////////////////////////////////////////////////////////////////

constexpr float get_focus_gain(float J, float cuspJ, float limit_J_max)
{
    const float thr = lerp(cuspJ, limit_J_max, focus_gain_blend);

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

constexpr float solve_J_intersect(float J, float M, float focusJ, float maxJ, float slope_gain)
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

constexpr f3 find_gamut_boundary_intersection(const f3 &JMh_s, const f2 &JM_cusp_in, float J_focus, float J_max, float slope_gain, float gamma_top, float gamma_bottom)
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

f3 get_reach_boundary(float J, float M, float h, const ODTParams &params)
{
    float limit_J_max = params.limit_J_max;
    float mid_J = params.mid_J;
    float model_gamma = params.model_gamma;
    float focus_dist = params.focus_dist;

    const int i_lo = hue_position_in_uniform_table(h, params.reach_cusp_table.size);
    const int i_hi = next_position_in_table(i_lo, params.reach_cusp_table.size);

    const float lo[3] {
        params.reach_cusp_table.table[i_lo][0],
        params.reach_cusp_table.table[i_lo][1],
        params.reach_cusp_table.table[i_lo][2]
    };
    const float hi[3] {
        params.reach_cusp_table.table[i_hi][0],
        params.reach_cusp_table.table[i_hi][1],
        params.reach_cusp_table.table[i_hi][2]
    };

    const float t = (h - lo[2]) / (360.f / params.reach_gamut_table.size);

    const float reachMaxM = lerp(lo[1], hi[1], t);

    const f2 JMcusp = cuspFromTable(h, params.gamut_cusp_table);
    const float focusJ = lerp(JMcusp[0], mid_J, std::min(1.f, cusp_mid_blend - (JMcusp[0] / limit_J_max)));
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

    const float boundary = limit_J_max * pow(intersectJ / limit_J_max, model_gamma) * reachMaxM / (limit_J_max - slope * reachMaxM);
    return {J, boundary, h};
}

float hueDependentUpperHullGamma(float h, const gammaTable &gt)
{
    const int i_lo = hue_position_in_uniform_table(h, gt.size);
    const int i_hi = next_position_in_table(i_lo, gt.size);

    const float base_hue = base_hue_for_position(i_lo, gt.size);
    const float t = wrap_to_360(h) - base_hue;

    return lerp(gt.table[i_lo], gt.table[i_hi], t);
}

float compressPowerP(float v, float thr, float lim, float power, bool invert=false)
{
    const float s = (lim-thr) / pow(pow((1.f-thr) / (lim-thr), -power) - 1.f, 1.f/power);
    const float nd = (v - thr) / s;
    const float p = pow(nd, power);

    float vCompressed;

    if (invert)
    {
        if (v < thr || lim < 1.0001f || v > thr + s)
        {
            vCompressed = v;
        }
        else
        {
            vCompressed = thr + s * pow(-(pow((v - thr) / s, power) / (pow((v - thr) / s, power) - 1.f)), 1.f / power);
        }
    }
    else
    {
        if ( v < thr || lim < 1.0001f)
        {
            vCompressed = v;
        }
        else
        {
            vCompressed = thr + s * nd / ( pow(1.f + p, 1.f / power));
        }
    }

    return vCompressed;
}

f3 compressGamut(const f3 &JMh, const ODTParams& params, float Jx, bool invert=false)
{
    float limit_J_max = params.limit_J_max;
    float mid_J = params.mid_J;
    float focus_dist = params.focus_dist;

    const f2 project_from = {JMh[0], JMh[1]};
    const f2 JMcusp = cuspFromTable(JMh[2], params.gamut_cusp_table);

    if (JMh[1] < 0.0001f || JMh[0] > limit_J_max)
    {
        return {JMh[0], 0.f, JMh[2]};
    }

    const float focusJ = lerp(JMcusp[0], mid_J, std::min(1.f, cusp_mid_blend - (JMcusp[0] / limit_J_max)));
    const float slope_gain = limit_J_max * focus_dist * get_focus_gain(Jx, JMcusp[0], limit_J_max);

    const float gamma_top = hueDependentUpperHullGamma(JMh[2], params.upperHullGammaTable);
    const float gamma_bottom = lower_hull_gamma;

    const f3 boundaryReturn = find_gamut_boundary_intersection(JMh, JMcusp, focusJ, limit_J_max, slope_gain, gamma_top, gamma_bottom);
    const f2 JMboundary = {boundaryReturn[0], boundaryReturn[1]};
    const f2 project_to = {boundaryReturn[2], 0.f};

    const f3 reachBoundary = get_reach_boundary(JMboundary[0], JMboundary[1], JMh[2], params);

    const float difference = std::max(1.0001f, reachBoundary[1] / JMboundary[1]);
    const float threshold = std::max(compr_func_params[0], 1.f / difference);

    float v = project_from[1] / JMboundary[1];
    v = compressPowerP(v, threshold, difference, compr_func_params[3], invert);

    const f2 JMcompressed {
        project_to[0] + v * (JMboundary[0] - project_to[0]),
        project_to[1] + v * (JMboundary[1] - project_to[1])
    };

    return {JMcompressed[0], JMcompressed[1], JMh[2]};
}

f3 gamutMap_fwd(const f3 &JMh, const ODTParams &params)
{
    return compressGamut(JMh, params, JMh[0], false);
}

f3 gamutMap_inv(const f3 &JMh, const ODTParams &params)
{
    const f2 JMcusp = cuspFromTable( JMh[2], params.gamut_cusp_table);
    float Jx = JMh[0];

    // Analytic inverse below threshold
    if (Jx <= lerp(JMcusp[0], params.limit_J_max, focus_gain_blend))
    {
        return compressGamut(JMh, params, Jx, true);
    }

    // Approximation above threshold
    Jx = compressGamut(JMh, params, Jx, true)[0];
    return compressGamut(JMh, params, Jx, true);
}


//////////////////////////////////////////////////////////////////////////
// Full transform
//////////////////////////////////////////////////////////////////////////

f3 outputTransform_fwd(const f3 &RGB, const ODTParams &params)
{
    f3 XYZ = mult_f3_f33(RGB, params.INPUT_RGB_TO_XYZ);

    // Clamp to AP1
    if (params.clamp_ap1)
    {
        const f3 AP1 = mult_f3_f33(XYZ, XYZ_TO_AP1);
        const f3 AP1_clamped = clamp_f3(AP1, 0.f, std::numeric_limits<float>::max());
        XYZ = mult_f3_f33(AP1_clamped, AP1_TO_XYZ);
    }

    const f3 JMh = RGB_to_JMh(XYZ, Identity_M33, reference_luminance, params.inputJMhParams);
    const f3 tonemappedJMh = tonemapAndCompress_fwd(JMh, params);
    const f3 compressedJMh = gamutMap_fwd(tonemappedJMh, params);
    const f3 limitRGB = JMh_to_RGB(compressedJMh, params.LIMIT_XYZ_TO_RGB, reference_luminance, params.limitJMhParams);

    return limitRGB;
}

f3 outputTransform_inv(const f3 &RGB, const ODTParams &params)
{
    const f3 compressedJMh = RGB_to_JMh(RGB, params.LIMIT_RGB_TO_XYZ, params.peakLuminance, params.limitJMhParams);
    const f3 tonemappedJMh = gamutMap_inv(compressedJMh, params);
    const f3 JMh = tonemapAndCompress_inv(tonemappedJMh, params);
    const f3 inputRGB = JMh_to_RGB(JMh, params.INPUT_XYZ_TO_RGB, params.peakLuminance, params.inputJMhParams);

    return inputRGB;
}

//////////////////////////////////////////////////////////////////////////
// Gamut lookup tables creation
//////////////////////////////////////////////////////////////////////////

constexpr gamutTable make_gamut_table(const Chromaticities &C, float peakLuminance)
{
    // TODO: Don't compute here?
    const m33f RGB_TO_XYZ = RGBtoXYZ_f33(C);
    const JMhParams params = initJMhParams(C);

    gamutTable gamutCuspTableUnsorted{};
    for (int i = 0; i < GAMUT_TABLE_SIZE; i++)
    {
        const float hNorm = (float) i / GAMUT_TABLE_SIZE;
        const f3 HSV = {hNorm, 1., 1.};
        const f3 RGB = HSV_to_RGB(HSV);
        const f3 JMh = RGB_to_JMh(RGB, RGB_TO_XYZ, peakLuminance, params);

        gamutCuspTableUnsorted.table[i][0] = JMh[0];
        gamutCuspTableUnsorted.table[i][1] = JMh[1];
        gamutCuspTableUnsorted.table[i][2] = JMh[2];
    }

    int minhIndex = 0;
    for (int i = 0; i < GAMUT_TABLE_SIZE; i++)
    {
        if ( gamutCuspTableUnsorted.table[i][2] < gamutCuspTableUnsorted.table[minhIndex][2])
            minhIndex = i;
    }

    gamutTable gamutCuspTable{};
    for (int i = 0; i < GAMUT_TABLE_SIZE; i++)
    {
        gamutCuspTable.table[i][0] = gamutCuspTableUnsorted.table[(minhIndex+i) % GAMUT_TABLE_SIZE][0];
        gamutCuspTable.table[i][1] = gamutCuspTableUnsorted.table[(minhIndex+i) % GAMUT_TABLE_SIZE][1];
        gamutCuspTable.table[i][2] = gamutCuspTableUnsorted.table[(minhIndex+i) % GAMUT_TABLE_SIZE][2];
    }

    return gamutCuspTable;
}

constexpr bool any_below_zero(const f3 &rgb)
{
    return (rgb[0] < 0. || rgb[1] < 0. || rgb[2] < 0.);
}

constexpr gamutTable make_reach_cusp_table(const Chromaticities &C, float peakLuminance)
{
    // TODO: Don't compute here?
    const m33f XYZ_TO_RGB = XYZtoRGB_f33(C);
    const JMhParams params = initJMhParams(C);
    const float limit_J_max = Y_to_Hellwig_J(peakLuminance, params);

    gamutTable gamutReachTable{};

    for (int i = 0; i < GAMUT_TABLE_SIZE; i++) {
        const float hue = base_hue_for_position( i, GAMUT_TABLE_SIZE);

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

        gamutReachTable.table[i][0] = limit_J_max;
        gamutReachTable.table[i][1] = high;
        gamutReachTable.table[i][2] = hue;
    }

    return gamutReachTable;
}

constexpr bool outside_hull(const f3 &rgb)
{
    // limit value, once we cross this value, we are outside of the top gamut shell
    const float maxRGBtestVal = 1.0;
    return rgb[0] > maxRGBtestVal || rgb[1] > maxRGBtestVal || rgb[2] > maxRGBtestVal;
}

constexpr bool evaluate_gamma_fit(const f2 &JMcusp, const f3 testJMh[3], float topGamma, float peakLuminance, float limit_J_max, float mid_J, float focus_dist, const m33f &limit_XYZ_to_RGB, const JMhParams &limitJMhParams)
{
    const float focusJ = lerp(JMcusp[0], mid_J, std::min(1.f, cusp_mid_blend - (JMcusp[0] / limit_J_max)));

    for (size_t testIndex = 0; testIndex < 3; testIndex++)
    {
        float slope_gain = limit_J_max * focus_dist * get_focus_gain(testJMh[testIndex][0], JMcusp[0], limit_J_max);
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

constexpr gammaTable make_upper_hull_gamma(const gamutTable &gamutCuspTable, float peakLuminance, float limit_J_max, float mid_J, float focus_dist, const m33f &limit_XYZ_to_RGB, const JMhParams &limitJMhParams)
{
    const int test_count = 3;
    const float testPositions[test_count] = {0.01f, 0.5f, 0.99f};

    gammaTable gt{};

    for (int i = 0; i < gt.size; i++)
    {
        gt.table[i] = -1.f;

        const float hue = base_hue_for_position(i, gt.size);
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

        const float search_range = 0.4f;
        float low = 0.f;
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
        while ( (high-low) > 1e-5)
        {
            testGamma = (high + low) / 2.f;
            const bool gammaFound = evaluate_gamma_fit(JMcusp, testJMh, testGamma, peakLuminance, limit_J_max, mid_J, focus_dist, limit_XYZ_to_RGB, limitJMhParams);
            if (gammaFound)
            {
                high = testGamma;
                gt.table[i] = high;
            }
            else
            {
                low = testGamma;
            }
        }
    }

    return gt;
}

//////////////////////////////////////////////////////////////////////////
// DRT parameters initialization
//////////////////////////////////////////////////////////////////////////

// Tonescale pre-calculations
constexpr TSParams init_TSParams(float peakLuminance)
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

    TSParams TonescaleParams = {
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

constexpr ODTParams init_ODTParams(
    float peakLuminance,
    Chromaticities limitingPrimaries,
    // Chromaticities encodingPrimaries,
    bool clampAP1)
{
    const TSParams tsParams = init_TSParams(peakLuminance);

    float limit_J_max = Y_to_Hellwig_J(peakLuminance, inputJMhParams);
    float mid_J = Y_to_Hellwig_J(tsParams.c_t * 100.f, inputJMhParams);

    // Calculated chroma compress variables
    const float log_peak = log10( tsParams.n / tsParams.n_r);
    const float compr = chroma_compress + (chroma_compress * chroma_compress_fact) * log_peak;
    const float sat = std::max(0.2f, chroma_expand - (chroma_expand * chroma_expand_fact) * log_peak);
    const float sat_thr = chroma_expand_thr / tsParams.n;
    const float model_gamma = 1.f / (surround[1] * (1.48f + sqrt(Y_b / L_A)));
    const float focus_dist = focus_distance + focus_distance * focus_distance_scaling * log_peak;

    ODTParams params{};

    params.clamp_ap1 = clampAP1;

    params.peakLuminance = peakLuminance;

    params.n_r = tsParams.n_r;
    params.g = tsParams.g;
    params.t_1 = tsParams.t_1;
    params.c_t = tsParams.c_t;
    params.s_2 = tsParams.s_2;
    params.u_2 = tsParams.u_2;
    params.m_2 = tsParams.m_2;

    params.limit_J_max = limit_J_max;
    params.mid_J = mid_J;
    params.model_gamma = model_gamma;
    params.sat = sat;
    params.sat_thr = sat_thr;
    params.compr = compr;
    params.focus_dist = focus_dist;

    // Input
    params.INPUT_RGB_TO_XYZ = RGBtoXYZ_f33(AP0_PRI);
    params.INPUT_XYZ_TO_RGB = XYZtoRGB_f33(AP0_PRI);
    params.inputJMhParams = inputJMhParams;

    // Limit
    params.LIMIT_RGB_TO_XYZ = RGBtoXYZ_f33(limitingPrimaries);
    params.LIMIT_XYZ_TO_RGB = XYZtoRGB_f33(limitingPrimaries);
    params.limitJMhParams = initJMhParams(limitingPrimaries);

    params.reach_gamut_table = make_gamut_table(AP1_PRI, peakLuminance);
    params.reach_cusp_table = make_reach_cusp_table(AP1_PRI, peakLuminance);
    params.gamut_cusp_table = make_gamut_table(limitingPrimaries, peakLuminance);
    params.upperHullGammaTable = make_upper_hull_gamma(
        params.gamut_cusp_table,
        peakLuminance,
        limit_J_max,
        mid_J,
        focus_dist,
        params.LIMIT_XYZ_TO_RGB,
        params.limitJMhParams);

    return params;
}

// TODO: Find way to make sure we don't get copy of these variables when this header is included in differents translation units
// C++17 inline variable would fix this, any C++14 solutions?
constexpr ODTParams srgb_100nits_odt = init_ODTParams(100.0f, REC709_PRI, /*REC709_PRI,*/ true);
constexpr ODTParams rec2100_p3lim_1000nits_odt = init_ODTParams(1000.0f, P3D65_PRI, /*REC2020_PRI,*/ true);

// TODO: Use map to cache results for custom parameters
// TODO: Encoding primaries needed for creative white point handling?
constexpr ODTParams get_transform_params(
    float peakLuminance,
    Chromaticities limitingPrimaries,
    // Chromaticities encodingPrimaries,
    bool ap1Clamp)
{
    if (peakLuminance == 100.f && limitingPrimaries == REC709_PRI /*&& encodingPrimaries == REC709_PRI*/ && ap1Clamp == true)
    {
        return srgb_100nits_odt;
    }
    else if (peakLuminance == 1000.f && limitingPrimaries == P3D65_PRI /*&& encodingPrimaries == REC2020_PRI*/ && ap1Clamp == true)
    {
        return rec2100_p3lim_1000nits_odt;
    }
    else
    {
        return init_ODTParams(peakLuminance, limitingPrimaries, /*encodingPrimaries,*/ ap1Clamp);
    }
}


} // namespace ACES2CPUHelpers

} // namespace OCIO_NAMESPACE

#endif