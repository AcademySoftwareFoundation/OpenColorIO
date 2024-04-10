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

//
// Config merging feature
//

/// Private implementation section

// ...

/// Public implementation section

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

//TODO Not implemented
std::ostream & operator<<(std::ostream & os, const ConfigMergingParameters & ms)
{
    (void)ms;
    return os;
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

void ConfigMergingParameters::setInactiveColorspaces(const char * colorspaces)
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

void ConfigMergingParameters::setAssumeCommonReferenceSpace(bool enabled)
{
    getImpl()->m_assumeCommonReferenceSpace = enabled;
}

bool ConfigMergingParameters::isAssumeCommonReferenceSpace() const
{
    return getImpl()->m_assumeCommonReferenceSpace;
}

void ConfigMergingParameters::setRoles(MergeStrategies strategy)
{
    getImpl()->m_roles = strategy;
}

ConfigMergingParameters::MergeStrategies ConfigMergingParameters::getRoles() const
{
    if (getImpl()->m_roles == MergeStrategies::STRATEGY_UNSET)
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
    if (getImpl()->m_fileRules == MergeStrategies::STRATEGY_UNSET)
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
    if (getImpl()->m_displayViews == MergeStrategies::STRATEGY_UNSET)
    {
        return getDefaultStrategy();
    }
    return getImpl()->m_displayViews;
}

void ConfigMergingParameters::setLooks(MergeStrategies strategy)
{
    getImpl()->m_looks = strategy;
}

ConfigMergingParameters::MergeStrategies ConfigMergingParameters::getLooks() const
{
    if (getImpl()->m_looks == MergeStrategies::STRATEGY_UNSET)
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
    if (getImpl()->m_colorspaces == MergeStrategies::STRATEGY_UNSET)
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
    if (getImpl()->m_namedTransforms == MergeStrategies::STRATEGY_UNSET)
    {
        return getDefaultStrategy();
    }
    return getImpl()->m_namedTransforms;
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

        // Look at each set of Params and check if there are any 'Unset' sections.
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
    try
    {
        // Try to load the provided config name as a file.
        return Config::CreateFromFile(value);
    }
    catch(...) { /* don't capture the exception */ }

    // Try to load the provided base config name as a built-in config.
    try
    {
        // Check if the base config name is a built-in config.
        return Config::CreateFromBuiltinConfig(value);
    }
    catch(...) { /* don't capture the exception */ }

    // Must be a reference to a config from a previous merge.
    for (int i = 0; i < getNumOfConfigMergingParameters(); i++)
    {
        if (Platform::Strcasecmp(getParams(i)->getOutputName(), value) == 0)
        {
            // Use the config from the index.
            if (i < (int) m_mergedConfigs.size())
            {
                return m_mergedConfigs.at(i);
            }
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
    if (index < static_cast<int>(getImpl()->m_mergeParams.size()))
    {
        return getImpl()->m_mergeParams.at(index);
    }
    return nullptr;
}

int ConfigMerger::getNumOfConfigMergingParameters() const
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

unsigned int ConfigMerger::getMajorVersion() const
{
    return getImpl()->m_majorVersion;
}

void ConfigMerger::setMajorVersion(unsigned int version)
{
    getImpl()->m_majorVersion = version;
}

unsigned int ConfigMerger::getMinorVersion() const
{
    return getImpl()->m_minorVersion;
}

void ConfigMerger::setMinorVersion(unsigned int version)
{
    getImpl()->m_minorVersion = version;
}

void ConfigMerger::setVersion(unsigned int major, unsigned int minor)
{
    setMajorVersion(major);
    setMinorVersion(minor);
}

void ConfigMerger::addMergedConfig(ConstConfigRcPtr cfg)
{
    getImpl()->m_mergedConfigs.push_back(cfg);
}

ConstConfigRcPtr ConfigMerger::getMergedConfig() const
{
    return getMergedConfig(static_cast<int>(getImpl()->m_mergedConfigs.size() - 1));
}

ConstConfigRcPtr ConfigMerger::getMergedConfig(int index) const
{
    if (index < static_cast<int>(getImpl()->m_mergedConfigs.size()))
    {
        return getImpl()->m_mergedConfigs.at(index);
    }
    return nullptr;
}

namespace ConfigMergingHelpers
{

ConstConfigRcPtr loadConfig(const ConfigMergerRcPtr merger, 
                            const char * value)
{
    // Get the absolute path.
    StringUtils::StringVec searchpaths;

    if (merger->getNumSearchPaths() == 0)
    {
        merger->addSearchPath(merger->getWorkingDir());
    }

    for (int i = 0; i < merger->getNumSearchPaths(); ++i)
    {
        // Resolve variables in case the expansion adds slashes.
        const std::string path = merger->getSearchPath(i);

        // Remove trailing "/", and spaces.
        std::string dirname = StringUtils::RightTrim(StringUtils::Trim(path), '/');

        if (!pystring::os::path::isabs(dirname))
        {
            dirname = pystring::os::path::join(merger->getWorkingDir(), dirname);
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
    for (int i = 0; i < merger->getNumOfConfigMergingParameters(); i++)
    {
        if (Platform::Strcasecmp(merger->getParams(i)->getOutputName(), value) == 0)
        {
            // Use the config from the index.
            return merger->getMergedConfig(i);
        }
    }

    return nullptr;
}

ConstConfigMergerRcPtr MergeConfigs(const ConstConfigMergerRcPtr & merger)
{
    ConfigMergerRcPtr editableMerger = merger->createEditableCopy();

    for (int i = 0; i < merger->getNumOfConfigMergingParameters(); i++)
    {
        ConstConfigMergingParametersRcPtr params = merger->getParams(i);
        
        // Load base config.
        ConstConfigRcPtr baseCfg = loadConfig(editableMerger, params->getBaseConfigName());

        // Load input config.
        ConstConfigRcPtr inputCfg = loadConfig(editableMerger, params->getInputConfigName());

        if (baseCfg && inputCfg)
        {
            // The merged config must be initialized with a copy of the base config.
            ConfigRcPtr mergedConfig = baseCfg->createEditableCopy();

            // Process merge.
            try
            {
                MergeHandlerOptions options = { baseCfg, inputCfg, merger->getParams(i), mergedConfig };
                GeneralMerger(options).merge();
                RolesMerger(options).merge();
                FileRulesMerger(options).merge();
                DisplayViewMerger(options).merge();
                LooksMerger(options).merge();
                ColorspacesMerger(options).merge();
                NamedTransformsMerger(options).merge();
            }
            catch(const Exception & e)
            {
                throw(e);
            }

            // Add new config object to m_mergedConfigs so they can be used for following merges.
            editableMerger->addMergedConfig(mergedConfig);
        }
        else
        {
            throw(Exception("Could not load the base or the input config"));
        }
    }

    return editableMerger;
}

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
    eParams->setAssumeCommonReferenceSpace(true);

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
