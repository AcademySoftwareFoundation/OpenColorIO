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
        value.resize(len+1);
        ::snprintf(&value[0], len, "%s", val);
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
#include "unittest.h"

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
    }
}

#endif // OCIO_UNIT_TEST
