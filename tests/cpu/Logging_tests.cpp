/*
Copyright (c) 2019 Autodesk Inc., et al.
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


// Have access to the source code to test.
#include "Logging.cpp"


#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


namespace
{

std::string output;

// The custom logging function for testing.
void CustomLoggingFunction(const char * message)
{
    output += message;
}

// Preserve the default default log settings.
class LogGuard
{
public:
    LogGuard()
    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_DEFAULT);
        OCIO::SetLoggingFunction(CustomLoggingFunction);
        output.clear();
    }

    ~LogGuard()
    {
        OCIO::ResetToDefaultLoggingFunction();
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_DEFAULT);
    }

    LogGuard(const LogGuard &) = delete;
    LogGuard & operator=(const LogGuard &) = delete;
};


}


OCIO_ADD_TEST(Logging, message_function)
{
    OCIO_CHECK_EQUAL(OCIO::GetLoggingLevel(), OCIO::LOGGING_LEVEL_DEFAULT);

    constexpr const char dummyStr[]{"Dummy message"};

    LogGuard guard;

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_NONE);

        OCIO::LogDebug(dummyStr);
        OCIO_CHECK_ASSERT(output.empty());

        OCIO::LogInfo(dummyStr);
        OCIO_CHECK_ASSERT(output.empty());

        OCIO::LogWarning(dummyStr);
        OCIO_CHECK_ASSERT(output.empty());

        OCIO_CHECK_ASSERT(!OCIO::IsDebugLoggingEnabled());
    }

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);

        OCIO::LogDebug(dummyStr);
        OCIO_CHECK_ASSERT(output.empty());

        OCIO::LogInfo(dummyStr);
        OCIO_CHECK_ASSERT(output.empty());

        OCIO::LogWarning(dummyStr);
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Warning]: Dummy message\n");
        output.clear();

        OCIO::LogWarning("");
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Warning]: \n");
        output.clear();

        OCIO_CHECK_ASSERT(!OCIO::IsDebugLoggingEnabled());
    }

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_INFO);

        OCIO::LogDebug(dummyStr);
        OCIO_CHECK_ASSERT(output.empty());

        OCIO::LogInfo(dummyStr);
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Info]: Dummy message\n");
        output.clear();

        OCIO::LogWarning(dummyStr);
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Warning]: Dummy message\n");
        output.clear();

        OCIO_CHECK_ASSERT(!OCIO::IsDebugLoggingEnabled());
    }

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_DEBUG);

        OCIO::LogDebug(dummyStr);
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Debug]: Dummy message\n");
        output.clear();

        OCIO::LogInfo(dummyStr);
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Info]: Dummy message\n");
        output.clear();

        OCIO::LogWarning(dummyStr);
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Warning]: Dummy message\n");
        output.clear();

        OCIO_CHECK_ASSERT(OCIO::IsDebugLoggingEnabled());
    }

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_UNKNOWN);

        OCIO::LogDebug(dummyStr);
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Debug]: Dummy message\n");
        output.clear();

        OCIO::LogInfo(dummyStr);
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Info]: Dummy message\n");
        output.clear();

        OCIO::LogWarning(dummyStr);
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Warning]: Dummy message\n");
        output.clear();

        OCIO_CHECK_ASSERT(OCIO::IsDebugLoggingEnabled());
    }

    // Validate multi-line messages.

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_DEBUG);

        OCIO::LogDebug(std::string("My first msg\nMy second msg\nMy third msg"));
        OCIO_CHECK_EQUAL(output, "[OpenColorIO Debug]: My first msg\n"\
                                 "[OpenColorIO Debug]: My second msg\n"\
                                 "[OpenColorIO Debug]: My third msg\n");
    }
}
