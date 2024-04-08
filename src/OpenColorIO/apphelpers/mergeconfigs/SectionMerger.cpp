// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <iostream>
#include <cstring>
#include <map>
#include <mutex>
#include <sstream>
#include <vector>

#include <pystring.h>

#include <OpenColorIO/OpenColorIO.h>

#include "ConfigUtils.h"
#include "CustomKeys.h"
#include "Logging.h"
#include "OCIOMYaml.h"
#include "ParseUtils.h"
#include "Platform.h"
#include "SectionMerger.h"
#include "TokensManager.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{

// void printStrVec(const StringUtils::StringVec str)
// {
//     for (int i=0; i<str.size(); i++)
//         std::cout << i << ": " << str[i] << "\n";
// }

void splitActiveList(const char * list, StringUtils::StringVec & result)
{
    if (list && *list)
    {
        result = StringUtils::Split(list, ',');
    }
}

void mergeStringsWithoutDuplicates(StringUtils::StringVec & inputVec,
                                   StringUtils::StringVec & mergedVec)
{
    // Note that Contain requires a full exact match, not a partial match.
    // Hence it's important that the items in mergedVec be trimmed as well.
    StringUtils::Trim(mergedVec);

    for (const auto & item : inputVec)
    {
        const std::string trimmedItem = StringUtils::Trim(item);
        if (!trimmedItem.empty())
        {
            if (!StringUtils::Contain(mergedVec, trimmedItem))
            {
                mergedVec.push_back(trimmedItem);
            }
        }
    }
}

} // anon.

void SectionMerger::notify(std::string s, bool mustThrow) const
{
    if (!mustThrow)
    {
        // By logging, we can see all errors.
        LogWarning(s);
    }
    else
    {
        // By throwing, we only get the first error on conflict, but also stop the merge.
        throw Exception(s.c_str());
    }
}

//
// Important implementation note: All of the section merger code assumes the merged config
// is initialized from the base config.
//

////////////////////////////////////// GeneralMerger /////////////////////////////////////

namespace
{

void setMergedConfigVersion(ConfigRcPtr & config,
                            unsigned int inputMajor,
                            unsigned int inputMinor,
                            unsigned int baseMajor,
                            unsigned int baseMinor)
{
    unsigned int major = baseMajor;
    unsigned int minor = baseMinor;

    // Use the higher of the input or base version.
    if ((inputMajor * 100 + inputMinor) > (baseMajor * 100 + baseMinor))
    {
        major = inputMajor;
        minor = inputMinor;
    }

    // A merge always produces at least a v2 config.
    const unsigned int minMajor = 2u;
    const unsigned int minMinor = 0u;

    if ((minMajor * 100 + minMinor) > (major * 100 + minor))
    {
        major = minMajor;
        minor = minMinor;
    }

    config->setVersion(major, minor);
}

} // anon.


void GeneralMerger::handlePreferInput()
{
    // Set the config Name.
    const char * name = m_params->getName();
    if (name && *name)
    {
        // Use name from override.
        m_mergedConfig->setName(name);
    }
    else
    {
        // TODO: If empty, take it from the other config?
        m_mergedConfig->setName(m_inputConfig->getName());
    }

    // Set the config description.
    const char * desc = m_params->getDescription();
    if (desc && *desc)
    {
        // Use description from override.
        m_mergedConfig->setDescription(desc);
    }
    else
    {
        // TODO: If empty, take it from the other config?
        m_mergedConfig->setDescription(m_inputConfig->getDescription());
    }

    // Use the higher value for ocio_profile_version.
    setMergedConfigVersion(m_mergedConfig,
                           m_inputConfig->getMajorVersion(),
                           m_inputConfig->getMinorVersion(),
                           m_baseConfig->getMajorVersion(),
                           m_baseConfig->getMinorVersion());

    double rgb[3];
    m_inputConfig->getDefaultLumaCoefs(rgb);
    m_mergedConfig->setDefaultLumaCoefs(rgb);
}

void GeneralMerger::handlePreferBase()
{
    // Set the config Name.
    const char * name = m_params->getName();
    if (name && *name)
    {
        // Use name from override.
        m_mergedConfig->setName(name);
    }
    else
    {
        // TODO: If empty, take it from the other config?
        m_mergedConfig->setName(m_baseConfig->getName());
    }

    // Set the config description.
    const char * desc = m_params->getDescription();
    if (desc && *desc)
    {
        // Use description from override.
        m_mergedConfig->setDescription(desc);
    }
    else
    {
        // TODO: If empty, take it from the other config?
        m_mergedConfig->setDescription(m_baseConfig->getDescription());
    }

    // Use the higher value for ocio_profile_version.
    setMergedConfigVersion(m_mergedConfig,
                           m_inputConfig->getMajorVersion(),
                           m_inputConfig->getMinorVersion(),
                           m_baseConfig->getMajorVersion(),
                           m_baseConfig->getMinorVersion());

    double rgb[3];
    m_baseConfig->getDefaultLumaCoefs(rgb);
    m_mergedConfig->setDefaultLumaCoefs(rgb);
}

void GeneralMerger::handleInputOnly()
{
    // Set the config Name.
    const char * name = m_params->getName();
    if (name && *name)
    {
        // Use name from override.
        m_mergedConfig->setName(name);
    }
    else
    {
        m_mergedConfig->setName(m_inputConfig->getName());
    }

    // Set the config description.
    const char * desc = m_params->getDescription();
    if (desc && *desc)
    {
        // Use description from override.
        m_mergedConfig->setDescription(desc);
    }
    else
    {
        m_mergedConfig->setDescription(m_inputConfig->getDescription());
    }

    setMergedConfigVersion(m_mergedConfig,
                           m_inputConfig->getMajorVersion(),
                           m_inputConfig->getMinorVersion(),
                           0u,  // ignore base
                           0u);

    double rgb[3];
    m_inputConfig->getDefaultLumaCoefs(rgb);
    m_mergedConfig->setDefaultLumaCoefs(rgb);
}

void GeneralMerger::handleBaseOnly()
{
    // Set the config Name.
    const char * name = m_params->getName();
    if (name && *name)
    {
        // Use name from override.
        m_mergedConfig->setName(name);
    }
    else
    {
        m_mergedConfig->setName(m_baseConfig->getName());
    }

    // Set the config description.
    const char * desc = m_params->getDescription();
    if (desc && *desc)
    {
        // Use description from override.
        m_mergedConfig->setDescription(desc);
    }
    else
    {
        m_mergedConfig->setDescription(m_baseConfig->getDescription());
    }

    setMergedConfigVersion(m_mergedConfig,
                           0u,  // ignore input
                           0u,
                           m_baseConfig->getMajorVersion(),
                           m_baseConfig->getMinorVersion());

    double rgb[3];
    m_baseConfig->getDefaultLumaCoefs(rgb);
    m_mergedConfig->setDefaultLumaCoefs(rgb);
}

////////////////////////////////// GeneralMerger end /////////////////////////////////////

bool hasAlias(const ConstColorSpaceRcPtr & cs, const char * aliasName)
{
    if (cs)
    {
        for (size_t i = 0; i < cs->getNumAliases(); i++)
        {
            if (Platform::Strcasecmp(cs->getAlias(i), aliasName) == 0)
                return true;
        }
    }
    return false;
}

bool hasAlias(const ConstNamedTransformRcPtr & nt, const char * aliasName)
{
    if (nt)
    {
        for (size_t i = 0; i < nt->getNumAliases(); i++)
        {
            if (Platform::Strcasecmp(nt->getAlias(i), aliasName) == 0)
                return true;
        }
    }
    return false;
}

////////////////////////////////////////// RolesMerger ///////////////////////////////////

void RolesMerger::mergeInputRoles()
{
    // Insert roles from input config.
    for (int i = 0; i < m_inputConfig->getNumRoles(); i++)
    {
        const char * name = m_inputConfig->getRoleName(i);
        const char * roleColorSpaceName = m_inputConfig->getRoleColorSpace(name);

        if (m_mergedConfig->hasRole(name))
        {
            // The base config already has this role.
            const char * baseRoleColorSpaceName = m_mergedConfig->getRoleColorSpace(name);

            const ConfigMergingParameters::MergeStrategies strategy = m_params->getRoles();
            if (Platform::Strcasecmp(roleColorSpaceName, baseRoleColorSpaceName) != 0)
            {
                // The color spaces are different. Replace based on the strategy.
                if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT
                 || strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY)
                {

                    m_mergedConfig->setRole(name, roleColorSpaceName);
                }

                std::ostringstream os;
                os << "The Input config contains a role that would override Base config role '";
                os << name << "'.";
                notify(os.str(), m_params->isErrorOnConflict());
            }
            continue;
        }

        // Check for any conflicts.  Not allowing input roles to override color spaces
        // or named transforms in the base config.  The merge strategy only applies to
        // overriding base config roles.

        ConstColorSpaceRcPtr existingCS = m_mergedConfig->getColorSpace(name);
        if (existingCS)
        {
            // There is a conflict, figure out what it is.

            std::ostringstream os;
            if (Platform::Strcasecmp(existingCS->getName(), name) == 0)
            {
                os << "The Input config contains a role '" << name << "' that would override Base config color space '";
                os << existingCS->getName() << "'.";
            }
            else if (hasAlias(existingCS, name))
            {
                os << "The Input config contains a role '" << name << "' that would override an alias of Base config color space '";
                os << existingCS->getName() << "'.";
            }
            else // (Should never happen.)
            {
                std::ostringstream os;
                os << "Problem merging role: '" << name << "' due to color space conflict.";
                throw Exception(os.str().c_str());
            }

            notify(os.str(), m_params->isErrorOnConflict());
            continue;
        }

        ConstNamedTransformRcPtr existingNT = m_mergedConfig->getNamedTransform(name);
        if (existingNT)
        {
            // There is a conflict, figure out what it is.

            std::ostringstream os;
            if (Platform::Strcasecmp(existingNT->getName(), name) == 0)
            {
                os << "The Input config contains a role '" << name << "' that would override Base config named transform: '";
                os << existingNT->getName() << "'.";
            }
            else if (hasAlias(existingNT, name))
            {
                os << "The Input config contains a role '" << name << "' that would override an alias of Base config named transform: '";
                os << existingNT->getName() << "'.";
            }
            else // (Should never happen.)
            {
                std::ostringstream os;
                os << "Problem merging role: '" << name << "'.";
                throw Exception(os.str().c_str());
            }

            notify(os.str(), m_params->isErrorOnConflict());
            continue;
        }

        // No conflicts, go ahead and merge it.
        m_mergedConfig->setRole(name, roleColorSpaceName);
    }
}

void RolesMerger::handlePreferInput()
{
    mergeInputRoles();
}

void RolesMerger::handlePreferBase()
{
    mergeInputRoles();
}

void RolesMerger::handleInputOnly()
{
    // Remove the roles from base and take the roles from input.
    for (int i = 0; i < m_baseConfig->getNumRoles(); i++)
    {
        // Unset role from base config.
        m_mergedConfig->setRole(m_baseConfig->getRoleName(i), nullptr);
    }

    // Insert roles from input config.
    mergeInputRoles();
}

void RolesMerger::handleBaseOnly()
{
    // Nothing to do, since the merged config is initialized from the base config.
}

void RolesMerger::handleRemove()
{
    for (int i = 0; i < m_inputConfig->getNumRoles(); i++)
    {
        if (m_mergedConfig->hasRole(m_inputConfig->getRoleName(i)))
        {
            // Remove the role.
            m_mergedConfig->setRole(m_inputConfig->getRoleName(i), nullptr);
        }
    }
}

////////////////////////////////////// RolesMerger end ///////////////////////////////////

//////////////////////////////////////// FileRulesMerger /////////////////////////////////

bool fileRulesAreEqual(const ConstFileRulesRcPtr & f1,
                       size_t f1Idx,
                       const ConstFileRulesRcPtr & f2,
                       size_t f2Idx)
{
    // NB: No need to compare the name of the rules, that should be done in the caller.

    // Compare color space name, pattern, extension, and regex strings.

    if (Platform::Strcasecmp(f1->getColorSpace(f1Idx), f2->getColorSpace(f2Idx)) != 0 ||
        Platform::Strcasecmp(f1->getPattern(f1Idx), f2->getPattern(f2Idx)) != 0 ||
        Platform::Strcasecmp(f1->getRegex(f1Idx), f2->getRegex(f2Idx)) != 0 ||
        Platform::Strcasecmp(f1->getExtension(f1Idx), f2->getExtension(f2Idx)) != 0)
    {
        return false;
    }

    // Compare the custom keys, handling the case where they may be in a different order.

    if (f1->getNumCustomKeys(f1Idx) != f2->getNumCustomKeys(f2Idx))
    {
        return false;
    }

    CustomKeysContainer f1CustomKeys;
    for (size_t m = 0; m < f1->getNumCustomKeys(f1Idx); m++)
    {
        f1CustomKeys.set(f1->getCustomKeyName(f1Idx, m), f1->getCustomKeyValue(f1Idx, m));
    }

    for (size_t m = 0; m < f2->getNumCustomKeys(f2Idx); m++)
    {
        if (!f1CustomKeys.hasKey(f2->getCustomKeyName(f2Idx, m)))
        {
            return false;
        }
        else
        {
            if (Platform::Strcasecmp(f1CustomKeys.getValueForKey(f2->getCustomKeyName(f2Idx, m)),
                                     f2->getCustomKeyValue(f2Idx, m)) != 0)
            {
                return false;
            }
        }
    }

    return true;
}

void copyRule(const ConstFileRulesRcPtr & input,   // rule source
              size_t inputRuleIdx,                 // rule source index
              FileRulesRcPtr & merged,             // rule dest
              size_t mergedRuleIdx)                // rule dest index
{
    // Handle case where the rule is ColorSpaceNamePathSearch.

    const char * name = input->getName(inputRuleIdx);
    if (Platform::Strcasecmp(name, FileRules::FilePathSearchRuleName) == 0)
    {
        merged->insertPathSearchRule(mergedRuleIdx);
        return;
    }

    // Normal rule case.

    const char * regex = input->getRegex(inputRuleIdx);
    if (!regex || !*regex)
    {
        // The regex is empty --> handle it as a pattern & extension type rule.
        const char * pattern = input->getPattern(inputRuleIdx);
        const char * extension = input->getExtension(inputRuleIdx);
        merged->insertRule(mergedRuleIdx,
                           name,
                           input->getColorSpace(inputRuleIdx),
                           (!pattern || !*pattern) ? "*" : pattern,
                           (!extension || !*extension) ? "*" : extension);
    }
    else
    {
        // Handle it as a regex type rule.
        merged->insertRule(mergedRuleIdx,
                           name,
                           input->getColorSpace(inputRuleIdx),
                           regex);
    }

    // Copy over any custom keys.

    for (size_t k = 0; k < input->getNumCustomKeys(inputRuleIdx); k++)
    {
        merged->setCustomKey(mergedRuleIdx,
                             input->getCustomKeyName(inputRuleIdx, k),
                             input->getCustomKeyValue(inputRuleIdx, k));
    }
}

void FileRulesMerger::addRulesIfNotPresent(const ConstFileRulesRcPtr & input,
                                           FileRulesRcPtr & merged) const
{
    for (size_t inputRuleIdx = 0; inputRuleIdx < input->getNumEntries(); inputRuleIdx++)
    {
        bool hasConflict = false;

        try
        {
            // Check if the rule is already present.
            const char * name = input->getName(inputRuleIdx);

            // This will throw if the merged rules don't contain this name.
            size_t mergedRuleIdx = merged->getIndexForRule(name);

            // Based on the name, this file rule exists in the merged config.

            // If the rules are not identical, need to report conflict.
            if (!fileRulesAreEqual(merged, mergedRuleIdx, input, inputRuleIdx))
            {
                hasConflict = true;

                std::ostringstream os;
                os << "The Input config contains a value that would override ";
                os << "the Base config: file_rules: " << name;
                throw Exception(os.str().c_str());
            }
        }
        catch(const Exception & e)
        {
            if (hasConflict)
            {
                // Log or throw an exception describing the conflict.
                notify(e.what(), m_params->isErrorOnConflict());
            }
            else
            {
                // File rule does not exist, add it in the penultimate positon, before the default.
                // (Note that a default rule is always present, so will never get here in that case.)
                copyRule(input, inputRuleIdx, merged, merged->getNumEntries() - 1);
            }
        }
    }
}

