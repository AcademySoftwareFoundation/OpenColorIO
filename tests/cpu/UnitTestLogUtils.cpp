// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include <iostream>
#include "Platform.h"
#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{

void MuteLoggingFunction(const char *)
{
    // Does nothing on purpose.
}

static std::string g_output;
void CustomLoggingFunction(const char * message)
{
    g_output += message;
}

} // anon.

LogGuard::LogGuard()
    :   m_logLevel(GetLoggingLevel())
{
    SetLoggingLevel(LOGGING_LEVEL_DEBUG);
    SetLoggingFunction(&CustomLoggingFunction);
}

LogGuard::~LogGuard()
{
    ResetToDefaultLoggingFunction();
    SetLoggingLevel(m_logLevel);
    g_output.clear();
}

const std::string & LogGuard::output() const
{
    return g_output;
}

void LogGuard::clear()
{
    g_output.clear();
}

bool LogGuard::empty() const
{
    return g_output.empty();
}

MuteLogging::MuteLogging()
{
    SetLoggingFunction(&MuteLoggingFunction);
}

MuteLogging::~MuteLogging()
{
    ResetToDefaultLoggingFunction();
}

// Check and remove str from vector of string if the str is found.
// Return true if found, otherwise, false.
bool checkAndRemove(std::vector<std::string> & svec, const std::string & str)
{
    size_t index = -1;
    size_t i = -1;
    for (size_t i = 0; i < svec.size(); i++)
    {
        if (Platform::Strcasecmp(svec[i].c_str(), str.c_str()) == 0)
        {
            index = i;
            break;
        }
    }

    // Expecting a 0 since the str is expected to be found.
    if (index != -1)
    {
        // found the string in the vector.
        svec.erase(svec.begin() + index);
        return true;
    }

    return false;
}

bool checkAndMuteInterchangeSceneRoleWarning(StringUtils::StringVec & svec)
{
    const std::string interchange_scene = "[OpenColorIO Error]: The scene_linear role is "\
                                          "required for a config version 2.2 or higher.";
    return checkAndRemove(svec, interchange_scene);
}

bool checkAndMuteCompositingLogRoleWarning(StringUtils::StringVec & svec)
{
    const std::string compositing_log = "[OpenColorIO Error]: The compositing_log role is "\
                                        "required for a config version 2.2 or higher.";
    return checkAndRemove(svec, compositing_log);
}

bool checkAndMuteColorTimingRoleWarning(StringUtils::StringVec & svec)
{
    const std::string color_timing = "[OpenColorIO Error]: The color_timing role is required "\
                                     "for a config version 2.2 or higher.";
    return checkAndRemove(svec, color_timing);
}

bool checkAndMuteAcesInterchangeRoleWarning(StringUtils::StringVec & svec)
{
    const std::string aces_interchange = "[OpenColorIO Error]: The aces_interchange role is "\
                                         "required when there are scene-referred color spaces and "\
                                         "the config version is 2.2 or higher.";
    return checkAndRemove(svec, aces_interchange);
}

bool checkAndMuteInterchangeDisplayRoleWarning(StringUtils::StringVec & svec)
{
    const std::string interchange_display = "[OpenColorIO Error]: The cie_xyz_d65_interchange "\
                                            "role is required when there are display-referred "\
                                            "color spaces and the config version is 2.2 or higher.";
    return checkAndRemove(svec, interchange_display);
}

void printVectorOfLog(StringUtils::StringVec & svec)
{
    StringUtils::StringVec::iterator iter = svec.begin();
    for(iter; iter < svec.end(); iter++)
    {
        std::cout << *iter << std::endl;
    }
}

} // namespace OCIO_NAMESPACE

