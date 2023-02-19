// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>

#include "fileformats/xmlutils/XMLReaderUtils.cpp"

#include "MathUtils.h"
#include "testutils/UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(XMLReaderHelper, string_to_float)
{
    float value = 0.0f;
    const char str[] = "12345";
    const size_t len = std::strlen(str);

    OCIO_CHECK_NO_THROW(OCIO::ParseNumber(str, 0, len, value));
    OCIO_CHECK_EQUAL(value, 12345.0f);
}

OCIO_ADD_TEST(XMLReaderHelper, string_to_float_failure)
{
    float value = 0.0f;
    const char str[] = "ABDSCSGFDS";
    const size_t len = std::strlen(str);

    OCIO_CHECK_THROW_WHAT(OCIO::ParseNumber(str, 0, len, value),
                          OCIO::Exception,
                          "can not be parsed");


    const char str1[] = "10 ";
    const size_t len1 = std::strlen(str1);

    OCIO_CHECK_THROW_WHAT(OCIO::ParseNumber(str1, 0, len1, value),
                          OCIO::Exception,
                          "followed by unexpected characters");

    // 2 characters are parsed and this is the length required.
    OCIO_CHECK_NO_THROW(OCIO::ParseNumber(str1, 0, 2, value));

    const char str2[] = "12345";
    const size_t len2 = std::strlen(str2);
    // All characters are parsed and this is more than the required length.
    // The string to double function strtod does not stop at a given length,
    // but we detect that strtod did read too many characters.
    OCIO_CHECK_THROW_WHAT(OCIO::ParseNumber(str2, 0, len2 - 2, value),
                          OCIO::Exception,
                          "followed by unexpected characters");


    const char str3[] = "123XX";
    const size_t len3 = std::strlen(str3);
    // Strtod will stop after parsing 123 and this happens to be the
    // exact length that is required to be parsed.
    OCIO_CHECK_NO_THROW(OCIO::ParseNumber(str3, 0, len3 - 2, value));
}

OCIO_ADD_TEST(XMLReaderHelper, get_numbers)
{
    const char str[] = "  1.0 , 2.0     3.0,4";
    const size_t len = std::strlen(str);

    std::vector<float> values;
    OCIO_CHECK_NO_THROW(values = OCIO::GetNumbers<float>(str, len));
    OCIO_REQUIRE_EQUAL(values.size(), 4);
    OCIO_CHECK_EQUAL(values[0], 1.0f);
    OCIO_CHECK_EQUAL(values[1], 2.0f);
    OCIO_CHECK_EQUAL(values[2], 3.0f);
    OCIO_CHECK_EQUAL(values[3], 4.0f);

    // Same test without a null terminated string:
    // Copy the string into a buffer that will not be null terminated.
    // Add a delimiter at the end of the buffer.
    std::vector<char> buffer(len+1);
    std::memcpy(buffer.data(), str, len * sizeof(char));
    buffer[len] = '\n';
    OCIO_CHECK_NO_THROW(values = OCIO::GetNumbers<float>(buffer.data(), len));
    OCIO_REQUIRE_EQUAL(values.size(), 4);
    OCIO_CHECK_EQUAL(values[0], 1.0f);
    OCIO_CHECK_EQUAL(values[1], 2.0f);
    OCIO_CHECK_EQUAL(values[2], 3.0f);
    OCIO_CHECK_EQUAL(values[3], 4.0f);

    // Testing with more values.
    const char str1[] = "inf, -infinity 1.0, -2.0 0x42 nan  , -nan 5.0";
    const size_t len1 = strlen(str1);

    OCIO_CHECK_NO_THROW(values = OCIO::GetNumbers<float>(str1, len1));
    OCIO_REQUIRE_EQUAL(values.size(), 8);
    OCIO_CHECK_ASSERT(std::isinf(values[0]));
    OCIO_CHECK_ASSERT(std::isinf(values[1]));
    OCIO_CHECK_EQUAL(values[2], 1.0f);
    OCIO_CHECK_EQUAL(values[3], -2.0f);
    OCIO_CHECK_EQUAL(values[4], 66.0f);         // i.e. 0x42
    OCIO_CHECK_ASSERT(OCIO::IsNan(values[5]));
    OCIO_CHECK_ASSERT(OCIO::IsNan(values[6]));
    OCIO_CHECK_EQUAL(values[7], 5.0f);

    // It is valid to start with delimiters.
    const char str2[] = ",  ,, , 0 2.0 \n \t 3.0 0.1e+1";
    const size_t len2 = strlen(str2);

    OCIO_CHECK_NO_THROW(values = OCIO::GetNumbers<float>(str2, len2));
    OCIO_REQUIRE_EQUAL(values.size(), 4);
    OCIO_CHECK_EQUAL(values[0], 0.0f);
    OCIO_CHECK_EQUAL(values[1], 2.0f);
    OCIO_CHECK_EQUAL(values[2], 3.0f);
    OCIO_CHECK_EQUAL(values[3], 1.0f);

    // Error: text is not a number.
    const char str3[] = "  0   error 2.0 3.0";
    const size_t len3 = strlen(str3);

    OCIO_CHECK_THROW_WHAT(values = OCIO::GetNumbers<float>(str3, len3),
                          OCIO::Exception,
                          "can not be parsed");

    // Error: number is not separated from text.
    const char str4[] = "0   1.0error 2.0 3.0";
    const size_t len4 = strlen(str4);

    OCIO_CHECK_THROW_WHAT(values = OCIO::GetNumbers<float>(str4, len4),
                          OCIO::Exception,
                          "followed by unexpected characters");
}

