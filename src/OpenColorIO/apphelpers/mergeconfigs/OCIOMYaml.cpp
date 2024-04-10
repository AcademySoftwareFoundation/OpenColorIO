// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <unordered_set>
#include <iostream>

#include <pystring.h>

#include <OpenColorIO/OpenColorIO.h>

#include "Logging.h"
#include "OCIOMYaml.h"
#include "ParseUtils.h"
#include "PathUtils.h"
#include "utils/StringUtils.h"
#include "yaml-cpp/yaml.h"

namespace OCIO_NAMESPACE
{

OCIOMYaml::OCIOMYaml()
{
    m_mergeStrategiesMap["PreferInput"] = ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT;
    m_mergeStrategiesMap["PreferBase"] = ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_BASE;
    m_mergeStrategiesMap["InputOnly"] = ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY;
    m_mergeStrategiesMap["BaseOnly"] = ConfigMergingParameters::MergeStrategies::STRATEGY_BASE_ONLY;
    m_mergeStrategiesMap["Remove"] = ConfigMergingParameters::MergeStrategies::STRATEGY_REMOVE;
}

inline void OCIOMYaml::load(const YAML::Node & node, std::string & x)
{
    try
    {
        x = node.as<std::string>();
    }
    catch (const std::exception & e)
    {
        std::ostringstream os;
        os << "At line " << (node.Mark().line + 1)
            << ", '" << node.Tag() << "' parsing string failed "
            << "with: " << e.what();
        throw Exception(os.str().c_str());
    }
}

inline void OCIOMYaml::load(const YAML::Node & node, std::vector<std::string> & x)
{
    try
    {
        x = node.as<std::vector<std::string>>();
    }
    catch (const std::exception & e)
    {
        std::ostringstream os;
        os << "At line " << (node.Mark().line + 1)
           << ", '" << node.Tag() << "' parsing StringVec failed "
           << "with: " << e.what();
        throw Exception(os.str().c_str());
    }
}

inline void OCIOMYaml::throwValueError(const std::string & nodeName,
                                       const YAML::Node & key,
                                       const std::string & msg)
{
    std::string keyName;
    load(key, keyName);

    std::ostringstream os;
    os << "At line " << (key.Mark().line + 1)
        << ", the value parsing of the property '" << keyName 
        << "' from '" << nodeName << "' failed: " << msg;

    throw Exception(os.str().c_str());
}

inline void OCIOMYaml::CheckDuplicates(const YAML::Node & node)
{
    std::unordered_set<std::string> keyset;

    for (YAML::const_iterator iter = node.begin(); iter != node.end(); ++iter)
    {
        const std::string & key = iter->first.as<std::string>();
        if (keyset.find(key) == keyset.end())
        {
            keyset.insert(key);
        }
        else
        {
            std::ostringstream os;
            os << "Key-value pair with key '" << key;
            os << "' specified more than once. ";
            throwValueError(node.Tag(), iter->first, os.str());
        }
    }
}
ConfigMergingParameters::MergeStrategies OCIOMYaml::strategyToEnum(const char * enumStr) const
{
    auto it = m_mergeStrategiesMap.find(enumStr);
    if (it != m_mergeStrategiesMap.end())
    {
        return it->second;
    }

    return ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET;
}


ConfigMergingParameters::MergeStrategies OCIOMYaml::genericStrategyHandler(const YAML::Node & pnode, const YAML::Node & node)
{
    if(node.Type() != YAML::NodeType::Map)
    {
        throwValueError(node.Tag(), pnode,
                        "The value type of a property 'strategy' needs to be a map.");
    }
    
    std::string strategy;
    for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
    {
        const std::string & prop = it->first.as<std::string>();
        const std::string & value = it->second.as<std::string>();

        if (prop == "strategy")
        {
            strategy = value;
        }
    }

    auto srategyEnum = strategyToEnum(strategy.c_str());
    if (srategyEnum == ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET)
    {
        std::ostringstream os;
        os << "The value '" << strategy;
        os << "' is not recognized. ";
        throwValueError(node.Tag(), pnode, os.str());
    }
    return srategyEnum;
}

void OCIOMYaml::loadOptions(const YAML::Node & node, ConfigMergingParametersRcPtr & params)
{
    CheckDuplicates(node);

    for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
    {
        const std::string & key = it->first.as<std::string>();
        if (key == "input_family_prefix")
        {
            params->setInputFamilyPrefix(it->second.as<std::string>().c_str());
        }
        else if (key == "base_family_prefix")
        {
           params->setBaseFamilyPrefix(it->second.as<std::string>().c_str());
        }
        else if (key == "input_first")
        {
            params->setInputFirst(it->second.as<bool>());
        }
        else if (key == "error_on_conflict")
        {
            params->setErrorOnConflict(it->second.as<bool>());
        }
        else if (key == "avoid_duplicates")
        {
            params->setAvoidDuplicates(it->second.as<bool>());
        }
        else if (key == "assume_common_reference_space")
        {
            params->setAssumeCommonReferenceSpace(it->second.as<bool>());
        }
        else if (key == "default_strategy")
        {
            const std::string & strategy = it->second.as<std::string>();
            auto srategyEnum = strategyToEnum(strategy.c_str());
            if (srategyEnum == ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET)
            {
                std::ostringstream os;
                os << "The value '" << strategy;
                os << "' is not recognized. ";
                throwValueError(node.Tag(), it->first, os.str());
            }
            params->setDefaultStrategy(srategyEnum);
        }
    }
}

void OCIOMYaml::loadOverrides(const YAML::Node & node, ConfigMergingParametersRcPtr & params)
{
    CheckDuplicates(node);

    std::string stringval;

    for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
    {
        const std::string & key = it->first.as<std::string>();

        if (it->second.IsNull() || !it->second.IsDefined()) continue;

        if (key == "name")
        {
            load(it->second, stringval);
            params->setName(stringval.c_str());
        }
        else if (key == "description")
        {
            load(it->second, stringval);
            params->setDescription(stringval.c_str());
        }
        else if (key == "search_path")
        {
            if (it->second.size() == 0)
            {
                load(it->second, stringval);
                params->setSearchPath(stringval.c_str());
            }
            else
            {
                std::vector<std::string> paths;
                load(it->second, paths);
                for (const auto & path : paths)
                {
                    params->addSearchPath(path.c_str());
                }
            }
        }
        else if (key == "environment")
        {
            if(it->second.Type() != YAML::NodeType::Map)
            {
                throwValueError(node.Tag(), it->first, 
                                "The value type of key 'environment' needs to be a map.");
            }
            for (YAML::const_iterator itEnv = it->second.begin(); itEnv != it->second.end(); ++itEnv)
            {
                const std::string & k = itEnv->first.as<std::string>();
                const std::string & v = itEnv->second.as<std::string>();
                params->addEnvironmentVar(k.c_str(), v.c_str());
            }
        }
        else if (key == "active_displays")
        {
            std::vector<std::string> display;
            load(it->second, display);
            std::string displays = StringUtils::Join(display, ',');
            params->setActiveDisplays(displays.c_str());
        }
        else if (key == "active_views")
        {
            std::vector<std::string> view;
            load(it->second, view);
            std::string views = StringUtils::Join(view, ',');
            params->setActiveViews(views.c_str());
        }
        else if (key == "inactive_colorspaces")
        {
            std::vector<std::string> inactiveCSs;
            load(it->second, inactiveCSs);
            const std::string inactivecCSsStr = StringUtils::Join(inactiveCSs, ',');
            params->setInactiveColorspaces(inactivecCSsStr.c_str());
        }
    }
}

void OCIOMYaml::loadParams(const YAML::Node & node, ConfigMergingParametersRcPtr & params)
{
    // Check for duplicates in params.
    CheckDuplicates(node);

    for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
    {
        const std::string & key = it->first.as<std::string>();
        if (key == "roles")
        {
            params->setRoles(genericStrategyHandler(it->first, it->second));
        }
        else if (key == "file_rules")
        {
            params->setFileRules(genericStrategyHandler(it->first, it->second));
        }
        else if (key == "display-views")
        {
            params->setDisplayViews(genericStrategyHandler(it->first, it->second));
        }  
        else if (key == "looks")
        {
            params->setLooks(genericStrategyHandler(it->first, it->second));
        }              
        else if (key == "colorspaces")
        {
            params->setColorspaces(genericStrategyHandler(it->first, it->second));
        }   
        else if (key == "named_transform")
        {
            params->setNamedTransforms(genericStrategyHandler(it->first, it->second));
        }                                                          
        else
        {
            // Handle unsupported property or use default handler.
            std::cout << "Unsupported property : " << key << std::endl;
        }
    }
}

void OCIOMYaml::load(const YAML::Node& node, ConfigMergerRcPtr & merger, const char * filename)
{
    CheckDuplicates(node);

    // Parse all properties.
    for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
    {
        const std::string & key = it->first.as<std::string>();

        if (it->second.IsNull() || !it->second.IsDefined()) continue;

        if (key == "ociom_version")
        {
            std::string version;
            std::vector<std::string> results;

            load(node["ociom_version"], version);
            results = StringUtils::Split(version, '.');

            if(results.size() == 1)
            {
                merger->setMajorVersion(std::stoi(results[0].c_str()));
            }
            else if(results.size() == 2)
            {
                merger->setMajorVersion(std::stoi(results[0].c_str()));
                merger->setMinorVersion(std::stoi(results[1].c_str()));
            }
        }
        else if (key == "search_path")
        {
            if (it->second.size() == 0)
            {
                std::string stringval;
                load(it->second, stringval);
                merger->setSearchPath(stringval.c_str());
            }
            else
            {
                StringUtils::StringVec paths;
                load(it->second, paths);
                for (const auto & path : paths)
                {
                    merger->addSearchPath(path.c_str());
                }
            }
        }
        else if (key == "merge")
        {
            if(it->second.Type() != YAML::NodeType::Map)
            {
                throwValueError(it->second.Tag(), it->first,
                                "The value type of the key 'merge' needs to be a map.");
            }

            int mergesCounter = 0;
            for (YAML::const_iterator mergeIt = it->second.begin(); mergeIt != it->second.end(); ++mergeIt)
            {
                const std::string & mergedConfigName = mergeIt->first.as<std::string>();
                ConfigMergingParametersRcPtr params = merger->getParams(mergesCounter);
                params->setOutputName(mergedConfigName.c_str());

                for (YAML::const_iterator paramsIt = mergeIt->second.begin(); paramsIt != mergeIt->second.end(); ++paramsIt)
                {
                    const std::string & key = paramsIt->first.as<std::string>();
                    
                    if (key == "base")
                    {
                        params->setBaseConfigName(paramsIt->second.as<std::string>().c_str());
                    }
                    else if (key == "input")
                    {
                        params->setInputConfigName(paramsIt->second.as<std::string>().c_str());
                    }
                    else if (key == "options")
                    {
                        loadOptions(paramsIt->second, params);
                    }
                    else if (key == "overrides")
                    {
                        loadOverrides(paramsIt->second, params);
                    }
                    else if (key == "params")
                    {
                        loadParams(paramsIt->second, params);
                    }
                }
                mergesCounter++;
            }
        }

        if (filename && filename[0])
        {
            // Working directory defaults to the directory of the OCIOM file.
            std::string realfilename = AbsPath(filename);
            std::string configrootdir = pystring::os::path::dirname(realfilename);
            merger->setWorkingDir(configrootdir.c_str());
        }
    }    
}

int OCIOMYaml::countMerges(const YAML::Node& node)
{
    int numOfMerges = 0;

    CheckDuplicates(node);

    // Parse all properties.
    for (YAML::const_iterator it = node.begin(); it != node.end(); ++it)
    {
        const std::string & key = it->first.as<std::string>();

        if (it->second.IsNull() || !it->second.IsDefined()) continue;

        if (key == "merge")
        {
            if(it->second.Type() != YAML::NodeType::Map)
            {
                throwValueError(it->second.Tag(), it->first,
                                "The value type of the key 'merge' needs to be a map.");
            }

            CheckDuplicates(it->second);

            for (YAML::const_iterator mergeIt = it->second.begin(); mergeIt != it->second.end(); ++mergeIt)
            {
                numOfMerges++;
            }
        }
    }

    return numOfMerges;
}

void OCIOMYaml::read(std::istream & istream, ConfigMergerRcPtr & merger, const char * filepath)
{
    try
    {
        YAML::Node node = YAML::Load(istream);
        load(node, merger, filepath);
    }
    catch(const std::exception & e)
    {
        std::ostringstream os;
        os << "Error: Loading the OCIOM Merge parameters ";
        os << "'" << filepath << "' failed. " << e.what();
        throw Exception(os.str().c_str());
    }
}

const char * stategyEnumToString(ConfigMergingParameters::MergeStrategies strategy)
{
    switch (strategy)
    {
        case ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT:
            return "PreferInput";
            break;
        case ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_BASE:
            return "PreferBase";
            break;
        case ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY:
            return "InputOnly";
            break;
        case ConfigMergingParameters::MergeStrategies::STRATEGY_BASE_ONLY:
            return "BaseOnly";
            break;
        case ConfigMergingParameters::MergeStrategies::STRATEGY_REMOVE:
            return "Remove";
            break;
        case ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET:
            return "Unset";
            break;
        default:
            return "Unknown";
            break;
    }
}

inline void save(YAML::Emitter & out, const ConfigMerger & merger)
{
    std::stringstream ss;
    const unsigned parserMajorVersion = merger.getMajorVersion();
    ss << parserMajorVersion;
    if (merger.getMinorVersion() != 0)
    {
        ss << "." << merger.getMinorVersion();
    }

    out << YAML::Block;
    out << YAML::BeginMap;
    out << YAML::Key << "ociom_version" << YAML::Value << ss.str();
    out << YAML::Key << "search_path";
    out << YAML::Value << YAML::BeginSeq;
    for (int i = 0; i < merger.getNumSearchPaths(); i++)
    {
        out << YAML::Value << merger.getSearchPath(i);
    }
    out << YAML::EndSeq;
    out << YAML::Newline;

    out << YAML::Key << "merge";
    out << YAML::Value << YAML::BeginMap;

    for (int mp = 0; mp < merger.getNumOfConfigMergingParameters(); mp++)
    {
        // Serialized every merge section.
        ConfigMergingParametersRcPtr p = merger.getParams(mp);
        out << YAML::Key << p->getOutputName();
        out << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "base" << YAML::Value << p->getBaseConfigName();
        out << YAML::Key << "input" << YAML::Value << p->getInputConfigName();
        out << YAML::Newline;

        out << YAML::Key << "options";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "input_family_prefix" << YAML::Value << p->getInputFamilyPrefix();
        out << YAML::Key << "base_family_prefix" << YAML::Value << p->getBaseFamilyPrefix();
        out << YAML::Key << "input_first" << YAML::Value << p->isInputFirst();
        out << YAML::Key << "error_on_conflict" << YAML::Value << p->isErrorOnConflict();
        out << YAML::Key << "default_strategy" << YAML::Value << stategyEnumToString(p->getDefaultStrategy());
        out << YAML::Key << "avoid_duplicates" << YAML::Value << p->isAvoidDuplicates();
        out << YAML::Key << "assume_common_reference_space" << YAML::Value << p->isAssumeCommonReferenceSpace();
        // End of options section.
        out << YAML::EndMap;
        out << YAML::Newline;

        out << YAML::Key << "overrides";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << p->getName();
        out << YAML::Key << "description" << YAML::Value << p->getDescription();
        out << YAML::Key << "search_path" << YAML::Value << p->getSearchPath();

        out << YAML::Key << "environment";
        out << YAML::Value << YAML::BeginMap;
        for(int i = 0; i < p->getNumEnvironmentVars(); ++i)
        {   
            const char* name = p->getEnvironmentVar(i);
            out << YAML::Key << name;
            out << YAML::Value << p->getEnvironmentVarValue(i);
        }
        out << YAML::EndMap;
        out << YAML::Newline;

        out << YAML::Key << "active_displays";
        StringUtils::StringVec active_displays;
        if (p->getActiveDisplays() != NULL && strlen(p->getActiveDisplays()) > 0)
            active_displays = SplitStringEnvStyle(p->getActiveDisplays());
        out << YAML::Value << YAML::Flow << active_displays;
        out << YAML::Newline;

        out << YAML::Key << "active_views";
        StringUtils::StringVec active_views;
        if (p->getActiveViews() != NULL && strlen(p->getActiveViews()) > 0)
            active_views = SplitStringEnvStyle(p->getActiveViews());
        out << YAML::Value << YAML::Flow << active_views;

        out << YAML::Key << "inactive_colorspaces";
        StringUtils::StringVec inactive_colorspaces;
        if (p->getInactiveColorSpaces() != NULL && strlen(p->getInactiveColorSpaces()) > 0)
            inactive_colorspaces = SplitStringEnvStyle(p->getInactiveColorSpaces());
        out << YAML::Value << YAML::Flow << inactive_colorspaces;

        // End of overrides section.
        out << YAML::EndMap;
        out << YAML::Newline;

        out << YAML::Key << "params";
        out << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "roles";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "strategy" << YAML::Value << stategyEnumToString(p->getRoles());
        out << YAML::EndMap;

        out << YAML::Key << "file_rules";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "strategy" << YAML::Value << stategyEnumToString(p->getFileRules());
        out << YAML::EndMap;

        out << YAML::Key << "display-views";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "strategy" << YAML::Value << stategyEnumToString(p->getDisplayViews());
        out << YAML::EndMap;

        out << YAML::Key << "looks";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "strategy" << YAML::Value << stategyEnumToString(p->getLooks());
        out << YAML::EndMap;

        out << YAML::Key << "colorspaces";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "strategy" << YAML::Value << stategyEnumToString(p->getColorspaces());
        out << YAML::EndMap;

        out << YAML::Key << "named_transform";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "strategy" << YAML::Value << stategyEnumToString(p->getNamedTransforms());
        out << YAML::EndMap;

        // End of params section.
        out << YAML::EndMap;

        // End of the current merge section.
        out << YAML::EndMap;    
    }

    // End of the merges.
    out << YAML::EndMap;

    out << YAML::EndMap;
}

void OCIOMYaml::write(std::ostream & ostream, const ConfigMerger & merger)
{
    YAML::Emitter out;
    save(out, merger);
    ostream << out.c_str();
}

} // namespace OCIO_NAMESPACE
