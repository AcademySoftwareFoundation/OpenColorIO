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

#ifndef INCLUDED_OCIO_FILEFORMATS_XML_XMLREADERUTILS_H
#define INCLUDED_OCIO_FILEFORMATS_XML_XMLREADERUTILS_H

#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Platform.h"

OCIO_NAMESPACE_ENTER
{

// Used by both CDL parsers and CLF parsers.

static constexpr const char * ATTR_ID = "id";
static constexpr const char * TAG_DESCRIPTION = "Description";
static constexpr const char * TAG_OFFSET = "Offset";
static constexpr const char * TAG_POWER = "Power";
static constexpr const char * TAG_SATNODE = "SatNode";
static constexpr const char * TAG_SATNODEALT = "SATNode";
static constexpr const char * TAG_SATURATION = "Saturation";
static constexpr const char * TAG_SLOPE = "Slope";
static constexpr const char * TAG_SOPNODE = "SOPNode";

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
inline size_t FindNextTokenStart(const char * s, size_t len, size_t pos)
{
    const char * ptr = s + pos;

    if (pos == len)
    {
        return pos;
    }

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
    const char * ptr = str + pos;

    while (!IsNumberDelimiter(*ptr))
    {
        if ((pos + 1) >= len)
        {
            return len;
        }
        ptr++; pos++;
    }

    return pos;
}

// Get first number from a string.
// str should start with number.
template<typename T>
void ParseNumber(const char * str, size_t len, T & value)
{
    const size_t end = FindDelim(str, len, 0);
    if (end == 0)
    {
        throw Exception("ParseNumber: string should not start with a delimiter.");
    }

    double val = 0.0f;
    const int matches = sscanf(str, "%lf", &val);

    value = (T)val;

    //
    // Make sure we got a match, and that there was not decimal truncation
    // due to a double value being read when an integer was expected.
    //
    if (matches == 0 || (double)value != val)
    {
        std::ostringstream oss;
        oss << "ParseNumber: Characters '";
        oss << std::string(str, len);
        oss << "' are illegal.";
        throw Exception(oss.str().c_str());
    }
    else if (matches == -1)
    {
        throw Exception("ParseNumber: error while scanning.");
    }
}

// Get first number from a string (specialized for floats).
// str should start with number.
template<>
inline void ParseNumber(const char * str, size_t len, float & value)
{
    const size_t end = FindDelim(str, len, 0);
    if (end == 0)
    {
        throw Exception("ParseNumber: string should not start with a delimiter.");
    }

    // Perhaps the float is not the only one.
    len = end;

    //
    // First check whether the string is a float value.  If there is no
    // match, only then do the infinity string comparison.
    //

    // Note: Always avoid to copy the string.
    //
    // Note: As str is a string (and not a raw buffer), the physical length
    // is always at least len+1. So the code could manipulate str[len] content.
    //
    char* p = const_cast<char*>(str);
    char t = '\0';
    const bool resizeString = len != strlen(str);
    if (resizeString)
    {
        t = str[len];
        p[len] = '\0';
    }
    const int matches = sscanf(p, "%f", &value);
    if (resizeString)
    {
        p[len] = t;
    }

    if (matches == 0)
    {
        //
        // Did not get a float value match.  See if infinity is present.
        // Only C99 nan and infinity representations are recognized.
        //

        if (((Platform::Strncasecmp(str, "INF", 3) == 0) && (end == 3)) ||
            ((Platform::Strncasecmp(str, "INFINITY", 8) == 0) && (end == 8)))
        {
            value = std::numeric_limits<float>::infinity();
        }
        else if
            (((Platform::Strncasecmp(str, "-INF", 4) == 0) && (end == 4)) ||
            ((Platform::Strncasecmp(str, "-INFINITY", 9) == 0) && (end == 9)))
        {
            value = -std::numeric_limits<float>::infinity();
        }
        else if (Platform::Strncasecmp(str, "NAN", 3) == 0 ||
                 Platform::Strncasecmp(str, "-NAN", 4) == 0)
        {
            value = std::numeric_limits<float>::quiet_NaN();
        }
        else
        {
            // No inifity match, bad value in file.
            std::ostringstream ss;
            ss << "ParserNumber: Characters '" << str << "' are illegal.";
            throw Exception(ss.str().c_str());
        }
    }
    else if (matches == -1)
    {
        throw Exception("ParseNumber: error while scanning.");
    }
}

// Get first number from a string (specialized for double)
// str should start with number.
template<>
inline void ParseNumber(const char * str, size_t len, double & value)
{
    const size_t end = FindDelim(str, len, 0);
    if (end == 0)
    {
        throw Exception("ParseNumber: string should not start with a delimiter.");
    }

    // Perhaps the double is not the only one.
    len = end;

    //
    // First check whether the string is a double value.  If there is no
    // match, only then do the infinity string comparison.
    //

    // Note: Always avoid to copy the string.
    //
    // Note: As str is a string (and not a raw buffer), the physical length
    // is always at least len+1. So the code could manipulate str[len] content.
    //
    char* p = const_cast<char*>(str);
    char t = '\0';
    const bool resizeString = len != strlen(str);
    if (resizeString)
    {
        t = str[len];
        p[len] = '\0';
    }
    const int matches = sscanf(p, "%lf", &value);
    if (resizeString)
    {
        p[len] = t;
    }

    if (matches == 0)
    {
        //
        // Did not get a double value match.  See if infinity is present.
        // Only C99 nan and infinity representations are recognized.
        //

        if (((Platform::Strncasecmp(str, "INF", 3) == 0) && (end == 3)) ||
            ((Platform::Strncasecmp(str, "INFINITY", 8) == 0) && (end == 8)))
        {
            value = std::numeric_limits<double>::infinity();
        }
        else if
            (((Platform::Strncasecmp(str, "-INF", 4) == 0) && (end == 4)) ||
            ((Platform::Strncasecmp(str, "-INFINITY", 9) == 0) && (end == 9)))
        {
            value = -std::numeric_limits<double>::infinity();
        }
        else if (Platform::Strncasecmp(str, "NAN", 3) == 0 ||
                 Platform::Strncasecmp(str, "-NAN", 4) == 0)
        {
            value = std::numeric_limits<double>::quiet_NaN();
        }
        else
        {
            // No inifity match, bad value in file.
            std::ostringstream ss;
            ss << "ParserNumber: Characters '" << str << "' are illegal.";
            throw Exception(ss.str().c_str());
        }
    }
    else if (matches == -1)
    {
        throw Exception("ParseNumber: error while scanning.");
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
        ParseNumber(s + pos, len - pos, num);

        pos = FindDelim(s, len, pos);
        if (pos != len)
        {
            pos = FindNextTokenStart(s, len, pos);
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

// This method truncates a string (mainly used for display purpose).
inline std::string TruncateString(const char * pStr, size_t len)
{
    static const unsigned MAX_SIZE = 17;

    std::string s(pStr, len);

    if (s.size()>MAX_SIZE)
    {
        s.resize(MAX_SIZE);
        s += "...";
    }

    return s;
}

}
OCIO_NAMESPACE_EXIT

#endif