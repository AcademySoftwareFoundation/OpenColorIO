// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

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

void checkAllRequiredRolesErrors(LogGuard & logGuard)
{
    checkRequiredRolesErrors(logGuard);
    
    StringUtils::StringVec svec = StringUtils::SplitByLines(logGuard.output());
    OCIO_CHECK_ASSERT(
        StringUtils::Contain(
            svec, 
            "[OpenColorIO Error]: The aces_interchange role is required when there are "\
            "scene-referred color spaces and the config version is 2.2 or higher."
        )
    );
}

void checkRequiredRolesErrors(LogGuard & logGuard)
{
    StringUtils::StringVec svec = StringUtils::SplitByLines(logGuard.output());
    OCIO_CHECK_ASSERT(
        StringUtils::Contain(
            svec, 
            "[OpenColorIO Error]: The scene_linear role is required for a config version 2.2 "\
            "or higher."
        )
    );

    OCIO_CHECK_ASSERT(
        StringUtils::Contain(
            svec, 
            "[OpenColorIO Error]: The compositing_log role is required for a config version "\
            "2.2 or higher."
        )
    );

    OCIO_CHECK_ASSERT(
        StringUtils::Contain(
            svec, 
            "[OpenColorIO Error]: The color_timing role is required for a config version 2.2 "\
            "or higher."
        )
    );

    OCIO_CHECK_ASSERT(
        StringUtils::Contain(
            svec, 
            "[OpenColorIO Error]: The aces_interchange role is required when there are "\
            "scene-referred color spaces and the config version is 2.2 or higher."
        )
    );
}

} // namespace OCIO_NAMESPACE

