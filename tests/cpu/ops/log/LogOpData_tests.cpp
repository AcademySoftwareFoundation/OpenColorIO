// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/log/LogOpData.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(LogOpData, accessor_test)
{
    OCIO::LogUtil::CTFParams ctfParams;
    auto & redP = ctfParams.get(OCIO::LogUtil::CTFParams::red);
    redP[OCIO::LogUtil::CTFParams::gamma]     = 2.4;
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 410.;
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 256.;
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.2; 
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.1; 

    auto & greenP = ctfParams.get(OCIO::LogUtil::CTFParams::green);
    greenP[OCIO::LogUtil::CTFParams::gamma]     = 3.5; 
    greenP[OCIO::LogUtil::CTFParams::refWhite]  = 620.;
    greenP[OCIO::LogUtil::CTFParams::refBlack]  = 485.;
    greenP[OCIO::LogUtil::CTFParams::highlight] = 0.7; 
    greenP[OCIO::LogUtil::CTFParams::shadow]    = 0.6; 

    auto & blueP = ctfParams.get(OCIO::LogUtil::CTFParams::blue);
    blueP[OCIO::LogUtil::CTFParams::gamma]     = 4.6; 
    blueP[OCIO::LogUtil::CTFParams::refWhite]  = 730.;
    blueP[OCIO::LogUtil::CTFParams::refBlack]  = 558.;
    blueP[OCIO::LogUtil::CTFParams::highlight] = 0.9; 
    blueP[OCIO::LogUtil::CTFParams::shadow]    = 0.7; 

    ctfParams.m_style = OCIO::LogUtil::LOG_TO_LIN;

    OCIO::LogOpData::Params paramsR, paramsG, paramsB;
    double base = 1.0;
    OCIO::TransformDirection dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);

    OCIO::LogOpData logOp(base, paramsR, paramsG, paramsB, dir);

    OCIO_CHECK_EQUAL(logOp.getType(), OCIO::OpData::LogType);

    OCIO_CHECK_ASSERT(!logOp.allComponentsEqual());
    OCIO_CHECK_EQUAL(logOp.getBase(), base);
    OCIO_CHECK_ASSERT(logOp.getRedParams() == paramsR);
    OCIO_CHECK_ASSERT(logOp.getGreenParams() == paramsG);
    OCIO_CHECK_ASSERT(logOp.getBlueParams() == paramsB);

    // Update all channels with same parameters.
    greenP = redP;
    blueP = redP;
    dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);

    OCIO::LogOpData logOp2(base, paramsR, paramsG, paramsB, dir);

    OCIO_CHECK_ASSERT(logOp2.allComponentsEqual());
    OCIO_CHECK_ASSERT(logOp2.getRedParams() == paramsR);
    OCIO_CHECK_ASSERT(logOp2.getGreenParams() == paramsR);
    OCIO_CHECK_ASSERT(logOp2.getBlueParams() == paramsR);

    // Update only red channel with new parameters.
    redP[OCIO::LogUtil::CTFParams::gamma]     = 0.6; 
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 358.;
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 115.;
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.7; 
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.3; 

    dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);

    OCIO::LogOpData logOp3(base, paramsR, paramsG, paramsB, dir);

    OCIO_CHECK_ASSERT(!logOp3.allComponentsEqual());
    OCIO_CHECK_ASSERT(logOp3.getRedParams() == paramsR);
    OCIO_CHECK_ASSERT(logOp3.getGreenParams() == paramsG);
    OCIO_CHECK_ASSERT(logOp3.getBlueParams() == paramsB);

    // Update only green channel with new parameters.
    redP = greenP;
    greenP[OCIO::LogUtil::CTFParams::gamma]     = 0.3;  
    greenP[OCIO::LogUtil::CTFParams::refWhite]  = 333.; 
    greenP[OCIO::LogUtil::CTFParams::refBlack]  = 155.; 
    greenP[OCIO::LogUtil::CTFParams::highlight] = 0.85; 
    greenP[OCIO::LogUtil::CTFParams::shadow]    = 0.111;

    dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);

    OCIO::LogOpData logOp4(base, paramsR, paramsG, paramsB, dir);
    OCIO_CHECK_ASSERT(!logOp4.allComponentsEqual());
    OCIO_CHECK_ASSERT(logOp4.getRedParams() == paramsR);
    OCIO_CHECK_ASSERT(logOp4.getGreenParams() == paramsG);
    OCIO_CHECK_ASSERT(logOp4.getBlueParams() == paramsB);

    // Update only blue channel with new parameters.
    greenP = redP;
    blueP[OCIO::LogUtil::CTFParams::gamma]     = 0.124;
    blueP[OCIO::LogUtil::CTFParams::refWhite]  = 55.;  
    blueP[OCIO::LogUtil::CTFParams::refBlack]  = 33.;  
    blueP[OCIO::LogUtil::CTFParams::highlight] = 0.27; 
    blueP[OCIO::LogUtil::CTFParams::shadow]    = 0.22; 

    dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);

    OCIO::LogOpData logOp5(base, paramsR, paramsG, paramsB, dir);
    OCIO_CHECK_ASSERT(!logOp5.allComponentsEqual());
    OCIO_CHECK_ASSERT(logOp5.getRedParams() == paramsR);
    OCIO_CHECK_ASSERT(logOp5.getGreenParams() == paramsG);
    OCIO_CHECK_ASSERT(logOp5.getBlueParams() == paramsB);

    // Initialize with base.
    const double baseVal = 2.0;
    OCIO::LogOpData logOp6(baseVal, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(logOp6.allComponentsEqual());
    auto & param = logOp6.getRedParams();
    OCIO_CHECK_EQUAL(logOp6.getBase(), baseVal);
    OCIO_CHECK_EQUAL(param[OCIO::LOG_SIDE_SLOPE], 1.0f);
    OCIO_CHECK_EQUAL(param[OCIO::LIN_SIDE_SLOPE], 1.0f);
    OCIO_CHECK_EQUAL(param[OCIO::LIN_SIDE_OFFSET], 0.0f);
    OCIO_CHECK_EQUAL(param[OCIO::LOG_SIDE_OFFSET], 0.0f);

    // Initialize with OCIO parameters.
    const double logSlope[] = { 1.5, 1.6, 1.7 };
    const double linSlope[] = { 1.1, 1.2, 1.3 };
    const double linOffset[] = { 1.0, 2.0, 3.0 };
    const double logOffset[] = { 10.0, 20.0, 30.0 };

    OCIO::LogOpData logOp7(base, logSlope, logOffset, linSlope, linOffset, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(!logOp7.allComponentsEqual());
    auto & paramR = logOp7.getRedParams();
    OCIO_CHECK_EQUAL(logOp7.getBase(), base);
    OCIO_CHECK_EQUAL(paramR[OCIO::LOG_SIDE_SLOPE], logSlope[0]);
    OCIO_CHECK_EQUAL(paramR[OCIO::LIN_SIDE_SLOPE], linSlope[0]);
    OCIO_CHECK_EQUAL(paramR[OCIO::LIN_SIDE_OFFSET], linOffset[0]);
    OCIO_CHECK_EQUAL(paramR[OCIO::LOG_SIDE_OFFSET], logOffset[0]);
    auto & paramG = logOp7.getGreenParams();
    OCIO_CHECK_EQUAL(paramG[OCIO::LOG_SIDE_SLOPE], logSlope[1]);
    OCIO_CHECK_EQUAL(paramG[OCIO::LIN_SIDE_SLOPE], linSlope[1]);
    OCIO_CHECK_EQUAL(paramG[OCIO::LIN_SIDE_OFFSET], linOffset[1]);
    OCIO_CHECK_EQUAL(paramG[OCIO::LOG_SIDE_OFFSET], logOffset[1]);
    auto & paramB = logOp7.getBlueParams();
    OCIO_CHECK_EQUAL(paramB[OCIO::LOG_SIDE_SLOPE], logSlope[2]);
    OCIO_CHECK_EQUAL(paramB[OCIO::LIN_SIDE_SLOPE], linSlope[2]);
    OCIO_CHECK_EQUAL(paramB[OCIO::LIN_SIDE_OFFSET], linOffset[2]);
    OCIO_CHECK_EQUAL(paramB[OCIO::LOG_SIDE_OFFSET], logOffset[2]);
}

OCIO_ADD_TEST(LogOpData, validation_fails_test)
{
    double base = 1.0;
    double logSlope[] = { 1.0, 1.0, 1.0 };
    double linSlope[] = { 1.0, 1.0, 1.0 };
    double linOffset[] = { 0.0, 0.0, 0.0 };
    double logOffset[] = { 0.0, 0.0, 0.0 };
    OCIO::TransformDirection direction = OCIO::TRANSFORM_DIR_FORWARD;

    // Fail invalid base.
    OCIO::LogOpData logOp1(base, logSlope, logOffset, linSlope, linOffset, direction);
    OCIO_CHECK_THROW_WHAT(logOp1.validate(), OCIO::Exception, "base cannot be 1");
    direction = OCIO::TRANSFORM_DIR_INVERSE;
    OCIO::LogOpData invlogOp1(base, logSlope, logOffset, linSlope, linOffset, direction);
    OCIO_CHECK_THROW_WHAT(invlogOp1.validate(), OCIO::Exception, "base cannot be 1");

    base = 10.0;

    // Fail invalid slope.
    direction = OCIO::TRANSFORM_DIR_FORWARD;
    linSlope[0] = linSlope[1] = linSlope[2] = 0.0;

    OCIO::LogOpData logOp2(base, logSlope, logOffset, linSlope, linOffset, direction);
    OCIO_CHECK_THROW_WHAT(logOp2.validate(), OCIO::Exception, "linear side slope cannot be 0");
    direction = OCIO::TRANSFORM_DIR_INVERSE;
    OCIO::LogOpData invlogOp2(base, logSlope, logOffset, linSlope, linOffset, direction);
    OCIO_CHECK_THROW_WHAT(invlogOp2.validate(), OCIO::Exception, "linear side slope cannot be 0");

    linSlope[0] = linSlope[1] = linSlope[2] = 1.0;

    // Fail invalid multiplier.
    direction = OCIO::TRANSFORM_DIR_FORWARD;
    logSlope[0] = logSlope[1] = logSlope[2] = 0.0;

    OCIO::LogOpData logOp3(base, logSlope, logOffset, linSlope, linOffset, direction);
    OCIO_CHECK_THROW_WHAT(logOp3.validate(), OCIO::Exception, "log side slope cannot be 0");
    direction = OCIO::TRANSFORM_DIR_INVERSE;
    OCIO::LogOpData invlogOp3(base, logSlope, logOffset, linSlope, linOffset, direction);
    OCIO_CHECK_THROW_WHAT(invlogOp3.validate(), OCIO::Exception, "log side slope cannot be 0");
}

OCIO_ADD_TEST(LogOpData, log_inverse)
{
    OCIO::LogOpData::Params paramR{ 1.5, 10.0, 1.1, 1.0 };
    OCIO::LogOpData::Params paramG{ 1.6, 20.0, 1.2, 2.0 };
    OCIO::LogOpData::Params paramB{ 1.7, 30.0, 1.3, 3.0 };
    const double base = 10.0;

    OCIO::LogOpData logOp0(base, paramR, paramG, paramB, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::ConstLogOpDataRcPtr invLogOp0 = logOp0.inverse();

    OCIO_CHECK_ASSERT(logOp0.getRedParams() == invLogOp0->getRedParams());
    OCIO_CHECK_ASSERT(logOp0.getGreenParams() == invLogOp0->getGreenParams());
    OCIO_CHECK_ASSERT(logOp0.getBlueParams() == invLogOp0->getBlueParams());

    // When components are not equals, ops are not considered inverse.
    OCIO_CHECK_ASSERT(!logOp0.isInverse(invLogOp0));

    // Using equal components.
    OCIO::LogOpData logOp1(base, paramR, paramR, paramR, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO::ConstLogOpDataRcPtr invLogOp1 = logOp1.inverse();

    OCIO_CHECK_ASSERT(logOp1.isInverse(invLogOp1));

}

OCIO_ADD_TEST(LogOpData, identity_replacement)
{
    OCIO::LogOpData::Params paramsR{ 1.5, 10.0, 2.0, 1.0 };
    const double base = 2.0;
    {
        OCIO::LogOpData logOp(base, paramsR, paramsR, paramsR, OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(logOp.getIdentityReplacement()->getType(),
                         OCIO::OpData::MatrixType);
    }
    {
        OCIO::LogOpData logOp(base, paramsR, paramsR, paramsR, OCIO::TRANSFORM_DIR_FORWARD);
        auto op = logOp.getIdentityReplacement();
        OCIO_CHECK_EQUAL(op->getType(),
                         OCIO::OpData::RangeType);
        auto r = std::dynamic_pointer_cast<OCIO::RangeOpData>(op);
        // -(1.0/2.0)
        OCIO_CHECK_EQUAL(r->getMinInValue(), -0.5);
        OCIO_CHECK_ASSERT(r->maxIsEmpty());
    }

    {
        OCIO::LogOpData logOp(2.0f, OCIO::TRANSFORM_DIR_FORWARD);
        OCIO_CHECK_EQUAL(logOp.getIdentityReplacement()->getType(),
                         OCIO::OpData::RangeType);
    }
    {
        OCIO::LogOpData logOp(2.0f, OCIO::TRANSFORM_DIR_INVERSE);
        OCIO_CHECK_EQUAL(logOp.getIdentityReplacement()->getType(),
                         OCIO::OpData::MatrixType);
    }
}

