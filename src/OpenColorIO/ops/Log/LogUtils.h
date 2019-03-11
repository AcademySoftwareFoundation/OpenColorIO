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

