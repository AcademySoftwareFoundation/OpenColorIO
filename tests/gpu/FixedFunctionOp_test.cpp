// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"
#include <cmath>

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_GPU_TEST(FixedFunction, style_aces_redmod03_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.90f,  0.05f,   0.22f,   0.50f,
            0.97f,  0.097f,  0.0097f, 1.0f,
            0.89f,  0.15f,   0.56f,   0.0f,
           -1.0f,  -0.001f,  1.2f,    0.0f
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_redmod03_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_03);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.90f,  0.05f,   0.22f,   0.50f,
            0.97f,  0.097f,  0.0097f, 1.0f,
            0.89f,  0.15f,   0.56f,   0.0f,
           -1.0f,  -0.001f,  1.2f,    0.0f
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_redmod10_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.90f,  0.05f,   0.22f,   0.50f,
            0.97f,  0.097f,  0.0097f, 1.0f,
            0.89f,  0.15f,   0.56f,   0.0f,
           -1.0f,  -0.001f,  1.2f,    0.0f
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_redmod10_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RED_MOD_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.90f,  0.05f,   0.22f,   0.50f,
            0.97f,  0.097f,  0.0097f, 1.0f,
            0.89f,  0.15f,   0.56f,   0.0f,
           -1.0f,  -0.001f,  1.2f,    0.0f
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_glow03_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.11f, 0.02f, 0.f,   0.5f, // YC = 0.10
            0.01f, 0.02f, 0.03f, 1.0f, // YC = 0.03
            0.11f, 0.91f, 0.01f, 0.f,  // YC = 0.84
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-7f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_glow03_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_03);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.11f, 0.02f, 0.f,   0.5f, // YC = 0.10
            0.01f, 0.02f, 0.03f, 1.0f, // YC = 0.03
            0.11f, 0.91f, 0.01f, 0.f,  // YC = 0.84
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-7f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_glow10_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.11f, 0.02f, 0.f,   0.5f, // YC = 0.10
            0.01f, 0.02f, 0.03f, 1.0f, // YC = 0.03
            0.11f, 0.91f, 0.01f, 0.f,  // YC = 0.84
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-7f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_glow10_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GLOW_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            0.11f, 0.02f, 0.f,   0.5f, // YC = 0.10
            0.01f, 0.02f, 0.03f, 1.0f, // YC = 0.03
            0.11f, 0.91f, 0.01f, 0.f,  // YC = 0.84
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-7f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_gamutcomp13_fwd)
{
    const double data[7] = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMP_13, &data[0], 7);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    // See FixedFunctionOpCPU_tests.cpp for informations on the dataset
    values.m_inputValues =
    {
        // ALEXA Wide Gamut
         0.96663409472f,   0.04819045216f,   0.00719300006f,  0.0f,
         0.11554181576f,   1.18493819237f,  -0.06659350544f,  0.0f,
        -0.08217582852f,  -0.23312863708f,   1.05940067768f,  0.0f,
        // BMD Wide Gamut
         0.92980366945f,   0.03025679290f,  -0.02240031771f,  0.0f,
         0.12437260151f,   1.19238424301f,  -0.08014731854f,  0.0f,
        -0.05417707562f,  -0.22264070809f,   1.10254764557f,  0.0f,
        // Cinema Gamut
         1.10869872570f,  -0.05317572504f,  -0.00306261564f,  0.0f,
         0.00142395718f,   1.31239914894f,  -0.22332298756f,  0.0f,
        -0.11012268066f,  -0.25922337174f,   1.22638559341f,  0.0f,
        // REDWideGamutRGB
         1.14983725548f,  -0.02548932098f,  -0.06720325351f,  0.0f,
        -0.06796986610f,   1.30455482006f,  -0.31973674893f,  0.0f,
        -0.08186896890f,  -0.27906489372f,   1.38694024086f,  0.0f,
        // S-Gamut3
         1.08979821205f,  -0.03117186762f,  -0.00326358480f,  0.0f,
        -0.03276504576f,   1.18293666840f,  -0.00156985107f,  0.0f,
        -0.05703317001f,  -0.15176482499f,   1.00483345985f,  0.0f,
        // Venice S-Gamut3
         1.15183949471f,  -0.04052511975f,  -0.01231821068f,  0.0f,
        -0.11769985408f,   1.20661473274f,   0.00725125661f,  0.0f,
        -0.03413961083f,  -0.16608965397f,   1.00506699085f,  0.0f,
        // V-Gamut
         1.04839742184f,  -0.02998665348f,  -0.00313943392f,  0.0f,
         0.01196120959f,   1.14840388298f,  -0.00963746291f,  0.0f,
        -0.06036021933f,  -0.11841656268f,   1.01277709007f,  0.0f,
        // CC24 hue selective patch
         0.13911968470f,   0.08746965975f,   0.05927771702f,  0.0f,
         0.45410454273f,   0.32112336159f,   0.23821924627f,  0.0f,
         0.15262818336f,   0.19457373023f,   0.31270095706f,  0.0f,
         0.11231111735f,   0.14410330355f,   0.06487321854f,  0.0f,
         0.24113640189f,   0.22817260027f,   0.40912008286f,  0.0f,
         0.27200737596f,   0.47832396626f,   0.40502992272f,  0.0f,
         0.49412208796f,   0.23219805956f,   0.05947655812f,  0.0f,
         0.09734666348f,   0.10917002708f,   0.33662334085f,  0.0f,
         0.37841814756f,   0.12591768801f,   0.12897071242f,  0.0f,
         0.09104857594f,   0.05404697359f,   0.13533248007f,  0.0f,
         0.38014721870f,   0.47619381547f,   0.10615456849f,  0.0f,
         0.60210841894f,   0.38621774316f,   0.08225912601f,  0.0f,
         0.05051656812f,   0.05367648974f,   0.27239432931f,  0.0f,
         0.14276765287f,   0.28139206767f,   0.09023084491f,  0.0f,
         0.28782477975f,   0.06140174344f,   0.05256444961f,  0.0f,
         0.70791155100f,   0.58026152849f,   0.09300658852f,  0.0f,
         0.35456034541f,   0.12329842150f,   0.27530980110f,  0.0f,
         0.08374430984f,   0.22774916887f,   0.35839819908f,  0.0f
    };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_gamutcomp13_inv)
{
    const double data[7] = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMP_13, &data[0], 7);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    // See FixedFunctionOpCPU_tests.cpp for informations on the dataset
    values.m_inputValues =
    {
        // ALEXA Wide Gamut
         0.96663409472f,   0.04819045216f,   0.00719300006f,  0.0f,
         0.11554181576f,   1.18493819237f,  -0.06659350544f,  0.0f,
        -0.08217582852f,  -0.23312863708f,   1.05940067768f,  0.0f,
        // BMD Wide Gamut
         0.92980366945f,   0.03025679290f,  -0.02240031771f,  0.0f,
         0.12437260151f,   1.19238424301f,  -0.08014731854f,  0.0f,
        -0.05417707562f,  -0.22264070809f,   1.10254764557f,  0.0f,
        // Cinema Gamut
         1.10869872570f,  -0.05317572504f,  -0.00306261564f,  0.0f,
         0.00142395718f,   1.31239914894f,  -0.22332298756f,  0.0f,
        -0.11012268066f,  -0.25922337174f,   1.22638559341f,  0.0f,
        // REDWideGamutRGB
         1.14983725548f,  -0.02548932098f,  -0.06720325351f,  0.0f,
        -0.06796986610f,   1.30455482006f,  -0.31973674893f,  0.0f,
        -0.08186896890f,  -0.27906489372f,   1.38694024086f,  0.0f,
        // S-Gamut3
         1.08979821205f,  -0.03117186762f,  -0.00326358480f,  0.0f,
        -0.03276504576f,   1.18293666840f,  -0.00156985107f,  0.0f,
        -0.05703317001f,  -0.15176482499f,   1.00483345985f,  0.0f,
        // Venice S-Gamut3
         1.15183949471f,  -0.04052511975f,  -0.01231821068f,  0.0f,
        -0.11769985408f,   1.20661473274f,   0.00725125661f,  0.0f,
        -0.03413961083f,  -0.16608965397f,   1.00506699085f,  0.0f,
        // V-Gamut
         1.04839742184f,  -0.02998665348f,  -0.00313943392f,  0.0f,
         0.01196120959f,   1.14840388298f,  -0.00963746291f,  0.0f,
        -0.06036021933f,  -0.11841656268f,   1.01277709007f,  0.0f,
        // CC24 hue selective patch
         0.13911968470f,   0.08746965975f,   0.05927771702f,  0.0f,
         0.45410454273f,   0.32112336159f,   0.23821924627f,  0.0f,
         0.15262818336f,   0.19457373023f,   0.31270095706f,  0.0f,
         0.11231111735f,   0.14410330355f,   0.06487321854f,  0.0f,
         0.24113640189f,   0.22817260027f,   0.40912008286f,  0.0f,
         0.27200737596f,   0.47832396626f,   0.40502992272f,  0.0f,
         0.49412208796f,   0.23219805956f,   0.05947655812f,  0.0f,
         0.09734666348f,   0.10917002708f,   0.33662334085f,  0.0f,
         0.37841814756f,   0.12591768801f,   0.12897071242f,  0.0f,
         0.09104857594f,   0.05404697359f,   0.13533248007f,  0.0f,
         0.38014721870f,   0.47619381547f,   0.10615456849f,  0.0f,
         0.60210841894f,   0.38621774316f,   0.08225912601f,  0.0f,
         0.05051656812f,   0.05367648974f,   0.27239432931f,  0.0f,
         0.14276765287f,   0.28139206767f,   0.09023084491f,  0.0f,
         0.28782477975f,   0.06140174344f,   0.05256444961f,  0.0f,
         0.70791155100f,   0.58026152849f,   0.09300658852f,  0.0f,
         0.35456034541f,   0.12329842150f,   0.27530980110f,  0.0f,
         0.08374430984f,   0.22774916887f,   0.35839819908f,  0.0f
    };
    test.setCustomValues(values);

    test.setErrorThreshold(3e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_output_transform_fwd)
{
     const double data[9] = {
        // Peak luminance
        1000.f,
        // P3D65 gamut
        0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3127, 0.3290
    };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20, &data[0], 9);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
    {
        // ACEScg primaries and secondaries scaled by 4
        2.781808965f, 0.179178253f, -0.022103530f, 1.0f,
        3.344523751f, 3.617862727f, -0.006002689f, 1.0f,
        0.562714786f, 3.438684474f, 0.016100841f, 1.0f,
        1.218191035f, 3.820821747f, 4.022103530f, 1.0f,
        0.655476249f, 0.382137273f, 4.006002689f, 1.0f,
        3.437285214f, 0.561315526f, 3.983899159f, 1.0f,
        // OCIO test values
        0.110000000f, 0.020000000f, 0.040000000f, 0.5f,
        0.710000000f, 0.510000000f, 0.810000000f, 1.0f,
        0.430000000f, 0.820000000f, 0.710000000f, 0.0f,
        // ColorChecker24 (SMPTE 2065-1 2021)
        0.118770000f, 0.087090000f, 0.058950000f, 1.0f,
        0.400020000f, 0.319160000f, 0.237360000f, 1.0f,
        0.184760000f, 0.203980000f, 0.313110000f, 1.0f,
        0.109010000f, 0.135110000f, 0.064930000f, 1.0f,
        0.266840000f, 0.246040000f, 0.409320000f, 1.0f,
        0.322830000f, 0.462080000f, 0.406060000f, 1.0f,
        0.386050000f, 0.227430000f, 0.057770000f, 1.0f,
        0.138220000f, 0.130370000f, 0.337030000f, 1.0f,
        0.302020000f, 0.137520000f, 0.127580000f, 1.0f,
        0.093100000f, 0.063470000f, 0.135250000f, 1.0f,
        0.348760000f, 0.436540000f, 0.106130000f, 1.0f,
        0.486550000f, 0.366850000f, 0.080610000f, 1.0f,
        0.087320000f, 0.074430000f, 0.272740000f, 1.0f,
        0.153660000f, 0.256920000f, 0.090710000f, 1.0f,
        0.217420000f, 0.070700000f, 0.051300000f, 1.0f,
        0.589190000f, 0.539430000f, 0.091570000f, 1.0f,
        0.309040000f, 0.148180000f, 0.274260000f, 1.0f,
        0.149010000f, 0.233780000f, 0.359390000f, 1.0f,
        0.866530000f, 0.867920000f, 0.858180000f, 1.0f,
        0.573560000f, 0.572560000f, 0.571690000f, 1.0f,
        0.353460000f, 0.353370000f, 0.353910000f, 1.0f,
        0.202530000f, 0.202430000f, 0.202870000f, 1.0f,
        0.094670000f, 0.095200000f, 0.096370000f, 1.0f,
        0.037450000f, 0.037660000f, 0.038950000f, 1.0f,
        // Spectrally non-selective 18 % reflecting diffuser
        0.180000000f, 0.180000000f, 0.180000000f, 1.0f,
        // Perfect reflecting diffuser
        0.977840000f, 0.977840000f, 0.977840000f, 1.0f,
    };
    test.setCustomValues(values);

    test.setErrorThreshold(2e-5f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_output_transform_inv)
{
     const double data[9] = {
        // Peak luminance
        1000.f,
        // P3D65 gamut
        0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3127, 0.3290
    };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20, &data[0], 9);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
    {
        // ACEScg primaries and secondaries scaled by 4
        4.965774059f, -0.032864563f, 0.041625995f, 1.0f,
        3.969441891f, 3.825784922f, -0.056133576f, 1.0f,
        -0.075329021f, 3.688980103f, 0.270296901f, 1.0f,
        -0.095423937f, 3.650517225f, 3.459972620f, 1.0f,
        -0.028930068f, 0.196428135f, 2.796343565f, 1.0f,
        4.900805950f, -0.064376131f, 3.838256121f, 1.0f,
        // OCIO test values
        0.096890204f, -0.001135312f, 0.018971510f, 0.5f,
        0.809614301f, 0.479856580f, 0.814239502f, 1.0f,
        0.107420206f, 0.920529068f, 0.726378500f, 0.0f,
        // ColorChecker24 (SMPTE 2065-1 2021)
        0.115475260f, 0.050812904f, 0.030212952f, 1.0f,
        0.484879673f, 0.301042974f, 0.226768956f, 1.0f,
        0.098463766f, 0.160814658f, 0.277010560f, 1.0f,
        0.071130365f, 0.107334383f, 0.035097566f, 1.0f,
        0.207111493f, 0.198474824f, 0.375326216f, 1.0f,
        0.195447892f, 0.481111974f, 0.393299013f, 1.0f,
        0.571913838f, 0.196872935f, 0.041634772f, 1.0f,
        0.045791931f, 0.069875360f, 0.291233480f, 1.0f,
        0.424848706f, 0.083199009f, 0.102153838f, 1.0f,
        0.059589427f, 0.022219172f, 0.091246888f, 1.0f,
        0.360365510f, 0.478741467f, 0.086726837f, 1.0f,
        0.695662081f, 0.371994525f, 0.068298168f, 1.0f,
        0.011806309f, 0.021665439f, 0.199594811f, 1.0f,
        0.076526314f, 0.256237417f, 0.060564656f, 1.0f,
        0.300064564f, 0.023416257f, 0.030360471f, 1.0f,
        0.805484772f, 0.596903503f, 0.082996152f, 1.0f,
        0.388385952f, 0.079899102f, 0.245819211f, 1.0f,
        0.010952532f, 0.196105912f, 0.307181358f, 1.0f,
        0.921019495f, 0.921707213f, 0.912856042f, 1.0f,
        0.590192318f, 0.588423848f, 0.587825358f, 1.0f,
        0.337743521f, 0.337685764f, 0.338155121f, 1.0f,
        0.169265985f, 0.169178501f, 0.169557109f, 1.0f,
        0.058346048f, 0.059387825f, 0.060296260f, 1.0f,
        0.012581184f, 0.012947139f, 0.013654195f, 1.0f,
        // Spectrally non-selective 18 % reflecting diffuser
        0.145115465f, 0.145115525f, 0.145115510f, 1.0f,
        // Perfect reflecting diffuser
        1.041565657f, 1.041566014f, 1.041565657f, 1.0f,
    };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-4f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_output_transform_invfwd)
{
    // Test that there are no shader resource (textures, functions, etc) conflicts between
    // the 2 different inverse and forward transforms.  This is not a round-trip test,
    // it tests that the forward and inverse may exist in the same shader with no issue.

    const double data_inv[9] = {
        // Peak luminance
        100.f,
        // REC709 gamut
        0.64, 0.33, 0.3, 0.6, 0.15, 0.06, 0.3127, 0.329
    };
    OCIO::FixedFunctionTransformRcPtr func_inv =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20, &data_inv[0], 9);
    func_inv->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

     const double data_fwd[9] = {
        // Peak luminance
        1000.f,
        // P3D65 gamut
        0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3127, 0.3290
    };

    OCIO::FixedFunctionTransformRcPtr func_fwd =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_OUTPUT_TRANSFORM_20, &data_fwd[0], 9);

    OCIO::GroupTransformRcPtr grp = OCIO::GroupTransform::Create();
    grp->appendTransform(func_inv);
    grp->appendTransform(func_fwd);

    test.setTestNaN(false); // TODO: Partially running the output transform without the clamp so do not test Nan or Inf values
    test.setTestInfinity(false);

    test.setProcessor(grp);

    // This is not expected to be an identity, but it should be the same between CPU & GPU.
    test.setErrorThreshold(7e-4f);
}

