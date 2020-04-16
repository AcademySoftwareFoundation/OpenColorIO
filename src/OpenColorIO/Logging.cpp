// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdlib>
#include <iostream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "Logging.h"
#include "Mutex.h"
#include "Platform.h"
#include "PrivateTypes.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{
namespace
{
constexpr static const char * OCIO_LOGGING_LEVEL_ENVVAR = "OCIO_LOGGING_LEVEL";

Mutex g_logmutex;

LoggingLevel g_logginglevel = LOGGING_LEVEL_UNKNOWN;

bool g_initialized = false;
bool g_loggingOverride = false;

// You must manually acquire the logging mutex before calling this.
// This will set g_logginglevel, g_initialized, g_loggingOverride
void InitLogging()
{
    if(g_initialized) return;

    g_initialized = true;

    std::string levelstr;
    Platform::Getenv(OCIO_LOGGING_LEVEL_ENVVAR, levelstr);
    if(!levelstr.empty())
    {
        g_loggingOverride = true;
        g_logginglevel = LoggingLevelFromString(levelstr.c_str());

        if(g_logginglevel == LOGGING_LEVEL_UNKNOWN)
        {
            std::cerr << "[OpenColorIO Warning]: Invalid $OCIO_LOGGING_LEVEL specified. ";
            std::cerr << "Options: none (0), warning (1), info (2), debug (3)" << std::endl;
            g_logginglevel = LOGGING_LEVEL_DEFAULT;
        }
    }
    else
    {
        g_logginglevel = LOGGING_LEVEL_DEFAULT;
    }
}

// That's the default logging function.
void DefaultLoggingFunction(const char * message)
{
    std::cerr << message;
}

// Hold the default logging function.
LoggingFunction g_loggingFunction = DefaultLoggingFunction;

// If the message contains multiple lines, then preprocess it 
// to output the content line by line.
void LogMessage(const char * messagePrefix, const std::string & message)
{
    const StringUtils::StringVec parts
        = StringUtils::SplitByLines(StringUtils::RightTrim(message));

    for (const auto & part : parts)
    {
        std::string msg(messagePrefix);
        msg += part;
        msg += "\n";

        g_loggingFunction(msg.c_str());
    }
}

}

LoggingLevel GetLoggingLevel()
{
    AutoMutex lock(g_logmutex);
    InitLogging();

    return g_logginglevel;
}

void SetLoggingLevel(LoggingLevel level)
{
    AutoMutex lock(g_logmutex);
    InitLogging();

    // Calls to SetLoggingLevel are ignored if OCIO_LOGGING_LEVEL_ENVVAR
    // is specified.  This is to allow users to optionally debug OCIO at
    // runtime even in applications that disable logging.

    if(!g_loggingOverride)
    {
        g_logginglevel = level;
    }
}

void SetLoggingFunction(LoggingFunction logFunction)
{
    g_loggingFunction = logFunction;
}

void ResetToDefaultLoggingFunction()
{
    g_loggingFunction = DefaultLoggingFunction;
}

void LogMessage(LoggingLevel level, const char * message)
{
    switch(level)
    {
        case LOGGING_LEVEL_WARNING:
        {
            LogWarning(message);
            break;
        }
        case LOGGING_LEVEL_INFO:
        {
            LogInfo(message);
            break;
        }
        case LOGGING_LEVEL_DEBUG:
        {
            LogDebug(message);
            break;
        }
        case LOGGING_LEVEL_NONE:
        {
            // No logging.
            break;
        }
        case LOGGING_LEVEL_UNKNOWN:
        {
            throw Exception("Unsupported logging level.");
        }
    }
}

void LogWarning(const std::string & text)
{
    AutoMutex lock(g_logmutex);
    InitLogging();

    if(g_logginglevel<LOGGING_LEVEL_WARNING) return;

    LogMessage("[OpenColorIO Warning]: ", text);
}

void LogInfo(const std::string & text)
{
    AutoMutex lock(g_logmutex);
    InitLogging();

    if(g_logginglevel<LOGGING_LEVEL_INFO) return;

    LogMessage("[OpenColorIO Info]: ", text);
}

void LogDebug(const std::string & text)
{
    AutoMutex lock(g_logmutex);
    InitLogging();

    if(g_logginglevel<LOGGING_LEVEL_DEBUG) return;

    LogMessage("[OpenColorIO Debug]: ", text);
}

bool IsDebugLoggingEnabled()
{
    return (GetLoggingLevel()>=LOGGING_LEVEL_DEBUG);
}

} // namespace OCIO_NAMESPACE
