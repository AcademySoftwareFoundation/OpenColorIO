// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>

#include "fileformats/xmlutils/XMLReaderUtils.h"

namespace OCIO_NAMESPACE
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

} // namespace OCIO_NAMESPACE

