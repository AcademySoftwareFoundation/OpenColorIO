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


using StringVec = std::vector<std::string>;

// Return the lower case character without taking into account locale like
// std::tolower, to avoid the "Turkish I" problem in file parsing.
inline unsigned char Lower(unsigned char c)
{
    if(c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    else {
        return c;
    }
}

// Return the upper case character, without taking into account locale.
inline unsigned char Upper(unsigned char c)
{
    if(c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }
    else {
        return c;
    }
}

// Return the lower case string.
inline std::string Lower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return Lower(c); });
    return str;
}

inline std::string Lower(const char * str)
{
    if (!str) return "";
    const std::string s{ str };
    return Lower(s);
}

// Return the upper case string.
inline std::string Upper(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return Upper(c); });
    return str;
}

inline std::string Upper(const char * str)
{
    if (!str) return "";
    const std::string s{ str };
    return Upper(s);
}

// Case insensitive comparison of strings.
inline bool Compare(const std::string & left, const std::string & right)
{
    return Lower(left) == Lower(right);
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
    const auto it = std::find_if(str.rbegin(), str.rend(), [&c](char ch) { return c!=ch; });
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

inline void Trim(StringVec & list)
{
    for (auto & entry : list)
    {
        entry = Trim(entry);
    }
}

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

// Join a list of strings using an arbitrary separator.
inline std::string Join(const StringVec & strings, char separator)
{
    if (strings.empty()) return "";

    const StringVec::size_type len = strings.size();

    if (len==1) return strings[0];

    const std::string sep(1, separator);

    std::string result{strings[0]};
    for (StringVec::size_type i=1; i<len; ++i)
    {
        result += sep + " " + strings[i];
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
inline bool ReplaceInPlace(std::string & subject, const std::string & search, const std::string & replace)
{
    bool changed = false;

    size_t pos =  0;
    while ((pos = subject.find(search, pos)) != std::string::npos)
    {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
        changed = true;
    }

    return changed;
}

// Replace the 'search' substring by the 'replace' string in 'str'.
inline std::string Replace(const std::string & subject, const std::string & search, const std::string & replace)
{
    std::string str{subject};
    ReplaceInPlace(str, search, replace);
    return str;
}

// Check if the 'entry' is in the 'list' using a case insensitive comparison.
inline bool Contain(const StringVec & list, const std::string & entry)
{
    const auto it = std::find_if(list.begin(), list.end(), 
                                 [entry](const std::string & ent) 
                                 { 
                                    return Compare(ent.c_str(), entry.c_str()); 
                                 });
    return it!=list.end();
}

// Remove the 'entry' from the 'list' using a case insensitive comparison.
// It returns true if found.
inline bool Remove(StringVec & list, const std::string & entry)
{
    const auto it = std::find_if(list.begin(), list.end(), 
                                 [entry](const std::string & ent) 
                                 { 
                                    return Compare(ent.c_str(), entry.c_str()); 
                                 });
    if (it!=list.end())
    {
        list.erase(it);
        return true;
    }

    return false;
}

} // namespace StringUtils

#endif // INCLUDED_STRINGUTILS_H
