// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "testutils/UnitTest.h"
#include "UnitTestOptimFlags.h"


#if !defined(NDEBUG) && defined(_WIN32)

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

    // Use an invalid ASCII character (in any locals).
    OCIO_CHECK_ASSERT(!::isspace(INT_MAX));
}

#endif


int main(int argc, const char ** argv)
{
    std::cerr << "\n OpenColorIO_Core_Unit_Tests \n\n";

    // Make sure OptimizationFlags env variable is turned off during tests and
    // restored at the end.
    OCIOOptimizationFlagsEnvGuard flagsGuard("");

    return UnitTestMain(argc, argv);
}
