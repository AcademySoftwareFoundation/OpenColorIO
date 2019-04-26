/*
Copyright (c) 2018 Autodesk Inc., et al.
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

#include "fileformats/xmlutils/XMLReaderUtils.h"

OCIO_NAMESPACE_ENTER
{

bool IsNotSpace(char c)
{
    return !IsSpace(c);
}

// Trim from start.
static inline void LTrim(std::string & s)
{
    s.erase(s.begin(),
        std::find_if(s.begin(), s.end(), IsNotSpace));
}

// Trim from end.
static inline void RTrim(std::string & s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), IsNotSpace).base(),
        s.end());
}

// Trim from both ends.
void Trim(std::string & s)
{
    LTrim(s);
    RTrim(s);
}

// Find the position of the first non-whitespace character.
// Whitespaces are defined as spaces, tabs or newlines here.
// Returns the position of the first non-whitespace character or
//         std::string::npos if the string only has whitespaces or is empty.
inline size_t FindFirstNonWhiteSpace(const char * str, size_t len)
{
    const char *ptr = str;
    size_t pos = 0;

    for (;;)
    {
        if (!IsSpace(*ptr))
        {
            return pos;
        }
        if (pos == len)
        {
            return len;
        }
        ptr++; pos++;
    }
}

// Find the position of the last the non-whitespace character.
// Whitespaces are defined as spaces, tabs or newlines here.
// Returns the position of the last non-whitespace character or
//         std::string::npos if the string only has whitespaces or is empty.
inline size_t FindLastNonWhiteSpace(const char * str, size_t len)
{
    size_t pos = len - 1;
    const char *ptr = str + pos;

    for (;;)
    {
        if (!IsSpace(*ptr))
        {
            return pos;
        }
        if (pos == 0)
        {
            return 0;
        }

        ptr--; pos--;
    }
}

// Get start (first non space character) and
// end (just after the last non space character).
void FindSubString(const char * str, size_t length,
                   size_t & start,
                   size_t & end)
{
    if (!str || !*str)
    {
        start = 0;
        end = 0;
        return; // nothing to Trim.
    }

    start = FindFirstNonWhiteSpace(str, length);
    if (start == length)
    {
        // str only contains spaces, tabs or newlines.
        // Return an empty string.
        start = 0;
        end = 0;
        return;
    }

    // It is guaranteed here that end will not be 'npos'.
    // At worst, it will equal start.
    end = FindLastNonWhiteSpace(str, length);

    // end-start should give the number of valid characters.
    if (!IsSpace(str[end])) ++end;
}

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "MathUtils.h"
#include "unittest.h"

#define EXPECTED_PRECISION 1e-11f

OIIO_ADD_TEST(XMLReaderHelper, string_to_float_failure)
{
    float value = 0.0f;
    const char str[] = "ABDSCSGFDS";
    const size_t len = strlen(str);

    OIIO_CHECK_THROW_WHAT(OCIO::ParseNumber(str, len, value),
                          OCIO::Exception,
                          "are illegal");
}

OIIO_ADD_TEST(XMLReaderHelper, trim)
{
    const std::string original1("    some text    ");
    const std::string original2(" \n \r some text  \t \v \f ");
    {
        std::string value(original1);
        OCIO::Trim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text", value.c_str()));
        value = original2;
        OCIO::Trim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text", value.c_str()));
    }

    {
        std::string value(original1);
        OCIO::RTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("    some text", value.c_str()));
        value = original2;
        OCIO::RTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp(" \n \r some text", value.c_str()));
    }

    {
        std::string value(original1);
        OCIO::LTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text    ", value.c_str()));
        value = original2;
        OCIO::LTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text  \t \v \f ", value.c_str()));
    }
}

OIIO_ADD_TEST(XMLReaderHelper, parse_number)
{
    float data = 0.0f;
    {
        std::string buffer("1 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0f 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0000 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data);)
            OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("10.0e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("0.1e+1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("-1 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0f 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0000 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data);)
            OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-10.0e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-0.1e+1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("INF");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("INFINITY");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("-INF");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("-INFINITY");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("NAN");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_ASSERT(OCIO::IsNan(data));
    }
    {
        std::string buffer("-NAN");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_ASSERT(OCIO::IsNan(data));
    }
    {
        std::string buffer("0.001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-0.001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer(".001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-.001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer(".01e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-.01e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1,");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1\n");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1\t");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }

    {
        std::string buffer("XY");
        OIIO_CHECK_THROW_WHAT(OCIO::ParseNumber(buffer.c_str(),
                                                buffer.size(),
                                                data),
                              OCIO::Exception,
                              "are illegal");
    }
    {
        std::string buffer("            1");
        OIIO_CHECK_THROW_WHAT(OCIO::ParseNumber(buffer.c_str(),
                                                buffer.size(),
                                                data),
                              OCIO::Exception,
                              "not start with a delimiter");
    }
}

OIIO_ADD_TEST(XMLReaderHelper, find_sub_string)
{
    {
        //                        012345678901234
        const std::string buffer("   new order   ");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 3);
        OIIO_CHECK_EQUAL(end, 12);
    }
    {
        //                        012345678901234
        const std::string buffer("new order   ");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 9);
    }
    {
        //                        012345678901234
        const std::string buffer("   new order");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 3);
        OIIO_CHECK_EQUAL(end, 12);
    }
    {
        //                        012345678901234
        const std::string buffer("new order");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 9);
    }
    {
        const std::string buffer("");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 0);
    }
    {
        const std::string buffer("      ");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 0);
    }
    {
        const std::string buffer("   \t123    ");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 4);
        OIIO_CHECK_EQUAL(end, 7);
    }
    {
        const std::string buffer("1   \t \n \r");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 1);
    }
    {
        const std::string buffer("\t");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 0);
    }
}

#endif
