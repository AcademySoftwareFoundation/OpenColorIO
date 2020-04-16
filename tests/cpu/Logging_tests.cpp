// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


// Have access to the source code to test.
#include "Logging.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestLogUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Logging, message_function)
{
    OCIO_CHECK_EQUAL(OCIO::GetLoggingLevel(), OCIO::LOGGING_LEVEL_DEFAULT);

    constexpr char dummyStr[]{"Dummy message"};

    OCIO::LogGuard guard;

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_NONE);

        OCIO::LogDebug(dummyStr);
        OCIO_CHECK_ASSERT(guard.empty());

        OCIO::LogInfo(dummyStr);
        OCIO_CHECK_ASSERT(guard.empty());

        OCIO::LogWarning(dummyStr);
        OCIO_CHECK_ASSERT(guard.empty());

        OCIO_CHECK_ASSERT(!OCIO::IsDebugLoggingEnabled());
    }

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);

        OCIO::LogDebug(dummyStr);
        OCIO_CHECK_ASSERT(guard.empty());

        OCIO::LogInfo(dummyStr);
        OCIO_CHECK_ASSERT(guard.empty());

        OCIO::LogWarning(dummyStr);
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Warning]: Dummy message\n");
        guard.clear();

        OCIO::LogWarning("");
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Warning]: \n");
        guard.clear();

        OCIO_CHECK_ASSERT(!OCIO::IsDebugLoggingEnabled());
    }

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_INFO);

        OCIO::LogDebug(dummyStr);
        OCIO_CHECK_ASSERT(guard.empty());

        OCIO::LogInfo(dummyStr);
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Info]: Dummy message\n");
        guard.clear();

        OCIO::LogWarning(dummyStr);
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Warning]: Dummy message\n");
        guard.clear();

        OCIO_CHECK_ASSERT(!OCIO::IsDebugLoggingEnabled());
    }

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_DEBUG);

        OCIO::LogDebug(dummyStr);
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Debug]: Dummy message\n");
        guard.clear();

        OCIO::LogInfo(dummyStr);
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Info]: Dummy message\n");
        guard.clear();

        OCIO::LogWarning(dummyStr);
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Warning]: Dummy message\n");
        guard.clear();

        OCIO_CHECK_ASSERT(OCIO::IsDebugLoggingEnabled());
    }

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_UNKNOWN);

        OCIO::LogDebug(dummyStr);
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Debug]: Dummy message\n");
        guard.clear();

        OCIO::LogInfo(dummyStr);
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Info]: Dummy message\n");
        guard.clear();

        OCIO::LogWarning(dummyStr);
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Warning]: Dummy message\n");
        guard.clear();

        OCIO_CHECK_ASSERT(OCIO::IsDebugLoggingEnabled());
    }

    // Validate multi-line messages.

    {
        OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_DEBUG);

        OCIO::LogDebug(std::string("My first msg\nMy second msg\nMy third msg"));
        OCIO_CHECK_EQUAL(guard.output(), "[OpenColorIO Debug]: My first msg\n"\
                                         "[OpenColorIO Debug]: My second msg\n"\
                                         "[OpenColorIO Debug]: My third msg\n");
    }
}
