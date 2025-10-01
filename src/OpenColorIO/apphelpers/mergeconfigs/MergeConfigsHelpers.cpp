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

#include "MergeConfigsHelpers.h"
#include "Logging.h"
#include "ParseUtils.h"
#include "Platform.h"
#include "OCIOMYaml.h"
#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{

ConfigMergingParameters::ConfigMergingParameters() : m_impl(new ConfigMergingParameters::Impl())
{

}

ConfigMergingParameters::~ConfigMergingParameters()
{
    delete m_impl;
    m_impl = nullptr;
}

ConfigMergingParametersRcPtr ConfigMergingParameters::Create()
{
    return ConfigMergingParametersRcPtr(new ConfigMergingParameters(), &deleter);
}

void ConfigMergingParameters::deleter(ConfigMergingParameters * c)
{
    delete c;
}

ConfigMergingParametersRcPtr ConfigMergingParameters::createEditableCopy() const
{
    ConfigMergingParametersRcPtr params = ConfigMergingParameters::Create();
    *params->m_impl = *m_impl;
    return params;
}

void ConfigMergingParameters::setBaseConfigName(const char * baseConfig)
{
    getImpl()->m_baseConfig = baseConfig;
}

const char * ConfigMergingParameters::getBaseConfigName() const
{
    return getImpl()->m_baseConfig.c_str();
}

void ConfigMergingParameters::setInputConfigName(const char * inputConfig)
{
    getImpl()->m_inputConfig = inputConfig;
}

const char * ConfigMergingParameters::getInputConfigName() const
{
    return getImpl()->m_inputConfig.c_str();
}

void ConfigMergingParameters::setName(const char * mergedConfigName)
{
    getImpl()->m_name = mergedConfigName;
}

const char * ConfigMergingParameters::getName() const
{
    return getImpl()->m_name.c_str();
}

const char * ConfigMergingParameters::getDescription() const
{
    return getImpl()->m_description.c_str();
}

void ConfigMergingParameters::setDescription(const char * mergedConfigDesc)
{
    getImpl()->m_description = mergedConfigDesc;
}

void ConfigMergingParameters::addEnvironmentVar(const char * name, const char * defaultValue)
{
    getImpl()->m_overrideCfg->addEnvironmentVar(name, defaultValue);
}

int ConfigMergingParameters::getNumEnvironmentVars() const
{
    return getImpl()->m_overrideCfg->getNumEnvironmentVars();
}

const char * ConfigMergingParameters::getEnvironmentVar(int index) const
{
    return getImpl()->m_overrideCfg->getEnvironmentVarNameByIndex(index);
}

const char * ConfigMergingParameters::getEnvironmentVarValue(int index) const
{
    const char * name = getImpl()->m_overrideCfg->getEnvironmentVarNameByIndex(index);
    return getImpl()->m_overrideCfg->getEnvironmentVarDefault(name);
}

void ConfigMergingParameters::setSearchPath(const char * path)
{
    getImpl()->m_overrideCfg->setSearchPath(path);
}

void ConfigMergingParameters::addSearchPath(const char * path)
{
    getImpl()->m_overrideCfg->addSearchPath(path);
}

const char * ConfigMergingParameters::getSearchPath() const
{
    return getImpl()->m_overrideCfg->getSearchPath();
}

void ConfigMergingParameters::setActiveDisplays(const char * displays)
{
    getImpl()->m_overrideCfg->setActiveDisplays(displays);
}

const char * ConfigMergingParameters::getActiveDisplays() const
{
    return getImpl()->m_overrideCfg->getActiveDisplays();
}

void ConfigMergingParameters::setActiveViews(const char * views)
{
    getImpl()->m_overrideCfg->setActiveViews(views);
}

const char * ConfigMergingParameters::getActiveViews() const
{
    return getImpl()->m_overrideCfg->getActiveViews();
}

void ConfigMergingParameters::setInactiveColorSpaces(const char * colorspaces)
{
    getImpl()->m_overrideCfg->setInactiveColorSpaces(colorspaces);
}

const char * ConfigMergingParameters::getInactiveColorSpaces() const
{
    return getImpl()->m_overrideCfg->getInactiveColorSpaces();
}

void ConfigMergingParameters::setOutputName(const char * outputName)
{
    getImpl()->m_outputName = outputName;
}

const char * ConfigMergingParameters::getOutputName() const
{
    return getImpl()->m_outputName.c_str();
}

void ConfigMergingParameters::setDefaultStrategy(const MergeStrategies strategy)
{
    getImpl()->m_defaultStrategy = strategy;
}

