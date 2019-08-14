/*
Copyright (c) 2019 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Log/LogOpData.h"
#include "ops/Log/LogUtils.h"

OCIO_NAMESPACE_ENTER
{
namespace LogUtil
{

void ConvertFromCTFToOCIO(const CTFParams::Params & ctfParams,
                          LogOpData::Params & ocioParams)
{
    // Base is 10.0.
    static const double range = 0.002 * 1023.0;

    const double gamma     = ctfParams[CTFParams::gamma];
    const double refWhite  = ctfParams[CTFParams::refWhite] / 1023.0;
    const double refBlack  = ctfParams[CTFParams::refBlack] / 1023.0;
    const double highlight = ctfParams[CTFParams::highlight];
    const double shadow    = ctfParams[CTFParams::shadow];

    const double mult_factor = range / gamma;

    double tmp_value = (refBlack - refWhite) * mult_factor;
    // The exact clamp value is not critical.
    // RefBlack and RefWhite are never very close to one another in practice.
    // We just need to avoid a div by 0 in the gain calculation.
    tmp_value = std::min(tmp_value, -0.0001);

    const double gain = (highlight - shadow)
                        / (1. - pow(10.0, tmp_value));
    const double offset = gain - (highlight - shadow);

    ocioParams[LOG_SIDE_SLOPE]    = 1. / mult_factor;
    ocioParams[LIN_SIDE_SLOPE]    = 1. / gain;
    ocioParams[LIN_SIDE_OFFSET]   = (offset - shadow) / gain;
    ocioParams[LOG_SIDE_OFFSET]   = refWhite;
}

void ValidateParams(const CTFParams::Params & ctfParams)
{
    // Params vector is [ gamma, refWhite, refBlack, highlight, shadow ].
    static const unsigned expectedSize = 5;
    if (ctfParams.size() != expectedSize)
    {
        throw Exception("Log: Expecting 5 parameters.");
    }

    const double gamma     = ctfParams[CTFParams::gamma];
    const double refWhite  = ctfParams[CTFParams::refWhite];
    const double refBlack  = ctfParams[CTFParams::refBlack];
    const double highlight = ctfParams[CTFParams::highlight];
    const double shadow    = ctfParams[CTFParams::shadow];

    // gamma > 0.01.
    if (!(gamma > 0.01f))
    {
        std::ostringstream oss;
        oss << "Log: Invalid gamma value '";
        oss << gamma;
        oss << "', gamma should be greater than 0.01.";
        throw Exception(oss.str().c_str());
    }

    // refWhite > refBlack.
    if (!(refWhite > refBlack))
    {
        std::ostringstream oss;
        oss << "Log: Invalid refWhite '" << refWhite;
        oss << "' and refBlack '" << refBlack;
        oss << "', refWhite should be greater than refBlack.";
        throw Exception(oss.str().c_str());
    }

    // highlight > shadow.
    if (!(highlight > shadow))
    {
        std::ostringstream oss;
        oss << "Log: Invalid highlight '" << highlight;
        oss << "' and shadow '" << shadow;
        oss << "', highlight should be greater than shadow.";
        throw Exception(oss.str().c_str());
    }
}

void ConvertLogParameters(const CTFParams & ctfParams,
                          double & base,
                          LogOpData::Params & redParams,
                          LogOpData::Params & greenParams,
                          LogOpData::Params & blueParams,
                          TransformDirection & dir)
{
    redParams.resize(4);
    greenParams.resize(4);
    blueParams.resize(4);
    base = 10.0;
    redParams[LOG_SIDE_SLOPE]  = greenParams[LOG_SIDE_SLOPE]  = blueParams[LOG_SIDE_SLOPE]  = 1.;
    redParams[LIN_SIDE_SLOPE]  = greenParams[LIN_SIDE_SLOPE]  = blueParams[LIN_SIDE_SLOPE]  = 1.;
    redParams[LIN_SIDE_OFFSET] = greenParams[LIN_SIDE_OFFSET] = blueParams[LIN_SIDE_OFFSET] = 0.;
    redParams[LOG_SIDE_OFFSET] = greenParams[LOG_SIDE_OFFSET] = blueParams[LOG_SIDE_OFFSET] = 0.;
    dir = TRANSFORM_DIR_FORWARD;

    switch (ctfParams.m_style)
    {
    case LOG10:
    {
        // out = log(in) / log(10);
        // Keep default values.
        break;
    }
    case LOG2:
    {
        // out = log(in) / log(2);
        // Keep default values but base.
        base = 2.;
        break;
    }
    case ANTI_LOG10:
    {
        // out = pow(10, in)
        // Keep default values but direction.
        dir = TRANSFORM_DIR_INVERSE;
        break;
    }
    case ANTI_LOG2:
    {
        // out = pow(2, in)
        // Keep default values but direction and base.
        dir = TRANSFORM_DIR_INVERSE;
        base = 2.;
        break;
    }
    case LIN_TO_LOG:
        // out = k3 * log(m3 * in + b3) / log(base3) + kb3
        // Keep base and direction to default values,
    case LOG_TO_LIN:
        // out = ( pow(base3, (in - kb3) / k3) - b3 ) / m3
        // Keep base to default values.
    {
        if (ctfParams.m_style == LOG_TO_LIN)
        {
            dir = TRANSFORM_DIR_INVERSE;
        }
        ValidateParams(ctfParams.get(CTFParams::red));
        ValidateParams(ctfParams.get(CTFParams::green));
        ValidateParams(ctfParams.get(CTFParams::blue));
        ConvertFromCTFToOCIO(ctfParams.get(CTFParams::red), redParams);
        ConvertFromCTFToOCIO(ctfParams.get(CTFParams::green), greenParams);
        ConvertFromCTFToOCIO(ctfParams.get(CTFParams::blue), blueParams);
        break;
    }
    }
}
}
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "UnitTest.h"

OCIO_ADD_TEST(LogUtil, ctf_to_ocio_fail)
{
    OCIO::LogUtil::CTFParams ctfParams;
    ctfParams.m_style = OCIO::LogUtil::LOG_TO_LIN;

    OCIO::LogOpData::Params paramsR, paramsG, paramsB;
    double base = 1.0;
    OCIO::TransformDirection dir;

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
        OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir),
        OCIO::Exception, "gamma should be greater than 0.01");

    redP[OCIO::LogUtil::CTFParams::gamma]     = 0.9;
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 375.; // invalid
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 375.; // invalid
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.8;
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.5;

    OCIO_CHECK_THROW_WHAT(
        OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir),
        OCIO::Exception, "refWhite should be greater than refBlack");

    redP[OCIO::LogUtil::CTFParams::gamma]     = 0.9;
    redP[OCIO::LogUtil::CTFParams::refWhite]  = 375.;
    redP[OCIO::LogUtil::CTFParams::refBlack]  = 140.;
    redP[OCIO::LogUtil::CTFParams::highlight] = 0.5; // invalid
    redP[OCIO::LogUtil::CTFParams::shadow]    = 0.5; // invalid

    OCIO_CHECK_THROW_WHAT(
        OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir),
        OCIO::Exception, "highlight should be greater than shadow");

}

OCIO_ADD_TEST(LogUtil, ctf_to_ocio_ok)
{
    OCIO::LogUtil::CTFParams ctfParams;
    ctfParams.m_style = OCIO::LogUtil::LOG10;

    OCIO::LogOpData::Params paramsR, paramsG, paramsB;
    double base = 1.0;
    OCIO::TransformDirection dir;
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);

    OCIO_CHECK_EQUAL(base, 10.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::LogOpData logOp(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, dir,
                          base, paramsR, paramsG, paramsB);

    OCIO_CHECK_ASSERT(!logOp.isIdentity());
    OCIO_CHECK_ASSERT(!logOp.hasChannelCrosstalk());
    OCIO_CHECK_NO_THROW(logOp.validate());

    ctfParams.m_style = OCIO::LogUtil::LOG2;
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);

    OCIO_CHECK_EQUAL(base, 2.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_FORWARD);

    OCIO::LogOpData logOp2(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, dir,
                           base, paramsR, paramsG, paramsB);
    OCIO_CHECK_NO_THROW(logOp2.validate());

    ctfParams.m_style = OCIO::LogUtil::ANTI_LOG10;
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);

    OCIO_CHECK_EQUAL(base, 10.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::LogOpData logOp3(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, dir,
                           base, paramsR, paramsG, paramsB);
    OCIO_CHECK_NO_THROW(logOp3.validate());

    ctfParams.m_style = OCIO::LogUtil::ANTI_LOG2;
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);

    OCIO_CHECK_EQUAL(base, 2.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_SLOPE], 1.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LIN_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(paramsR[OCIO::LOG_SIDE_OFFSET], 0.);
    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::LogOpData logOp4(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, dir,
                           base, paramsR, paramsG, paramsB);
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
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);

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

    OCIO::LogOpData logOp5(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, dir,
                           base, paramsR, paramsG, paramsB);
    OCIO_CHECK_NO_THROW(logOp5.validate());

    ctfParams.m_style = OCIO::LogUtil::LOG_TO_LIN;
    OCIO::LogUtil::ConvertLogParameters(ctfParams, base, paramsR, paramsG, paramsB, dir);
    OCIO_CHECK_EQUAL(dir, OCIO::TRANSFORM_DIR_INVERSE);

    OCIO::LogOpData logOp6(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32, dir,
                           base, paramsR, paramsG, paramsB);
    OCIO_CHECK_NO_THROW(logOp6.validate());
}

#endif