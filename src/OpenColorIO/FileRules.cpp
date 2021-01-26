// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cctype>
#include <map>
#include <regex>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "CustomKeys.h"
#include "FileRules.h"
#include "PathUtils.h"
#include "Platform.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

const char * FileRules::DefaultRuleName        = "Default";
const char * FileRules::FilePathSearchRuleName = "ColorSpaceNamePathSearch";

namespace
{

std::string SanitizeRegularExpression(const std::string & regexPattern)
{
    std::string r = regexPattern;

    // regex 1
    // "*?" => "*"
    // "?*" => "*"

    r = std::regex_replace(r, std::regex("(\\.\\*\\.^\\*)+|(^\\\\\\.\\.\\*)+"), ".*");

    // regex 2
    // "**" => "*"
    r = std::regex_replace(r, std::regex("(\\.\\*)+"), ".*");

    // ex:  "*?*"
    //      regex 1 part 1 ==> "*?*" = "***"
    //      regex 2        ==> "***" = "*"

    return r;
}

void ThrowInvalidRegex(const char * globPattern, const char * what)
{
    std::ostringstream oss;
    oss << "File rules: invalid regular expression '" << std::string(globPattern)
        << "' with '" << std::string(what) << "'.";
    throw Exception(oss.str().c_str());
}

std::string ConvertToRegularExpression(const char * globPattern, bool ignoreCase)
{
    std::string globString;

    if (ignoreCase)
    {
        const size_t length = strlen(globPattern);
        bool respectCase = false;
        for (size_t i = 0; i < length; ++i)
        {
            if (globPattern[i] == '[' || globPattern[i] == '*' ||
                globPattern[i] == '?')
            {
                respectCase = true;
                break;
            }
            if (isalpha(globPattern[i]))
            {
                std::string replaceBy{ "[" };
                replaceBy += tolower(globPattern[i]);
                replaceBy += toupper(globPattern[i]);
                replaceBy += "]";
                globString += replaceBy;
            }
            else
            {
                globString += globPattern[i];
            }
        }
        if (respectCase)
        {
            globString = globPattern;
        }
    }
    else
    {
        globString = globPattern;
    }

    std::string regexPattern;
    const size_t globSize = globString.size();
    size_t nextIdx = 0;
    for (size_t idx = 0; idx < globSize; idx = nextIdx)
    {
        nextIdx = idx + 1;
        if (globString[idx] == '.') { regexPattern += "\\."; continue; }
        if (globString[idx] == '?') { regexPattern += ".";   continue; }
        if (globString[idx] == '*') { regexPattern += ".*";  continue; }

        // Escape regex characters.
        if (globString[idx] == '+') { regexPattern += "\\+";  continue; }
        if (globString[idx] == '^') { regexPattern += "\\^";  continue; }
        if (globString[idx] == '$') { regexPattern += "\\$";  continue; }
        if (globString[idx] == '{') { regexPattern += "\\{";  continue; }
        if (globString[idx] == '}') { regexPattern += "\\}";  continue; }
        if (globString[idx] == '(') { regexPattern += "\\(";  continue; }
        if (globString[idx] == ')') { regexPattern += "\\)";  continue; }
        if (globString[idx] == '|') { regexPattern += "\\|";  continue; }

        if (globString[idx] == ']')
        {
            ThrowInvalidRegex(globPattern, globPattern + idx);
        }

        // Full processing from '[' to ']'.
        if (globString[idx] == '[')
        {
            std::string subString("[");
            size_t end = idx + 1; // +1 to bypass the '['
            while (globString[end] != ']' && end < globSize)
            {
                if (globString[end] == '!')
                {
                    subString += '^';
                }
                else if (globString[end] == '+'
                    || globString[end] == '^'
                    || globString[end] == '$'
                    || globString[end] == '{'
                    || globString[end] == '}'
                    || globString[end] == '('
                    || globString[end] == ')'
                    || globString[end] == '|')
                {
                    // Escape regex characters.
                    subString += "\\";
                    subString += globString[end];
                }
                else if (globString[end] == '\\')
                {
                    subString += "\\\\";
                }
                else if (globString[end] == '.'
                    || globString[end] == '?'
                    || globString[end] == '*')
                {
                    if (globString[end - 1] != '\\')
                    {
                        ThrowInvalidRegex(globPattern, &globString[idx]);
                    }
                    subString += globString[end];
                }
                else if (globString[end] == '[')
                {
                    ThrowInvalidRegex(globPattern, &globString[idx]);
                }
                else
                {
                    subString += globString[end];
                }
                ++end;
            }
            if (globString[end] == ']')
            {
                subString += globString[end];
            }

            // Some validations.

            if (end >= globSize)
            {
                ThrowInvalidRegex(globPattern, &globString[idx]);
            }
            else if (subString == "[]")
            {
                ThrowInvalidRegex(globPattern, "[]");
            }
            else if (subString == "[^]")
            {
                ThrowInvalidRegex(globPattern, "[!]");
            }

            // Keep the result.
            regexPattern += subString;
            nextIdx = end + 1;
            continue;
        }

        regexPattern += globString[idx];
    }

    return regexPattern;
}

std::string BuildRegularExpression(const char * filePathPattern, const char * fileNameExtension)
{
    std::string str;
    str += "^(";

    if (!filePathPattern)
    {
        throw Exception("File rules: file pattern is empty.");
    }
    else if (!*filePathPattern)
    {
        // An empty file path pattern is internally converted to "*" in order to simplify
        // the user writing of the glob pattern.
        str += "(.*)";
    }
    else
    {
        str += "(";
        str += ConvertToRegularExpression(filePathPattern, false);
        str += ")";
    }

    if (!fileNameExtension)
    {
        throw Exception("File rules: file extension is empty.");
    }
    else if (!*fileNameExtension)
    {
        // An empty file extension is internally converted to ".*" in order to simplify
        // the user writing of the glob pattern.
        str += "(\\..*)";
    }
    else
    {
        str += "(\\.";
        str += ConvertToRegularExpression(fileNameExtension, true);
        str += ")";
    }

    str += ")$";

    std::string res;
    try
    {
        res = SanitizeRegularExpression(str);
    }
    catch (std::regex_error & ex)
    {
        std::ostringstream oss;
        oss << "File rules: invalid regular expression '"
            << str
            << "' built from pattern '" << filePathPattern
            << " and extension '" << fileNameExtension << "': '"
            << ex.what()
            << "'.";
        throw Exception(oss.str().c_str());
    }
    return res;
}

void ValidateRegularExpression(const char * regex)
{
    if (!regex || !*regex)
    {
        throw Exception("File rules: regex is empty.");
    }

    try
    {
        // Throws an exception if the expression is ill-formed.
        const std::regex reg(regex);
    }
    catch (std::regex_error & ex)
    {
        std::ostringstream oss;
        oss << "File rules: invalid regular expression '" 
            << regex
            << "': '"
            << ex.what() 
            << "'.";
        throw Exception(oss.str().c_str());
    }
}

void ValidateRegularExpression(const char * filePathPattern, const char * fileNameExtension)
{
    const std::string exp = BuildRegularExpression(filePathPattern, fileNameExtension);
    ValidateRegularExpression(exp.c_str());
}

}

