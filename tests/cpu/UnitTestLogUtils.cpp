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

} // namespace OCIO_NAMESPACE

