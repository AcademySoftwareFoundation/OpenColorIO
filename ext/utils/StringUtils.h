// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_STRINGUTILS_H
#define INCLUDED_STRINGUTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace StringUtils
{

inline std::string Lower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::tolower(c); }
                  );
    return str;
}

inline std::string Upper(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::toupper(c); }
                  );
    return str;
}

inline std::string LeftTrim(std::string str)
{
    const auto it = std::find_if(str.begin(), str.end(),
                                 [](char ch){ return !std::isspace(ch); }
                                );
    str.erase(str.begin(), it);
    return str;
}

inline std::string RightTrim(std::string str)
{
    const auto it = std::find_if(str.rbegin(), str.rend(),
                                 [](char ch){ return !std::isspace(ch); }
                                );
    str.erase(it.base(), str.end());
    return str;
}

inline std::string Trim(std::string str)
{
   return LeftTrim(RightTrim(str));
}

inline void Split(const std::string & str, std::vector<std::string> & results, char separator)
{
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, separator))
    {
        results.push_back(std::move(item));
    }
}

inline void SplitByLines(const std::string & str, std::vector<std::string> & results)
{
    Split(str, results, '\n');
}

inline bool EndsWith(const std::string & str, const std::string & suffix)
{
    return str.size() >= suffix.size()
        && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}

inline bool StartsWith(const std::string & str, const std::string & prefix)
{
    return str.size() >= prefix.size()
        && 0 == str.compare(0, prefix.size(), prefix);
}


} // namespace StringUtils


#endif  // INCLUDED_STRINGUTILS_H