namespace
{

OCIO::GroupTransformRcPtr BuildDisplayViewTransform(const char * display_style, 
                                                    const char * view_style, 
                                                    bool doRoundTrip)
{
    // Built-in transform for the display.
    OCIO::BuiltinTransformRcPtr display_builtin = OCIO::BuiltinTransform::Create();
    display_builtin->setStyle(display_style);
    display_builtin->validate();
    auto display_builtin_inv = display_builtin->createEditableCopy();
    display_builtin_inv->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    // Built-in transform for the view.
    OCIO::BuiltinTransformRcPtr view_builtin = OCIO::BuiltinTransform::Create();
    view_builtin->setStyle(view_style);
    view_builtin->validate();
    auto view_builtin_inv = view_builtin->createEditableCopy();
    view_builtin_inv->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    // Assemble inverse and forward transform into a group transform that goes from
    // display code values to ACES2065-1 and (optionally) back to display code values.
    OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
    group->appendTransform(display_builtin_inv);
    group->appendTransform(view_builtin_inv);
    if (doRoundTrip)
    {
        group->appendTransform(view_builtin);
        group->appendTransform(display_builtin);
    }
    return group;
}

void GenerateIdentityLut3D(OCIOGPUTest::CustomValues & values, int edgeLen, float scale)
{
    const int numChannels = 4;
    int num_samples = edgeLen * edgeLen * edgeLen;
    std::vector<float> img(num_samples * numChannels, 0.f);

    float c = 1.0f / ((float)edgeLen - 1.0f);
    for (int i = 0; i < edgeLen*edgeLen*edgeLen; i++)
    {
        img[numChannels*i + 0] = scale * (float)(i%edgeLen) * c;
        img[numChannels*i + 1] = scale * (float)((i / edgeLen) % edgeLen) * c;
        img[numChannels*i + 2] = scale * (float)((i / edgeLen / edgeLen) % edgeLen) * c;
    }
    values.m_inputValues = img;
}

} // anon.

