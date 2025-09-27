// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "TokensManager.h"
#include "CustomKeys.h"
#include "Logging.h"
#include "ParseUtils.h"
#include "Platform.h"
#include "ViewingRules.h"

namespace OCIO_NAMESPACE
{

namespace
{
bool IsEncodingUsed(const ColorSpaceSetRcPtr & colorspaces, const char * encName)
{
    const std::string teststr = StringUtils::Lower(encName);
    const int numCS = colorspaces->getNumColorSpaces();
    for (int cs = 0; cs < numCS; ++cs)
    {
        auto colorspace = colorspaces->getColorSpaceByIndex(cs);
        const std::string csis{ colorspace->getEncoding() };
        if (StringUtils::Lower(csis) == teststr)
        {
            return true;
        }
    }
    return false;
}
}

class ViewingRule
{
public:

    ViewingRule() = delete;
    ViewingRule(const ViewingRule &) = delete;
    ViewingRule & operator=(const ViewingRule &) = delete;

    explicit ViewingRule(const char * name)
        : m_name(name)
    {
    }

    ViewingRuleRcPtr clone() const
    {
        ViewingRuleRcPtr rule = std::make_shared<ViewingRule>(m_name.c_str());

        rule->m_colorSpaces = m_colorSpaces;
        rule->m_encodings   = m_encodings;
        rule->m_customKeys  = m_customKeys;

        return rule;
    }

    const char * getName() const noexcept
    {
        return m_name.c_str();
    }

    void validate(std::function<ConstColorSpaceRcPtr(const char *)> colorSpaceAccesssor,
                     const ColorSpaceSetRcPtr & colorspaces) const
    {
        const auto numCS = m_colorSpaces.getNumTokens();
        for (int csIdx = 0; csIdx < numCS; ++csIdx)
        {
            const char * csname = m_colorSpaces.getToken(csIdx);
            // Can be a color space or a role (all color spaces).
            ConstColorSpaceRcPtr cs = colorSpaceAccesssor(csname);
            if (!cs)
            {
                std::ostringstream os;
                os << "The rule '" << m_name << "' ";
                os << "refers to color space '" << std::string(csname);
                os << "' which is not defined.";
                throw Exception(os.str().c_str());
            }
        }
        const int numEnc = m_encodings.getNumTokens();
        for (int encIdx = 0; encIdx < numEnc; ++encIdx)
        {
            const char * encName = m_encodings.getToken(encIdx);
            if (!IsEncodingUsed(colorspaces, encName))
            {
                std::ostringstream os;
                os << "The rule '" << m_name << "' ";
                os << "refers to encoding '" << std::string(encName);
                os << "' that is not used by any of the color spaces.";
                LogWarning(os.str());
            }
        }
        if (numCS + numEnc == 0)
        {
            std::ostringstream os;
            os << "The rule '" << m_name << "' ";
            os << "must have either a color space or an encoding.";
            throw Exception(os.str().c_str());
        }
        else if (numCS != 0 && numEnc != 0)
        {
            std::ostringstream os;
            os << "The rule '" << m_name << "' ";
            os << "cannot refer to both a color space and an encoding.";
            throw Exception(os.str().c_str());
        }
    }


    CustomKeysContainer m_customKeys;
    TokensManager m_colorSpaces;
    TokensManager m_encodings;

private:

