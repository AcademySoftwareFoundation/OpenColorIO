// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include <iostream>
#include <regex>

#include "Platform.h"
#include "testutils/UnitTest.h"
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

// Find and remove specified line from g_output.
// Return true if found, otherwise, false.
bool LogGuard::findAndRemove(const std::string & line) const
{
    // Escape the line to prevent error in regex if the line contains regex character.
    std::string escaped_line = std::regex_replace(line, std::regex("[\\[\\](){}*+?.^$|\\\\]"), "\\$&");
    std::regex pattern(escaped_line + R"([\r\n]+)");
    std::smatch match;
    if (std::regex_search(g_output, match, pattern)) 
    {
        // If the line is found, remove it.
        auto pos = std::next(g_output.begin(), match.position());
        auto end = std::next(pos, match.length());
        g_output.erase(pos, end);
        return true;
    }
    return false;
}

bool LogGuard::findAllAndRemove(const std::string & sPattern) const
{
    bool found = false;
    std::regex pattern(sPattern);
    std::smatch match;
    while (std::regex_search(g_output, match, pattern)) 
    {
        found = true;
        auto pos = std::next(g_output.begin(), match.position());
        auto end = std::next(pos, match.length());
        g_output.erase(pos, end);
    }
    return found;
}

void LogGuard::print()
{
    std::cout << g_output;
}

MuteLogging::MuteLogging()
{
    SetLoggingFunction(&MuteLoggingFunction);
}

MuteLogging::~MuteLogging()
{
    ResetToDefaultLoggingFunction();
}

bool checkAndMuteSceneLinearRoleError(LogGuard & logGuard)
{
    const std::string interchange_scene = "[OpenColorIO Error]: The scene_linear role is "\
                                          "required for a config version 2.2 or higher.";
    return logGuard.findAndRemove(interchange_scene);
}

bool checkAndMuteCompositingLogRoleError(LogGuard & logGuard)
{
    const std::string compositing_log = "[OpenColorIO Error]: The compositing_log role is "\
                                        "required for a config version 2.2 or higher.";
    return logGuard.findAndRemove(compositing_log);
}

bool checkAndMuteColorTimingRoleError(LogGuard & logGuard)
{
    const std::string color_timing = "[OpenColorIO Error]: The color_timing role is required "\
                                     "for a config version 2.2 or higher.";
    return logGuard.findAndRemove(color_timing);
}

bool checkAndMuteAcesInterchangeRoleError(LogGuard & logGuard)
{
    const std::string aces_interchange = "[OpenColorIO Error]: The aces_interchange role is "\
                                         "required when there are scene-referred color spaces and "\
                                         "the config version is 2.2 or higher.";
    return logGuard.findAndRemove(aces_interchange);
}

bool checkAndMuteDisplayInterchangeRoleError(LogGuard & logGuard)
{
    const std::string interchange_display = "[OpenColorIO Error]: The cie_xyz_d65_interchange "\
                                            "role is required when there are display-referred "\
                                            "color spaces and the config version is 2.2 or higher.";
    return logGuard.findAndRemove(interchange_display);
}

void muteInactiveColorspaceInfo(LogGuard & logGuard)
{
    const std::string str = "- Display' is neither a color space nor a named transform.";
    const std::string pattern = R"(^\[OpenColorIO Info\]: Inactive.*)" + str + R"([\r\n]+)";
    logGuard.findAllAndRemove(pattern);
}

} // namespace OCIO_NAMESPACE

