/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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


#ifndef INCLUDED_OCIO_CTFREADERUTILS_H
#define INCLUDED_OCIO_CTFREADERUTILS_H

#include <OpenColorIO/OpenColorIO.h>
#include "../Platform.h"
#include <string>
#include <vector>
#include <sstream>

OCIO_NAMESPACE_ENTER
{

// Private namespace to the CTF sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{
// Is c a 'space' character
// Note: Do not use the std::isspace which is very slow.
inline bool IsSpace(char c)
{
    // Note \n is unix while \r\n is windows line feed
    return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

// Find the position of the first the non-whitespace character.
// Whitespaces are defined as spaces, tabs or newlines here.
// - str string to check.
// - len is the length of the string.
// returns the position of the first non-whitespace character or
//         std::string::npos if the string only has whitespaces or is empty.
inline size_t FindFirstNonWhiteSpace(const char* str, size_t len)
{
    const char *ptr = str;
    size_t pos = 0;

    for (;;)
    {
        if (!Reader::IsSpace(*ptr))
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
// - str string to check.
// - len is the length of the string.
// returns the position of the last non-whitespace character or
//         std::string::npos if the string only has whitespaces or is empty.
inline size_t FindLastNonWhiteSpace(const char* str, size_t len)
{
    size_t pos = len - 1;
    const char *ptr = str + pos;

    for (;;)
    {
        if (!Reader::IsSpace(*ptr))
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
// end (just after the last non space character)
inline void FindSubString(const char* str, size_t length,
                            size_t& start,
                            size_t& end)
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
    // At worst, it will equal start
    end = FindLastNonWhiteSpace(str, length);

    // end-start should give the number of valid characters
    if (!Reader::IsSpace(str[end])) ++end;
}

// This method truncates a string (mainly used for display purpose)
// - pStr string to search
// - len is the length of the string
// returns the truncated string
inline std::string TruncateString(const char* pStr,// String to truncate
                                    size_t len       // String length
)
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

// Is the character a valid number delimiter?
// - c is the character to test
// returns true if the character is a valid delimiter
inline bool IsNumberDelimiter(char c)
{
    return Reader::IsSpace(c) || c == ',';
}

inline size_t FindDelim(const char* str, size_t len, size_t pos)
{
    const char *ptr = str + pos;

    while (!Reader::IsNumberDelimiter(*ptr))
    {
        if ((pos + 1) >= len)
        {
            return len;
        }
        ptr++; pos++;
    }

    return pos;
}

// Find the position of the next character to start scanning at
// Delimiters checked are spaces, commas, tabs and newlines.
// - str string to search
// - len is the length of the string
// - start position in string to start the search at
// returns the position of the next delimiter, or len if there are none.
inline size_t FindNextTokenStart(const char* s, size_t len, size_t pos)
{
    const char *ptr = s + pos;

    if (pos == len)
    {
        return pos;
    }

    while (CTF::Reader::IsNumberDelimiter(*ptr))
    {
        ptr++; pos++;

        if (pos >= len)
        {
            return len;
        }
    }

    return pos;
}

// Get first number from a string.
// str should can contain spaces before number.
template<typename T>
void ParseNumber(const char *str, size_t len, T& value)
{
    const size_t end = FindDelim(str, len, 0);
    if (end == 0)
    {
        throw Exception("ParseNumber: string should not start with space.");
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
        oss << str;
        oss << "' are illegal.";
        throw Exception(oss.str().c_str());
    }
    else if (matches == -1)
    {
        throw Exception("ParseNumber: error while scanning.");
    }
}

// Get first number from a string (specialized for floats)
// str should start with number.
template<>
inline void ParseNumber(const char *str, // The string to parse
                        size_t len,      // The length of the string
                        float& value     // Value extracted
)
{
    const size_t end = FindDelim(str, len, 0);
    if (end == 0)
    {
        throw Exception("ParseNumber: string should not start with space.");
    }

    // Perhaps the float is not the only one
    len = end;

    //
    // First check whether the string is a float value.  If there is no
    // match, only then do the infinity string comparison.
    //

    // Note: Always avoid to copy the string
    //
    // Note: As str is a string (and not a raw buffer), the physical length
    //       is always at least len+1. So the code could manipulate str[len] content.
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
inline void ParseNumber(const char *str, // The string to parse
                        size_t len,      // The length of the string
                        double& value    // Value extracted
)
{
    const size_t end = FindDelim(str, len, 0);
    if (end == 0)
    {
        throw Exception("ParseNumber: string should not start with space.");
    }

    // Perhaps the double is not the only one
    len = end;

    //
    // First check whether the string is a double value.  If there is no
    // match, only then do the infinity string comparison.
    //

    // Note: Always avoid to copy the string
    //
    // Note: As str is a string (and not a raw buffer), the physical length
    //       is always at least len+1. So the code could manipulate str[len] content.
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
// - str the string th search
// - prev position to start the search at.
// returns the next in the string.  Note that prev gets updated to the
//         position of the next delimiter, or to std::string::npos
//         if the value returned is the last one in the string.
template<typename T>
void GetNextNumber(const char *s, size_t len, size_t& pos, T& num)
{
    pos = FindNextTokenStart(s, len, pos);

    if (pos != len)
    {
        CTF::Reader::ParseNumber(s + pos, len - pos, num);

        pos = CTF::Reader::FindDelim(s, len, pos);
        if (pos != len)
        {
            pos = FindNextTokenStart(s, len, pos);
        }
    }
}

// This method tokenizes a string like "0 1 2" of integers or floats
// returns the numbers extracted from the string
template<typename T>
std::vector<T> GetNumbers(
    const char* str, // The string to parse
    size_t len       // The length of the string
)
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


// Trim from both ends
void Trim(std::string &s);

class Element;

// Stack of elements
class ElementStack
{
    // Definition of a stack
    typedef std::vector<Element*> Stack;

public:
    // Constructor
    ElementStack();
        
    // Destructor
    ~ElementStack();

    // Dump the stack content with a simplified format
    std::string dump() const;

    // Is empty ?
    bool empty() const;

    // Get the current size
    unsigned size() const;

    // Push back an Element
    void push_back(Element* pElt);

    // Pop back the last element
    void pop_back();

    // Get the last element
    Element* back() const;

    // Get the first element
    Element* front() const;

    // Clear the stack
    void clear();

private:
    Stack m_elms; // The list of elements
};

} // exit Reader namespace

// Process List tag name
extern const char TAG_PROCESS_LIST[];

// Info tag name
extern const char TAG_INFO[];

// Description tag name
extern const char TAG_DESCRIPTION[];

// Input Descriptor tag name
extern const char TAG_INPUT_DESCRIPTOR[];

// Output Descriptor tag name
extern const char TAG_OUTPUT_DESCRIPTOR[];

// Matrix tag name
extern const char TAG_MATRIX[];

// Array tag name
extern const char TAG_ARRAY[];

// LUT1D tag name
extern const char TAG_LUT1D[];

// Inv LUT1D tag name
extern const char TAG_INVLUT1D[];

// Index map tag name
extern const char TAG_INDEX_MAP[];

// Range tag name
extern const char TAG_RANGE[];

// Range value tag name
extern const char TAG_MIN_IN_VALUE[];

// Range value tag name
extern const char TAG_MAX_IN_VALUE[];

// Range value tag name
extern const char TAG_MIN_OUT_VALUE[];

// Range value tag name
extern const char TAG_MAX_OUT_VALUE[];

// CDL tag name
extern const char TAG_CDL[];

// SOPTNode tag name
extern const char TAG_SOPNODE[];

// Slope tag name
extern const char TAG_SLOPE[];

// Offset tag name
extern const char TAG_OFFSET[];

// Power tag name
extern const char TAG_POWER[];

// SatNode tag name
extern const char TAG_SATNODE[];

// Saturation tag name
extern const char TAG_SATURATION[];

// Lut3D tag name
extern const char TAG_LUT3D[];

// Inverse Lut3D tag name
extern const char TAG_INVLUT3D[];

// id attribute
extern const char ATTR_ID[];

// name attribute
extern const char ATTR_NAME[];

// inverseOf attribute
extern const char ATTR_INVERSE_OF[];

// Version attribute.
extern const char ATTR_VERSION[];

// Version attribute.
extern const char ATTR_COMP_CLF_VERSION[];

// inBitDepth attribute
extern const char ATTR_IN_BIT_DEPTH[];

// outBitDepth attribute
extern const char ATTR_OUT_BIT_DEPTH[];

// Array dimension attribute
extern const char ATTR_DIMENSION[];

// LUT interpolation attribute
extern const char ATTR_INTERPOLATION[];

// half domain attribute
extern const char ATTR_HALF_DOMAIN[];

// raw halfs attribute
extern const char ATTR_RAW_HALFS[];

// hue adjust attribute
extern const char ATTR_HUE_ADJUST[];

// Range style
extern const char ATTR_RANGE_STYLE[];

// CDL style
extern const char ATTR_CDL_STYLE[];


} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#endif