OCIO_ADD_TEST(XMLReaderHelper, trim)
{
    const std::string original1("    some text    ");
    const std::string original2(" \n \r some text  \t \v \f ");
    {
        std::string value(original1);
        OCIO::Trim(value);
        OCIO_CHECK_ASSERT(0 == strcmp("some text", value.c_str()));
        value = original2;
        OCIO::Trim(value);
        OCIO_CHECK_ASSERT(0 == strcmp("some text", value.c_str()));
    }

    {
        std::string value(original1);
        OCIO::RTrim(value);
        OCIO_CHECK_ASSERT(0 == strcmp("    some text", value.c_str()));
        value = original2;
        OCIO::RTrim(value);
        OCIO_CHECK_ASSERT(0 == strcmp(" \n \r some text", value.c_str()));
    }

    {
        std::string value(original1);
        OCIO::LTrim(value);
        OCIO_CHECK_ASSERT(0 == strcmp("some text    ", value.c_str()));
        value = original2;
        OCIO::LTrim(value);
        OCIO_CHECK_ASSERT(0 == strcmp("some text  \t \v \f ", value.c_str()));
    }
}

OCIO_ADD_TEST(XMLReaderHelper, parse_number)
{
    float data = 0.0f;
    {
        std::string buffer("1 0");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 1, data));
        OCIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer(" 1 0");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 2, data));
        OCIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0 0");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 3, data));
        OCIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0000 0");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 6, data));
        OCIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 3, data));
        OCIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 1, data);)
        OCIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("10.0e-1");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("0.1e+1");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("-1 0");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 2, data));
        OCIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0 0");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 4, data));
        OCIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0000 0");
        const size_t end = OCIO::FindDelim(buffer.c_str(), buffer.size(), 0);
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, end, data));
        OCIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("  -1.0");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("   -1");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data);)
            OCIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer(" -10.0e-1");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-0.1e+1");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("INF");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, 3, data));
        OCIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("INF 1.0 2.0");
        size_t pos = 0;
        const size_t size = buffer.size();
        size_t nextPos = OCIO::FindDelim(buffer.c_str(), size, pos);
        OCIO_CHECK_EQUAL(nextPos, 3);
        OCIO_CHECK_NO_THROW(
            OCIO::ParseNumber(buffer.c_str(),
                              pos, nextPos, data));
        OCIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
        pos = OCIO::FindNextTokenStart(buffer.c_str(), size, nextPos);
        OCIO_CHECK_EQUAL(pos, 4);
        nextPos = OCIO::FindDelim(buffer.c_str(), size, pos);
        OCIO_CHECK_EQUAL(nextPos, 7);
        OCIO_CHECK_NO_THROW(
            OCIO::ParseNumber(buffer.c_str(),
                              pos, nextPos, data));
        OCIO_CHECK_EQUAL(data, 1.0f);
        pos = OCIO::FindNextTokenStart(buffer.c_str(), size, nextPos);
        OCIO_CHECK_EQUAL(pos, 8);
        nextPos = OCIO::FindDelim(buffer.c_str(), size, pos);
        OCIO_CHECK_EQUAL(nextPos, 11);
        OCIO_CHECK_NO_THROW(
            OCIO::ParseNumber(buffer.c_str(),
                              pos, nextPos, data));
        OCIO_CHECK_EQUAL(data, 2.0f);
    }
    {
        std::string buffer("INFINITY");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("-INF");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, -std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("-INFINITY");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, -std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("NAN");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_ASSERT(OCIO::IsNan(data));
    }
    {
        std::string buffer("-NAN");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_ASSERT(OCIO::IsNan(data));
    }
    {
        std::string buffer("0.001");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-0.001");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer(".001");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-.001");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer(".01e-1");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-.01e-1");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1,");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size() - 1, data));
        OCIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1\n");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size() - 1, data));
        OCIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1\t");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size() - 1, data));
        OCIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("10E-1");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("0.10E01");
        OCIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              0, buffer.size(), data));
        OCIO_CHECK_EQUAL(data, 1.0f);
    }

    {
        std::string buffer(" 123 ");
        OCIO_CHECK_THROW_WHAT(OCIO::ParseNumber(buffer.c_str(),
                                                0, 3, data),
                              OCIO::Exception,
                              "followed by unexpected characters");
    }
    {
        std::string buffer("XY");
        OCIO_CHECK_THROW_WHAT(OCIO::ParseNumber(buffer.c_str(),
                                                0, 2, data),
                              OCIO::Exception,
                              "can not be parsed");
    }
}