void FileRulesMerger::addRulesAndOverwrite(const ConstFileRulesRcPtr & input,
                                           FileRulesRcPtr & merged) const
{
    for (size_t inputRuleIdx = 0; inputRuleIdx < input->getNumEntries(); inputRuleIdx++)
    {
        bool hasConflict = false;

        const char * name = input->getName(inputRuleIdx);
        try
        {
            // Check if the rule is already present, throw if it is not.
            size_t mergedRuleIdx = merged->getIndexForRule(name);

            if (!fileRulesAreEqual(merged, mergedRuleIdx, input, inputRuleIdx))
            {
                hasConflict = true;

                // Overwrite the existing rule.
                if (Platform::Strcasecmp(name, FileRules::DefaultRuleName) != 0)
                {
                    merged->removeRule(mergedRuleIdx);
                    copyRule(input, inputRuleIdx, merged, mergedRuleIdx);
                }
                else
                {
                    merged->setDefaultRuleColorSpace(input->getColorSpace(inputRuleIdx));
                }

                std::ostringstream os;
                os << "The Input config contains a value that would override ";
                os << "the Base config: file_rules: " << name;
                throw Exception(os.str().c_str());
            }
        }
        catch(const Exception & e)
        {
            if (hasConflict)
            {
                notify(e.what(), m_params->isErrorOnConflict());
            }
            else
            {
                // File rule does not exist, add it in the penultimate positon, before the default.
                // (Note that a default rule is always present, so will never get here in that case.)
                copyRule(input, inputRuleIdx, merged, merged->getNumEntries() - 1);
            }
        }
    }
}

void FileRulesMerger::handlePreferInput()
{
    ConstFileRulesRcPtr baseFr = m_baseConfig->getFileRules();
    ConstFileRulesRcPtr inputFr = m_inputConfig->getFileRules();

    // Handle strictparsing.
    m_mergedConfig->setStrictParsingEnabled(m_inputConfig->isStrictParsingEnabled());

    // Technique depends on whether the input rules should go first or not.
    if (m_params->isInputFirst())
    {
        // Copying file rules from input config.
        FileRulesRcPtr mergedFileRules = inputFr->createEditableCopy();
        // Insert file rules from base config, if not present.
        // If it doesn't exist, add it right before the default rule.
        addRulesIfNotPresent(baseFr, mergedFileRules);
        m_mergedConfig->setFileRules(mergedFileRules);
    }
    else
    {
        // Copying file rules from base config.
        FileRulesRcPtr mergedFileRules = baseFr->createEditableCopy();
        // Insert file rules from input config.
        // If the rule already exists, overwrite it.
        // If it doesn't exist, add it right before the default rule.
        addRulesAndOverwrite(inputFr, mergedFileRules);
        m_mergedConfig->setFileRules(mergedFileRules);
    }
}

void FileRulesMerger::handlePreferBase()
{
    ConstFileRulesRcPtr baseFr = m_baseConfig->getFileRules();
    ConstFileRulesRcPtr inputFr = m_inputConfig->getFileRules();

    // Handle strictparsing.
    // Nothing to do. Keep the base config value.

    // Technique depends on whether the input rules should go first or not.
    if (m_params->isInputFirst())
    {
        // Copying file rules from input config.
        FileRulesRcPtr mergedFileRules = inputFr->createEditableCopy();
        // Insert file rules from base config.
        // If the rule already exists, overwrite it.
        // If it doesn't exist, add it right before the default rule.
        addRulesAndOverwrite(baseFr, mergedFileRules);
        m_mergedConfig->setFileRules(mergedFileRules);
    }
    else
    {
        // Copying file rules from base config.
        FileRulesRcPtr mergedFileRules = baseFr->createEditableCopy();
        // Insert file rules from input config, if not present.
        // If it doesn't exist, add it right before the default rule.
        addRulesIfNotPresent(inputFr, mergedFileRules);
        m_mergedConfig->setFileRules(mergedFileRules);
    }
}

void FileRulesMerger::handleInputOnly()
{
    // Handle strictparsing.
    m_mergedConfig->setStrictParsingEnabled(m_inputConfig->isStrictParsingEnabled());

    // Simply take the rules from the input config.
    m_mergedConfig->setFileRules(m_inputConfig->getFileRules()->createEditableCopy());
}

void FileRulesMerger::handleBaseOnly()
{
    // Supported, but nothing to do.
}

void FileRulesMerger::handleRemove()
{
    ConstFileRulesRcPtr inputFr = m_inputConfig->getFileRules();
    FileRulesRcPtr mergedFileRules = m_baseConfig->getFileRules()->createEditableCopy();

    for (size_t f = 0; f < inputFr->getNumEntries(); f++)
    {
        const char * name = inputFr->getName(f);

        // Never remove the Default rule.
        if (Platform::Strcasecmp(name, FileRules::DefaultRuleName) == 0)
        {
            continue;
        }

        // Check if the input rule name is present in the base config.
        try
        {
            // Will throw if the name is not present.
            size_t idx = mergedFileRules->getIndexForRule(name);

            // Remove the rule (regardless of whether the content matches the base config).
            mergedFileRules->removeRule(idx);
        }
        catch(...)
        {
            // Do nothing if it is not present.
        }
    }
    m_mergedConfig->setFileRules(mergedFileRules);
}

//////////////////////////////////// FileRulesMerger end /////////////////////////////////

/////////////////////////////////////// DisplayViewMerger ////////////////////////////////

namespace
{

bool displayHasView(const ConstConfigRcPtr & cfg, const char * dispName, const char * viewName)
{
    // This returns null if either the display or view doesn't exist.
    // It works regardless of whether the display or view are active,
    // and it works regardless of whether the view is display-defined
    // or if the display has this as a shared view.
    //
    // It will only check config level shared views if dispName is null.
    // It will not check config level shared views if dispName is not null.
    const char * cs = cfg->getDisplayViewColorSpaceName(dispName, viewName);

    // All views must have a color space, so if it's not empty, the view exists.
    return (cs && *cs);
}

bool hasVirtualView(const ConstConfigRcPtr & cfg, const char * viewName)
{
    const char * cs = cfg->getVirtualDisplayViewColorSpaceName(viewName);

    // All views must have a color space, so if it's not empty, the view exists.
    return (cs && *cs);
}

void clearSharedViews(ConfigRcPtr & cfg)
{
    int numViews = cfg->getNumViews(VIEW_SHARED, nullptr);
    for (int v = numViews - 1; v >= 0; v--)
    {
        const char * sharedViewName = cfg->getView(VIEW_SHARED, nullptr, v);
        if (sharedViewName && *sharedViewName)
        {
            cfg->removeSharedView(sharedViewName);
        }
    }
}

bool viewIsShared(const ConstConfigRcPtr & cfg,
                  const char * dispName,
                  const char * viewName)
{
    // Check if a view within a given display is a display-defined view or is referencing
    // one of the config's shared views.

    for (int v = 0; v < cfg->getNumViews(VIEW_SHARED, dispName); v++)
    {
        const char * sharedViewName = cfg->getView(VIEW_SHARED, dispName, v);
        if (sharedViewName && *sharedViewName && Platform::Strcasecmp(sharedViewName, viewName) == 0)
        {
            return true;
        }
    }

    return false;
}

bool virtualViewIsShared(const ConstConfigRcPtr & cfg,
                         const char * viewName)
{
    for (int v = 0; v < cfg->getVirtualDisplayNumViews(VIEW_SHARED); v++)
    {
        const char * sharedViewName = cfg->getVirtualDisplayView(VIEW_SHARED, v);
        if (sharedViewName && *sharedViewName && Platform::Strcasecmp(sharedViewName, viewName) == 0)
        {
            return true;
        }
    }

    return false;
}

bool viewsAreEqual(const ConstConfigRcPtr & first,
                   const ConstConfigRcPtr & second,
                   const char * dispName,               // may be empty or nullptr for shared views
                   const char * viewName)
{
    // It's ok to call this even for displays/views that don't exist, it will simply return false.

    // Note that this will return true even if the view is display-defined in one config and a reference
    // to a shared view in the other config (both within the same display), as long as the contents match.

    // These calls return null if either the display or view doesn't exist (regardless if it's active).
    const char * cs1 = first->getDisplayViewColorSpaceName(dispName, viewName);
    const char * cs2 = second->getDisplayViewColorSpaceName(dispName, viewName);

    // If the color space is not null, the display and view exist.
    if (cs1 && *cs1 && cs2 && *cs2)
    {
        // Both configs have a display and view by this name, now check the contents.
        if (Platform::Strcasecmp(cs1, cs2) == 0)
        {
            // Note the remaining strings may be empty in a valid view.
            // Intentionally not checking the description since it is not a functional difference.
            if ( (Platform::Strcasecmp(first->getDisplayViewLooks(dispName, viewName),
                                       second->getDisplayViewLooks(dispName, viewName)) == 0) &&
                 (Platform::Strcasecmp(first->getDisplayViewTransformName(dispName, viewName),
                                       second->getDisplayViewTransformName(dispName, viewName)) == 0) &&
                 (Platform::Strcasecmp(first->getDisplayViewRule(dispName, viewName),
                                       second->getDisplayViewRule(dispName, viewName)) == 0) )
            {
                return true;
            }
        }
    }
    return false;
}

bool virtualViewsAreEqual(const ConstConfigRcPtr & first,
                          const ConstConfigRcPtr & second,
                          const char * viewName)
{
    const char * cs1 = first->getVirtualDisplayViewColorSpaceName(viewName);
    const char * cs2 = second->getVirtualDisplayViewColorSpaceName(viewName);

    // If the color space is not null, the display and view exist.
    if (cs1 && *cs1 && cs2 && *cs2)
    {
        if (Platform::Strcasecmp(cs1, cs2) == 0)
        {
            // Note the remaining strings may be empty in a valid view.
            // Intentionally not checking the description since it is not a functional difference.
            if ( (Platform::Strcasecmp(first->getVirtualDisplayViewLooks(viewName),
                                       second->getVirtualDisplayViewLooks(viewName)) == 0) &&
                 (Platform::Strcasecmp(first->getVirtualDisplayViewTransformName(viewName),
                                       second->getVirtualDisplayViewTransformName(viewName)) == 0) &&
                 (Platform::Strcasecmp(first->getVirtualDisplayViewRule(viewName),
                                       second->getVirtualDisplayViewRule(viewName)) == 0) )
            {
                return true;
            }
        }
    }
    return false;
}

bool viewTransformsAreEqual(const ConstConfigRcPtr & first,
                            const ConstConfigRcPtr & second,
                            const char * name)
{
    ConstViewTransformRcPtr vt1 = first->getViewTransform(name);
    ConstViewTransformRcPtr vt2 = second->getViewTransform(name);
    if (vt1 && vt2)
    {
        // Both configs have a view transform by this name, now check the parts.
        // Note: Not checking family or description since it is not a functional difference.

        // FIXME: Check categories.

        if (vt1->getReferenceSpaceType() != vt2->getReferenceSpaceType())
        {
            return false;
        }

        ConstTransformRcPtr t1_to = vt1->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE);
        ConstTransformRcPtr t2_to = vt2->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE);
        if (t1_to || t2_to)
        {
            if (!t1_to || !t2_to)  // one of them has a transform but the other does not
            {
                return false;
            }

            // FIXME: Compare transforms.
        }

        ConstTransformRcPtr t1_from = vt1->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE);
        ConstTransformRcPtr t2_from = vt2->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE);
        if (t1_from || t2_from)
        {
            if (!t1_from || !t2_from)  // one of them has a transform but the other does not
            {
                return false;
            }

            // FIXME: Compare transforms.
        }

        return true;
    }
    return false;
}

bool viewingRulesAreEqual(const ConstViewingRulesRcPtr & r1,
                          size_t r1Idx,
                          const ConstViewingRulesRcPtr & r2,
                          size_t r2Idx)
{
    // NB: No need to compare the name of the rules, that should be done in the caller.

    // Compare color space tokens, handling the case where they may be in a different order.

    if (r1->getNumColorSpaces(r1Idx) != r2->getNumColorSpaces(r2Idx))
    {
        return false;
    }

    TokensManager r1ColorSpaces;
    for (size_t m = 0; m < r1->getNumColorSpaces(r1Idx); m++)
    {
        r1ColorSpaces.addToken(r1->getColorSpace(r1Idx, m));
    }

    for (size_t m = 0; m < r2->getNumColorSpaces(r2Idx); m++)
    {
        if (!r1ColorSpaces.hasToken(r2->getColorSpace(r2Idx, m)))
        {
            return false;
        }
    }

    // Compare encoding tokens, handling the case where they may be in a different order.

    if (r1->getNumEncodings(r1Idx) != r2->getNumEncodings(r2Idx))
    {
        return false;
    }

    TokensManager r1Encodings;
    for (size_t m = 0; m < r1->getNumEncodings(r1Idx); m++)
    {
        r1Encodings.addToken(r1->getEncoding(r1Idx, m));
    }

    for (size_t m = 0; m < r2->getNumEncodings(r2Idx); m++)
    {
        if(!r1Encodings.hasToken(r2->getEncoding(r2Idx, m)))
        {
            return false;
        }
    }

    // Compare the custom keys, handling the case where they may be in a different order.

    if (r1->getNumCustomKeys(r1Idx) != r2->getNumCustomKeys(r2Idx))
    {
        return false;
    }

    CustomKeysContainer r1CustomKeys;
    for (size_t m = 0; m < r1->getNumCustomKeys(r1Idx); m++)
    {
        r1CustomKeys.set(r1->getCustomKeyName(r1Idx, m), r1->getCustomKeyValue(r1Idx, m));
    }

    for (size_t m = 0; m < r2->getNumCustomKeys(r2Idx); m++)
    {
        if (!r1CustomKeys.hasKey(r2->getCustomKeyName(r2Idx, m)))
        {
            return false;
        }
        else
        {
            if (Platform::Strcasecmp(r1CustomKeys.getValueForKey(r2->getCustomKeyName(r2Idx, m)),
                                     r2->getCustomKeyValue(r2Idx, m)) != 0)
            {
                return false;
            }
        }
    }

    return true;
}

void copyViewingRule(const ConstViewingRulesRcPtr & src,
                     size_t srcIdx,
                     size_t dstIdx,
                     ViewingRulesRcPtr & rules)
{
    try
    {
        rules->insertRule(dstIdx, src->getName(srcIdx));

        for (int j = 0; j < static_cast<int>(src->getNumColorSpaces(srcIdx)); j++)
        {
            rules->addColorSpace(dstIdx, src->getColorSpace(srcIdx, j));
        }

        for (int k = 0; k < static_cast<int>(src->getNumEncodings(srcIdx)); k++)
        {
            rules->addEncoding(dstIdx, src->getEncoding(srcIdx, k));
        }

        for (int l = 0; l < static_cast<int>(src->getNumCustomKeys(srcIdx)); l++)
        {
            rules->setCustomKey(dstIdx, src->getCustomKeyName(srcIdx, l), src->getCustomKeyValue(srcIdx, l));
        }
    }
    catch(...)
    {
        // Don't add it if any errors.
        // Continue.
    }
}

void addUniqueViewingRules(const ConstViewingRulesRcPtr & rules,
                           ViewingRulesRcPtr & mergedRules)
{
    for (size_t i = 0; i < rules->getNumEntries(); i++)
    {
        const char * name = rules->getName(i);
        // Take the rule from the first config if it does not exist.
        try
        {
            mergedRules->getIndexForRule(name);
        }
        catch(...)
        {
            // Rule does not exist in merged rules.
            // Add it.
            copyViewingRule(rules, i, mergedRules->getNumEntries(), mergedRules);
        }
    }
}

} // anon.

void DisplayViewMerger::addUniqueDisplays(const ConstConfigRcPtr & cfg)
{
    // For each display, add any views from cfg that are not already in the merged config.

    for (int i = 0; i < cfg->getNumDisplaysAll(); i++)
    {
        const char * dispName = cfg->getDisplayAll(i);

        // Display defined views.
        for (int v = 0; v < cfg->getNumViews(VIEW_DISPLAY_DEFINED, dispName); v++)
        {
            const char * displayDefinedView = cfg->getView(VIEW_DISPLAY_DEFINED, dispName, v);

            // This will return true if the display contains either a display-defined or
            // shared view with this name.
            const bool dispDefinedExists = displayHasView(m_mergedConfig, dispName, displayDefinedView);

            if (displayDefinedView && *displayDefinedView && !dispDefinedExists)
            {
                // (Note this works for either the new or old style of view.)
                m_mergedConfig->addDisplayView(dispName,
                                               displayDefinedView,
                                               cfg->getDisplayViewTransformName(dispName, displayDefinedView),
                                               cfg->getDisplayViewColorSpaceName(dispName, displayDefinedView),
                                               cfg->getDisplayViewLooks(dispName, displayDefinedView),
                                               cfg->getDisplayViewRule(dispName, displayDefinedView),
                                               cfg->getDisplayViewDescription(dispName, displayDefinedView));
            }
        }

        // Shared views.
        for (int v = 0; v < cfg->getNumViews(VIEW_SHARED, dispName); v++)
        {
            const char * sharedViewName = cfg->getView(VIEW_SHARED, dispName, v);

            const bool sharedViewExists = displayHasView(m_mergedConfig, dispName, sharedViewName);

            if (sharedViewName && *sharedViewName && !sharedViewExists)
            {
                m_mergedConfig->addDisplaySharedView(dispName, sharedViewName);
            }
        }
    }
}

