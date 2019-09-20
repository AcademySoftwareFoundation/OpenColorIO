// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
#include "UnitTest.h"

#include <string.h>


OCIO_ADD_TEST(Exception, Basic)
{
    static const char* dummyErrorStr = "Dummy error";

    // Test 0 - Trivial one...

    try
    {
        throw OCIO::Exception(dummyErrorStr);
    }
    catch(const OCIO::Exception& ex)
    {
        OCIO_CHECK_EQUAL(strcmp(ex.what(), dummyErrorStr), 0);
    }
    catch(...)
    {
        OCIO_CHECK_ASSERT(!"Wrong exception type");
    }

    // Test 1

    try
    {
        throw OCIO::Exception(dummyErrorStr);
    }
    catch(const std::exception& ex)
    {
        OCIO_CHECK_EQUAL(strcmp(ex.what(), dummyErrorStr), 0);
    }
    catch(...)
    {
        OCIO_CHECK_ASSERT(!"Wrong exception type");
    }

    // Test 2

    try
    {
        OCIO::Exception ex(dummyErrorStr);
        throw OCIO::Exception(ex);
    }
    catch(const std::exception& ex)
    {
        OCIO_CHECK_EQUAL(strcmp(ex.what(), dummyErrorStr), 0);
    }
    catch(...)
    {
        OCIO_CHECK_ASSERT(!"Wrong exception type");
    }
}


OCIO_ADD_TEST(Exception, MissingFile)
{
    static const char* dummyErrorStr = "Dummy error";

    try
    {
        throw OCIO::ExceptionMissingFile(dummyErrorStr);
    }
    catch(const std::exception& ex)
    {
        OCIO_CHECK_EQUAL(strcmp(ex.what(), dummyErrorStr), 0);
    }
    catch(...)
    {
        OCIO_CHECK_ASSERT(!"Wrong exception type");
    }
}

#endif // OCIO_UNIT_TEST