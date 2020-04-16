// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <string.h>

#include "Exception.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(Exception, basic)
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


OCIO_ADD_TEST(Exception, missing_file)
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

