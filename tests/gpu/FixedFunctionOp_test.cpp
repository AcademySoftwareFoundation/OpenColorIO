// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"

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
    values.m_inputValues =
    {
        // ALEXA Wide Gamut
         0.96663408,  0.04819045,  0.007193  , 0.0f,
         0.11554181,  1.18493814, -0.0665935 , 0.0f,
        -0.08217583, -0.23312863,  1.05940073, 0.0f,
        // BMD Wide Gamut V4
         0.92988223,  0.03024877, -0.02236726, 0.0f,
         0.12427823,  1.19236958, -0.0801014 , 0.0f,
        -0.05416045, -0.22261835,  1.10246866, 0.0f,
        // Cinema Gamut
         1.10869873, -0.05317572, -0.00306262, 0.0f,
         0.00142396,  1.3123991 , -0.22332298, 0.0f,
        -0.11012268, -0.25922338,  1.2263856 , 0.0f,
        // REDWideGamutRGB
         1.14983724, -0.02548932, -0.06720325, 0.0f,
        -0.06796986,  1.30455482, -0.31973675, 0.0f,
        -0.08186897, -0.27906488,  1.38694022, 0.0f,
        // S-Gamut3
         1.08979822, -0.03117187, -0.00326358, 0.0f,
        -0.03276505,  1.18293669, -0.00156985, 0.0f,
        -0.05703317, -0.15176482,  1.00483344, 0.0f,
        // Venice S-Gamut3
         1.15183946, -0.04052512, -0.01231821, 0.0f,
        -0.11769985,  1.20661478,  0.00725126, 0.0f,
        -0.03413961, -0.16608966,  1.00506695, 0.0f,
        // V-Gamut
         1.04839741, -0.02998665, -0.00313943, 0.0f,
         0.01196121,  1.14840383, -0.00963746, 0.0f,
        -0.06036022, -0.11841656,  1.01277711, 0.0f,
        // CC24 hue selective patch
         0.13911969,  0.08746966,  0.05927772, 0.0f,
         0.45410454,  0.32112335,  0.23821925, 0.0f,
         0.15262818,  0.19457373,  0.31270094, 0.0f,
         0.11231112,  0.14410331,  0.06487322, 0.0f,
         0.2411364 ,  0.2281726 ,  0.40912009, 0.0f,
         0.27200737,  0.47832396,  0.40502991, 0.0f,
         0.49412209,  0.23219806,  0.05947656, 0.0f,
         0.09734666,  0.10917003,  0.33662333, 0.0f,
         0.37841816,  0.12591768,  0.12897071, 0.0f,
         0.09104857,  0.05404697,  0.13533248, 0.0f,
         0.38014721,  0.47619381,  0.10615457, 0.0f,
         0.60210844,  0.38621775,  0.08225913, 0.0f,
         0.05051657,  0.05367649,  0.27239431, 0.0f,
         0.14276765,  0.28139208,  0.09023085, 0.0f,
         0.28782479,  0.06140174,  0.05256445, 0.0f,
         0.70791153,  0.58026154,  0.09300659, 0.0f,
         0.35456033,  0.12329842,  0.27530981, 0.0f,
         0.08374431,  0.22774917,  0.35839818, 0.0f
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
    values.m_inputValues =
    {
        // ALEXA Wide Gamut
        0.96663409472,  0.08610087633,  0.04698687792,  0.0,
        0.13048231602,  1.18493819237,  0.03576064110,  0.0,
        0.02295053005,  0.00768482685,  1.05940067768,  0.0,
        // BMD Wide Gamut V4
        0.92988222837,  0.07499861717,  0.03569263220,  0.0,
        0.13707232475,  1.19236958027,  0.03312134743,  0.0,
        0.03551697731,  0.01163780689,  1.10246860981,  0.0,
        // Cinema Gamut
        1.10869872570,  0.05432271957,  0.04990577698,  0.0,
        0.07070028782,  1.31239914894,  0.01541912556,  0.0,
        0.02140641212,  0.01080632210,  1.22638559341,  0.0,
        // REDWideGamutRGB
        1.14983725548,  0.06666719913,  0.03411936760,  0.0,
        0.04051816463,  1.30455482006,  0.00601124763,  0.0,
        0.03941023350,  0.01482784748,  1.38694024086,  0.0,
        // S-Gamut3
        1.08979821205,  0.06064450741,  0.04896950722,  0.0,
        0.04843533039,  1.18293666840,  0.05382478237,  0.0,
        0.02941548824,  0.02107459307,  1.00483345985,  0.0,
        // Venice S-Gamut3
        1.15183949471,  0.06142425537,  0.04885411263,  0.0,
        0.01795542240,  1.20661473274,  0.05802130699,  0.0,
        0.03851079941,  0.01796829700,  1.00506699085,  0.0,
        // V-Gamut
        1.04839742184,  0.05834102631,  0.04710924625,  0.0,
        0.06705272198,  1.14840388298,  0.04955554008,  0.0,
        0.02856093645,  0.02944415808,  1.01277709007,  0.0,
        // CC24 hue selective patch
        0.13911968470,  0.08746965975,  0.05927772075,  0.0,
        0.45410454273,  0.32112336159,  0.23821924627,  0.0,
        0.15262818336,  0.19457373023,  0.31270092726,  0.0,
        0.11231111735,  0.14410330355,  0.06487321109,  0.0,
        0.24113640189,  0.22817260027,  0.40912008286,  0.0,
        0.27200737596,  0.47832396626,  0.40502992272,  0.0,
        0.49412208796,  0.23219805956,  0.05947655439,  0.0,
        0.09734666348,  0.10917001963,  0.33662334085,  0.0,
        0.37841814756,  0.12591767311,  0.12897071242,  0.0,
        0.09104856849,  0.05404697359,  0.13533248007,  0.0,
        0.38014721870,  0.47619381547,  0.10615456104,  0.0,
        0.60210841894,  0.38621774316,  0.08225911856,  0.0,
        0.05051657557,  0.05367648602,  0.27239429951,  0.0,
        0.14276765287,  0.28139206767,  0.09023086727,  0.0,
        0.28782477975,  0.06140173972,  0.05256444216,  0.0,
        0.70791155100,  0.58026152849,  0.09300661087,  0.0,
        0.35456031561,  0.12329842150,  0.27530980110,  0.0,
        0.08374431729,  0.22774916887,  0.35839816928,  0.0
    };
    test.setCustomValues(values);

    test.setErrorThreshold(1e-6f);
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

#ifdef __APPLE__
    test.setTestNaN(false);
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

    test.setErrorThreshold(1e-5f);
}

OCIO_ADD_GPU_TEST(FixedFunction, style_XYZ_TO_LUV_inv)
{
    OCIO::FixedFunctionTransformRcPtr func =
        OCIO::FixedFunctionTransform::Create(OCIO::FIXED_FUNCTION_XYZ_TO_LUV);
    func->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    test.setProcessor(func);

    test.setErrorThreshold(1e-5f);
}
