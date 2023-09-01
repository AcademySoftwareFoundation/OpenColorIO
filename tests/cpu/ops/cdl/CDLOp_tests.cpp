// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <limits>

#include "ops/cdl/CDLOp.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{
void ApplyCDL(float * in, const float * ref, unsigned numPixels,
              const double * slope, const double * offset,
              const double * power, double saturation,
              OCIO::CDLOpData::Style style, float errorThreshold)
{
    OCIO::CDLOpDataRcPtr data
        = std::make_shared<OCIO::CDLOpData>(style,
                                            OCIO::CDLOpData::ChannelParams(slope[0],  slope[1],  slope[2]),
                                            OCIO::CDLOpData::ChannelParams(offset[0], offset[1], offset[2]),
                                            OCIO::CDLOpData::ChannelParams(power[0],  power[1],  power[2]),
                                            saturation);
    OCIO::CDLOp cdlOp(data);

    OCIO_CHECK_NO_THROW(cdlOp.validate());

    const auto cpu = cdlOp.getCPUOp(true);
    cpu->apply(in, in, numPixels);

    for(unsigned idx=0; idx<(numPixels*4); ++idx)
    {
        // Using rel error with a large minExpected value of 1 will transition
        // from absolute error for expected values < 1 and
        // relative error for values > 1.
        const bool equalRel = OCIO::EqualWithSafeRelError(in[idx],
                                                          ref[idx],
                                                          errorThreshold,
                                                          1.0f);
        if (!equalRel)
        {
            std::ostringstream message;
            message << "Index: " << idx;
            message << " - Values: " << in[idx] << " and: " << ref[idx];
            message << " - Threshold: " << errorThreshold;
            OCIO_CHECK_ASSERT_MESSAGE(0, message.str());
        }
    }
}

}

// TODO: CDL Unit tests with bit depth are missing.


namespace CDL_DATA_1
{
const double slope[3]  = { 1.35,  1.1,  0.071 };
const double offset[3] = { 0.05, -0.23, 0.11  };
const double power[3]  = { 0.93,  0.81, 1.27  };
const double saturation = 1.23;
};

