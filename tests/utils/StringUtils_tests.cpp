// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "testutils/UnitTest.h"
#include "utils/StringUtils.h"


OCIO_ADD_TEST(StringUtils, cases)
{
    constexpr char ref[]{"lOwEr 1*& ctfG"};

    {
        const std::string str = StringUtils::Lower(ref);
        OCIO_CHECK_EQUAL(str, "lower 1*& ctfg");
    }

    {
        const std::string str = StringUtils::Upper(ref);
        OCIO_CHECK_EQUAL(str, "LOWER 1*& CTFG");
    }

    {
        const std::string str = StringUtils::Lower(nullptr);
        OCIO_CHECK_EQUAL(str, "");
    }

    {
        const std::string str = StringUtils::Upper(nullptr);
        OCIO_CHECK_EQUAL(str, "");
    }

}

OCIO_ADD_TEST(StringUtils, trim)
{
    constexpr char ref[]{" \t\n lOwEr 1*& ctfG \n\n "};

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

    {
        // Test that no assert happens when the Trim argument is not an unsigned char (see issue #1874).
        constexpr char ref2[]{ char(-1), char(-2), char(-3), '\0' };
        const std::string str = StringUtils::Trim(ref2);
    }
}

OCIO_ADD_TEST(StringUtils, split)
{
    constexpr char ref[]{" \t\n lOwEr 1*& ctfG \n\n "};

    {
        const StringUtils::StringVec results = StringUtils::Split(ref, 'O');
        OCIO_REQUIRE_EQUAL(results.size(), 2);
        OCIO_CHECK_EQUAL(results[0], " \t\n l");
        OCIO_CHECK_EQUAL(results[1], "wEr 1*& ctfG \n\n ");
    }

    // Test to validate the former pystring::split() behavior.
    {
        const StringUtils::StringVec results = StringUtils::Split("", ',');
        OCIO_REQUIRE_EQUAL(results.size(), 1);
        OCIO_CHECK_EQUAL(results[0], "");
    }

    // Test to validate the former pystring::split() behavior.
    {
        const StringUtils::StringVec results = StringUtils::Split(",", ',');
        OCIO_REQUIRE_EQUAL(results.size(), 2);
        OCIO_CHECK_EQUAL(results[0], "");
        OCIO_CHECK_EQUAL(results[1], "");
    }

    {
        const StringUtils::StringVec results = StringUtils::SplitByLines(ref);
        OCIO_REQUIRE_EQUAL(results.size(), 4);
        OCIO_CHECK_EQUAL(results[0], " \t");
        OCIO_CHECK_EQUAL(results[1], " lOwEr 1*& ctfG ");
        OCIO_CHECK_EQUAL(results[2], "");
        OCIO_CHECK_EQUAL(results[3], " ");
    }

    {
        const StringUtils::StringVec results = StringUtils::SplitByLines("\n");
        OCIO_REQUIRE_EQUAL(results.size(), 1);
        OCIO_CHECK_EQUAL(results[0], "");
    }

    // Test to validate the former pystring::splitlines() behavior.
    {
        const StringUtils::StringVec results = StringUtils::SplitByLines("");
        OCIO_CHECK_EQUAL(results.size(), 1);
        OCIO_CHECK_EQUAL(results[0], "");
    }

    // Something important to notice and preserve.
    {
        // Note: StringUtils::Split() is mainly used to parse some string content enumerating 
        // a list of substrings (i.e. separator could be a space, comma, etc). In that use case,
        // a string like ",," must return three entries. Refer to 'looks' parsing for example.
        // However, StringUtils::SplitByLines() is mainly used to read some file content where
        // "xx\n" only means one string equal to "xx".

        constexpr char content[]{"\n"};
        const StringUtils::StringVec res1 = StringUtils::Split(content, '\n');
        const StringUtils::StringVec res2 = StringUtils::SplitByLines(content);

        OCIO_CHECK_EQUAL(res1.size(), 2);
        OCIO_CHECK_EQUAL(res2.size(), 1);
    }
}

