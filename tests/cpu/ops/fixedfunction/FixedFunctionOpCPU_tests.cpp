// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>

#include "ops/fixedfunction/FixedFunctionOpCPU.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"
#include "ops/lut3d/Lut3DOp.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
void ApplyFixedFunction(float * input_32f, 
                        const float * expected_32f, 
                        unsigned numSamples,
                        OCIO::ConstFixedFunctionOpDataRcPtr & fnData, 
                        float errorThreshold,
                        int lineNo)
{
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW_FROM(op = OCIO::GetFixedFunctionCPURenderer(fnData), lineNo);
    OCIO_CHECK_NO_THROW_FROM(op->apply(input_32f, input_32f, numSamples), lineNo);

    for(unsigned idx=0; idx<(numSamples*4); ++idx)
    {
        // Using rel error with a large minExpected value of 1 will transition
        // from absolute error for expected values < 1 and
        // relative error for values > 1.
        const bool equalRel = OCIO::EqualWithSafeRelError(input_32f[idx],
                                                          expected_32f[idx],
                                                          errorThreshold,
                                                          1.0f);
        if (!equalRel)
        {
            std::ostringstream errorMsg;
            errorMsg.precision(14);
            errorMsg << "Index: " << idx;
            errorMsg << " - Values: " << input_32f[idx] << " expected: " << expected_32f[idx];
            errorMsg << " - Threshold: " << errorThreshold;
            OCIO_CHECK_ASSERT_MESSAGE_FROM(0, errorMsg.str(), lineNo);
        }
    }
}
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_red_mod_03)
{
    const unsigned num_samples = 4;

    const float input_32f[num_samples*4] = {
            0.90f,  0.05f,   0.22f,   0.5f,
            0.97f,  0.097f,  0.0097f, 1.0f,
            0.89f,  0.15f,   0.56f,   0.0f,
           -1.0f,  -0.001f,  1.2f,    0.0f
        };

    float output_32f[num_samples*4];
    memcpy(&output_32f[0], &input_32f[0], sizeof(float)*num_samples*4);

    const float expected_32f[num_samples*4] = {
            0.79670035f, 0.05f,       0.19934007f, 0.5f,
            0.83517569f, 0.08474324f, 0.0097f,     1.0f,
            0.87166744f, 0.15f,       0.54984271f, 0.0f,
           -1.0f,       -0.001f,      1.2f,        0.0f
        };

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-7f,
                           __LINE__);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcData,
                           1e-7f,
                           __LINE__);
    }
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_red_mod_10)
{
    const unsigned num_samples = 4;

    const float input_32f[num_samples*4] = {
            0.90f,  0.05f,   0.22f,   0.5f,
            0.97f,  0.097f,  0.0097f, 1.0f,
            0.89f,  0.15f,   0.56f,   0.0f,
           -1.0f,  -0.001f,  1.2f,    0.0f,
        };

    float output_32f[num_samples*4];
    memcpy(&output_32f[0], &input_32f[0], sizeof(float)*num_samples*4);

    const float expected_32f[num_samples*4] = {
            0.77148211f,  0.05f,   0.22f,    0.5f,
            0.80705338f,  0.097f,  0.0097f,  1.0f,
            0.85730940f,  0.15f,   0.56f,    0.0f,
           -1.0f,        -0.001f,  1.2f,     0.0f,
        };

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-7f,
                           __LINE__);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_RED_MOD_10_INV);

        float adjusted_input_32f[num_samples*4];
        memcpy(&adjusted_input_32f[0], &input_32f[0], sizeof(float)*num_samples*4);

        // Note: There is a known issue in ACES 1.0 where the red modifier inverse algorithm 
        // is not quite exact.  Hence the aim values here aren't quite the same as the input.
        adjusted_input_32f[0] = 0.89146208f;
        adjusted_input_32f[4] = 0.96750682f;
        adjusted_input_32f[8] = 0.88518190f;


        ApplyFixedFunction(&output_32f[0], &adjusted_input_32f[0], num_samples, 
                           funcData,
                           1e-7f,
                           __LINE__);
    }
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_glow_03)
{
    const unsigned num_samples = 4;

    const float input_32f[num_samples*4] = {
            0.11f,  0.02f,  0.f,   0.5f, // YC = 0.10
            0.01f,  0.02f,  0.03f, 1.0f, // YC = 0.03
            0.11f,  0.91f,  0.01f, 0.0f, // YC = 0.84
           -1.0f,  -0.001f, 1.2f,  0.0f
        };

    float output_32f[num_samples*4];
    memcpy(&output_32f[0], &input_32f[0], sizeof(float)*num_samples*4);

    const float expected_32f[num_samples*4] = {
            0.11392101f, 0.02071291f, 0.0f,        0.5f,
            0.01070833f, 0.02141666f, 0.03212499f, 1.0f,
            0.10999999f, 0.91000002f, 0.00999999f, 0.0f,
           -1.0f,       -0.001f,      1.2f,        0.0f
        };

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-7f,
                           __LINE__);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_GLOW_03_INV);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcData,
                           1e-7f,
                           __LINE__);
    }
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_glow_10)
{
    const unsigned num_samples = 4;

    const float input_32f[num_samples*4] = {
            0.11f,  0.02f,  0.f,   0.5f, // YC = 0.10
            0.01f,  0.02f,  0.03f, 1.0f, // YC = 0.03
            0.11f,  0.91f,  0.01f, 0.0f, // YC = 0.84
           -1.0f,  -0.001f, 1.2f,  0.0f
        };

    float output_32f[num_samples*4];
    memcpy(&output_32f[0], &input_32f[0], sizeof(float)*num_samples*4);

    const float expected_32f[num_samples*4] = {
            0.11154121f, 0.02028021f, 0.0f,        0.5f,
            0.01047222f, 0.02094444f, 0.03141666f, 1.0f,
            0.10999999f, 0.91000002f, 0.00999999f, 0.0f,
           -1.0f,       -0.001f,      1.2f,        0.0f
        };

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_GLOW_10_FWD);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-7f,
                           __LINE__);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_GLOW_10_INV);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcData,
                           1e-7f,
                           __LINE__);
    }
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_dark_to_dim_10)
{
    const unsigned num_samples = 4;

    const float input_32f[num_samples*4] = {
            0.11f,  0.02f,  0.04f, 0.5f,
            0.71f,  0.51f,  0.92f, 1.0f,
            0.43f,  0.82f,  0.71f, 0.0f,
           -0.3f,   0.5f,   1.2f,  0.0f
        };

    float output_32f[num_samples*4];
    memcpy(&output_32f[0], &input_32f[0], sizeof(float)*num_samples*4);

    const float expected_32f[num_samples*4] = {
            0.11661188f,  0.02120216f,  0.04240432f,  0.5f,
            0.71719729f,  0.51516991f,  0.92932611f,  1.0f,
            0.43281638f,  0.82537078f,  0.71465027f,  0.0f,
           -0.30653429f,  0.51089048f,  1.22613716f,  0.0f
        };

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-7f,
                           __LINE__);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcData,
                           1e-7f,
                           __LINE__);
    }
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_gamut_map_13)
{
    const unsigned num_samples = 39;

    // Test dataset consist of ACEScg values:
    // - Common camera color space primaries
    // - ColorChecker 24 values as per SMPTE 2065-1
    const float input_32f[num_samples*4] = {
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

    float output_32f[num_samples*4];
    memcpy(&output_32f[0], &input_32f[0], sizeof(float)*num_samples*4);

    // Above values are passed through ctlrender and the CTL implementation (1),
    // using openEXR 32bits as the i/o image format. For more details, see
    // https://gist.github.com/remia/380d972fa568493d570f2ba298b3f23a
    // (1) urn:ampas:aces:transformId:v1.5:LMT.Academy.GamutCompress.a1.3.0
    //     Note: AP0 to / from AP1 conversions have been disabled
    const float expected_32f[num_samples*4] = {
        // ALEXA Wide Gamut
        0.96663409472f,  0.08610087633f,  0.04698687792f,  0.0f,
        0.13048231602f,  1.18493819237f,  0.03576064110f,  0.0f,
        0.02295053005f,  0.00768482685f,  1.05940067768f,  0.0f,
        // BMD Wide Gamut
        0.92980366945f,  0.07499730587f,  0.03567957878f,  0.0f,
        0.13714194298f,  1.19238424301f,  0.03311228752f,  0.0f,
        0.03551459312f,  0.01163744926f,  1.10254764557f,  0.0f,
        // Cinema Gamut
        1.10869872570f,  0.05432271957f,  0.04990577698f,  0.0f,
        0.07070028782f,  1.31239914894f,  0.01541912556f,  0.0f,
        0.02140641212f,  0.01080632210f,  1.22638559341f,  0.0f,
        // REDWideGamutRGB
        1.14983725548f,  0.06666719913f,  0.03411936760f,  0.0f,
        0.04051816463f,  1.30455482006f,  0.00601124763f,  0.0f,
        0.03941023350f,  0.01482784748f,  1.38694024086f,  0.0f,
        // S-Gamut3
        1.08979821205f,  0.06064450741f,  0.04896950722f,  0.0f,
        0.04843533039f,  1.18293666840f,  0.05382478237f,  0.0f,
        0.02941548824f,  0.02107459307f,  1.00483345985f,  0.0f,
        // Venice S-Gamut3
        1.15183949471f,  0.06142425537f,  0.04885411263f,  0.0f,
        0.01795542240f,  1.20661473274f,  0.05802130699f,  0.0f,
        0.03851079941f,  0.01796829700f,  1.00506699085f,  0.0f,
        // V-Gamut
        1.04839742184f,  0.05834102631f,  0.04710924625f,  0.0f,
        0.06705272198f,  1.14840388298f,  0.04955554008f,  0.0f,
        0.02856093645f,  0.02944415808f,  1.01277709007f,  0.0f,
        // CC24 hue selective patch
        0.13911968470f,  0.08746965975f,  0.05927771330f,  0.0f,
        0.45410454273f,  0.32112336159f,  0.23821924627f,  0.0f,
        0.15262818336f,  0.19457373023f,  0.31270095706f,  0.0f,
        0.11231111735f,  0.14410330355f,  0.06487321109f,  0.0f,
        0.24113640189f,  0.22817260027f,  0.40912008286f,  0.0f,
        0.27200737596f,  0.47832396626f,  0.40502992272f,  0.0f,
        0.49412208796f,  0.23219805956f,  0.05947655439f,  0.0f,
        0.09734666348f,  0.10917001963f,  0.33662334085f,  0.0f,
        0.37841814756f,  0.12591767311f,  0.12897071242f,  0.0f,
        0.09104857594f,  0.05404697359f,  0.13533248007f,  0.0f,
        0.38014721870f,  0.47619381547f,  0.10615456104f,  0.0f,
        0.60210841894f,  0.38621774316f,  0.08225911856f,  0.0f,
        0.05051657557f,  0.05367648602f,  0.27239432931f,  0.0f,
        0.14276765287f,  0.28139206767f,  0.09023086727f,  0.0f,
        0.28782477975f,  0.06140173972f,  0.05256444216f,  0.0f,
        0.70791155100f,  0.58026152849f,  0.09300661087f,  0.0f,
        0.35456034541f,  0.12329842150f,  0.27530980110f,  0.0f,
        0.08374431729f,  0.22774916887f,  0.35839819908f,  0.0f
    };

    OCIO::FixedFunctionOpData::Params params = { 1.147, 1.264, 1.312, 0.815, 0.803, 0.880, 1.2 };

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_GAMUT_COMP_13_FWD,
                                                          params);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-6f,
                           __LINE__);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_GAMUT_COMP_13_INV,
                                                          params);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcData,
                           1e-6f,
                           __LINE__);
    }
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_output_transform_20)
{
    const unsigned num_samples = 35;

    float input_32f[num_samples*4] = {
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

    float input2_32f[num_samples * 4];
    memcpy(&input2_32f[0], &input_32f[0], sizeof(float)*num_samples * 4);

    const float expected_32f[num_samples*4] = {
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

    OCIO::FixedFunctionOpData::Params params = {
        // Peak luminance
        1000.f,
        // P3D65 gamut
        0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3127, 0.3290
    };
    OCIO::ConstFixedFunctionOpDataRcPtr funcData
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_FWD,
                                                      params);

    ApplyFixedFunction(&input2_32f[0], &expected_32f[0], num_samples,
                       funcData,
                       1e-5f,
                       __LINE__);

    OCIO::ConstFixedFunctionOpDataRcPtr funcData2
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_INV,
                                                      params);

    ApplyFixedFunction(&input2_32f[0], &input_32f[0], num_samples,
                       funcData2,
                       1e-4f,
                       __LINE__);
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_ot_20_rec709_100n_rt)
{
    const int lut_size = 8;
    const int num_channels = 4;
    int num_samples = lut_size * lut_size * lut_size;
    std::vector<float> input_32f(num_samples * num_channels, 0.f);
    std::vector<float> output_32f(num_samples * num_channels, 0.f);

    GenerateIdentityLut3D(input_32f.data(), lut_size, num_channels, OCIO::LUT3DORDER_FAST_RED);

    OCIO::FixedFunctionOpData::Params params = {
        // Peak luminance
        100.f,
        // Rec709 gamut
        0.6400, 0.3300, 0.3000, 0.6000, 0.1500, 0.0600, 0.3127, 0.3290
    };

    OCIO::ConstFixedFunctionOpDataRcPtr funcData
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_INV,
                                                      params);

    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetFixedFunctionCPURenderer(funcData));
    OCIO_CHECK_NO_THROW(op->apply(&input_32f[0], &output_32f[0], num_samples));

    OCIO::ConstFixedFunctionOpDataRcPtr funcData2
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_FWD,
                                                      params);

    ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples,
                       funcData2,
                       1e-3f,
                       __LINE__);
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_ot_20_p3d65_100n_rt)
{
    const int lut_size = 8;
    const int num_channels = 4;
    int num_samples = lut_size * lut_size * lut_size;
    std::vector<float> input_32f(num_samples * num_channels, 0.f);
    std::vector<float> output_32f(num_samples * num_channels, 0.f);

    GenerateIdentityLut3D(input_32f.data(), lut_size, num_channels, OCIO::LUT3DORDER_FAST_RED);

    OCIO::FixedFunctionOpData::Params params = {
        // Peak luminance
        100.f,
        // P3D65 gamut
        0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3127, 0.3290
    };

    OCIO::ConstFixedFunctionOpDataRcPtr funcData
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_INV,
                                                      params);

    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetFixedFunctionCPURenderer(funcData));
    OCIO_CHECK_NO_THROW(op->apply(&input_32f[0], &output_32f[0], num_samples));

    OCIO::ConstFixedFunctionOpDataRcPtr funcData2
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_FWD,
                                                      params);

    ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples,
                       funcData2,
                       1e-2f,
                       __LINE__);
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_ot_20_p3d65_1000n_rt)
{
    const int lut_size = 8;
    const int num_channels = 4;
    int num_samples = lut_size * lut_size * lut_size;
    std::vector<float> input_32f(num_samples * num_channels, 0.f);
    std::vector<float> output_32f(num_samples * num_channels, 0.f);

    GenerateIdentityLut3D(input_32f.data(), lut_size, num_channels, OCIO::LUT3DORDER_FAST_RED);

    const float normPeakLuminance = 10.f;
    for (unsigned int i = 0; i < input_32f.size(); ++i)
    {
        input_32f[i] *= normPeakLuminance;
    }

    OCIO::FixedFunctionOpData::Params params = {
        // Peak luminance
        1000.f,
        // P3D65 gamut
        0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3127, 0.3290
    };

    OCIO::ConstFixedFunctionOpDataRcPtr funcData
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_INV,
                                                      params);

    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetFixedFunctionCPURenderer(funcData));
    OCIO_CHECK_NO_THROW(op->apply(&input_32f[0], &output_32f[0], num_samples));

    OCIO::ConstFixedFunctionOpDataRcPtr funcData2
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_OUTPUT_TRANSFORM_20_FWD,
                                                      params);

    ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples,
                       funcData2,
                       1e-3f,
                       __LINE__);
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_rgb_to_jmh_20)
{
    const unsigned num_samples = 27;

    // The following input values are processed and carried over to the next
    // FixedFunctionOp test along the ACES2 output transform steps.

    float input_32f[num_samples*4] = {
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

    float input2_32f[num_samples * 4];
    memcpy(&input2_32f[0], &input_32f[0], sizeof(float)*num_samples * 4);

    const float expected_32f[num_samples*4] = {
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

    // ACES AP0
    OCIO::FixedFunctionOpData::Params params = {0.7347, 0.2653, 0.0000, 1.0000, 0.0001, -0.0770, 0.32168, 0.33767};
    OCIO::ConstFixedFunctionOpDataRcPtr funcData
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_RGB_TO_JMh_20,
                                                      params);

    ApplyFixedFunction(&input2_32f[0], &expected_32f[0], num_samples,
                       funcData,
                       1e-5f,
                       __LINE__);

    OCIO::ConstFixedFunctionOpDataRcPtr funcData2
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_JMh_TO_RGB_20,
                                                      params);

    ApplyFixedFunction(&input2_32f[0], &input_32f[0], num_samples,
                       funcData2,
                       1e-4f,
                       __LINE__);
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_tonescale_compress_20)
{
    const unsigned num_samples = 27;

    float input_32f[num_samples*4] = {
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

    float input2_32f[num_samples * 4];
    memcpy(&input2_32f[0], &input_32f[0], sizeof(float)*num_samples * 4);

    const float expected_32f[num_samples*4] = {
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
    };

    OCIO::FixedFunctionOpData::Params params = {1000.f};
    OCIO::ConstFixedFunctionOpDataRcPtr funcData
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_FWD,
                                                      params);

    ApplyFixedFunction(&input2_32f[0], &expected_32f[0], num_samples,
                       funcData,
                       1e-5f,
                       __LINE__);

    OCIO::ConstFixedFunctionOpDataRcPtr funcData2
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_TONESCALE_COMPRESS_20_INV,
                                                      params);

    ApplyFixedFunction(&input2_32f[0], &input_32f[0], num_samples,
                       funcData2,
                       1e-4f,
                       __LINE__);
}