void DisplayViewMerger::addUniqueVirtualViews(const ConstConfigRcPtr & cfg)
{
    // Display defined views.
    for (int v = 0; v < cfg->getVirtualDisplayNumViews(VIEW_DISPLAY_DEFINED); v++)
    {
        const char * displayDefinedView = cfg->getVirtualDisplayView(VIEW_DISPLAY_DEFINED, v);
        const bool dispDefinedExists = hasVirtualView(m_mergedConfig, displayDefinedView);
        if (displayDefinedView && *displayDefinedView && !dispDefinedExists)
        {
            m_mergedConfig->addVirtualDisplayView(displayDefinedView,
                                                  cfg->getVirtualDisplayViewTransformName(displayDefinedView),
                                                  cfg->getVirtualDisplayViewColorSpaceName(displayDefinedView),
                                                  cfg->getVirtualDisplayViewLooks(displayDefinedView),
                                                  cfg->getVirtualDisplayViewRule(displayDefinedView),
                                                  cfg->getVirtualDisplayViewDescription(displayDefinedView));
        }
    }

    // Shared views.
    for (int v = 0; v < cfg->getVirtualDisplayNumViews(VIEW_SHARED); v++)
    {
        const char * sharedViewName = cfg->getVirtualDisplayView(VIEW_SHARED, v);
        const bool sharedViewExists = hasVirtualView(m_mergedConfig, sharedViewName);
        if (sharedViewName && *sharedViewName && !sharedViewExists)
        {
            m_mergedConfig->addVirtualDisplaySharedView(sharedViewName);
        }
    }
}

void DisplayViewMerger::processDisplays(const ConstConfigRcPtr & first,
                                        const ConstConfigRcPtr & second,
                                        bool preferSecond)
{
    // Iterate over the first config's displays.

    for (int i = 0; i < first->getNumDisplaysAll(); i++)
    {
        const char * dispName = first->getDisplayAll(i);

        // Iterate over this display's display-defined views.

        for (int v = 0; v < first->getNumViews(VIEW_DISPLAY_DEFINED, dispName); v++)
        {
            const char * displayDefinedView = first->getView(VIEW_DISPLAY_DEFINED, dispName, v);

            if (displayDefinedView && *displayDefinedView)
            {
                // One case to be aware of is where both configs have the same display with the same
                // view name, but it's a display-defined view in one and a shared view in the other.
                // This check will return true if it exists in either form.
                const bool existsInSecond = displayHasView(second, dispName, displayDefinedView);

                if (existsInSecond && !viewsAreEqual(first, second, dispName, displayDefinedView))
                {
                    // Throw or log on conflict.
                    std::ostringstream os;
                    os << "The Input config contains a value that would override ";
                    os << "the Base config: display: " << dispName << ", view: " << displayDefinedView;
                    notify(os.str(), m_params->isErrorOnConflict());
                }

                // Display defined views.
                if (existsInSecond && preferSecond)
                {
                    // Take the view from the second config.

                    // This was a display-defined view in the first config but it may not be in
                    // the second config.  Want to add it as the same type of view.
                    if (viewIsShared(second, dispName, displayDefinedView))
                    {
                        m_mergedConfig->addDisplaySharedView(dispName, displayDefinedView);
                    }
                    else
                    {
                        m_mergedConfig->addDisplayView(dispName,
                                                       displayDefinedView,
                                                       second->getDisplayViewTransformName(dispName, displayDefinedView),
                                                       second->getDisplayViewColorSpaceName(dispName, displayDefinedView),
                                                       second->getDisplayViewLooks(dispName, displayDefinedView),
                                                       second->getDisplayViewRule(dispName, displayDefinedView),
                                                       second->getDisplayViewDescription(dispName, displayDefinedView));
                    }
                }
                else
                {
                    // Take the view from the first config (where it is display-defined).
                    // (Note this works for either the new or old style of view.)
                    m_mergedConfig->addDisplayView(dispName,
                                                   displayDefinedView,
                                                   first->getDisplayViewTransformName(dispName, displayDefinedView),
                                                   first->getDisplayViewColorSpaceName(dispName, displayDefinedView),
                                                   first->getDisplayViewLooks(dispName, displayDefinedView),
                                                   first->getDisplayViewRule(dispName, displayDefinedView),
                                                   first->getDisplayViewDescription(dispName, displayDefinedView));
                }
            }
        }

        // Iterate over this display's shared views.

        for (int v = 0; v < first->getNumViews(VIEW_SHARED, dispName); v++)
        {
            const char * sharedViewName = first->getView(VIEW_SHARED, dispName, v);

            if (sharedViewName && *sharedViewName)
            {
                const bool existsInSecond = displayHasView(second, dispName, sharedViewName);

                if (existsInSecond && preferSecond)
                {
                    // This was a shared view in the first config but it may not be in
                    // the second config.  Want to add it as the same type of view.
                    if (viewIsShared(second, dispName, sharedViewName))
                    {
                        m_mergedConfig->addDisplaySharedView(dispName, sharedViewName);
                    }
                    else
                    {
                        if (!viewsAreEqual(first, second, dispName, sharedViewName))
                        {
                            // Throw or log on conflict.
                            std::ostringstream os;
                            os << "The Input config contains a value that would override ";
                            os << "the Base config: display: " << dispName << ", view: " << sharedViewName;
                            notify(os.str(), m_params->isErrorOnConflict());
                        }
                        m_mergedConfig->addDisplayView(dispName,
                                                       sharedViewName,
                                                       second->getDisplayViewTransformName(dispName, sharedViewName),
                                                       second->getDisplayViewColorSpaceName(dispName, sharedViewName),
                                                       second->getDisplayViewLooks(dispName, sharedViewName),
                                                       second->getDisplayViewRule(dispName, sharedViewName),
                                                       second->getDisplayViewDescription(dispName, sharedViewName));
                    }
                }
                else
                {
                    // Note: The error-on-conflict check happens in processSharedViews,
                    // this is just adding the reference, so it's not checked again here.
                    m_mergedConfig->addDisplaySharedView(dispName, sharedViewName);
                }
            }
        }
    }

    // Add the remaining views for all displays from the second config. (This only adds views
    // that are not already present.)

    addUniqueDisplays(second);
}

void DisplayViewMerger::processVirtualDisplay(const ConstConfigRcPtr & first,
                                              const ConstConfigRcPtr & second,
                                              bool preferSecond)
{
    for (int v = 0; v < first->getVirtualDisplayNumViews(VIEW_DISPLAY_DEFINED); v++)
    {
        const char * displayDefinedView = first->getVirtualDisplayView(VIEW_DISPLAY_DEFINED, v);

        if (displayDefinedView && *displayDefinedView)
        {
            // Check if it displays shared view exists in second config.
            const bool existsInSecond = hasVirtualView(second, displayDefinedView);

            if (existsInSecond && !virtualViewsAreEqual(first, second, displayDefinedView))
            {
                // Throw or log on conflict.
                std::ostringstream os;
                os << "The Input config contains a value that would override ";
                os << "the Base config: virtual_display: " << displayDefinedView;
                notify(os.str(), m_params->isErrorOnConflict());
            }

            // Display defined views.
            if (existsInSecond && preferSecond)
            {
                // Take the view from the second config.

                // This was a display-defined view in the first config but it may not be in
                // the second config.  Want to add it as the same type of view.
                if (virtualViewIsShared(second, displayDefinedView))
                {
                    m_mergedConfig->addVirtualDisplaySharedView(displayDefinedView);
                }
                else
                {
                    m_mergedConfig->addVirtualDisplayView(displayDefinedView,
                                                          second->getVirtualDisplayViewTransformName(displayDefinedView),
                                                          second->getVirtualDisplayViewColorSpaceName(displayDefinedView),
                                                          second->getVirtualDisplayViewLooks(displayDefinedView),
                                                          second->getVirtualDisplayViewRule(displayDefinedView),
                                                          second->getVirtualDisplayViewDescription(displayDefinedView));
                }
            }
            else
            {
                // Take the view from the first config (where it is display-defined).
                // (Note this works for either the new or old style of view.)
                m_mergedConfig->addVirtualDisplayView(displayDefinedView,
                                                      first->getVirtualDisplayViewTransformName(displayDefinedView),
                                                      first->getVirtualDisplayViewColorSpaceName(displayDefinedView),
                                                      first->getVirtualDisplayViewLooks(displayDefinedView),
                                                      first->getVirtualDisplayViewRule(displayDefinedView),
                                                      first->getVirtualDisplayViewDescription(displayDefinedView));
            }
        }
    }

    // Iterate over this display's shared views.

    for (int v = 0; v < first->getVirtualDisplayNumViews(VIEW_SHARED); v++)
    {
        const char * sharedViewName = first->getVirtualDisplayView(VIEW_SHARED, v);

        if (sharedViewName && *sharedViewName)
        {
            const bool existsInSecond = hasVirtualView(second, sharedViewName);

            if (existsInSecond && preferSecond)
            {
                // This was a shared view in the first config but it may not be in
                // the second config.  Want to add it as the same type of view.
                if (virtualViewIsShared(second, sharedViewName))
                {
                    m_mergedConfig->addVirtualDisplaySharedView(sharedViewName);
                }
                else
                {
                    if (!virtualViewsAreEqual(first, second, sharedViewName))
                    {
                        // Throw or log on conflict.
                        std::ostringstream os;
                        os << "The Input config contains a value that would override ";
                        os << "the Base config: virtual_display: " << sharedViewName;
                        notify(os.str(), m_params->isErrorOnConflict());
                    }
                    m_mergedConfig->addVirtualDisplayView(sharedViewName,
                                                          second->getVirtualDisplayViewTransformName(sharedViewName),
                                                          second->getVirtualDisplayViewColorSpaceName(sharedViewName),
                                                          second->getVirtualDisplayViewLooks(sharedViewName),
                                                          second->getVirtualDisplayViewRule(sharedViewName),
                                                          second->getVirtualDisplayViewDescription(sharedViewName));
                }
            }
            else
            {
                // Note: The error-on-conflict check happens in processSharedViews,
                // this is just adding the reference, so it's not checked again here.
                m_mergedConfig->addVirtualDisplaySharedView(sharedViewName);
            }
        }
    }

    // Add the remaining views from the second config.

    addUniqueVirtualViews(second);
}

void DisplayViewMerger::addUniqueSharedViews(const ConstConfigRcPtr & cfg)
{
    // Add any shared views that are not already in the merged config.

    for (int v = 0; v < cfg->getNumViews(VIEW_SHARED, nullptr); v++)
    {
        const char * sharedViewName = cfg->getView(VIEW_SHARED, nullptr, v);

        // Check if shared view exists in merged config.
        bool sharedViewExists = displayHasView(m_mergedConfig, nullptr, sharedViewName);

        if (sharedViewName && *sharedViewName && !sharedViewExists)
        {
            m_mergedConfig->addSharedView(sharedViewName,
                                          cfg->getDisplayViewTransformName(nullptr, sharedViewName),
                                          cfg->getDisplayViewColorSpaceName(nullptr, sharedViewName),
                                          cfg->getDisplayViewLooks(nullptr, sharedViewName),
                                          cfg->getDisplayViewRule(nullptr, sharedViewName),
                                          cfg->getDisplayViewDescription(nullptr, sharedViewName));
        }
    }
}

void DisplayViewMerger::processSharedViews(const ConstConfigRcPtr & first,
                                           const ConstConfigRcPtr & second,
                                           bool preferSecond)
{
    // Iterate over all shared views in the first config.
    for (int v = 0; v < first->getNumViews(VIEW_SHARED, nullptr); v++)
    {
        const char * sharedViewName = first->getView(VIEW_SHARED, nullptr, v);

        if (sharedViewName && *sharedViewName)
        {
            // Check if shared view exists in second config.
            bool existsInSecond = displayHasView(second, nullptr, sharedViewName);

            if (existsInSecond && !viewsAreEqual(first, second, nullptr, sharedViewName))
            {
                // Throw or log on conflict.
                std::ostringstream os;
                os << "The Input config contains a value that would override ";
                os << "the Base config: shared_views: " << sharedViewName;
                notify(os.str(), m_params->isErrorOnConflict());
            }

            if (existsInSecond && preferSecond)
            {
                // Take the shared view from the second config.
                // (Note this works for either the new or old style of view.)
                m_mergedConfig->addSharedView(sharedViewName,
                                              second->getDisplayViewTransformName(nullptr, sharedViewName),
                                              second->getDisplayViewColorSpaceName(nullptr, sharedViewName),
                                              second->getDisplayViewLooks(nullptr, sharedViewName),
                                              second->getDisplayViewRule(nullptr, sharedViewName),
                                              second->getDisplayViewDescription(nullptr, sharedViewName));
            }
            else
            {
                // Take the shared view from the first config.
                m_mergedConfig->addSharedView(sharedViewName,
                                              first->getDisplayViewTransformName(nullptr, sharedViewName),
                                              first->getDisplayViewColorSpaceName(nullptr, sharedViewName),
                                              first->getDisplayViewLooks(nullptr, sharedViewName),
                                              first->getDisplayViewRule(nullptr, sharedViewName),
                                              first->getDisplayViewDescription(nullptr, sharedViewName));
            }
        }
    }

    // Add the remaining shared views that are only in the second config.

    addUniqueSharedViews(second);
}

void DisplayViewMerger::processActiveLists()
{
    // Merge active_displays.
    const char * activeDisplays = m_params->getActiveDisplays();
    if (activeDisplays && *activeDisplays)
    {
        // Take active_displays from overrides.
        m_mergedConfig->setActiveDisplays(activeDisplays);
    }
    else
    {
        // Take active_displays from config.

        StringUtils::StringVec mergedActiveDisplays;
        if (m_params->isInputFirst())
        {
            StringUtils::StringVec baseActiveDisplays;
            splitActiveList(m_baseConfig->getActiveDisplays(), baseActiveDisplays);
            splitActiveList(m_inputConfig->getActiveDisplays(), mergedActiveDisplays);

            mergeStringsWithoutDuplicates(baseActiveDisplays, mergedActiveDisplays);
        }
        else
        {
            StringUtils::StringVec inputActiveDisplays;
            splitActiveList(m_inputConfig->getActiveDisplays(), inputActiveDisplays);
            splitActiveList(m_baseConfig->getActiveDisplays(), mergedActiveDisplays);

            mergeStringsWithoutDuplicates(inputActiveDisplays, mergedActiveDisplays);
        }

        // NB: StringUtils::Join adds a space after the comma.
        m_mergedConfig->setActiveDisplays(StringUtils::Join(mergedActiveDisplays, ',').c_str());
    }

    // Merge active_views.
    const char * activeViews = m_params->getActiveViews();
    if (activeViews && *activeViews)
    {
        // Take active_views from overrides.
        m_mergedConfig->setActiveViews(activeViews);
    }
    else
    {
        // Take active_views from config.

        StringUtils::StringVec mergedActiveViews;
        if (m_params->isInputFirst())
        {
            StringUtils::StringVec baseActiveViews;
            splitActiveList(m_baseConfig->getActiveViews(), baseActiveViews);
            splitActiveList(m_inputConfig->getActiveViews(), mergedActiveViews);

            mergeStringsWithoutDuplicates(baseActiveViews, mergedActiveViews);
        }
        else
        {
            StringUtils::StringVec inputActiveViews;
            splitActiveList(m_inputConfig->getActiveViews(), inputActiveViews);
            splitActiveList(m_baseConfig->getActiveViews(), mergedActiveViews);

            mergeStringsWithoutDuplicates(inputActiveViews, mergedActiveViews);
        }

        m_mergedConfig->setActiveViews(StringUtils::Join(mergedActiveViews, ',').c_str());
    }
}

void DisplayViewMerger::addUniqueViewTransforms(const ConstConfigRcPtr & cfg)
{
    for (int i = 0; i < cfg->getNumViewTransforms(); i++)
    {
        const char * name = cfg->getViewTransformNameByIndex(i);
        // Take the view from the config if it does not exist in merged config.
        if (m_mergedConfig->getViewTransform(name) == nullptr)
        {
            m_mergedConfig->addViewTransform(cfg->getViewTransform(name));
        }
    }
}