class FileRule
{
public:
    enum RuleType
    {
        FILE_RULE_DEFAULT = 0,
        FILE_RULE_PARSE_FILEPATH,
        FILE_RULE_REGEX,
        FILE_RULE_GLOB
    };

    FileRule() = delete;
    FileRule(const FileRule &) = delete;
    FileRule & operator=(const FileRule &) = delete;

    explicit FileRule(const char * name)
        : m_name(name ? name : "")
    {
        if (m_name.empty())
        {
            throw Exception("The file rule name is empty");
        }
        else if (0==Platform::Strcasecmp(name, FileRules::DefaultRuleName))
        {
            m_name = FileRules::DefaultRuleName; // Enforce case consistency.
            m_type = FILE_RULE_DEFAULT;
        }
        else if (0==Platform::Strcasecmp(name, FileRules::FilePathSearchRuleName))
        {
            m_name = FileRules::FilePathSearchRuleName; // Enforce case consistency.
            m_type = FILE_RULE_PARSE_FILEPATH;
        }
        else
        {
            m_pattern   = "*";
            m_extension = "*";
            m_type      = FILE_RULE_GLOB;
        }
    }

    FileRuleRcPtr clone() const
    {
        FileRuleRcPtr rule = std::make_shared<FileRule>(m_name.c_str());

        rule->m_customKeys = m_customKeys;
        rule->m_colorSpace = m_colorSpace;
        rule->m_pattern    = m_pattern;
        rule->m_extension  = m_extension;
        rule->m_regex      = m_regex;
        rule->m_type       = m_type;

        return rule;
    }