OCIO_ADD_TEST(FixedFunctionOpCPU, aces_gamut_map_20)
{
    const unsigned num_samples = 27;

    float input_32f[num_samples*4] = {
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
    };

    float input2_32f[num_samples * 4];
    memcpy(&input2_32f[0], &input_32f[0], sizeof(float)*num_samples * 4);

    const float expected_32f[num_samples*4] = {
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

    OCIO::FixedFunctionOpData::Params params = {
        // Peak luminance
        1000.f,
        // P3D65 gamut
        0.680, 0.320, 0.265, 0.690, 0.150, 0.060, 0.3127, 0.3290
    };
    OCIO::ConstFixedFunctionOpDataRcPtr funcData
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_FWD,
                                                      params);

    ApplyFixedFunction(&input2_32f[0], &expected_32f[0], num_samples,
                       funcData,
                       1e-5f,
                       __LINE__);

    OCIO::ConstFixedFunctionOpDataRcPtr funcData2
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::ACES_GAMUT_COMPRESS_20_INV,
                                                      params);

    ApplyFixedFunction(&input2_32f[0], &input_32f[0], num_samples,
                       funcData2,
                       1e-5f,
                       __LINE__);
}

OCIO_ADD_TEST(FixedFunctionOpCPU, rec2100_surround)
{
    const unsigned num_samples = 5;

    float input_32f[num_samples*4] = {
            8.4e-5f, 2.4e-5f, 1.4e-4f, 0.1f,
            0.11f,   0.02f,   0.04f,   0.5f,
            0.71f,   0.51f,   0.81f,   1.0f,
            0.43f,   0.82f,   0.71f,   0.0f,
           -1.00f,  -0.001f,  1.2f,    0.0f
        };
    {
        OCIO::FixedFunctionOpData::Params params = { 0.78 };

        float output_32f[num_samples * 4];
        memcpy(&output_32f[0], &input_32f[0], sizeof(float)*num_samples * 4);

        const float expected_32f[num_samples*4] = {
                0.000637205163f, 0.000182058618f, 0.001062008605f, 0.1f,
                0.21779590f,     0.03959925f,     0.07919850f,     0.5f,
                0.80029451f,     0.57485944f,     0.91301214f,     1.0f,
                0.46350446f,     0.88389223f,     0.76532131f,     0.0f,
               -1.43735918f,    -0.00143735918f,  1.72483102f,     0.0f
            };

        // Forward transform -- input to expected.
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD,
                                                          params);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           4e-7f,
                           __LINE__);

        // Inverse transform -- output back to original.
        OCIO::ConstFixedFunctionOpDataRcPtr funcDataInv
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::REC2100_SURROUND_INV,
                                                          params);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcDataInv,
                           3e-7f,
                           __LINE__);
    }
    {
        OCIO::FixedFunctionOpData::Params params = { 1.2 };

        float output_32f[num_samples * 4];
        memcpy(&output_32f[0], &input_32f[0], sizeof(float)*num_samples * 4);

        const float expected_32f[num_samples*4] = {
                1.331310281667e-05f,  3.803743661907e-06f, 2.218850469446e-05f, 0.1f,
                0.059115925805f,      0.010748350146f,     0.021496700293f,     0.5f,
                0.636785774786f,      0.457409500198f,     0.726473912080f,     1.0f,
                0.401647721515f,      0.765932864285f,     0.663185772735f,     0.0f,
               -7.190495367684e-01f, -7.190495367684e-04f, 8.628594441221e-01f, 0.0f
            };

        // Forward transform -- input to expected.
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD,
                                                          params);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           2e-7f,
                           __LINE__);

        // Inverse transform -- output back to original.
        OCIO::ConstFixedFunctionOpDataRcPtr funcDataInv
            = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::REC2100_SURROUND_INV,
                                                          params);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcDataInv,
                           2e-7f,
                           __LINE__);
    }
}

