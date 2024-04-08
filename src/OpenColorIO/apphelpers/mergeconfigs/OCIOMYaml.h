// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <unordered_map>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
#include "yaml-cpp/yaml.h"

#ifndef INCLUDED_OCIOM_YAML_H
#define INCLUDED_OCIOM_YAML_H

namespace OCIO_NAMESPACE
{

// Handles the OCIOM file parsing.
class OCIOMYaml
{
public:
    OCIOMYaml();
    ~OCIOMYaml() = default;

    void throwValueError(const std::string & nodeName, 
                         const YAML::Node & key, 
                         const std::string & msg);
    void CheckDuplicates(const YAML::Node & node);
    
    /**
     * \brief Load the options section.
     */
    void loadOptions(const YAML::Node & node, ConfigMergingParametersRcPtr & params);
    /**
     * \brief Load the overrides section.
     */
    void loadOverrides(const YAML::Node & node, ConfigMergingParametersRcPtr & params);
    /**
     * \brief Load the params section.
     */
    void loadParams(const YAML::Node & node, ConfigMergingParametersRcPtr & params);

    ConfigMergingParameters::MergeStrategies genericStrategyHandler(const YAML::Node & pnode, const YAML::Node & node);

    /**
     * \brief Converts a string into a ConfigMergingParameters::MergeStrategies enum.
     */
    ConfigMergingParameters::MergeStrategies strategyToEnum(const char * enumStr) const;

    void read(std::istream & istream, ConfigMergerRcPtr & merger, const char * filepath);
    void write(std::ostream & ostream, const ConfigMerger & merger);

    void load(const YAML::Node & node, std::string & x);
    void load(const YAML::Node & node, std::vector<std::string> & x);

    /**
     * \brief Load a OCIOM file.
     */
    void load(const YAML::Node& node, ConfigMergerRcPtr & merger, const char * filename);
    
    /**
     * \brief Counts the number of merges in an OCIOM file to calculate the right number of
     *        objects to create.
     */
    int countMerges(const YAML::Node& node);

private:
    std::unordered_map<std::string, ConfigMergingParameters::MergeStrategies> m_mergeStrategiesMap;
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIOM_YAML_H