ConfigMergingParameters::MergeStrategies ConfigMergingParameters::getDefaultStrategy() const
{
    return getImpl()->m_defaultStrategy;
}

void ConfigMergingParameters::setInputFamilyPrefix(const char * prefix)
{
    getImpl()->m_inputFamilyPrefix = prefix;
}

const char * ConfigMergingParameters::getInputFamilyPrefix() const
{
    return getImpl()->m_inputFamilyPrefix.c_str();
}

void ConfigMergingParameters::setBaseFamilyPrefix(const char * prefix)
{
    getImpl()->m_baseFamilyPrefix = prefix;
}

const char * ConfigMergingParameters::getBaseFamilyPrefix() const
{
    return getImpl()->m_baseFamilyPrefix.c_str();
}

void ConfigMergingParameters::setInputFirst(bool enabled)
{
    getImpl()->m_inputFirst = enabled;
}

bool ConfigMergingParameters::isInputFirst() const
{
    return getImpl()->m_inputFirst;
}

void ConfigMergingParameters::setErrorOnConflict(bool enabled)
{
    getImpl()->m_errorOnConflict = enabled;
}

bool ConfigMergingParameters::isErrorOnConflict() const
{
    return getImpl()->m_errorOnConflict;
}

void ConfigMergingParameters::setAvoidDuplicates(bool enabled)
{
    getImpl()->m_avoidDuplicates = enabled;
}

bool ConfigMergingParameters::isAvoidDuplicates() const
{
    return getImpl()->m_avoidDuplicates;
}

void ConfigMergingParameters::setAdjustInputReferenceSpace(bool enabled)
{
    getImpl()->m_adjustInputReferenceSpace = enabled;
}

bool ConfigMergingParameters::isAdjustInputReferenceSpace() const
{
    return getImpl()->m_adjustInputReferenceSpace;
}

void ConfigMergingParameters::setRoles(MergeStrategies strategy)
{
    getImpl()->m_roles = strategy;
}

ConfigMergingParameters::MergeStrategies ConfigMergingParameters::getRoles() const
{
    if (getImpl()->m_roles == MergeStrategies::STRATEGY_UNSPECIFIED)
    {
        return getDefaultStrategy();
    }
    return getImpl()->m_roles;
}

void ConfigMergingParameters::setFileRules(MergeStrategies strategy)
{
    getImpl()->m_fileRules = strategy;
}

ConfigMergingParameters::MergeStrategies ConfigMergingParameters::getFileRules() const
{
    if (getImpl()->m_fileRules == MergeStrategies::STRATEGY_UNSPECIFIED)
    {
        return getDefaultStrategy();
    }
    return getImpl()->m_fileRules;
}

void ConfigMergingParameters::setDisplayViews(MergeStrategies strategy)
{
    getImpl()->m_displayViews = strategy;
}

ConfigMergingParameters::MergeStrategies ConfigMergingParameters::getDisplayViews() const
{
    if (getImpl()->m_displayViews == MergeStrategies::STRATEGY_UNSPECIFIED)
    {
        return getDefaultStrategy();
    }
    return getImpl()->m_displayViews;
}

void ConfigMergingParameters::setViewTransforms(MergeStrategies strategy)
{
    getImpl()->m_viewTransforms = strategy;
}

ConfigMergingParameters::MergeStrategies ConfigMergingParameters::getViewTransforms() const
{
    if (getImpl()->m_viewTransforms == MergeStrategies::STRATEGY_UNSPECIFIED)
    {
        return getDefaultStrategy();
    }
    return getImpl()->m_viewTransforms;
}

void ConfigMergingParameters::setLooks(MergeStrategies strategy)
{
    getImpl()->m_looks = strategy;
}

ConfigMergingParameters::MergeStrategies ConfigMergingParameters::getLooks() const
{
    if (getImpl()->m_looks == MergeStrategies::STRATEGY_UNSPECIFIED)
    {
        return getDefaultStrategy();
    }
    return getImpl()->m_looks;
}

void ConfigMergingParameters::setColorspaces(MergeStrategies strategy)
{
    getImpl()->m_colorspaces = strategy;
}

ConfigMergingParameters::MergeStrategies ConfigMergingParameters::getColorspaces() const
{
    if (getImpl()->m_colorspaces == MergeStrategies::STRATEGY_UNSPECIFIED)
    {
        return getDefaultStrategy();
    }
    return getImpl()->m_colorspaces;
}

void ConfigMergingParameters::setNamedTransforms(MergeStrategies strategy)
{
    getImpl()->m_namedTransforms = strategy;
}