// NOTE: Some of the following tests compare the round-trip from display code value to ACES2065-1
// and back to display code value. The round-trip is not perfect (see BuiltinTransform_tests.cpp)
// but the tests here simply check if the CPU and GPU are giving the same result.

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_rec709_rndtrip)
{
    const char * display_style = "DISPLAY - CIE-XYZ-D65_to_REC.1886-REC.709";
    const char * view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-REC709_2.0";
    const bool do_roundtrip = true;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);

    // The test harness gets a processor from the transform with the default optimization
    // level. However, the forward/inverse does not optimize out due to the clamp to AP1
    // in-between the FixedFunctions.
    test.setProcessor(group);

    // Set up a grid of RGBA custom values.
    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    GenerateIdentityLut3D(values, lut_size, 1.0f);
    test.setCustomValues(values);

    test.setErrorThreshold(0.004f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_rec709_inv)
{
    const char * display_style = "DISPLAY - CIE-XYZ-D65_to_REC.1886-REC.709";
    const char * view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-REC709_2.0";
    const bool do_roundtrip = false;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    GenerateIdentityLut3D(values, lut_size, 1.0f);
    test.setCustomValues(values);

    test.setRelativeComparison(true);
    test.setExpectedMinimalValue(1.f);
    test.setErrorThreshold(0.001f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_displayp3_rndtrip)
{
    const char * display_style = "DISPLAY - CIE-XYZ-D65_to_DisplayP3";
    const char * view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-P3-D65_2.0";
    const bool do_roundtrip = true;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    const float lum_scale = 1.0f;
    GenerateIdentityLut3D(values, lut_size, lum_scale);
    test.setCustomValues(values);

    test.setErrorThreshold(0.001f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_displayp3_inv)
{
    const char * display_style = "DISPLAY - CIE-XYZ-D65_to_DisplayP3";
    const char * view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - SDR-100nit-P3-D65_2.0";
    const bool do_roundtrip = false;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    const float lum_scale = 1.0f;
    GenerateIdentityLut3D(values, lut_size, lum_scale);
    test.setCustomValues(values);

    test.setRelativeComparison(true);
    test.setExpectedMinimalValue(1.f);
    test.setErrorThreshold(0.001f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_1000nit_p3_rndtrip)
{
    const char * display_style = "DISPLAY - CIE-XYZ-D65_to_ST2084-P3-D65";
    const char * view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-1000nit-P3-D65_2.0";
    const bool do_roundtrip = true;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    const float lum_scale = 0.75183f;  // scale to 1000 nits
    GenerateIdentityLut3D(values, lut_size, lum_scale);
    test.setCustomValues(values);

    // TODO: Investigate why this is not closer.
    // Setting the CPUProcessor to OPTIMIZATION_NONE helps slightly, but is not the main
    // cause of the error.
    test.setErrorThreshold(0.012f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_1000nit_p3_inv)
{
    const char * display_style = "DISPLAY - CIE-XYZ-D65_to_ST2084-P3-D65";
    const char * view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-1000nit-P3-D65_2.0";
    const bool do_roundtrip = false;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    const float lum_scale = 0.75183f;  // scale to 1000 nits
    GenerateIdentityLut3D(values, lut_size, lum_scale);
    test.setCustomValues(values);

    test.setRelativeComparison(true);
    test.setExpectedMinimalValue(1.f);
    test.setErrorThreshold(0.001f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_4000nit_p3_rndtrip)
{
    const char * display_style = "DISPLAY - CIE-XYZ-D65_to_ST2084-P3-D65";
    const char * view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-P3-D65_2.0";
    const bool do_roundtrip = true;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    const float lum_scale = 0.90257f;  // scale to 4000 nits
    GenerateIdentityLut3D(values, lut_size, lum_scale);
    test.setCustomValues(values);

    // TODO: Investigate why this is not closer.
    test.setErrorThreshold(0.018f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_4000nit_p3_inv)
{
    const char * display_style = "DISPLAY - CIE-XYZ-D65_to_ST2084-P3-D65";
    const char * view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-P3-D65_2.0";
    const bool do_roundtrip = false;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    const float lum_scale = 0.90257f;  // scale to 4000 nits
    GenerateIdentityLut3D(values, lut_size, lum_scale);
    test.setCustomValues(values);

    test.setRelativeComparison(true);
    test.setExpectedMinimalValue(1.f);
    test.setErrorThreshold(0.001f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_4000nit_rec2020_rndtrip)
{
    const char * display_style = "DISPLAY - CIE-XYZ-D65_to_REC.2100-PQ";
    const char * view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-REC2020_2.0";
    const bool do_roundtrip = true;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    const float lum_scale = 0.90257f;  // scale to 4000 nits
    GenerateIdentityLut3D(values, lut_size, lum_scale);
    test.setCustomValues(values);

    // TODO: Investigate why this is not closer.
    test.setErrorThreshold(0.03f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_4000nit_rec2020_inv)
{
    const char * display_style = "DISPLAY - CIE-XYZ-D65_to_REC.2100-PQ";
    const char * view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-REC2020_2.0";
    const bool do_roundtrip = false;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    const float lum_scale = 0.90257f;  // scale to 4000 nits
    GenerateIdentityLut3D(values, lut_size, lum_scale);
    test.setCustomValues(values);

    test.setRelativeComparison(true);
    test.setExpectedMinimalValue(1.f);
    test.setErrorThreshold(0.001f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_d60_4000nit_p3_rndtrip)
{
    const char* display_style = "DISPLAY - CIE-XYZ-D65_to_ST2084-P3-D65";
    const char* view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-P3-D60-in-P3-D65_2.0";
    const bool do_roundtrip = true;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    const float lum_scale = 0.90257f;  // scale to 4000 nits
    GenerateIdentityLut3D(values, lut_size, lum_scale);
    test.setCustomValues(values);

    // TODO: Investigate why this is not closer.
    test.setErrorThreshold(0.03f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_d60_4000nit_p3_inv)
{
    const char* display_style = "DISPLAY - CIE-XYZ-D65_to_ST2084-P3-D65";
    const char* view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-P3-D60-in-P3-D65_2.0";
    const bool do_roundtrip = false;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    const int lut_size = 17;
    OCIOGPUTest::CustomValues values;
    const float lum_scale = 0.90257f;  // scale to 4000 nits
    GenerateIdentityLut3D(values, lut_size, lum_scale);
    test.setCustomValues(values);

    // Difference is on equal RGB, above about 3600, peaking around 3684, and stopping at 3696
    test.setRelativeComparison(true);
    test.setExpectedMinimalValue(1.f);
    test.setErrorThreshold(0.005f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_nan_bug)
{
    const char* display_style = "DISPLAY - CIE-XYZ-D65_to_ST2084-P3-D65";
    const char* view_style = "ACES-OUTPUT - ACES2065-1_to_CIE-XYZ-D65 - HDR-4000nit-P3-D60-in-P3-D65_2.0";
    const bool do_roundtrip = false;
    auto group = BuildDisplayViewTransform(display_style, view_style, do_roundtrip);
    test.setProcessor(group);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
    {
        0.89942779f, 0.89942779f, 0.89942779f, 1.0f,
        // This second value became NaN on the GPU before the Aab_to_RGB fix.
        // FIXME: The GPU is no longer NaN, but it is still hugely different from the CPU.
        // 0.89944305f, 0.89944305f, 0.89944305f, 1.0f
    };
    test.setCustomValues(values);

    test.setRelativeComparison(true);
    test.setExpectedMinimalValue(1.f);
    test.setErrorThreshold(0.01f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_rgb_to_jmh_fwd)
{
    // ACES AP0
    const double data[8] = { 0.7347, 0.2653, 0.0000, 1.0000, 0.0001, -0.0770, 0.32168, 0.33767 };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RGB_TO_JMH_20, &data[0], 8);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
    {
        // ACEScg primaries and secondaries scaled by 4
        2.781808965f, 0.179178253f, -0.022103530f, 1.0f,
        3.344523751f, 3.617862727f, -0.006002689f, 1.0f,
        0.562714786f, 3.438684474f, 0.016100841f, 1.0f,
        1.218191035f, 3.820821747f, 4.022103530f, 1.0f,
        0.655476249f, 0.382137273f, 4.006002689f, 1.0f,
        3.437285214f, 0.561315526f, 3.983899159f, 1.0f,
        // OCIO test values
        0.110000000f, 0.020000000f, 0.040000000f, 0.5f,
        0.710000000f, 0.510000000f, 0.810000000f, 1.0f,
        0.430000000f, 0.820000000f, 0.710000000f, 0.0f,
        // ColorChecker24 (SMPTE 2065-1 2021)
        0.118770000f, 0.087090000f, 0.058950000f, 1.0f,
        0.400020000f, 0.319160000f, 0.237360000f, 1.0f,
        0.184760000f, 0.203980000f, 0.313110000f, 1.0f,
        0.109010000f, 0.135110000f, 0.064930000f, 1.0f,
        0.266840000f, 0.246040000f, 0.409320000f, 1.0f,
        0.322830000f, 0.462080000f, 0.406060000f, 1.0f,
        0.386050000f, 0.227430000f, 0.057770000f, 1.0f,
        0.138220000f, 0.130370000f, 0.337030000f, 1.0f,
        0.302020000f, 0.137520000f, 0.127580000f, 1.0f,
        0.093100000f, 0.063470000f, 0.135250000f, 1.0f,
        0.348760000f, 0.436540000f, 0.106130000f, 1.0f,
        0.486550000f, 0.366850000f, 0.080610000f, 1.0f,
        0.087320000f, 0.074430000f, 0.272740000f, 1.0f,
        0.153660000f, 0.256920000f, 0.090710000f, 1.0f,
        0.217420000f, 0.070700000f, 0.051300000f, 1.0f,
        0.589190000f, 0.539430000f, 0.091570000f, 1.0f,
        0.309040000f, 0.148180000f, 0.274260000f, 1.0f,
        0.149010000f, 0.233780000f, 0.359390000f, 1.0f,
    };
    test.setCustomValues(values);

    test.setErrorThreshold(2e-4f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_rgb_to_jmh_inv)
{
    // ACES AP0
    const double data[8] = { 0.7347, 0.2653, 0.0000, 1.0000, 0.0001, -0.0770, 0.32168, 0.33767 };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_RGB_TO_JMH_20, &data[0], 8);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
    {
        // ACEScg primaries and secondaries scaled by 4
        107.480636597f, 206.827301025f, 25.025110245f, 1.0f,
        173.194076538f, 133.330886841f, 106.183448792f, 1.0f,
        139.210220337f, 191.922363281f, 147.056488037f, 1.0f,
        157.905166626f, 111.975311279f, 192.204727173f, 1.0f,
        79.229278564f, 100.424659729f, 268.442108154f, 1.0f,
        132.888137817f, 173.358779907f, 341.715240479f, 1.0f,
        // OCIO test values
        26.112514496f, 42.523605347f, 4.173158169f, 0.5f,
        79.190460205f, 25.002300262f, 332.159759521f, 1.0f,
        81.912559509f, 39.754810333f, 182.925750732f, 0.0f,
        // ColorChecker24 (SMPTE 2065-1 2021)
        33.924663544f, 12.254567146f, 38.146659851f, 1.0f,
        61.332393646f, 15.169423103f, 39.841842651f, 1.0f,
        47.191543579f, 11.839941978f, 249.107116699f, 1.0f,
        37.328300476f, 13.224150658f, 128.878036499f, 1.0f,
        53.465549469f, 13.121579170f, 285.658966064f, 1.0f,
        65.414512634f, 19.172147751f, 179.324264526f, 1.0f,
        55.711513519f, 37.182041168f, 50.924011230f, 1.0f,
        40.020961761f, 20.762512207f, 271.008331299f, 1.0f,
        47.704769135f, 35.791145325f, 13.975610733f, 1.0f,
        30.385913849f, 14.544739723f, 317.544281006f, 1.0f,
        64.222846985f, 33.487697601f, 119.145133972f, 1.0f,
        65.570358276f, 35.864013672f, 70.842193604f, 1.0f,
        31.800464630f, 23.920211792f, 273.228973389f, 1.0f,
        47.950405121f, 28.027387619f, 144.154159546f, 1.0f,
        38.440967560f, 42.604164124f, 17.892261505f, 1.0f,
        75.117736816f, 40.952045441f, 90.752044678f, 1.0f,
        49.311210632f, 33.812240601f, 348.832092285f, 1.0f,
        47.441757202f, 22.915655136f, 218.454376221f, 1.0f,
    };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-4f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_tonescale_compress_fwd)
{
    const double data[1] = { 1000.f };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_TONESCALE_COMPRESS_20, &data[0], 1);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
    {
        // ACEScg primaries and secondaries scaled by 4
        107.480636597f, 206.827301025f, 25.025110245f, 1.0f,
        173.194076538f, 133.330886841f, 106.183448792f, 1.0f,
        139.210220337f, 191.922363281f, 147.056488037f, 1.0f,
        157.905166626f, 111.975311279f, 192.204727173f, 1.0f,
        79.229278564f, 100.424659729f, 268.442108154f, 1.0f,
        132.888137817f, 173.358779907f, 341.715240479f, 1.0f,
        // OCIO test values
        26.112514496f, 42.523605347f, 4.173158169f, 0.5f,
        79.190460205f, 25.002300262f, 332.159759521f, 1.0f,
        81.912559509f, 39.754810333f, 182.925750732f, 0.0f,
        // ColorChecker24 (SMPTE 2065-1 2021)
        33.924663544f, 12.254567146f, 38.146659851f, 1.0f,
        61.332393646f, 15.169423103f, 39.841842651f, 1.0f,
        47.191543579f, 11.839941978f, 249.107116699f, 1.0f,
        37.328300476f, 13.224150658f, 128.878036499f, 1.0f,
        53.465549469f, 13.121579170f, 285.658966064f, 1.0f,
        65.414512634f, 19.172147751f, 179.324264526f, 1.0f,
        55.711513519f, 37.182041168f, 50.924011230f, 1.0f,
        40.020961761f, 20.762512207f, 271.008331299f, 1.0f,
        47.704769135f, 35.791145325f, 13.975610733f, 1.0f,
        30.385913849f, 14.544739723f, 317.544281006f, 1.0f,
        64.222846985f, 33.487697601f, 119.145133972f, 1.0f,
        65.570358276f, 35.864013672f, 70.842193604f, 1.0f,
        31.800464630f, 23.920211792f, 273.228973389f, 1.0f,
        47.950405121f, 28.027387619f, 144.154159546f, 1.0f,
        38.440967560f, 42.604164124f, 17.892261505f, 1.0f,
        75.117736816f, 40.952045441f, 90.752044678f, 1.0f,
        49.311210632f, 33.812240601f, 348.832092285f, 1.0f,
        47.441757202f, 22.915655136f, 218.454376221f, 1.0f,
        93.610260010f, 0.439610571f, 108.271926880f, 1.0f,
        77.237663269f, 0.131636351f, 33.296318054f, 1.0f,
        61.655914307f, 0.041985143f, 291.004058838f, 1.0f,
        47.493667603f, 0.048908804f, 297.386047363f, 1.0f,
        33.264842987f, 0.283808023f, 234.276382446f, 1.0f,
        21.467216492f, 0.409062684f, 255.025634766f, 1.0f,
        // Spectrally non-selective 18 % reflecting diffuser
        44.938602448f, 0.000004705f, 299.357757568f, 1.0f,
        // Perfect reflecting diffuser
        98.969635010f, 0.000083445f, 5.640549183f, 1.0f,
    };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-4f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_tonescale_compress_inv)
{
    const double data[1] = { 1000.f };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_TONESCALE_COMPRESS_20, &data[0], 1);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
    {
        // ACEScg primaries and secondaries scaled by 4
        110.702453613f, 211.251770020f, 25.025110245f, 1.0f,
        168.016815186f, 129.796249390f, 106.183448792f, 1.0f,
        140.814849854f, 193.459213257f, 147.056488037f, 1.0f,
        156.429519653f, 110.938514709f, 192.204727173f, 1.0f,
        80.456542969f, 98.490524292f, 268.442108154f, 1.0f,
        135.172195435f, 175.559280396f, 341.715240479f, 1.0f,
        // OCIO test values
        18.187314987f, 33.819175720f, 4.173158169f, 0.5f,
        80.413116455f, 21.309329987f, 332.159759521f, 1.0f,
        83.447891235f, 37.852291107f, 182.925750732f, 0.0f,
        // ColorChecker24 (SMPTE 2065-1 2021)
        27.411964417f, 13.382769585f, 38.146659851f, 1.0f,
        59.987670898f, 14.391894341f, 39.841842651f, 1.0f,
        43.298923492f, 12.199877739f, 249.107116699f, 1.0f,
        31.489658356f, 14.075142860f, 128.878036499f, 1.0f,
        50.749198914f, 12.731814384f, 285.658966064f, 1.0f,
        64.728637695f, 18.593795776f, 179.324264526f, 1.0f,
        53.399448395f, 37.394428253f, 50.924011230f, 1.0f,
        34.719596863f, 21.616765976f, 271.008331299f, 1.0f,
        43.910713196f, 36.788166046f, 13.975610733f, 1.0f,
        23.196525574f, 15.118354797f, 317.544281006f, 1.0f,
        63.348674774f, 33.283493042f, 119.145133972f, 1.0f,
        64.908889771f, 35.371044159f, 70.842193604f, 1.0f,
        24.876911163f, 23.143159866f, 273.228973389f, 1.0f,
        44.203376770f, 28.918329239f, 144.154159546f, 1.0f,
        32.824356079f, 43.447875977f, 17.892261505f, 1.0f,
        75.830871582f, 39.872474670f, 90.752044678f, 1.0f,
        45.823116302f, 34.652069092f, 348.832092285f, 1.0f,
        43.597240448f, 23.079078674f, 218.454376221f, 1.0f,
        96.212783813f, 0.322624743f, 108.271926880f, 1.0f,
        78.222122192f, 0.094044082f, 33.296318054f, 1.0f,
        60.364795685f, 0.031291425f, 291.004058838f, 1.0f,
        43.659111023f, 0.038717352f, 297.386047363f, 1.0f,
        26.623359680f, 0.269155562f, 234.276382446f, 1.0f,
        12.961384773f, 0.366550505f, 255.025634766f, 1.0f,
        // Spectrally non-selective 18 % reflecting diffuser
        40.609165192f, 0.000000000f, 299.357757568f, 1.0f,
        // Perfect reflecting diffuser
        101.899215698f, 0.000068110f, 5.640549183f, 1.0f,
    };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-4f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_gamut_compress_fwd)
{
    const double data[9] = {
        // Peak luminance
        1000.f,
        // P3D65 gamut
        0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3127, 0.3290
    };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMPRESS_20, &data[0], 9);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
    {
        // ACEScg primaries and secondaries scaled by 4
        110.702453613f, 211.251770020f, 25.025110245f, 1.0f,
        168.016815186f, 129.796249390f, 106.183448792f, 1.0f,
        140.814849854f, 193.459213257f, 147.056488037f, 1.0f,
        156.429519653f, 110.938514709f, 192.204727173f, 1.0f,
        80.456542969f, 98.490524292f, 268.442108154f, 1.0f,
        135.172195435f, 175.559280396f, 341.715240479f, 1.0f,
        // OCIO test values
        18.187314987f, 33.819175720f, 4.173158169f, 0.5f,
        80.413116455f, 21.309329987f, 332.159759521f, 1.0f,
        83.447891235f, 37.852291107f, 182.925750732f, 0.0f,
        // ColorChecker24 (SMPTE 2065-1 2021)
        27.411964417f, 13.382769585f, 38.146659851f, 1.0f,
        59.987670898f, 14.391894341f, 39.841842651f, 1.0f,
        43.298923492f, 12.199877739f, 249.107116699f, 1.0f,
        31.489658356f, 14.075142860f, 128.878036499f, 1.0f,
        50.749198914f, 12.731814384f, 285.658966064f, 1.0f,
        64.728637695f, 18.593795776f, 179.324264526f, 1.0f,
        53.399448395f, 37.394428253f, 50.924011230f, 1.0f,
        34.719596863f, 21.616765976f, 271.008331299f, 1.0f,
        43.910713196f, 36.788166046f, 13.975610733f, 1.0f,
        23.196525574f, 15.118354797f, 317.544281006f, 1.0f,
        63.348674774f, 33.283493042f, 119.145133972f, 1.0f,
        64.908889771f, 35.371044159f, 70.842193604f, 1.0f,
        24.876911163f, 23.143159866f, 273.228973389f, 1.0f,
        44.203376770f, 28.918329239f, 144.154159546f, 1.0f,
        32.824356079f, 43.447875977f, 17.892261505f, 1.0f,
        75.830871582f, 39.872474670f, 90.752044678f, 1.0f,
        45.823116302f, 34.652069092f, 348.832092285f, 1.0f,
        43.597240448f, 23.079078674f, 218.454376221f, 1.0f,
        96.212783813f, 0.322624743f, 108.271926880f, 1.0f,
        78.222122192f, 0.094044082f, 33.296318054f, 1.0f,
        60.364795685f, 0.031291425f, 291.004058838f, 1.0f,
        43.659111023f, 0.038717352f, 297.386047363f, 1.0f,
        26.623359680f, 0.269155562f, 234.276382446f, 1.0f,
        12.961384773f, 0.366550505f, 255.025634766f, 1.0f,
        // Spectrally non-selective 18 % reflecting diffuser
        40.609165192f, 0.000000000f, 299.357757568f, 1.0f,
        // Perfect reflecting diffuser
        101.899215698f, 0.000068110f, 5.640549183f, 1.0f,
    };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-4f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces2_gamut_compress_inv)
{
    const double data[9] = {
        // Peak luminance
        1000.f,
        // P3D65 gamut
        0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3127, 0.3290
    };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_GAMUT_COMPRESS_20, &data[0], 9);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
    {
        // ACEScg primaries and secondaries scaled by 4
        107.831291199f, 174.252944946f, 25.025119781f, 1.0f,
        168.028198242f, 118.224960327f, 106.183464050f, 1.0f,
        140.030105591f, 127.177192688f, 147.056488037f, 1.0f,
        156.512435913f, 73.218856812f, 192.204727173f, 1.0f,
        79.378631592f, 72.613555908f, 268.442108154f, 1.0f,
        133.827835083f, 149.929809570f, 341.715240479f, 1.0f,
        // OCIO test values
        18.194000244f, 33.312938690f, 4.173166752f, 0.5f,
        80.413116455f, 21.309329987f, 332.159759521f, 1.0f,
        83.467437744f, 37.305160522f, 182.925750732f, 0.0f,
        // ColorChecker24 (SMPTE 2065-1 2021)
        27.411962509f, 13.382793427f, 38.146591187f, 1.0f,
        59.987670898f, 14.391893387f, 39.841842651f, 1.0f,
        43.298923492f, 12.199877739f, 249.107116699f, 1.0f,
        31.489658356f, 14.075142860f, 128.878036499f, 1.0f,
        50.749198914f, 12.731814384f, 285.658966064f, 1.0f,
        64.728637695f, 18.593795776f, 179.324264526f, 1.0f,
        53.399448395f, 37.394428253f, 50.924011230f, 1.0f,
        34.719596863f, 21.616765976f, 271.008331299f, 1.0f,
        43.910709381f, 36.788166046f, 13.975610733f, 1.0f,
        23.196525574f, 15.118361473f, 317.544250488f, 1.0f,
        63.348674774f, 33.283493042f, 119.145133972f, 1.0f,
        64.908889771f, 35.371044159f, 70.842193604f, 1.0f,
        24.876916885f, 23.143167496f, 273.229034424f, 1.0f,
        44.203376770f, 28.918329239f, 144.154159546f, 1.0f,
        32.824352264f, 43.447864532f, 17.892255783f, 1.0f,
        75.830871582f, 39.872474670f, 90.752044678f, 1.0f,
        45.823104858f, 34.652038574f, 348.832092285f, 1.0f,
        43.635551453f, 21.629474640f, 218.454376221f, 1.0f,
    };
    test.setCustomValues(values);

    // TODO: Improve inversion match?
    test.setErrorThreshold(4e-4f);
}

// The next four tests run into a problem on some graphics cards where 0.0 * Inf = 0.0,
// rather than the correct value of NaN.  Therefore turning off TestInfinity for these tests.

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_darktodim10_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_aces_darktodim10_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_ACES_DARK_TO_DIM_10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_rec2100_surround_fwd)
{
    const double data[1] = { 0.7 };
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_REC2100_SURROUND, &data[0], 1);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setErrorThreshold(2e-6f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_rec2100_surround_inv)
{
    const double data[1] = { 1. / 0.7 };
    // (Since we're not calling inverse() here, set the param to be the inverse of prev test.)
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_REC2100_SURROUND, &data[0], 1);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    test.setErrorThreshold(2e-6f);

    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_RGB_TO_HSV_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);

    test.setTestNaN(false);
#ifdef __APPLE__
    test.setTestInfinity(false);
#endif
}

OCIO_ADD_GPU_TEST(FixedFunction, style_RGB_TO_HSV_fwd_custom)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
            1.500f,   2.500f,   0.500f,   0.50f,
            3.125f,  -0.625f,   1.250f,   1.00f,
           -5.f/3.f, -4.f/3.f, -1.f/3.f,  0.25f,
            0.100f,  -0.800f,   0.400f,   0.25f,
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_RGB_TO_HSV_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    // Mostly passes at 1e-6 but there are some values where the result is large in one chan
    // and very small in another and so doing relative comparison per channel is not fair.
    test.setErrorThreshold(5e-4f);
    test.setExpectedMinimalValue(5e-3f);
    test.setRelativeComparison(true);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_RGB_TO_HSV_inv_custom)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_RGB_TO_HSV);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    OCIOGPUTest::CustomValues values;
    values.m_inputValues =
        {
             3.f/12.f,  0.80f,  2.50f,  0.50f,      // val > 1
            11.f/12.f,  1.20f,  2.50f,  1.00f,      // sat > 1
            15.f/24.f,  0.80f, -2.00f,  0.25f,      // val < 0
            19.f/24.f,  1.50f, -0.40f,  0.25f,      // sat > 1, val < 0
           -89.f/24.f,  0.50f,  0.40f,  2.00f,      // under-range hue
            81.f/24.f,  1.50f, -0.40f, -0.25f,      // over-range hue, sat > 1, val < 0
            81.f/24.f, -0.50f,  0.40f,  0.00f,      // sat < 0
              0.5000f,  2.50f,  0.04f,  0.00f,      // sat > 2
        };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_xyY_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_xyY);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_xyY_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_xyY);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_uvY_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_uvY);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setErrorThreshold(1e-6f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_uvY_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_uvY);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    test.setErrorThreshold(1e-5f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_LUV_fwd)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_LUV);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setProcessor(func);

    test.setTestInfinity(false);
    test.setErrorThreshold(5e-5f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_LUV_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_LUV);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    test.setTestInfinity(false);
    test.setErrorThreshold(1e-5f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_LIN_TO_PQ_fwd)
{
    auto func = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_LIN_TO_PQ);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    
    test.setWideRangeInterval(-0.1f, 100.1f);
    test.setProcessor(func);

    test.setTestInfinity(false);
    test.setTestNaN(false);

    // Using large threshold for SSE2 as that will enable usage of fast but
    // approximate power function ssePower.
    test.setErrorThreshold(OCIO_USE_SSE2 ? 0.0008f : 2e-5f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_LIN_TO_PQ_inv)
{
    auto func = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_LIN_TO_PQ);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    // Picking a tight epsilon is tricky with this function due to nested power
    // operations and [0,100] output range for [0,1] input range.
 
    // MaxDiff in range [-0.1, 1.1] against...
    //      scalar double precision         : 0.000094506
    //      scalar single precision         : 0.000144501
    //      SSE2 (intrinsic pow)            : 0.000144441
    //      SSE2 (fastPower)                : 0.002207260
    test.setWideRangeInterval(-0.1f, 1.1f);
    test.setProcessor(func);
    test.setRelativeComparison(true); // Since the output range will be 0..100, we set the relative epsilon.
    test.setErrorThreshold(OCIO_USE_SSE2 ? 0.0023f : 1.5e-4f);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