void DisplayViewMerger::processViewTransforms(const ConstConfigRcPtr & first,
                                              const ConstConfigRcPtr & second,
                                              bool preferSecond)
{
    for (int i = 0; i < first->getNumViewTransforms(); i++)
    {
        const char * name = first->getViewTransformNameByIndex(i);
        if (name && *name)
        {
            ConstViewTransformRcPtr vt2 = second->getViewTransform(name);
            if (vt2 && !viewTransformsAreEqual(first, second, name))
            {
                std::ostringstream os;
                os << "The Input config contains a value that would override ";
                os << "the Base config: view_transforms: " << name;
                notify(os.str(), m_params->isErrorOnConflict());
            }

            if (vt2 && preferSecond)
            {
                m_mergedConfig->addViewTransform(second->getViewTransform(name));
            }
            else
            {
                m_mergedConfig->addViewTransform(first->getViewTransform(name));
            }
        }
    }

    // Add the remaining unique views transform.

    addUniqueViewTransforms(second);
}

void DisplayViewMerger::processViewingRules(const ConstConfigRcPtr & first,
                                            const ConstConfigRcPtr & second,
                                            bool preferSecond) const
{
    ViewingRulesRcPtr mergedRules = ViewingRules::Create();

    ConstViewingRulesRcPtr firstRules = first->getViewingRules();
    ConstViewingRulesRcPtr secondRules = second->getViewingRules();

    for (size_t i = 0; i < firstRules->getNumEntries(); i++)
    {
        bool hasConflict = false;

        const char * name = firstRules->getName(i);
        try
        {
            // Check if it exists in second rules, will throw if not.
            size_t idx = secondRules->getIndexForRule(name);

            if (!viewingRulesAreEqual(firstRules, i, secondRules, idx))
            {
                hasConflict = true;

                if (preferSecond)
                {
                    // Take rule from the second config.
                    copyViewingRule(secondRules, idx, mergedRules->getNumEntries(), mergedRules);
                }
                else
                {
                    // Found, but not overriding. Take rule from the first config.
                    copyViewingRule(firstRules, i, mergedRules->getNumEntries(), mergedRules);
                }

                std::ostringstream os;
                os << "The Input config contains a value that would override ";
                os << "the Base config: viewing_rules: " << name;
                throw Exception(os.str().c_str());
            }

        }
        catch(const Exception & e)
        {
            if (hasConflict)
            {
                notify(e.what(), m_params->isErrorOnConflict());
            }
            else
            {
                // Not found in second rules. Take rule from the first config.
                copyViewingRule(firstRules, i, mergedRules->getNumEntries(), mergedRules);
            }
        }
    }

    // Add the remaining rules.
    addUniqueViewingRules(secondRules, mergedRules);

    m_mergedConfig->setViewingRules(mergedRules);
}

void DisplayViewMerger::handlePreferInput()
{
    // The error_on_conflict option implies to shared_views, displays/views, virtual_display,
    // view_transforms, default_view_transform, and viewing_rules.

    // Clear displays and shared_views from merged config.
    m_mergedConfig->clearDisplays();
    clearSharedViews(m_mergedConfig);

    // Merge displays and views.
    // The order is important: shared_views, and then displays.
    if (m_params->isInputFirst())
    {
        processSharedViews(m_inputConfig, m_baseConfig, false);
        processDisplays(m_inputConfig, m_baseConfig, false);
    }
    else
    {
        processSharedViews(m_baseConfig, m_inputConfig, true);
        processDisplays(m_baseConfig, m_inputConfig, true);
    }

    // Merge virtual_display.
    m_mergedConfig->clearVirtualDisplay();
    if (m_params->isInputFirst())
    {
        processVirtualDisplay(m_inputConfig, m_baseConfig, false);
    }
    else
    {
        processVirtualDisplay(m_baseConfig, m_inputConfig, true);
    }

    // Merge active_displays and active_views.
    processActiveLists();

    // Merge view_transforms.
    m_mergedConfig->clearViewTransforms();
    if (m_params->isInputFirst())
    {
        processViewTransforms(m_inputConfig, m_baseConfig, false);
    }
    else
    {
        processViewTransforms(m_baseConfig, m_inputConfig, true);
    }

    // Merge default_view_transform.
    const char * baseName = m_baseConfig->getDefaultViewTransformName();
    const char * inputName = m_inputConfig->getDefaultViewTransformName();
    if (!(Platform::Strcasecmp(baseName, inputName) == 0))
    {
        notify("The Input config contains a value that would override the Base config: "\
               "default_view_transform: " + std::string(inputName), m_params->isErrorOnConflict());
    }
    // If the input config does not specify a default, keep the one from the base.
    if (inputName && *inputName)
    {
        m_mergedConfig->setDefaultViewTransformName(inputName);
    }

    // Merge viewing_rules.
    if (m_params->isInputFirst())
    {
        processViewingRules(m_inputConfig, m_baseConfig, false);
    }
    else
    {
        processViewingRules(m_baseConfig, m_inputConfig, true);
    }
}

void DisplayViewMerger::handlePreferBase()
{
    // Clear displays and shared_views from merged config.
    m_mergedConfig->clearDisplays();
    clearSharedViews(m_mergedConfig);

    // Merge displays and views.
    // The order is important: shared_views, and then displays.
    if (m_params->isInputFirst())
    {
        processSharedViews(m_inputConfig, m_baseConfig, true);
        processDisplays(m_inputConfig, m_baseConfig, true);
    }
    else
    {
        processSharedViews(m_baseConfig, m_inputConfig, false);
        processDisplays(m_baseConfig, m_inputConfig, false);
    }

    // Merge virtual_display.
    m_mergedConfig->clearVirtualDisplay();
    if (m_params->isInputFirst())
    {
        processVirtualDisplay(m_inputConfig, m_baseConfig, true);
    }
    else
    {
        processVirtualDisplay(m_baseConfig, m_inputConfig, false);
    }

    // Merge active_displays and active_views.
    processActiveLists();

    // Merge view_transforms.
    m_mergedConfig->clearViewTransforms();
    if (m_params->isInputFirst())
    {
        processViewTransforms(m_inputConfig, m_baseConfig, true);
    }
    else
    {
        processViewTransforms(m_baseConfig, m_inputConfig, false);
    }

    // Merge default_view_transform.
    const char * baseName = m_baseConfig->getDefaultViewTransformName();
    const char * inputName = m_inputConfig->getDefaultViewTransformName();
    if (!(Platform::Strcasecmp(baseName, inputName) == 0))
    {
        notify("The Input config contains a value that would override the Base config: "\
               "default_view_transform: " + std::string(inputName), m_params->isErrorOnConflict());
    }
    // Only use the input if the base is missing.
    if (!baseName || !*baseName)
    {
        m_mergedConfig->setDefaultViewTransformName(inputName);
    }

    // Merge viewing_rules.
    if (m_params->isInputFirst())
    {
        processViewingRules(m_inputConfig, m_baseConfig, true);
    }
    else
    {
        processViewingRules(m_baseConfig, m_inputConfig, false);
    }
}

void DisplayViewMerger::handleInputOnly()
{
    // Clear displays and shared_views from merged config.
    m_mergedConfig->clearDisplays();
    clearSharedViews(m_mergedConfig);

    // Merge displays and views.
    addUniqueSharedViews(m_inputConfig);
    addUniqueDisplays(m_inputConfig);

    // Merge virtual_display.
    m_mergedConfig->clearVirtualDisplay();
    addUniqueVirtualViews(m_inputConfig);

    // Merge active_displays.
    const char * activeDisplays = m_params->getActiveDisplays();
    if (activeDisplays && *activeDisplays)
    {
        // Take active_displays from overrides.
        m_mergedConfig->setActiveDisplays(activeDisplays);
    }
    else
    {
        // Take active_displays from config.
        m_mergedConfig->setActiveDisplays(m_inputConfig->getActiveDisplays());
    }

    // Merge active_views.
    const char * activeViews = m_params->getActiveViews();
    if (activeViews && *activeViews)
    {
        // Take active_views from overrides.
        m_mergedConfig->setActiveViews(activeViews);
    }
    else
    {
        // Take active_views from config.
        m_mergedConfig->setActiveViews(m_inputConfig->getActiveViews());
    }

    // Merge view_transforms.
    m_mergedConfig->clearViewTransforms();
    addUniqueViewTransforms(m_inputConfig);

    // Merge default_view_transform.
    m_mergedConfig->setDefaultViewTransformName(m_inputConfig->getDefaultViewTransformName());

    // Merge viewing_rules.
    m_mergedConfig->setViewingRules(m_inputConfig->getViewingRules());
}

void DisplayViewMerger::handleBaseOnly()
{
    // Process the overrides only since the merged config is initialized to
    // the base config.

    const char * activeDisplays = m_params->getActiveDisplays();
    if (activeDisplays && *activeDisplays)
    {
        // Take active_displays from overrides.
        m_mergedConfig->setActiveDisplays(activeDisplays);
    }

    const char * activeViews = m_params->getActiveViews();
    if (activeViews && *activeViews)
    {
        // Take active_views from overrides.
        m_mergedConfig->setActiveViews(activeViews);
    }
}

void DisplayViewMerger::handleRemove()
{
    // Remove shared_views.

    clearSharedViews(m_mergedConfig);

    for (int v = 0; v < m_baseConfig->getNumViews(VIEW_SHARED, nullptr); v++)
    {
        // Add shared views that are present in the base config and NOT present in the input config.
        const char * sharedViewName = m_baseConfig->getView(VIEW_SHARED, nullptr, v);
        if (sharedViewName && *sharedViewName
            && !displayHasView(m_inputConfig, nullptr, sharedViewName))
        {
            m_mergedConfig->addSharedView(sharedViewName,
                                          m_baseConfig->getDisplayViewTransformName(nullptr, sharedViewName),
                                          m_baseConfig->getDisplayViewColorSpaceName(nullptr, sharedViewName),
                                          m_baseConfig->getDisplayViewLooks(nullptr, sharedViewName),
                                          m_baseConfig->getDisplayViewRule(nullptr, sharedViewName),
                                          m_baseConfig->getDisplayViewDescription(nullptr, sharedViewName));
        }
    }

    // Remove views from displays.

    m_mergedConfig->clearDisplays();

    for (int i = 0; i < m_baseConfig->getNumDisplaysAll(); i++)
    {
        // Add views that are present in the base config and NOT present in the input config.

        // Display-defined views.
        const char * dispName = m_baseConfig->getDisplayAll(i);
        for (int v = 0; v < m_baseConfig->getNumViews(VIEW_DISPLAY_DEFINED, dispName); v++)
        {
            const char * displayDefinedView = m_baseConfig->getView(VIEW_DISPLAY_DEFINED, dispName, v);
            // Check if the view is not present in the input config.
            if (displayDefinedView && *displayDefinedView
                && !displayHasView(m_inputConfig, dispName, displayDefinedView))
            {
                m_mergedConfig->addDisplayView(dispName,
                                               displayDefinedView,
                                               m_baseConfig->getDisplayViewTransformName(dispName, displayDefinedView),
                                               m_baseConfig->getDisplayViewColorSpaceName(dispName, displayDefinedView),
                                               m_baseConfig->getDisplayViewLooks(dispName, displayDefinedView),
                                               m_baseConfig->getDisplayViewRule(dispName, displayDefinedView),
                                               m_baseConfig->getDisplayViewDescription(dispName, displayDefinedView));
            }
        }

        // Shared views.
        for (int v = 0; v < m_baseConfig->getNumViews(VIEW_SHARED, dispName); v++)
        {
            const char * sharedViewName = m_baseConfig->getView(VIEW_SHARED, dispName, v);
            // Check if the view is not present in the input config.
            if (sharedViewName && *sharedViewName
                && !displayHasView(m_inputConfig, dispName, sharedViewName))
            {
                m_mergedConfig->addDisplaySharedView(dispName, sharedViewName);
            }
        }
    }

    // Remove views from virtual_display.

    m_mergedConfig->clearVirtualDisplay();

    {
        // Add virtual views that are present in the base config and NOT present in the input config.

        // Display-defined views.
        for (int v = 0; v < m_baseConfig->getVirtualDisplayNumViews(VIEW_DISPLAY_DEFINED); v++)
        {
            const char * displayDefinedView = m_baseConfig->getVirtualDisplayView(VIEW_DISPLAY_DEFINED, v);
            // Check if the view is not present in the input config.
            if (displayDefinedView && *displayDefinedView
                && !hasVirtualView(m_inputConfig, displayDefinedView))
            {
                // Add the display defined view
                m_mergedConfig->addVirtualDisplayView(displayDefinedView,
                                                      m_baseConfig->getVirtualDisplayViewTransformName(displayDefinedView),
                                                      m_baseConfig->getVirtualDisplayViewColorSpaceName(displayDefinedView),
                                                      m_baseConfig->getVirtualDisplayViewLooks(displayDefinedView),
                                                      m_baseConfig->getVirtualDisplayViewRule(displayDefinedView),
                                                      m_baseConfig->getVirtualDisplayViewDescription(displayDefinedView));
            }
        }

        // Shared views.
        for (int v = 0; v < m_baseConfig->getVirtualDisplayNumViews(VIEW_SHARED); v++)
        {
            const char * sharedViewName = m_baseConfig->getVirtualDisplayView(VIEW_SHARED, v);
            // Check if the view is not present in the input config.
            if (sharedViewName && *sharedViewName
                && !hasVirtualView(m_inputConfig, sharedViewName))
            {
                // Add the shared view
                m_mergedConfig->addVirtualDisplaySharedView(sharedViewName);
            }
        }
    }

    // Remove from active_displays.

    StringUtils::StringVec inputActiveDisplays, mergedActiveDisplays;
    splitActiveList(m_inputConfig->getActiveDisplays(), inputActiveDisplays);
    splitActiveList(m_baseConfig->getActiveDisplays(), mergedActiveDisplays);

    for (const auto & disp : inputActiveDisplays)
    {
        StringUtils::Remove(mergedActiveDisplays, disp);
    }
    m_mergedConfig->setActiveDisplays(StringUtils::Join(mergedActiveDisplays, ',').c_str());

    // Remove from active_views.

    StringUtils::StringVec inputActiveViews, mergedActiveViews;
    splitActiveList(m_inputConfig->getActiveViews(), inputActiveViews);
    splitActiveList(m_baseConfig->getActiveViews(), mergedActiveViews);

    for (const auto & view : inputActiveViews)
    {
        StringUtils::Remove(mergedActiveViews, view);
    }
    m_mergedConfig->setActiveViews(StringUtils::Join(mergedActiveViews, ',').c_str());

    // Remove from view_transforms.
    m_mergedConfig->clearViewTransforms();
    // Add view transforms that are present in the base config and NOT present in the input config.
    for (int i = 0; i < m_baseConfig->getNumViewTransforms(); i++)
    {
        const char * name = m_baseConfig->getViewTransformNameByIndex(i);
        if (m_inputConfig->getViewTransform(name) == nullptr)
        {
            m_mergedConfig->addViewTransform(m_baseConfig->getViewTransform(name));
        }
    }

    // Handle default_view_transform.
    // Leave the base alone unless it identified a view transform that was removed.
    const char * baseName = m_baseConfig->getDefaultViewTransformName();
    if (m_mergedConfig->getViewTransform(baseName) == nullptr)
    {
        // Set to empty string, the first view transform will be used by default.
        m_mergedConfig->setDefaultViewTransformName("");
    }

    // Handle viewing_rules.

    ViewingRulesRcPtr mergedRules = ViewingRules::Create();
    ConstViewingRulesRcPtr inputRules = m_inputConfig->getViewingRules();
    ConstViewingRulesRcPtr baseRules = m_baseConfig->getViewingRules();

    for (size_t i = 0; i < baseRules->getNumEntries(); i++)
    {
        const char * name = baseRules->getName(i);
        try
        {
            // Throws if the input doesn't have the base rule.
            inputRules->getIndexForRule(name);
        }
        catch(...)
        {
            // Keep any base rules that aren't in the input.
            copyViewingRule(baseRules, i, mergedRules->getNumEntries(), mergedRules);
        }
    }

    m_mergedConfig->setViewingRules(mergedRules);
}

/////////////////////////////////// DisplayViewMerger end ////////////////////////////////


////////////////////////////////////////// LooksMerger ////////////////////////////////////////////
void LooksMerger::handlePreferInput()
{
    m_mergedConfig->clearLooks();

    if (m_params->isInputFirst())
    {
        // Add the Looks from the input config.
        for(int i = 0; i < m_inputConfig->getNumLooks(); ++i)
        {
            m_mergedConfig->addLook(m_inputConfig->getLook(m_inputConfig->getLookNameByIndex(i)));
        }

        // Add the Looks from the base config if it does not exist in the merged config.
        for(int i = 0; i < m_baseConfig->getNumLooks(); ++i)
        {
            if (m_mergedConfig->getLook(m_baseConfig->getLookNameByIndex(i)) == nullptr)
            {
                m_mergedConfig->addLook(m_baseConfig->getLook(m_baseConfig->getLookNameByIndex(i)));
            }
        }
    }
    else
    {
        // PreferInput, InputFirst = false
        // Add the looks from the base config.
        for(int i = 0; i < m_baseConfig->getNumLooks(); ++i)
        {
            m_mergedConfig->addLook(m_baseConfig->getLook(m_baseConfig->getLookNameByIndex(i)));
        }

        // Add the Looks from the input config and overwrite look if the name is the same.
        for(int i = 0; i < m_inputConfig->getNumLooks(); ++i)
        {
            m_mergedConfig->addLook(m_inputConfig->getLook(m_inputConfig->getLookNameByIndex(i)));
        }
    }
}

