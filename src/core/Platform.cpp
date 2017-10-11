///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008-2017, Sony Pictures Imageworks Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// Neither the name of the organization Sony Pictures Imageworks nor the
// names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER
// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include <OpenColorIO/OpenColorIO.h>

#include "Platform.h"

OCIO_NAMESPACE_ENTER
{

    namespace Platform
    {
        char* getenv (const char* name)
        {
#ifdef WINDOWS
            static std::string str;

            char * value;
            size_t len = 0;         
            const errno_t err = ::_dupenv_s(&value, &len, name);
            if(err!=0 || len==0 || !value || !*value)
            {
                return 0x0;
            }
            else
            {
                str.resize(len+1);
                ::strncpy_s(&str[0], len, value, len);
            }

            return const_cast<char*>(str.c_str());
#else
            return ::getenv(name);
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
        const char* env = OCIO::Platform::getenv("NotExistingEnvVariable");
        OIIO_CHECK_ASSERT(!env);
    }

    {
        const char* env = OCIO::Platform::getenv("PATH");
        OIIO_CHECK_ASSERT(env && *env);
    }

    {
        char* env = OCIO::Platform::getenv("PATH");
        env = OCIO::Platform::getenv("NotExistingEnvVariable");
        OIIO_CHECK_ASSERT(!env);
    }

    {
        char* env = OCIO::Platform::getenv("NotExistingEnvVariable");
        env = OCIO::Platform::getenv("PATH");
        OIIO_CHECK_ASSERT(env && *env);
    }
}

#endif // OCIO_UNIT_TEST
