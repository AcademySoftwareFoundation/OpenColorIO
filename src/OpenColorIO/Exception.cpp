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

OCIO_NAMESPACE_ENTER
{
  
Exception::Exception(const char * msg)
    :   std::runtime_error(msg)
{
}

Exception::Exception(const Exception & e)
    :   std::runtime_error(e)
{
}

Exception::~Exception()
{
}


ExceptionMissingFile::ExceptionMissingFile(const char * msg)
    :   Exception(msg)
{
}

ExceptionMissingFile::ExceptionMissingFile(const ExceptionMissingFile & e)
    :   Exception(e)
{
}

ExceptionMissingFile::~ExceptionMissingFile()
{
}

}
OCIO_NAMESPACE_EXIT


///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

#include <string.h>


OIIO_ADD_TEST(Exception, Basic)
{
    static const char* dummyErrorStr = "Dummy error";

    // Test 0 - Trivial one...

    try
    {
        throw OCIO::Exception(dummyErrorStr);
    }
    catch(const OCIO::Exception& ex)
    {
        OIIO_CHECK_EQUAL(strcmp(ex.what(), dummyErrorStr), 0);
    }
    catch(...)
    {
        OIIO_CHECK_ASSERT(!"Wrong exception type");
    }

    // Test 1

    try
    {
        throw OCIO::Exception(dummyErrorStr);
    }
    catch(const std::exception& ex)
    {
        OIIO_CHECK_EQUAL(strcmp(ex.what(), dummyErrorStr), 0);
    }
    catch(...)
    {
        OIIO_CHECK_ASSERT(!"Wrong exception type");
    }

    // Test 2

    try
    {
        OCIO::Exception ex(dummyErrorStr);
        throw OCIO::Exception(ex);
    }
    catch(const std::exception& ex)
    {
        OIIO_CHECK_EQUAL(strcmp(ex.what(), dummyErrorStr), 0);
    }
    catch(...)
    {
        OIIO_CHECK_ASSERT(!"Wrong exception type");
    }
}


OIIO_ADD_TEST(Exception, MissingFile)
{
    static const char* dummyErrorStr = "Dummy error";

    try
    {
        throw OCIO::ExceptionMissingFile(dummyErrorStr);
    }
    catch(const std::exception& ex)
    {
        OIIO_CHECK_EQUAL(strcmp(ex.what(), dummyErrorStr), 0);
    }
    catch(...)
    {
        OIIO_CHECK_ASSERT(!"Wrong exception type");
    }
}

#endif // OCIO_UNIT_TEST