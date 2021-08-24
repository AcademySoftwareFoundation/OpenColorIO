/*
OFX Support Library, a library that skins the OFX plug-in API with C++ classes.
Copyright (C) 2004-2005 The Open Effects Association Ltd
Author Bruno Nicoletti bruno@thefoundry.co.uk

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name The Open Effects Association Ltd, nor the names of its 
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The Open Effects Association Ltd
1 Wardour St
London W1D 6PA
England


*/

/** @file This file contains the body of functions used for logging ofx problems etc...

The log file is written to using printf style functions, rather than via c++ iostreams.

*/

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <string>

#include "ofxsLog.h"

namespace OFX {
  namespace Log {

    /** @brief log file */
    static FILE *gLogFP = 0;

    /// environment variable for the log file
#define kLogFileEnvVar "OFX_PLUGIN_LOGFILE"

    /** @brief the global logfile name */
    static std::string gLogFileName(getenv(kLogFileEnvVar) ? getenv(kLogFileEnvVar) : "ofxTestLog.txt");

    /** @brief global indent level, not MP sane */
    static int gIndent = 0;

    /** @brief Sets the name of the log file. */
    void setFileName(const std::string &value)
    {
      gLogFileName = value;
    }

    /** @brief Opens the log file, returns whether this was sucessful or not. */
    bool open(void)
    {
#ifdef DEBUG
      if(!gLogFP) {
        gLogFP = fopen(gLogFileName.c_str(), "w");
        return gLogFP != 0;
      }
#endif
      return gLogFP != 0;
    }

    /** @brief Closes the log file. */
    void close(void)
    {
      if(gLogFP) {
        fclose(gLogFP);
      }
      gLogFP = 0;
    }

    /** @brief Indent it, not MP sane at the moment */
    void indent(void)
    {
      ++gIndent;
    }

    /** @brief Outdent it, not MP sane at the moment */
    void outdent(void)
    {
      --gIndent;
    }

    /** @brief do the indenting */
    static void doIndent(void)
    {
      if(open()) {
        for(int i = 0; i < gIndent; i++) {
          fputs("    ", gLogFP);
        }
      }
    }

    /** @brief Prints to the log file. */
    void print(const char *format, ...)
    {
      if(open()) {
        doIndent();
        va_list args;
        va_start(args, format);
        vfprintf(gLogFP, format, args);
        fputc('\n', gLogFP);
        fflush(gLogFP);
        va_end(args);
      }  
    }

    /** @brief Prints to the log file only if the condition is true and prepends a warning notice. */
    void warning(bool condition, const char *format, ...)
    {
      if(condition && open()) {
        doIndent();
        fputs("WARNING : ", gLogFP);

        va_list args;
        va_start(args, format);
        vfprintf(gLogFP, format, args);
        fputc('\n', gLogFP);
        va_end(args);

        fflush(gLogFP);
      }  
    }

    /** @brief Prints to the log file only if the condition is true and prepends an error notice. */
    void error(bool condition, const char *format, ...)
    {
      if(condition && open()) {
        doIndent();
        fputs("ERROR : ", gLogFP);

        va_list args;
        va_start(args, format);
        vfprintf(gLogFP, format, args);
        fputc('\n', gLogFP);
        va_end(args);

        fflush(gLogFP);
      }  
    }
  };
};
