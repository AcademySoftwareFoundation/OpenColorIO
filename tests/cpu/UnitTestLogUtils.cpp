// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include <iostream>
#include "Platform.h"
#include "UnitTestLogUtils.h"

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


void checkAndRemove(std::vector<std::string> & svec, const std::string & str)
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

    OCIO_CHECK_GT(-1, index);
    svec.erase(svec.begin() + index);
}

void checkRequiredRolesErrors(StringUtils::StringVec & svec, bool printLog)
{
    const std::string interchange_scene = "[OpenColorIO Error]: The scene_linear role is "\
                                          "required for a config version 2.2 or higher.";
    const std::string compositing_log = "[OpenColorIO Error]: The compositing_log role is "\
                                        "required for a config version 2.2 or higher.";

    const std::string color_timing = "[OpenColorIO Error]: The color_timing role is required "\
                                     "for a config version 2.2 or higher.";

    const std::string aces_interchange = "[OpenColorIO Error]: The aces_interchange role is "\
                                         "required when there are scene-referred color spaces and "\
                                         "the config version is 2.2 or higher.";
    
    checkAndRemove(svec, interchange_scene);
    checkAndRemove(svec, compositing_log);
    checkAndRemove(svec, color_timing);
    checkAndRemove(svec, aces_interchange);
    if (printLog)
    {
        StringUtils::StringVec::iterator iter = svec.begin();
        for(iter; iter < svec.end(); iter++)
        {
            std::cout << *iter << std::endl;
        }
    }
}

void checkRequiredRolesErrors(LogGuard & logGuard, bool printLog)
{
    StringUtils::StringVec svec = StringUtils::SplitByLines(logGuard.output());
    checkRequiredRolesErrors(svec, printLog);
}

void checkAllRequiredRolesErrors(LogGuard & logGuard, bool printLog)
{
    StringUtils::StringVec svec = StringUtils::SplitByLines(logGuard.output());
    const std::string interchange_display = "[OpenColorIO Error]: The cie_xyz_d65_interchange "\
                                            "role is required when there are display-referred "\
                                            "color spaces and the config version is 2.2 or higher.";
    
    checkAndRemove(svec, interchange_display);
    checkRequiredRolesErrors(svec, true);
}

} // namespace OCIO_NAMESPACE