OCIO_ADD_TEST(FixedFunctionOpCPU, RGB_TO_HSV)
{
    const std::vector<float> hsvFrame {
         3.f/12.f,  0.80f,  2.50f,  0.50f,      // val > 1
        11.f/12.f,  1.20f,  2.50f,  1.00f,      // sat > 1
        15.f/24.f,  0.80f, -2.00f,  0.25f,      // val < 0
        19.f/24.f,  1.50f, -0.40f,  0.25f,      // sat > 1, val < 0
       -89.f/24.f,  0.50f,  0.40f,  2.00f,      // under-range hue
        81.f/24.f,  1.50f, -0.40f, -0.25f,      // over-range hue, sat > 1, val < 0
        81.f/24.f, -0.50f,  0.40f,  0.00f,      // sat < 0
          0.5000f,  2.50f,  0.04f,  0.00f, };   // sat > 2

    const std::vector<float> rgbFrame {
        1.500f,   2.500f,   0.500f,   0.50f,
        3.125f,  -0.625f,   1.250f,   1.00f,
       -5.f/3.f, -4.f/3.f, -1.f/3.f,  0.25f,
        0.100f,  -0.800f,   0.400f,   0.25f,
        0.250f,   0.400f,   0.200f,   2.00f,
       -0.800f,   0.400f,  -0.500f,  -0.25f,
        0.400f,   0.400f,   0.400f,   0.00f,
       -39.96f,   40.00f,   40.00f,   0.00f, };

    OCIO::ConstFixedFunctionOpDataRcPtr dataFwd 
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::RGB_TO_HSV);

    const int numRGB = 4;   // only the first 4 are relevant for RGB --> HSV
    std::vector<float> img = rgbFrame;
    ApplyFixedFunction(&img[0], &hsvFrame[0], numRGB, dataFwd, 1e-6f, __LINE__);

    OCIO::ConstFixedFunctionOpDataRcPtr dataFInv
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::HSV_TO_RGB);

    const int numHSV = 7;   // not using the last one as it requires a looser tolerance
    img = hsvFrame;
    ApplyFixedFunction(&img[0], &rgbFrame[0], numHSV, dataFInv, 1e-6f, __LINE__);
}

