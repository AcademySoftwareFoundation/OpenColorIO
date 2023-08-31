 /*
  Copyright 2010 Larry Gritz and the other authors and contributors.
  All Rights Reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  * Neither the name of the software's owners nor the names of its
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

  (This is the Modified BSD License)
*/

#ifndef INCLUDED_OCIO_UNITTEST_H
#define INCLUDED_OCIO_UNITTEST_H

#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

extern int unit_test_failures;

void unittest_fail();

using OCIOTestFuncCallback = std::function<void(void)>;

struct OCIOTest
{
    OCIOTest(const std::string & testgroup, const std::string & testname, const OCIOTestFuncCallback & test)
        :    group(testgroup), name(testname), function(test)
    { };

    std::string group, name;
    OCIOTestFuncCallback function;
};

class SkipException : public std::exception {};

typedef std::shared_ptr<OCIOTest> OCIOTestRcPtr;
typedef std::vector<OCIOTestRcPtr> UnitTests;

UnitTests & GetUnitTests();

extern int unit_test_failures;

struct AddTest
{
    explicit AddTest(const OCIOTestRcPtr & test)
    {
        GetUnitTests().push_back(test);
    }
};


// Method running through all the unit tests.
int UnitTestMain(int argc, const char ** argv);


// Helper macro.

#ifdef FIELD_STR
    #error Unexpected defined macro 'FIELD_STR'
#endif

// Refer to https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html
// for the macro 'stringizing' and MISRA C++ 2008, 16-3-1 for the potential issue
// of having several # (and/or ##) in the same macro.
#define FIELD_STR(field) #field


#ifdef OCIO_CHECK_ASSERT
    #error Unexpected defined macro 'OCIO_CHECK_ASSERT'
#endif

/// OCIO_CHECK_* macros checks if the conditions is met, and if not,
/// prints an error message indicating the module and line where the
/// error occurred, but does NOT abort.  This is helpful for unit tests
/// where we do not want one failure.
///
/// OCIO_REQUIRE_* macros checks if the conditions is met, and if not,
/// prints an error message indicating the module and line where the
/// error occurred, but does abort.  This is helpful for unit tests
/// where we have to fail as following code would be not testable.
///
#define OCIO_CHECK_ASSERT_FROM(x, line)                                 \
    ((x) ? ((void)0)                                                    \
         : ((std::cout << __FILE__ << ":" << line << ":\n"              \
                       << "FAILED: " << FIELD_STR(x) << "\n"),          \
            (void)++unit_test_failures))

#define OCIO_CHECK_ASSERT(x) OCIO_CHECK_ASSERT_FROM(x, __LINE__)

#define OCIO_REQUIRE_ASSERT_FROM(x, line)                               \
    if(!(x)) {                                                          \
        std::stringstream ss;                                           \
        ss <<  __FILE__ << ":" << line << ":\n"                         \
           << "FAILED: " << FIELD_STR(x) << "\n";                       \
        throw std::runtime_error(ss.str()); }

#define OCIO_REQUIRE_ASSERT(x) OCIO_REQUIRE_ASSERT_FROM(x, __LINE__)

#define OCIO_CHECK_ASSERT_MESSAGE_FROM(x, M, line)                      \
    ((x) ? ((void)0)                                                    \
         : ((std::cout << __FILE__ << ":" << line << ":\n"              \
                       << "FAILED: " << M << "\n"),                     \
            (void)++unit_test_failures))

#define OCIO_CHECK_ASSERT_MESSAGE(x, M)                                 \
    OCIO_CHECK_ASSERT_MESSAGE_FROM(x, M, __LINE__)

#define OCIO_CHECK_EQUAL(x,y) OCIO_CHECK_EQUAL_FROM(x,y,__LINE__)

