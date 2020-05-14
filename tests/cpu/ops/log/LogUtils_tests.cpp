// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ops/log/LogUtils.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(LogUtil, ctf_to_ocio_fail)
{
    OCIO::LogUtil::CTFParams ctfParams;
    ctfParams.m_style = OCIO::LogUtil::LOG_TO_LIN;

    OCIO::LogOpData::Params paramsR, paramsG, paramsB;
    double base = 1.0;

    auto & redP = ctfParams.get(OCIO::LogUtil::CTFParams::red);
    auto & greenP = ctfParams.get(OCIO::LogUtil::CTFParams::green);
    auto & blueP = ctfParams.get(OCIO::LogUtil::CTFParams::blue);

    redP[OCIO::LogUtil::CTFParams::gamma]     = 0.005;  // invalid
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 375.;
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 140.;
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.8;
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.5;   
    greenP = redP;
    blueP = redP;

    OCIO_CHECK_THROW_WHAT( 
        OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB),
        OCIO::Exception, "gamma should be greater than 0.01");

    redP[OCIO::LogUtil::CTFParams::gamma]     = 0.9;
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 375.; // invalid
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 375.; // invalid
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.8;
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.5;

    OCIO_CHECK_THROW_WHAT(
        OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB),
        OCIO::Exception, "refWhite should be greater than refBlack");

    redP[OCIO::LogUtil::CTFParams::gamma]     = 0.9;
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 375.;
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 140.;
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.5; // invalid
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.5; // invalid

    OCIO_CHECK_THROW_WHAT(
        OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB),
        OCIO::Exception, "highlight should be greater than shadow");

}

OCIO_ADD_TEST(LogUtil, ctf_to_ocio_ok)
{
    OCIO::LogUtil::CTFParams ctfParams;
    ctfParams.m_style = OCIO::LogUtil::LOG10;

    OCIO::LogOpData::Params paramsR, paramsG, paramsB;
    double base = 1.0;
    OCIO::TransformDirection dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);

    OCIO_CHECK_EQUAL(base, 10.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::LogOpData logOp(base, paramsR, paramsG, paramsB, dir);

    OCIO_CHECK_ASSERT(!logOp.isIdentity());
    OCIO_CHECK_ASSERT(!logOp.hasChannelCrosstalk());
    OCIO_CHECK_NO_THROW(logOp.validate());

    ctfParams.m_style = OCIO::LogUtil::LOG2;
    dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);

    OCIO_CHECK_EQUAL(base, 2.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::LogOpData logOp2(base, paramsR, paramsG, paramsB, dir);
    OCIO_CHECK_NO_THROW(logOp2.validate());

    ctfParams.m_style = OCIO::LogUtil::ANTI_LOG10;
    dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);

    OCIO_CHECK_EQUAL(base, 10.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::LogOpData logOp3(base, paramsR, paramsG, paramsB, dir);
    OCIO_CHECK_NO_THROW(logOp3.validate());

    ctfParams.m_style = OCIO::LogUtil::ANTI_LOG2;
    dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);

    OCIO_CHECK_EQUAL(base, 2.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::LogOpData logOp4(base, paramsR, paramsG, paramsB, dir);
    OCIO_CHECK_NO_THROW(logOp4.validate());

    auto & redP = ctfParams.get(OCIO::LogUtil::CTFParams::red);
    redP[OCIO::LogUtil::CTFParams::gamma]     = 4.6;
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 758.;
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 30.;
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.7;
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.4;

    auto & greenP = ctfParams.get(OCIO::LogUtil::CTFParams::green);
    greenP[OCIO::LogUtil::CTFParams::gamma]     = 2.6;
    greenP[OCIO::LogUtil::CTFParams::refWhite]  = 300.;
    greenP[OCIO::LogUtil::CTFParams::refBlack]  = 42.;
    greenP[OCIO::LogUtil::CTFParams::highlight] = 0.8;
    greenP[OCIO::LogUtil::CTFParams::shadow]    = 0.1;

    auto & blueP = ctfParams.get(OCIO::LogUtil::CTFParams::blue);
    blueP = redP;

    ctfParams.m_style = OCIO::LogUtil::LIN_TO_LOG;
    dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);

    const double tol = 1e-6;
    OCIO_CHECK_EQUAL(base, 10.);
    OCIO_CHECK_CLOSE(paramsR[OCIO::LOG_SIDE_SLOPE], 2.2482893, tol);
    OCIO_CHECK_CLOSE(paramsR[OCIO::LIN_SIDE_SLOPE], 1.7250706, tol);
    OCIO_CHECK_CLOSE(paramsR[OCIO::LIN_SIDE_OFFSET], -0.2075494, tol);
    OCIO_CHECK_CLOSE(paramsR[OCIO::LOG_SIDE_OFFSET], 0.7409580, tol);

    OCIO_CHECK_CLOSE(paramsG[OCIO::LOG_SIDE_SLOPE], 1.2707722, tol);
    OCIO_CHECK_CLOSE(paramsG[OCIO::LIN_SIDE_SLOPE], 0.5240051, tol);
    OCIO_CHECK_CLOSE(paramsG[OCIO::LIN_SIDE_OFFSET], 0.5807959, tol);
    OCIO_CHECK_CLOSE(paramsG[OCIO::LOG_SIDE_OFFSET], 0.2932551, tol);

    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::LogOpData logOp5(base, paramsR, paramsG, paramsB, dir);
    OCIO_CHECK_NO_THROW(logOp5.validate());

    ctfParams.m_style = OCIO::LogUtil::LOG_TO_LIN;
    dir = OCIO::LogUtil::GetLogDirection(ctfParams.m_style);
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB);
    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::LogOpData logOp6(base, paramsR, paramsG, paramsB, dir);
    OCIO_CHECK_NO_THROW(logOp6.validate());
}