OCIO_ADD_TEST(StringUtils, searches)
{
    constexpr char ref[]{"lOwEr 1*& ctfG"};

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

OCIO_ADD_TEST(StringUtils, replace)
{
    std::string ref{"lOwEr 1*& ctfG"};

    ref = StringUtils::Replace(ref, "wEr", "12345");
    OCIO_CHECK_EQUAL(ref, "lO12345 1*& ctfG");

    ref = StringUtils::Replace(ref, "345 1*", "ABC");
    OCIO_CHECK_EQUAL(ref, "lO12ABC& ctfG");

    // Test a not existing subbstring.
    ref = StringUtils::Replace(ref, "ZY", "TO");
    OCIO_CHECK_EQUAL(ref, "lO12ABC& ctfG");

    OCIO_CHECK_ASSERT(StringUtils::ReplaceInPlace(ref, "ct", "TO"));
    OCIO_CHECK_EQUAL(ref, "lO12ABC& TOfG");

    OCIO_CHECK_ASSERT(!StringUtils::ReplaceInPlace(ref, "12345", "TO"));
    OCIO_CHECK_EQUAL(ref, "lO12ABC& TOfG");
}

OCIO_ADD_TEST(StringUtils, split_whitespaces)
{
    constexpr char ref[]{"10.0 9. 1 er\t1e-5f"};

    const StringUtils::StringVec res1 = StringUtils::SplitByWhiteSpaces(ref);
    OCIO_REQUIRE_EQUAL(res1.size(), 5);
    OCIO_CHECK_EQUAL(res1[0], "10.0");
    OCIO_CHECK_EQUAL(res1[1], "9.");
    OCIO_CHECK_EQUAL(res1[2], "1");
    OCIO_CHECK_EQUAL(res1[3], "er");
    OCIO_CHECK_EQUAL(res1[4], "1e-5f");
}

OCIO_ADD_TEST(StringUtils, find)
{
    constexpr char ref[]{"10.0 9. 1 er\t1e-5f"};

    OCIO_CHECK_EQUAL( 0, StringUtils::Find(ref, "1"));
    OCIO_CHECK_EQUAL(12, StringUtils::Find(ref, "\t"));

    OCIO_CHECK_EQUAL(std::string::npos, StringUtils::Find(ref, "TO"));
    OCIO_CHECK_EQUAL(std::string::npos, StringUtils::Find(ref, "9.1"));

    OCIO_CHECK_EQUAL(13, StringUtils::ReverseFind(ref, "1"));
    OCIO_CHECK_EQUAL(17, StringUtils::ReverseFind(ref, "f"));

    OCIO_CHECK_EQUAL(std::string::npos, StringUtils::ReverseFind(ref, "TO"));
}

OCIO_ADD_TEST(StringUtils, remove_contain)
{
    static constexpr char ref[]{"1,\t2, 3, 4,5,      6"};

    StringUtils::StringVec res = StringUtils::Split(ref, ',');

    {
        OCIO_CHECK_EQUAL(res.size(), 6);
        OCIO_CHECK_NO_THROW(StringUtils::Trim(res));
        static const StringUtils::StringVec values{"1", "2", "3", "4", "5", "6"};
        OCIO_CHECK_ASSERT(res==values);

        const std::string str = StringUtils::Join(res, ',');
        OCIO_CHECK_EQUAL(str, std::string("1, 2, 3, 4, 5, 6"));
    }

    {
        OCIO_CHECK_ASSERT(StringUtils::Contain(res, "3"));
        OCIO_CHECK_ASSERT(StringUtils::Contain(res, "6"));

        OCIO_CHECK_ASSERT(!StringUtils::Contain(res, "9"));

        OCIO_CHECK_ASSERT(StringUtils::Remove(res, "3"));
        OCIO_CHECK_EQUAL(res.size(), 5);
        OCIO_CHECK_ASSERT(!StringUtils::Contain(res, "3"));
    }
}
