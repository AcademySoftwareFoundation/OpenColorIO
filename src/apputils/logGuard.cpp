// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "logGuard.h"

static std::string g_output;
void CustomLoggingFunction(const char * message)
{
    g_output += message;
}

LogGuard::LogGuard()
    :   m_logLevel(OCIO::GetLoggingLevel())
{
    OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_DEBUG);
    OCIO::SetLoggingFunction(&CustomLoggingFunction);
}

LogGuard::~LogGuard()
{
    OCIO::ResetToDefaultLoggingFunction();
    OCIO::SetLoggingLevel(m_logLevel);
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