void LooksMerger::handlePreferBase()
{
    m_mergedConfig->clearLooks();

    if (m_params->isInputFirst())
    {
        // Add the Looks from the input config.
        for(int i = 0; i < m_inputConfig->getNumLooks(); ++i)
        {
            m_mergedConfig->addLook(m_inputConfig->getLook(m_inputConfig->getLookNameByIndex(i)));
        }

        // Add the Looks from the input config and overwrite look if the name is the same.
        for(int i = 0; i < m_baseConfig->getNumLooks(); ++i)
        {
            m_mergedConfig->addLook(m_baseConfig->getLook(m_baseConfig->getLookNameByIndex(i)));
        }
    }
    else
    {
        // PreferBase, InputFirst = false
        // Add the looks from the base config.
        for(int i = 0; i < m_baseConfig->getNumLooks(); ++i)
        {
            m_mergedConfig->addLook(m_baseConfig->getLook(m_baseConfig->getLookNameByIndex(i)));
        }

        // Add the Looks from the input config if it does not exist in the merged config.
        for(int i = 0; i < m_inputConfig->getNumLooks(); ++i)
        {
            if (m_mergedConfig->getLook(m_inputConfig->getLookNameByIndex(i)) == nullptr)
            {
                m_mergedConfig->addLook(m_inputConfig->getLook(m_inputConfig->getLookNameByIndex(i)));
            }
        }
    }
}

void LooksMerger::handleInputOnly()
{
    m_mergedConfig->clearLooks();
    // Add the Looks from the input config.
    for(int i = 0; i < m_inputConfig->getNumLooks(); ++i)
    {
        m_mergedConfig->addLook(m_inputConfig->getLook(m_inputConfig->getLookNameByIndex(i)));
    }
}

void LooksMerger::handleBaseOnly()
{
    // Supported, but nothing to do.
}

void LooksMerger::handleRemove()
{
    m_mergedConfig->clearLooks();
    // Add the looks from the base config if they do not exist in the INPUT config.
    for(int i = 0; i < m_baseConfig->getNumLooks(); ++i)
    {
        const char * baseName = m_baseConfig->getLookNameByIndex(i);
        if (m_inputConfig->getLook(baseName) == nullptr)
        {
            m_mergedConfig->addLook(m_baseConfig->getLook(baseName));
        }
    }
}

////////////////////////////////////// LooksMerger end ////////////////////////////////////////////


/////////////////////////////////////// ColorspacesMerger /////////////////////////////////////////

/*
void ColorspacesMerger::handleErrorCodes(ColorSpaceRcPtr & eColorspace)
{
            switch (errorCode)
            {
                case ADD_CS_ERROR_NAME_IDENTICAL_TO_A_ROLE_NAME:
                    skipCurrentColorspace = handleAddCsErrorNameIdenticalToARoleName(eColorspace);
                break;

                case ADD_CS_ERROR_NAME_IDENTICAL_TO_NT_NAME_OR_ALIAS:
                    skipCurrentColorspace = handleAddCsErrorNameIdenticalToNTNameOrAlias(eColorspace);
                break;

                case ADD_CS_ERROR_NAME_CONTAIN_CTX_VAR_TOKEN:
                    // Not handled as it can't happen in this context.
                    // The config will error out while loading the config (before the merge process).
                break;

                case ADD_CS_ERROR_NAME_IDENTICAL_TO_EXISTING_COLORSPACE_ALIAS:
                    skipCurrentColorspace = handleAddCsErrorNameIdenticalToExistingColorspaceAlias(eColorspace);
                break;

                case ADD_CS_ERROR_ALIAS_IDENTICAL_TO_A_ROLE_NAME:
                    skipCurrentColorspace = handleAddCsErrorAliasIdenticalToARoleName(eColorspace);
                break;

                case ADD_CS_ERROR_ALIAS_IDENTICAL_TO_NT_NAME_OR_ALIAS:
                    skipCurrentColorspace = handleAddCsErrorAliasIdenticalToNTNameOrAlias(eColorspace);
                break;

                case ADD_CS_ERROR_ALIAS_CONTAIN_CTX_VAR_TOKEN:
                    // Not handled as it can't happen in this context.
                    // The config will error out while loading the config (before the merge process).
                break;

                case ADD_CS_ERROR_ALIAS_IDENTICAL_TO_EXISTING_COLORSPACE_NAME:
                    skipCurrentColorspace = handleAddCsErrorAliasIdenticalToExistingColorspaceName(eColorspace);
                break;

                case ADD_CS_ERROR_ALIAS_IDENTICAL_TO_EXISTING_COLORSPACE_ALIAS:
                    skipCurrentColorspace = handleAddCsErrorAliasIdenticalToExistingColorspaceAlias(eColorspace);
                break;

                case ADD_CS_ERROR_NONE:
                    allErrorsResolved = true;
                break;

                case ADD_CS_ERROR_EMPTY:
                    // Nothing to do.
                break;

                default:
                break;
            }
}
*/

//TODO We need to decide if we want to do this or it is the responsibility of the person
//     doing the merge.
/*
void ColorspacesMerger::handleMarkedToBeDeletedColorspaces()
{
    // Go through the marked colorspaces, remove them and replace their name.
    // colorspace %duplicate% should be deleted and the named replaced everywhere it is used.


    if (m_colorspaceMarkedToBeDeleted.size() > 0)
    {
        // A color space name may appear in the following places:
        // ColorSpaceTransforms (src, dst)
        // DisplayViewTransforms (src)
        // Look (process_space)
        // View (colorspace, display_colorspace)
        // inactive_colorspaces,
        // file_rules (colorspace),
        // viewing_rules (colorspaces list)

        // roles
        // for (int r = 0; r < m_mergedConfig->getNumRoles(); r++)
        // {
        //     auto csName = m_mergedConfig->getRoleColorSpace(r);
        //     for (int i = 0; i < m_colorspaceMarkedToBeDeleted.size(); i++)
        //     {
        //         if (Platform::Strcasecmp(csName, m_colorspaceMarkedToBeDeleted[i].c_str()) == 0)
        //         {
        //             // Replace the name with the colorspace that we kept earlier.
        //             m_mergedConfig->setRole(m_mergedConfig->getRoleName(r),
        //                                     m_colorspaceReplacingMarkedCs[i].c_str());
        //         }
        //     }
        // }

        // // environment
        // for (int e = 0; e < m_mergedConfig->getNumEnvironmentVars(); e++)
        // {
        //     const char * name = m_mergedConfig->getEnvironmentVarNameByIndex(e);
        //     const char * value = m_mergedConfig->getEnvironmentVarDefault(name);
        //     for (int i = 0; i < m_colorspaceMarkedToBeDeleted.size(); i++)
        //     {
        //         if (Platform::Strcasecmp(value, m_colorspaceMarkedToBeDeleted[i].c_str()) == 0)
        //         {
        //             // Replace the name with the colorspace that we kept earlier.
        //             m_mergedConfig->addEnvironmentVar(name, m_colorspaceReplacingMarkedCs[i].c_str());
        //         }
        //     }
        // }

        m_colorspaceMarkedToBeDeleted.clear();
        m_colorspaceReplacingMarkedCs.clear();
    }
}

void ColorspacesMerger::replaceFamilySeparatorInFamily(ColorSpaceRcPtr & incomingCs)
{
    std::string family = incomingCs->getFamily();
    if (!family.empty())
    {
        char separator = '/';
        std::string updatedFamily = "";

        if (m_params->getColorspaces() == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT)
        {
            // Incoming colorspaces are from the base config.
            separator = m_baseConfig->getFamilySeparator();
        }
        else if (m_params->getColorspaces() == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_BASE)
        {
            // Incoming colorspaces are from the input config.
            separator = m_inputConfig->getFamilySeparator();
        }

        std::string separatorStr(1, separator);
        std::string mergedSeparatorStr(1, m_mergedConfig->getFamilySeparator());
        updatedFamily = StringUtils::Replace(incomingCs->getFamily(), separatorStr, mergedSeparatorStr);
        incomingCs->setFamily(updatedFamily.c_str());
    }
    // Do nothing if family is empty
}
*/

bool hasSearchPath(const ConstConfigRcPtr & cfg, const char * path)
{
    for (int i = 0; i < cfg->getNumSearchPaths(); i++)
    {
        if (Platform::Strcasecmp(cfg->getSearchPath(i), path) == 0)
        {
            return true;
        }
    }
    return false;
}

void ColorspacesMerger::processSearchPaths() const
{
    const char * searchPaths = m_params->getSearchPath();
    if (searchPaths && *searchPaths)
    {
        // Use the override.
        m_mergedConfig->setSearchPath(searchPaths);
        return;
    }

    if (m_params->isInputFirst())
    {
        m_mergedConfig->clearSearchPaths();
        // Add all from input config.
        for (int i = 0; i < m_inputConfig->getNumSearchPaths(); i++)
        {
            m_mergedConfig->addSearchPath(m_inputConfig->getSearchPath(i));
        }

        // Only add the new ones from the base config.
        for (int i = 0; i < m_baseConfig->getNumSearchPaths(); i++)
        {
            if (!hasSearchPath(m_inputConfig, m_baseConfig->getSearchPath(i)))
            {
                m_mergedConfig->addSearchPath(m_baseConfig->getSearchPath(i));
            }
        }
    }
    else
    {
        // NB: The m_mergecConfig is initialized with the contents of the m_baseConfig.

        for (int i = 0; i < m_inputConfig->getNumSearchPaths(); i++)
        {
            if (!hasSearchPath(m_baseConfig, m_inputConfig->getSearchPath(i)))
            {
                m_mergedConfig->addSearchPath(m_inputConfig->getSearchPath(i));
            }
        }
    }
}

void cleanUpInactiveList(ConfigRcPtr & mergeConfig)
{
    StringUtils::StringVec originalList, validList;
    splitActiveList(mergeConfig->getInactiveColorSpaces(), originalList);
    for (auto & item : originalList)
    {
        const std::string name = StringUtils::Trim(item);
        ConstColorSpaceRcPtr existingCS = mergeConfig->getColorSpace(name.c_str());
        ConstNamedTransformRcPtr existingNT = mergeConfig->getNamedTransform(name.c_str());

        if (existingCS)
        {
            // Don't want aliases in the inactive list.
            if (Platform::Strcasecmp(existingCS->getName(), name.c_str()) == 0)
            {
                validList.push_back(name);
            }
        }
        else if (existingNT)
        {
            if (Platform::Strcasecmp(existingNT->getName(), name.c_str()) == 0)
            {
                validList.push_back(name);
            }
        }
    }
    mergeConfig->setInactiveColorSpaces(StringUtils::Join(validList, ',').c_str());
}

std::string replaceSeparator(std::string str, char inSep, char outSep)
{
    std::string defaultSeparatorStr(1, inSep);
    std::string mergedSeparatorStr(1, outSep);
    std::string updatedStr = StringUtils::Replace(str, defaultSeparatorStr, mergedSeparatorStr);
    return updatedStr;
}

void ColorspacesMerger::updateFamily(std::string & family, bool fromBase) const
{
    // Note that if a prefix is present, it is always added, even if the CS did not have a family.

    std::string updatedPrefix = "";
    if (m_params->getColorspaces() == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT)
    {
        if (fromBase)
        {
            // If the color space is from the base config, need to update its family separator.
            if (!family.empty())
            {
                family = replaceSeparator(family, m_baseConfig->getFamilySeparator(), m_mergedConfig->getFamilySeparator());
            }
            // Note: The family prefix argument must always use the default slash separator.
            // TODO: Should do this just once in the initializer.
            updatedPrefix = replaceSeparator(m_params->getBaseFamilyPrefix(), '/', m_mergedConfig->getFamilySeparator());
        }
        else
        {
            updatedPrefix = replaceSeparator(m_params->getInputFamilyPrefix(), '/', m_mergedConfig->getFamilySeparator());
        }
    }
    else if (m_params->getColorspaces() == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_BASE)
    {
        if (fromBase)
        {
            // TODO: Should do this just once in the initializer.
            updatedPrefix = replaceSeparator(m_params->getBaseFamilyPrefix(), '/', m_mergedConfig->getFamilySeparator());
        }
        else
        {
            // If the color space is from the input config, need to update its family separator.
            if (!family.empty())
            {
                family = replaceSeparator(family, m_inputConfig->getFamilySeparator(), m_mergedConfig->getFamilySeparator());
            }
            updatedPrefix = replaceSeparator(m_params->getInputFamilyPrefix(), '/', m_mergedConfig->getFamilySeparator());
        }
    }
    // Append the prefix to the family.
    // Note that the prefix should end with a separator, if desired.  Not adding one here.
    family = updatedPrefix + family;
}

bool hasColorSpaceRefType(const ConstConfigRcPtr & config, ReferenceSpaceType refType)
{
    SearchReferenceSpaceType searchRefType = static_cast<SearchReferenceSpaceType>(refType);
    int n = config->getNumColorSpaces(searchRefType, COLORSPACE_ALL);
    return n > 0;
}

void ColorspacesMerger::initializeRefSpaceConverters(ConstTransformRcPtr & inputToBaseGtScene,
                                                     ConstTransformRcPtr & inputToBaseGtDisplay)
{
    // Note: The base config reference space is always used, regardless of strategy.

    if (!m_params->isAssumeCommonReferenceSpace())
    {
        if (hasColorSpaceRefType(m_inputConfig, REFERENCE_SPACE_SCENE))
        {
            try
            {
                inputToBaseGtScene = ConfigUtils::getRefSpaceConverter(
                    m_inputConfig,
                    m_baseConfig,
                    REFERENCE_SPACE_SCENE
                );
            }
            catch(const Exception & e)
            {
                LogError(e.what());
            }
        }

        // Only attempt to build the converter if the input config has this type of
        // reference space. Using the input config for this determination since it is
        // only input config color spaces whose reference space is converted.
        if (hasColorSpaceRefType(m_inputConfig, REFERENCE_SPACE_DISPLAY))
        {
            try
            {
                inputToBaseGtDisplay = ConfigUtils::getRefSpaceConverter(
                    m_inputConfig,
                    m_baseConfig,
                    REFERENCE_SPACE_DISPLAY
                );
            }
            catch(const Exception & e)
            {
                LogError(e.what());
            }
        }
    }
}

// TODO: Make this a functional inside where it's called from.
void ColorspacesMerger::attemptToAddAlias(const ConstConfigRcPtr & mergeConfig,
                                          ColorSpaceRcPtr & dupeCS,
                                          const ConstColorSpaceRcPtr & inputCS,
                                          const char * aliasName)
{
    // It is assumed that the base and input configs start out in a legal state,
    // however, when adding anything from one config to another, must always
    // check that it doesn't conflict with anything.

    // It is assumed that the strategy is prefer_base when this function is called.

    // It's OK if aliasName used in the duplicate color space itself.
    if ((Platform::Strcasecmp(dupeCS->getName(), aliasName) == 0)
        || hasAlias(dupeCS, aliasName))
    {
        // It's already present, no need to add anything.
        return;
    }

    // Check if aliasName is already a name that is used in the config.

    ConstColorSpaceRcPtr conflictingCS = mergeConfig->getColorSpace(aliasName);
    if (conflictingCS)
    {
        // The conflict could be the name of a color space, an alias, or a role.
        // But it doesn't matter, the strategy is prefer base so don't want to
        // remove this conflict from the base config to accomodate adding an
        // alias from the input config.
        std::ostringstream os;
        os << "Input color space '" << inputCS->getName() << "'";
        os << " is a duplicate of base color space '";
        os << dupeCS->getName() << "' but was unable to add alias '";
        os << aliasName << "' since it conflicts with base color space '";
        os << conflictingCS->getName() << "'.";
        notify(os.str(), m_params->isErrorOnConflict());

        return;
    }

    // No conflicts encountered, it's ok to update the alias and add it to the config.

    dupeCS->addAlias(aliasName);
}

