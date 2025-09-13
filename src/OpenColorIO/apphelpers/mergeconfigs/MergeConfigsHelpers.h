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
    // Names or paths for identifying the base and input configs.
    std::string m_baseConfig;
    std::string m_inputConfig;

    // Name for the output config (may be used as the input or base config for a subsequent merger).
    std::string m_outputName;

    // Overrides used to replace various parameters of a merged config.
    std::string m_name;
    std::string m_description;

    // Used to store the overrides for the following sections:
    // search_path, active_displays, active_views and inactive_colorspace.
    ConfigRcPtr m_overrideCfg;

    // Options for the merger.
    std::string m_inputFamilyPrefix;
    std::string m_baseFamilyPrefix;
    bool m_inputFirst;
    bool m_errorOnConflict;
    bool m_avoidDuplicates;
    bool m_adjustInputReferenceSpace;

    // Merge strategy for each section of the config.

    // The default strategy is used for any sections where a strategy was not specified.
    MergeStrategies m_defaultStrategy;
    MergeStrategies m_roles;
    MergeStrategies m_fileRules;
    // Includes shared_views, displays, viewing_rules, virtual_display, active_display, active_views.
    MergeStrategies m_displayViews;
    // Includes view_transforms and default_view_transform. 
    MergeStrategies m_viewTransforms;
    MergeStrategies m_looks;
    // Includes colorspaces, environment, search_path, family_separator, and inactive_colorspaces.
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
        m_inputFamilyPrefix  = "";
        m_baseFamilyPrefix  = "";
        m_inputFirst = true;
        m_errorOnConflict = false;
        m_avoidDuplicates = true;
        m_adjustInputReferenceSpace = true;
        
        m_defaultStrategy = STRATEGY_PREFER_INPUT;
        m_roles = STRATEGY_UNSPECIFIED;
        m_fileRules = STRATEGY_UNSPECIFIED;
        m_displayViews = STRATEGY_UNSPECIFIED;
        m_viewTransforms = STRATEGY_UNSPECIFIED;
        m_looks = STRATEGY_UNSPECIFIED;
        m_colorspaces = STRATEGY_UNSPECIFIED;
        m_namedTransforms = STRATEGY_UNSPECIFIED;
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
            m_overrideCfg = rhs.m_overrideCfg;

            // Options
            m_inputFamilyPrefix  = rhs.m_inputFamilyPrefix;
            m_baseFamilyPrefix  = rhs.m_baseFamilyPrefix;
            m_inputFirst = rhs.m_inputFirst;
            m_errorOnConflict = rhs.m_errorOnConflict;
            m_avoidDuplicates = rhs.m_avoidDuplicates;
            m_adjustInputReferenceSpace = rhs.m_adjustInputReferenceSpace;

            m_defaultStrategy = rhs.m_defaultStrategy;
            m_roles = rhs.m_roles;
            m_fileRules = rhs.m_fileRules;
            m_displayViews = rhs.m_displayViews;
            m_viewTransforms = rhs.m_viewTransforms;
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
    // This is the set of search paths for the config files that will be merged.
    // (Each of the configs has its own search path for its LUTs.)
    StringUtils::StringVec m_searchPaths;

    // 
    std::string m_workingDir;

    // Version for the .ociom file format.
    unsigned int m_majorVersion = 1u;
    unsigned int m_minorVersion = 0u;

    // Vector of merge parameter objects, each one corresponding to one merge.
    std::vector<ConfigMergingParametersRcPtr> m_mergeParams;

    // Vector of config objects, the output of each merge.
    std::vector<ConstConfigRcPtr> m_mergedConfigs;

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
            
            m_mergeParams.clear();
            m_mergeParams.reserve(rhs.m_mergeParams.size());
            for (const auto & param : rhs.m_mergeParams)
            {
                m_mergeParams.push_back(param->createEditableCopy());
            }

            m_mergedConfigs.clear();
            m_mergedConfigs.reserve(rhs.m_mergedConfigs.size());
            for (const auto & config : rhs.m_mergedConfigs)
            {
                m_mergedConfigs.push_back(config->createEditableCopy());
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
};

}  // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_MERGE_CONFIG_HELPERS_H