    const char * getName() const noexcept
    {
        return m_name.c_str();
    }

    const char * getPattern() const noexcept
    {
        if (m_type != FILE_RULE_GLOB)
        {
            return "";
        }
        return m_pattern.c_str();
    }

    void setPattern(const char * pattern)
    {
        if (m_type == FILE_RULE_DEFAULT || m_type == FILE_RULE_PARSE_FILEPATH)
        {
            if (pattern && *pattern)
            {
                throw Exception("File rules: Default and ColorSpaceNamePathSearch rules "
                                "do not accept any pattern.");
            }
        }
        else
        {
            if (!pattern || !*pattern)
            {
                throw Exception("File rules: The file name pattern is empty.");
            }
            ValidateRegularExpression(pattern, m_extension.c_str());
            m_pattern = pattern;
            m_regex = "";
            m_type = FILE_RULE_GLOB;
        }
    }

    const char * getExtension() const noexcept
    {
        if (m_type != FILE_RULE_GLOB)
        {
            return "";
        }
        return m_extension.c_str();
    }

    void setExtension(const char * extension)
    {
        if (m_type == FILE_RULE_DEFAULT || m_type == FILE_RULE_PARSE_FILEPATH)
        {
            if (extension && *extension)
            {
                throw Exception("File rules: Default and ColorSpaceNamePathSearch rules do "
                                "not accept any extension.");
            }
        }
        else
        {
            if (!extension || !*extension)
            {
                throw Exception("File rules: The file extension pattern is empty.");
            }
            ValidateRegularExpression(m_pattern.c_str(), extension);
            m_extension = extension;
            m_regex = "";
            m_type = FILE_RULE_GLOB;
        }
    }

    const char * getRegex() const noexcept
    {
        if (m_type != FILE_RULE_REGEX)
        {
            return "";
        }
        return m_regex.c_str();
    }

    void setRegex(const char * regex)
    {
        if (m_type == FILE_RULE_DEFAULT || m_type == FILE_RULE_PARSE_FILEPATH)
        {
            if (regex && *regex)
            {
                throw Exception("File rules: Default and ColorSpaceNamePathSearch rules do "
                                "not accept any regex.");
            }
        }
        else
        {
            ValidateRegularExpression(regex);
            m_regex = regex;
            m_pattern = "";
            m_extension = "";
            m_type = FILE_RULE_REGEX;
        }
    }

    const char * getColorSpace() const
    {
        return m_colorSpace.c_str();
    }

    void setColorSpace(const char * colorSpace)
    {
        if (m_type == FILE_RULE_PARSE_FILEPATH)
        {
            if (colorSpace && *colorSpace)
            {
                throw Exception("File rules: ColorSpaceNamePathSearch rule does not accept any "
                                "color space.");
            }
        }
        else
        {
            if (!colorSpace || !*colorSpace)
            {
                throw Exception("File rules: color space name can't be empty.");
            }
            m_colorSpace = colorSpace;
        }
    }

    bool matches(const Config & config, const char * path) const
    {
        switch (m_type)
        {
        case FILE_RULE_DEFAULT:
            return true;
        case FILE_RULE_PARSE_FILEPATH:
        {
            const int rightMostColorSpaceIndex = ParseColorSpaceFromString(config, path);
            if (rightMostColorSpaceIndex >= 0)
            {
                m_colorSpace = config.getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL,
                                                               COLORSPACE_ALL,
                                                               rightMostColorSpaceIndex);
                return true;
            }
            return false;
        }
        case FILE_RULE_REGEX:
        {
            const std::regex reg(m_regex.c_str());
            return regex_match(path, reg);
        }
        case FILE_RULE_GLOB:
        {
            const std::string exp = BuildRegularExpression(m_pattern.c_str(),
                                                           m_extension.c_str());
            const std::regex reg(exp.c_str());
            return regex_match(path, reg);
        }
        }
        return false;
    }

    void validate(const Config & cfg) const
    {
        if (m_type != FILE_RULE_PARSE_FILEPATH)
        {
            // Can be a color space, a role (all color spaces) or a named transform.
            if (!cfg.getColorSpace(m_colorSpace.c_str()))
            {
                if (!cfg.getNamedTransform(m_colorSpace.c_str()))
                {
                    std::ostringstream oss;
                    oss << "File rules: rule named '" << m_name << "' is referencing '"
                        << m_colorSpace << "' that is neither a color space nor a named "
                                           "transform.";
                    throw Exception(oss.str().c_str());
                }
            }
        }
    }

    CustomKeysContainer m_customKeys;