OCIO_ADD_TEST(CDLOp, computed_identifier)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateCDLOp(ops, 
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation, 
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::CreateCDLOp(ops, 
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    std::string id0, id1;
    OCIO_CHECK_NO_THROW(id0 = ops[0]->getCacheID());
    OCIO_CHECK_NO_THROW(id1 = ops[1]->getCacheID());
    OCIO_CHECK_EQUAL(id0, id1);

    const OCIO::CDLOpData::ChannelParams slope(CDL_DATA_1::slope[0],
                                               CDL_DATA_1::slope[1],
                                               CDL_DATA_1::slope[2]);
    const OCIO::CDLOpData::ChannelParams offset(CDL_DATA_1::offset[0],
                                                CDL_DATA_1::offset[1],
                                                CDL_DATA_1::offset[2]);
    const OCIO::CDLOpData::ChannelParams power(CDL_DATA_1::power[0],
                                               CDL_DATA_1::power[1],
                                               CDL_DATA_1::power[2]);

    auto cdlData = std::make_shared<OCIO::CDLOpData>(OCIO::CDLOpData::CDL_V1_2_FWD,
                                                     slope, offset,
                                                     power, CDL_DATA_1::saturation);
    cdlData->getFormatMetadata().addAttribute(OCIO::METADATA_ID, "1");
    OCIO_CHECK_EQUAL(cdlData->getID(), "1");

    OCIO::CreateCDLOp(ops, cdlData, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 3);

    std::string id2;
    OCIO_CHECK_NO_THROW(id2 = ops[2]->getCacheID());

    OCIO_CHECK_ASSERT( id0 != id2 );
    OCIO_CHECK_ASSERT( id1 != id2 );

    OCIO::CreateCDLOp(ops, 
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation + 0.002f, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 4);

    std::string id3;
    OCIO_CHECK_NO_THROW(id3 = ops[3]->getCacheID());

    OCIO_CHECK_ASSERT( id0 != id3 );
    OCIO_CHECK_ASSERT( id1 != id3 );
    OCIO_CHECK_ASSERT( id2 != id3 );

    OCIO::CreateCDLOp(ops, 
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation + 0.002f, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 5);

    std::string id4;
    OCIO_CHECK_NO_THROW(id4 = ops[4]->getCacheID());

    OCIO_CHECK_ASSERT( id0 != id4 );
    OCIO_CHECK_ASSERT( id1 != id4 );
    OCIO_CHECK_ASSERT( id2 != id4 );
    OCIO_CHECK_ASSERT( id3 == id4 );

    OCIO::CreateCDLOp(ops, 
                      OCIO::CDLOpData::CDL_NO_CLAMP_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation + 0.002f, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 6);

    std::string id5;
    OCIO_CHECK_NO_THROW(id5 = ops[5]->getCacheID());

    OCIO_CHECK_ASSERT( id3 != id5 );
    OCIO_CHECK_ASSERT( id4 != id5 );
}

OCIO_ADD_TEST(CDLOp, is_inverse)
{
    OCIO::OpRcPtrVec ops;

    OCIO::CreateCDLOp(ops, 
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation, 
                      OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);

    OCIO::CreateCDLOp(ops, 
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset,
                      CDL_DATA_1::power, CDL_DATA_1::saturation, 
                      OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 2);

    OCIO::ConstOpRcPtr op0 = ops[0];
    OCIO::ConstOpRcPtr op1 = ops[1];

    OCIO_CHECK_ASSERT(ops[0]->isInverse(op1));
    OCIO_CHECK_ASSERT(ops[1]->isInverse(op0));

    OCIO::CreateCDLOp(ops, 
                      OCIO::CDLOpData::CDL_V1_2_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset, 
                      CDL_DATA_1::power, 1.30, 
                      OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 3);
    OCIO::ConstOpRcPtr op2 = ops[2];

    OCIO_CHECK_ASSERT(!ops[0]->isInverse(op2));
    OCIO_CHECK_ASSERT(!ops[1]->isInverse(op2));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op0));
    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op1));

    OCIO::CreateCDLOp(ops, 
                      OCIO::CDLOpData::CDL_V1_2_REV, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset, 
                      CDL_DATA_1::power, 1.30, 
                      OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO::ConstOpRcPtr op3 = ops[3];

    OCIO_CHECK_ASSERT(ops[2]->isInverse(op3));

    OCIO::CreateCDLOp(ops,
                      OCIO::CDLOpData::CDL_V1_2_REV, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset, 
                      CDL_DATA_1::power, 1.30, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 5);
    OCIO::ConstOpRcPtr op4 = ops[4];

    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op4));
    OCIO_CHECK_ASSERT(ops[3]->isInverse(op4));

    OCIO::CreateCDLOp(ops,
                      OCIO::CDLOpData::CDL_NO_CLAMP_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset, 
                      CDL_DATA_1::power, 1.30, 
                      OCIO::TRANSFORM_DIR_FORWARD);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 6);
    OCIO::ConstOpRcPtr op5 = ops[5];

    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op5));
    OCIO_CHECK_ASSERT(!ops[3]->isInverse(op5));
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op5));

    OCIO::CreateCDLOp(ops,
                      OCIO::CDLOpData::CDL_NO_CLAMP_FWD, 
                      CDL_DATA_1::slope, CDL_DATA_1::offset, 
                      CDL_DATA_1::power, 1.30, 
                      OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(ops.finalize());
    OCIO_REQUIRE_EQUAL(ops.size(), 7);
    OCIO::ConstOpRcPtr op6 = ops[6];

    OCIO_CHECK_ASSERT(!ops[2]->isInverse(op6));
    OCIO_CHECK_ASSERT(!ops[3]->isInverse(op6));
    OCIO_CHECK_ASSERT(!ops[4]->isInverse(op6));
    OCIO_CHECK_ASSERT(ops[5]->isInverse(op6));
}

