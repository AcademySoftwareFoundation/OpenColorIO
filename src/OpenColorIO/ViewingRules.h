// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_VIEWINGRULES_H
#define INCLUDED_OCIO_VIEWINGRULES_H

#include <functional>
#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

namespace ViewingRuleUtils
{
constexpr char Name[]       { "name" };
constexpr char ColorSpaces[]{ "colorspaces" };
constexpr char Encodings[]  { "encodings" };
constexpr char CustomKey[]  { "custom" };
}

bool FindRule(ConstViewingRulesRcPtr vr, const std::string & name, size_t & ruleIndex);

class ViewingRule;
using ViewingRuleRcPtr = OCIO_SHARED_PTR<ViewingRule>;

class ViewingRules::Impl
{
public:

    Impl() = default;
    Impl(const Impl &) = delete;
    Impl & operator=(const Impl & rhs);
    ~Impl() = default;

    void validatePosition(size_t ruleIndex) const;
    void validateNewRule(const char * name) const;

    void validate(std::function<ConstColorSpaceRcPtr(const char *)> colorSpaceAccessor,
                     const ColorSpaceSetRcPtr & colorspaces) const;

private:

    friend class ViewingRules;

    // All rules.
    std::vector<ViewingRuleRcPtr> m_rules;
};

} // namespace OCIO_NAMESPACE

#endif
