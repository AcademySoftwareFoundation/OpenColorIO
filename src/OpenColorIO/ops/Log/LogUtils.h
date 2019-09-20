// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_LOGUTILS_H
#define INCLUDED_OCIO_LOGUTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include "ops/Log/LogOpData.h"

OCIO_NAMESPACE_ENTER
{
namespace LogUtil
{

// Enumeration of the CTF log styles.
enum LogStyle
{
    LOG10 = 0,  // base-10 logarithm.
    LOG2,       // base-2  logarithm.
    ANTI_LOG10, // base-10 anti-logarithm (power).
    ANTI_LOG2,  // base-2  anti-logarithm (power).
    LOG_TO_LIN, // Convert Cineon (or similar) log media to scene-linear or video.
    LIN_TO_LOG  // Convert scene-linear or video to Cineon (or similar) log media.
};

struct CTFParams
{
    CTFParams()
    {
    }

    LogStyle m_style = LOG10;

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
};

void ConvertLogParameters(const CTFParams & ctfParams,
                          double & base,
                          LogOpData::Params & redParams,
                          LogOpData::Params & greenParams,
                          LogOpData::Params & blueParams,
                          TransformDirection & dir);

}
}
OCIO_NAMESPACE_EXIT

#endif

