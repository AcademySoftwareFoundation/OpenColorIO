// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// NOTE:
// The file is a copy and paste from the corresponding GPU unitest i.e. []/tests/gpu/FixedFunctionOp_test.cpp.
// Keep the two files in sync. to increase the test coverage.


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "UnitTestMain.h"


OCIO_OSL_TEST(FixedFunction, style_aces_redmod03_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_inputValues =
        {
            {  0.90f,   0.05f,   0.22f,   0.50f },
            {  0.97f,   0.097f,  0.0097f, 1.00f },
            {  0.89f,   0.15f,   0.56f,   0.00f },
            { -1.00f,  -0.001f,  1.2f,    0.00f }
        };

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_aces_redmod03_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    m_data->m_inputValues =
        {
           {  0.90f,  0.05f,   0.22f,   0.50f },
           {  0.97f,  0.097f,  0.0097f, 1.00f },
           {  0.89f,  0.15f,   0.56f,   0.00f },
           { -1.0f,  -0.001f,  1.2f,    0.00f }
        };

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_aces_redmod10_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_inputValues =
        {
           {  0.90f,  0.05f,   0.22f,   0.50f },
           {  0.97f,  0.097f,  0.0097f, 1.00f },
           {  0.89f,  0.15f,   0.56f,   0.00f },
           { -1.0f,  -0.001f,  1.2f,    0.00f }
        };

    m_data->m_threshold = 1e-5f; // Slight difference with the GLSL unit test i.e. 1e-6f
}

OCIO_OSL_TEST(FixedFunction, style_aces_redmod10_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    m_data->m_inputValues =
        {
           {  0.90f,  0.05f,   0.22f,   0.50f },
           {  0.97f,  0.097f,  0.0097f, 1.00f },
           {  0.89f,  0.15f,   0.56f,   0.00f },
           { -1.0f,  -0.001f,  1.2f,    0.00f }
        };

    m_data->m_threshold = 1e-5f; // Slight difference with the GLSL unit test i.e. 1e-6f
}

OCIO_OSL_TEST(FixedFunction, style_aces_glow03_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_inputValues =
        {
            { 0.11f, 0.02f, 0.f,   0.5f }, // YC = 0.10
            { 0.01f, 0.02f, 0.03f, 1.0f }, // YC = 0.03
            { 0.11f, 0.91f, 0.01f, 0.0f }  // YC = 0.84
        };

    m_data->m_threshold = 1e-7f;
}

OCIO_OSL_TEST(FixedFunction, style_aces_glow03_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    m_data->m_inputValues =
        {
            { 0.11f, 0.02f, 0.f,   0.5f }, // YC = 0.10
            { 0.01f, 0.02f, 0.03f, 1.0f }, // YC = 0.03
            { 0.11f, 0.91f, 0.01f, 0.0f }  // YC = 0.84
        };

    m_data->m_threshold = 1e-7f;
}

OCIO_OSL_TEST(FixedFunction, style_aces_glow10_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_inputValues =
        {
            { 0.11f, 0.02f, 0.f,   0.5f }, // YC = 0.10
            { 0.01f, 0.02f, 0.03f, 1.0f }, // YC = 0.03
            { 0.11f, 0.91f, 0.01f, 0.0f }  // YC = 0.84
        };

    m_data->m_threshold = 1e-7f;
}

OCIO_OSL_TEST(FixedFunction, style_aces_glow10_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    m_data->m_inputValues =
        {
            { 0.11f, 0.02f, 0.f,   0.5f }, // YC = 0.10
            { 0.01f, 0.02f, 0.03f, 1.0f }, // YC = 0.03
            { 0.11f, 0.91f, 0.01f, 00.f }  // YC = 0.84
        };

    m_data->m_threshold = 1e-7f;
}

