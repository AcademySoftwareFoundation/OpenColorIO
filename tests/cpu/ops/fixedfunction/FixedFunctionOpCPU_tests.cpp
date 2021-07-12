// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>

#include "ops/fixedfunction/FixedFunctionOpCPU.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

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

OCIO_ADD_TEST(FixedFunctionOpCPU, rec2100_surround)
{
    const unsigned num_samples = 4;

    float input_32f[num_samples*4] = {
            0.11f,  0.02f,  0.04f, 0.5f,
            0.71f,  0.51f,  0.81f, 1.0f,
            0.43f,  0.82f,  0.71f, 0.0f,
           -1.0f,  -0.001f, 1.2f,  0.0f
        };

    float input2_32f[num_samples * 4];
    memcpy(&input2_32f[0], &input_32f[0], sizeof(float)*num_samples * 4);

    const float expected_32f[num_samples*4] = {
            0.21779590f,  0.03959925f, 0.07919850f, 0.5f,
            0.80029451f,  0.57485944f, 0.91301214f, 1.0f,
            0.46350446f,  0.88389223f, 0.76532131f, 0.0f,
           -7.58577776f, -0.00758577f, 9.10293388f, 0.0f,
        };

    OCIO::FixedFunctionOpData::Params params = { 0.78 };
    OCIO::ConstFixedFunctionOpDataRcPtr funcData 
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::REC2100_SURROUND_FWD,
                                                      params);

    ApplyFixedFunction(&input_32f[0], &expected_32f[0], num_samples, 
                       funcData,
                       1e-7f,
                       __LINE__);

    OCIO::FixedFunctionOpData::Params params_inv = { 1 / 0.78 };
    OCIO::ConstFixedFunctionOpDataRcPtr funcData2 
        = std::make_shared<OCIO::FixedFunctionOpData>(OCIO::FixedFunctionOpData::REC2100_SURROUND_INV,
                                                      params_inv);

    ApplyFixedFunction(&input2_32f[0], &expected_32f[0], num_samples, 
                       funcData2,
                       1e-7f,
                       __LINE__);
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