OCIO_ADD_TEST(FixedFunctionOpCPU, XYZ_TO_xyY)
{
    const std::vector<float> inputFrame {
        3600.0f / 4095.0f,  250.0f / 4095.0f,  900.0f / 4095.0f, 2000.0f / 4095.0f,
         400.0f / 4095.0f, 3000.0f / 4095.0f, 4000.0f / 4095.0f, 4095.0f / 4095.0f };

    const std::vector<float> outputFrame {
        49669.0f / 65535.0f,  3449.0f / 65535.0f,  4001.0f / 65535.0f, 32007.0f / 65535.0f,
         3542.0f / 65535.0f, 26568.0f / 65535.0f, 48011.0f / 65535.0f, 65535.0f / 65535.0f };

    std::vector<float> img = inputFrame;

    OCIO::ConstFixedFunctionOpDataRcPtr dataFwd 
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::XYZ_TO_xyY);

    ApplyFixedFunction(&img[0], &outputFrame[0], 2, dataFwd, 1e-5f, __LINE__);

    OCIO::ConstFixedFunctionOpDataRcPtr dataFInv
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::xyY_TO_XYZ);

    img = outputFrame;
    ApplyFixedFunction(&img[0], &inputFrame[0], 2, dataFInv, 1e-4f, __LINE__);
}