private:

    std::string m_name;
    mutable std::string m_colorSpace;
    std::string m_pattern;
    std::string m_extension;
    std::string m_regex;
    RuleType m_type{ FILE_RULE_GLOB };
};

FileRules::FileRules()
    : m_impl(new FileRules::Impl())
{
}

FileRules::~FileRules()
{
    delete m_impl;
    m_impl = nullptr;
}

FileRulesRcPtr FileRules::Create()
{
    return FileRulesRcPtr(new FileRules(), &deleter);
}

void FileRules::deleter(FileRules * fr)
{
    delete fr;
}

FileRulesRcPtr FileRules::createEditableCopy() const
{
    FileRulesRcPtr rules = Create();
    *rules->m_impl = *m_impl; // Deep copy.
    return rules;
}

FileRules::Impl::Impl()
{
    auto defaultRule = std::make_shared<FileRule>(FileRules::DefaultRuleName);
    defaultRule->setColorSpace(ROLE_DEFAULT);
    m_rules.push_back(defaultRule);
}

FileRules::Impl & FileRules::Impl::operator=(const FileRules::Impl & rhs)
{
    if (this!=&rhs)
    {
        m_rules.clear(); // Remove the 'Default' rule.

        for (const auto & rule : rhs.m_rules)
        {
            m_rules.push_back(rule->clone());
        }
    }

    return *this;
}

void FileRules::Impl::validatePosition(size_t ruleIndex, DefaultAllowed allowDefault) const
{
    const auto numRules = m_rules.size();
    if (ruleIndex >= numRules)
    {
        std::ostringstream oss;
        oss << "File rules: rule index '" << ruleIndex << "' invalid."
            << " There are only '" << numRules << "' rules.";
        throw Exception(oss.str().c_str());
    }
    if (allowDefault == DEFAULT_NOT_ALLOWED && ruleIndex + 1 == numRules)
    {
        std::ostringstream oss;
        oss << "File rules: rule index '" << ruleIndex << "' is the default rule.";
        throw Exception(oss.str().c_str());
    }
}

void FileRules::Impl::validateNewRule(size_t ruleIndex, const char * name) const
{
    if (!name || !*name)
    {
        throw Exception("File rules: rule should have a non-empty name.");
    }
    auto existingRule = std::find_if(m_rules.begin(), m_rules.end(),
                                     [name](const FileRuleRcPtr & rule)
                                     {
                                         return 0==Platform::Strcasecmp(name, rule->getName());
                                     });
    if (existingRule != m_rules.end())
    {
        std::ostringstream oss;
        oss << "File rules: A rule named '" << name << "' already exists.";
        throw Exception(oss.str().c_str());
    }
    validatePosition(ruleIndex, DEFAULT_ALLOWED);
    if (0==Platform::Strcasecmp(name, FileRules::DefaultRuleName))
    {
        std::ostringstream oss;
        oss << "File rules: Default rule already exists at index "
            << " '" << m_rules.size() - 1 << "'.";
        throw Exception(oss.str().c_str());
    }
}

const char * FileRules::Impl::getRuleFromFilepath(const Config & config, const char * filePath,
                                                  size_t & ruleIndex) const
{
    const auto numRules = m_rules.size();
    for (size_t i = 0; i < numRules; ++i)
    {
        if (m_rules[i]->matches(config, filePath))
        {
            ruleIndex = i;
            return m_rules[i]->getColorSpace();
        }
    }
    // Should not be reached since the default rule always matches.
    return m_rules.back()->getColorSpace();
}

void FileRules::Impl::moveRule(size_t ruleIndex, int offset)
{
    validatePosition(ruleIndex, DEFAULT_NOT_ALLOWED);
    int newIndex = (int)ruleIndex + offset;
    if (newIndex < 0 || newIndex >= (int)m_rules.size() - 1)
    {
        std::ostringstream oss;
        oss << "File rules: rule at index '" << ruleIndex << "' may not be moved to index '"
            << newIndex << "'.";
        throw Exception(oss.str().c_str());
    }
    auto rule = m_rules[ruleIndex];
    m_rules.erase(m_rules.begin() + ruleIndex);
    m_rules.insert(m_rules.begin() + newIndex, rule);
}


