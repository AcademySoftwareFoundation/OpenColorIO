// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <algorithm>

#include <pystring.h>

#include "Context.cpp"

#include "PathUtils.h"
#include "Platform.h"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


#define STR(x) FIELD_STR(x)

#ifndef OCIO_SOURCE_DIR
static_assert(0, "OCIO_SOURCE_DIR should be defined by tests/cpu/CMakeLists.txt");
#endif // OCIO_SOURCE_DIR
static const std::string ociodir(STR(OCIO_SOURCE_DIR));

// Method to compare paths.
std::string SanitizePath(const char* path)
{
    return { pystring::os::path::normpath(path) };
}

OCIO_ADD_TEST(Context, search_paths)
{
    OCIO::ContextRcPtr con = OCIO::Context::Create();
    OCIO_CHECK_EQUAL(con->getNumSearchPaths(), 0);
    const std::string empty{ "" };
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath()), empty);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath(42)), empty);

    con->addSearchPath(empty.c_str());
    OCIO_CHECK_EQUAL(con->getNumSearchPaths(), 0);

    const std::string first{ "First" };
    con->addSearchPath(first.c_str());
    OCIO_CHECK_EQUAL(con->getNumSearchPaths(), 1);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath()), first);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath(0)), first);
    con->clearSearchPaths();
    OCIO_CHECK_EQUAL(con->getNumSearchPaths(), 0);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath()), empty);

    const std::string second{ "Second" };
    const std::string firstSecond{ first + ":" + second };
    con->addSearchPath(first.c_str());
    con->addSearchPath(second.c_str());
    OCIO_CHECK_EQUAL(con->getNumSearchPaths(), 2);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath()), firstSecond);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath(0)), first);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath(1)), second);
    con->addSearchPath(empty.c_str());
    OCIO_CHECK_EQUAL(con->getNumSearchPaths(), 2);

    con->setSearchPath(first.c_str());
    OCIO_CHECK_EQUAL(con->getNumSearchPaths(), 1);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath()), first);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath(0)), first);

    con->setSearchPath(firstSecond.c_str());
    OCIO_CHECK_EQUAL(con->getNumSearchPaths(), 2);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath()), firstSecond);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath(0)), first);
    OCIO_CHECK_EQUAL(std::string(con->getSearchPath(1)), second);
}

OCIO_ADD_TEST(Context, abs_path)
{
    const std::string contextpath(ociodir + std::string("/src/OpenColorIO/Context.cpp"));

    OCIO::ContextRcPtr con = OCIO::Context::Create();
    con->addSearchPath(ociodir.c_str());
    con->setStringVar("non_abs", "src/OpenColorIO/Context.cpp");
    con->setStringVar("is_abs", contextpath.c_str());
    
    OCIO_CHECK_NO_THROW(con->resolveFileLocation("${non_abs}"));

    OCIO_CHECK_ASSERT(strcmp(SanitizePath(con->resolveFileLocation("${non_abs}")).c_str(),
                             SanitizePath(contextpath.c_str()).c_str()) == 0);
    
    OCIO_CHECK_NO_THROW(con->resolveFileLocation("${is_abs}"));
    OCIO_CHECK_ASSERT(strcmp(con->resolveFileLocation("${is_abs}"),
                             SanitizePath(contextpath.c_str()).c_str()) == 0);
}

OCIO_ADD_TEST(Context, var_search_path)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    const std::string contextpath(ociodir + std::string("/src/OpenColorIO/Context.cpp"));

    context->setStringVar("SOURCE_DIR", ociodir.c_str());
    context->addSearchPath("${SOURCE_DIR}/src/OpenColorIO");

    std::string resolvedSource;
    OCIO_CHECK_NO_THROW(resolvedSource = context->resolveFileLocation("Context.cpp"));
    OCIO_CHECK_ASSERT(strcmp(SanitizePath(resolvedSource.c_str()).c_str(),
                             SanitizePath(contextpath.c_str()).c_str()) == 0);
}