OCIO_ADD_TEST(FixedFunctionOpCPU, XYZ_TO_uvY)
{
    const std::vector<float> inputFrame {
        3600.0f / 4095.0f,  350.0f / 4095.0f, 1900.0f / 4095.0f, 2000.0f / 4095.0f,
         400.0f / 4095.0f, 3000.0f / 4095.0f, 4000.0f / 4095.0f, 4095.0f / 4095.0f };

    const std::vector<float> outputFrame {
        64859.0f / 65535.0f, 14188.0f / 65535.0f,  5601.0f / 65535.0f, 32007.0f / 65535.0f,
         1827.0f / 65535.0f, 30827.0f / 65535.0f, 48011.0f / 65535.0f, 65535.0f / 65535.0f };

    std::vector<float> img = inputFrame;

    OCIO::ConstFixedFunctionOpDataRcPtr dataFwd 
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::XYZ_TO_uvY);

    ApplyFixedFunction(&img[0], &outputFrame[0], 2, dataFwd, 1e-5f, __LINE__);

    OCIO::ConstFixedFunctionOpDataRcPtr dataFInv
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::uvY_TO_XYZ);

    img = outputFrame;
    ApplyFixedFunction(&img[0], &inputFrame[0], 2, dataFInv, 1e-4f, __LINE__);
}

