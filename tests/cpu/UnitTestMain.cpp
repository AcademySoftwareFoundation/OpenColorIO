// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#if defined(_WIN32) && !defined(NDEBUG)
#include <crtdbg.h>
#include <ctype.h>
#endif

#include <cstring>

#include "testutils/UnitTest.h"
#include "apputils/argparse.h"
#include "utils/StringUtils.h"

#include "UnitTestOptimFlags.h"
#include "CPUInfo.h"

namespace OCIO = OCIO_NAMESPACE;

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

#if OCIO_ARCH_X86 || OCIO_USE_SSE2NEON
    #define ENABLE_SIMD_USAGE
#endif

int main(int argc, const char ** argv)
{
    std::cerr << "\n OpenColorIO_Core_Unit_Tests \n\n";

    // Make sure OptimizationFlags env variable is turned off during tests and
    // restored at the end.
    OCIOOptimizationFlagsEnvGuard flagsGuard("");


#if !defined(NDEBUG) && defined(_WIN32)
    // Disable the 'assert' dialog box in debug mode.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif

    bool printHelp        = false;
    bool stopOnFirstError = false;

    // Note that empty strings mean to run all the unit tests.
    std::string filter, utestGroupAllowed, utestNameAllowed;
#if defined(ENABLE_SIMD_USAGE)
    bool no_accel = false;
    bool sse2     = false;
    bool avx      = false;
    bool avx2     = false;
    bool f16c     = false;
#endif
    ArgParse ap;
    ap.options("\nCommand line arguments:\n",
               "--help",          &printHelp,        "Print help message",
               "--stop_on_error", &stopOnFirstError, "Stop on the first error",
#if defined(ENABLE_SIMD_USAGE)
               "--no_accel",      &no_accel,         "Disable ALL Accelerated features",
               "--sse2",          &sse2,             "Enable SSE2 Accelerated features",
               "--avx",           &avx,              "Enable AVX Accelerated features",
               "--avx2",          &avx2,             "Enable AVX2 Accelerated features",
               "--f16c",          &f16c,             "Enable F16C Accelerated features",
#endif
               "--run_only %s",   &filter,           "Run only some unit tests\n"
                                                     "\tex: --run_only \"FileRules/clone\"\n"
                                                     "\tex: --run_only FileRules i.e. \"FileRules/*\"\n"
                                                     "\tex: --run_only /clone    i.e. \"*/clone\"\n",
               nullptr);

    if (ap.parse(argc, argv) < 0)
    {
        std::cerr << ap.geterror() << std::endl;
        ap.usage();
        return 1;
    }

    if (printHelp)
    {
        ap.usage();
        return 1;
    }

#if defined(ENABLE_SIMD_USAGE)
    OCIO::CPUInfo &cpu = OCIO::CPUInfo::instance();
    if (no_accel || sse2 || avx || avx2 || f16c)
    {
        unsigned flags = 0;
        if (sse2)
        {
            if (!cpu.hasSSE2())
            {
                std::cerr << "-sse2 disabled or not supported by processor\n";
                GetUnitTests().clear();
            }
            flags |= X86_CPU_FLAG_SSE2;
        }
        if (avx)
        {
            if (!cpu.hasAVX())
            {
                std::cerr << "-avx disabled or not supported by processor\n";
                GetUnitTests().clear();
            }
            flags |= X86_CPU_FLAG_AVX;
        }

        if (avx2)
        {
            if (!cpu.hasAVX2())
            {
                std::cerr << "-avx2 not supported by processor\n";
                GetUnitTests().clear();
            }
            flags |= X86_CPU_FLAG_AVX2;
        }
        if (f16c)
        {
            if (!cpu.hasF16C())
            {
                std::cerr << "-f16c disabled or not supported by processor\n";
                GetUnitTests().clear();
            }
            flags |= X86_CPU_FLAG_F16C;
        }
        cpu.flags = flags;
    }

    std::cerr << cpu.name << " ";
    if (cpu.hasSSE2())
        std::cerr << "+sse2";

    if (cpu.hasAVX())
        std::cerr << "+avx";

    if (cpu.hasAVX2())
        std::cerr << "+avx2";

    if (cpu.hasF16C())
        std::cerr << "+f16c";

    std::cerr << "\n\n";
#endif

    if (!filter.empty())
    {
        const std::vector<std::string> results = StringUtils::Split(filter, '/');
        if (results.size() >= 1)
        {
            utestGroupAllowed = StringUtils::Lower(StringUtils::Trim(results[0]));
            if (results.size() >= 2)
            {
                utestNameAllowed = StringUtils::Lower(StringUtils::Trim(results[1]));

                if (results.size() >= 3)
                {
                    std::cerr << "Invalid value for the argument '--run_only'." << std::endl;
                    ap.usage();
                    return 1;
                }
            }
        }
    }


    int unit_test_failed = 0;
    int unit_test_skipped = 0;

    const size_t numTests = GetUnitTests().size();
    for(size_t index = 0; index < numTests; ++index)
    {
        const std::string utestGroup = GetUnitTests()[index]->group;
        const std::string utestName  = GetUnitTests()[index]->name;

        bool utestAllowed = true;

        if (!utestGroupAllowed.empty() && StringUtils::Lower(utestGroup)!=utestGroupAllowed)
        {
            utestAllowed = false;
        }

        if (!utestNameAllowed.empty() && StringUtils::Lower(utestName)!=utestNameAllowed)
        {
            utestAllowed = false;
        }

        if (!utestAllowed)
        {
            continue;
        }

        const int _tmp = unit_test_failures;
        bool skipped = false;

        try
        {
            GetUnitTests()[index]->function();
        }
        catch(SkipException &)
        {
            skipped = true;
            ++unit_test_skipped;
        }
        catch(std::exception & ex)
        {
            std::cerr << "\nFAILED: " << ex.what() << "." << std::endl;
            ++unit_test_failures;
        }
        catch(...)
        {
            std::cerr << "\nFAILED: Unexpected error." << std::endl;
            ++unit_test_failures;
        }

        const bool passing = (_tmp == unit_test_failures);
        if (!passing)
        {
            ++unit_test_failed;
        }

        std::string name(utestGroup);
        name += " / " + utestName;

        constexpr size_t maxCharToDisplay = 59;
        if (name.size() > maxCharToDisplay)
        {
            name.resize(maxCharToDisplay);
        }

        std::cerr << "[" << std::right << std::setw(4)
                  << (index+1) << "/" << numTests << "] ["
                  << std::left << std::setw(maxCharToDisplay+1)
                  << name << "] - "
                  << (passing ? skipped ? "SKIPPED" : "PASSED" : "FAILED")
                  << std::endl;

        if (stopOnFirstError && !passing)
        {
            break;
        }
    }

    std::cerr << "\n\n" << unit_test_failed << " tests failed with "
              << unit_test_failures << " errors "
              << unit_test_skipped <<  " skips.\n\n";

    GetUnitTests().clear();


    return unit_test_failures;
}
