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

OIIO_ADD_TEST(XMLReaderHelper, string_to_float)
{
    float value = 0.0f;
    const char str[] = "12345";
    const size_t len = strlen(str);

    OIIO_CHECK_NO_THROW(OCIO::ParseNumber(str, 0, len, value));
    OIIO_CHECK_EQUAL(value, 12345.0f);
}

OIIO_ADD_TEST(XMLReaderHelper, string_to_float_failure)
{
    float value = 0.0f;
    const char str[] = "ABDSCSGFDS";
    const size_t len = strlen(str);

    OIIO_CHECK_THROW_WHAT(OCIO::ParseNumber(str, 0, len, value),
                          OCIO::Exception,
                          "are illegal");


    const char str1[] = "10 ";
    const size_t len1 = strlen(str1);

    OIIO_CHECK_THROW_WHAT(OCIO::ParseNumber(str1, 0, len1, value),
                          OCIO::Exception,
                          "followed by characters");

    // 2 characters are parsed and this is the length required.
    OIIO_CHECK_NO_THROW(OCIO::ParseNumber(str1, 0, 2, value));

    const char str2[] = "12345";
    const size_t len2 = strlen(str2);
    // All characters are parsed and this is more than the required length.
    // The string to double function strtod does not stop at a given length,
    // but we detect that strtod did read too many characters.
    OIIO_CHECK_THROW_WHAT(OCIO::ParseNumber(str2, 0, len2 - 2, value),
                          OCIO::Exception,
                          "followed by characters");


    const char str3[] = "123XX";
    const size_t len3 = strlen(str3);
    // Strtod will stop after parsing 123 and this happens to be the
    // excact length that is required to be parsed.
    OIIO_CHECK_NO_THROW(OCIO::ParseNumber(str3, 0, len3 - 2, value));
}

OIIO_ADD_TEST(XMLReaderHelper, get_numbers)
{
    const char str[] = "1.0 , 2.0     3.0,4";
    const size_t len = strlen(str);

    std::vector<float> values;
    OIIO_CHECK_NO_THROW(values = OCIO::GetNumbers<float>(str, len));
    OIIO_REQUIRE_EQUAL(values.size(), 4);
    OIIO_CHECK_EQUAL(values[0], 1.0f);
    OIIO_CHECK_EQUAL(values[1], 2.0f);
    OIIO_CHECK_EQUAL(values[2], 3.0f);
    OIIO_CHECK_EQUAL(values[3], 4.0f);

    // Same test without a null terminated string:
    // Copy the string into a buffer that will not be null terminated.
    // Add a delimiter at the end of the buffer.
    char * buffer = new char[len+1];
    memcpy(buffer, str, len * sizeof(char));
    buffer[len] = '\n';
    OIIO_CHECK_NO_THROW(values = OCIO::GetNumbers<float>(buffer, len));
    OIIO_REQUIRE_EQUAL(values.size(), 4);
    OIIO_CHECK_EQUAL(values[0], 1.0f);
    OIIO_CHECK_EQUAL(values[1], 2.0f);
    OIIO_CHECK_EQUAL(values[2], 3.0f);
    OIIO_CHECK_EQUAL(values[3], 4.0f);

    // Testing with more values.
    const char str1[] = "inf, -infinity 1.0, -2.0 0x42 nan  , -nan 5.0";
    const size_t len1 = strlen(str1);

    OIIO_CHECK_NO_THROW(values = OCIO::GetNumbers<float>(str1, len1));
    OIIO_REQUIRE_EQUAL(values.size(), 8);
    OIIO_CHECK_EQUAL(values[2], 1.0f);
    OIIO_CHECK_EQUAL(values[4], 66.0f); // 0x42

    // It is valid to start with delimiters.
    const char str2[] = ",  ,, , 0 2.0 3.0";
    const size_t len2 = strlen(str2);

    OIIO_CHECK_NO_THROW(values = OCIO::GetNumbers<float>(str2, len2));
    OIIO_REQUIRE_EQUAL(values.size(), 3);
    OIIO_CHECK_EQUAL(values[0], 0.0f);
    OIIO_CHECK_EQUAL(values[2], 3.0f);

    // Error: text is not a number.
    const char str3[] = "0   error 2.0 3.0";
    const size_t len3 = strlen(str3);

    OIIO_CHECK_THROW_WHAT(values = OCIO::GetNumbers<float>(str3, len3),
                          OCIO::Exception,
                          "are illegal");

    // Error: number is not separated from text.
    const char str4[] = "0   1.0error 2.0 3.0";
    const size_t len4 = strlen(str4);

    OIIO_CHECK_THROW_WHAT(values = OCIO::GetNumbers<float>(str4, len4),
                          OCIO::Exception,
                          "followed by characters");
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
                                              0, 1, data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 3, data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0000 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 6, data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 3, data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 1, data);)
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("10.0e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("0.1e+1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("-1 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 2, data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 4, data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0000 0");
        size_t end = OCIO::FindDelim(buffer.c_str(), buffer.size(), 0);
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, end, data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("  -1.0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("   -1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data);)
            OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer(" -10.0e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-0.1e+1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("INF");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 3, data));
        OIIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("INF 1.0 2.0");
        size_t pos = 0;
        const size_t size = buffer.size();
        size_t nextPos = OCIO::FindDelim(buffer.c_str(), size, pos);
        OIIO_CHECK_EQUAL(nextPos, 3);
        OIIO_CHECK_NO_THROW(
            OCIO::ParseNumber(buffer.c_str(),
                              pos, nextPos, data));
        OIIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
        pos = OCIO::FindNextTokenStart(buffer.c_str(), size, nextPos);
        OIIO_CHECK_EQUAL(pos, 4);
        nextPos = OCIO::FindDelim(buffer.c_str(), size, pos);
        OIIO_CHECK_EQUAL(nextPos, 7);
        OIIO_CHECK_NO_THROW(
            OCIO::ParseNumber(buffer.c_str(),
                              pos, nextPos, data));
        OIIO_CHECK_EQUAL(data, 1.0f);
        pos = OCIO::FindNextTokenStart(buffer.c_str(), size, nextPos);
        OIIO_CHECK_EQUAL(pos, 8);
        nextPos = OCIO::FindDelim(buffer.c_str(), size, pos);
        OIIO_CHECK_EQUAL(nextPos, 11);
        OIIO_CHECK_NO_THROW(
            OCIO::ParseNumber(buffer.c_str(),
                              pos, nextPos, data));
        OIIO_CHECK_EQUAL(data, 2.0f);
    }
    {
        std::string buffer("INFINITY");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("-INF");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, -std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("-INFINITY");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, -std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("NAN");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_ASSERT(OCIO::IsNan(data));
    }
    {
        std::string buffer("-NAN");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_ASSERT(OCIO::IsNan(data));
    }
    {
        std::string buffer("0.001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-0.001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer(".001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-.001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer(".01e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-.01e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1,");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size() - 1, data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1\n");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size() - 1, data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1\t");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size() - 1, data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("10E-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("0.10E01");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }

    {
        std::string buffer("XY");
        OIIO_CHECK_THROW_WHAT(OCIO::ParseNumber(buffer.c_str(),
                                                0, 2, data),
                              OCIO::Exception,
                              "are illegal");
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