OCIO_ADD_TEST(FixedFunctionOpCPU, XYZ_TO_LUV)
{
    const std::vector<float> inputFrame {
        3600.0f / 4095.0f, 3500.0f / 4095.0f, 1900.0f / 4095.0f, 2000.0f / 4095.0f,
          50.0f / 4095.0f,   30.0f / 4095.0f,   19.0f / 4095.0f, 4095.0f / 4095.0f }; // below the L* break

    const std::vector<float> outputFrame {
        61659.0f / 65535.0f, 28199.0f / 65535.0f, 33176.0f / 65535.0f, 32007.0f / 65535.0f,
         4337.0f / 65535.0f,  9090.0f / 65535.0f,   926.0f / 65535.0f, 65535.0f / 65535.0f };

    std::vector<float> img = inputFrame;

    OCIO::ConstFixedFunctionOpDataRcPtr dataFwd 
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::XYZ_TO_LUV);

    ApplyFixedFunction(&img[0], &outputFrame[0], 2, dataFwd, 1e-5f, __LINE__);

    OCIO::ConstFixedFunctionOpDataRcPtr dataFInv
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::LUV_TO_XYZ);

    img = outputFrame;
    ApplyFixedFunction(&img[0], &inputFrame[0], 2, dataFInv, 1e-5f, __LINE__);
}
