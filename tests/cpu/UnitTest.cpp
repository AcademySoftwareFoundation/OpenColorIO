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
