// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "LookParse.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(LookParse, parse)
{
    OCIO::LookParseResult r;

    {
    const OCIO::LookParseResult::Options & options = r.parse("");
    OCIO_CHECK_EQUAL(options.size(), 0);
    OCIO_CHECK_EQUAL(options.empty(), true);
    }

    {
    const OCIO::LookParseResult::Options & options = r.parse("  ");
    OCIO_CHECK_EQUAL(options.size(), 0);
    OCIO_CHECK_EQUAL(options.empty(), true);
    }

    {
    const OCIO::LookParseResult::Options & options = r.parse("cc");
    OCIO_CHECK_EQUAL(options.size(), 1);
    OCIO_CHECK_EQUAL(options[0][0].name, "cc");
    OCIO_CHECK_EQUAL(options[0][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options.empty(), false);
    }

    {
    const OCIO::LookParseResult::Options & options = r.parse("+cc");
    OCIO_CHECK_EQUAL(options.size(), 1);
    OCIO_CHECK_EQUAL(options[0][0].name, "cc");
    OCIO_CHECK_EQUAL(options[0][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options.empty(), false);
    }

    {
    const OCIO::LookParseResult::Options & options = r.parse("  +cc");
    OCIO_CHECK_EQUAL(options.size(), 1);
    OCIO_CHECK_EQUAL(options[0][0].name, "cc");
    OCIO_CHECK_EQUAL(options[0][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options.empty(), false);
    }

    {
    const OCIO::LookParseResult::Options & options = r.parse("  +cc   ");
    OCIO_CHECK_EQUAL(options.size(), 1);
    OCIO_CHECK_EQUAL(options[0][0].name, "cc");
    OCIO_CHECK_EQUAL(options[0][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options.empty(), false);
    }

    {
    const OCIO::LookParseResult::Options & options = r.parse("+cc,-di");
    OCIO_CHECK_EQUAL(options.size(), 1);
    OCIO_CHECK_EQUAL(options[0].size(), 2);
    OCIO_CHECK_EQUAL(options[0][0].name, "cc");
    OCIO_CHECK_EQUAL(options[0][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options[0][1].name, "di");
    OCIO_CHECK_EQUAL(options[0][1].dir, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(options.empty(), false);
    }

    {
    const OCIO::LookParseResult::Options & options = r.parse("  +cc ,  -di");
    OCIO_CHECK_EQUAL(options.size(), 1);
    OCIO_CHECK_EQUAL(options[0].size(), 2);
    OCIO_CHECK_EQUAL(options[0][0].name, "cc");
    OCIO_CHECK_EQUAL(options[0][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options[0][1].name, "di");
    OCIO_CHECK_EQUAL(options[0][1].dir, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(options.empty(), false);
    }

    {
    const OCIO::LookParseResult::Options & options = r.parse("  +cc :  -di");
    OCIO_CHECK_EQUAL(options.size(), 1);
    OCIO_CHECK_EQUAL(options[0].size(), 2);
    OCIO_CHECK_EQUAL(options[0][0].name, "cc");
    OCIO_CHECK_EQUAL(options[0][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options[0][1].name, "di");
    OCIO_CHECK_EQUAL(options[0][1].dir, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(options.empty(), false);
    }

    {
    const OCIO::LookParseResult::Options & options = r.parse("+cc, -di |-cc");
    OCIO_CHECK_EQUAL(options.size(), 2);
    OCIO_CHECK_EQUAL(options[0].size(), 2);
    OCIO_CHECK_EQUAL(options[0][0].name, "cc");
    OCIO_CHECK_EQUAL(options[0][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options[0][1].name, "di");
    OCIO_CHECK_EQUAL(options[0][1].dir, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(options[1].size(), 1);
    OCIO_CHECK_EQUAL(options.empty(), false);
    OCIO_CHECK_EQUAL(options[1][0].name, "cc");
    OCIO_CHECK_EQUAL(options[1][0].dir, OCIO::TRANSFORM_DIR_INVERSE);
    }

    {
    const OCIO::LookParseResult::Options & options = r.parse("+cc, -di |-cc|   ");
    OCIO_CHECK_EQUAL(options.size(), 3);
    OCIO_CHECK_EQUAL(options[0].size(), 2);
    OCIO_CHECK_EQUAL(options[0][0].name, "cc");
    OCIO_CHECK_EQUAL(options[0][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options[0][1].name, "di");
    OCIO_CHECK_EQUAL(options[0][1].dir, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(options[1].size(), 1);
    OCIO_CHECK_EQUAL(options.empty(), false);
    OCIO_CHECK_EQUAL(options[1][0].name, "cc");
    OCIO_CHECK_EQUAL(options[1][0].dir, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(options[2].size(), 1);
    OCIO_CHECK_EQUAL(options[2][0].name, "");
    OCIO_CHECK_EQUAL(options[2][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    }
}

OCIO_ADD_TEST(LookParse, reverse)
{
    OCIO::LookParseResult r;

    {
    r.parse("+cc, -di |-cc|   ");
    r.reverse();
    const OCIO::LookParseResult::Options & options = r.getOptions();

    OCIO_CHECK_EQUAL(options.size(), 3);
    OCIO_CHECK_EQUAL(options[0].size(), 2);
    OCIO_CHECK_EQUAL(options[0][1].name, "cc");
    OCIO_CHECK_EQUAL(options[0][1].dir, OCIO::TRANSFORM_DIR_INVERSE);
    OCIO_CHECK_EQUAL(options[0][0].name, "di");
    OCIO_CHECK_EQUAL(options[0][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options[1].size(), 1);
    OCIO_CHECK_EQUAL(options.empty(), false);
    OCIO_CHECK_EQUAL(options[1][0].name, "cc");
    OCIO_CHECK_EQUAL(options[1][0].dir, OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_EQUAL(options[2].size(), 1);
    OCIO_CHECK_EQUAL(options[2][0].name, "");
    OCIO_CHECK_EQUAL(options[2][0].dir, OCIO::TRANSFORM_DIR_INVERSE);
    }

}

