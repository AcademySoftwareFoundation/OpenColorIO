// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_STRINGUTILS_H
#define INCLUDED_STRINGUTILS_H

#include <algorithm>
#include <cctype>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>


// Note: Most of the implementations heavily rely on the C++ std::move() in order
//       to simplify the writing without performance penalty.

namespace StringUtils
{

// Return the lower case string.
inline std::string Lower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return str;
}

// Return the upper case string.
inline std::string Upper(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return str;
}

// Return true if the string ends with the suffix.
// Note: The comparison is case sensitive.
inline bool EndsWith(const std::string & str, const std::string & suffix)
{
    return str.size() >= suffix.size() &&
           0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

// Return true if the string starts with the prefix.
// Note: The comparison is case sensitive.
inline bool StartsWith(const std::string & str, const std::string & prefix)
{
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

// Starting from the left, trim the character.
inline std::string LeftTrim(std::string str, char c)
{
    const auto it = std::find_if(str.begin(), str.end(), [&c](char ch) { return c!=ch; });
    str.erase(str.begin(), it);
    return str;
}

// Starting from the left, trim all the space characters i.e. space, tabulation, etc.
inline std::string LeftTrim(std::string str)
{
    const auto it = std::find_if(str.begin(), str.end(), [](char ch) { return !std::isspace(ch); });
    str.erase(str.begin(), it);
    return str;
}

// Starting from the right, trim the character.
inline std::string RightTrim(std::string str, char c)
{
    const auto it =
        std::find_if(str.rbegin(), str.rend(), [&c](char ch) { return c!=ch; });
    str.erase(it.base(), str.end());
    return str;
}

// Starting from the right, trim all the space characters i.e. space, tabulation, etc.
inline std::string RightTrim(std::string str)
{
    const auto it =
        std::find_if(str.rbegin(), str.rend(), [](char ch) { return !std::isspace(ch); });
    str.erase(it.base(), str.end());
    return str;
}

// From the left and right, trim the character.
inline std::string Trim(std::string str, char c)
{
    return LeftTrim(RightTrim(str, c), c);
}

// From the left and right, trim all the space characters i.e. space, tabulation, etc.
inline std::string Trim(std::string str)
{
    return LeftTrim(RightTrim(str));
}


using StringVec = std::vector<std::string>;


// Split a string content using an arbitrary separator.
inline StringVec Split(const std::string & str, char separator)
{
    if (str.empty())  return {""};

    StringVec results;

    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, separator))
    {
        results.push_back(std::move(item));
    }

    if (EndsWith(str, std::string(1, separator)))
    {
        results.push_back("");
    }

    return results;
}

inline std::string Join(const std::string & separator, const StringVec & strings)
{
    if (strings.empty()) return "";

    const StringVec::size_type len = strings.size();

    if (len==1) return strings[0];

    std::string result{strings[0]};

    for (StringVec::size_type i=1; i<len; ++i)
    {
        result += separator + strings[i];
    }

    return result;
}

// Split a string content by line feeds.
inline StringVec SplitByLines(const std::string & str)
{
    if (str.empty())  return {""};

    StringVec results;

    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item))
    {
        results.push_back(std::move(item));
    }

    return results;
}

// Split a string content by whitespaces i.e. ' ', tab.
inline StringVec SplitByWhiteSpaces(const std::string & str)
{
    std::stringstream stream(str);
    return std::vector<std::string>(std::istream_iterator<std::string>(stream),
                                    std::istream_iterator<std::string>());
}

// Find the position of 'search' substring.
// It returns std::string::npos if not found.
inline std::string::size_type Find(const std::string & subject, const std::string & search)
{
    return subject.find(search);
}

// Find the position of 'search' substring starting from the end.
// It returns std::string::npos if not found.
inline std::string::size_type ReverseFind(const std::string & subject, const std::string & search)
{
    return subject.rfind(search);
}

// In place replace the 'search' substring by the 'replace' string in 'str'.
inline void ReplaceInPlace(std::string & subject, const std::string & search, const std::string & replace)
{
    size_t pos =  0;
    while ((pos = subject.find(search, pos)) != std::string::npos)
    {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

// Replace the 'search' substring by the 'replace' string in 'str'.
inline std::string Replace(const std::string & subject, const std::string & search, const std::string & replace)
{
    std::string str{subject};
    ReplaceInPlace(str, search, replace);
    return str;
}


} // namespace StringUtils

#endif // INCLUDED_STRINGUTILS_H