void FileRules::Impl::validate(const Config & cfg) const
{
    for (auto & rule : m_rules)
    {
        rule->validate(cfg);
    }
}

size_t FileRules::getNumEntries() const noexcept
{
    return m_impl->m_rules.size();
}

size_t FileRules::getIndexForRule(const char * ruleName) const
{
    const size_t numRules = m_impl->m_rules.size();
    for (size_t idx = 0; idx < numRules; ++idx)
    {
        if (0==Platform::Strcasecmp(ruleName, m_impl->m_rules[idx]->getName()))
        {
            return idx;
        }
    }

    std::ostringstream oss;
    oss << "File rules: rule name '" << ruleName << "' not found.";
    throw Exception(oss.str().c_str());
}

const char * FileRules::getName(size_t ruleIndex) const
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_ALLOWED);
    return m_impl->m_rules[ruleIndex]->getName();
}

const char * FileRules::getPattern(size_t ruleIndex) const
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_ALLOWED);
    return m_impl->m_rules[ruleIndex]->getPattern();
}

void FileRules::setPattern(size_t ruleIndex, const char * pattern)
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_NOT_ALLOWED);
    m_impl->m_rules[ruleIndex]->setPattern(pattern);
}

const char * FileRules::getExtension(size_t ruleIndex) const
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_ALLOWED);
    return m_impl->m_rules[ruleIndex]->getExtension();
}

void FileRules::setExtension(size_t ruleIndex, const char * extension)
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_NOT_ALLOWED);
    m_impl->m_rules[ruleIndex]->setExtension(extension);
}

const char * FileRules::getRegex(size_t ruleIndex) const
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_ALLOWED);
    return m_impl->m_rules[ruleIndex]->getRegex();
}

void FileRules::setRegex(size_t ruleIndex, const char * regex)
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_NOT_ALLOWED);
    m_impl->m_rules[ruleIndex]->setRegex(regex);
}

// Color space or role.
const char * FileRules::getColorSpace(size_t ruleIndex) const
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_ALLOWED);
    return m_impl->m_rules[ruleIndex]->getColorSpace();
}

void FileRules::setColorSpace(size_t ruleIndex, const char * colorSpace)
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_ALLOWED);
    m_impl->m_rules[ruleIndex]->setColorSpace(colorSpace);
}

size_t FileRules::getNumCustomKeys(size_t ruleIndex) const
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_ALLOWED);
    return m_impl->m_rules[ruleIndex]->m_customKeys.getSize();
}

const char * FileRules::getCustomKeyName(size_t ruleIndex, size_t key) const
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_ALLOWED);
    try
    {
        return m_impl->m_rules[ruleIndex]->m_customKeys.getName(key);
    }
    catch (Exception & e)
    {
        std::ostringstream oss;
        oss << "File rules: the custom key access for file rule '"
            << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' failed: " << e.what();
        throw Exception(oss.str().c_str());
    }
}

const char * FileRules::getCustomKeyValue(size_t ruleIndex, size_t key) const
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_ALLOWED);
    try
    {
        return m_impl->m_rules[ruleIndex]->m_customKeys.getValue(key);
    }
    catch (Exception & e)
    {
        std::ostringstream oss;
        oss << "File rules: the custom key access for file rule '"
            << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' failed: " << e.what();
        throw Exception(oss.str().c_str());
    }
}

void FileRules::setCustomKey(size_t ruleIndex, const char * key, const char * value)
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_ALLOWED);
    try
    {
        m_impl->m_rules[ruleIndex]->m_customKeys.set(key, value);
    }
    catch (Exception & e)
    {
        std::ostringstream oss;
        oss << "File rules: rule named '" << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' error: " << e.what();
        throw Exception(oss.str().c_str());
    }
}