// The expected values below were calculated via an independent ASC CDL
// implementation.
// Note that the error thresholds are higher for the SSE version because
// of the use of a much faster, but somewhat less accurate, implementation
// of the power function.
// TODO: The NaN and Inf handling is probably not ideal, as shown by the
// tests below, and could be improved.
OCIO_ADD_TEST(CDLOp, apply_clamp_fwd)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
         qnan,    qnan,  qnan,  0.0f,
         0.0f,    0.0f,  0.0f,  qnan,
         inf,     inf,   inf,   inf,
        -inf,    -inf,  -inf,  -inf,
         0.3278f, 0.01f, 1.0f,  0.0f,
         0.25f,   0.5f,  0.75f, 1.0f,
         1.25f,   1.5f,  1.75f, 0.75f,
        -0.2f,    0.5f,  1.4f,  0.0f,
        -0.25f,  -0.5f, -0.75f, 0.25f,
         0.0f,    0.8f,  0.99f, 0.5f };

    const float expected_32f[] = {
        0.0f,      0.0f,      0.0f,      0.0f,
        0.071827f, 0.0f,      0.070533f, qnan,
        1.0f,      1.0f,      1.0f,      inf,
        0.0f,      0.0f,      0.0f,     -inf,
        0.609399f, 0.000000f, 0.113130f, 0.0f,
        0.422056f, 0.401466f, 0.035820f, 1.0f,
        1.000000f, 1.000000f, 0.000000f, 0.75f,
        0.000000f, 0.421096f, 0.101225f, 0.0f,
        0.000000f, 0.000000f, 0.031735f, 0.25f,
        0.000000f, 0.746748f, 0.018691f, 0.5f  };

    ApplyCDL(input_32f, expected_32f, 10,
             CDL_DATA_1::slope, CDL_DATA_1::offset,
             CDL_DATA_1::power, CDL_DATA_1::saturation,
             OCIO::CDLOpData::CDL_V1_2_FWD,
#if OCIO_USE_SSE2
             4e-6f);
#else
             2e-6f);
#endif
}

OCIO_ADD_TEST(CDLOp, apply_clamp_rev)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
       qnan,       qnan,       qnan,      0.0f,
       0.0f,       0.0f,       0.0f,      qnan,
       inf,        inf,        inf,       inf,
      -inf,       -inf,       -inf,      -inf,
       0.609399f,  0.100000f,  0.113130f, 0.0f,
       0.001000f,  0.746748f,  0.018691f, 0.5f,
       0.422056f,  0.401466f,  0.035820f, 1.0f,
      -0.25f,     -0.5f,      -0.75f,     0.25f,
       1.25f,      1.5f,       1.75f,     0.75f,
      -0.2f,       0.5f,       1.4f,      0.0f };

    const float expected_32f[] = {
      0.0f,      0.209091f, 0.0f,      0.0f,
      0.0f,      0.209091f, 0.0f,      qnan,
      0.703713f, 1.0f,      1.0f,       inf,
      0.0f,      0.209091f, 0.0f,      -inf,
      0.340710f, 0.275726f, 1.000000f, 0.0f,
      0.025902f, 0.801895f, 1.000000f, 0.5f,
      0.250000f, 0.500000f, 0.750006f, 1.0f,
      0.000000f, 0.209091f, 0.000000f, 0.25f,
      0.703704f, 1.000000f, 1.000000f, 0.75f,
      0.012206f, 0.582944f, 1.000000f, 0.0f };

    ApplyCDL(input_32f, expected_32f, 10,
             CDL_DATA_1::slope, CDL_DATA_1::offset,
             CDL_DATA_1::power, CDL_DATA_1::saturation,
             OCIO::CDLOpData::CDL_V1_2_REV,
#if OCIO_USE_SSE2
             9e-6f);
#else
             1e-5f);
#endif
}

