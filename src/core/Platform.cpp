//
// made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
//
//

#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Platform.h"


OCIO_NAMESPACE_ENTER
{

    namespace Platform
    {
        // Unlike the ::getenv(), the method does not use any static buffer 
        // for the Windows platform only. *nix platforms are still using
        // the ::getenv method, but reducing the static vairable usage.
        // 
        void getenv (const char* name, std::string& value)
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

        void setenv (const char* name, const std::string& value)
        {
#ifdef _WIN32
            ::_putenv_s(name, value.c_str());
#else
            ::setenv(name, value.c_str(), 1);
#endif
        }

    }//namespace platform

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(Platform, getenv)
{
    {
        std::string env;
        OCIO::Platform::getenv("NotExistingEnvVariable", env);
        OIIO_CHECK_ASSERT(env.empty());
    }

    {
        std::string env;
        OCIO::Platform::getenv("PATH", env);
        OIIO_CHECK_ASSERT(!env.empty());
    }

    {
        std::string env;
        OCIO::Platform::getenv("PATH", env);
        OCIO::Platform::getenv("NotExistingEnvVariable", env);
        OIIO_CHECK_ASSERT(env.empty());
    }

    {
        std::string env;
        OCIO::Platform::getenv("NotExistingEnvVariable", env);
        OCIO::Platform::getenv("PATH", env);
        OIIO_CHECK_ASSERT(!env.empty());
    }
#ifdef _WIN32
    {
        SetEnvironmentVariable("MY_WINDOWS_DUMMY_ENV", "1");
        std::string env;
        OCIO::Platform::getenv("MY_WINDOWS_DUMMY_ENV", env);
        OIIO_CHECK_EQUAL(env, std::string("1"));
    }
    {
        SetEnvironmentVariable("MY_WINDOWS_DUMMY_ENV", " ");
        std::string env;
        OCIO::Platform::getenv("MY_WINDOWS_DUMMY_ENV", env);
        OIIO_CHECK_EQUAL(env, std::string(" "));
    }
    {
        SetEnvironmentVariable("MY_WINDOWS_DUMMY_ENV", "");
        std::string env;
        OCIO::Platform::getenv("MY_WINDOWS_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(env.empty());
    }
#endif  
}

OIIO_ADD_TEST(Platform, setenv)
{
    {
        OCIO::Platform::setenv("MY_DUMMY_ENV", "SomeValue");
        std::string env;
        OCIO::Platform::getenv("MY_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(!env.empty());

        OIIO_CHECK_ASSERT(0==strcmp("SomeValue", env.c_str()));
        OIIO_CHECK_EQUAL(strlen("SomeValue"), env.size());
    }
    {
        OCIO::Platform::setenv("MY_DUMMY_ENV", " ");
        std::string env;
        OCIO::Platform::getenv("MY_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(!env.empty());

        OIIO_CHECK_ASSERT(0==strcmp(" ", env.c_str()));
        OIIO_CHECK_EQUAL(strlen(" "), env.size());
    }
    {
        OCIO::Platform::setenv("MY_DUMMY_ENV", "");
        std::string env;
        OCIO::Platform::getenv("MY_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(env.empty());
    }
}

#endif // OCIO_UNIT_TEST
