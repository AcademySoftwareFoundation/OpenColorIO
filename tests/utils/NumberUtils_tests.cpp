// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "testutils/UnitTest.h"
#include "utils/NumberUtils.h"

#include <limits>

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(NumberUtils, from_chars_float)
{
#define TEST_FROM_CHARS(text) \
    str = text; \
    res = OCIO::NumberUtils::from_chars(str.data(), str.data() + str.size(), val); \
    OCIO_CHECK_ASSERT(res.ec == std::errc()); \
    OCIO_CHECK_ASSERT(res.ptr == str.data() + str.size())

    std::string str;
    float val = 0.0f;
    OCIO::NumberUtils::from_chars_result res;

    // regular numbers
    TEST_FROM_CHARS("-7"); OCIO_CHECK_EQUAL(val, -7.0f);
    TEST_FROM_CHARS("1.5"); OCIO_CHECK_EQUAL(val, 1.5f);
    TEST_FROM_CHARS("-17.25"); OCIO_CHECK_EQUAL(val, -17.25f);
    TEST_FROM_CHARS("-.75"); OCIO_CHECK_EQUAL(val, -.75f);
    TEST_FROM_CHARS("11."); OCIO_CHECK_EQUAL(val, 11.0f);
    // exponent notation
    TEST_FROM_CHARS("1e3"); OCIO_CHECK_EQUAL(val, 1000.0f);
    TEST_FROM_CHARS("1e+2"); OCIO_CHECK_EQUAL(val, 100.0f);
    TEST_FROM_CHARS("50e-2"); OCIO_CHECK_EQUAL(val, 0.5f);
    TEST_FROM_CHARS("-1.5e2"); OCIO_CHECK_EQUAL(val, -150.0f);
    // whitespace/prefix handling
    TEST_FROM_CHARS("+57.125"); OCIO_CHECK_EQUAL(val, +57.125f);
    TEST_FROM_CHARS("  \t 123.5"); OCIO_CHECK_EQUAL(val, 123.5f);
    // special values
    TEST_FROM_CHARS("-infinity"); OCIO_CHECK_EQUAL(val, -std::numeric_limits<float>::infinity());
    TEST_FROM_CHARS("nan"); OCIO_CHECK_ASSERT(std::isnan(val));
    // hex format should be parsed
    TEST_FROM_CHARS("0x42"); OCIO_CHECK_EQUAL(val, 66.0f);
    TEST_FROM_CHARS("0x42ab.c"); OCIO_CHECK_EQUAL(val, 17067.75f);

    // valid numbers with trailing non-number chars should stop there
    str = "-7.5ab";
    res = OCIO::NumberUtils::from_chars(str.data(), str.data() + str.size(), val);
    OCIO_CHECK_ASSERT(res.ptr == str.data() + 4);
    OCIO_CHECK_EQUAL(val, -7.5f);
    str = "infinitya";
    res = OCIO::NumberUtils::from_chars(str.data(), str.data() + str.size(), val);
    OCIO_CHECK_ASSERT(res.ptr == str.data() + 8);
    OCIO_CHECK_EQUAL(val, std::numeric_limits<float>::infinity());
    str = "0x18g";
    res = OCIO::NumberUtils::from_chars(str.data(), str.data() + str.size(), val);
    OCIO_CHECK_ASSERT(res.ptr == str.data() + 4);
    OCIO_CHECK_EQUAL(val, 24.0f);

#undef TEST_FROM_CHARS
}

OCIO_ADD_TEST(NumberUtils, from_chars_float_failures)
{
#define TEST_FROM_CHARS(text) \
    str = text; \
    res = OCIO::NumberUtils::from_chars(str.data(), str.data() + str.size(), val); \
    OCIO_CHECK_EQUAL(val, 7.5f); \
    OCIO_CHECK_ASSERT(res.ec == std::errc::invalid_argument)

    std::string str;
    float val = 7.5f;
    OCIO::NumberUtils::from_chars_result res;

    TEST_FROM_CHARS("");
    TEST_FROM_CHARS("ab");
    TEST_FROM_CHARS("   ");
    TEST_FROM_CHARS("---");
    TEST_FROM_CHARS("e3");
    TEST_FROM_CHARS("_x");
    TEST_FROM_CHARS("+.");

#undef TEST_FROM_CHARS
}
