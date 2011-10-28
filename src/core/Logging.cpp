/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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

#include <cstdlib>
#include <iostream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "Logging.h"
#include "Mutex.h"

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
    }
    
    LoggingLevel GetLoggingLevel()
    {
        AutoMutex lock(g_logmutex);
        
        if(g_initialized == false)
        {
            g_initialized = true;
            
            char* levelstr = std::getenv(OCIO_LOGGING_LEVEL_ENVVAR);
            if(levelstr)
            {
                g_loggingOverride = true;
                g_logginglevel = LoggingLevelFromString(levelstr);
                
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
        
        return g_logginglevel;
    }
    
    void SetLoggingLevel(LoggingLevel level)
    {
        AutoMutex lock(g_logmutex);
        if(!g_loggingOverride)
        {
            g_logginglevel = level;
        }
    }
    
    void LogWarning(const std::string & text)
    {
        LoggingLevel level = GetLoggingLevel();
        
        AutoMutex lock(g_logmutex);
        if (level>=LOGGING_LEVEL_WARNING)
        {
            std::cerr << "[OpenColorIO Warning]: " << text << std::endl;
        }
    }
    
    void LogInfo(const std::string & text)
    {
        LoggingLevel level = GetLoggingLevel();
        
        AutoMutex lock(g_logmutex);
        if (level>=LOGGING_LEVEL_INFO)
        {
            std::cerr << "[OpenColorIO Info]: " << text << std::endl;
        }
    }
    
    void LogDebug(const std::string & text)
    {
        LoggingLevel level = GetLoggingLevel();
        
        AutoMutex lock(g_logmutex);
        if (level>=LOGGING_LEVEL_DEBUG)
        {
            std::cerr << "[OpenColorIO Debug]: " << text << std::endl;
        }
    }
    
    bool IsDebugLoggingEnabled()
    {
        return (GetLoggingLevel()>=LOGGING_LEVEL_DEBUG);
    }
    
}
OCIO_NAMESPACE_EXIT