void FileRules::insertRule(size_t ruleIndex, const char * name, const char * colorSpace,
                           const char * pattern, const char * extension)
{
    const std::string ruleName(StringUtils::Trim(name ? name : ""));

    m_impl->validateNewRule(ruleIndex, ruleName.c_str());

    auto newRule = std::make_shared<FileRule>(ruleName.c_str());
    newRule->setColorSpace(colorSpace);
    newRule->setPattern(pattern);
    newRule->setExtension(extension);
    m_impl->m_rules.insert(m_impl->m_rules.begin() + ruleIndex, newRule);
}

void FileRules::insertRule(size_t ruleIndex, const char * name, const char * colorSpace,
                           const char * regex)
{
    const std::string ruleName(StringUtils::Trim(name ? name : ""));

    m_impl->validateNewRule(ruleIndex, ruleName.c_str());

    auto newRule = std::make_shared<FileRule>(ruleName.c_str());
    newRule->setColorSpace(colorSpace);
    newRule->setRegex(regex);
    m_impl->m_rules.insert(m_impl->m_rules.begin() + ruleIndex, newRule);
}

void FileRules::insertPathSearchRule(size_t ruleIndex)
{
    return insertRule(ruleIndex, FileRules::FilePathSearchRuleName, nullptr, nullptr);
}

void FileRules::setDefaultRuleColorSpace(const char * colorSpace)
{
    m_impl->m_rules.back()->setColorSpace(colorSpace);
}

void FileRules::removeRule(size_t ruleIndex)
{
    m_impl->validatePosition(ruleIndex, Impl::DEFAULT_NOT_ALLOWED);
    m_impl->m_rules.erase(m_impl->m_rules.begin() + ruleIndex);
}

void FileRules::increaseRulePriority(size_t ruleIndex)
{
    m_impl->moveRule(ruleIndex, -1);
}

void FileRules::decreaseRulePriority(size_t ruleIndex)
{
    m_impl->moveRule(ruleIndex, 1);
}

bool FileRules::isDefault() const noexcept
{
    if (m_impl->m_rules.size() == 1)
    {
        const auto & rule = m_impl->m_rules[0];
        if (rule->m_customKeys.getSize() == 0)
        {
            // NB: Don't need to check the rule name -- the default rule may not be removed, so if
            // there is only one rule, it's the default one.
            if (StringUtils::Compare(rule->getColorSpace(), ROLE_DEFAULT))
            {
                return true;
            }
        }
    }
    return false;
}

const char * FileRules::Impl::getColorSpaceFromFilepath(const Config & config,
                                                        const char * filePath) const
{
    size_t ruleIndex = 0;
    return getColorSpaceFromFilepath(config, filePath, ruleIndex);
}

const char * FileRules::Impl::getColorSpaceFromFilepath(const Config & config, const char * filePath,
                                                        size_t & ruleIndex) const
{
    return getRuleFromFilepath(config, filePath, ruleIndex);
}

bool FileRules::Impl::filepathOnlyMatchesDefaultRule(const Config & config, const char * filePath) const
{
    size_t rulePos = 0;
    getColorSpaceFromFilepath(config, filePath, rulePos);
    return (rulePos + 1) == m_rules.size();
}

std::ostream & operator<< (std::ostream & os, const FileRules & fr)
{
    const size_t numRules = fr.getNumEntries();
    for (size_t r = 0; r < numRules; ++r)
    {
        os << "<FileRule name=" << fr.getName(r);
        const char * cs{ fr.getColorSpace(r) };
        if (cs && *cs)
        {
            os << ", colorspace=" << cs;
        }
        const char * regex{ fr.getRegex(r) };
        if (regex && *regex)
        {
            os << ", regex=" << regex;
        }
        const char * pattern{ fr.getPattern(r) };
        if (pattern && *pattern)
        {
            os << ", pattern=" << pattern;
        }
        const char * extension{ fr.getExtension(r) };
        if (extension && *extension)
        {
            os << ", extension=" << extension;
        }
        const size_t numCK = fr.getNumCustomKeys(r);
        if (numCK)
        {
            os << ", customKeys=[";
            for (size_t ck = 0; ck < numCK; ++ck)
            {
                os << "(" << fr.getCustomKeyName(r, ck);
                os << ", " << fr.getCustomKeyValue(r, ck) << ")";
                if (ck + 1 != numCK)
                {
                    os << ", ";
                }
            }
            os << "]";
        }
        os << ">";
        if (r + 1 != numRules)
        {
            os << "\n";
        }
    }
    return os;
}
} // namespace OCIO_NAMESPACE

