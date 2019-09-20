// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include <sstream>

#include "Platform.h"

#ifndef _WIN32
#include <chrono>
#include <random>
#endif


OCIO_NAMESPACE_ENTER
{

namespace Platform
{
void Getenv (const char * name, std::string & value)
{
#ifdef _WIN32
    if(uint32_t size = GetEnvironmentVariable(name, nullptr, 0))
    {
        std::vector<char> buffer(size);
        GetEnvironmentVariable(name, buffer.data(), size);
        value = std::string(buffer.data());
    }
    else
    {
        value.clear();
    }
#else
    const char* val = ::getenv(name);
    value = (val && *val) ? val : "";
#endif 
}

int Strcasecmp(const char * str1, const char * str2)
{
#ifdef _WIN32
    return ::_stricmp(str1, str2);
#else
    return ::strcasecmp(str1, str2);
#endif
}

int Strncasecmp(const char * str1, const char * str2, size_t n)
{
#ifdef _WIN32
    return ::_strnicmp(str1, str2, n);
#else
    return ::strncasecmp(str1, str2, n);
#endif
}

void* AlignedMalloc(size_t size, size_t alignment)
{
#ifdef _WIN32
    void* memBlock = _aligned_malloc(size, alignment);
    return memBlock;
#else
    void* memBlock = 0x0;
    if (!posix_memalign(&memBlock, alignment, size)) return memBlock;
    return 0x0;
#endif
}

void AlignedFree(void* memBlock)
{
#ifdef _WIN32
    _aligned_free(memBlock);
#else
    free(memBlock);
#endif
}

void CreateTempFilename(std::string & filename, const std::string & filenameExt)
{
    // Note: Because of security issue, tmpnam could not be used.

#ifdef _WIN32

    char tmpFilename[L_tmpnam];
    if(tmpnam_s(tmpFilename))
    {
        throw Exception("Could not create a temporary file.");
    }

    filename = tmpFilename;

#else

    std::stringstream ss;
    ss << "/tmp/ocio";

    // Obtain a seed from the system clock.
    const unsigned seed 
        = (unsigned)std::chrono::system_clock::now().time_since_epoch().count();
    // Use the standard mersenne_twister_engine.
    std::mt19937 generator(seed);
    ss << generator();

    filename = ss.str();

#endif

    filename += filenameExt;
}


}//namespace platform

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

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

OCIO_ADD_TEST(Platform, putenv)
{
    {
        const std::string value("MY_DUMMY_ENV=SomeValue");
        ::putenv(const_cast<char*>(value.c_str()));
        std::string env;
        OCIO::Platform::Getenv("MY_DUMMY_ENV", env);
        OCIO_CHECK_ASSERT(!env.empty());

        OCIO_CHECK_ASSERT(0==strcmp("SomeValue", env.c_str()));
        OCIO_CHECK_EQUAL(strlen("SomeValue"), env.size());
    }
    {
        const std::string value("MY_DUMMY_ENV= ");
        ::putenv(const_cast<char*>(value.c_str()));
        std::string env;
        OCIO::Platform::Getenv("MY_DUMMY_ENV", env);
        OCIO_CHECK_ASSERT(!env.empty());

        OCIO_CHECK_ASSERT(0==strcmp(" ", env.c_str()));
        OCIO_CHECK_EQUAL(strlen(" "), env.size());
    }
    {
        const std::string value("MY_DUMMY_ENV=");
        ::putenv(const_cast<char*>(value.c_str()));
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


OCIO_ADD_TEST(Platform, CreateTempFilename)
{
    std::string f1, f2;

    OCIO_CHECK_NO_THROW(OCIO::Platform::CreateTempFilename(f1, ""));
    OCIO_CHECK_NO_THROW(OCIO::Platform::CreateTempFilename(f2, ""));
    OCIO_CHECK_ASSERT(f1!=f2);

    OCIO_CHECK_NO_THROW(OCIO::Platform::CreateTempFilename(f1, ".ctf"));
    OCIO_CHECK_NO_THROW(OCIO::Platform::CreateTempFilename(f2, ".ctf"));
    OCIO_CHECK_ASSERT(f1!=f2);
}

#endif // OCIO_UNIT_TEST
