// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "ContextVariableUtils.cpp"

#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(ContextVariableUtils, env_check)
{
    // Test the detection of the context variables.

    OCIO_CHECK_ASSERT(!OCIO::ContainsContextVariableToken("1234"));

    OCIO_CHECK_ASSERT(OCIO::ContainsContextVariableToken("${1234}"));
    OCIO_CHECK_ASSERT(OCIO::ContainsContextVariableToken("${1234"));
    OCIO_CHECK_ASSERT(OCIO::ContainsContextVariableToken("$1234"));
    OCIO_CHECK_ASSERT(!OCIO::ContainsContextVariableToken("{1234}"));

    OCIO_CHECK_ASSERT(OCIO::ContainsContextVariableToken("1234%"));
    OCIO_CHECK_ASSERT(OCIO::ContainsContextVariableToken("12%34"));
    OCIO_CHECK_ASSERT(OCIO::ContainsContextVariableToken("123%4%"));


    OCIO_CHECK_ASSERT(!OCIO::ContainsContextVariables("1234"));
    OCIO_CHECK_ASSERT(OCIO::ContainsContextVariables("${1234}"));
    OCIO_CHECK_ASSERT(!OCIO::ContainsContextVariables("%1234"));
    OCIO_CHECK_ASSERT(OCIO::ContainsContextVariables("%1234%"));

    // Test succeeds even if '{1234' is a suspicious name for a context variable.
    OCIO_CHECK_ASSERT(OCIO::ContainsContextVariables("${1234"));
}

OCIO_ADD_TEST(ContextVariableUtils, env_expand)
{
    // Test the resolution of the context variables.

    // Build env by hand for unit test.
    OCIO::EnvMap env_map;

    // Add some fake context variables so the test runs.
    env_map.insert(OCIO::EnvMap::value_type("TEST1", "foo.bar"));
    env_map.insert(OCIO::EnvMap::value_type("TEST1NG", "bar.foo"));
    env_map.insert(OCIO::EnvMap::value_type("FOO_foo.bar", "cheese"));

    // The string to test.
    static const std::string foo = "/a/b/${TEST1}/${TEST1NG}/%TEST1%/$TEST1NG/${FOO_${TEST1}}/";
    static const std::string foo_result = "/a/b/foo.bar/bar.foo/foo.bar/bar.foo/cheese/";

    OCIO::UsedEnvs usedEnvs;

    {
        // Resolve the string.
        const std::string testresult = OCIO::ResolveContextVariables(foo, env_map, usedEnvs);

        // Check the resulting string.
        OCIO_CHECK_EQUAL(testresult, foo_result);
        OCIO_CHECK_ASSERT(!OCIO::ContainsContextVariables(testresult.c_str()));

        // Check the used context variables.
        OCIO_CHECK_EQUAL(3, usedEnvs.size());
        OCIO::UsedEnvs::iterator iter = usedEnvs.begin();
        OCIO_CHECK_EQUAL(std::string("FOO_foo.bar"), iter->first);
        OCIO_CHECK_EQUAL(std::string("cheese"), iter->second);
        ++iter;
        OCIO_CHECK_EQUAL(std::string("TEST1"), iter->first);
        OCIO_CHECK_EQUAL(std::string("foo.bar"), iter->second);
        ++iter;
        OCIO_CHECK_EQUAL(std::string("TEST1NG"), iter->first);
        OCIO_CHECK_EQUAL(std::string("bar.foo"), iter->second);
    }

    // Now, test some faulty cases.

    env_map.clear();
    env_map.insert(OCIO::EnvMap::value_type("TEST1", "foo.bar"));
    env_map.insert(OCIO::EnvMap::value_type("TEST1NG", "bar.foo"));

    usedEnvs.clear();
    {
        // That's a right context variable syntax but the env does not contain one of the vars. 
        const std::string testresult = OCIO::ResolveContextVariables(foo, env_map, usedEnvs);

        // Check the resulting string.
        OCIO_CHECK_EQUAL(testresult, "/a/b/foo.bar/bar.foo/foo.bar/bar.foo/${FOO_foo.bar}/");
        OCIO_CHECK_ASSERT(OCIO::ContainsContextVariables(testresult.c_str()));

        // Check the used context variables.
        OCIO_CHECK_EQUAL(2, usedEnvs.size());
        OCIO::UsedEnvs::iterator iter = usedEnvs.begin();
        OCIO_CHECK_EQUAL(std::string("TEST1"), iter->first);
        OCIO_CHECK_EQUAL(std::string("foo.bar"), iter->second);
        ++iter;
        OCIO_CHECK_EQUAL(std::string("TEST1NG"), iter->first);
        OCIO_CHECK_EQUAL(std::string("bar.foo"), iter->second);
    }

    usedEnvs.clear();
    {
        // That's also a right context variable syntax but it still does not exist. 
        const std::string testresult = OCIO::ResolveContextVariables("$TEST2", env_map, usedEnvs);
        OCIO_CHECK_EQUAL(testresult, "$TEST2");
        OCIO_CHECK_ASSERT(OCIO::ContainsContextVariables(testresult.c_str()));
        OCIO_CHECK_EQUAL(0, usedEnvs.size());
    }

    usedEnvs.clear();
    {
        // That's not a context variable because of a wrong syntax. But a context variable named
        // TEST1 exists so it means that %TEST1% would have succeeded.
        const std::string testresult = OCIO::ResolveContextVariables("%TEST1", env_map, usedEnvs);
        OCIO_CHECK_EQUAL(testresult, "%TEST1");
        OCIO_CHECK_ASSERT(!OCIO::ContainsContextVariables(testresult.c_str()));
        OCIO_CHECK_EQUAL(0, usedEnvs.size());
    }

    usedEnvs.clear();
    {
        // That's still not a context variable because of a wrong syntax.
        const std::string testresult = OCIO::ResolveContextVariables("TEST1%", env_map, usedEnvs);
        OCIO_CHECK_EQUAL(testresult, "TEST1%");
        OCIO_CHECK_ASSERT(!OCIO::ContainsContextVariables(testresult.c_str()));
        OCIO_CHECK_EQUAL(0, usedEnvs.size());
    }

    usedEnvs.clear();
    {
        // That's an ambiguous context variable as the syntax is right but the name is '{TEST1'
        // which does not exist (but 'TEST1' exists).
        const std::string testresult = OCIO::ResolveContextVariables("${TEST1", env_map, usedEnvs);
        OCIO_CHECK_EQUAL(testresult, "${TEST1");
        OCIO_CHECK_ASSERT(OCIO::ContainsContextVariables(testresult.c_str()));
        OCIO_CHECK_EQUAL(0, usedEnvs.size());
    }
}

