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

#ifndef OPENCOLORIO_UNITTEST_H
#define OPENCOLORIO_UNITTEST_H

#include <iostream>
#include <cmath>
#include <vector>
#include <string>

extern int unit_test_failures;

void unittest_fail();

typedef void (*OIIOTestFunc)();

struct OIIOTest
{
    OIIOTest(std::string testgroup, std::string testname, OIIOTestFunc test) :
        group(testgroup), name(testname), function(test) { };
    std::string group, name;
    OIIOTestFunc function;
};

typedef std::vector<OIIOTest*> UnitTests;

UnitTests& GetUnitTests();

struct AddTest { AddTest(OIIOTest* test); };

/// OIIO_CHECK_* macros checks if the conditions is met, and if not,
/// prints an error message indicating the module and line where the
/// error occurred, but does NOT abort.  This is helpful for unit tests
/// where we do not want one failure.
#define OIIO_CHECK_ASSERT(x)                                            \
    ((x) ? ((void)0)                                                    \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
                       << "FAILED: " << #x << "\n"),                    \
            (void)++unit_test_failures))

#define OIIO_CHECK_EQUAL(x,y)                                           \
    (((x) == (y)) ? ((void)0)                                           \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << #x << " == " << #y << "\n"                \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OIIO_CHECK_NE(x,y)                                              \
    (((x) != (y)) ? ((void)0)                                           \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << #x << " != " << #y << "\n"                \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OIIO_CHECK_LT(x,y)                                              \
    (((x) < (y)) ? ((void)0)                                            \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << #x << " < " << #y << "\n"                 \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OIIO_CHECK_GT(x,y)                                              \
    (((x) > (y)) ? ((void)0)                                            \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << #x << " > " << #y << "\n"                 \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OIIO_CHECK_LE(x,y)                                              \
    (((x) <= (y)) ? ((void)0)                                           \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << #x << " <= " << #y << "\n"                \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OIIO_CHECK_GE(x,y)                                              \
    (((x) >= (y)) ? ((void)0)                                           \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: " << #x << " >= " << #y << "\n"                \
             << "\tvalues were '" << (x) << "' and '" << (y) << "'\n"), \
            (void)++unit_test_failures))

#define OIIO_CHECK_CLOSE(x,y,tol)                                       \
    ((std::abs((x) - (y)) < tol) ? ((void)0)                            \
         : ((std::cout << __FILE__ << ":" << __LINE__ << ":\n"          \
             << "FAILED: abs(" << #x << " - " << #y << ") < " << #tol << "\n" \
             << "\tvalues were '" << (x) << "', '" << (y) << "' and '" << (tol) << "'\n"), \
            (void)++unit_test_failures))

#define OIIO_CHECK_THOW(S, E)                                           \
    try { S; throw "throwanything"; } catch( E const& ex ) { } catch (...) { \
        std::cout << __FILE__ << ":" << __LINE__ << ":\n"               \
        << "FAILED: " << #E << " is expected to be thrown\n";           \
        ++unit_test_failures; }

#define OIIO_CHECK_NO_THOW(S)                                           \
    try { S; } catch (...) {                                            \
        std::cout << __FILE__ << ":" << __LINE__ << ":\n"               \
        << "FAILED: exception thrown from " << #S <<"\n";               \
        ++unit_test_failures; }

#define OIIO_ADD_TEST(group, name)                                      \
    static void oiiotest_##group##_##name();                            \
    AddTest oiioaddtest_##group##_##name(new OIIOTest(#group, #name, oiiotest_##group##_##name)); \
    static void oiiotest_##group##_##name()

#define OIIO_TEST_SETUP() \
    int unit_test_failures = 0

#define OIIO_TEST_APP(app)                                              \
    std::vector<OIIOTest*>& GetUnitTests() {                            \
        static std::vector<OIIOTest*> oiio_unit_tests;                  \
        return oiio_unit_tests; }                                       \
    AddTest::AddTest(OIIOTest* test){GetUnitTests().push_back(test);};  \
    OIIO_TEST_SETUP(); \
    int main(int, char **) { std::cerr << "\n" << #app <<"\n\n";        \
        for(size_t i = 0; i < GetUnitTests().size(); ++i) {             \
            int _tmp = unit_test_failures; GetUnitTests()[i]->function(); \
            std::cerr << "Test [" << GetUnitTests()[i]->group << "] [" << GetUnitTests()[i]->name << "] - "; \
            std::cerr << (_tmp == unit_test_failures ? "PASSED" : "FAILED") << "\n"; } \
        std::cerr << "\n" << unit_test_failures << " tests failed\n\n";   \
        return unit_test_failures; }

#endif /* OPENCOLORIO_UNITTEST_H */
