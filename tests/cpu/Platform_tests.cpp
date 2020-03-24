// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>
#include <set>

#include "Platform.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Platform, envVariable)
{
    // Only validates the public API.
    // Complete validations are done below using the private methods.

    const char * path = OCIO::GetEnvVariable("PATH");
    OCIO_CHECK_ASSERT(path && *path);

    OCIO::SetEnvVariable("MY_DUMMY_ENV", "SomeValue");
    const char * value = OCIO::GetEnvVariable("MY_DUMMY_ENV");
    OCIO_CHECK_ASSERT(value && *value);
    OCIO_CHECK_EQUAL(std::string(value), "SomeValue");

    OCIO::SetEnvVariable("MY_DUMMY_ENV", "");
    value = OCIO::GetEnvVariable("MY_DUMMY_ENV");
    OCIO_CHECK_ASSERT(!value || !*value);
}

OCIO_ADD_TEST(Platform, getenv)
{
    std::string env;
    OCIO::Platform::Getenv("NotExistingEnvVariable", env);
    OCIO_CHECK_ASSERT(env.empty());

    OCIO::Platform::Getenv("PATH", env);
    OCIO_CHECK_ASSERT(!env.empty());

    OCIO::Platform::Getenv("NotExistingEnvVariable", env);
    OCIO_CHECK_ASSERT(env.empty());

    OCIO::Platform::Getenv("PATH", env);
    OCIO_CHECK_ASSERT(!env.empty());
}

OCIO_ADD_TEST(Platform, setenv)
{
    {
        OCIO::Platform::Setenv("MY_DUMMY_ENV", "SomeValue");
        std::string env;
        OCIO::Platform::Getenv("MY_DUMMY_ENV", env);
        OCIO_CHECK_ASSERT(!env.empty());

        OCIO_CHECK_ASSERT(0==std::strcmp("SomeValue", env.c_str()));
        OCIO_CHECK_EQUAL(strlen("SomeValue"), env.size());
    }
    {
        OCIO::Platform::Setenv("MY_DUMMY_ENV", " ");
        std::string env;
        OCIO::Platform::Getenv("MY_DUMMY_ENV", env);
        OCIO_CHECK_ASSERT(!env.empty());

        OCIO_CHECK_ASSERT(0==std::strcmp(" ", env.c_str()));
        OCIO_CHECK_EQUAL(std::strlen(" "), env.size());
    }
    {
        OCIO::Platform::Setenv("MY_DUMMY_ENV", "");
        std::string env;
        OCIO::Platform::Getenv("MY_DUMMY_ENV", env);
        OCIO_CHECK_ASSERT(env.empty());
    }
#ifdef _WIN32
    {
        SetEnvironmentVariable("MY_WINDOWS_DUMMY_ENV", "1");
        std::string env;
        OCIO::Platform::Getenv("MY_WINDOWS_DUMMY_ENV", env);
        OCIO_CHECK_EQUAL(env, std::string("1"));
    }
    {
        SetEnvironmentVariable("MY_WINDOWS_DUMMY_ENV", " ");
        std::string env;
        OCIO::Platform::Getenv("MY_WINDOWS_DUMMY_ENV", env);
        OCIO_CHECK_EQUAL(env, std::string(" "));
    }
    {
        SetEnvironmentVariable("MY_WINDOWS_DUMMY_ENV", "");
        std::string env;
        OCIO::Platform::Getenv("MY_WINDOWS_DUMMY_ENV", env);
        OCIO_CHECK_ASSERT(env.empty());
    }
#endif
}

OCIO_ADD_TEST(Platform, string_compare)
{
    OCIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp("TtOoPp", "TtOoPp"));
    OCIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp("TtOoPp", "ttOoPp"));
    OCIO_CHECK_NE(0, OCIO::Platform::Strcasecmp("TtOoPp", "tOoPp"));
    OCIO_CHECK_NE(0, OCIO::Platform::Strcasecmp("TtOoPp", "TtOoPp1"));

    OCIO_CHECK_EQUAL(0, OCIO::Platform::Strncasecmp("TtOoPp", "TtOoPp", 2));
    OCIO_CHECK_EQUAL(0, OCIO::Platform::Strncasecmp("TtOoPp", "ttOoPp", 2));
    OCIO_CHECK_EQUAL(0, OCIO::Platform::Strncasecmp("TtOoPp", "ttOOOO", 2));
    OCIO_CHECK_NE(0, OCIO::Platform::Strcasecmp("TtOoPp", "tOoPp"));
    OCIO_CHECK_NE(0, OCIO::Platform::Strcasecmp("TtOoPp", "TOoPp"));
}

OCIO_ADD_TEST(Platform, aligned_memory_test)
{
    size_t alignement = 16u;
    void* memBlock = OCIO::Platform::AlignedMalloc(1001, alignement);

    OCIO_CHECK_ASSERT(memBlock);
    OCIO_CHECK_EQUAL(((uintptr_t)memBlock) % alignement, 0);

    OCIO::Platform::AlignedFree(memBlock);
}

OCIO_ADD_TEST(Platform, create_temp_filename)
{
    constexpr size_t TestMax = 20;

    std::set<std::string> uids;
    for (size_t idx=0; idx<TestMax; ++idx)
    {
        uids.insert(OCIO::Platform::CreateTempFilename(""));
    }

    // Check that it only generates unique random strings.
    OCIO_CHECK_EQUAL(uids.size(), TestMax);
}
