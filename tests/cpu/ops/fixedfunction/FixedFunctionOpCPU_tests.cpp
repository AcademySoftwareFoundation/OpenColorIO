// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>

#include "ops/fixedfunction/FixedFunctionOpCPU.cpp"

#include "UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{
void ApplyFixedFunction(float * input_32f, 
                        const float * expected_32f, 
                        unsigned numSamples,
                        OCIO::ConstFixedFunctionOpDataRcPtr & fnData, 
                        float errorThreshold)
{
    OCIO::ConstOpCPURcPtr op;
    OCIO_CHECK_NO_THROW(op = OCIO::GetFixedFunctionCPURenderer(fnData));
    OCIO_CHECK_NO_THROW(op->apply(input_32f, input_32f, numSamples));

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
            errorMsg << " - Values: " << input_32f[idx] << " and: " << expected_32f[idx];
            errorMsg << " - Threshold: " << errorThreshold;
            OCIO_CHECK_ASSERT_MESSAGE(0, errorMsg.str());
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

    OCIO::FixedFunctionOpData::Params params;

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                          OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-7f);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                          OCIO::FixedFunctionOpData::ACES_RED_MOD_03_INV);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcData,
                           1e-7f);
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

    OCIO::FixedFunctionOpData::Params params;

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                          OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-7f);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                          OCIO::FixedFunctionOpData::ACES_RED_MOD_10_INV);

        float adjusted_input_32f[num_samples*4];
        memcpy(&adjusted_input_32f[0], &input_32f[0], sizeof(float)*num_samples*4);

        // Note: There is a known issue in ACES 1.0 where the red modifier inverse algorithm 
        // is not quite exact.  Hence the aim values here aren't quite the same as the input.
        adjusted_input_32f[0] = 0.89146208f;
        adjusted_input_32f[4] = 0.96750682f;
        adjusted_input_32f[8] = 0.88518190f;


        ApplyFixedFunction(&output_32f[0], &adjusted_input_32f[0], num_samples, 
                           funcData,
                           1e-7f);
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

    OCIO::FixedFunctionOpData::Params params;

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                          OCIO::FixedFunctionOpData::ACES_GLOW_03_FWD);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-7f);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                          OCIO::FixedFunctionOpData::ACES_GLOW_03_INV);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcData,
                           1e-7f);
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

    OCIO::FixedFunctionOpData::Params params;

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                          OCIO::FixedFunctionOpData::ACES_GLOW_10_FWD);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-7f);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                          OCIO::FixedFunctionOpData::ACES_GLOW_10_INV);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcData,
                           1e-7f);
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

    OCIO::FixedFunctionOpData::Params params;

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                          OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD);

        ApplyFixedFunction(&output_32f[0], &expected_32f[0], num_samples, 
                           funcData,
                           1e-7f);
    }

    {
        OCIO::ConstFixedFunctionOpDataRcPtr funcData 
            = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                          OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV);

        ApplyFixedFunction(&output_32f[0], &input_32f[0], num_samples, 
                           funcData,
                           1e-7f);
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

    const float expected_32f[num_samples*4] = {
            0.21779590f,  0.03959925f, 0.07919850f, 0.5f,
            0.80029451f,  0.57485944f, 0.91301214f, 1.0f,
            0.46350446f,  0.88389223f, 0.76532131f, 0.0f,
           -7.58577776f, -0.00758577f, 9.10293388f, 0.0f,
        };

    OCIO::FixedFunctionOpData::Params params = { 0.78 };
    OCIO::ConstFixedFunctionOpDataRcPtr funcData 
        = std::make_shared<OCIO::FixedFunctionOpData>(params, 
                                                      OCIO::FixedFunctionOpData::REC2100_SURROUND);

    ApplyFixedFunction(&input_32f[0], &expected_32f[0], num_samples, 
                       funcData,
                       1e-7f);
}   