bool ColorspacesMerger::handleAvoidDuplicatesOption(ConfigRcPtr & eBase, ColorSpaceRcPtr & inputCS)
{
    bool notDuplicate = true;

    if (!m_params->isAvoidDuplicates())
    {
        return notDuplicate;
    }

    // Note: The search for duplicate color spaces only searches for color spaces with
    // the same reference space type (i.e., scene or display), so it won't remove spaces
    // that are otherwise equivalent (e.g., an sRGB transform).
    //
    // However, some configs may intentionally have duplicate color spaces (e.g., aliases
    // from v1 configs). Since this search is only done using spaces from the input config,
    // those duplicates in the base won't be removed. But if the input config contains
    // duplicates, those will be condensed into one space containing aliases for all of
    // names of the duplicates.
    const char * duplicateInBase = ConfigUtils::findEquivalentColorspace(
        eBase,
        inputCS,
        inputCS->getReferenceSpaceType()
    );

    const ConfigMergingParameters::MergeStrategies strategy = m_params->getColorspaces();
    if (duplicateInBase && *duplicateInBase)
    {
        if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT)
        {
//                 m_colorspaceMarkedToBeDeleted.push_back(duplicateInBase);
//                 m_colorspaceReplacingMarkedCs.push_back(inputCS->getName());

            // Add the name and aliases from the duplicate colorspace to the input colorspace.
            //
            // Note that the aliases added here should not have conflicts with the base config
            // (since that's where they originated), but may cause conflicts with other color
            // spaces in the input config, but these will be handled as the spaces get added
            // to the merged config by the calling function.

            ConstColorSpaceRcPtr dupeCS = eBase->getColorSpace(duplicateInBase);
            if (dupeCS)
            {
                // Note that addAlias will check the argument and won't add it if it matches
                // the name of the color space or one of the existing aliases.
                inputCS->addAlias(dupeCS->getName());

                for (size_t i = 0; i < dupeCS->getNumAliases(); i++)
                {
                    inputCS->addAlias(dupeCS->getAlias(i));
                }

                eBase->removeColorSpace(duplicateInBase);
            }
        }
        else if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_BASE)
        {
            // Don't add the input color space, but add it's name and alias to the duplicate.
            // Need to be more careful of conflicts, since the modified color space is
            // receiving aliases from the input config and yet is not going through the
            // mergeColorSpace checking below.

            ConstColorSpaceRcPtr cs = eBase->getColorSpace(duplicateInBase);
            if (cs)
            {
                ColorSpaceRcPtr eCS = cs->createEditableCopy();

                attemptToAddAlias(eBase, eCS, inputCS, inputCS->getName());

                for (size_t i = 0; i < inputCS->getNumAliases(); i++)
                {
                    attemptToAddAlias(eBase, eCS, inputCS, inputCS->getAlias(i));
                }

                // Replace the color space in the merge config. (This preserves its
                // order in the color space list.)
                eBase->addColorSpace(eCS);

                notDuplicate = false;
            }
        }
    }
    return notDuplicate;
}


bool ColorspacesMerger::colorSpaceMayBeMerged(const ConstConfigRcPtr & mergeConfig, 
                                              const ConstColorSpaceRcPtr & inputCS) const
{
    // This should only be called on color spaces from the input config.

    // NB: This routine assumes all NamedTransforms have been removed from the mergeConfig.
    // Not trying to handle name conflicts with NamedTransforms, color spaces have precedence.

    if (!inputCS)
    {
        return false;
    }

    const char * name = inputCS->getName();

    // This will compare the name against roles, color space names, and aliases.
    ConstColorSpaceRcPtr existingCS = mergeConfig->getColorSpace(name);

    if (!existingCS)
    {
        // No name conflicts, go ahead and add it.
        return true;
    }

    // OK, something has this name, figure out what it is.

    // Does it have the same name as a role?
    if (mergeConfig->hasRole(name))
    {
        // Don't merge it if it would override a role.

        std::ostringstream os;
        os << "Color space '" << name << "' was not merged as it's identical to a role name.";
        notify(os.str(), m_params->isErrorOnConflict());

        return false;
    }

    const ConfigMergingParameters::MergeStrategies strategy = m_params->getColorspaces();

    // Does it have the same name as another color space?
    if (Platform::Strcasecmp(existingCS->getName(), name) == 0)
    {
        // The name matches a color space name in the mergeConfig.
        // Whether to allow the merge is based on the merge strategy.

        if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT
         || strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY)
        {
            // Allow the merger. 

            std::ostringstream os;
            os << "Color space '" << name << "' will replace a color space in the base config.";
            notify(os.str(), m_params->isErrorOnConflict());

            return true;
        }
        else
        {
            // Don't merge since it would replace a color space from the base config.

            std::ostringstream os;
            os << "Color space '" << name << "' was not merged as it's already present in the base config.";
            notify(os.str(), m_params->isErrorOnConflict());

            return false;
        }
    }
    else
    {
        // The name conflicts with an alias of another color space.
        // Whether to allow the merge is based on the merge strategy.

        if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT
         || strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY)
        {
            // Allow the merger. 

            std::ostringstream os;
            os << "The name of merged color space '" << name << "'";
            os << " has a conflict with an alias in color space '";
            os << existingCS->getName() << "'.";
            notify(os.str(), m_params->isErrorOnConflict());

            return true;
        }
        else
        {
            // Don't merge it if it would replace an alias from the base config.

            std::ostringstream os;
            os << "Color space '" << name << "' was not merged as it conflicts with an alias ";
            os << "in color space '" << existingCS->getName() << "'.";
            notify(os.str(), m_params->isErrorOnConflict());

            return false;
        }
    }
}

void ColorspacesMerger::mergeColorSpace(ConfigRcPtr & mergeConfig, 
                                        ColorSpaceRcPtr & eInputCS,
                                        std::vector<std::string> & addedInputColorSpaces)
{
    // NB: This routine assumes all NamedTransforms have been removed from the mergeConfig.
    // Not trying to handle name conflicts with NamedTransforms, color spaces have precedence.

    // Check if mergeConfig already has a color space with the same name as the input CS.

    const char * name = eInputCS->getName();
    ConstColorSpaceRcPtr originalCS = mergeConfig->getColorSpace(name);

    if (originalCS)
    {
        // Note that the color space which gets discarded and the color space being added below may not
        // have the same reference space type (i.e., scene vs. display). This is currently allowed
        // but log a warning.
        if (eInputCS->getReferenceSpaceType() != originalCS->getReferenceSpaceType())
        {
            std::ostringstream os;
            os << "Merged color space '" << name << "' has a different reference ";
            os << "space type than the color space it's replacing.";
            notify(os.str(), false);
        }

        // If there is a color space with this name in the existing config,
        // remove it (and any aliases it may contain). This is the case when
        // the strategy calls for replacing an existing color space.
        //
        // If the eInputCS name matched an alias rather than a color space name,
        // this does nothing (and the alias is handled below).
        //
        // The notification is handled in colorSpaceMayBeMerged to avoid having to determine
        // again whether the conflict is with the name or alias of originalCS.

        mergeConfig->removeColorSpace(name);
    }

    // Handle conflicts of the eInputCS name with aliases of other color spaces.

    ConstColorSpaceRcPtr existingCS = mergeConfig->getColorSpace(name);
    if (existingCS)
    {
        // Get the name of the color space that contains the alias.

        // Verify that the name is actually an alias rather than some other conflict.
        // (Should never happen.)
        if (!hasAlias(existingCS, name))
        {
            std::ostringstream os;
            os << "Problem merging color space: '" << name << "'.";
            throw Exception(os.str().c_str());
        }

        // Remove the alias from that existing color space.
        //    Note that this conflict was detected and allowed in colorSpaceMayBeMerged
        //    based on the merge strategy, so the decision has already been made to remove
        //    this alias from a color space in the base config.

        auto eExistingCS = existingCS->createEditableCopy();
        eExistingCS->removeAlias(name);
        // Edit the colorspace in the copy of the merged config.
        mergeConfig->addColorSpace(eExistingCS);

        // The notification is handled in colorSpaceMayBeMerged to avoid having to determine
        // again whether the conflict is with the name or alias of originalCS.
    }

    const ConfigMergingParameters::MergeStrategies strategy = m_params->getColorspaces();

    // Handle conflicts of the eInputCS aliases with other color spaces or aliases.

    for (size_t i = 0; i < eInputCS->getNumAliases(); i++)
    {
        const char * aliasName = eInputCS->getAlias(i);

        std::ostringstream os;

        ConstColorSpaceRcPtr conflictingCS = mergeConfig->getColorSpace(aliasName);
        if (conflictingCS)
        {
            if (Platform::Strcasecmp(conflictingCS->getName(), aliasName) == 0)
            {
                // The alias conflicts with the name of an existing color space.

                os << "Merged color space '" << name << "'";
                os << " has an alias '";
                os << aliasName << "' that conflicts with color space '";
                os << conflictingCS->getName() << "'.";

                if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT
                 || strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY)
                {
                    // Remove that base color space.
                    mergeConfig->removeColorSpace(conflictingCS->getName());
                }
                else
                {
                    // Remove the alias from the input color space.
                    eInputCS->removeAlias(aliasName);
                }
            }
            else if (hasAlias(conflictingCS, aliasName))
            {
                // The alias conflicts with an alias of the conflicting color space.

                os << "Merged color space '" << name << "'";
                os << " has a conflict with alias '";
                os << aliasName << "' in color space '";
                os << conflictingCS->getName() << "'.";

                if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT
                 || strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY)
                {
                    // Remove the alias from that base color space.
                    auto eConflictingCS = conflictingCS->createEditableCopy();
                    eConflictingCS->removeAlias(aliasName);

                    // Replace the colorspace in the copy of the merged config.
                    mergeConfig->addColorSpace(eConflictingCS);
                }
                else
                {
                    // Remove the alias from the input color space.
                    eInputCS->removeAlias(aliasName);
                }
            }
            else if (mergeConfig->hasRole(aliasName))
            {
                os << "Merged color space '" << name << "'";
                os << " has an alias '";
                os << aliasName << "' that conflicts with a role.";

                // Remove the alias from the input color space.
                eInputCS->removeAlias(aliasName);
            }
            else // (Should never happen.)
            {
                std::ostringstream os;
                os << "Problem merging color space: '" << name << "' due to its aliases.";
                throw Exception(os.str().c_str());
            }

            notify(os.str(), m_params->isErrorOnConflict());
        }
    }

    // Add the color space. This will throw if a problem is found. 
    // (But all name conflicts should have been handled already.)
    mergeConfig->addColorSpace(eInputCS);

    // Keep a record that this input color space was added to allow reordering later.
    addedInputColorSpaces.push_back(name);

    // TODO: Is it ever possible that a CS added to the list would be removed as another is merged?

    // TODO: When color spaces or aliases are removed above, it's possible it could break
    // some other part of the config that referenced them. This would include elements such as:
    // environment, views, inactive_colorspaces, ColorSpaceTransforms, or DisplayViewTransforms.
}

void ColorspacesMerger::addColorSpaces()
{
    // Delete all the NamedTransforms, color spaces take precedence, so don't want them
    // interfering with merges by causing name conflicts.

    // NB: This is only intended to be called for the prefer_input and prefer_base strategies.

    m_mergedConfig->clearNamedTransforms();

    // Make a temp copy to merge the input color spaces into (will reorder them later).
    ConfigRcPtr mergeConfig = m_mergedConfig->createEditableCopy();

    mergeConfig->clearNamedTransforms();

    ConstTransformRcPtr inputToBaseGtScene, inputToBaseGtDisplay;
    initializeRefSpaceConverters(inputToBaseGtScene, inputToBaseGtDisplay);

    // Loop over all active and inactive color spaces of all reference types in the input config.
    // Merge them into the temp config (which already contains the base color spaces).
    std::vector<std::string> addedInputColorSpaces;

    for (int i = 0; i < m_inputConfig->getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL); ++i)
    {
        const char * name = m_inputConfig->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL,
                                                                    COLORSPACE_ALL,
                                                                    i);
        ConstColorSpaceRcPtr cs = m_inputConfig->getColorSpace(name);
        if (!cs)
            continue;
        ColorSpaceRcPtr eCS = cs->createEditableCopy();

            if (!m_params->isAssumeCommonReferenceSpace())
            {
                if (eCS->getReferenceSpaceType() == REFERENCE_SPACE_DISPLAY)
                    ConfigUtils::updateReferenceColorspace(eCS, inputToBaseGtDisplay);
                else
                    ConfigUtils::updateReferenceColorspace(eCS, inputToBaseGtScene);
            }

        // Doing this against the mergedConfig rather than the base config so that the most
        // recent state of any aliases that get added or color spaces that are removed are
        // considered by the duplicate consolidation process.
        const bool notDuplicate = handleAvoidDuplicatesOption(mergeConfig, eCS);

        if (notDuplicate && colorSpaceMayBeMerged(mergeConfig, eCS))
        {
            // NB: This may make changes to existing color spaces in mergeConfig
            // to resolve name conflicts.
            mergeColorSpace(mergeConfig, eCS, addedInputColorSpaces);
        }
    }

    m_mergedConfig->clearColorSpaces();

    // Add the color spaces to the real merged config.

    if (m_params->isInputFirst())
    {
        // Add color spaces from the input config.
        for (auto & name : addedInputColorSpaces)
        {
            ConstColorSpaceRcPtr inputCS = mergeConfig->getColorSpace(name.c_str());
            if (inputCS)
            {
                ColorSpaceRcPtr eInputCS = inputCS->createEditableCopy();

                // Add family prefix.
                const bool fromBase = false;
                std::string family = eInputCS->getFamily();
                updateFamily(family, fromBase);
                eInputCS->setFamily(family.c_str());

                m_mergedConfig->addColorSpace(eInputCS);

                mergeConfig->removeColorSpace(name.c_str());
            }
        }

        // Add color spaces from the base config.
        for (int i = 0; i < mergeConfig->getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL); ++i)
        {
            // Note that during the merge process, some of the color spaces from the base config may
            // be replaced if their aliases are edited. This should not change their order in the config,
            // but may want to account for this, if that ever changes and they get moved to the end.
            const char * name = mergeConfig->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL,
                                                                      COLORSPACE_ALL,
                                                                      i);
            ConstColorSpaceRcPtr baseCS = mergeConfig->getColorSpace(name);
            if (baseCS)
            {
                auto eBaseCS = baseCS->createEditableCopy();

                // Add family prefix.
                const bool fromBase = true;
                std::string family = eBaseCS->getFamily();
                updateFamily(family, fromBase);
                eBaseCS->setFamily(family.c_str());

                m_mergedConfig->addColorSpace(eBaseCS);
            }
        }
    }
    else
    {
        // The color spaces should already be in the correct order.
        // Copy them into the real merged config and add family prefix.
        for (int i = 0; i < mergeConfig->getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL); ++i)
        {
            const char * name = mergeConfig->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL,
                                                                      COLORSPACE_ALL,
                                                                      i);
            ConstColorSpaceRcPtr cs = mergeConfig->getColorSpace(name);
            if (cs)
            {
                auto eCS = cs->createEditableCopy();
                bool fromBase = true;

                // Add family prefix.
                if (std::find(addedInputColorSpaces.begin(), addedInputColorSpaces.end(), name)
                    != addedInputColorSpaces.end())
                {
                    fromBase = false;
                }
                std::string family = eCS->getFamily();
                updateFamily(family, fromBase);
                eCS->setFamily(family.c_str());

                m_mergedConfig->addColorSpace(eCS);
            }
        }
    }

    // (Not cleaning up the inactive list here, it would remove named transforms, wait until after NT.)

    // TODO: What if the environment contains a color space that was removed?
}


void ColorspacesMerger::handlePreferInput()
{
    // Set environment.
    //     Since the environment variables are stored inside a std::map, the key are ordered
    //     alphabetically. Therefore, there is no point to look at the option "InputFirst".
    if (m_params->getNumEnvironmentVars() > 0)
    {
        // Take environment variables from overrides.
        m_mergedConfig->clearEnvironmentVars();
        for (int i = 0; i < m_params->getNumEnvironmentVars(); i++)
        {
            m_mergedConfig->addEnvironmentVar(m_params->getEnvironmentVar(i),
                                              m_params->getEnvironmentVarValue(i));
        }
    }
    else
    {
        // Add environment variables from input config to base config.
        for (int i = 0; i < m_inputConfig->getNumEnvironmentVars(); i++)
        {
            const std::string & name = m_inputConfig->getEnvironmentVarNameByIndex(i);
            // Overwrite any existing env. variable with the same name.
            m_mergedConfig->addEnvironmentVar(name.c_str(),
                                              m_inputConfig->getEnvironmentVarDefault(name.c_str()));
        }
    }

    // Set search_path.
    processSearchPaths();

    // Set inactive_colorspaces.
    const char * inactiveCS = m_params->getInactiveColorSpaces();
    if (inactiveCS && *inactiveCS)
    {
        // Take inactive colorspaces from overrides.
        m_mergedConfig->setInactiveColorSpaces(inactiveCS);
    }
    else
    {
        // Add inactive colorspaces from input config to base config.

        StringUtils::StringVec mergedInactiveCS;
        if (m_params->isInputFirst())
        {
            StringUtils::StringVec baseInactiveCS;
            splitActiveList(m_baseConfig->getInactiveColorSpaces(), baseInactiveCS);
            splitActiveList(m_inputConfig->getInactiveColorSpaces(), mergedInactiveCS);

            mergeStringsWithoutDuplicates(baseInactiveCS, mergedInactiveCS);
        }
        else
        {
            StringUtils::StringVec inputInactiveCS;
            splitActiveList(m_inputConfig->getInactiveColorSpaces(), inputInactiveCS);
            splitActiveList(m_mergedConfig->getInactiveColorSpaces(), mergedInactiveCS);

            mergeStringsWithoutDuplicates(inputInactiveCS, mergedInactiveCS);
        }
        m_mergedConfig->setInactiveColorSpaces(StringUtils::Join(mergedInactiveCS, ',').c_str());
    }

    // Set family_separator.
    m_mergedConfig->setFamilySeparator(m_inputConfig->getFamilySeparator());

    // Merge the color spaces.
    addColorSpaces();
}

