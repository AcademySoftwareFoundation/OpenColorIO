/*
Copyright (c) 2019 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>

#include "Context.cpp"

#include "PathUtils.h"
#include "Platform.h"
#include "pystring/pystring.h"
#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

#define STR(x) FIELD_STR(x)

#ifndef OCIO_SOURCE_DIR
static_assert(0, "OCIO_SOURCE_DIR should be defined by tests/cpu/CMakeLists.txt");
#endif // OCIO_SOURCE_DIR
static const std::string ociodir(STR(OCIO_SOURCE_DIR));

// Method to compare paths.
std::string SanitizePath(const char* path)
{
    std::string s{ pystring::os::path::normpath(path) };
    return s;
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

