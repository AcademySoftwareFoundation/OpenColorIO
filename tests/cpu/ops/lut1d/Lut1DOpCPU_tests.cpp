// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/lut1d/Lut1DOpCPU.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(GamutMapUtil, order3_test)
{
    const float posinf = std::numeric_limits<float>::infinity();
    const float qnan = std::numeric_limits<float>::quiet_NaN();

    // { A, NaN, B } with A > B test (used to be a crash).
    {
        const float RGB[] = { 65504.f, -qnan, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 2);
        OCIO_CHECK_EQUAL(mid, 1);
        OCIO_CHECK_EQUAL(min, 0);
    }
    // Triple NaN test.
    {
    const float RGB[] = { qnan, qnan, -qnan };
    int min, mid, max;
    OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
    OCIO_CHECK_EQUAL(max, 2);
    OCIO_CHECK_EQUAL(mid, 1);
    OCIO_CHECK_EQUAL(min, 0);
    }
    // -Inf test.
    {
        const float RGB[] = { 65504.f, -posinf, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 0);
        OCIO_CHECK_EQUAL(mid, 2);
        OCIO_CHECK_EQUAL(min, 1);
    }
    // Inf test.
    {
        const float RGB[] = { 0.f, posinf, -65504.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 1);
        OCIO_CHECK_EQUAL(mid, 0);
        OCIO_CHECK_EQUAL(min, 2);
    }
    // Double Inf test.
    {
        const float RGB[] = { posinf, posinf, -65504.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 1);
        OCIO_CHECK_EQUAL(mid, 0);
        OCIO_CHECK_EQUAL(min, 2);
    }

    // Equal values.
    {
        const float RGB[] = { 0.f, 0.f, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        // In this case we only really care that they are distinct and in [0,2]
        // so this test could be changed (it is ok, but overly restrictive).
        OCIO_CHECK_EQUAL(max, 2);
        OCIO_CHECK_EQUAL(mid, 1);
        OCIO_CHECK_EQUAL(min, 0);
    }

    // Now test the six typical possibilities.
    {
        const float RGB[] = { 3.f, 2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 0);
        OCIO_CHECK_EQUAL(mid, 1);
        OCIO_CHECK_EQUAL(min, 2);
    }
    {
        const float RGB[] = { -3.f, -2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 2);
        OCIO_CHECK_EQUAL(mid, 1);
        OCIO_CHECK_EQUAL(min, 0);
    }
    {
        const float RGB[] = { -3.f, 2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 1);
        OCIO_CHECK_EQUAL(mid, 2);
        OCIO_CHECK_EQUAL(min, 0);
    }
    {
        const float RGB[] = { -0.3f, 2.f, -1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 1);
        OCIO_CHECK_EQUAL(mid, 0);
        OCIO_CHECK_EQUAL(min, 2);
    }
    {
        const float RGB[] = { 3.f, -2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 0);
        OCIO_CHECK_EQUAL(mid, 2);
        OCIO_CHECK_EQUAL(min, 1);
    }
    {
        const float RGB[] = { 3.f, -2.f, 10.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 2);
        OCIO_CHECK_EQUAL(mid, 0);
        OCIO_CHECK_EQUAL(min, 1);
    }

}

OCIO_ADD_TEST(Lut1DRenderer, nan_test)
{
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(8);

    float * values = &lut->getArray().getValues()[0];

    values[0]  = 0.0f;      values[1]  = 0.0f;      values[2]  = 0.002333f;
    values[3]  = 0.0f;      values[4]  = 0.291341f; values[5]  = 0.015624f;
    values[6]  = 0.106521f; values[7]  = 0.334331f; values[8]  = 0.462431f;
    values[9]  = 0.515851f; values[10] = 0.474151f; values[11] = 0.624611f;
    values[12] = 0.658791f; values[13] = 0.527381f; values[14] = 0.685071f;
    values[15] = 0.908501f; values[16] = 0.707951f; values[17] = 0.886331f;
    values[18] = 0.926671f; values[19] = 0.846431f; values[20] = 1.0f;
    values[21] = 1.0f;      values[22] = 1.0f;      values[23] = 1.0f;

    OCIO::ConstLut1DOpDataRcPtr lutConst = lut;
    OCIO::ConstOpCPURcPtr renderer;
    OCIO_CHECK_NO_THROW(renderer = OCIO::GetLut1DRenderer(lutConst,
                                                          OCIO::BIT_DEPTH_F32,
                                                          OCIO::BIT_DEPTH_F32));

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();

    float pixels[24] = { qnan, 0.5f, 0.3f, -0.2f,
                         0.5f, qnan, 0.3f, 0.2f, 
                         0.5f, 0.3f, qnan, 1.2f,
                         0.5f, 0.3f, 0.2f, qnan,
                         inf,  inf,  inf,  inf,
                         -inf, -inf, -inf, -inf };

    renderer->apply(pixels, pixels, 6);

    OCIO_CHECK_CLOSE(pixels[0], values[0], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[5], values[1], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[10], values[2], 1e-7f);
    OCIO_CHECK_ASSERT(OCIO::IsNan(pixels[15]));
    OCIO_CHECK_CLOSE(pixels[16], values[21], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[17], values[22], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[18], values[23], 1e-7f);
    OCIO_CHECK_EQUAL(pixels[19], inf);
    OCIO_CHECK_CLOSE(pixels[20], values[0], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[21], values[1], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[22], values[2], 1e-7f);
    OCIO_CHECK_EQUAL(pixels[23], -inf);
}

OCIO_ADD_TEST(Lut1DRenderer, nan_half_test)
{
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(
        OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE, 65536, false);

    float * values = &lut->getArray().getValues()[0];

    // Changed values for nan input.
    constexpr int nanIdRed = 32256 * 3;
    values[nanIdRed] = -1.0f;
    values[nanIdRed + 1] = -2.0f;
    values[nanIdRed + 2] = -3.0f;

    OCIO::ConstLut1DOpDataRcPtr lutConst = lut;
    OCIO::ConstOpCPURcPtr renderer;
    OCIO_CHECK_NO_THROW(renderer = OCIO::GetLut1DRenderer(lutConst,
                                                          OCIO::BIT_DEPTH_F32,
                                                          OCIO::BIT_DEPTH_F32));

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    float pixels[16] = { qnan, 0.5f, 0.3f, -0.2f,
                         0.5f, qnan, 0.3f, 0.2f,
                         0.5f, 0.3f, qnan, 1.2f,
                         0.5f, 0.3f, 0.2f, qnan };

    renderer->apply(pixels, pixels, 4);

    // This verifies that a half-domain Lut1D can map NaNs to whatever the LUT author wants.
    // In this test, a different value for R, G, and B.

    OCIO_CHECK_CLOSE(pixels[0], values[nanIdRed], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[5], values[nanIdRed + 1], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[10], values[nanIdRed + 2], 1e-7f);
    OCIO_CHECK_ASSERT(OCIO::IsNan(pixels[15]));
}

namespace
{
OCIO::ConstLut1DOpDataRcPtr FastFromInverse(OCIO::Lut1DOpDataRcPtr & invLutData, unsigned line)
{
    OCIO_CHECK_NO_THROW_FROM(invLutData->validate(), line);
    OCIO_CHECK_NO_THROW_FROM(invLutData->finalize(), line);
    OCIO::ConstLut1DOpDataRcPtr constInvLutData = invLutData;
    OCIO::ConstLut1DOpDataRcPtr fastInvLutData;
    OCIO_CHECK_NO_THROW_FROM(fastInvLutData = OCIO::MakeFastLut1DFromInverse(constInvLutData),
                             line);
    return fastInvLutData;
}
}

OCIO_ADD_TEST(Lut1DRenderer, bit_depth_support)
{
    // Unit test to validate the pixel bit depth processing with the 1D LUT.

    // Note: Copy & paste of logtolin_8to8.lut

    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(256);

    OCIO::Array::Values & vals = lutData->getArray().getValues();

    static const std::vector<float> lutValues = { 
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         1.0f,          1.0f,          1.0f,
         1.0f,          1.0f,          1.0f,
         2.0f,          2.0f,          2.0f,
         2.0f,          2.0f,          2.0f,
         3.0f,          3.0f,          3.0f,
         3.0f,          3.0f,          3.0f,
         4.0f,          4.0f,          4.0f,
         5.0f,          5.0f,          5.0f,
         5.0f,          5.0f,          5.0f,
         6.0f,          6.0f,          6.0f,
         6.0f,          6.0f,          6.0f,
         7.0f,          7.0f,          7.0f,
         8.0f,          8.0f,          8.0f,
         8.0f,          8.0f,          8.0f,
         9.0f,          9.0f,          9.0f,
        10.0f,         10.0f,         10.0f,
        10.0f,         10.0f,         10.0f,
        11.0f,         11.0f,         11.0f,
        12.0f,         12.0f,         12.0f,
        12.0f,         12.0f,         12.0f,
        13.0f,         13.0f,         13.0f,
        14.0f,         14.0f,         14.0f,
        15.0f,         15.0f,         15.0f,
        15.0f,         15.0f,         15.0f,
        16.0f,         16.0f,         16.0f,
        17.0f,         17.0f,         17.0f,
        18.0f,         18.0f,         18.0f,
        18.0f,         18.0f,         18.0f,
        19.0f,         19.0f,         19.0f,
        20.0f,         20.0f,         20.0f,
        21.0f,         21.0f,         21.0f,
        22.0f,         22.0f,         22.0f,
        22.0f,         22.0f,         22.0f,
        23.0f,         23.0f,         23.0f,
        24.0f,         24.0f,         24.0f,
        25.0f,         25.0f,         25.0f,
        26.0f,         26.0f,         26.0f,
        27.0f,         27.0f,         27.0f,
        28.0f,         28.0f,         28.0f,
        29.0f,         29.0f,         29.0f,
        30.0f,         30.0f,         30.0f,
        30.0f,         30.0f,         30.0f,
        31.0f,         31.0f,         31.0f,
        32.0f,         32.0f,         32.0f,
        33.0f,         33.0f,         33.0f,
        34.0f,         34.0f,         34.0f,
        35.0f,         35.0f,         35.0f,
        36.0f,         36.0f,         36.0f,
        37.0f,         37.0f,         37.0f,
        39.0f,         39.0f,         39.0f,
        40.0f,         40.0f,         40.0f,
        41.0f,         41.0f,         41.0f,
        42.0f,         42.0f,         42.0f,
        43.0f,         43.0f,         43.0f,
        44.0f,         44.0f,         44.0f,
        45.0f,         45.0f,         45.0f,
        46.0f,         46.0f,         46.0f,
        48.0f,         48.0f,         48.0f,
        49.0f,         49.0f,         49.0f,
        50.0f,         50.0f,         50.0f,
        51.0f,         51.0f,         51.0f,
        52.0f,         52.0f,         52.0f,
        54.0f,         54.0f,         54.0f,
        55.0f,         55.0f,         55.0f,
        56.0f,         56.0f,         56.0f,
        58.0f,         58.0f,         58.0f,
        59.0f,         59.0f,         59.0f,
        60.0f,         60.0f,         60.0f,
        62.0f,         62.0f,         62.0f,
        63.0f,         63.0f,         63.0f,
        64.0f,         64.0f,         64.0f,
        66.0f,         66.0f,         66.0f,
        67.0f,         67.0f,         67.0f,
        69.0f,         69.0f,         69.0f,
        70.0f,         70.0f,         70.0f,
        72.0f,         72.0f,         72.0f,
        73.0f,         73.0f,         73.0f,
        75.0f,         75.0f,         75.0f,
        76.0f,         76.0f,         76.0f,
        78.0f,         78.0f,         78.0f,
        80.0f,         80.0f,         80.0f,
        81.0f,         81.0f,         81.0f,
        83.0f,         83.0f,         83.0f,
        85.0f,         85.0f,         85.0f,
        86.0f,         86.0f,         86.0f,
        88.0f,         88.0f,         88.0f,
        90.0f,         90.0f,         90.0f,
        92.0f,         92.0f,         92.0f,
        94.0f,         94.0f,         94.0f,
        95.0f,         95.0f,         95.0f,
        97.0f,         97.0f,         97.0f,
        99.0f,         99.0f,         99.0f,
       101.0f,        101.0f,        101.0f,
       103.0f,        103.0f,        103.0f,
       105.0f,        105.0f,        105.0f,
       107.0f,        107.0f,        107.0f,
       109.0f,        109.0f,        109.0f,
       111.0f,        111.0f,        111.0f,
       113.0f,        113.0f,        113.0f,
       115.0f,        115.0f,        115.0f,
       117.0f,        117.0f,        117.0f,
       120.0f,        120.0f,        120.0f,
       122.0f,        122.0f,        122.0f,
       124.0f,        124.0f,        124.0f,
       126.0f,        126.0f,        126.0f,
       129.0f,        129.0f,        129.0f,
       131.0f,        131.0f,        131.0f,
       133.0f,        133.0f,        133.0f,
       136.0f,        136.0f,        136.0f,
       138.0f,        138.0f,        138.0f,
       140.0f,        140.0f,        140.0f,
       143.0f,        143.0f,        143.0f,
       145.0f,        145.0f,        145.0f,
       148.0f,        148.0f,        148.0f,
       151.0f,        151.0f,        151.0f,
       153.0f,        153.0f,        153.0f,
       156.0f,        156.0f,        156.0f,
       159.0f,        159.0f,        159.0f,
       161.0f,        161.0f,        161.0f,
       164.0f,        164.0f,        164.0f,
       167.0f,        167.0f,        167.0f,
       170.0f,        170.0f,        170.0f,
       173.0f,        173.0f,        173.0f,
       176.0f,        176.0f,        176.0f,
       179.0f,        179.0f,        179.0f,
       182.0f,        182.0f,        182.0f,
       185.0f,        185.0f,        185.0f,
       188.0f,        188.0f,        188.0f,
       191.0f,        191.0f,        191.0f,
       194.0f,        194.0f,        194.0f,
       198.0f,        198.0f,        198.0f,
       201.0f,        201.0f,        201.0f,
       204.0f,        204.0f,        204.0f,
       208.0f,        208.0f,        208.0f,
       211.0f,        211.0f,        211.0f,
       214.0f,        214.0f,        214.0f,
       218.0f,        218.0f,        218.0f,
       222.0f,        222.0f,        222.0f,
       225.0f,        225.0f,        225.0f,
       229.0f,        229.0f,        229.0f,
       233.0f,        233.0f,        233.0f,
       236.0f,        236.0f,        236.0f,
       240.0f,        240.0f,        240.0f,
       244.0f,        244.0f,        244.0f,
       248.0f,        248.0f,        248.0f,
       252.0f,        252.0f,        252.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f   };

    vals = lutValues;
    lutData->getArray().scale(1.0f / 255.0f);
    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;

    constexpr unsigned NB_PIXELS = 4;

    const std::vector<uint8_t> uint8_inImg = { 
          0,   1,   2,   0,
         50,  51,  52, 255,
        150, 151, 152,   0,
        230, 240, 250, 255  };

    const std::vector<uint16_t> uint16_outImg = {
            0,     0,     0,     0,
         4369,  4626,  4626, 65535,
        46774, 47545, 48316,     0,
        65535, 65535, 65535, 65535 };

    // Processing from UINT8 to UINT8.
    {
        OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_UINT8,
                                                           OCIO::BIT_DEPTH_UINT8));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_UINT8,
                                                                     OCIO::BIT_DEPTH_UINT8>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(isLookup);

        std::vector<uint8_t> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&uint8_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0],   0);
        OCIO_CHECK_EQUAL(outImg[ 1],   0);
        OCIO_CHECK_EQUAL(outImg[ 2],   0);
        OCIO_CHECK_EQUAL(outImg[ 3],   0);

        OCIO_CHECK_EQUAL(outImg[ 4],  17);
        OCIO_CHECK_EQUAL(outImg[ 5],  18);
        OCIO_CHECK_EQUAL(outImg[ 6],  18);
        OCIO_CHECK_EQUAL(outImg[ 7], 255);

        OCIO_CHECK_EQUAL(outImg[ 8], 182);
        OCIO_CHECK_EQUAL(outImg[ 9], 185);
        OCIO_CHECK_EQUAL(outImg[10], 188);
        OCIO_CHECK_EQUAL(outImg[11],   0);

        OCIO_CHECK_EQUAL(outImg[12], 255);
        OCIO_CHECK_EQUAL(outImg[13], 255);
        OCIO_CHECK_EQUAL(outImg[14], 255);
        OCIO_CHECK_EQUAL(outImg[15], 255);
    }

    // Processing from UINT8 to UINT8, using the inverse LUT.
    {
        OCIO::Lut1DOpDataRcPtr lutInvData = lutData->inverse();
        OCIO::ConstLut1DOpDataRcPtr constInvLut = FastFromInverse(lutInvData, __LINE__);

        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_UINT8,
                                                           OCIO::BIT_DEPTH_UINT8));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_UINT8,
                                                                     OCIO::BIT_DEPTH_UINT8>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(isLookup);

        std::vector<uint8_t> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&uint8_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0],  24);
        OCIO_CHECK_EQUAL(outImg[ 1],  25);
        OCIO_CHECK_EQUAL(outImg[ 2],  27);
        OCIO_CHECK_EQUAL(outImg[ 3],   0);

        OCIO_CHECK_EQUAL(outImg[ 4],  84);
        OCIO_CHECK_EQUAL(outImg[ 5],  85);
        OCIO_CHECK_EQUAL(outImg[ 6],  86);
        OCIO_CHECK_EQUAL(outImg[ 7], 255);

        OCIO_CHECK_EQUAL(outImg[ 8], 139);
        OCIO_CHECK_EQUAL(outImg[ 9], 139);
        OCIO_CHECK_EQUAL(outImg[10], 140);
        OCIO_CHECK_EQUAL(outImg[11],   0);

        OCIO_CHECK_EQUAL(outImg[12], 164);
        OCIO_CHECK_EQUAL(outImg[13], 167);
        OCIO_CHECK_EQUAL(outImg[14], 170);
        OCIO_CHECK_EQUAL(outImg[15], 255);
    }

    // Processing from UINT8 to UINT16.
    {
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_UINT8,
                                                           OCIO::BIT_DEPTH_UINT16));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_UINT8,
                                                                     OCIO::BIT_DEPTH_UINT16>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(isLookup);

        std::vector<uint16_t> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&uint8_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0], uint16_outImg[ 0]);
        OCIO_CHECK_EQUAL(outImg[ 1], uint16_outImg[ 1]);
        OCIO_CHECK_EQUAL(outImg[ 2], uint16_outImg[ 2]);
        OCIO_CHECK_EQUAL(outImg[ 3], uint16_outImg[ 3]);

        OCIO_CHECK_EQUAL(outImg[ 4], uint16_outImg[ 4]);
        OCIO_CHECK_EQUAL(outImg[ 5], uint16_outImg[ 5]);
        OCIO_CHECK_EQUAL(outImg[ 6], uint16_outImg[ 6]);
        OCIO_CHECK_EQUAL(outImg[ 7], uint16_outImg[ 7]);

        OCIO_CHECK_EQUAL(outImg[ 8], uint16_outImg[ 8]);
        OCIO_CHECK_EQUAL(outImg[ 9], uint16_outImg[ 9]);
        OCIO_CHECK_EQUAL(outImg[10], uint16_outImg[10]);
        OCIO_CHECK_EQUAL(outImg[11], uint16_outImg[11]);

        OCIO_CHECK_EQUAL(outImg[12], uint16_outImg[12]);
        OCIO_CHECK_EQUAL(outImg[13], uint16_outImg[13]);
        OCIO_CHECK_EQUAL(outImg[14], uint16_outImg[14]);
        OCIO_CHECK_EQUAL(outImg[15], uint16_outImg[15]);
    }

    // Processing from UINT8 to F16.
    {
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_UINT8,
                                                           OCIO::BIT_DEPTH_F16));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_UINT8,
                                                                     OCIO::BIT_DEPTH_F16>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(isLookup);

        std::vector<half> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&uint8_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 1], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 2], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 3], 0.0f);

        OCIO_CHECK_CLOSE(outImg[ 4], 0.066650390625f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 5], 0.070617675781f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 6], 0.070617675781f, 1e-6f);
        OCIO_CHECK_EQUAL(outImg[ 7], 1.0f);

        OCIO_CHECK_CLOSE(outImg[ 8], 0.7138671875f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 9], 0.7255859375f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[10], 0.7373046875f, 1e-6f);
        OCIO_CHECK_EQUAL(outImg[11], 0.0f);

        OCIO_CHECK_EQUAL(outImg[12], 1.0f);
        OCIO_CHECK_EQUAL(outImg[13], 1.0f);
        OCIO_CHECK_EQUAL(outImg[14], 1.0f);
        OCIO_CHECK_EQUAL(outImg[15], 1.0f);
    }

    // Processing from UINT8 to F32.
    {
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_UINT8,
                                                           OCIO::BIT_DEPTH_F32));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_UINT8,
                                                                     OCIO::BIT_DEPTH_F32>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(isLookup);

        std::vector<float> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&uint8_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 1], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 2], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 3], 0.0f);

        OCIO_CHECK_CLOSE(outImg[ 4], 0.06666666666666667f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 5], 0.07058823529411765f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 6], 0.07058823529411765f, 1e-6f);
        OCIO_CHECK_EQUAL(outImg[ 7], 1.0f);

        OCIO_CHECK_CLOSE(outImg[ 8], 0.7137254901960784f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 9], 0.7254901960784313f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[10], 0.7372549019607844f, 1e-6f);
        OCIO_CHECK_EQUAL(outImg[11], 0.0f);

        OCIO_CHECK_EQUAL(outImg[12], 1.0f);
        OCIO_CHECK_EQUAL(outImg[13], 1.0f);
        OCIO_CHECK_EQUAL(outImg[14], 1.0f);
        OCIO_CHECK_EQUAL(outImg[15], 1.0f);
    }

    // Use scaled previous input values so previous output values could be 
    // reused (i.e. uint16_outImg) to validate the pixel bit depth processing.

    const std::vector<float> float_inImg = { 
        uint8_inImg[ 0]/255.0f, uint8_inImg[ 1]/255.0f, uint8_inImg[ 2]/255.0f, uint8_inImg[ 3]/255.0f,
        uint8_inImg[ 4]/255.0f, uint8_inImg[ 5]/255.0f, uint8_inImg[ 6]/255.0f, uint8_inImg[ 7]/255.0f,
        uint8_inImg[ 8]/255.0f, uint8_inImg[ 9]/255.0f, uint8_inImg[10]/255.0f, uint8_inImg[11]/255.0f,
        uint8_inImg[12]/255.0f, uint8_inImg[13]/255.0f, uint8_inImg[14]/255.0f, uint8_inImg[15]/255.0f };

    // LUT will be used for interpolation, not look-up.
    {
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_UINT8));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_F32,
                                                                     OCIO::BIT_DEPTH_UINT8>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(!isLookup);

        std::vector<uint8_t> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&float_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0],   0);
        OCIO_CHECK_EQUAL(outImg[ 1],   0);
        OCIO_CHECK_EQUAL(outImg[ 2],   0);
        OCIO_CHECK_EQUAL(outImg[ 3],   0);

        OCIO_CHECK_EQUAL(outImg[ 4],  17);
        OCIO_CHECK_EQUAL(outImg[ 5],  18);
        OCIO_CHECK_EQUAL(outImg[ 6],  18);
        OCIO_CHECK_EQUAL(outImg[ 7], 255);

        OCIO_CHECK_EQUAL(outImg[ 8], 182);
        OCIO_CHECK_EQUAL(outImg[ 9], 185);
        OCIO_CHECK_EQUAL(outImg[10], 188);
        OCIO_CHECK_EQUAL(outImg[11],   0);

        OCIO_CHECK_EQUAL(outImg[12], 255);
        OCIO_CHECK_EQUAL(outImg[13], 255);
        OCIO_CHECK_EQUAL(outImg[14], 255);
        OCIO_CHECK_EQUAL(outImg[15], 255);
    }

    // LUT will be used for interpolation, not look-up.
    {
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_UINT16));
        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_F32,
                                                                     OCIO::BIT_DEPTH_UINT16>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(!isLookup);

        std::vector<uint16_t> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&float_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0], uint16_outImg[ 0]);
        OCIO_CHECK_EQUAL(outImg[ 1], uint16_outImg[ 1]);
        OCIO_CHECK_EQUAL(outImg[ 2], uint16_outImg[ 2]);
        OCIO_CHECK_EQUAL(outImg[ 3], uint16_outImg[ 3]);

        OCIO_CHECK_EQUAL(outImg[ 4], uint16_outImg[ 4]);
        OCIO_CHECK_EQUAL(outImg[ 5], uint16_outImg[ 5]);
        OCIO_CHECK_EQUAL(outImg[ 6], uint16_outImg[ 6]);
        OCIO_CHECK_EQUAL(outImg[ 7], uint16_outImg[ 7]);

        OCIO_CHECK_EQUAL(outImg[ 8], uint16_outImg[ 8]);
        OCIO_CHECK_EQUAL(outImg[ 9], uint16_outImg[ 9]);
        OCIO_CHECK_EQUAL(outImg[10], uint16_outImg[10]);
        OCIO_CHECK_EQUAL(outImg[11], uint16_outImg[11]);

        OCIO_CHECK_EQUAL(outImg[12], uint16_outImg[12]);
        OCIO_CHECK_EQUAL(outImg[13], uint16_outImg[13]);
        OCIO_CHECK_EQUAL(outImg[14], uint16_outImg[14]);
        OCIO_CHECK_EQUAL(outImg[15], uint16_outImg[15]);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, basic)
{
    // By default, this constructor creates an 'identity LUT'.
    OCIO::Lut1DOpDataRcPtr lutData =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 65536, false);

    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());

    const float step = 1.f / ((float)lutData->getArray().getLength() - 1.0f);

    const float inImg[8] = {
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, step, 1.0f };

    const float error = 1e-6f;
    {
        OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32));

        std::vector<float> outImg(2 * 4, 1);
        cpuOp->apply(&inImg[0], &outImg[0], 2);

        OCIO_CHECK_CLOSE(outImg[0], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[1], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[2], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[3], 1.0f, error);

        OCIO_CHECK_CLOSE(outImg[4], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[5], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[6], step, error);
        OCIO_CHECK_CLOSE(outImg[7], 1.0f, error);
    }

    // No more an 'identity LUT 1D'.
    const float arbitraryVal = 0.123456f;

    lutData->getArray()[5] = arbitraryVal;

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());
    OCIO_CHECK_ASSERT(!lutData->isIdentity());
    {
        OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32));

        std::vector<float> outImg(2 * 4, 1);
        cpuOp->apply(&inImg[0], &outImg[0], 2);

        OCIO_CHECK_CLOSE(outImg[0], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[1], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[2], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[3], 1.0f, error);

        OCIO_CHECK_CLOSE(outImg[4], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[5], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[6], arbitraryVal, error);
        OCIO_CHECK_CLOSE(outImg[7], 1.0f, error);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, half)
{
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 65536, false);

    const float step = 1.f / ((float)lutData->getArray().getLength() - 1.0f);

    // No more an 'identity LUT 1D'.
    constexpr float arbitraryVal = 0.123456f;
    lutData->getArray()[5] = arbitraryVal;
    OCIO_CHECK_ASSERT(!lutData->isIdentity());

    const half inImg[8] = {
        0.1f, 0.3f, 0.4f, 1.0f,
        0.0f, 0.9f, step, 0.0f };

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut, OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32));

    std::vector<float> outImg(2 * 4, -1.f);
    cpuOp->apply(&inImg[0], &outImg[0], 2);

    OCIO_CHECK_EQUAL(outImg[0], inImg[0]);
    OCIO_CHECK_EQUAL(outImg[1], inImg[1]);
    OCIO_CHECK_EQUAL(outImg[2], inImg[2]);
    OCIO_CHECK_EQUAL(outImg[3], inImg[3]);

    OCIO_CHECK_EQUAL(outImg[4], inImg[4]);
    OCIO_CHECK_EQUAL(outImg[5], inImg[5]);
    OCIO_CHECK_CLOSE(outImg[6], arbitraryVal, 1e-5f);
    OCIO_CHECK_EQUAL(outImg[7], inImg[7]);
}

