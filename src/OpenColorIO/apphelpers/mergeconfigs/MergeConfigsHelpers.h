// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MERGE_CONFIG_HELPERS_H
#define INCLUDED_OCIO_MERGE_CONFIG_HELPERS_H

#include <string>
#include <unordered_map>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "SectionMerger.h"
#include "utils/StringUtils.h"
#include "Logging.h"

namespace OCIO_NAMESPACE
{

class ConfigMergingParameters::Impl
{
public:
    std::string m_baseConfig;
    std::string m_inputConfig;
    std::string m_outputName;

    // overrides
    std::string m_name;
    std::string m_description;
    std::vector<std::string> m_searchPaths;
    StringUtils::StringVec m_activeDisplays;
    StringUtils::StringVec m_activeViews;
    StringUtils::StringVec m_inactiveColorspaces;
    mutable std::string m_activeDisplaysStr;
    mutable std::string m_activeViewsStr;
    mutable std::string m_inactiveColorSpaceStr;

    // Used to store the overrides for the following sections:
    // search_path, active_displays, active_views and inactive_colorspace.
    ConfigRcPtr m_overrideCfg;

    // Options
    std::string m_inputFamilyPrefix;
    std::string m_baseFamilyPrefix;
    bool m_inputFirst;
    bool m_errorOnConflict;
    bool m_avoidDuplicates;
    bool m_assumeCommonReferenceSpace;

    // Strategies
    MergeStrategies m_defaultStrategy;
    MergeStrategies m_roles;
    MergeStrategies m_fileRules;
    // Includes shared_views, displays, view_transforms, viewing_rules, virtual_display, 
    //          active_display, active_views and default_view_transform.
    MergeStrategies m_displayViews;
    MergeStrategies m_looks;
    // colorspace, environment, search_path, family_separator and inactive_colorspaces
    MergeStrategies m_colorspaces;
    MergeStrategies m_namedTransforms;

    Impl()
    {
        m_baseConfig = "";
        m_inputConfig = "";
        m_outputName = "merged";

        // Overrides
        m_name = "";
        m_description = "";
        m_overrideCfg = Config::Create();
        m_overrideCfg->clearEnvironmentVars();

        // Options
        m_defaultStrategy = STRATEGY_PREFER_INPUT;
        m_inputFamilyPrefix  = "";
        m_baseFamilyPrefix  = "";
        m_inputFirst = true;
        m_errorOnConflict = false;
        m_avoidDuplicates = true;
        m_assumeCommonReferenceSpace = false;
        
        m_roles = STRATEGY_UNSET;
        m_fileRules = STRATEGY_UNSET;
        m_displayViews = STRATEGY_UNSET;
        m_looks = STRATEGY_UNSET;
        m_colorspaces = STRATEGY_UNSET;
        m_namedTransforms = STRATEGY_UNSET;
    }

    ~Impl() = default;
    Impl(const Impl&) = delete;

    Impl& operator= (const Impl & rhs)
    {
        if(this!=&rhs)
        {
            m_baseConfig = rhs.m_baseConfig;
            m_inputConfig = rhs.m_inputConfig;
            m_outputName = rhs.m_outputName;

            // Overrides
            m_name = rhs.m_name;
            m_description = rhs.m_description;
            m_searchPaths = rhs.m_searchPaths;

            // Options
            m_defaultStrategy = rhs.m_defaultStrategy;
            m_inputFamilyPrefix  = rhs.m_inputFamilyPrefix;
            m_baseFamilyPrefix  = rhs.m_baseFamilyPrefix;
            m_inputFirst = rhs.m_inputFirst;
            m_errorOnConflict = rhs.m_errorOnConflict;
            m_avoidDuplicates = rhs.m_avoidDuplicates;
            m_assumeCommonReferenceSpace = rhs.m_assumeCommonReferenceSpace;

            m_roles = rhs.m_roles;
            m_fileRules = rhs.m_fileRules;
            m_displayViews = rhs.m_displayViews;
            m_looks = rhs.m_looks;
            m_colorspaces = rhs.m_colorspaces;
            m_namedTransforms = rhs.m_namedTransforms;
        }
        return *this;
    }
};

class ConfigMerger::Impl
{
public:
    StringUtils::StringVec m_searchPaths;
    std::string m_workingDir;

    unsigned int m_majorVersion;
    unsigned int m_minorVersion;

    std::vector<ConfigMergingParametersRcPtr> mergesParams;
    std::vector<ConstConfigRcPtr> mergedConfigs;

    Impl()
    {

    }

    ~Impl() = default;
    Impl(const Impl&) = delete;

    Impl& operator= (const Impl & rhs)
    {
        if(this != &rhs)
        {
            m_searchPaths = rhs.m_searchPaths;
            m_workingDir = rhs.m_workingDir;
            m_majorVersion = rhs.m_majorVersion;
            m_minorVersion = rhs.m_minorVersion;
            
            mergesParams.clear();
            mergesParams.reserve(rhs.mergesParams.size());
            for (const auto & param : rhs.mergesParams)
            {
                mergesParams.push_back(param->createEditableCopy());
            }

            mergedConfigs.clear();
            mergedConfigs.reserve(rhs.mergedConfigs.size());
            for (const auto & config : rhs.mergedConfigs)
            {
                mergedConfigs.push_back(config->createEditableCopy());
            }
        }
        return *this;
    }

    static ConstConfigMergerRcPtr Read(std::istream & istream, const char * filepath);

    /**
     * \brief Load the config based on the name/filepath specified.
     * 
     * Here's the steps:
     * 1 - Try to find the config name/filepath using the search_paths.
     * 2 - If not found, try to use the name as a built-in config's name.
     * 3 - If not found, try to use the name as the output of a previous merge.
     * 4 - If still not found, return an empty config object.
     */
    ConstConfigRcPtr loadConfig(const char * value) const;

    ConfigMergingParametersRcPtr getParams(int index) const
    {
        if (index >= 0 && index < static_cast<int>(mergesParams.size()))
        {
            return nullptr;
        }
        return mergesParams.at(index);
    }

    int getNumOfConfigMergingParameters() const
    {
        return static_cast<int>(mergesParams.size());
    }
};

}  // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_MERGE_CONFIG_HELPERS_H
