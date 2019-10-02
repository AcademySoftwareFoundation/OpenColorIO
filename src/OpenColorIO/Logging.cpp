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
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        const char * OCIO_LOGGING_LEVEL_ENVVAR = "OCIO_LOGGING_LEVEL";
        const LoggingLevel OCIO_DEFAULT_LOGGING_LEVEL = LOGGING_LEVEL_INFO;
        
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
                    g_logginglevel = OCIO_DEFAULT_LOGGING_LEVEL;
                }
            }
            else
            {
                g_logginglevel = OCIO_DEFAULT_LOGGING_LEVEL;
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
    
    void LogWarning(const std::string & text)
    {
        AutoMutex lock(g_logmutex);
        InitLogging();
        
        if(g_logginglevel<LOGGING_LEVEL_WARNING) return;
        
        StringVec parts;
        pystring::split( pystring::rstrip(text), parts, "\n");
        
        for(unsigned int i=0; i<parts.size(); ++i)
        {
            std::cerr << "[OpenColorIO Warning]: " << parts[i] << std::endl;
        }
    }
    
    void LogInfo(const std::string & text)
    {
        AutoMutex lock(g_logmutex);
        InitLogging();
        
        if(g_logginglevel<LOGGING_LEVEL_INFO) return;
        
        StringVec parts;
        pystring::split( pystring::rstrip(text), parts, "\n");
        
        for(unsigned int i=0; i<parts.size(); ++i)
        {
            std::cerr << "[OpenColorIO Info]: " << parts[i] << std::endl;
        }
    }
    
    void LogDebug(const std::string & text)
    {
        AutoMutex lock(g_logmutex);
        InitLogging();
        
        if(g_logginglevel<LOGGING_LEVEL_DEBUG) return;
        
        StringVec parts;
        pystring::split( pystring::rstrip(text), parts, "\n");
        
        for(unsigned int i=0; i<parts.size(); ++i)
        {
            std::cerr << "[OpenColorIO Debug]: " << parts[i] << std::endl;
        }
    }
    
    bool IsDebugLoggingEnabled()
    {
        return (GetLoggingLevel()>=LOGGING_LEVEL_DEBUG);
    }
    
}
OCIO_NAMESPACE_EXIT
