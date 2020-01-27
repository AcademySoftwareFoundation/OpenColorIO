// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "StringUtils.h"

#include "UnitTest.h"

OCIO_ADD_TEST(StringUtils, cases)
{
    const std::string ref{"lOwEr 1*& ctfG"};

    {
        const std::string str = StringUtils::Lower(ref);
        OCIO_CHECK_EQUAL(str, "lower 1*& ctfg");
    }

    {
        const std::string str = StringUtils::Upper(ref);
        OCIO_CHECK_EQUAL(str, "LOWER 1*& CTFG");
    }
}

OCIO_ADD_TEST(StringUtils, trim)
{
    const std::string ref{" \t\n lOwEr 1*& ctfG \n\n "};

    {
        const std::string str = StringUtils::LeftTrim(ref);
        OCIO_CHECK_EQUAL(str, "lOwEr 1*& ctfG \n\n ");
    }

    {
        const std::string str = StringUtils::RightTrim(ref);
        OCIO_CHECK_EQUAL(str, " \t\n lOwEr 1*& ctfG");
    }

    {
        const std::string str = StringUtils::Trim(ref);
        OCIO_CHECK_EQUAL(str, "lOwEr 1*& ctfG");
    }
}

OCIO_ADD_TEST(StringUtils, split)
{
    const std::string ref{" \t\n lOwEr 1*& ctfG \n\n "};

    {
        std::vector<std::string> results;
        StringUtils::Split(ref, results, 'O');
        OCIO_CHECK_EQUAL(results.size(), 2);
        OCIO_CHECK_EQUAL(results[0], " \t\n l");
        OCIO_CHECK_EQUAL(results[1], "wEr 1*& ctfG \n\n ");
    }

    {
        std::vector<std::string> results;
        StringUtils::SplitByLines(ref, results);
        OCIO_CHECK_EQUAL(results.size(), 4);
        OCIO_CHECK_EQUAL(results[0], " \t");
        OCIO_CHECK_EQUAL(results[1], " lOwEr 1*& ctfG ");
        OCIO_CHECK_EQUAL(results[2], "");
        OCIO_CHECK_EQUAL(results[3], " ");
    }
}

OCIO_ADD_TEST(StringUtils, searches)
{
    const std::string ref{"lOwEr 1*& ctfG"};

    {
        OCIO_CHECK_ASSERT(StringUtils::StartsWith(ref, "lOwEr"));

        OCIO_CHECK_ASSERT(!StringUtils::StartsWith(ref, "wEr"));
        OCIO_CHECK_ASSERT(!StringUtils::StartsWith(ref, "LOwEr"));
    }

    {
        OCIO_CHECK_ASSERT(StringUtils::EndsWith(ref, "ctfG"));

        OCIO_CHECK_ASSERT(!StringUtils::EndsWith(ref, "ctf"));
        OCIO_CHECK_ASSERT(!StringUtils::EndsWith(ref, "CtfG"));
    }
}