ConfigMergingParameters::MergeStrategies ConfigMergingParameters::getNamedTransforms() const
{
    if (getImpl()->m_namedTransforms == MergeStrategies::STRATEGY_UNSPECIFIED)
    {
        return getDefaultStrategy();
    }
    return getImpl()->m_namedTransforms;
}

std::ostream & operator<<(std::ostream & os, const ConfigMergingParameters & params)
{
    os << "<";
    bool first = true;

    auto print_str = [&](const char* label, const char* value) {
        if (value && *value)
        {
            if (!first) os << ", ";
            os << label << ": " << value;
            first = false;
        }
    };

    auto print_bool = [&](const char* label, bool value) {
        if (!first) os << ", ";
        os << label << ": " << (value ? "true" : "false");
        first = false;
    };

    auto print_enum = [&](const char* label, ConfigMergingParameters::MergeStrategies value) {
        if (!first) os << ", ";
        os << label << ": " << OCIOMYaml::EnumToStrategyString(value);
        first = false;
    };

    print_str("base", params.getBaseConfigName());
    print_str("input", params.getInputConfigName());
    print_str("output_name", params.getOutputName());
    print_str("input_family_prefix", params.getInputFamilyPrefix());
    print_str("base_family_prefix", params.getBaseFamilyPrefix());
    print_bool("input_first", params.isInputFirst());
    print_bool("error_on_conflict", params.isErrorOnConflict());
    print_enum("default_strategy", params.getDefaultStrategy());
    print_bool("avoid_duplicates", params.isAvoidDuplicates());
    print_bool("adjust_input_reference_space", params.isAdjustInputReferenceSpace());
    print_str("name", params.getName());
    print_str("description", params.getDescription());
    print_str("search_path", params.getSearchPath());
    print_str("active_displays", params.getActiveDisplays());
    print_str("active_views", params.getActiveViews());
    print_str("inactive_colorspaces", params.getInactiveColorSpaces());
    print_enum("roles", params.getRoles());
    print_enum("file_rules", params.getFileRules());
    print_enum("display-views", params.getDisplayViews());
    print_enum("view_transforms", params.getViewTransforms());
    print_enum("looks", params.getLooks());
    print_enum("colorspaces", params.getColorspaces());
    print_enum("named_transforms", params.getNamedTransforms());

    // Environment vars.
    int numEnv = params.getNumEnvironmentVars();
    if (numEnv > 0)
    {
        if (!first) os << ", ";
        os << "environment: [";
        for (int i = 0; i < numEnv; ++i)
        {
            if (i > 0) os << ", ";
            os << params.getEnvironmentVar(i);
            const char* val = params.getEnvironmentVarValue(i);
            if (val && *val) os << "=" << val;
        }
        os << "]";
    }

    os << ">";
    return os;
}

/////////////////////////////////////////
// Implementation ConfigMerger
/////////////////////////////////////////

// Private

ConstConfigMergerRcPtr ConfigMerger::Impl::Read(std::istream & istream, const char * filepath)
{
    OCIOMYaml ociomParser;
    ConfigMergerRcPtr merger;

    try
    {
        YAML::Node node = YAML::Load(istream);
        int numOfMerges = ociomParser.countMerges(node);

        merger = ConfigMerger::Create();

        // Create the number of ConfigMergingParametersRcPtr needed.
        for (int i = 0; i < numOfMerges; i++)
        {
            ConfigMergingParametersRcPtr params = ConfigMergingParameters::Create();
            merger->getImpl()->m_mergeParams.push_back(params);
        }

        ociomParser.load(node, merger, filepath);

        // Look at each set of Params and check if there are any 'Unspecified' sections.
        // If so, initialize them to the default strategy.
        // If there are no default, use PreferInput.
    }
    catch(const std::exception & e)
    {
        std::ostringstream os;
        os << "Error: Loading the OCIOM Merge parameters ";
        os << "'" << filepath << "' failed. " << e.what();
        throw Exception(os.str().c_str());
    }

    return merger;
}

