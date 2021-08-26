// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#if defined(_WIN32) && !defined(NDEBUG)
#include <crtdbg.h>
#include <ctype.h>
#endif

#include <cstring>

#include "apputils/argparse.h"
#include "UnitTest.h"
#include "utils/StringUtils.h"


UnitTests & GetUnitTests()
{
    static UnitTests oiio_unit_tests;
    return oiio_unit_tests; 
}

int unit_test_failures{ 0 };

int UnitTestMain(int argc, const char ** argv)
{

#if !defined(NDEBUG) && defined(_WIN32)
    // Disable the 'assert' dialog box in debug mode.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif

    bool printHelp        = false;
    bool stopOnFirstError = false;

    // Note that empty strings mean to run all the unit tests.
    std::string filter, utestGroupAllowed, utestNameAllowed;

    ArgParse ap;
    ap.options("\nCommand line arguments:\n",
               "--help",          &printHelp,        "Print help message",
               "--stop_on_error", &stopOnFirstError, "Stop on the first error",
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

        try
        {
            GetUnitTests()[index]->function();
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
                  << (passing ? "PASSED" : "FAILED")
                  << std::endl;

        if (stopOnFirstError && !passing)
        {
            break;
        }
    }

    std::cerr << "\n\n" << unit_test_failed << " tests failed with " 
              << unit_test_failures << " errors.\n\n";

    GetUnitTests().clear();


    return unit_test_failures;
}
