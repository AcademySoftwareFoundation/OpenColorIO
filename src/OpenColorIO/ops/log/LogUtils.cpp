// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cmath>

#include <OpenColorIO/OpenColorIO.h>

#include "ops/log/LogOpData.h"
#include "ops/log/LogUtils.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{
namespace LogUtil
{

LogStyle ConvertStringToStyle(const char * str)
{
    if (str && *str)
    {
        if (0 == Platform::Strcasecmp(str, LOG10_STR))
        {
            return LogUtil::LOG10;
        }
        else if (0 == Platform::Strcasecmp(str, LOG2_STR))
        {
            return LogUtil::LOG2;
        }
        else if (0 == Platform::Strcasecmp(str, ANTI_LOG10_STR))
        {
            return LogUtil::ANTI_LOG10;
        }
        else if (0 == Platform::Strcasecmp(str, ANTI_LOG2_STR))
        {
            return LogUtil::ANTI_LOG2;
        }
        else if (0 == Platform::Strcasecmp(str, LOG_TO_LIN_STR))
        {
            return LogUtil::LOG_TO_LIN;
        }
        else if (0 == Platform::Strcasecmp(str, LIN_TO_LOG_STR))
        {
            return LogUtil::LIN_TO_LOG;
        }
        else if (0 == Platform::Strcasecmp(str, CAMERA_LOG_TO_LIN_STR))
        {
            return LogUtil::CAMERA_LOG_TO_LIN;
        }
        else if (0 == Platform::Strcasecmp(str, CAMERA_LIN_TO_LOG_STR))
        {
            return LogUtil::CAMERA_LIN_TO_LOG;
        }
        else
        {
            std::stringstream ss("Unknown Log style: '");
            ss << str << "'.";
            throw Exception(ss.str().c_str());
        }
    }

    throw Exception("Missing Log style.");
}

const char * ConvertStyleToString(LogStyle style)
{
    switch (style)
    {
    case LOG10:
        return LOG10_STR;
    case LOG2:
        return LOG2_STR;
    case ANTI_LOG10:
        return ANTI_LOG10_STR;
    case ANTI_LOG2:
        return ANTI_LOG2_STR;
    case LOG_TO_LIN:
        return LOG_TO_LIN_STR;
    case LIN_TO_LOG:
        return LIN_TO_LOG_STR;
    case CAMERA_LOG_TO_LIN:
        return CAMERA_LOG_TO_LIN_STR;
    case CAMERA_LIN_TO_LOG:
        return CAMERA_LIN_TO_LOG_STR;
    }

    std::stringstream ss("Unknown Log style: ");
    ss << style;

    throw Exception(ss.str().c_str());
}


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

void ValidateLegacyParams(const CTFParams::Params & ctfParams)
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
                          LogOpData::Params & blueParams)
{
    redParams.resize(4);
    greenParams.resize(4);
    blueParams.resize(4);
    redParams[LOG_SIDE_SLOPE]  = greenParams[LOG_SIDE_SLOPE]  = blueParams[LOG_SIDE_SLOPE]  = 1.;
    redParams[LIN_SIDE_SLOPE]  = greenParams[LIN_SIDE_SLOPE]  = blueParams[LIN_SIDE_SLOPE]  = 1.;
    redParams[LIN_SIDE_OFFSET] = greenParams[LIN_SIDE_OFFSET] = blueParams[LIN_SIDE_OFFSET] = 0.;
    redParams[LOG_SIDE_OFFSET] = greenParams[LOG_SIDE_OFFSET] = blueParams[LOG_SIDE_OFFSET] = 0.;

    switch (ctfParams.m_style)
    {
    case LOG10:
    {
        // out = log(in) / log(10);
        // Keep default values.
        base = 10.0;
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
        base = 10.0;
        break;
    }
    case ANTI_LOG2:
    {
        // out = pow(2, in)
        // Keep default values but direction and base.
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
        base = 10.0;
        ValidateLegacyParams(ctfParams.get(CTFParams::red));
        ValidateLegacyParams(ctfParams.get(CTFParams::green));
        ValidateLegacyParams(ctfParams.get(CTFParams::blue));
        ConvertFromCTFToOCIO(ctfParams.get(CTFParams::red), redParams);
        ConvertFromCTFToOCIO(ctfParams.get(CTFParams::green), greenParams);
        ConvertFromCTFToOCIO(ctfParams.get(CTFParams::blue), blueParams);
        break;
    }
    case CAMERA_LIN_TO_LOG:
    case CAMERA_LOG_TO_LIN:
        // Should not be used for new style.
        break;
    }
}

TransformDirection GetLogDirection(LogStyle style)
{
    switch (style)
    {
    case LOG10:
    case LOG2:
    case LIN_TO_LOG:
    case CAMERA_LIN_TO_LOG:
        return TRANSFORM_DIR_FORWARD;
    case ANTI_LOG10:
    case ANTI_LOG2:
    case LOG_TO_LIN:
    case CAMERA_LOG_TO_LIN:
        return TRANSFORM_DIR_INVERSE;
    }
    return TRANSFORM_DIR_FORWARD;
}

float GetLinearSlope(const LogOpData::Params & params, double base)
{
    // If value is defined, use it, else compute value.
    if (params.size() > LINEAR_SLOPE)
    {
        return (float)params[LINEAR_SLOPE];
    }
    else
    {
        return (float)(params[LOG_SIDE_SLOPE] * params[LIN_SIDE_SLOPE] /
                       ( (params[LIN_SIDE_SLOPE] * params[LIN_SIDE_BREAK] +
                          params[LIN_SIDE_OFFSET] ) * log(base) ));
    }
}

float GetLogSideBreak(const LogOpData::Params & params, double base)
{
    // out = ( logSlope * log( base, (linBreak*linSlope + linOffset) ) + logOffset )
    //
    // out = log2(linBreak*linSlope + linOffset) * logSlope / log2(base) + logOffset

    float logSideBreak = log2((float)(params[LIN_SIDE_SLOPE] * params[LIN_SIDE_BREAK] + params[LIN_SIDE_OFFSET]));
    logSideBreak *= (float)params[LOG_SIDE_SLOPE] / log2((float)base);
    logSideBreak += (float)params[LOG_SIDE_OFFSET];

    return logSideBreak;
}

float GetLinearOffset(const LogOpData::Params & params, float linearSlope, float logSideBreak)
{
    return logSideBreak - linearSlope * (float)params[LIN_SIDE_BREAK];
}

}
} // namespace OCIO_NAMESPACE