    const std::string m_name;
};

ViewingRules::ViewingRules()
    : m_impl(new ViewingRules::Impl())
{
}

ViewingRules::~ViewingRules()
{
    delete m_impl;
    m_impl = nullptr;
}

ViewingRulesRcPtr ViewingRules::Create()
{
    return ViewingRulesRcPtr(new ViewingRules(), &deleter);
}

void ViewingRules::deleter(ViewingRules * vr)
{
    delete vr;
}

ViewingRulesRcPtr ViewingRules::createEditableCopy() const
{
    ViewingRulesRcPtr rules = Create();
    *rules->m_impl = *m_impl; // Deep copy.
    return rules;
}

ViewingRules::Impl & ViewingRules::Impl::operator=(const ViewingRules::Impl & rhs)
{
    if (this != &rhs)
    {
        m_rules.clear();

        for (const auto & rule : rhs.m_rules)
        {
            m_rules.push_back(rule->clone());
        }
    }

    return *this;
}

void ViewingRules::Impl::validatePosition(size_t ruleIndex) const
{
    const auto numRules = m_rules.size();
    if (ruleIndex >= numRules)
    {
        std::ostringstream oss;
        oss << "Viewing rules: rule index '" << ruleIndex << "' invalid."
            << " There are only '" << numRules << "' rules.";
        throw Exception(oss.str().c_str());
    }
}

void ViewingRules::Impl::validateNewRule(const char * name) const
{
    if (!name || !*name)
    {
        throw Exception("Viewing rules: rule must have a non-empty name.");
    }
    auto existingRule = std::find_if(m_rules.begin(), m_rules.end(),
                                     [name](const ViewingRuleRcPtr & rule)
                                     {
                                         return 0 == Platform::Strcasecmp(name, rule->getName());
                                     });
    if (existingRule != m_rules.end())
    {
        std::ostringstream oss;
        oss << "Viewing rules: A rule named '" << name << "' already exists.";
        throw Exception(oss.str().c_str());
    }
}

void ViewingRules::Impl::validate(
    std::function<ConstColorSpaceRcPtr(const char *)> colorSpaceAccessor,
    const ColorSpaceSetRcPtr & colorspaces) const
{
    for (auto & rule : m_rules)
    {
        rule->validate(colorSpaceAccessor, colorspaces);
    }
}

size_t ViewingRules::getNumEntries() const noexcept
{
    return m_impl->m_rules.size();
}

size_t ViewingRules::getIndexForRule(const char * ruleName) const
{
    const size_t numRules = m_impl->m_rules.size();
    for (size_t idx = 0; idx < numRules; ++idx)
    {
        if (0 == Platform::Strcasecmp(ruleName, m_impl->m_rules[idx]->getName()))
        {
            return idx;
        }
    }

    std::ostringstream oss;
    oss << "Viewing rules: rule name '" << ruleName << "' not found.";
    throw Exception(oss.str().c_str());
}

const char * ViewingRules::getName(size_t ruleIndex) const
{
    m_impl->validatePosition(ruleIndex);
    return m_impl->m_rules[ruleIndex]->getName();
}

size_t ViewingRules::getNumColorSpaces(size_t ruleIndex) const
{
    m_impl->validatePosition(ruleIndex);
    return m_impl->m_rules[ruleIndex]->m_colorSpaces.getNumTokens();
}

const char * ViewingRules::getColorSpace(size_t ruleIndex, size_t colorSpaceIndex) const
{
    m_impl->validatePosition(ruleIndex);
    const auto numCS = m_impl->m_rules[ruleIndex]->m_colorSpaces.getNumTokens();
    if (static_cast<int>(colorSpaceIndex) >= numCS)
    {
        std::ostringstream oss;
        oss << "Viewing rules: rule '" << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' at index '" << ruleIndex << "': colorspace index '" << colorSpaceIndex
            << "' is invalid. There are only '" << numCS << "' colorspaces.";
        throw Exception(oss.str().c_str());
    }
    return m_impl->m_rules[ruleIndex]->m_colorSpaces.getToken(static_cast<int>(colorSpaceIndex));
}

void ViewingRules::addColorSpace(size_t ruleIndex, const char * colorSpace)
{
    m_impl->validatePosition(ruleIndex);
    if (!colorSpace || !*colorSpace)
    {
        std::ostringstream oss;
        oss << "Viewing rules: rule '" << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' at index '" << ruleIndex << "': colorspace should have a non-empty name.";
        throw Exception(oss.str().c_str());
    }
    if (m_impl->m_rules[ruleIndex]->m_encodings.getNumTokens() != 0)
    {
        std::ostringstream oss;
        oss << "Viewing rules: rule '" << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' at index '" << ruleIndex
            << "': colorspace can't be added if there are encodings.";
        throw Exception(oss.str().c_str());
    }
    m_impl->m_rules[ruleIndex]->m_colorSpaces.addToken(colorSpace);
}

void ViewingRules::removeColorSpace(size_t ruleIndex, size_t colorSpaceIndex)
{
    const char * cs = getColorSpace(ruleIndex, colorSpaceIndex);
    m_impl->m_rules[ruleIndex]->m_colorSpaces.removeToken(cs);
}

size_t ViewingRules::getNumEncodings(size_t ruleIndex) const
{
    m_impl->validatePosition(ruleIndex);
    return m_impl->m_rules[ruleIndex]->m_encodings.getNumTokens();
}

const char * ViewingRules::getEncoding(size_t ruleIndex, size_t encodingIndex) const
{
    m_impl->validatePosition(ruleIndex);
    const auto numEnc = m_impl->m_rules[ruleIndex]->m_encodings.getNumTokens();
    if (static_cast<int>(encodingIndex) >= numEnc)
    {
        std::ostringstream oss;
        oss << "Viewing rules: rule '" << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' at index '" << ruleIndex << "': encoding index '" << encodingIndex
            << "' is invalid. There are only '" << numEnc << "' encodings.";
        throw Exception(oss.str().c_str());
    }
    return m_impl->m_rules[ruleIndex]->m_encodings.getToken(static_cast<int>(encodingIndex));
}

void ViewingRules::addEncoding(size_t ruleIndex, const char * encoding)
{
    m_impl->validatePosition(ruleIndex);
    if (!encoding || !*encoding)
    {
        std::ostringstream oss;
        oss << "Viewing rules: rule '" << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' at index '" << ruleIndex << "': encoding should have a non-empty name.";
        throw Exception(oss.str().c_str());
    }
    if (m_impl->m_rules[ruleIndex]->m_colorSpaces.getNumTokens() != 0)
    {
        std::ostringstream oss;
        oss << "Viewing rules: rule '" << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' at index '" << ruleIndex
            << "': encoding can't be added if there are colorspaces.";
        throw Exception(oss.str().c_str());
    }
    m_impl->m_rules[ruleIndex]->m_encodings.addToken(encoding);
}

void ViewingRules::removeEncoding(size_t ruleIndex, size_t encodingIndex)
{
    const char * encoding = getEncoding(ruleIndex, encodingIndex);
    m_impl->m_rules[ruleIndex]->m_encodings.removeToken(encoding);
}

size_t ViewingRules::getNumCustomKeys(size_t ruleIndex) const
{
    m_impl->validatePosition(ruleIndex);
    return m_impl->m_rules[ruleIndex]->m_customKeys.getSize();
}

const char * ViewingRules::getCustomKeyName(size_t ruleIndex, size_t key) const
{
    m_impl->validatePosition(ruleIndex);
    try
    {
        return m_impl->m_rules[ruleIndex]->m_customKeys.getName(key);
    }
    catch (Exception & e)
    {
        std::ostringstream oss;
        oss << "Viewing rules: rule named '" << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' error: " << e.what();
        throw Exception(oss.str().c_str());
    }
}

const char * ViewingRules::getCustomKeyValue(size_t ruleIndex, size_t key) const
{
    m_impl->validatePosition(ruleIndex);
    try
    {
        return m_impl->m_rules[ruleIndex]->m_customKeys.getValue(key);
    }
    catch (Exception & e)
    {
        std::ostringstream oss;
        oss << "Viewing rules: rule named '" << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' error: " << e.what();
        throw Exception(oss.str().c_str());
    }
}

void ViewingRules::setCustomKey(size_t ruleIndex, const char * key, const char * value)
{
    m_impl->validatePosition(ruleIndex);
    try
    {
        m_impl->m_rules[ruleIndex]->m_customKeys.set(key, value);
    }
    catch (Exception & e)
    {
        std::ostringstream oss;
        oss << "Viewing rules: rule named '" << std::string(m_impl->m_rules[ruleIndex]->getName())
            << "' error: " << e.what();
        throw Exception(oss.str().c_str());
    }
}

void ViewingRules::insertRule(size_t ruleIndex, const char * name)
{
    const std::string ruleName(StringUtils::Trim(name ? name : ""));

    m_impl->validateNewRule(ruleName.c_str());

    auto newRule = std::make_shared<ViewingRule>(ruleName.c_str());
    if (ruleIndex == getNumEntries())
    {
        m_impl->m_rules.push_back(newRule);
    }
    else
    {
        m_impl->validatePosition(ruleIndex);
        m_impl->m_rules.insert(m_impl->m_rules.begin() + ruleIndex, newRule);
    }
}

void ViewingRules::removeRule(size_t ruleIndex)
{
    m_impl->validatePosition(ruleIndex);
    m_impl->m_rules.erase(m_impl->m_rules.begin() + ruleIndex);
}

std::ostream & operator<< (std::ostream & os, const ViewingRules & vr)
{
    const size_t numRules = vr.getNumEntries();
    for (size_t r = 0; r < numRules; ++r)
    {
        os << "<ViewingRule name=" << vr.getName(r);
        const size_t numCS = vr.getNumColorSpaces(r);
        if (numCS)
        {
            os << ", colorspaces=[";
            for (size_t cs = 0; cs < numCS; ++cs)
            {
                os << vr.getColorSpace(r, cs);
                if (cs + 1 != numCS)
                {
                    os << ", ";
                }
            }
            os << "]";
        }
        const size_t numEnc = vr.getNumEncodings(r);
        if (numEnc)
        {
            os << ", encodings=[";
            for (size_t enc = 0; enc < numEnc; ++enc)
            {
                os << vr.getEncoding(r, enc);
                if (enc + 1 != numEnc)
                {
                    os << ", ";
                }
            }
            os << "]";
        }

        const size_t numCK = vr.getNumCustomKeys(r);
        if (numCK)
        {
            os << ", customKeys=[";
            for (size_t ck = 0; ck < numCK; ++ck)
            {
                os << "(" << vr.getCustomKeyName(r, ck);
                os << ", " << vr.getCustomKeyValue(r, ck) << ")";
                if (ck +1 != numCK)
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

bool FindRule(ConstViewingRulesRcPtr vr, const std::string & name, size_t & ruleIndex)
{
    const auto numrules = vr->getNumEntries();
    for (size_t index = 0; index < numrules; ++index)
    {
        const std::string ruleName{ vr->getName(index) };
        if (StrEqualsCaseIgnore(ruleName, name))
        {
            ruleIndex = index;
            return true;
        }
    }
    return false;
}

} // namespace OCIO_NAMESPACE