ConstConfigRcPtr ConfigMerger::Impl::loadConfig(const char * value) const
{
    // Get the absolute path.
    StringUtils::StringVec searchpaths;

    if (m_searchPaths.size() == 0)
    {
        searchpaths.emplace_back(m_workingDir);
    }

    for (size_t i = 0; i < m_searchPaths.size(); ++i)
    {
        // Resolve variables in case the expansion adds slashes.
        const std::string path = m_searchPaths[i];

        // Remove trailing "/", and spaces.
        std::string dirname = StringUtils::RightTrim(StringUtils::Trim(path), '/');

        if (!pystring::os::path::isabs(dirname))
        {
            dirname = pystring::os::path::join(m_workingDir, dirname);
        }

        searchpaths.push_back(pystring::os::path::normpath(dirname));
    }

    for (size_t i = 0; i < searchpaths.size(); ++i)
    {
        try
        {
            // Try to load the provided config using the search paths.
            // Return as soon as they find a valid path.
            const std::string resolvedfullpath = pystring::os::path::join(searchpaths[i], 
                                                                          value);
            return Config::CreateFromFile(resolvedfullpath.c_str());
        }
        // TODO: If the file exists but won't load, this hides the error.
        // (Tried using ExceptionMissingFile, but the implementation of that is not what I
        // expected, Config::CreateFromFile only uses that if the argument is empty, not
        // if it can't read the file.)
        catch(...) { /* don't capture the exception */ }
    }

    // Try to load the provided base config name as a built-in config.
    try
    {
        // Check if the base config name is a built-in config.
        return Config::CreateFromBuiltinConfig(value);
    }
    catch(...) { /* don't capture the exception */ }

    // Must be a reference to a config from a previous merge.
    for (size_t i = 0; i < m_mergeParams.size(); i++)
    {
        if (Platform::Strcasecmp(m_mergeParams.at(i)->getOutputName(), value) == 0)
        {
            // Use the config from the index.
            return m_mergedConfigs.at(i);
        }
    }

    return nullptr;
}


// Public

ConfigMerger::ConfigMerger() : m_impl(new ConfigMerger::Impl())
{

}

ConfigMerger::~ConfigMerger()
{
    delete m_impl;
    m_impl = nullptr;
}

ConfigMergerRcPtr ConfigMerger::Create()
{
    return ConfigMergerRcPtr(new ConfigMerger(), &deleter);
}

ConstConfigMergerRcPtr ConfigMerger::CreateFromFile(const char * filepath)
{
    if (!filepath || !*filepath)
    {
        throw ExceptionMissingFile ("The merge options filepath is missing.");
    }

    std::ifstream ifstream = Platform::CreateInputFileStream(
        filepath, 
        std::ios_base::in | std::ios_base::binary
    );

    if (ifstream.fail())
    {
        std::ostringstream os;
        os << "Error could not read '" << filepath;
        os << "' merge options.";
        throw Exception (os.str().c_str());
    }

    return ConfigMerger::Impl::Read(ifstream, filepath);
}

ConfigMergerRcPtr ConfigMerger::createEditableCopy() const
{
    ConfigMergerRcPtr merger = ConfigMerger::Create();
    *merger->m_impl = *m_impl;
    return merger;
}

void ConfigMerger::deleter(ConfigMerger * c)
{
    delete c;
}

int ConfigMerger::getNumSearchPaths() const
{
    return (int)getImpl()->m_searchPaths.size();
}

const char * ConfigMerger::getSearchPath(int index) const
{
    if (index < 0 || index >= (int)getImpl()->m_searchPaths.size()) return "";
    return getImpl()->m_searchPaths[index].c_str();
}

void ConfigMerger::setSearchPath(const char * path)
{
    getImpl()->m_searchPaths = StringUtils::Split(path ? path : "", ':');
}

void ConfigMerger::addSearchPath(const char * path)
{
    if (path && *path)
    {
        getImpl()->m_searchPaths.emplace_back(path ? path : "");
    }
}

void ConfigMerger::setWorkingDir(const char * dirname)
{
    getImpl()->m_workingDir = dirname;
}

const char * ConfigMerger::getWorkingDir() const
{
    return getImpl()->m_workingDir.c_str();
}

ConfigMergingParametersRcPtr ConfigMerger::getParams(int index) const
{
    if (index >= 0 && index < static_cast<int>(getImpl()->m_mergeParams.size()))
    {
        return getImpl()->m_mergeParams.at(index);
    }
    return nullptr;
}

int ConfigMerger::getNumConfigMergingParameters() const
{
    return static_cast<int>(getImpl()->m_mergeParams.size());
}

void ConfigMerger::addParams(ConfigMergingParametersRcPtr params)
{
    getImpl()->m_mergeParams.push_back(params);
}

void ConfigMerger::serialize(std::ostream& os) const
{
    try
    {
        OCIOMYaml ociom;
        ociom.write(os, *this);
    }
    catch (const std::exception & e)
    {
        std::ostringstream error;
        error << "Error building YAML: " << e.what();
        throw Exception(error.str().c_str());
    }
}

std::ostream & operator<<(std::ostream & os, const ConfigMerger & m)
{
    m.serialize(os);
    return os;
}

unsigned int ConfigMerger::getMajorVersion() const
{
    return getImpl()->m_majorVersion;
}