OCIO_ADD_TEST(Lut1DRenderer, nan)
{
    // By default, this constructor creates an 'identity LUT'.
    OCIO::Lut1DOpDataRcPtr lutData =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 65536, false);

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32));

    const float step = 1.f / ((float)lutData->getArray().getLength() - 1.0f);

    float myImage[8] = {
        std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f, 1.0f,
                                           0.0f, 0.0f, step, 1.0f };

    std::vector<float> outImg(2 * 4);
    cpuOp->apply(&myImage[0], &outImg[0], 2);

    OCIO_CHECK_EQUAL(outImg[0], 0.0f);
    OCIO_CHECK_EQUAL(outImg[1], 0.0f);
    OCIO_CHECK_EQUAL(outImg[2], 0.0f);
    OCIO_CHECK_EQUAL(outImg[3], 1.0f);

    OCIO_CHECK_EQUAL(outImg[4], 0.0f);
    OCIO_CHECK_EQUAL(outImg[5], 0.0f);
    OCIO_CHECK_EQUAL(outImg[6], step);
    OCIO_CHECK_EQUAL(outImg[7], 1.0f);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_red)
{
    OCIO::Lut1DOpDataRcPtr lutData =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 32, false);

    OCIO::Array::Values & vals = lutData->getArray().getValues();
    const std::vector<float> lutValues = {
            0.f / 1023.f, 0.f, 0.f,
           33.f / 1023.f, 0.f, 0.f,
           66.f / 1023.f, 0.f, 0.f,
           99.f / 1023.f, 0.f, 0.f,
          132.f / 1023.f, 0.f, 0.f,
          165.f / 1023.f, 0.f, 0.f,
          198.f / 1023.f, 0.f, 0.f,
          231.f / 1023.f, 0.f, 0.f,
          264.f / 1023.f, 0.f, 0.f,
          297.f / 1023.f, 0.f, 0.f,
          330.f / 1023.f, 0.f, 0.f,
          363.f / 1023.f, 0.f, 0.f,
          396.f / 1023.f, 0.f, 0.f,
          429.f / 1023.f, 0.f, 0.f,
          462.f / 1023.f, 0.f, 0.f,
          495.f / 1023.f, 0.f, 0.f,
          528.f / 1023.f, 0.f, 0.f,
          561.f / 1023.f, 0.f, 0.f,
          594.f / 1023.f, 0.f, 0.f,
          627.f / 1023.f, 0.f, 0.f,
          660.f / 1023.f, 0.f, 0.f,
          693.f / 1023.f, 0.f, 0.f,
          726.f / 1023.f, 0.f, 0.f,
          759.f / 1023.f, 0.f, 0.f,
          792.f / 1023.f, 0.f, 0.f,
          825.f / 1023.f, 0.f, 0.f,
          858.f / 1023.f, 0.f, 0.f,
          891.f / 1023.f, 0.f, 0.f,
          924.f / 1023.f, 0.f, 0.f,
          957.f / 1023.f, 0.f, 0.f,
          990.f / 1023.f, 0.f, 0.f,
         1023.f / 1023.f, 0.f, 0.f
    };
    vals = lutValues;

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT16));

    constexpr float step = 1.f / 31.f;
    const std::vector<float> inImg = {
        0.f,   0.f,  0.f,  0.f,
        step,  0.f,  0.f,  0.f,
        0.f,  step,  0.f,  0.f,
        0.f,   0.f, step,  0.f,
        step, step, step,  0.f };

    std::vector<uint16_t> outImg(5 * 4, 1);
    cpuOp->apply(&inImg[0], &outImg[0], 5);

    const uint16_t scaledStep = (uint16_t)roundf(step * 65535.0f);

    OCIO_CHECK_EQUAL(outImg[0], 0);
    OCIO_CHECK_EQUAL(outImg[1], 0);
    OCIO_CHECK_EQUAL(outImg[2], 0);
    OCIO_CHECK_EQUAL(outImg[3], 0);

    OCIO_CHECK_EQUAL(outImg[4], scaledStep);
    OCIO_CHECK_EQUAL(outImg[5], 0);
    OCIO_CHECK_EQUAL(outImg[6], 0);
    OCIO_CHECK_EQUAL(outImg[7], 0);

    OCIO_CHECK_EQUAL(outImg[8], 0);
    OCIO_CHECK_EQUAL(outImg[9], 0);
    OCIO_CHECK_EQUAL(outImg[10], 0);
    OCIO_CHECK_EQUAL(outImg[11], 0);

    OCIO_CHECK_EQUAL(outImg[12], 0);
    OCIO_CHECK_EQUAL(outImg[13], 0);
    OCIO_CHECK_EQUAL(outImg[14], 0);
    OCIO_CHECK_EQUAL(outImg[15], 0);

    OCIO_CHECK_EQUAL(outImg[16], scaledStep);
    OCIO_CHECK_EQUAL(outImg[17], 0);
    OCIO_CHECK_EQUAL(outImg[18], 0);
    OCIO_CHECK_EQUAL(outImg[19], 0);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_green)
{
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 32, false);

    OCIO::Array::Values & vals = lutData->getArray().getValues();
    const std::vector<float> lutValues = {
        0.f,    0.f / 1023.f, 0.f,
        0.f,   33.f / 1023.f, 0.f,
        0.f,   66.f / 1023.f, 0.f,
        0.f,   99.f / 1023.f, 0.f,
        0.f,  132.f / 1023.f, 0.f,
        0.f,  165.f / 1023.f, 0.f,
        0.f,  198.f / 1023.f, 0.f,
        0.f,  231.f / 1023.f, 0.f,
        0.f,  264.f / 1023.f, 0.f,
        0.f,  297.f / 1023.f, 0.f,
        0.f,  330.f / 1023.f, 0.f,
        0.f,  363.f / 1023.f, 0.f,
        0.f,  396.f / 1023.f, 0.f,
        0.f,  429.f / 1023.f, 0.f,
        0.f,  462.f / 1023.f, 0.f,
        0.f,  495.f / 1023.f, 0.f,
        0.f,  528.f / 1023.f, 0.f,
        0.f,  561.f / 1023.f, 0.f,
        0.f,  594.f / 1023.f, 0.f,
        0.f,  627.f / 1023.f, 0.f,
        0.f,  660.f / 1023.f, 0.f,
        0.f,  693.f / 1023.f, 0.f,
        0.f,  726.f / 1023.f, 0.f,
        0.f,  759.f / 1023.f, 0.f,
        0.f,  792.f / 1023.f, 0.f,
        0.f,  825.f / 1023.f, 0.f,
        0.f,  858.f / 1023.f, 0.f,
        0.f,  891.f / 1023.f, 0.f,
        0.f,  924.f / 1023.f, 0.f,
        0.f,  957.f / 1023.f, 0.f,
        0.f,  990.f / 1023.f, 0.f,
        0.f, 1023.f / 1023.f, 0.f
    };
    vals = lutValues;

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_UINT16,
                                                       OCIO::BIT_DEPTH_F32));

    constexpr uint16_t step = (uint16_t)(65535 / 31);
    const std::vector<uint16_t> uint16_inImg = {
        0,    0,    0,    0,
        step, 0,    0,    0,
        0,    step, 0,    0,
        0,    0,    step, 0,
        step, step, step, 0 };

    std::vector<float> outImg(5 * 4, -1.f);
    cpuOp->apply(&uint16_inImg[0], &outImg[0], 5);

    constexpr float scaledStep = (float)step / 65535.0f;

    OCIO_CHECK_EQUAL(outImg[0], 0.f);
    OCIO_CHECK_EQUAL(outImg[1], 0.f);
    OCIO_CHECK_EQUAL(outImg[2], 0.f);
    OCIO_CHECK_EQUAL(outImg[3], 0.f);

    OCIO_CHECK_EQUAL(outImg[4], 0.f);
    OCIO_CHECK_EQUAL(outImg[5], 0.f);
    OCIO_CHECK_EQUAL(outImg[6], 0.f);
    OCIO_CHECK_EQUAL(outImg[7], 0.f);

    OCIO_CHECK_EQUAL(outImg[8], 0.f);
    OCIO_CHECK_EQUAL(outImg[9], scaledStep);
    OCIO_CHECK_EQUAL(outImg[10], 0.f);
    OCIO_CHECK_EQUAL(outImg[11], 0.f);

    OCIO_CHECK_EQUAL(outImg[12], 0.f);
    OCIO_CHECK_EQUAL(outImg[13], 0.f);
    OCIO_CHECK_EQUAL(outImg[14], 0.f);
    OCIO_CHECK_EQUAL(outImg[15], 0.f);

    OCIO_CHECK_EQUAL(outImg[16], 0.f);
    OCIO_CHECK_EQUAL(outImg[17], scaledStep);
    OCIO_CHECK_EQUAL(outImg[18], 0.f);
    OCIO_CHECK_EQUAL(outImg[19], 0.f);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_blue)
{
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 32, false);

    OCIO::Array::Values & vals = lutData->getArray().getValues();
    const std::vector<float> lutValues = {
        0.f, 0.f,    0.f / 1023.f,
        0.f, 0.f,   33.f / 1023.f,
        0.f, 0.f,   66.f / 1023.f,
        0.f, 0.f,   99.f / 1023.f,
        0.f, 0.f,  132.f / 1023.f,
        0.f, 0.f,  165.f / 1023.f,
        0.f, 0.f,  198.f / 1023.f,
        0.f, 0.f,  231.f / 1023.f,
        0.f, 0.f,  264.f / 1023.f,
        0.f, 0.f,  297.f / 1023.f,
        0.f, 0.f,  330.f / 1023.f,
        0.f, 0.f,  363.f / 1023.f,
        0.f, 0.f,  396.f / 1023.f,
        0.f, 0.f,  429.f / 1023.f,
        0.f, 0.f,  462.f / 1023.f,
        0.f, 0.f,  495.f / 1023.f,
        0.f, 0.f,  528.f / 1023.f,
        0.f, 0.f,  561.f / 1023.f,
        0.f, 0.f,  594.f / 1023.f,
        0.f, 0.f,  627.f / 1023.f,
        0.f, 0.f,  660.f / 1023.f,
        0.f, 0.f,  693.f / 1023.f,
        0.f, 0.f,  726.f / 1023.f,
        0.f, 0.f,  759.f / 1023.f,
        0.f, 0.f,  792.f / 1023.f,
        0.f, 0.f,  825.f / 1023.f,
        0.f, 0.f,  858.f / 1023.f,
        0.f, 0.f,  891.f / 1023.f,
        0.f, 0.f,  924.f / 1023.f,
        0.f, 0.f,  957.f / 1023.f,
        0.f, 0.f,  990.f / 1023.f,
        0.f, 0.f, 1023.f / 1023.f
    };
    vals = lutValues;

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_UINT16,
                                                       OCIO::BIT_DEPTH_UINT16));

    constexpr uint16_t step = (uint16_t)(65535 / 31);
    const std::vector<uint16_t> uint16_inImg = {
        0,    0,    0,    0,
        step, 0,    0,    0,
        0,    step, 0,    0,
        0,    0,    step, 0,
        step, step, step, 0 };

    std::vector<uint16_t> outImg(5 * 4, 2000);
    cpuOp->apply(&uint16_inImg[0], &outImg[0], 5);

    OCIO_CHECK_EQUAL(outImg[0], 0);
    OCIO_CHECK_EQUAL(outImg[1], 0);
    OCIO_CHECK_EQUAL(outImg[2], 0);
    OCIO_CHECK_EQUAL(outImg[3], 0);

    OCIO_CHECK_EQUAL(outImg[4], 0);
    OCIO_CHECK_EQUAL(outImg[5], 0);
    OCIO_CHECK_EQUAL(outImg[6], 0);
    OCIO_CHECK_EQUAL(outImg[7], 0);

    OCIO_CHECK_EQUAL(outImg[8], 0);
    OCIO_CHECK_EQUAL(outImg[9], 0);
    OCIO_CHECK_EQUAL(outImg[10], 0);
    OCIO_CHECK_EQUAL(outImg[11], 0);

    OCIO_CHECK_EQUAL(outImg[12], 0);
    OCIO_CHECK_EQUAL(outImg[13], 0);
    OCIO_CHECK_EQUAL(outImg[14], step);
    OCIO_CHECK_EQUAL(outImg[15], 0);

    OCIO_CHECK_EQUAL(outImg[16], 0);
    OCIO_CHECK_EQUAL(outImg[17], 0);
    OCIO_CHECK_EQUAL(outImg[18], step);
    OCIO_CHECK_EQUAL(outImg[19], 0);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_special_values)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    const std::string ctfLUT("lut1d_infinity.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));

    // This test should use the "interpolation" renderer path.
    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getDefaultCPUProcessor());

    constexpr float step = 1.f / 65535.f;

    float inImage[] = {
                  0.0f,     0.5f * step,            step, 1.0f,
        3000.0f * step, 32000.0f * step, 65535.0f * step, 1.0f
    };

    std::vector<float> outImage(2 * 4, -12.345f);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], 2, 1, 4);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], 2, 1, 4);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    // -Inf is mapped to -MAX_FLOAT
    const float negmax = -std::numeric_limits<float>::max();

    const float lutElem_1 = -3216.000488281f;
    const float lutElem_3000 = -539.565734863f;

    const float rtol = powf(2.f, -14.f);

    // LUT output bit-depth is 12i so normalize to F32.
    const float outRange = 4095.f;

    OCIO_CHECK_CLOSE(outImage[0], negmax, rtol);
    OCIO_CHECK_CLOSE(outImage[1], (lutElem_1 / outRange + negmax) * 0.5f, rtol);
    OCIO_CHECK_CLOSE(outImage[2], lutElem_1 / outRange, rtol);
    OCIO_CHECK_EQUAL(outImage[3], 1.0f);

    OCIO_CHECK_CLOSE(outImage[4], lutElem_3000 / outRange, rtol);
    OCIO_CHECK_CLOSE(outImage[5], negmax, rtol);
    OCIO_CHECK_CLOSE(outImage[6], negmax, rtol);
    OCIO_CHECK_EQUAL(outImage[7], 1.0f);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_hd_above_half_max)
{
    // Test the processing of half-domain Lut1D for float input values that are greater than
    // HALF_MAX but round down to HALF_MAX.  These are the values 65504 < x < 65520.
    // In other words, half(65519) rounds down to 65504 and half(65520) rounds up to Inf.
    // There was a bug where these values were not processed correctly.

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    const std::string ctfLUT("lut1d_hd_16f_16i_1chan.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));

    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getDefaultCPUProcessor());

    float inImage[] = {
        65505.0f,  65519.0f,  65520.0f, 0.0f,
       -65505.0f, -65519.0f, -65520.0f, 1.0f
    };

    std::vector<float> outImage(2 * 4, -12.345f);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], 2, 1, 4);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], 2, 1, 4);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    static const float rtol = 1e-5f;
    OCIO_CHECK_CLOSE(outImage[0], 0.7785763f, rtol);
    OCIO_CHECK_CLOSE(outImage[1], 0.7785763f, rtol);
    OCIO_CHECK_CLOSE(outImage[2], 0.7785763f, rtol);
    OCIO_CHECK_EQUAL(outImage[3], 0.0f);

    OCIO_CHECK_CLOSE(outImage[4], 0.f, rtol);
    OCIO_CHECK_CLOSE(outImage[5], 0.f, rtol);
    OCIO_CHECK_CLOSE(outImage[6], 0.f, rtol);
    OCIO_CHECK_EQUAL(outImage[7], 1.0f);
}

