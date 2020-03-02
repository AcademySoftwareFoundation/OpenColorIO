// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LOGUTILS_H
#define INCLUDED_OCIO_LOGUTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/log/LogOpData.h"

namespace OCIO_NAMESPACE
{
namespace LogUtil
{

// Enumeration of the CTF log styles.
enum LogStyle
{
    LOG10 = 0,         // base-10 logarithm.
    LOG2,              // base-2  logarithm.
    ANTI_LOG10,        // base-10 anti-logarithm (power).
    ANTI_LOG2,         // base-2  anti-logarithm (power).
    LOG_TO_LIN,        // Convert Cineon (or similar) log media to scene-linear or video.
    LIN_TO_LOG,        // Convert scene-linear or video to Cineon (or similar) log media.
    CAMERA_LOG_TO_LIN, // Log-to-lin with linear section near black.
    CAMERA_LIN_TO_LOG  // Lin-to-log with linear section near black.
};

// Strings for CLF/CTF files.
static constexpr char LOG2_STR[] = "log2";
static constexpr char LOG10_STR[] = "log10";
static constexpr char ANTI_LOG2_STR[] = "antiLog2";
static constexpr char ANTI_LOG10_STR[] = "antiLog10";
static constexpr char LIN_TO_LOG_STR[] = "linToLog";
static constexpr char LOG_TO_LIN_STR[] = "logToLin";
static constexpr char CAMERA_LIN_TO_LOG_STR[] = "cameraLinToLog";
static constexpr char CAMERA_LOG_TO_LIN_STR[] = "cameraLogToLin";

LogStyle ConvertStringToStyle(const char * str);
const char * ConvertStyleToString(LogStyle style);

struct CTFParams
{
    CTFParams()
    {
    }

    LogStyle m_style = LOG10;

    enum Type
    {
        UNKNOWN,
        CINEON,
        CLF
    };

    typedef std::vector<double> Params;
    enum Channels
    {
        red,
        green,
        blue
    };

    enum Values
    {
        gamma,
        refWhite,
        refBlack,
        highlight,
        shadow
    };

    Params & get(Channels c)
    {
        return m_params[c];
    }

    const Params & get(Channels c) const
    {
        return m_params[c];
    }

    // red, green, blue.
    // Gamma, refWhite, refBlack, highlight, shadow.
    Params m_params[3] = { { 0., 0., 0., 0., 0. },
                           { 0., 0., 0., 0., 0. },
                           { 0., 0., 0., 0., 0. } };

    bool setType(Type type)
    {
        if (m_type == UNKNOWN)
        {
            m_type = type;
        }
        else if (type != m_type)
        {
            return false;
        }
        return true;
    }

    Type getType() const
    {
        return m_type;
    }

private:
    Type m_type = UNKNOWN;
};

void ConvertLogParameters(const CTFParams & ctfParams,
                          double & base,
                          LogOpData::Params & redParams,
                          LogOpData::Params & greenParams,
                          LogOpData::Params & blueParams);

TransformDirection GetLogDirection(LogStyle style);

float GetLinearSlope(const LogOpData::Params & params, double base);
float GetLogSideBreak(const LogOpData::Params & params, double base);
float GetLinearOffset(const LogOpData::Params & params, float linearSlope, float logSideBreak);

}
} // namespace OCIO_NAMESPACE

#endif