namespace
{
namespace HLG
{
    // Parameters for the Rec.2100 HLG curve.
    double params[10]
    {
        0.0,            // mirror point
        0.25,           // break point

        // Gamma segment.
        0.5,            // gamma power
        1.0,            // post-power scale
        0.0,            // pre-power offset

        // Log segment.
        std::exp(1.0),  // log base (e)
        0.17883277,     // log-side slope
        0.807825590164, // log-side offset
        1.0,            // lin-side slope
        -0.07116723,    // lin-side offset
    };
}
}

OCIO_ADD_GPU_TEST(FixedFunction, style_LIN_TO_GAMMA_LOG_fwd)
{
    auto func = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_LIN_TO_GAMMA_LOG, HLG::params, 10);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setWideRangeInterval(-0.1f, 3.35f); // Output ~[-0.3, 1.02]
    test.setProcessor(func);
    test.setErrorThreshold(1e-6f);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_LIN_TO_GAMMA_LOG_inv)
{
    auto func = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_LIN_TO_GAMMA_LOG, HLG::params, 10);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setWideRangeInterval(-0.3f, 1.02f); // Output ~[-0.1, 3.35]
    test.setProcessor(func);
    test.setErrorThreshold(1e-6f);
    test.setTestInfinity(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_LIN_TO_DOUBLE_LOG_fwd)
{
    double params[13]
    {
        10.0,                  // Base for the log
        0.1,                   // Break point between Log1 and Linear segments
        0.5,                   // Break point between Linear and Log2 segments
        -1.0, -1.0, -1.0, 0.2, // Log curve 1: LogSideSlope, LogSideOffset, LinSideSlope, LinSideOffset
        1.0, 1.0, 1.0, 0.5,    // Log curve 2: LogSideSlope, LogSideOffset, LinSideSlope, LinSideOffset
        1.0, 0.0,              // Linear segment slope and offset
    };

    auto func = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_LIN_TO_DOUBLE_LOG, params, 13);
    func->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

    test.setWideRangeInterval(-1.0f, 2.0f); // Output ~[-1.08, 1.4]
    test.setProcessor(func);
    test.setErrorThreshold(1e-6f);
    test.setTestInfinity(false);
    test.setTestNaN(false);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_LIN_TO_DOUBLE_LOG_inv)
{
    double params[13]
    {
        10.0,                  // Base for the log
        0.1,                   // Break point between Log1 and Linear segments
        0.5,                   // Break point between Linear and Log2 segments
        -1.0, -1.0, -1.0, 0.2, // Log curve 1: LogSideSlope, LogSideOffset, LinSideSlope, LinSideOffset
        1.0, 1.0, 1.0, 0.5,    // Log curve 2: LogSideSlope, LogSideOffset, LinSideSlope, LinSideOffset
        1.0, 0.0,              // Linear segment slope and offset
    };

    auto func = OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_LIN_TO_DOUBLE_LOG, params, 13);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setWideRangeInterval(-1.1f, 1.4f); // Output ~[-1.0, 2.0]
    test.setProcessor(func);
    test.setErrorThreshold(1e-6f);
    test.setTestInfinity(false);
}
