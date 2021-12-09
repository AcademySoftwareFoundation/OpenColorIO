// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FILEFORMATS_XML_XMLREADERUTILS_H
#define INCLUDED_OCIO_FILEFORMATS_XML_XMLREADERUTILS_H


#include <type_traits>
#include <string>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "utils/StringUtils.h"
#include "utils/NumberUtils.h"
#include "Platform.h"


namespace OCIO_NAMESPACE
{

// Strings used by CDL and CLF parsers or writers.

static constexpr char ATTR_ID[] = "id";
static constexpr char ATTR_NAME[] = "name";

static constexpr char CDL_TAG_COLOR_CORRECTION[] = "ColorCorrection";

static constexpr char TAG_DESCRIPTION[] = "Description";
static constexpr char TAG_OFFSET[] = "Offset";
static constexpr char TAG_POWER[] = "Power";
static constexpr char TAG_SATNODE[] = "SatNode";
static constexpr char TAG_SATNODEALT[] = "SATNode";
static constexpr char TAG_SATURATION[] = "Saturation";
static constexpr char TAG_SLOPE[] = "Slope";
static constexpr char TAG_SOPNODE[] = "SOPNode";


// This method truncates a string (mainly used for display purpose).
inline std::string TruncateString(const char * pStr, size_t len, size_t limit)
{
    const size_t new_len = (limit < len) ? limit : len;
    return std::string(pStr, new_len);
}

// This method truncates a string (mainly used for display purpose) to a default limit.
inline std::string TruncateString(const char * pStr, size_t len)
{
    static constexpr size_t MAX_SIZE = 17;
    return TruncateString(pStr, len, MAX_SIZE);
}

void Trim(std::string & s);

// Find the first valid sub string delimited by spaces.
// Avoid any character copy(ies) as the method is intensively used
// when reading values of 1D & 3D luts
void FindSubString(const char * str, size_t length,
                   size_t & start,
                   size_t & end);

// Is c a 'space' character ( '\n', '\t', ' ' ... )?
// Note: Do not use the std::isspace which is very slow.
inline bool IsSpace(char c)
{
    // Note: \n is unix while \r\n is windows line feed.
    return c == ' ' || c == '\n' || c == '\t' || c == '\r' ||
           c == '\v' || c == '\f';
}

// Is the character a valid number delimiter?
inline bool IsNumberDelimiter(char c)
{
    return IsSpace(c) || c == ',';
}

// Find the position of the next character to start scanning at.
// Delimiters checked are spaces, commas, tabs and newlines.
inline size_t FindNextTokenStart(const char * str, size_t len, size_t pos)
{
    if (pos >= len)
    {
        return len;
    }

    const char * ptr = str + pos;

    while (IsNumberDelimiter(*ptr))
    {
        ptr++; pos++;

        if (pos >= len)
        {
            return len;
        }
    }

    return pos;
}

// Find the position of the next delimiter in the string.
// Delimiters checked are spaces and commas.
inline size_t FindDelim(const char * str, size_t len, size_t pos)
{
    if (pos >= len)
    {
        return len;
    }

    const char * ptr = str + pos;

    while (!IsNumberDelimiter(*ptr))
    {
        ptr++; pos++;

        if (pos >= len)
        {
            return len;
        }
    }

    return pos;
}

namespace
{

// When using an integer ParseNumber template, it is an error if the string
// actually contains a number with a decimal part.
template<typename T>
bool IsValid(T value, double val)
{
    // Returns true, if T is the type float, double or long double.
    if (std::is_floating_point<T>::value) return true;

    return static_cast<double>(value) == val;
}

}

// Get first number from a string between startPos & endPos.
// EndPos should not be greater than length of the string.
// Will throw if str[endPos-1] is not part of the number.
// The character after the last one that is part of the number has to be
// accessible (most likely str[endPos]).
// Note: For performance reasons, this function does not copy the string
//       unless an exception needs to be thrown.
template<typename T>
void ParseNumber(const char * str, size_t startPos, size_t endPos, T & value)
{
    if (endPos == startPos)
    {
        throw Exception("ParseNumber: nothing to parse.");
    }

    const char * startParse = str + startPos;

    double val = 0.0f;

    size_t adjustedStartPos = startPos;
    size_t adjustedEndPos = endPos;

    FindSubString(startParse, endPos - startPos, adjustedStartPos, adjustedEndPos);

    const auto result = NumberUtils::from_chars(startParse + adjustedStartPos, startParse + adjustedEndPos, val);

    value = (T)val;

    if (result.ec == std::errc::invalid_argument)
    {
        std::string fullStr(str, endPos);
        std::string parsedStr(startParse, endPos - startPos);
        std::ostringstream oss;
        oss << "ParserNumber: Characters '"
            << parsedStr
            << "' can not be parsed to numbers in '"
            << TruncateString(fullStr.c_str(), endPos, 100) << "'.";
        throw Exception(oss.str().c_str());
    }
    else if (!IsValid(value, val))
    {
        std::string fullStr(str, endPos);
        std::string parsedStr(startParse, endPos - startPos);
        std::ostringstream oss;
        oss << "ParserNumber: Characters '"
            << parsedStr
            << "' are illegal in '"
            << TruncateString(fullStr.c_str(), endPos, 100) << "'.";
        throw Exception(oss.str().c_str());
    }
    else if (result.ptr != str + endPos)
    {
        // Number is followed by something.
        std::string fullStr(str, endPos);
        std::string parsedStr(startParse, endPos - startPos);
        std::ostringstream oss;
        oss << "ParserNumber: '"
            << parsedStr
            << "' number is followed by unexpected characters in '"
            << TruncateString(fullStr.c_str(), endPos, 100) << "'.";
        throw Exception(oss.str().c_str());
    }
}

// Extract the next number contained in the string.
// Note that pos gets updated to the position of the next delimiter, or to
// std::string::npos if the value returned is the last one in the string.
template<typename T>
void GetNextNumber(const char * s, size_t len, size_t & pos, T & num)
{
    pos = FindNextTokenStart(s, len, pos);

    if (pos != len)
    {
        size_t nextPos = FindDelim(s, len, pos);
        ParseNumber(s, pos, nextPos, num);
        pos = nextPos;

        if (pos != len)
        {
            pos = FindNextTokenStart(s, len, nextPos);
        }
    }
}

// This method tokenizes a string like "0 1 2" of integers or floats.
// returns the numbers extracted from the string.
template<typename T>
std::vector<T> GetNumbers(const char * str, size_t len)
{
    std::vector<T> numbers;

    size_t pos = FindNextTokenStart(str, len, 0);
    while (pos != len)
    {
        T num(0);
        GetNextNumber(str, len, pos, num);
        numbers.push_back(num);
    }

    return numbers;
}

} // namespace OCIO_NAMESPACE

#endif
