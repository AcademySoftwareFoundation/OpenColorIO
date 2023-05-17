// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>
#include <set>

#include "Platform.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Platform, envVariable)
{
    // Only validates the public API.
    // Complete validations are done below using the private methods.

    const char * path = OCIO::GetEnvVariable(U8("PATH"));
    OCIO_CHECK_ASSERT(path && *path);

    OCIO::SetEnvVariable(U8("MY_DUMMY_ENV"), U8("SomeValue"));
    const char * value = OCIO::GetEnvVariable(U8("MY_DUMMY_ENV"));
    OCIO_CHECK_ASSERT(value && *value);
    OCIO_CHECK_EQUAL(std::string(value), U8("SomeValue"));

#ifdef _WIN32
    // Assert that we retrieve the correct value from the Windows API too.
    uint32_t win_env_sz = GetEnvironmentVariable(TEXT("MY_DUMMY_ENV"), NULL, 0);
    OCIO_CHECK_NE(win_env_sz, 0);

    std::tstring win_env_value(win_env_sz, 0);
    GetEnvironmentVariable(TEXT("MY_DUMMY_ENV"), &win_env_value[0], win_env_sz);
    win_env_value.pop_back(); // Remove null terminator that interferes with comparison
    OCIO_CHECK_ASSERT(win_env_value == TEXT("SomeValue"));
#endif

    OCIO::UnsetEnvVariable(U8("MY_DUMMY_ENV"));
    value = OCIO::GetEnvVariable(U8("MY_DUMMY_ENV"));
    OCIO_CHECK_ASSERT(!value || !*value);

#ifdef _WIN32
    // Assert that the variable has been unset from the Windows API too, which will result in
    // GetEnvironmentVariable returning 0 and GetLastError returning ERROR_ENVVAR_NOT_FOUND.
    OCIO_CHECK_EQUAL(GetEnvironmentVariable(TEXT("MY_DUMMY_ENV"), NULL, 0), 0);
    OCIO_CHECK_EQUAL(GetLastError(), ERROR_ENVVAR_NOT_FOUND);
#endif
}

OCIO_ADD_TEST(Platform, getenv)
{
    std::string env;
    OCIO_CHECK_ASSERT(!OCIO::Platform::Getenv("NotExistingEnvVariable", env));
    OCIO_CHECK_ASSERT(env.empty());

    OCIO_CHECK_ASSERT(OCIO::Platform::Getenv("PATH", env));
    OCIO_CHECK_ASSERT(!env.empty());

    // Test a not existing env. variable.

    OCIO_CHECK_ASSERT(!OCIO::Platform::isEnvPresent("NotExistingEnvVariable"));

    OCIO_CHECK_ASSERT(!OCIO::Platform::Getenv("NotExistingEnvVariable", env));
    OCIO_CHECK_ASSERT(env.empty());

    // Test an existing env. variable.

    OCIO_CHECK_ASSERT(OCIO::Platform::isEnvPresent("PATH"));

    OCIO_CHECK_ASSERT(OCIO::Platform::Getenv("PATH", env));
    OCIO_CHECK_ASSERT(!env.empty());

#ifdef _WIN32
    // Assert that all results match in Windows API.

    // Assert this variable doesn't exist, which will result in GetEnvironmentVariable returning
    // 0 and GetLastError returning ERROR_ENVVAR_NOT_FOUND.
    OCIO_CHECK_EQUAL(GetEnvironmentVariable(TEXT("NotExistingEnvVariable"), NULL, 0), 0);
    OCIO_CHECK_EQUAL(GetLastError(), ERROR_ENVVAR_NOT_FOUND);

    // Assert this variable does exist.
    OCIO_CHECK_NE(GetEnvironmentVariable(TEXT("PATH"), NULL, 0), 0);

    // Create a variable and test that it's retrievable through the Windows API.
    OCIO::Platform::Setenv(U8("MY_WINDOWS_DUMMY_ENV"), U8("SomeValue"));
    uint32_t win_env_sz = GetEnvironmentVariable(TEXT("MY_WINDOWS_DUMMY_ENV"), NULL, 0);
    OCIO_CHECK_NE(win_env_sz, 0);

    std::tstring win_env_value(win_env_sz, 0);
    GetEnvironmentVariable(TEXT("MY_WINDOWS_DUMMY_ENV"), &win_env_value[0], win_env_sz);
    win_env_value.pop_back(); // Remove null terminator that interferes with comparison
    OCIO_CHECK_ASSERT(win_env_value == TEXT("SomeValue"));

    OCIO::Platform::Unsetenv(U8("MY_WINDOWS_DUMMY_ENV"));
    OCIO_CHECK_EQUAL(GetEnvironmentVariable(TEXT("MY_WINDOWS_DUMMY_ENV"), NULL, 0), 0);
    OCIO_CHECK_EQUAL(GetLastError(), ERROR_ENVVAR_NOT_FOUND);
#endif
}