OCIO_OSL_TEST(FixedFunction, style_aces_gamutcomp13_fwd)
{
    static constexpr double data[7] = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMP_13, &data[0], 7);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    // See FixedFunctionOpCPU_tests.cpp for informations on the dataset
    m_data->m_inputValues =
    {
        // ALEXA Wide Gamut
        {  0.96663409472f,   0.04819045216f,   0.00719300006f,  0.0f },
        {  0.11554181576f,   1.18493819237f,  -0.06659350544f,  0.0f },
        { -0.08217582852f,  -0.23312863708f,   1.05940067768f,  0.0f },
        // BMD Wide Gamut
        {  0.92980366945f,   0.03025679290f,  -0.02240031771f,  0.0f },
        {  0.12437260151f,   1.19238424301f,  -0.08014731854f,  0.0f },
        { -0.05417707562f,  -0.22264070809f,   1.10254764557f,  0.0f },
        // Cinema Gamut
        {  1.10869872570f,  -0.05317572504f,  -0.00306261564f,  0.0f },
        {  0.00142395718f,   1.31239914894f,  -0.22332298756f,  0.0f },
        { -0.11012268066f,  -0.25922337174f,   1.22638559341f,  0.0f },
        // REDWideGamutRGB
        {  1.14983725548f,  -0.02548932098f,  -0.06720325351f,  0.0f },
        { -0.06796986610f,   1.30455482006f,  -0.31973674893f,  0.0f },
        { -0.08186896890f,  -0.27906489372f,   1.38694024086f,  0.0f },
        // S-Gamut3
        {  1.08979821205f,  -0.03117186762f,  -0.00326358480f,  0.0f },
        { -0.03276504576f,   1.18293666840f,  -0.00156985107f,  0.0f },
        { -0.05703317001f,  -0.15176482499f,   1.00483345985f,  0.0f },
        // Venice S-Gamut3
        {  1.15183949471f,  -0.04052511975f,  -0.01231821068f,  0.0f },
        { -0.11769985408f,   1.20661473274f,   0.00725125661f,  0.0f },
        { -0.03413961083f,  -0.16608965397f,   1.00506699085f,  0.0f },
        // V-Gamut
        {  1.04839742184f,  -0.02998665348f,  -0.00313943392f,  0.0f },
        {  0.01196120959f,   1.14840388298f,  -0.00963746291f,  0.0f },
        { -0.06036021933f,  -0.11841656268f,   1.01277709007f,  0.0f },
        // CC24 hue selective patch
        {  0.13911968470f,   0.08746965975f,   0.05927771702f,  0.0f },
        {  0.45410454273f,   0.32112336159f,   0.23821924627f,  0.0f },
        {  0.15262818336f,   0.19457373023f,   0.31270095706f,  0.0f },
        {  0.11231111735f,   0.14410330355f,   0.06487321854f,  0.0f },
        {  0.24113640189f,   0.22817260027f,   0.40912008286f,  0.0f },
        {  0.27200737596f,   0.47832396626f,   0.40502992272f,  0.0f },
        {  0.49412208796f,   0.23219805956f,   0.05947655812f,  0.0f },
        {  0.09734666348f,   0.10917002708f,   0.33662334085f,  0.0f },
        {  0.37841814756f,   0.12591768801f,   0.12897071242f,  0.0f },
        {  0.09104857594f,   0.05404697359f,   0.13533248007f,  0.0f },
        {  0.38014721870f,   0.47619381547f,   0.10615456849f,  0.0f },
        {  0.60210841894f,   0.38621774316f,   0.08225912601f,  0.0f },
        {  0.05051656812f,   0.05367648974f,   0.27239432931f,  0.0f },
        {  0.14276765287f,   0.28139206767f,   0.09023084491f,  0.0f },
        {  0.28782477975f,   0.06140174344f,   0.05256444961f,  0.0f },
        {  0.70791155100f,   0.58026152849f,   0.09300658852f,  0.0f },
        {  0.35456034541f,   0.12329842150f,   0.27530980110f,  0.0f },
        {  0.08374430984f,   0.22774916887f,   0.35839819908f,  0.0f }
    };

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_aces_gamutcomp13_inv)
{
    static constexpr double data[7] = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMP_13, &data[0], 7);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    // See FixedFunctionOpCPU_tests.cpp for informations on the dataset
    m_data->m_inputValues =
    {
        // ALEXA Wide Gamut
        {  0.96663409472f,   0.04819045216f,   0.00719300006f,  0.0f },
        {  0.11554181576f,   1.18493819237f,  -0.06659350544f,  0.0f },
        { -0.08217582852f,  -0.23312863708f,   1.05940067768f,  0.0f },
        // BMD Wide Gamut
        {  0.92980366945f,   0.03025679290f,  -0.02240031771f,  0.0f },
        {  0.12437260151f,   1.19238424301f,  -0.08014731854f,  0.0f },
        { -0.05417707562f,  -0.22264070809f,   1.10254764557f,  0.0f },
        // Cinema Gamut
        {  1.10869872570f,  -0.05317572504f,  -0.00306261564f,  0.0f },
        {  0.00142395718f,   1.31239914894f,  -0.22332298756f,  0.0f },
        { -0.11012268066f,  -0.25922337174f,   1.22638559341f,  0.0f },
        // REDWideGamutRGB
        {  1.14983725548f,  -0.02548932098f,  -0.06720325351f,  0.0f },
        { -0.06796986610f,   1.30455482006f,  -0.31973674893f,  0.0f },
        { -0.08186896890f,  -0.27906489372f,   1.38694024086f,  0.0f },
        // S-Gamut3
        {  1.08979821205f,  -0.03117186762f,  -0.00326358480f,  0.0f },
        { -0.03276504576f,   1.18293666840f,  -0.00156985107f,  0.0f },
        { -0.05703317001f,  -0.15176482499f,   1.00483345985f,  0.0f },
        // Venice S-Gamut3
        {  1.15183949471f,  -0.04052511975f,  -0.01231821068f,  0.0f },
        { -0.11769985408f,   1.20661473274f,   0.00725125661f,  0.0f },
        { -0.03413961083f,  -0.16608965397f,   1.00506699085f,  0.0f },
        // V-Gamut
        {  1.04839742184f,  -0.02998665348f,  -0.00313943392f,  0.0f },
        {  0.01196120959f,   1.14840388298f,  -0.00963746291f,  0.0f },
        { -0.06036021933f,  -0.11841656268f,   1.01277709007f,  0.0f },
        // CC24 hue selective patch
        {  0.13911968470f,   0.08746965975f,   0.05927771702f,  0.0f },
        {  0.45410454273f,   0.32112336159f,   0.23821924627f,  0.0f },
        {  0.15262818336f,   0.19457373023f,   0.31270095706f,  0.0f },
        {  0.11231111735f,   0.14410330355f,   0.06487321854f,  0.0f },
        {  0.24113640189f,   0.22817260027f,   0.40912008286f,  0.0f },
        {  0.27200737596f,   0.47832396626f,   0.40502992272f,  0.0f },
        {  0.49412208796f,   0.23219805956f,   0.05947655812f,  0.0f },
        {  0.09734666348f,   0.10917002708f,   0.33662334085f,  0.0f },
        {  0.37841814756f,   0.12591768801f,   0.12897071242f,  0.0f },
        {  0.09104857594f,   0.05404697359f,   0.13533248007f,  0.0f },
        {  0.38014721870f,   0.47619381547f,   0.10615456849f,  0.0f },
        {  0.60210841894f,   0.38621774316f,   0.08225912601f,  0.0f },
        {  0.05051656812f,   0.05367648974f,   0.27239432931f,  0.0f },
        {  0.14276765287f,   0.28139206767f,   0.09023084491f,  0.0f },
        {  0.28782477975f,   0.06140174344f,   0.05256444961f,  0.0f },
        {  0.70791155100f,   0.58026152849f,   0.09300658852f,  0.0f },
        {  0.35456034541f,   0.12329842150f,   0.27530980110f,  0.0f },
        {  0.08374430984f,   0.22774916887f,   0.35839819908f,  0.0f }
    };

    m_data->m_threshold = 5e-5f;  // Slight difference with the GLSL unit test i.e. 3e-6f
}