void ColorspacesMerger::handlePreferBase()
{
    // Set environment.
    //     Since the environment variables are stored inside a std::map, the key are ordered
    //     alphabetically. Therefore, there is no point to look at the option "InputFirst".
    if (m_params->getNumEnvironmentVars() > 0)
    {
        // Take environment variables from overrides.
        m_mergedConfig->clearEnvironmentVars();
        for (int i = 0; i < m_params->getNumEnvironmentVars(); i++)
        {
            m_mergedConfig->addEnvironmentVar(m_params->getEnvironmentVar(i),
                                              m_params->getEnvironmentVarValue(i));
        }
    }
    else
    {
        // Take environment variables from config.
        for (int i = 0; i < m_inputConfig->getNumEnvironmentVars(); i++)
        {
            const std::string & name = m_inputConfig->getEnvironmentVarNameByIndex(i);
            // If the var's default value is empty, it doesn't exist, so nothing to overwrite.
            const bool varDoesNotExist = std::string(m_mergedConfig->getEnvironmentVarDefault(name.c_str())).empty();
            if (varDoesNotExist)
            {
                m_mergedConfig->addEnvironmentVar(name.c_str(),
                                                  m_inputConfig->getEnvironmentVarDefault(name.c_str()));
            }
        }
    }

    // Set search_path.
    processSearchPaths();

    // Set inactive_colorspaces.
    const char * inactiveCS = m_params->getInactiveColorSpaces();
    if (inactiveCS && *inactiveCS)
    {
        // Take inactive colorspaces from overrides.
        m_mergedConfig->setInactiveColorSpaces(inactiveCS);
    }
    else
    {
        // Add inactive colorspaces from input config to base config.

        StringUtils::StringVec mergedInactiveCS;
        if (m_params->isInputFirst())
        {
            StringUtils::StringVec baseInactiveCS;
            splitActiveList(m_baseConfig->getInactiveColorSpaces(), baseInactiveCS);
            splitActiveList(m_inputConfig->getInactiveColorSpaces(), mergedInactiveCS);

            mergeStringsWithoutDuplicates(baseInactiveCS, mergedInactiveCS);
        }
        else
        {
            StringUtils::StringVec inputInactiveCS;
            splitActiveList(m_inputConfig->getInactiveColorSpaces(), inputInactiveCS);
            splitActiveList(m_mergedConfig->getInactiveColorSpaces(), mergedInactiveCS);

            mergeStringsWithoutDuplicates(inputInactiveCS, mergedInactiveCS);
        }
        m_mergedConfig->setInactiveColorSpaces(StringUtils::Join(mergedInactiveCS, ',').c_str());
    }

    // Set family_separator.
    m_mergedConfig->setFamilySeparator(m_baseConfig->getFamilySeparator());

    // Merge the color spaces.
    addColorSpaces();
}

void ColorspacesMerger::handleInputOnly()
{
    // Set environment.
    if (m_params->getNumEnvironmentVars() > 0)
    {
        // Take environment variables from overrides.
        m_mergedConfig->clearEnvironmentVars();
        for (int i = 0; i < m_params->getNumEnvironmentVars(); i++)
        {
            m_mergedConfig->addEnvironmentVar(m_params->getEnvironmentVar(i),
                                              m_params->getEnvironmentVarValue(i));
        }
    }
    else
    {
        // Take environment variables from config.
        m_mergedConfig->clearEnvironmentVars();
        for (int i = 0; i < m_inputConfig->getNumEnvironmentVars(); i++)
        {
            const std::string & name = m_inputConfig->getEnvironmentVarNameByIndex(i);
            m_mergedConfig->addEnvironmentVar(name.c_str(),
                                              m_inputConfig->getEnvironmentVarDefault(name.c_str()));
        }
    }

    // Set search_path.
    const char * searchPaths = m_params->getSearchPath();
    if (searchPaths && *searchPaths)
    {
        // Use the override.
        m_mergedConfig->setSearchPath(searchPaths);
    }
    else
    {
        m_mergedConfig->setSearchPath(m_inputConfig->getSearchPath());
    }

    // Set inactive_colorspaces.
    const char * inactiveCS = m_params->getInactiveColorSpaces();
    if (inactiveCS && *inactiveCS)
    {
        // Take inactive color spaces from overrides.
        m_mergedConfig->setInactiveColorSpaces(inactiveCS);
    }
    else
    {
        // Take inactive color spaces from config.
        m_mergedConfig->setInactiveColorSpaces(m_inputConfig->getInactiveColorSpaces());
    }

    // Set family_separator.
    m_mergedConfig->setFamilySeparator(m_inputConfig->getFamilySeparator());

    // Remove all color spaces from base config.
    m_mergedConfig->clearColorSpaces();

    // Take color spaces from input config.
    // No error expected as it only adds the color spaces from the input config.

    // Avoid any conflicts with named transforms from the base config.
    m_mergedConfig->clearNamedTransforms();

    // Merge the color spaces.
    const size_t numCS = m_inputConfig->getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL);
    for (unsigned long i = 0; i < numCS; ++i)
    {
        const char * name = m_inputConfig->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL,
                                                                    COLORSPACE_ALL,
                                                                    i);
        ConstColorSpaceRcPtr cs = m_inputConfig->getColorSpace(name);
        m_mergedConfig->addColorSpace(cs);
    }
}

void ColorspacesMerger::handleBaseOnly()
{
    // Process the overrides only since the merged config is initialized to
    // the base config.

    // Do search_path override.
    const char * searchPaths = m_params->getSearchPath();
    if (searchPaths && *searchPaths)
    {
        // Use the override.
        m_mergedConfig->setSearchPath(searchPaths);
    }

    // Do environment override.
    if (m_params->getNumEnvironmentVars() > 0)
    {
        // Take environment variables from overrides.
        m_mergedConfig->clearEnvironmentVars();
        for (int i = 0; i < m_params->getNumEnvironmentVars(); i++)
        {
            m_mergedConfig->addEnvironmentVar(m_params->getEnvironmentVar(i),
                                              m_params->getEnvironmentVarValue(i));
        }
    }

    // Do inactive_colorspaces override.
    const char * inactiveCS = m_params->getInactiveColorSpaces();
    if (inactiveCS && *inactiveCS)
    {
        m_mergedConfig->setInactiveColorSpaces(inactiveCS);
    }

    // Nothing to do for display_colorspaces and colorspaces as the merged config
    // is initialized to the base config.

    // TODO: Avoid conflicts if roles are added from input config?
}

void ColorspacesMerger::handleRemove()
{
    // Handle environment.
    //     If an environment variable is used somewhere and got removed, validating the config
    //     will return an error.
    for (int i = 0; i < m_inputConfig->getNumEnvironmentVars(); i++)
    {
        const std::string & name = m_inputConfig->getEnvironmentVarNameByIndex(i);
        bool exists = !std::string(m_mergedConfig->getEnvironmentVarDefault(name.c_str())).empty();
        if (exists)
        {
            m_mergedConfig->addEnvironmentVar(name.c_str(), NULL);
        }
    }

    // Handle search_path.
    m_mergedConfig->clearSearchPaths();
    for (int i = 0; i < m_baseConfig->getNumSearchPaths(); i++)
    {
        size_t found = StringUtils::Find(m_inputConfig->getSearchPath(), m_baseConfig->getSearchPath(i));
        if (found == std::string::npos)
        {
            m_mergedConfig->addSearchPath(m_baseConfig->getSearchPath(i));
        }
    }

    // Handle inactive_colorspaces.
    StringUtils::StringVec baseInactiveCS, inputInactiveCS, mergedInactiveCS;
    splitActiveList(m_baseConfig->getInactiveColorSpaces(), baseInactiveCS);
    splitActiveList(m_inputConfig->getInactiveColorSpaces(), inputInactiveCS);
    StringUtils::Trim(inputInactiveCS);
    for (const auto & name : baseInactiveCS)
    {
        const std::string trimmedName = StringUtils::Trim(name);
        if (!trimmedName.empty())
        {
            if (!StringUtils::Contain(inputInactiveCS, trimmedName))
            {
                mergedInactiveCS.push_back(trimmedName);
            }
        }
    }
    m_mergedConfig->setInactiveColorSpaces(StringUtils::Join(mergedInactiveCS, ',').c_str());

    // The family_separator never gets removed.

    // Handle display_colorspaces and colorspaces.
    //   This could obviously break any other part of the base config that references the
    //   removed color space, so it is up to the user to know what they are doing.

    const size_t numCS = m_inputConfig->getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL);
    for (unsigned long i = 0; i < numCS; ++i)
    {
        const char * name = m_inputConfig->getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL,
                                                                    COLORSPACE_ALL,
                                                                    i);
        // Note: The remove does nothing if the color space is not present.
        m_mergedConfig->removeColorSpace(name);
    }
}

/////////////////////////////////// ColorspacesMerger end /////////////////////////////////////////


///////////////////////////////////// NamedTransformsMerger ///////////////////////////////////////

void NamedTransformsMerger::updateFamily(std::string & family, bool fromBase) const
{
    // Note that if a prefix is present, it is always added, even if the CS did not have a family.

        std::string updatedPrefix = "";
    if (m_params->getColorspaces() == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT)
    {
        if (fromBase)
        {
            // If the color space is from the base config, need to update its family separator.
            if (!family.empty())
            {
                family = replaceSeparator(family, m_baseConfig->getFamilySeparator(), m_mergedConfig->getFamilySeparator());
            }
            // Note: The family prefix argument must always use the default slash separator.
            // TODO: Should do this just once in the initializer.
            updatedPrefix = replaceSeparator(m_params->getBaseFamilyPrefix(), '/', m_mergedConfig->getFamilySeparator());
        }
        else
        {
            updatedPrefix = replaceSeparator(m_params->getInputFamilyPrefix(), '/', m_mergedConfig->getFamilySeparator());
        }
    }
    else if (m_params->getColorspaces() == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_BASE)
    {
        if (fromBase)
        {
            // TODO: Should do this just once in the initializer.
            updatedPrefix = replaceSeparator(m_params->getBaseFamilyPrefix(), '/', m_mergedConfig->getFamilySeparator());
        }
        else
        {
            // If the color space is from the input config, need to update its family separator.
            if (!family.empty())
            {
                family = replaceSeparator(family, m_inputConfig->getFamilySeparator(), m_mergedConfig->getFamilySeparator());
            }
            updatedPrefix = replaceSeparator(m_params->getInputFamilyPrefix(), '/', m_mergedConfig->getFamilySeparator());
        }
    }
    // Append the prefix to the family.
    // Note that the prefix should end with a separator, if desired.  Not adding one here.
    family = updatedPrefix + family;
}

bool NamedTransformsMerger::namedTransformMayBeMerged(const ConstConfigRcPtr & mergeConfig, 
                                                      const ConstNamedTransformRcPtr & nt,
                                                      bool fromBase) const
{
    if (!nt)
    {
        return false;
    }

    const char * name = nt->getName();

    // This will compare the name against roles, color space names, and aliases.
    // (Note that if the role refers to a named transform, this will return null,
    //  but it's illegal for a role to point to a named transform.)
    ConstColorSpaceRcPtr existingCS = mergeConfig->getColorSpace(name);

    ConstNamedTransformRcPtr existingNT = mergeConfig->getNamedTransform(name);

    if (!existingCS && !existingNT)
    {
        // No name conflicts, go ahead and add it.
        return true;
    }

    // OK, something has this name, figure out what it is.

    // Does it have the same name as a role?
    if (mergeConfig->hasRole(name))
    {
        // Don't merge it if it would override a role.

        std::ostringstream os;
        os << "Named transform '" << name << "' was not merged as it's identical to a role name.";
        notify(os.str(), m_params->isErrorOnConflict());

        return false;
    }

    if (existingCS)
    {
        // Does it have the same name as another color space?
        if (Platform::Strcasecmp(existingCS->getName(), name) == 0)
        {
            // The name matches a color space name in the mergeConfig.
            // Don't merge it, color spaces always have precedence.
            std::ostringstream os;
            os << "Named transform '" << name << "' was not merged as there's a color space with that name.";
            notify(os.str(), m_params->isErrorOnConflict());

            return false;
        }
        else
        {
            // The name conflicts with an alias of a color space.
            // Don't merge it, color spaces always have precedence.
            std::ostringstream os;
            os << "Named transform '" << name << "' was not merged as there's a color space alias with that name.";
            notify(os.str(), m_params->isErrorOnConflict());

            return false;
        }
    }

    if (existingNT)
    {
        if (fromBase)
        {
            // Should not happen if the base config was legal.
            std::ostringstream os;
            os << "Named transform '" << name << "' was not merged as there's more than one with that name in the base config.";
            notify(os.str(), m_params->isErrorOnConflict());

            return false;
        }

        const ConfigMergingParameters::MergeStrategies strategy = m_params->getNamedTransforms();

        // At this point, only dealing with transforms from the input config.

        if (Platform::Strcasecmp(existingNT->getName(), name) == 0)
        {
            // The name matches a named transform name in the mergeConfig.
            // Whether to allow the merge is based on the merge strategy.

            if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT
             || strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY)
            {
                // Allow the merger.

                std::ostringstream os;
                os << "Named transform '" << name << "' will replace a named transform in the base config.";
                notify(os.str(), m_params->isErrorOnConflict());

                return true;
            }
            else
            {
                // Don't merge it if it would replace a named transform from the base config.

                std::ostringstream os;
                os << "Named transform '" << name << "' was not merged as it's already present in the base config.";
                notify(os.str(), m_params->isErrorOnConflict());

                return false;
            }
        }
        else
        {
            // The name conflicts with an alias of another named transform.
            // Whether to allow the merge is based on the merge strategy.

            if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT
             || strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY)
            {
                // Allow the merger. 

                std::ostringstream os;
                os << "The name of merged named transform '" << name << "'";
                os << " has a conflict with an alias in named transform '";
                os << existingNT->getName() << "'.";
                notify(os.str(), m_params->isErrorOnConflict());

                return true;
            }
            else
            {
                // Don't merge it if it would replace an alias from the base config.

                std::ostringstream os;
                os << "Named transform '" << name << "' was not merged as it conflicts with an alias ";
                os << "in named transform '" << existingNT->getName() << "'.";
                notify(os.str(), m_params->isErrorOnConflict());

                return false;
            }
        }
    }
    return false;
}