OCIO_ADD_TEST(CDLOp, apply_noclamp_fwd)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
       qnan,    qnan,  qnan,  0.0f,
       0.0f,    0.0f,  0.0f,  qnan,
       inf,     inf,   inf,   inf,
      -inf,    -inf,  -inf,  -inf,
       0.3278f, 0.01f, 1.0f,  0.0f,
       0.0f,    0.8f,  0.99f, 0.5f,
       0.25f,   0.5f,  0.75f, 1.0f,
      -0.25f,  -0.5f, -0.75f, 0.25f,
       1.25f,   1.5f,  1.75f, 0.75f,
      -0.2f,    0.5f,  1.4f,  0.0f };

    const float expected_32f[] = {
       0.0f,       0.0f,       0.0f,      0.0f,
       0.109661f, -0.249088f,  0.108368f, qnan,
       qnan,      qnan,        qnan,       inf,
       qnan,      qnan,        qnan,      -inf,
       0.645424f, -0.260548f,  0.149154f, 0.0f,
      -0.045094f,  0.746748f,  0.018691f, 0.5f,
       0.422056f,  0.401466f,  0.035820f, 1.0f,
      -0.211694f, -0.817469f,  0.174100f, 0.25f,
       1.753162f,  1.331130f, -0.108181f, 0.75f,
      -0.327485f,  0.431854f,  0.111983f, 0.0f };

    ApplyCDL(input_32f, expected_32f, 10,
             CDL_DATA_1::slope, CDL_DATA_1::offset,
             CDL_DATA_1::power, CDL_DATA_1::saturation,
             OCIO::CDLOpData::CDL_NO_CLAMP_FWD,
#if OCIO_USE_SSE2
             2e-5f);
#else
             2e-6f);
#endif
}

OCIO_ADD_TEST(CDLOp, apply_noclamp_rev)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
       qnan,       qnan,       qnan,      0.0f,
       0.0f,       0.0f,       0.0f,      qnan,
       inf,        inf,        inf,       inf,
      -inf,       -inf,       -inf,      -inf,
       0.609399f,  0.100000f,  0.113130f, 0.0f,
       0.001000f,  0.746748f,  0.018691f, 0.5f,
       0.422056f,  0.401466f,  0.035820f, 1.0f,
      -0.25f,     -0.5f,      -0.75f,     0.25f,
       1.25f,      1.5f,       1.75f,     0.75f,
      -0.2f,       0.5f,       1.4f,      0.0f };

    const float expected_32f[] = {
      -0.037037f,  0.209091f, -1.549296f,  0.0f,
      -0.037037f,  0.209091f, -1.549296f,  qnan,
      -0.037037f,  0.209091f, -1.549296f,  inf,
      -0.037037f,  0.209091f, -1.549296f, -inf,
       0.340710f,  0.275726f,  1.294827f,  0.0f,
       0.025902f,  0.801895f,  1.022221f,  0.5f,
       0.250000f,  0.500000f,  0.750006f,  1.0f,
      -0.251989f, -0.239488f, -11.361812f, 0.25f,
       0.937160f,  1.700692f,  19.807237f, 0.75f,
      -0.099839f,  0.580528f,  14.880301f, 0.0f };

    ApplyCDL(input_32f, expected_32f, 10,
             CDL_DATA_1::slope, CDL_DATA_1::offset,
             CDL_DATA_1::power, CDL_DATA_1::saturation,
             OCIO::CDLOpData::CDL_NO_CLAMP_REV,
#if OCIO_USE_SSE2
             3e-5f);
#else
             1e-6f);
#endif
}

namespace CDL_DATA_2
{
const double slope[3]  = { 1.15, 1.10, 0.9  };
const double offset[3] = { 0.05, 0.02, 0.07 };
const double power[3]  = { 1.2,  0.95, 1.13 };
const double saturation = 0.87;
};

OCIO_ADD_TEST(CDLOp, apply_clamp_fwd_2)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
      qnan,  qnan,  qnan,  0.0f,
      0.0f,  0.0f,  0.0f,  qnan,
      inf,   inf,   inf,   inf,
     -inf,  -inf,  -inf,  -inf,
      0.65f, 0.55f, 0.20f, 0.0f,
      0.41f, 0.81f, 0.39f, 0.5f,
      0.25f, 0.50f, 0.75f, 1.0f };

    const float expected_32f[] = {
      0.0f,      0.0f,      0.0f,      0.0f,
      0.027379f, 0.024645f, 0.046585f, qnan,
      1.0f,      1.0f,      1.0f,      inf,
      0.0f,      0.0f,      0.0,      -inf,
      0.745644f, 0.639197f, 0.264149f, 0.0f,
      0.499594f, 0.897554f, 0.428591f, 0.5f,
      0.305035f, 0.578779f, 0.692558f, 1.0f };

    ApplyCDL(input_32f, expected_32f, 7,
             CDL_DATA_2::slope, CDL_DATA_2::offset,
             CDL_DATA_2::power, CDL_DATA_2::saturation,
             OCIO::CDLOpData::CDL_V1_2_FWD,
#if OCIO_USE_SSE2
             7e-6f);
