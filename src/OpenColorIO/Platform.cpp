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


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Platform.h"


OCIO_NAMESPACE_ENTER
{

namespace Platform
{
void Getenv (const char * name, std::string & value)
{
#ifdef WINDOWS
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
#ifdef WINDOWS
    return ::_stricmp(str1, str2);
#else
    return ::strcasecmp(str1, str2);
#endif
}

int Strncasecmp(const char * str1, const char * str2, size_t n)
{
#ifdef WINDOWS
    return ::_strnicmp(str1, str2, n);
#else
    return ::strncasecmp(str1, str2, n);
#endif
}

void* AlignedMalloc(size_t size, size_t alignment)
{
#ifdef WINDOWS
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
#ifdef WINDOWS
    _aligned_free(memBlock);
#else
    free(memBlock);
#endif
}

}//namespace platform

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(Platform, getenv)
{
    std::string env;
    OCIO::Platform::Getenv("NotExistingEnvVariable", env);
    OIIO_CHECK_ASSERT(env.empty());

    OCIO::Platform::Getenv("PATH", env);
    OIIO_CHECK_ASSERT(!env.empty());

    OCIO::Platform::Getenv("NotExistingEnvVariable", env);
    OIIO_CHECK_ASSERT(env.empty());

    OCIO::Platform::Getenv("PATH", env);
    OIIO_CHECK_ASSERT(!env.empty());
}

OIIO_ADD_TEST(Platform, putenv)
{
    {
        const std::string value("MY_DUMMY_ENV=SomeValue");
        ::putenv(const_cast<char*>(value.c_str()));
        std::string env;
        OCIO::Platform::Getenv("MY_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(!env.empty());

        OIIO_CHECK_ASSERT(0==strcmp("SomeValue", env.c_str()));
        OIIO_CHECK_EQUAL(strlen("SomeValue"), env.size());
    }
    {
        const std::string value("MY_DUMMY_ENV= ");
        ::putenv(const_cast<char*>(value.c_str()));
        std::string env;
        OCIO::Platform::Getenv("MY_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(!env.empty());

        OIIO_CHECK_ASSERT(0==strcmp(" ", env.c_str()));
        OIIO_CHECK_EQUAL(strlen(" "), env.size());
    }
    {
        const std::string value("MY_DUMMY_ENV=");
        ::putenv(const_cast<char*>(value.c_str()));
        std::string env;
        OCIO::Platform::Getenv("MY_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(env.empty());
    }
#ifdef WINDOWS
    {
        SetEnvironmentVariable("MY_WINDOWS_DUMMY_ENV", "1");
        std::string env;
        OCIO::Platform::Getenv("MY_WINDOWS_DUMMY_ENV", env);
        OIIO_CHECK_EQUAL(env, std::string("1"));
    }
    {
        SetEnvironmentVariable("MY_WINDOWS_DUMMY_ENV", " ");
        std::string env;
        OCIO::Platform::Getenv("MY_WINDOWS_DUMMY_ENV", env);
        OIIO_CHECK_EQUAL(env, std::string(" "));
    }
    {
        SetEnvironmentVariable("MY_WINDOWS_DUMMY_ENV", "");
        std::string env;
        OCIO::Platform::Getenv("MY_WINDOWS_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(env.empty());
    }
#endif
}

OIIO_ADD_TEST(Platform, string_compare)
{
    OIIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp("TtOoPp", "TtOoPp"));
    OIIO_CHECK_EQUAL(0, OCIO::Platform::Strcasecmp("TtOoPp", "ttOoPp"));
    OIIO_CHECK_NE(0, OCIO::Platform::Strcasecmp("TtOoPp", "tOoPp"));
    OIIO_CHECK_NE(0, OCIO::Platform::Strcasecmp("TtOoPp", "TtOoPp1"));

    OIIO_CHECK_EQUAL(0, OCIO::Platform::Strncasecmp("TtOoPp", "TtOoPp", 2));
    OIIO_CHECK_EQUAL(0, OCIO::Platform::Strncasecmp("TtOoPp", "ttOoPp", 2));
    OIIO_CHECK_EQUAL(0, OCIO::Platform::Strncasecmp("TtOoPp", "ttOOOO", 2));
    OIIO_CHECK_NE(0, OCIO::Platform::Strcasecmp("TtOoPp", "tOoPp"));
    OIIO_CHECK_NE(0, OCIO::Platform::Strcasecmp("TtOoPp", "TOoPp"));
}

OIIO_ADD_TEST(Platform, aligned_memory_test)
{
    size_t alignement = 16u;
    void* memBlock = OCIO::Platform::AlignedMalloc(1001, alignement);

    OIIO_CHECK_ASSERT(memBlock);
    OIIO_CHECK_EQUAL(((uintptr_t)memBlock) % alignement, 0);

    OCIO::Platform::AlignedFree(memBlock);
}
#endif // OCIO_UNIT_TEST
