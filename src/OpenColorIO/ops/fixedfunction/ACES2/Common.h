// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_ACES2_COMMON_H
#define INCLUDED_OCIO_ACES2_COMMON_H

#include "MatrixLib.h"
#include "ColorLib.h"


namespace OCIO_NAMESPACE
{

namespace ACES2
{

constexpr int TABLE_SIZE = 360;
constexpr int TABLE_ADDITION_ENTRIES = 2;
constexpr int TABLE_TOTAL_SIZE = TABLE_SIZE + TABLE_ADDITION_ENTRIES;
constexpr int GAMUT_TABLE_BASE_INDEX = 1;

struct Table3D
{
    static constexpr int base_index = GAMUT_TABLE_BASE_INDEX;
    static constexpr int size = TABLE_SIZE;
    static constexpr int total_size = TABLE_TOTAL_SIZE;
    float table[TABLE_TOTAL_SIZE][3];
};

struct Table1D
{
    static constexpr int base_index = GAMUT_TABLE_BASE_INDEX;
    static constexpr int size = TABLE_SIZE;
    static constexpr int total_size = TABLE_TOTAL_SIZE;
    float table[TABLE_TOTAL_SIZE];
};

struct JMhParams
{
    float F_L;
    float z;
    float A_w;
    float A_w_J;
    f3 XYZ_w;
    f3 D_RGB;
    m33f MATRIX_RGB_to_CAM16;
    m33f MATRIX_CAM16_to_RGB;
};

struct ToneScaleParams
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

struct ChromaCompressParams
{
    float limit_J_max;
    float model_gamma;
    float sat;
    float sat_thr;
    float compr;
    Table1D reach_m_table;
    float chroma_compress_scale;
    static constexpr float cusp_mid_blend = 1.3f;
};

struct GamutCompressParams
{
    float limit_J_max;
    float mid_J;
    float model_gamma;
    float focus_dist;
    float lower_hull_gamma;
    Table1D reach_m_table;
    Table3D gamut_cusp_table;
    Table1D upper_hull_gamma_table;
};

// CAM
constexpr float reference_luminance = 100.f;
constexpr float L_A = 100.f;
constexpr float Y_b = 20.f;
constexpr float ac_resp = 1.f;
constexpr float ra = 2.f * ac_resp;
constexpr float ba = 0.05f + (2.f - ra);
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
constexpr float compression_threshold = 0.75f;

namespace CAM16
{
    static const Chromaticities red_xy(0.8336,  0.1735);
    static const Chromaticities grn_xy(2.3854, -1.4659);
    static const Chromaticities blu_xy(0.087 , -0.125 );
    static const Chromaticities wht_xy(0.333 ,  0.333 );

    const Primaries primaries(red_xy, grn_xy, blu_xy, wht_xy);
}

// Table generation
constexpr float gammaMinimum = 0.0f;
constexpr float gammaMaximum = 5.0f;
constexpr float gammaSearchStep = 0.4f;
constexpr float gammaAccuracy = 1e-5f;


} // namespace ACES2

} // OCIO namespace

#endif