OCIO_ADD_TEST(XMLReaderHelper, find_sub_string)
{
    {
        //                        012345678901234
        const std::string buffer("   new order   ");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OCIO_CHECK_EQUAL(start, 3);
        OCIO_CHECK_EQUAL(end, 12);
    }
    {
        //                        012345678901234
        const std::string buffer("new order   ");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OCIO_CHECK_EQUAL(start, 0);
        OCIO_CHECK_EQUAL(end, 9);
    }
    {
        //                        012345678901234
        const std::string buffer("   new order");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OCIO_CHECK_EQUAL(start, 3);
        OCIO_CHECK_EQUAL(end, 12);
    }
    {
        //                        012345678901234
        const std::string buffer("new order");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OCIO_CHECK_EQUAL(start, 0);
        OCIO_CHECK_EQUAL(end, 9);
    }
    {
        const std::string buffer("");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OCIO_CHECK_EQUAL(start, 0);
        OCIO_CHECK_EQUAL(end, 0);
    }
    {
        const std::string buffer("      ");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OCIO_CHECK_EQUAL(start, 0);
        OCIO_CHECK_EQUAL(end, 0);
    }
    {
        const std::string buffer("   \t123    ");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OCIO_CHECK_EQUAL(start, 4);
        OCIO_CHECK_EQUAL(end, 7);
    }
    {
        const std::string buffer("1   \t \n \r");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OCIO_CHECK_EQUAL(start, 0);
        OCIO_CHECK_EQUAL(end, 1);
    }
    {
        const std::string buffer("\t");
        size_t start = 0, end = 0;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OCIO_CHECK_EQUAL(start, 0);
        OCIO_CHECK_EQUAL(end, 0);
    }
}