namespace
{
constexpr OCIO::OptimizationFlags DefaultNoLutInvFast =
    (OCIO::OptimizationFlags)(OCIO::OPTIMIZATION_DEFAULT & ~OCIO::OPTIMIZATION_LUT_INV_FAST);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_hue_adjust)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    const std::string ctfLUT("lut1d_1024_hue_adjust_test.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));

    // This test should use the "lookup" renderer path.
    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_UINT16,
                                                                OCIO::BIT_DEPTH_UINT16,
                                                                DefaultNoLutInvFast));

    constexpr long NB_PIXELS = 2;
    // TODO: use UINT10 when implemented by ImageDesc.
    const uint16_t inImage[NB_PIXELS * 4] = {
        100 * 65535 / 1023, 500 * 65535 / 1023, 800 * 65535 / 1023,  200 * 65535 / 1023,
        400 * 65535 / 1023, 700 * 65535 / 1023, 300 * 65535 / 1023, 1023 * 65535 / 1023 };

    std::vector<uint16_t> outImage(NB_PIXELS * 4, 2000);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], NB_PIXELS, 1, 4,
                                     OCIO::BIT_DEPTH_UINT16,
                                     sizeof(uint16_t),
                                     OCIO::AutoStride,
                                     OCIO::AutoStride);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], NB_PIXELS, 1, 4,
                                     OCIO::BIT_DEPTH_UINT16,
                                     sizeof(uint16_t),
                                     OCIO::AutoStride,
                                     OCIO::AutoStride);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    OCIO_CHECK_EQUAL(outImage[0], 1523);
    OCIO_CHECK_EQUAL(outImage[1], 33883);  // Would be 31583 w/out hue adjust.
    OCIO_CHECK_EQUAL(outImage[2], 58154);
    OCIO_CHECK_EQUAL(outImage[3], 12812);

    OCIO_CHECK_EQUAL(outImage[4], 22319);  // Would be 21710 w/out hue adjust.
    OCIO_CHECK_EQUAL(outImage[5], 50648);
    OCIO_CHECK_EQUAL(outImage[6], 12877);
    OCIO_CHECK_EQUAL(outImage[7], 65535);

    // This test should use the "interpolation" renderer path.
    float inFloatImage[8];
    for (int i = 0; i < 8; ++i)
    {
        inFloatImage[i] = (float)inImage[i] / 65535.f;
    }
    std::fill(outImage.begin(), outImage.end(), 200);
    OCIO::PackedImageDesc srcImgFDesc((void*)&inFloatImage[0], NB_PIXELS, 1, 4,
                                      OCIO::BIT_DEPTH_F32,
                                      sizeof(float),
                                      OCIO::AutoStride,
                                      OCIO::AutoStride);

    OCIO::ConstCPUProcessorRcPtr cpuFwdFast;
    OCIO_CHECK_NO_THROW(cpuFwdFast = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                    OCIO::BIT_DEPTH_UINT16,
                                                                    OCIO::OPTIMIZATION_DEFAULT));

    cpuFwdFast->apply(srcImgFDesc, dstImgDesc);
    OCIO_CHECK_EQUAL(outImage[0], 1523);
    OCIO_CHECK_EQUAL(outImage[1], 33883);  // Would be 31583 w/out hue adjust.
    OCIO_CHECK_EQUAL(outImage[2], 58154);
    OCIO_CHECK_EQUAL(outImage[3], 12812);

    OCIO_CHECK_EQUAL(outImage[4], 22319);  // Would be 21710 w/out hue adjust.
    OCIO_CHECK_EQUAL(outImage[5], 50648);
    OCIO_CHECK_EQUAL(outImage[6], 12877);
    OCIO_CHECK_EQUAL(outImage[7], 65535);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_half_domain_hue_adjust)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    const std::string ctfLUT("lut1d_hd_hue_adjust.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));

    // This test should use the "interpolation" renderer path.
    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getDefaultCPUProcessor());

    const float inImage[] = {
        0.05f, 0.18f, 1.1f, 0.5f,
        2.3f, 0.01f, 0.3f, 1.0f };

    std::vector<float> outImage(2 * 4, -1.f);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], 2, 1, 4);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], 2, 1, 4);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    constexpr float rtol = 1e-6f;
    constexpr float minExpected = 1e-3f;

    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[0], 0.54780269f, rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[1],
                                    9.57448578f, // Would be 5.0 w/out hue adjust.
                                    rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[2], 73.45562744f, rtol, minExpected));
    OCIO_CHECK_EQUAL(outImage[3], 0.5f);

    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[4], 188.087067f, rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[5], 0.0324990489f, rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[6],
                                    23.8472710f, // Would be 11.3372078 w/out hue adjust.
                                    rtol, minExpected));
    OCIO_CHECK_EQUAL(outImage[7], 1.0f);

    // This test should use the "lookup" renderer path.
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_UINT16,
                                                                OCIO::BIT_DEPTH_F32,
                                                                DefaultNoLutInvFast));

    // TODO: Use 10i when ImageDesc handles 10i.
    const uint16_t inImageInt[] = {
        200 * 65535 / 1023, 800 * 65535 / 1023, 500 * 65535 / 1023, 0,
        400 * 65535 / 1023, 100 * 65535 / 1023, 700 * 65535 / 1023, 1023 * 65535 / 1023
    };

    std::fill(outImage.begin(), outImage.end(), -1.f);
    OCIO::PackedImageDesc srcImgIntDesc((void*)&inImageInt[0], 2, 1, 4,
                                        OCIO::BIT_DEPTH_UINT16,
                                        sizeof(uint16_t),
                                        OCIO::AutoStride,
                                        OCIO::AutoStride);
    cpuFwd->apply(srcImgIntDesc, dstImgDesc);

    OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(outImage[0],
                                                  5.72640753f, rtol, minExpected));
    OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(outImage[1],
                                                  46.2259789f, rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[2],
                                    25.9756680f, // Would be 23.6895752 w/out hue adjust.
                                    rtol, minExpected));
    OCIO_CHECK_EQUAL(outImage[3], 0.0f);

    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[4],
                                    20.0804043f, // Would be 17.0063152 w/out hue adjust.
                                    rtol, minExpected));
    OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(outImage[5],
                                                  1.78572845f, rtol, minExpected));
    OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(outImage[6],
                                                  38.3760300f, rtol, minExpected));
    OCIO_CHECK_EQUAL(outImage[7], 1.0f);
}