OCIO_OSL_TEST(FixedFunction, style_aces_darktodim10_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_aces_darktodim10_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_rec2100_surround_fwd)
{
    static constexpr double data[1] = { 0.7 };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_REC2100_SURROUND, &data[0], 1);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-5f; // Slight difference with the GLSL unit test i.e. 2e-6f
}

OCIO_OSL_TEST(FixedFunction, style_rec2100_surround_inv)
{
    static constexpr double data[1] = { 1. / 0.7 };
    // (Since we're not calling inverse() here, set the param to be the inverse of prev test.)
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_REC2100_SURROUND, &data[0], 1);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-5f; // Slight difference with the GLSL unit test i.e. 2e-6f
}

OCIO_OSL_TEST(FixedFunction, style_RGB_TO_HSV_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_RGB_TO_HSV_fwd_custom)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_inputValues =
        {
            {  1.500f,   2.500f,   0.500f,   0.50f },
            {  3.125f,  -0.625f,   1.250f,   1.00f },
            { -5.f/3.f, -4.f/3.f, -1.f/3.f,  0.25f },
            {  0.100f,  -0.800f,   0.400f,   0.25f }
        };

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_RGB_TO_HSV_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    m_data->m_threshold = 5e-4f;
    m_data->m_relativeComparison = true;
    m_data->m_expectedMinimalValue = 5e-3f;
}

OCIO_OSL_TEST(FixedFunction, style_RGB_TO_HSV_inv_custom)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

     m_data->m_inputValues =
        {
           {   3.f/12.f,  0.80f,  2.50f,  0.50f }, // val > 1
           {  11.f/12.f,  1.20f,  2.50f,  1.00f }, // sat > 1
           {  15.f/24.f,  0.80f, -2.00f,  0.25f }, // val < 0
           {  19.f/24.f,  1.50f, -0.40f,  0.25f }, // sat > 1, val < 0
           { -89.f/24.f,  0.50f,  0.40f,  2.00f }, // under-range hue
           {  81.f/24.f,  1.50f, -0.40f, -0.25f }, // over-range hue, sat > 1, val < 0
           {  81.f/24.f, -0.50f,  0.40f,  0.00f }, // sat < 0
           {    0.5000f,  2.50f,  0.04f,  0.00f }  // sat > 2
        };

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_XYZ_TO_xyY_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_xyY);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_XYZ_TO_xyY_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_xyY);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_XYZ_TO_uvY_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_uvY);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-6f;
}

OCIO_OSL_TEST(FixedFunction, style_XYZ_TO_uvY_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_uvY);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-5f;
}

OCIO_OSL_TEST(FixedFunction, style_XYZ_TO_LUV_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_LUV);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-5f;
}

OCIO_OSL_TEST(FixedFunction, style_XYZ_TO_LUV_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_LUV);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    m_data->m_transform = func;

    m_data->m_threshold = 1e-5f;
}