void NamedTransformsMerger::mergeNamedTransform(ConfigRcPtr & mergeConfig, 
                                                NamedTransformRcPtr & eNT,
                                                bool fromBase,
                                                std::vector<std::string> & addedInputNamedTransforms)
{
    // NB: This routine assumes all NamedTransforms have been removed from the mergeConfig.
    // Not trying to handle name conflicts with NamedTransforms, color spaces have precedence.

    // Check if mergeConfig already has a color space with the same name as the input CS.

    const char * name = eNT->getName();
    ConstNamedTransformRcPtr originalNT = mergeConfig->getNamedTransform(name);

    if (originalNT)
    {
        // If there is a named transform with this name in the existing config,
        // remove it (and any aliases it may contain). This is the case when
        // the strategy calls for replacing an existing transform.
        //
        // If the eNT name matched an alias rather than a named transform name,
        // this does nothing (and the alias is handled below).
        //
        // The notification is handled in namedTransformMayBeMerged to avoid having to determine
        // again whether the conflict is with the name or alias of originalNT.

        mergeConfig->removeNamedTransform(name);
    }

    // Handle conflicts of the eNT name with aliases of other named transforms.
    // NB: Would not be here if there is a name conflict with anything other than
    // named transforms since the decision would have been not to merge it.

    ConstNamedTransformRcPtr existingNT = mergeConfig->getNamedTransform(name);
    if (existingNT)
    {
        // Get the name of the named transform that contains the alias.

        // Verify that the name is actually an alias rather than some other conflict.
        // (Should never happen.)
        if (!hasAlias(existingNT, name))
        {
            std::ostringstream os;
            os << "Problem merging named transform: '" << name << "'.";
            throw Exception(os.str().c_str());
        }

        // Remove the alias from that existing named transform.
        //    Note that this conflict was detected and allowed in namedTransformMayBeMerged
        //    based on the merge strategy, so the decision has already been made to remove
        //    this alias from a named transform in the base config.

        auto eExistingNT = existingNT->createEditableCopy();
        eExistingNT->removeAlias(name);
        // Edit the named transform in the copy of the merged config.
        mergeConfig->addNamedTransform(eExistingNT);

        // The notification is handled in namedTransformMayBeMerged to avoid having to determine
        // again whether the conflict is with the name or alias of originalNT.
    }

    ConfigMergingParameters::MergeStrategies strategy = m_params->getNamedTransforms();
    if (fromBase)
    {
        strategy = ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_BASE;
    }

    // Handle conflicts of the eNT aliases with other color spaces, named transforms, and etc.

    for (size_t i = 0; i < eNT->getNumAliases(); i++)
    {
        const char * aliasName = eNT->getAlias(i);

        // Conflicts with color spaces or roles.  (Always remove this alias.)

        std::ostringstream os;

        const char * source = fromBase ? "Base" : "Input";

        ConstColorSpaceRcPtr conflictingCS = mergeConfig->getColorSpace(aliasName);
        if (conflictingCS)
        {
            if (Platform::Strcasecmp(conflictingCS->getName(), aliasName) == 0)
            {
                // The alias conflicts with the name of the conflicting color space.

                os << "Merged " << source << " named transform '" << name << "'";
                os << " has an alias '";
                os << aliasName << "' that conflicts with color space '";
                os << conflictingCS->getName() << "'.";

                // Remove the alias from the named transform.
                eNT->removeAlias(aliasName);
            }
            else if (hasAlias(conflictingCS, aliasName))
            {
                // The alias conflicts with an alias of the conflicting color space.

                os << "Merged " << source << " named transform '" << name << "'";
                os << " has a conflict with alias '";
                os << aliasName << "' in color space '";
                os << conflictingCS->getName() << "'.";

                // Remove the alias from the named transform.
                eNT->removeAlias(aliasName);
            }
            else if (mergeConfig->hasRole(aliasName))
            {
                os << "Merged " << source << " named transform '" << name << "'";
                os << " has an alias '";
                os << aliasName << "' that conflicts with a role.";

                // Remove the alias from the named transform.
                eNT->removeAlias(aliasName);
            }
            else // (Should never happen.)
            {
                std::ostringstream os;
                os << "Problem merging named transform: '" << name << "' due to its aliases.";
                throw Exception(os.str().c_str());
            }

            // Throw if requested, otherwise log a warning.
            notify(os.str(), m_params->isErrorOnConflict());
        }

        // Conflicts of the alias with other named transforms.

        ConstNamedTransformRcPtr conflictingNT = mergeConfig->getNamedTransform(aliasName);
        if (conflictingNT)
        {
            if (Platform::Strcasecmp(conflictingNT->getName(), aliasName) == 0)
            {
                // The alias conflicts with the name of an existing named transform.

                os << "Merged " << source << " named transform '" << name << "'";
                os << " has an alias '";
                os << aliasName << "' that conflicts with named transform '";
                os << conflictingNT->getName() << "'.";

                if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT
                 || strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY)
                {
                    // Remove that base named transform.
                    mergeConfig->removeNamedTransform(conflictingNT->getName());
                }
                else
                {
                    // Remove the alias from the input named transform.
                    eNT->removeAlias(aliasName);
                }
            }
            else if (hasAlias(conflictingNT, aliasName))
            {
                // The alias conflicts with an alias of the conflicting named transform.

                os << "Merged " << source << " named transform '" << name << "'";
                os << " has a conflict with alias '";
                os << aliasName << "' in named transform '";
                os << conflictingNT->getName() << "'.";

                if (strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT
                 || strategy == ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY)
                {
                    // Remove the alias from that base named transform.
                    auto eConflictingNT = conflictingNT->createEditableCopy();
                    eConflictingNT->removeAlias(aliasName);

                    // Replace the colorspace in the copy of the merged config.
                    mergeConfig->addNamedTransform(eConflictingNT);
                }
                else
                {
                    // Remove the alias from the input named transform.
                    eNT->removeAlias(aliasName);
                }
            }
            else // (Should never happen.)
            {
                std::ostringstream os;
                os << "Problem merging named transform: '" << name << "' due to its aliases.";
                throw Exception(os.str().c_str());
            }

            notify(os.str(), m_params->isErrorOnConflict());
        }
    }

    // Add the named transform. This will throw if a problem is found. 
    // (But all name conflicts should have been handled already.)
    mergeConfig->addNamedTransform(eNT);

    // Keep a record that this input color space was added to allow reordering later.
    if (!fromBase)
    {
        addedInputNamedTransforms.push_back(name);
    }

    // TODO: Is it ever possible that a CS added to the list would be removed as another is merged?

    // TODO: When color spaces or aliases are removed above, it's possible it could break
    // some other part of the config that referenced them. This would include elements such as:
    // environment, views, inactive_colorspaces, ColorSpaceTransforms, or DisplayViewTransforms.
}

void NamedTransformsMerger::addNamedTransforms()
{
    // Delete all the NamedTransforms, color spaces take precedence, so don't want them
    // interfering with merges by causing name conflicts.

    // NB: This is only intended to be called for the prefer_input and prefer_base strategies.

    // Need to add even the base to ensure there are not conflicts with the merged color spaces.

    m_mergedConfig->clearNamedTransforms();

    // Make a temp copy to merge the input color spaces into (will reorder them later).
    ConfigRcPtr mergeConfig = m_mergedConfig->createEditableCopy();

    std::vector<std::string> addedInputNamedTransforms;

    // Loop over all active and inactive color spaces of all reference types in the input config.
    // Merge them into the temp config (which already contains the base color spaces).

    // Merge from Base config.

    for (int i = 0; i < m_baseConfig->getNumNamedTransforms(NAMEDTRANSFORM_ALL); ++i)
    {
        const char * name = m_baseConfig->getNamedTransformNameByIndex(NAMEDTRANSFORM_ALL, i);

        ConstNamedTransformRcPtr nt = m_baseConfig->getNamedTransform(name);
        if (!nt)
        {
            continue;
        }

        NamedTransformRcPtr eNT = nt->createEditableCopy();

        const bool fromBase = true;
        if (namedTransformMayBeMerged(mergeConfig, eNT, fromBase))
        {
            // NB: This may make changes to existing color spaces in mergeConfig
            // to resolve name conflicts.
            mergeNamedTransform(mergeConfig, eNT, fromBase, addedInputNamedTransforms);
        }
    }

    // Merge from Input config.

    for (int i = 0; i < m_inputConfig->getNumNamedTransforms(NAMEDTRANSFORM_ALL); ++i)
    {
        const char * name = m_inputConfig->getNamedTransformNameByIndex(NAMEDTRANSFORM_ALL, i);

        ConstNamedTransformRcPtr nt = m_inputConfig->getNamedTransform(name);
        if (!nt)
        {
            continue;
        }

        NamedTransformRcPtr eNT = nt->createEditableCopy();

        // Doing this against the mergedConfig rather than the base config so that the most
        // recent state of any aliases that get added or color spaces that are removed are
        // considered by the duplicate consolidation process.

// FIXME: Handle duplicate named transforms.
//        const bool notDuplicate = handleAvoidDuplicatesOption(mergeConfig, eCS);

        const bool fromBase = false;
        if (namedTransformMayBeMerged(mergeConfig, eNT, fromBase))
        {
            // NB: This may make changes to existing color spaces in mergeConfig
            // to resolve name conflicts.
            mergeNamedTransform(mergeConfig, eNT, fromBase, addedInputNamedTransforms);
        }
    }

    m_mergedConfig->clearNamedTransforms();

    // Add the named transforms to the real merged config.

    if (m_params->isInputFirst())
    {
        // Add color spaces from the input config.
        for (auto & name : addedInputNamedTransforms)
        {
            ConstNamedTransformRcPtr inputNT = mergeConfig->getNamedTransform(name.c_str());
            if (inputNT)
            {
                NamedTransformRcPtr eInputNT = inputNT->createEditableCopy();

                // Add family prefix.
                const bool fromBase = false;
                std::string family = eInputNT->getFamily();
                updateFamily(family, fromBase);
                eInputNT->setFamily(family.c_str());

                m_mergedConfig->addNamedTransform(eInputNT);

                mergeConfig->removeNamedTransform(name.c_str());
            }
        }

        // Add color spaces from the base config.
        for (int i = 0; i < mergeConfig->getNumNamedTransforms(NAMEDTRANSFORM_ALL); ++i)
        {
            // Note that during the merge process, some of the color spaces from the base config may
            // be replaced if their aliases are edited. This should not change their order in the config,
            // but may want to account for this, if that ever changes and they get moved to the end.
            const char * name = mergeConfig->getNamedTransformNameByIndex(NAMEDTRANSFORM_ALL, i);

            ConstNamedTransformRcPtr baseNT = mergeConfig->getNamedTransform(name);
            if (baseNT)
            {
                auto eBaseNT = baseNT->createEditableCopy();

                // Add family prefix.
                const bool fromBase = true;
                std::string family = eBaseNT->getFamily();
                updateFamily(family, fromBase);
                eBaseNT->setFamily(family.c_str());

                m_mergedConfig->addNamedTransform(eBaseNT);
            }
        }
    }
    else
    {
        // The color spaces should already be in the correct order.
        // Copy them into the real merged config and add family prefix.
        for (int i = 0; i < mergeConfig->getNumNamedTransforms(NAMEDTRANSFORM_ALL); ++i)
        {
            const char * name = mergeConfig->getNamedTransformNameByIndex(NAMEDTRANSFORM_ALL, i);

            ConstNamedTransformRcPtr nt = mergeConfig->getNamedTransform(name);
            if (nt)
            {
                auto eNT = nt->createEditableCopy();
                bool fromBase = true;

                // Add family prefix.
                if (std::find(addedInputNamedTransforms.begin(), addedInputNamedTransforms.end(), name)
                    != addedInputNamedTransforms.end())
                {
                    fromBase = false;
                }
                std::string family = eNT->getFamily();
                updateFamily(family, fromBase);
                eNT->setFamily(family.c_str());

                m_mergedConfig->addNamedTransform(eNT);
            }
        }
    }

    // Ensure the inactive_colorspaces doesn't contain anything that was removed.
    // TODO: Should move this to a higher level?
    cleanUpInactiveList(m_mergedConfig);

    // TODO: What if the environment contains a color space that was removed?
}

/*
void NamedTransformsMerger::handleErrorCodes(NamedTransformRcPtr & eNamedTransform)
{
            switch (errorCode)
            {
                case ADD_NT_ERROR_NONE:
                    allErrorsResolved = true;
                    break;
                case ADD_NT_ERROR_NULL:
                    // Should not happen.
                    break;
                case ADD_NT_ERROR_EMPTY:
                    // Should not happen.
                    break;

                case ADD_NT_ERROR_AT_LEAST_ONE_TRANSFORM:
                    skipCurrentNamedTransform = handleAddNtErrorNeedAtLeastOneTransform(eNamedTransform);
                    break;

                case ADD_NT_ERROR_NAME_IDENTICAL_TO_A_ROLE_NAME:
                    skipCurrentNamedTransform = handleAddNtErrorNameIdenticalToARoleName(eNamedTransform);
                    break;

                case ADD_NT_ERROR_NAME_IDENTICAL_TO_COLORSPACE_OR_ALIAS:
                    skipCurrentNamedTransform = handleAddNtErrorNameIdenticalToColorspaceNameOrAlias(eNamedTransform);
                    break;

                case ADD_NT_ERROR_NAME_CONTAIN_CTX_VAR_TOKEN:
                    // Should not happen. It will error out before the merge process.
                    break;

                case ADD_NT_ERROR_NAME_IDENTICAL_TO_EXISTING_NT_ALIAS:
                    skipCurrentNamedTransform = handleAddNtErrorNameIdenticalToExistingNtAlias(eNamedTransform);
                    break;

                case ADD_NT_ERROR_ALIAS_IDENTICAL_TO_A_ROLE_NAME:
                    skipCurrentNamedTransform = handleAddNtErrorAliasIdenticalToARoleName(eNamedTransform);
                    break;

                case ADD_NT_ERROR_ALIAS_IDENTICAL_TO_COLORSPACE_OR_ALIAS:
                    skipCurrentNamedTransform = handleAddNtErrorAliasIdenticalToColorspaceNameOrAlias(eNamedTransform);
                    break;

                case ADD_NT_ERROR_ALIAS_CONTAIN_CTX_VAR_TOKEN:
                    // Should not happen. It will error out before the merge process.
                    break;

                case ADD_NT_ERROR_ALIAS_IDENTICAL_TO_EXISTING_NT_ALIAS:
                    skipCurrentNamedTransform = handleAddNtErrorAliasIdenticalToExistingNtAlias(eNamedTransform);
                    break;

                default:
                    break;
            }
}
*/

void NamedTransformsMerger::handlePreferInput()
{
    addNamedTransforms();
}

void NamedTransformsMerger::handlePreferBase()
{
    addNamedTransforms();
}

void NamedTransformsMerger::handleInputOnly()
{
    m_mergedConfig->clearNamedTransforms();

    // Add the NamedTransforms from the input config.
    for(int i = 0; i < m_inputConfig->getNumNamedTransforms(NAMEDTRANSFORM_ALL); ++i)
    {
        const char * name = m_inputConfig->getNamedTransformNameByIndex(NAMEDTRANSFORM_ALL, i);

        ConstNamedTransformRcPtr nt = m_inputConfig->getNamedTransform(name);
        if (!nt)
        {
            continue;
        }

        NamedTransformRcPtr eNT = nt->createEditableCopy();

        const bool fromBase = false;
        std::vector<std::string> addedInputNamedTransforms;
        if (namedTransformMayBeMerged(m_mergedConfig, eNT, fromBase))
        {
            // NB: This may make changes to existing color spaces in mergeConfig
            // to resolve name conflicts.
            mergeNamedTransform(m_mergedConfig, eNT, fromBase, addedInputNamedTransforms);
        }
    }

    // Ensure the inactive_colorspaces doesn't contain anything that was removed.
    cleanUpInactiveList(m_mergedConfig);
}

void NamedTransformsMerger::handleBaseOnly()
{
    m_mergedConfig->clearNamedTransforms();

    // Add the NamedTransforms from the base config.
    for (int i = 0; i < m_baseConfig->getNumNamedTransforms(NAMEDTRANSFORM_ALL); ++i)
    {
        const char * name = m_baseConfig->getNamedTransformNameByIndex(NAMEDTRANSFORM_ALL, i);

        ConstNamedTransformRcPtr nt = m_baseConfig->getNamedTransform(name);
        if (!nt)
        {
            continue;
        }

        NamedTransformRcPtr eNT = nt->createEditableCopy();

        const bool fromBase = true;
        std::vector<std::string> addedInputNamedTransforms;
        if (namedTransformMayBeMerged(m_mergedConfig, eNT, fromBase))
        {
            // NB: This may make changes to existing color spaces in mergeConfig
            // to resolve name conflicts.
            mergeNamedTransform(m_mergedConfig, eNT, fromBase, addedInputNamedTransforms);
        }
    }

    // Ensure the inactive_colorspaces doesn't contain anything that was removed.
    cleanUpInactiveList(m_mergedConfig);
}

void NamedTransformsMerger::handleRemove()
{
    m_mergedConfig->clearNamedTransforms();
    // Add the NamedTransforms from the base config if they do not exist in the INPUT config.
    for(int i = 0; i < m_baseConfig->getNumNamedTransforms(NAMEDTRANSFORM_ALL); ++i)
    {
        const char * name = m_baseConfig->getNamedTransformNameByIndex(NAMEDTRANSFORM_ALL, i);
        if (m_inputConfig->getNamedTransform(name) == nullptr)
        {
            ConstNamedTransformRcPtr nt = m_baseConfig->getNamedTransform(name);
            if (!nt)
            {
                continue;
            }

            NamedTransformRcPtr eNT = nt->createEditableCopy();

            const bool fromBase = true;
            std::vector<std::string> addedInputNamedTransforms;
            if (namedTransformMayBeMerged(m_mergedConfig, eNT, fromBase))
            {
                // NB: This may make changes to existing color spaces in mergeConfig
                // to resolve name conflicts.
                mergeNamedTransform(m_mergedConfig, eNT, fromBase, addedInputNamedTransforms);
            }
        }
    }

    // Ensure the inactive_colorspaces doesn't contain anything that was removed.
    cleanUpInactiveList(m_mergedConfig);
}

///////////////////////////////// NamedTransformsMerger end ///////////////////////////////////////

} // namespace OCIO_NAMESPACE