namespace
{
std::string GetErrorMessage(float expected, float actual)
{
    std::ostringstream oss;
    oss << "expected: " << expected << " != " << "actual: " << actual;
    return oss.str();
}
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_hue_adjust)
{
    const std::string ctfLUT("lut1d_1024_hue_adjust_test.ctf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, ctfLUT, context,
                        OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);

    auto lutData = lut->clone();

    // Currently need to set this to 16i in order for style == FAST to pass.
    // See comment in MakeFastLut1DFromInverse.
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT16);
    lutData->finalize();

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_F32,
                                                       OCIO::BIT_DEPTH_F32));

    const float inImage[] = {
        0.1f,  0.25f, 0.7f,  0.f,
        0.66f, 0.25f, 0.81f, 0.5f,
        // 0.18f, 0.80f, 0.45f, 1.f }; // This one is easier to get correct.
        0.18f, 0.99f, 0.45f, 1.f };    // Setting G way up on the s-curve is harder.
    constexpr long NB_PIXELS = 3;

    std::vector<float> outImage(NB_PIXELS * 4);
    cpuOp->apply(inImage, &outImage[0], NB_PIXELS);

    // Inverse using FAST.
    auto lutDataInv = lutData->inverse();
    OCIO::ConstLut1DOpDataRcPtr constLutInv = FastFromInverse(lutDataInv, __LINE__);
    OCIO::ConstOpCPURcPtr cpuOpInv;
    OCIO_CHECK_NO_THROW(cpuOpInv = OCIO::GetLut1DRenderer(constLutInv,
                                                          OCIO::BIT_DEPTH_F32,
                                                          OCIO::BIT_DEPTH_F32));

    std::vector<float> backImage(NB_PIXELS * 4, -1.f);
    cpuOpInv->apply(&outImage[0], &backImage[0], NB_PIXELS);

    for (long i = 0; i < NB_PIXELS * 4; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 130, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }

    // Repeat with EXACT.
    auto lutDataInv2 = lutData->inverse();

    OCIO_CHECK_NO_THROW(lutDataInv2->validate());
    OCIO_CHECK_NO_THROW(lutDataInv2->finalize());
    OCIO::ConstOpCPURcPtr cpuOpInv2;
    OCIO::ConstLut1DOpDataRcPtr constLutInv2 = lutDataInv2;
    OCIO_CHECK_NO_THROW(cpuOpInv2 = OCIO::GetLut1DRenderer(constLutInv2,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_F32));

    std::fill(backImage.begin(), backImage.end(), -1.f),
    cpuOpInv2->apply(&outImage[0], &backImage[0], NB_PIXELS);

    for (long i = 0; i < NB_PIXELS * 4; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 50, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_identity_half)
{
    // Create the 64k 16f Identity 1D LUT and the test Image.

    // By default, this constructor creates an 'identity lut'.
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE,
                                              65536, false);

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_F16,
                                                       OCIO::BIT_DEPTH_F16));

    constexpr unsigned nbPixels = 65536;
    std::vector<half> myImage(nbPixels * 4);

    unsigned imgCntr = 0;

    for (unsigned i = 0; i < nbPixels; i++)
    {
        half hVal;
        hVal.setBits((unsigned short)i);

        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = 1.0f;
    }

    std::vector<half> outImg(nbPixels * 4);
    cpuOp->apply(&myImage[0], &outImg[0], nbPixels);

    imgCntr = 0;
    for (unsigned i = 0; i < nbPixels; i++, imgCntr += 4)
    {
        half hVal;
        hVal.setBits((unsigned short)i);

        if (hVal.isNan())
        {
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 0], 0.f);
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 1], 0.f);
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 2], 0.f);
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 3], 1.f);
        }
        else if (hVal.isInfinity())
        {
            OCIO_CHECK_ASSERT(outImg[imgCntr + 0].isInfinity());
            OCIO_CHECK_ASSERT(outImg[imgCntr + 1].isInfinity());
            OCIO_CHECK_ASSERT(outImg[imgCntr + 2].isInfinity());
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 3], 1.f);
        }
        else
        {
            OCIO_CHECK_EQUAL(outImg[imgCntr + 0], hVal);
            OCIO_CHECK_EQUAL(outImg[imgCntr + 1], hVal);
            OCIO_CHECK_EQUAL(outImg[imgCntr + 2], hVal);
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 3], 1.f);
        }
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_identity_half_to_int)
{
    // Create the 64k 16f Identity 1D LUT and the test Image.

    // By default, this constructor creates an 'identity lut'.
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE,
                                              65536, false);

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_F16,
                                                       OCIO::BIT_DEPTH_UINT16));

    constexpr unsigned nbPixels = 65536;
    std::vector<half> myImage(nbPixels * 4);

    unsigned imgCntr = 0;

    for (unsigned i = 0; i < nbPixels; i++)
    {
        half hVal;
        hVal.setBits((unsigned short)i);

        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = 1.0f;
    }

    std::vector<uint16_t> outImg(nbPixels * 4);
    cpuOp->apply(&myImage[0], &outImg[0], nbPixels);

    const float scaleFactor = (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT16);

    imgCntr = 0;
    for (unsigned i = 0; i < nbPixels; i++, imgCntr += 4)
    {
        half hVal;
        hVal.setBits((unsigned short)i);
        const float fVal = scaleFactor * (float)hVal;

        const uint16_t val = (uint16_t)OCIO::Clamp(fVal + 0.5f, 0.0f, scaleFactor);

        OCIO_CHECK_EQUAL(val, outImg[imgCntr + 0]);
        OCIO_CHECK_EQUAL(val, outImg[imgCntr + 1]);
        OCIO_CHECK_EQUAL(val, outImg[imgCntr + 2]);
        OCIO_CHECK_EQUAL(1.0f*scaleFactor, outImg[imgCntr + 3]);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_identity_int_to_half)
{
    // Create the 64k 16f Identity 1D LUT and the test Image.

    // By default, this constructor creates an 'identity lut'.
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE,
                                              65536, false);

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_UINT16,
                                                       OCIO::BIT_DEPTH_F16));

    constexpr unsigned nbPixels = 65536;
    std::vector<uint16_t> myImage(nbPixels * 4);

    unsigned imgCntr = 0;

    for (unsigned i = 0; i < nbPixels; i++)
    {
        myImage[imgCntr++] = (uint16_t)i;
        myImage[imgCntr++] = (uint16_t)i;
        myImage[imgCntr++] = (uint16_t)i;
        myImage[imgCntr++] = 1;
    }

    std::vector<half> outImg(nbPixels * 4);
    cpuOp->apply(&myImage[0], &outImg[0], nbPixels);

    const float scaleFactor = 1.f / (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT16);
    const half hScaleFactor = scaleFactor;
    imgCntr = 0;
    constexpr int tol = 1;
    for (unsigned i = 0; i < nbPixels; i++, imgCntr += 4)
    {
        const half hVal = scaleFactor * i;

        OCIO_CHECK_ASSERT(!OCIO::HalfsDiffer(outImg[imgCntr + 0], hVal, tol));
        OCIO_CHECK_ASSERT(!OCIO::HalfsDiffer(outImg[imgCntr + 1], hVal, tol));
        OCIO_CHECK_ASSERT(!OCIO::HalfsDiffer(outImg[imgCntr + 2], hVal, tol));
        OCIO_CHECK_EQUAL(outImg[imgCntr + 3].bits(), hScaleFactor.bits());
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_identity_half_code)
{
    // Create the 64k 16f Identity 1D LUT and the test Image.

    // By default, this constructor creates an 'identity lut'.
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE,
                                              65536, false);

    OCIO_CHECK_NO_THROW(lutData->validate());
    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_F16,
                                                       OCIO::BIT_DEPTH_F16));

    constexpr unsigned nbPixels = 5;
    std::vector<half> myImage(nbPixels * 4);

    myImage[0] = 0.0f;
    myImage[1] = 0.0f;
    myImage[2] = 0.0f;
    myImage[3] = 1.0f;

    // Use values between points to test interpolation code.
    for (unsigned i = 4; i < 4 * nbPixels; i += 4)
    {
        half hVal1, hVal2;
        hVal1.setBits((unsigned short)i);
        hVal2.setBits((unsigned short)(i + 1));
        const float delta = fabs(hVal2 - hVal1);
        const float min = hVal1 < hVal2 ? hVal1 : hVal2;

        myImage[i + 0] = min + (delta / i);
        myImage[i + 1] = min + (delta / i);
        myImage[i + 2] = min + (delta / i);
        myImage[i + 3] = 1.0f;
    }

    std::vector<half> outImg(nbPixels * 4);
    cpuOp->apply(&myImage[0], &outImg[0], nbPixels);

    for (unsigned i = 0; i < 4 * nbPixels; i += 4)
    {
        OCIO_CHECK_EQUAL(outImg[i + 0].bits(), myImage[i + 0].bits());
        OCIO_CHECK_EQUAL(outImg[i + 1].bits(), myImage[i + 1].bits());
        OCIO_CHECK_EQUAL(outImg[i + 2].bits(), myImage[i + 2].bits());
        OCIO_CHECK_EQUAL((float)outImg[i + 3], 1.f);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_identity)
{
    // By default, this constructor creates an 'identity lut'.
    const auto dim = OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_UINT10);

    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, dim, false);

    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    auto invLut = lutData->inverse();
    OCIO::ConstLut1DOpDataRcPtr constLut = FastFromInverse(invLut, __LINE__);

    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_UINT10,
                                                       OCIO::BIT_DEPTH_F32));

    constexpr uint16_t stepui = 700;  // relative to 10i.
    constexpr float step = stepui / 1023.f;

    const uint16_t inImage[] = {
        0,      0,      0,      0,
        stepui, 0,      0,      0,
        0,      stepui, 0,      0,
        0,      0,      stepui, 0,
        stepui, stepui, stepui, 0 };

    float outImage[] = {
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f };

    // Inverse of identity should still be identity.
    const float expected[] = {
        0.f,   0.f,  0.f, 0.f,
        step,  0.f,  0.f, 0.f,
        0.f,  step,  0.f, 0.f,
        0.f,   0.f, step, 0.f,
        step, step, step, 0.f };

    cpuOp->apply(&inImage[0], &outImage[0], 5);

    for (unsigned i = 0; i < 20; ++i)
    {
        OCIO_CHECK_CLOSE(outImage[i], expected[i], 1e-6f);
    }

    // Repeat with EXACT.
    constLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOpExact;
    OCIO_CHECK_NO_THROW(cpuOpExact = OCIO::GetLut1DRenderer(constLut,
                                                            OCIO::BIT_DEPTH_UINT10,
                                                            OCIO::BIT_DEPTH_F32));

    cpuOpExact->apply(&inImage[0], &outImage[0], 5);

    for (unsigned i = 0; i < 20; ++i)
    {
        OCIO_CHECK_CLOSE(outImage[i], expected[i], 1e-6f);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_increasing)
{
    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(32);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    // This is a typical "easy" lut with a simple power function.
    // Linear to 1/2.2 gamma corrected code values.
    OCIO::Array::Values & vals = lutData->getArray().getValues();
    int i = 0;
    vals[i] = vals[i+1] = vals[i+2] =    0.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  215.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  294.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  354.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  403.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  446.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  485.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  520.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  553.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  583.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  612.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  639.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  665.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  689.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  713.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  735.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  757.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  779.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  799.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  819.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  838.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  857.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  875.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  893.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  911.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  928.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  944.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  961.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  977.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  992.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 1008.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 1023.f / 1023.f;

    auto invLut = lutData->inverse();

    OCIO_CHECK_NO_THROW(invLut->validate());
    OCIO_CHECK_NO_THROW(invLut->finalize());

    OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                       OCIO::BIT_DEPTH_UINT10,
                                                       OCIO::BIT_DEPTH_UINT16));

    // The first 2 rows are actual LUT entries, the others are intermediate values.
    const uint16_t inImage[] = {  // scaled to 10i
        (uint16_t)   0, (uint16_t) 215, (uint16_t) 446, (uint16_t)   0,
        (uint16_t) 639, (uint16_t) 944, (uint16_t)1023, (uint16_t) 445, // also test alpha
        (uint16_t)  40, (uint16_t) 190, (uint16_t) 260, (uint16_t) 685,
        (uint16_t) 380, (uint16_t) 540, (uint16_t) 767, (uint16_t)1023,
        (uint16_t) 888, (uint16_t)1000, (uint16_t)1018, (uint16_t)   0 };

    uint16_t outImage[] = {
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1 };

    const uint16_t expected[] = { // scaled to 16i
        (uint16_t)    0, (uint16_t)  2114, (uint16_t)10570, (uint16_t)    0,
        (uint16_t)23254, (uint16_t) 54965, (uint16_t)65535, (uint16_t)28507,
        (uint16_t)  393, (uint16_t)  1868, (uint16_t) 3318, (uint16_t)43882,
        (uint16_t) 7464, (uint16_t) 16079, (uint16_t)34785, (uint16_t)65535,
        (uint16_t)48036, (uint16_t) 62364, (uint16_t)64830, (uint16_t)    0 };

    cpuOp->apply(&inImage[0], &outImage[0], 5);

    for (unsigned i = 0; i < 20; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }

    // Repeat with FAST.
    constInvLut = FastFromInverse(invLut, __LINE__);
    OCIO::ConstOpCPURcPtr cpuOpFast;
    OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_UINT10,
                                                           OCIO::BIT_DEPTH_UINT16));

    cpuOpFast->apply(&inImage[0], &outImage[0], 5);

    for (unsigned i = 0; i < 20; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_decreasing_reversals)
{
    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(12);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    // This is a more "difficult" LUT that is decreasing and has reversals
    // and values outside the typical range.
    OCIO::Array::Values & vals = lutData->getArray().getValues();
    int i = 0;
    vals[i] = vals[i+1] = vals[i+2] =  90.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  90.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 100.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  80.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  70.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  50.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  60.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  70.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  40.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  20.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = -10.f / 255.f; // note: LUT vals may exceed [0,255]
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = -10.f / 255.f;

    auto invLut = lutData->inverse();

    // Render as 32f in depth so we can test negative input vals.
    OCIO_CHECK_NO_THROW(invLut->validate());
    OCIO_CHECK_NO_THROW(invLut->finalize());

    // Default InvStyle should be 'FAST' but we test the 'EXACT' InvStyle first.
    OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                       OCIO::BIT_DEPTH_F32,
                                                       OCIO::BIT_DEPTH_UINT16));

    // Render as 32f in depth so we can test negative input vals.
    const float inScaleFactor = 1.0f / (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT8);

    const float inImage[] = {  // scaled to 32f
        100.f * inScaleFactor, 90.f * inScaleFactor,  85.f * inScaleFactor, 0.f,
         75.f * inScaleFactor, 60.f * inScaleFactor,  50.f * inScaleFactor, 0.f,
         45.f * inScaleFactor, 30.f * inScaleFactor, -10.f * inScaleFactor, 0.f,
        -20.f * inScaleFactor, 75.f * inScaleFactor,  30.f * inScaleFactor, 0.f };

    uint16_t outImage[] = {
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1 };

    uint16_t expected[] = { // scaled to 16i
        (uint16_t)11915, (uint16_t)11915, (uint16_t)14894, (uint16_t)0,
        (uint16_t)20852, (uint16_t)26810, (uint16_t)29789, (uint16_t)0,
        (uint16_t)44683, (uint16_t)50641, (uint16_t)59577, (uint16_t)0,
        (uint16_t)59577, (uint16_t)20852, (uint16_t)50641, (uint16_t)0 };

    cpuOp->apply(&inImage[0], &outImage[0], 4);

    for (unsigned i = 0; i < 16; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }

    // Repeat with FAST.
    constInvLut = FastFromInverse(invLut, __LINE__);
    OCIO::ConstOpCPURcPtr cpuOpFast;
    OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_UINT16));

    cpuOpFast->apply(&inImage[0], &outImage[0], 4);

    // Note: When there are flat spots in the original LUT, the approximate 
    // inverse LUT used in FAST mode has vertical jumps and so one would expect
    // significant differences from EXACT mode (which returns the left edge).
    // Since any value that is within the flat spot would result in the original
    // value on forward interpolation, we may loosen the tolerance for the inverse
    // to the domain of the flat spot.  Also, note that this is only an issue for
    // 32f inDepths since in all other cases EXACT mode is used to compute a LUT
    // that is used for look-up rather than interpolation.
    expected[1] = 11924;
    expected[6] = 38433;

    for (unsigned i = 0; i < 16; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_clamp_to_range)
{
    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(12);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    // Note that the start and end values do not span the full [0,255] range
    // so we test that input values are clamped correctly to this range when
    // the LUT has no flat spots at start or end.
    OCIO::Array::Values & vals = lutData->getArray().getValues();
    int i = 0;
    vals[i] = vals[i+1] = vals[i+2] =  30.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  40.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  60.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  65.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  70.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  50.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  60.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  70.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 100.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 190.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 200.f / 255.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 210.f / 255.f;

    auto invLut = lutData->inverse();

    // Render as 32f in depth so we can test negative input vals.
    OCIO_CHECK_NO_THROW(invLut->validate());
    OCIO_CHECK_NO_THROW(invLut->finalize());

    OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                       OCIO::BIT_DEPTH_F32,
                                                       OCIO::BIT_DEPTH_UINT16));

    const float inScaleFactor = 1.0f / (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT8);

    const float inImage[] = {  // scaled to 32f
          0.f * inScaleFactor,  10.f * inScaleFactor,  30.f * inScaleFactor, 0.f,
         35.f * inScaleFactor, 202.f * inScaleFactor, 210.f * inScaleFactor, 0.f,
        -10.f * inScaleFactor, 255.f * inScaleFactor, 355.f * inScaleFactor, 0.f, };

    uint16_t outImage[] = {
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1 };

    uint16_t expected[] = { // scaled to 16i
        (uint16_t)    0, (uint16_t)    0, (uint16_t)    0, (uint16_t)0,
        (uint16_t) 2979, (uint16_t)60769, (uint16_t)65535, (uint16_t)0,
        (uint16_t)    0, (uint16_t)65535, (uint16_t)65535, (uint16_t)0, };

    cpuOp->apply(&inImage[0], &outImage[0], 3);

    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }

    // Repeat with FAST.
    constInvLut = FastFromInverse(invLut, __LINE__);
    OCIO::ConstOpCPURcPtr cpuOpFast;
    OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_UINT16));

    cpuOpFast->apply(&inImage[0], &outImage[0], 3);

    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_flat_start_or_end)
{
    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(9);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    // This LUT tests that flat spots at beginning and end of various lengths
    // are handled for increasing and decreasing LUTs (it also verifies that
    // LUTs with different R, G, B values are handled correctly).
    OCIO::Array::Values & vals = lutData->getArray().getValues();
    const std::vector<float> lutValues = { // scaled to 32f
        900.f / 1023.f,  70.f / 1023.f,  70.f / 1023.f,
        900.f / 1023.f,  70.f / 1023.f, 120.f / 1023.f,
        900.f / 1023.f, 120.f / 1023.f, 300.f / 1023.f,
        900.f / 1023.f, 300.f / 1023.f, 450.f / 1023.f,
        450.f / 1023.f, 450.f / 1023.f, 900.f / 1023.f,
        300.f / 1023.f, 900.f / 1023.f, 900.f / 1023.f,
        120.f / 1023.f, 900.f / 1023.f, 900.f / 1023.f,
         70.f / 1023.f, 900.f / 1023.f, 900.f / 1023.f,
         70.f / 1023.f, 900.f / 1023.f, 900.f / 1023.f };

    vals = lutValues;

    auto invLut = lutData->inverse();

    OCIO_CHECK_NO_THROW(invLut->validate());
    OCIO_CHECK_NO_THROW(invLut->finalize());

    OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                       OCIO::BIT_DEPTH_UINT10,
                                                       OCIO::BIT_DEPTH_UINT16));

    const uint16_t inImage[] = {  // scaled to 10i
        (uint16_t)1023, (uint16_t)1023, (uint16_t)1023, (uint16_t)0,
        (uint16_t) 900, (uint16_t) 900, (uint16_t) 900, (uint16_t)0,
        (uint16_t) 800, (uint16_t) 800, (uint16_t) 800, (uint16_t)0,
        (uint16_t) 500, (uint16_t) 500, (uint16_t) 500, (uint16_t)0,
        (uint16_t) 450, (uint16_t) 450, (uint16_t) 450, (uint16_t)0,
        (uint16_t) 330, (uint16_t) 330, (uint16_t) 330, (uint16_t)0,
        (uint16_t) 150, (uint16_t) 150, (uint16_t) 150, (uint16_t)0,
        (uint16_t) 120, (uint16_t) 120, (uint16_t) 120, (uint16_t)0,
        (uint16_t)  80, (uint16_t)  80, (uint16_t)  80, (uint16_t)0,
        (uint16_t)  70, (uint16_t)  70, (uint16_t)  70, (uint16_t)0,
        (uint16_t)  60, (uint16_t)  60, (uint16_t)  60, (uint16_t)0,
        (uint16_t)   0, (uint16_t)   0, (uint16_t)   0, (uint16_t)0 };

    uint16_t outImage[] = {
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1 };

    const uint16_t expected[] = { // scaled to 16i
        (uint16_t)24576, (uint16_t)40959, (uint16_t)32768, (uint16_t)0,
        (uint16_t)24576, (uint16_t)40959, (uint16_t)32768, (uint16_t)0,
        (uint16_t)26396, (uint16_t)39139, (uint16_t)30947, (uint16_t)0,
        (uint16_t)31857, (uint16_t)33678, (uint16_t)25486, (uint16_t)0,
        (uint16_t)32768, (uint16_t)32768, (uint16_t)24576, (uint16_t)0,
        (uint16_t)39321, (uint16_t)26214, (uint16_t)18022, (uint16_t)0,
        (uint16_t)47786, (uint16_t)17749, (uint16_t) 9557, (uint16_t)0,
        (uint16_t)49151, (uint16_t)16384, (uint16_t) 8192, (uint16_t)0,
        (uint16_t)55705, (uint16_t) 9830, (uint16_t) 1638, (uint16_t)0,
        (uint16_t)57343, (uint16_t) 8192, (uint16_t)    0, (uint16_t)0,
        (uint16_t)57343, (uint16_t) 8192, (uint16_t)    0, (uint16_t)0,
        (uint16_t)57343, (uint16_t) 8192, (uint16_t)    0, (uint16_t)0 };

    cpuOp->apply(&inImage[0], &outImage[0], 12);

    for (unsigned i = 0; i < 12 * 4; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }

    // Repeat with FAST.
    constInvLut = FastFromInverse(invLut, __LINE__);
    OCIO::ConstOpCPURcPtr cpuOpFast;
    OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_UINT10,
                                                           OCIO::BIT_DEPTH_UINT16));

    cpuOpFast->apply(&inImage[0], &outImage[0], 12);

    for (unsigned i = 0; i < 12 * 4; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }

}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_half_input)
{
    constexpr unsigned dim = 15;
    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(dim);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    // LUT entries.
    const float lutEntries[] = {
        0.00f, 0.05f, 0.10f, 0.15f, 0.20f,
        0.30f, 0.40f, 0.50f, 0.60f, 0.70f,
        0.80f, 0.85f, 0.90f, 0.95f, 1.00f };

    lutData->getArray().resize(dim, 1);
    for (unsigned i = 0; i < dim; ++i)
    {
        lutData->getArray()[i * 3 + 0] = lutEntries[i];
        lutData->getArray()[i * 3 + 1] = lutEntries[i];
        lutData->getArray()[i * 3 + 2] = lutEntries[i];
    }

    auto invLut = lutData->inverse();
    
    OCIO_CHECK_NO_THROW(invLut->validate());
    OCIO_CHECK_NO_THROW(invLut->finalize());

    OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                       OCIO::BIT_DEPTH_F16,
                                                       OCIO::BIT_DEPTH_F16));

    const half inImage[] = {
        1.00f, 0.91f, 0.85f, 0.f,
        0.75f, 0.02f, 0.53f, 0.f,
        0.47f, 0.30f, 0.21f, 0.f,
        0.50f, 0.11f, 0.00f, 0.f };

    half outImage[] = {
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f };

    // (dist + (val-low)/(high-low)) / (dim-1)
    const half expected[] = {
        1.0000000000f, 0.8714285714f, 0.7857142857f, 0.f,
        0.6785714285f, 0.0285714285f, 0.5214285714f, 0.f,
        0.4785714285f, 0.3571428571f, 0.2928571428f, 0.f,
        0.5000000000f, 0.1571428571f, 0.0000000000f, 0.f };

    cpuOp->apply(&inImage[0], &outImage[0], 4);

    for (unsigned i = 0; i < 4 * 4; ++i)
    {
        OCIO_CHECK_CLOSE(outImage[i].bits(), expected[i].bits(), 1.1f);
    }

    // Repeat with FAST.
    constInvLut = FastFromInverse(invLut, __LINE__);
    OCIO::ConstOpCPURcPtr cpuOpFast;
    OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_F16,
                                                           OCIO::BIT_DEPTH_F16));

    cpuOpFast->apply(&inImage[0], &outImage[0], 4);

    for (unsigned i = 0; i < 4 * 4; ++i)
    {
        OCIO_CHECK_CLOSE(outImage[i].bits(), expected[i].bits(), 1.1f);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_half_identity)
{
    // Need to do 10i-->32f and vice versa to check that
    // both the in scaling and out scaling are working correctly.

    constexpr uint16_t stepui = 700;  // relative to 10i
    constexpr float step = stepui / 1023.f;

    // Process from 10i to 32f bit-depths.
    {
        // By default, this constructor creates an 'identity LUT'.
        OCIO::Lut1DOpDataRcPtr lutData
            = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE,
                                                  65536, false);
        lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

        auto invLut = lutData->inverse();

        OCIO_CHECK_NO_THROW(invLut->validate());
        OCIO_CHECK_NO_THROW(invLut->finalize());

        OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_UINT10,
                                                           OCIO::BIT_DEPTH_F32));

        const uint16_t inImage[] = {
            0,      0,      0,      0,
            stepui, 0,      0,      0,
            0,      stepui, 0,      0,
            0,      0,      stepui, 0,
            stepui, stepui, stepui, 0 };

        float outImage[] = {
            -1.f, -1.f, -1.f, -1.f,
            -1.f, -1.f, -1.f, -1.f,
            -1.f, -1.f, -1.f, -1.f,
            -1.f, -1.f, -1.f, -1.f,
            -1.f, -1.f, -1.f, -1.f };

        // Inverse of identity should still be identity.
        const float expected[] = {
            0.f,   0.f,  0.f, 0.f,
            step,  0.f,  0.f, 0.f,
            0.f,  step,  0.f, 0.f,
            0.f,   0.f, step, 0.f,
            step, step, step, 0.f };

        cpuOp->apply(&inImage[0], &outImage[0], 5);

        for (unsigned i = 0; i < 5 * 4; ++i)
        {
            OCIO_CHECK_CLOSE(outImage[i], expected[i], 1e-6f);
        }

        // Repeat with FAST.
        constInvLut = FastFromInverse(invLut, __LINE__);
        OCIO::ConstOpCPURcPtr cpuOpFast;
        OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                               OCIO::BIT_DEPTH_UINT10,
                                                               OCIO::BIT_DEPTH_F32));

        cpuOpFast->apply(&inImage[0], &outImage[0], 5);

        for (unsigned i = 0; i < 5 * 4; ++i)
        {
            OCIO_CHECK_CLOSE(outImage[i], expected[i], 1e-6f);
        }
    }
    // Process from 32f to 10i bit-depths.
    {
        // By default, this constructor creates an 'identity LUT'.
        OCIO::Lut1DOpDataRcPtr lutData
            = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE,
                                                  65536, false);
        lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);

        auto invLut = lutData->inverse();

        OCIO_CHECK_NO_THROW(invLut->validate());
        OCIO_CHECK_NO_THROW(invLut->finalize());

        OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_UINT10));

        const float inImage[] = {
            0.f,   0.f,  0.f, 0.f,
            step,  0.f,  0.f, 0.f,
            0.f,  step,  0.f, 0.f,
            0.f,   0.f, step, 0.f,
            step, step, step, 0.f };

        uint16_t outImage[] = {
            10000, 10000, 10000, 10000,
            10000, 10000, 10000, 10000,
            10000, 10000, 10000, 10000,
            10000, 10000, 10000, 10000,
            10000, 10000, 10000, 10000 };

        // Inverse of identity should still be identity.
        const uint16_t expected[] = {
            0,      0,      0,      0,
            stepui, 0,      0,      0,
            0,      stepui, 0,      0,
            0,      0,      stepui, 0,
            stepui, stepui, stepui, 0 };
        cpuOp->apply(&inImage[0], &outImage[0], 5);

        for (unsigned i = 0; i < 5 * 4; ++i)
        {
            OCIO_CHECK_EQUAL(outImage[i], expected[i]);
        }

        // Repeat with FAST.
        constInvLut = FastFromInverse(invLut, __LINE__);
        OCIO::ConstOpCPURcPtr cpuOpFast;
        OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                               OCIO::BIT_DEPTH_F32,
                                                               OCIO::BIT_DEPTH_UINT10));

        cpuOpFast->apply(&inImage[0], &outImage[0], 5);

        for (unsigned i = 0; i < 5 * 4; ++i)
        {
            OCIO_CHECK_EQUAL(outImage[i], expected[i]);
        }
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_half_ctf)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    // This ctf has increasing R & B channels and decreasing G channel.
    // It also has flat spots.
    const std::string ctfLUT("lut1d_halfdom.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));

    // This test should use the "interpolation" renderer path.
    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getDefaultCPUProcessor());

    const float inImage[12] = {
         1.f,    1.f,   0.5f, 0.f,
         0.001f, 0.1f,  4.f,  0.5f,  // test positive half domain of R, G, B channels
        -0.08f, -1.f, -10.f,  1.f }; // test negative half domain of R, G, B channels

    // Apply forward LUT.
    std::vector<float> outImage(12, -1.f);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], 3, 1, 4);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], 3, 1, 4);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    // Apply inverse LUT.
    // Inverse using FAST.
    fileTransform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    OCIO::ConstCPUProcessorRcPtr cpuOpInv;
    OCIO_CHECK_NO_THROW(cpuOpInv = proc->getDefaultCPUProcessor());

    std::vector<float> backImage(12, -1.f);
    OCIO::PackedImageDesc backImgDesc((void*)&backImage[0], 3, 1, 4);
    cpuOpInv->apply(dstImgDesc, backImgDesc);

    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 50, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }

    // Repeat with EXACT.
    OCIO::ConstCPUProcessorRcPtr cpuInvFast;
    std::fill(backImage.begin(), backImage.end(), -1.f);
    OCIO_CHECK_NO_THROW(cpuInvFast = proc->getOptimizedCPUProcessor(DefaultNoLutInvFast));
    cpuInvFast->apply(dstImgDesc, backImgDesc);

    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 50, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_half_fclut)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    // TODO: Review the test to add LUT & inverse LUT together when optimization is reworked.

    // Lustre log_default.fclut.  All positive halfs map to unique 16-bit ints
    // so it's a good test to see that the inverse can restore the halfs losslessly.
    const std::string ctfLUT("lut1d_hd_16f_16i_1chan.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    OCIO::ConstCPUProcessorRcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F16,
                                                               OCIO::BIT_DEPTH_F32,
                                                               DefaultNoLutInvFast));

    // Test all positive halfs (less than inf) round-trip losslessly.
    const int nbPixels = 31744;
    std::vector<half> inImage(nbPixels * 4);
    std::vector<float> outImage(nbPixels * 4, -1.f);
    for (unsigned short i = 0; i < nbPixels; ++i)
    {
        inImage[i * 4 + 0].setBits(i);
        inImage[i * 4 + 1].setBits(i);
        inImage[i * 4 + 2].setBits(i);
        inImage[i * 4 + 3].setBits(i);
    }

    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], nbPixels, 1, 4,
                                     OCIO::BIT_DEPTH_F16,
                                     sizeof(half),
                                     OCIO::AutoStride,
                                     OCIO::AutoStride);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], nbPixels, 1, 4);
    cpuOp->apply(srcImgDesc, dstImgDesc);

    fileTransform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    OCIO::ConstCPUProcessorRcPtr cpuOpInv;
    OCIO_CHECK_NO_THROW(cpuOpInv = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                  OCIO::BIT_DEPTH_F16,
                                                                  DefaultNoLutInvFast));

    std::vector<half> backImage(nbPixels * 4, -1.f);
    OCIO::PackedImageDesc backImgDesc((void*)&backImage[0], nbPixels, 1, 4,
                                      OCIO::BIT_DEPTH_F16,
                                      sizeof(half),
                                      OCIO::AutoStride,
                                      OCIO::AutoStride);

    cpuOpInv->apply(dstImgDesc, backImgDesc);

    for (unsigned short i = 0; i < nbPixels; ++i)
    {
        OCIO_CHECK_EQUAL(inImage[i * 4 + 0].bits(), backImage[i * 4 + 0].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 1].bits(), backImage[i * 4 + 1].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 2].bits(), backImage[i * 4 + 2].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 3].bits(), backImage[i * 4 + 3].bits());
    }

    // Repeat with FAST.
    OCIO_CHECK_NO_THROW(cpuOpInv = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                  OCIO::BIT_DEPTH_F16,
                                                                  DefaultNoLutInvFast));

    std::fill(backImage.begin(), backImage.end(), -1.f);
    cpuOpInv->apply(dstImgDesc, backImgDesc);

    for (unsigned short i = 0; i < nbPixels; ++i)
    {
        OCIO_CHECK_EQUAL(inImage[i * 4 + 0].bits(), backImage[i * 4 + 0].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 1].bits(), backImage[i * 4 + 1].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 2].bits(), backImage[i * 4 + 2].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 3].bits(), backImage[i * 4 + 3].bits());
    }

}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_half_domain_hue_adjust)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    const std::string ctfLUT("lut1d_hd_hue_adjust.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getDefaultCPUProcessor());

    constexpr long NB_PIXELS = 3;
    const float inImage[NB_PIXELS * 4] = {
        0.1f,  0.25f, 0.7f,  0.f,
        0.66f, 0.25f, 0.81f, 0.5f,
        0.18f, 0.99f, 0.45f, 1.f };

    std::vector<float> outImage(NB_PIXELS * 4, 1.f);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], NB_PIXELS, 1, 4);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], NB_PIXELS, 1, 4);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    // Inverse using FAST (which is part of the default optimization level).
    fileTransform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    OCIO::ConstCPUProcessorRcPtr cpuInv;
    OCIO_CHECK_NO_THROW(cpuInv = proc->getDefaultCPUProcessor());

    std::vector<float> backImage(NB_PIXELS * 4, -1.f);
    OCIO::PackedImageDesc backImgDesc((void*)&backImage[0], NB_PIXELS, 1, 4);
    cpuInv->apply(dstImgDesc, backImgDesc);

    for (long i = 0; i < NB_PIXELS * 4; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 50, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }

    // Repeat with EXACT.
    OCIO::ConstCPUProcessorRcPtr cpuInvFast;
    OCIO_CHECK_NO_THROW(cpuInvFast = proc->getOptimizedCPUProcessor(DefaultNoLutInvFast));
    std::fill(backImage.begin(), backImage.end(), -1.f);
    cpuInvFast->apply(dstImgDesc, backImgDesc);

    for (long i = 0; i < NB_PIXELS * 4; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 50, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }
}