OCIO_ADD_TEST(Context, use_searchpaths)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();

    // Add 2 absolute search paths.
    const std::string searchPath1 = ociodir + "/src/OpenColorIO";
    const std::string searchPath2 = ociodir + "/tests/gpu";
    context->addSearchPath(searchPath1.c_str());
    context->addSearchPath(searchPath2.c_str());

    std::string resolvedSource;
    OCIO_CHECK_NO_THROW(resolvedSource = context->resolveFileLocation("Context.cpp"));
    const std::string res1 = searchPath1 + "/Context.cpp";
    OCIO_CHECK_ASSERT(strcmp(SanitizePath(resolvedSource.c_str()).c_str(),
                             SanitizePath(res1.c_str()).c_str()) == 0);
    OCIO_CHECK_NO_THROW(resolvedSource = context->resolveFileLocation("GPUHelpers.h"));
    const std::string res2 = searchPath2 + "/GPUHelpers.h";
    OCIO_CHECK_ASSERT(strcmp(SanitizePath(resolvedSource.c_str()).c_str(),
                             SanitizePath(res2.c_str()).c_str()) == 0);
}

OCIO_ADD_TEST(Context, use_searchpaths_workingdir)
{
    OCIO::ContextRcPtr context = OCIO::Context::Create();

    // Set working directory and add 2 relative search paths. 
    const std::string searchPath1 = "src/OpenColorIO";
    const std::string searchPath2 = "tests/gpu";
    context->setWorkingDir(ociodir.c_str());
    context->addSearchPath(searchPath1.c_str());
    context->addSearchPath(searchPath2.c_str());

    std::string resolvedSource;
    OCIO_CHECK_NO_THROW(resolvedSource = context->resolveFileLocation("Context.cpp"));
    const std::string res1 = ociodir + "/" + searchPath1 + "/Context.cpp";
    OCIO_CHECK_ASSERT(strcmp(SanitizePath(resolvedSource.c_str()).c_str(),
                             SanitizePath(res1.c_str()).c_str()) == 0);
    OCIO_CHECK_NO_THROW(resolvedSource = context->resolveFileLocation("GPUHelpers.h"));
    const std::string res2 = ociodir + "/" + searchPath2 + "/GPUHelpers.h";
    OCIO_CHECK_ASSERT(strcmp(SanitizePath(resolvedSource.c_str()).c_str(),
                             SanitizePath(res2.c_str()).c_str()) == 0);
}

OCIO_ADD_TEST(Context, string_vars)
{
    // Test Context::addStringVars().

    OCIO::ContextRcPtr ctx1 = OCIO::Context::Create();
    ctx1->setStringVar("var1", "val1");
    ctx1->setStringVar("var2", "val2");

    OCIO::ContextRcPtr ctx2 = OCIO::Context::Create();
    ctx2->setStringVar("var1", "val11");
    ctx2->setStringVar("var3", "val3");

    OCIO::ConstContextRcPtr const_ctx2 = ctx2;
    ctx1->addStringVars(const_ctx2);
    OCIO_REQUIRE_EQUAL(3, ctx1->getNumStringVars());

    OCIO_CHECK_EQUAL(std::string("var1"), ctx1->getStringVarNameByIndex(0));
    OCIO_CHECK_EQUAL(std::string("val11"), ctx1->getStringVarByIndex(0));

    OCIO_CHECK_EQUAL(std::string("var2"), ctx1->getStringVarNameByIndex(1));
    OCIO_CHECK_EQUAL(std::string("val2"), ctx1->getStringVarByIndex(1));

    OCIO_CHECK_EQUAL(std::string("var3"), ctx1->getStringVarNameByIndex(2));
    OCIO_CHECK_EQUAL(std::string("val3"), ctx1->getStringVarByIndex(2));
}
