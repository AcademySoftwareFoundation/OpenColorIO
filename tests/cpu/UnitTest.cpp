// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifdef OCIO_UNIT_TEST

#ifndef _WIN32
#pragma GCC visibility push(default)
#endif

#if defined(_WIN32) && !defined(NDEBUG)
#include <crtdbg.h>
#include <ctype.h>
#endif
#include <memory>


#include "UnitTest.h" // OCIO unit tests header


UnitTests & GetUnitTests()
{
    static UnitTests oiio_unit_tests;
    return oiio_unit_tests; 
}


int unit_test_failures = 0;


OCIO_ADD_TEST(UnitTest, windows_debug)
{
    // The debug VC++ CRT libraries do pop an 'assert' dialog box 
    // on some errors hanging unit test executions. To avoid the hang 
    // (for Debug mode only) the code disables the dialog box.
    // The new behavior is now to crash or continue (i.e. later crash)
    // depending of the operation:
    // 1. isspace() works like:
    //      --> returns false when it's not a 'space' character,
    //          but hiding that it's not an ASCII character.
    // 2. iterator error crashes the program (like in Release mode).
    //      --> std::vector<int> v; v[10]; // Crash

#if !defined(NDEBUG) && defined(_WIN32)

    // Use an invalid ASCII character (in any locals).
    OCIO_CHECK_ASSERT(!::isspace(INT_MAX));

#endif
}


int main(int, char **)
{

#if !defined(NDEBUG) && defined(_WIN32)
    // Disable the 'assert' dialog box in debug mode.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif

    std::cerr << "\n OpenColorIO_Core_Unit_Tests \n\n";

    const size_t numTests = GetUnitTests().size();
    for(size_t i = 0; i < numTests; ++i)
    {
        int _tmp = unit_test_failures;
        try
        {
            GetUnitTests()[i]->function();
        }
        catch(std::exception & ex)
        {
            std::cout << "FAILED: " << ex.what() << std::endl;
            ++unit_test_failures;
        }
        catch(...)
        { 
            ++unit_test_failures; 
        }
        
        std::string name(GetUnitTests()[i]->group);
        name += " / " + GetUnitTests()[i]->name;
        std::cerr << "[" << std::right << std::setw(3)
                  << (i+1) << "/" << numTests << "] ["
                  << std::left << std::setw(50)
                  << name << "] - "
                  << (_tmp == unit_test_failures ? "PASSED" : "FAILED")
                  << std::endl;
    }

    std::cerr << "\n" << unit_test_failures << " tests failed\n\n";

    GetUnitTests().clear();

    return unit_test_failures;
}




#ifndef _WIN32
#pragma GCC visibility pop
#endif

#endif // OCIO_UNIT_TEST
