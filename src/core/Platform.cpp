//
// made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
//
//

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
#ifdef WINDOWS
            // To remove the security compilation warning, the _dupenv_s method
            // must be used (instead of the getenv). The improvement is that
            // the buffer length is now under control to mitigate buffer overflow attacks.
            //
            char * val;
            size_t len = 0;
            // At least _dupenv_s validates the memory size by returning ENOMEM
            //  in case of allocation size issue.
            const errno_t err = ::_dupenv_s(&val, &len, name);
            if(err!=0 || len==0 || !val || !*val)
            {
                if(val) free(val);
                value.resize(0);
            }
            else
            {
                // NB: len is the sizeof() of a string ( i.e. not its strlen() )
                value = val;
                value.resize(len-1);
                if(val) free(val);
            }
#else
            const char* val = ::getenv(name);
            value = (val && *val) ? val : "";
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
}

OIIO_ADD_TEST(Platform, putenv)
{
    {
        const std::string value("MY_DUMMY_ENV=SomeValue");
        ::putenv(const_cast<char*>(value.c_str()));
        std::string env;
        OCIO::Platform::getenv("MY_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(!env.empty());

        OIIO_CHECK_ASSERT(0==strcmp("SomeValue", env.c_str()));
        OIIO_CHECK_EQUAL(strlen("SomeValue"), env.size());
    }
    {
        const std::string value("MY_DUMMY_ENV= ");
        ::putenv(const_cast<char*>(value.c_str()));
        std::string env;
        OCIO::Platform::getenv("MY_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(!env.empty());

        OIIO_CHECK_ASSERT(0==strcmp(" ", env.c_str()));
        OIIO_CHECK_EQUAL(strlen(" "), env.size());
    }
    {
        const std::string value("MY_DUMMY_ENV=");
        ::putenv(const_cast<char*>(value.c_str()));
        std::string env;
        OCIO::Platform::getenv("MY_DUMMY_ENV", env);
        OIIO_CHECK_ASSERT(env.empty());
    }
}

#endif // OCIO_UNIT_TEST