OCIO_ADD_TEST(Platform, setenv)
{
    // Guard to automatically unset the env. variable.
    struct Guard
    {
        Guard() = default;
        ~Guard()
        {
            OCIO::Platform::Unsetenv(U8("MY_DUMMY_ENV"));
            OCIO::Platform::Unsetenv(U8("MY_WINDOWS_DUMMY_ENV"));
        }
    } guard;

    {
        OCIO::Platform::Setenv(U8("MY_DUMMY_ENV"), U8("SomeValue"));
        std::string env;
        OCIO_CHECK_ASSERT(OCIO::Platform::Getenv(U8("MY_DUMMY_ENV"), env));
        OCIO_CHECK_ASSERT(!env.empty());

        OCIO_CHECK_ASSERT(0==std::strcmp(U8("SomeValue"), env.c_str()));
        OCIO_CHECK_EQUAL(strlen(U8("SomeValue")), env.size());
    }
    {
        OCIO::Platform::Setenv(U8("MY_DUMMY_ENV"), U8(" "));
        std::string env;
        OCIO_CHECK_ASSERT(OCIO::Platform::Getenv(U8("MY_DUMMY_ENV"), env));
        OCIO_CHECK_ASSERT(!env.empty());

        OCIO_CHECK_ASSERT(0==std::strcmp(U8(" "), env.c_str()));
        OCIO_CHECK_EQUAL(std::strlen(U8(" ")), env.size());
    }
    {
        OCIO::Platform::Unsetenv(U8("MY_DUMMY_ENV"));
        std::string env;
        OCIO_CHECK_ASSERT(!OCIO::Platform::Getenv(U8("MY_DUMMY_ENV"), env));
        OCIO_CHECK_ASSERT(env.empty());
    }
#ifdef _WIN32
    {
        SetEnvironmentVariable(TEXT("MY_WINDOWS_DUMMY_ENV"), TEXT("1"));
        std::string env;
        OCIO_CHECK_ASSERT(OCIO::Platform::Getenv(U8("MY_WINDOWS_DUMMY_ENV"), env));
        OCIO_CHECK_EQUAL(env, std::string(U8("1")));
    }
    {
        SetEnvironmentVariable(TEXT("MY_WINDOWS_DUMMY_ENV"), TEXT(" "));
        std::string env;
        OCIO_CHECK_ASSERT(OCIO::Platform::Getenv(U8("MY_WINDOWS_DUMMY_ENV"), env));
        OCIO_CHECK_EQUAL(env, std::string(U8(" ")));
    }
    {
        // Windows SetEnvironmentVariable() sets the env. variable to empty like
        // the Linux ::setenv() in contradiction with the Windows _putenv_s().

        SetEnvironmentVariable(TEXT("MY_WINDOWS_DUMMY_ENV"), TEXT(""));
        std::string env;
        OCIO_CHECK_ASSERT(OCIO::Platform::Getenv(U8("MY_WINDOWS_DUMMY_ENV"), env));
        OCIO_CHECK_ASSERT(env.empty());
    }
    {
        SetEnvironmentVariable(TEXT("MY_WINDOWS_DUMMY_ENV"), nullptr);
        std::string env;
        OCIO_CHECK_ASSERT(!OCIO::Platform::Getenv(U8("MY_WINDOWS_DUMMY_ENV"), env));
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

OCIO_ADD_TEST(Platform, utf8_utf16_convert)
{
#ifdef _WIN32
    // Define the same string in both UTF-8 and UTF-16LE encoding:
    // - Hiragana letter KO:        xe3, x81, x93       x3053
    // - Hiragana letter N:         xe3, x82, x93       x3093
    // - Hiragana letter NI:        xe3, x81, xab       x306b
    // - Hiragana letter CHI:       xe3, x81, xa1       x3061
    // - Hiragana letter HA/WA:     xe3, x81, xaf       x306f
    std::string utf8_str = "\xe3\x81\x93\xe3\x82\x93\xe3\x81\xab\xe3\x81\xa1\xe3\x81\xaf";
    std::wstring utf16_str = L"\x3053\x3093\x306b\x3061\x306f";

    // Convert each string to the other encoding and assert that the result matches the other
    std::string utf16_to_utf8 = OCIO::Platform::Utf16ToUtf8(utf16_str);
    std::wstring utf8_to_utf16 = OCIO::Platform::Utf8ToUtf16(utf8_str);

    OCIO_CHECK_EQUAL(utf16_to_utf8, utf8_str);

    // wstring can't be sent to cout, so we run an assert
    OCIO_CHECK_ASSERT(wcscmp(utf8_to_utf16.c_str(), utf16_str.c_str()) == 0);
#endif
}