#else
             1e-6f);
#endif
}

namespace CDL_DATA_3
{
const double slope[3]  = {  3.405,  1.0,    1.0   };
const double offset[3] = { -0.178, -0.178, -0.178 };
const double power[3]  = {  1.095,  1.095,  1.095 };
const double saturation = 0.99;
};

OCIO_ADD_TEST(CDLOp, apply_clamp_fwd_3)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
      qnan,  qnan, qnan, 0.0f,
      0.0f,  0.0f, 0.0f, qnan,
      inf,   inf,  inf,  inf,
     -inf,  -inf, -inf, -inf,

      0.02f, 0.0f, 0.0f, 0.0f,
      0.17f, 0.0f, 0.0f, 0.0f,
      0.65f, 0.0f, 0.0f, 0.0f,
      0.97f, 0.0f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.0f, 0.0f,
      0.17f, 0.13f, 0.0f, 0.0f,
      0.65f, 0.13f, 0.0f, 0.0f,
      0.97f, 0.13f, 0.0f, 0.0f,

      0.02f, 0.23f, 0.0f, 0.0f,
      0.17f, 0.23f, 0.0f, 0.0f,
      0.65f, 0.23f, 0.0f, 0.0f,
      0.97f, 0.23f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.23f, 0.0f,
      0.17f, 0.13f, 0.23f, 0.0f,
      0.65f, 0.13f, 0.23f, 0.0f,
      0.97f, 0.13f, 0.23f, 0.0f };

    const float expected_32f[] = {
      0.000000f, 0.000000f, 0.000000f, 0.0f,
      0.000000f, 0.000000f, 0.000000f, qnan,
      1.0f,      1.0f,      1.0f,      inf,
      0.0f,      0.0f,      0.0f,     -inf,

      0.000000f, 0.000000f, 0.000000f, 0.0f,
      0.364613f, 0.000781f, 0.000781f, 0.0f,
      0.992126f, 0.002126f, 0.002126f, 0.0f,
      0.992126f, 0.002126f, 0.002126f, 0.0f,

      0.000000f, 0.000000f, 0.000000f, 0.0f,
      0.364613f, 0.000781f, 0.000781f, 0.0f,
      0.992126f, 0.002126f, 0.002126f, 0.0f,
      0.992126f, 0.002126f, 0.002126f, 0.0f,

      0.000281f, 0.039155f, 0.0002808f, 0.0f,
      0.364894f, 0.039936f, 0.0010621f, 0.0f,
      0.992407f, 0.041281f, 0.0024068f, 0.0f,
      0.992407f, 0.041281f, 0.0024068f, 0.0f,

      0.000028f, 0.000028f, 0.0389023f, 0.0f,
      0.364641f, 0.000810f, 0.0396836f, 0.0f,
      0.992154f, 0.002154f, 0.0410283f, 0.0f,
      0.992154f, 0.002154f, 0.0410283f, 0.0f };

    ApplyCDL(input_32f, expected_32f, 20,
             CDL_DATA_3::slope, CDL_DATA_3::offset,
             CDL_DATA_3::power, CDL_DATA_3::saturation,
             OCIO::CDLOpData::CDL_V1_2_FWD,
#if OCIO_USE_SSE2
             2e-5f);
#else
             1e-6f);
#endif
}

