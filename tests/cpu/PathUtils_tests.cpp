// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "PathUtils.cpp"

#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(PathUtils, env_expand)
{
    // build env by hand for unit test
    OCIO::EnvMap env_map; // = OCIO::GetEnvMap();

    // add some fake env vars so the test runs
    env_map.insert(OCIO::EnvMap::value_type("TEST1", "foo.bar"));
    env_map.insert(OCIO::EnvMap::value_type("TEST1NG", "bar.foo"));
    env_map.insert(OCIO::EnvMap::value_type("FOO_foo.bar", "cheese"));

    //
    std::string foo = "/a/b/${TEST1}/${TEST1NG}/$TEST1/$TEST1NG/${FOO_${TEST1}}/";
    std::string foo_result = "/a/b/foo.bar/bar.foo/foo.bar/bar.foo/cheese/";
    std::string testresult = OCIO::EnvExpand(foo, env_map);
    OCIO_CHECK_ASSERT( testresult == foo_result );
}