unsigned int ConfigMerger::getMinorVersion() const
{
    return getImpl()->m_minorVersion;
}

void ConfigMerger::setVersion(unsigned int major, unsigned int minor)
{
    getImpl()->m_majorVersion = major;
    getImpl()->m_minorVersion = minor;
}

int ConfigMerger::getNumMergedConfigs() const
{
    return static_cast<int>(getImpl()->m_mergedConfigs.size());
}

ConstConfigRcPtr ConfigMerger::getMergedConfig() const
{
    return getMergedConfig(static_cast<int>(getImpl()->m_mergedConfigs.size() - 1));
}

ConstConfigRcPtr ConfigMerger::getMergedConfig(int index) const
{
    if (index >= 0 && index < static_cast<int>(getImpl()->m_mergedConfigs.size()))
    {
        return getImpl()->m_mergedConfigs.at(index);
    }
    return nullptr;
}

ConstConfigMergerRcPtr ConfigMerger::mergeConfigs() const
{
    ConfigMergerRcPtr editableMerger = this->createEditableCopy();

    for (int i = 0; i < getNumConfigMergingParameters(); i++)
    {
        ConfigMergingParametersRcPtr params = getImpl()->m_mergeParams[i];
        
        // Load base config.
        ConstConfigRcPtr baseCfg = editableMerger->getImpl()->loadConfig(params->getBaseConfigName());

        // Load input config.
        ConstConfigRcPtr inputCfg = editableMerger->getImpl()->loadConfig(params->getInputConfigName());

        if (baseCfg && inputCfg)
        {
            // The merged config must be initialized with a copy of the base config.
            ConfigRcPtr mergedConfig = baseCfg->createEditableCopy();

            // Process merge.
            try
            {
                MergeHandlerOptions options = { baseCfg, inputCfg, params, mergedConfig };
                GeneralMerger(options).merge();
                RolesMerger(options).merge();
                FileRulesMerger(options).merge();
                DisplayViewMerger(options).merge();
                ViewTransformsMerger(options).merge();
                LooksMerger(options).merge();
                ColorspacesMerger(options).merge();
                NamedTransformsMerger(options).merge();
            }
            catch(const Exception & e)
            {
                throw(e);
            }

            // Add new config object to m_mergedConfigs so they can be used for following merges.
            editableMerger->getImpl()->m_mergedConfigs.push_back(mergedConfig);

        }
        else
        {
            throw(Exception("Could not load the base or the input config"));
        }
    }

    return editableMerger;
}

namespace ConfigMergingHelpers
{

ConfigRcPtr MergeConfigs(const ConfigMergingParametersRcPtr & params,
                         const ConstConfigRcPtr & baseConfig,
                         const ConstConfigRcPtr & inputConfig)
{
    if (!baseConfig || !inputConfig)
    {
        throw(Exception("The input or base config was not set."));
    }

    // The merged config must be initialized with a copy of the base config.
    ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();

    // Process the merge.
    try
    {
        MergeHandlerOptions options = { baseConfig, inputConfig, params, mergedConfig };
        GeneralMerger(options).merge();
        RolesMerger(options).merge();
        FileRulesMerger(options).merge();
        DisplayViewMerger(options).merge();
        ViewTransformsMerger(options).merge();
        LooksMerger(options).merge();
        ColorspacesMerger(options).merge();
        NamedTransformsMerger(options).merge();
    }
    catch(const Exception & e)
    {
        throw(e);
    }

    return mergedConfig;
}

ConfigRcPtr MergeColorSpace(const ConfigMergingParametersRcPtr & params,
                            const ConstConfigRcPtr & baseConfig,
                            const ConstColorSpaceRcPtr & colorspace)
{
    if (!baseConfig || !colorspace)
    {
        throw(Exception("The base config or color space object was not set."));
    }

    // Create an input config and add the color space.
    ConfigRcPtr inputConfig = Config::Create();
    inputConfig->addColorSpace(colorspace);

    // The merged config must be initialized with a copy of the base config.
    ConfigRcPtr mergedConfig = baseConfig->createEditableCopy();

    // With only the color space, the reference space is unknown, so turn off
    // automatic reference space conversion to the reference space of the base config.
    ConfigMergingParametersRcPtr eParams = params->createEditableCopy();
    eParams->setAdjustInputReferenceSpace(false);

    // Process the merge.
    try
    {
        MergeHandlerOptions options = { baseConfig, inputConfig, eParams, mergedConfig };
        ColorspacesMerger(options).merge();
    }
    catch(const Exception & e)
    {
        throw(e);
    }

    return mergedConfig;
}

} // ConfigMergingHelpers

} // namespace OCIO_NAMESPACE