OCIO_ADD_TEST(CDLOp, apply_noclamp_fwd_3)
{
    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    float input_32f[] = {
      qnan,  qnan, qnan, 0.0f,
      0.0f,  0.0f, 0.0f, qnan,
      inf,   inf,  inf,  inf,
     -inf,  -inf, -inf, -inf,

      0.02f, 0.0f, 0.0f, 0.0f,
      0.17f, 0.0f, 0.0f, 0.0f,
      0.65f, 0.0f, 0.0f, 0.0f,
      0.97f, 0.0f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.0f, 0.0f,
      0.17f, 0.13f, 0.0f, 0.0f,
      0.65f, 0.13f, 0.0f, 0.0f,
      0.97f, 0.13f, 0.0f, 0.0f,

      0.02f, 0.23f, 0.0f, 0.0f,
      0.17f, 0.23f, 0.0f, 0.0f,
      0.65f, 0.23f, 0.0f, 0.0f,
      0.97f, 0.23f, 0.0f, 0.0f,

      0.02f, 0.13f, 0.23f, 0.0f,
      0.17f, 0.13f, 0.23f, 0.0f,
      0.65f, 0.13f, 0.23f, 0.0f,
      0.97f, 0.13f, 0.23f, 0.0f };

    const float expected_32f[] = {
       0.0f,       0.0f,       0.0f,      0.0f,
      -0.178000f, -0.178000f, -0.178000f, qnan,
       qnan,       qnan,       qnan,      inf,
       qnan,       qnan,       qnan,     -inf,

      -0.110436f, -0.177855f, -0.177855f, 0.0f,
       0.363211f, -0.176840f, -0.176840f, 0.0f,
       2.158845f, -0.172992f, -0.172992f, 0.0f,
       3.453254f, -0.170219f, -0.170219f, 0.0f,

      -0.109506f, -0.048225f, -0.176925f, 0.0f,
       0.364141f, -0.047210f, -0.175910f, 0.0f,
       2.159774f, -0.043363f, -0.172063f, 0.0f,
       3.454184f, -0.040589f, -0.169289f, 0.0f,

      -0.108882f, 0.038793f, -0.176301f, 0.0f,
       0.364765f, 0.039808f, -0.175286f, 0.0f,
       2.160399f, 0.043655f, -0.171438f, 0.0f,
       3.454808f, 0.046429f, -0.168665f, 0.0f,

      -0.109350f, -0.048069f, 0.038325f, 0.0f,
       0.364298f, -0.047054f, 0.039340f, 0.0f,
       2.159931f, -0.043206f, 0.043188f, 0.0f,
       3.454341f, -0.040432f, 0.045962f, 0.0f };

    ApplyCDL(input_32f, expected_32f, 20,
             CDL_DATA_3::slope, CDL_DATA_3::offset,
             CDL_DATA_3::power, CDL_DATA_3::saturation,
             OCIO::CDLOpData::CDL_NO_CLAMP_FWD,
#if OCIO_USE_SSE2
             5e-6f);
#else
             1e-6f);
#endif
}

