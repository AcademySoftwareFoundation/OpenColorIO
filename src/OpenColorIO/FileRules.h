// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FILERULES_H
#define INCLUDED_OCIO_FILERULES_H

#include <functional>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

namespace FileRuleUtils
{

constexpr char Name[]       { "name" };
constexpr char ColorSpace[] { "colorspace" };
constexpr char Pattern[]    { "pattern" };
constexpr char Extension[]  { "extension" };
constexpr char Regex[]      { "regex" };
constexpr char CustomKey[]  { "custom" };

}

class FileRule;
using FileRuleRcPtr = OCIO_SHARED_PTR<FileRule>;

class FileRules::Impl
{
public:
    enum DefaultAllowed
    {
        DEFAULT_ALLOWED,
        DEFAULT_NOT_ALLOWED
    };

    Impl();
    Impl(const Impl &) = delete;
    Impl & operator=(const Impl & rhs);
    ~Impl() = default;

    const char * getRuleFromFilepath(const Config & config, const char * filePath,
                                     size_t & ruleIndex) const;

    void validatePosition(size_t ruleIndex, DefaultAllowed allowDefault) const;

    // Throws if ruleIndex or name are invalid.
    void validateNewRule(size_t ruleIndex, const char * name) const;

    void moveRule(size_t ruleIndex, int offset);

    const char * getColorSpaceFromFilepath(const Config & config, const char * filePath) const;

    const char * getColorSpaceFromFilepath(const Config & config, const char * filePath,
                                           size_t & ruleIndex) const;

    bool filepathOnlyMatchesDefaultRule(const Config & config, const char * filePath) const;

    void validate(const Config & cfg) const;

private:

    friend class FileRules;

    // All rules, default rule always at the end.
    std::vector<FileRuleRcPtr> m_rules;
};

} // namespace OCIO_NAMESPACE

#endif