// When using OCIO_CHECK_EQUAL in an helper method used by one or more
// unit tests, the error message indicates the helper method line number
// and not the unit test line number.
//
// Use OCIO_CHECK_EQUAL_FROM to propagate the right line number
// to the error message.
//
#define OCIO_CHECK_EQUAL_FROM(x,y,line)                                 \
    (((x) == (y)) ? ((void)0)                                           \
         : ((std::cout << __FILE__ << ":" << line << ":\n"              \
             << "FAILED: " << FIELD_STR(x) << " == " << FIELD_STR(y) << "\n" \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OCIO_REQUIRE_EQUAL_FROM(x,y, line)                              \
    if((x)!=(y)) {                                                      \
        std::stringstream ss;                                           \
        ss <<  __FILE__ << ":" << line << ":\n"                         \
           << "FAILED: " << FIELD_STR(x) << " == " << FIELD_STR(y) << "\n" \
           << "\tvalues were '" << (x) << "' and '" << (y) << "'\n";    \
        throw std::runtime_error(ss.str()); }

#define OCIO_REQUIRE_EQUAL(x,y) OCIO_REQUIRE_EQUAL_FROM(x,y, __LINE__)

#define OCIO_CHECK_NE(x,y)                                              \
    (((x) != (y)) ? ((void)0)                                           \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << FIELD_STR(x) << " != " << FIELD_STR(y) << "\n" \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OCIO_CHECK_LT(x,y)                                              \
    (((x) < (y)) ? ((void)0)                                            \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << FIELD_STR(x) << " < " << FIELD_STR(y) << "\n" \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OCIO_CHECK_GT(x,y)                                              \
    (((x) > (y)) ? ((void)0)                                            \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << FIELD_STR(x) << " > " << FIELD_STR(y) << "\n" \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OCIO_CHECK_LE(x,y)                                              \
    (((x) <= (y)) ? ((void)0)                                           \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << FIELD_STR(x) << " <= " << FIELD_STR(y) << "\n" \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OCIO_CHECK_GE(x,y)                                              \
    (((x) >= (y)) ? ((void)0)                                           \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << FIELD_STR(x) << " >= " << FIELD_STR(y) << "\n" \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OCIO_CHECK_CLOSE(x,y,tol) OCIO_CHECK_CLOSE_FROM(x,y,tol,__LINE__)

// When using OCIO_CHECK_CLOSE in an helper method used by one or more
// unit tests, the error message indicates the helper method line number
// and not the unit test line number.
//
// Use OCIO_CHECK_CLOSE_FROM to propagate the right line number
// to the error message.
//
#define OCIO_CHECK_CLOSE_FROM(x,y,tol,line)                             \
    ((std::abs((x) - (y)) < (tol)) ? ((void)0)                          \
         : ((std::cout << std::setprecision(10)                         \
             << __FILE__ << ":" << line << ":\n"                        \
             << "FAILED: abs(" << FIELD_STR(x)<< " - "                  \
                               << FIELD_STR(y) << ") < "                \
                               << FIELD_STR(tol)                        \
             << "\n"                                                    \
             << "\tvalues were '" << (x) << "', '" << (y) << "' and '"  \
             << (tol) << "'\n"), (void)++unit_test_failures))

#define OCIO_CHECK_THROW(S, E)                                          \
    try { S; throw "throwanything"; } catch( E const& ) { }             \
    catch (...) {                                                       \
        std::cout << __FILE__ << ":" << __LINE__ << ":\n"               \
                  << "FAILED: " << FIELD_STR(E)                         \
                  << " is expected to be thrown\n";                     \
        ++unit_test_failures;                                           \
    }

/// Check that an exception E is thrown and that what() contains W
/// When a function can throw different exceptions this can be used
/// to verify that the right one is thrown.
#define OCIO_CHECK_THROW_WHAT(S, E, W)                                  \
    try { S; throw "throwanything"; } catch (E const& ex) {             \
        const std::string what(ex.what());                              \
        if (std::string(W).empty() || what.empty()                      \
                || what.find(W) == std::string::npos) {                 \
            std::cout << __FILE__ << ":" << __LINE__ << ":\n"           \
            << "FAILED: " << FIELD_STR(E) << " was thrown with \""      \
            << what <<  "\". Expecting to contain \"" << W << "\"\n";   \
            ++unit_test_failures;                                       \
        }                                                               \
    } catch (...) {                                                     \
        std::cout << __FILE__ << ":" << __LINE__ << ":\n"               \
        << "FAILED: " << FIELD_STR(E) << " is expected to be thrown\n"; \
        ++unit_test_failures; }

#define OCIO_CHECK_NO_THROW(S) OCIO_CHECK_NO_THROW_FROM(S, __LINE__)

#define OCIO_CHECK_NO_THROW_FROM(S, line)                               \
    try {                                                               \
        S;                                                              \
    } catch (std::exception & ex ) {                                    \
        std::cout << __FILE__ << ":" << line << ":\n"                   \
            << "FAILED: exception thrown from " << FIELD_STR(S)         \
            << ": \"" << ex.what() << "\"\n";                           \
        ++unit_test_failures;                                           \
    } catch (...) {                                                     \
        std::cout << __FILE__ << ":" << line << ":\n"                   \
        << "FAILED: exception thrown from " << FIELD_STR(S) <<"\n";     \
        ++unit_test_failures; }

// Note: Add a SonarCloud tag to suppress all warnings for the following method.
#define OCIO_ADD_TEST(group, name)                                      \
    static void ociotest_##group##_##name();                            \
    AddTest ocioaddtest_##group##_##name(                               \
        std::make_shared<OCIOTest>(FIELD_STR(group), FIELD_STR(name),   \
                                   ociotest_##group##_##name));         \
    /* @SuppressWarnings('all') */                                      \
    static void ociotest_##group##_##name()

#endif /* INCLUDED_OCIO_UNITTEST_H */