OCIO_ADD_TEST(CDLOp, create_transform)
{
    const OCIO::CDLOpData::ChannelParams sp(CDL_DATA_1::slope[0],
                                            CDL_DATA_1::slope[1],
                                            CDL_DATA_1::slope[2]);
    const OCIO::CDLOpData::ChannelParams so(CDL_DATA_1::offset[0],
                                            CDL_DATA_1::offset[1],
                                            CDL_DATA_1::offset[2]);
    const OCIO::CDLOpData::ChannelParams sw(CDL_DATA_1::power[0],
                                            CDL_DATA_1::power[1],
                                            CDL_DATA_1::power[2]);

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    {
        // Forward direction.
        auto cdlData = std::make_shared<OCIO::CDLOpData>(OCIO::CDLOpData::CDL_V1_2_FWD,
                                                         sp, so, sw,
                                                         CDL_DATA_1::saturation);
        auto & metadataSrc = cdlData->getFormatMetadata();
        metadataSrc.addAttribute(OCIO::METADATA_ID, "Test look: 01-A.");
        auto cdlOp = std::make_shared<OCIO::CDLOp>(cdlData);

        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        OCIO::ConstOpRcPtr op(cdlOp);
        OCIO::CreateCDLTransform(group, op);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        auto transform = group->getTransform(0);
        OCIO_REQUIRE_ASSERT(transform);
        auto cdlTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::CDLTransform>(transform);
        OCIO_REQUIRE_ASSERT(cdlTransform);

        const auto & metadata = cdlTransform->getFormatMetadata();
        OCIO_REQUIRE_EQUAL(metadata.getNumAttributes(), 1);
        OCIO_CHECK_EQUAL(std::string(metadata.getAttributeName(0)), OCIO::METADATA_ID);
        OCIO_CHECK_EQUAL(std::string(metadata.getAttributeValue(0)), "Test look: 01-A.");
        OCIO_CHECK_EQUAL(cdlTransform->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);

        double slope[3];
        cdlTransform->getSlope(slope);
        OCIO_CHECK_EQUAL(slope[0], CDL_DATA_1::slope[0]);
        OCIO_CHECK_EQUAL(slope[1], CDL_DATA_1::slope[1]);
        OCIO_CHECK_EQUAL(slope[2], CDL_DATA_1::slope[2]);

        double offset[3];
        cdlTransform->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], CDL_DATA_1::offset[0]);
        OCIO_CHECK_EQUAL(offset[1], CDL_DATA_1::offset[1]);
        OCIO_CHECK_EQUAL(offset[2], CDL_DATA_1::offset[2]);

        double power[3];
        cdlTransform->getPower(power);
        OCIO_CHECK_EQUAL(power[0], CDL_DATA_1::power[0]);
        OCIO_CHECK_EQUAL(power[1], CDL_DATA_1::power[1]);
        OCIO_CHECK_EQUAL(power[2], CDL_DATA_1::power[2]);

        OCIO_CHECK_EQUAL(cdlTransform->getSat(), CDL_DATA_1::saturation);
        OCIO_CHECK_EQUAL(cdlTransform->getStyle(), OCIO::CDL_ASC);

        // Back to op.
        OCIO::OpRcPtrVec ops;
        OCIO::BuildCDLOp(ops, *config, *cdlTransform,
                         OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(ops.size(), 1);
        OCIO::BuildCDLOp(ops, *config, *cdlTransform,
                         OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        OCIO::ConstOpRcPtr op0 = ops[0];
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO_CHECK_EQUAL(op0->data()->getType(), OCIO::OpData::CDLType);
        OCIO_CHECK_EQUAL(op1->data()->getType(), OCIO::OpData::CDLType);
        auto cdl0 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::CDLOpData>(op0->data());
        auto cdl1 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::CDLOpData>(op1->data());
        OCIO_CHECK_EQUAL(cdl0->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);
        OCIO_CHECK_EQUAL(cdl1->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);
    }
    {
        // Inverse direction.
        auto cdlData = std::make_shared<OCIO::CDLOpData>(OCIO::CDLOpData::CDL_V1_2_REV,
                                                         sp, so, sw,
                                                         CDL_DATA_1::saturation);
        auto cdlOp = std::make_shared<OCIO::CDLOp>(cdlData);

        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        OCIO::ConstOpRcPtr op(cdlOp);
        OCIO::CreateCDLTransform(group, op);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        auto transform = group->getTransform(0);
        OCIO_REQUIRE_ASSERT(transform);
        auto cdlTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::CDLTransform>(transform);
        OCIO_REQUIRE_ASSERT(cdlTransform);

        OCIO_CHECK_EQUAL(cdlTransform->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(cdlTransform->getStyle(), OCIO::CDL_ASC);

        double slope[3];
        cdlTransform->getSlope(slope);
        OCIO_CHECK_EQUAL(slope[0], CDL_DATA_1::slope[0]);
        OCIO_CHECK_EQUAL(slope[1], CDL_DATA_1::slope[1]);
        OCIO_CHECK_EQUAL(slope[2], CDL_DATA_1::slope[2]);

        double offset[3];
        cdlTransform->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], CDL_DATA_1::offset[0]);
        OCIO_CHECK_EQUAL(offset[1], CDL_DATA_1::offset[1]);
        OCIO_CHECK_EQUAL(offset[2], CDL_DATA_1::offset[2]);

        double power[3];
        cdlTransform->getPower(power);
        OCIO_CHECK_EQUAL(power[0], CDL_DATA_1::power[0]);
        OCIO_CHECK_EQUAL(power[1], CDL_DATA_1::power[1]);
        OCIO_CHECK_EQUAL(power[2], CDL_DATA_1::power[2]);

        OCIO_CHECK_EQUAL(cdlTransform->getSat(), CDL_DATA_1::saturation);

        // Back to op.
        OCIO::OpRcPtrVec ops;
        OCIO::BuildCDLOp(ops, *config, *cdlTransform,
                         OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(ops.size(), 1);
        OCIO::BuildCDLOp(ops, *config, *cdlTransform,
                         OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        OCIO::ConstOpRcPtr op0 = ops[0];
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO_CHECK_EQUAL(op0->data()->getType(), OCIO::OpData::CDLType);
        OCIO_CHECK_EQUAL(op1->data()->getType(), OCIO::OpData::CDLType);
        auto cdl0 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::CDLOpData>(op0->data());
        auto cdl1 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::CDLOpData>(op1->data());
        OCIO_CHECK_EQUAL(cdl0->getStyle(), OCIO::CDLOpData::CDL_V1_2_REV);
        OCIO_CHECK_EQUAL(cdl1->getStyle(), OCIO::CDLOpData::CDL_V1_2_FWD);
    }
    {
        auto cdlData = std::make_shared<OCIO::CDLOpData>(OCIO::CDLOpData::CDL_NO_CLAMP_FWD,
                                                         sp, so, sw,
                                                         CDL_DATA_1::saturation);
        auto cdlOp = std::make_shared<OCIO::CDLOp>(cdlData);
        OCIO::GroupTransformRcPtr group = OCIO::GroupTransform::Create();
        OCIO::ConstOpRcPtr op(cdlOp);
        OCIO::CreateCDLTransform(group, op);
        OCIO_REQUIRE_EQUAL(group->getNumTransforms(), 1);
        auto transform = group->getTransform(0);
        OCIO_REQUIRE_ASSERT(transform);
        auto cdlTransform = OCIO_DYNAMIC_POINTER_CAST<OCIO::CDLTransform>(transform);
        OCIO_REQUIRE_ASSERT(cdlTransform);
        OCIO_CHECK_EQUAL(cdlTransform->getStyle(), OCIO::CDL_NO_CLAMP);
        OCIO_CHECK_EQUAL(cdlTransform->getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
        cdlTransform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(cdlTransform->getDirection(), OCIO::TRANSFORM_DIR_INVERSE);

        double slope[3];
        cdlTransform->getSlope(slope);
        OCIO_CHECK_EQUAL(slope[0], CDL_DATA_1::slope[0]);
        OCIO_CHECK_EQUAL(slope[1], CDL_DATA_1::slope[1]);
        OCIO_CHECK_EQUAL(slope[2], CDL_DATA_1::slope[2]);

        double offset[3];
        cdlTransform->getOffset(offset);
        OCIO_CHECK_EQUAL(offset[0], CDL_DATA_1::offset[0]);
        OCIO_CHECK_EQUAL(offset[1], CDL_DATA_1::offset[1]);
        OCIO_CHECK_EQUAL(offset[2], CDL_DATA_1::offset[2]);

        double power[3];
        cdlTransform->getPower(power);
        OCIO_CHECK_EQUAL(power[0], CDL_DATA_1::power[0]);
        OCIO_CHECK_EQUAL(power[1], CDL_DATA_1::power[1]);
        OCIO_CHECK_EQUAL(power[2], CDL_DATA_1::power[2]);

        OCIO_CHECK_EQUAL(cdlTransform->getSat(), CDL_DATA_1::saturation);

        // Back to op.
        OCIO::OpRcPtrVec ops;
        OCIO::BuildCDLOp(ops, *config, *cdlTransform,
                         OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(ops.size(), 1);
        OCIO::BuildCDLOp(ops, *config, *cdlTransform,
                         OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_REQUIRE_EQUAL(ops.size(), 2);
        OCIO::ConstOpRcPtr op0 = ops[0];
        OCIO::ConstOpRcPtr op1 = ops[1];
        OCIO_CHECK_EQUAL(op0->data()->getType(), OCIO::OpData::CDLType);
        OCIO_CHECK_EQUAL(op1->data()->getType(), OCIO::OpData::CDLType);
        auto cdl0 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::CDLOpData>(op0->data());
        auto cdl1 = OCIO_DYNAMIC_POINTER_CAST<const OCIO::CDLOpData>(op1->data());
        OCIO_CHECK_EQUAL(cdl0->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_REV);
        OCIO_CHECK_EQUAL(cdl1->getStyle(), OCIO::CDLOpData::CDL_NO_CLAMP_FWD);
    }
}

