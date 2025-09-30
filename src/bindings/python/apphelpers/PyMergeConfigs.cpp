// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

//void bindPyConfigMergingParameters(py::module & m);

void bindPyConfigMergingHelpers(py::module & m)
{
    auto mConfigMergingHelpers = m.def_submodule("ConfigMergingHelpers");

    auto clsConfigMergingParameters = py::class_<ConfigMergingParameters, ConfigMergingParametersRcPtr>(
           m.attr("ConfigMergingParameters"));

    auto enumMergeStrategies = 
        py::enum_<ConfigMergingParameters::MergeStrategies>(
            clsConfigMergingParameters, "MergeStrategies", 
            DOC(ConfigMergingParameters, MergeStrategies));

    clsConfigMergingParameters
        .def_static("Create", &ConfigMergingParameters::Create,
            DOC(ConfigMergingParameters, Create))
        .def("__deepcopy__", [](const ConstConfigMergingParametersRcPtr & self, py::dict)
            {
                return self->createEditableCopy();
            },
            "memo"_a)
        .def("setBaseConfigName", &ConfigMergingParameters::setBaseConfigName, "baseConfig"_a,
            DOC(ConfigMergingParameters, setBaseConfigName))
        .def("getBaseConfigName", &ConfigMergingParameters::getBaseConfigName,
            DOC(ConfigMergingParameters, getBaseConfigName))
        .def("setInputConfigName", &ConfigMergingParameters::setInputConfigName, "inputConfig"_a,
            DOC(ConfigMergingParameters, setInputConfigName))
        .def("getInputConfigName", &ConfigMergingParameters::getInputConfigName,
            DOC(ConfigMergingParameters, getInputConfigName))
        .def("setOutputName", &ConfigMergingParameters::setOutputName, "outputName"_a,
            DOC(ConfigMergingParameters, setOutputName))
        .def("getOutputName", &ConfigMergingParameters::getOutputName,
            DOC(ConfigMergingParameters, getOutputName))
        .def("setDefaultStrategy", &ConfigMergingParameters::setDefaultStrategy, "strategy"_a,
            DOC(ConfigMergingParameters, setDefaultStrategy))
        .def("getDefaultStrategy", &ConfigMergingParameters::getDefaultStrategy,
            DOC(ConfigMergingParameters, getDefaultStrategy))
        .def("setInputFamilyPrefix", &ConfigMergingParameters::setInputFamilyPrefix, "prefix"_a,
            DOC(ConfigMergingParameters, setInputFamilyPrefix))
        .def("getInputFamilyPrefix", &ConfigMergingParameters::getInputFamilyPrefix,
            DOC(ConfigMergingParameters, getInputFamilyPrefix))
        .def("setBaseFamilyPrefix", &ConfigMergingParameters::setBaseFamilyPrefix, "prefix"_a,
            DOC(ConfigMergingParameters, setBaseFamilyPrefix))
        .def("getBaseFamilyPrefix", &ConfigMergingParameters::getBaseFamilyPrefix,
            DOC(ConfigMergingParameters, getBaseFamilyPrefix))
        .def("setInputFirst", &ConfigMergingParameters::setInputFirst, "enabled"_a,
            DOC(ConfigMergingParameters, setInputFirst))
        .def("isInputFirst", &ConfigMergingParameters::isInputFirst,
            DOC(ConfigMergingParameters, isInputFirst))
        .def("setErrorOnConflict", &ConfigMergingParameters::setErrorOnConflict, "enabled"_a,
            DOC(ConfigMergingParameters, setErrorOnConflict))
        .def("isErrorOnConflict", &ConfigMergingParameters::isErrorOnConflict,
            DOC(ConfigMergingParameters, isErrorOnConflict))
        .def("setAvoidDuplicates", &ConfigMergingParameters::setAvoidDuplicates, "enabled"_a,
            DOC(ConfigMergingParameters, setAvoidDuplicates))
        .def("isAvoidDuplicates", &ConfigMergingParameters::isAvoidDuplicates,
            DOC(ConfigMergingParameters, isAvoidDuplicates))
        .def("setAdjustInputReferenceSpace", &ConfigMergingParameters::setAdjustInputReferenceSpace, "enabled"_a,
            DOC(ConfigMergingParameters, setAdjustInputReferenceSpace))
        .def("isAdjustInputReferenceSpace", &ConfigMergingParameters::isAdjustInputReferenceSpace,
            DOC(ConfigMergingParameters, isAdjustInputReferenceSpace))
        .def("setName", &ConfigMergingParameters::setName, "mergedConfigName"_a,
            DOC(ConfigMergingParameters, setName))
        .def("getName", &ConfigMergingParameters::getName,
            DOC(ConfigMergingParameters, getName))
        .def("setDescription", &ConfigMergingParameters::setDescription, "mergedConfigDesc"_a,
            DOC(ConfigMergingParameters, setDescription))
        .def("getDescription", &ConfigMergingParameters::getDescription,
            DOC(ConfigMergingParameters, getDescription))

        .def("addEnvironmentVar", &ConfigMergingParameters::addEnvironmentVar, "name"_a, "defaultValue"_a,
            DOC(ConfigMergingParameters, addEnvironmentVar))
        .def("getNumEnvironmentVars", &ConfigMergingParameters::getNumEnvironmentVars,
            DOC(ConfigMergingParameters, getNumEnvironmentVars))
        .def("getEnvironmentVar", &ConfigMergingParameters::getEnvironmentVar, "index"_a,
            DOC(ConfigMergingParameters, getEnvironmentVar))
        .def("getEnvironmentVarValue", &ConfigMergingParameters::getEnvironmentVarValue, "index"_a,
            DOC(ConfigMergingParameters, getEnvironmentVarValue))

        .def("setSearchPath", &ConfigMergingParameters::setSearchPath, "path"_a,
            DOC(ConfigMergingParameters, setSearchPath))
        .def("addSearchPath", &ConfigMergingParameters::addSearchPath, "path"_a,
            DOC(ConfigMergingParameters, addSearchPath))
        .def("getSearchPath", &ConfigMergingParameters::getSearchPath,
            DOC(ConfigMergingParameters, getSearchPath))

        .def("setActiveDisplays", &ConfigMergingParameters::setActiveDisplays, "displays"_a,
            DOC(ConfigMergingParameters, setActiveDisplays))
        .def("getActiveDisplays", &ConfigMergingParameters::getActiveDisplays,
            DOC(ConfigMergingParameters, getActiveDisplays))
        .def("setActiveViews", &ConfigMergingParameters::setActiveViews, "views"_a,
            DOC(ConfigMergingParameters, setActiveViews))
        .def("getActiveViews", &ConfigMergingParameters::getActiveViews,
            DOC(ConfigMergingParameters, getActiveViews))
        .def("setInactiveColorSpaces", &ConfigMergingParameters::setInactiveColorSpaces, "colorspaces"_a,
            DOC(ConfigMergingParameters, setInactiveColorSpaces))
        .def("getInactiveColorSpaces", &ConfigMergingParameters::getInactiveColorSpaces,
            DOC(ConfigMergingParameters, getInactiveColorSpaces))

        .def("setRoles", &ConfigMergingParameters::setRoles, "strategy"_a,
            DOC(ConfigMergingParameters, setRoles))
        .def("getRoles", &ConfigMergingParameters::getRoles,
            DOC(ConfigMergingParameters, getRoles))
        .def("setFileRules", &ConfigMergingParameters::setFileRules, "strategy"_a,
            DOC(ConfigMergingParameters, setFileRules))
        .def("getFileRules", &ConfigMergingParameters::getFileRules,
            DOC(ConfigMergingParameters, getFileRules))
        .def("setDisplayViews", &ConfigMergingParameters::setDisplayViews, "strategy"_a,
            DOC(ConfigMergingParameters, setDisplayViews))
        .def("getDisplayViews", &ConfigMergingParameters::getDisplayViews,
            DOC(ConfigMergingParameters, getDisplayViews))
        .def("setViewTransforms", &ConfigMergingParameters::setViewTransforms, "strategy"_a,
            DOC(ConfigMergingParameters, setViewTransforms))
        .def("getViewTransforms", &ConfigMergingParameters::getViewTransforms,
            DOC(ConfigMergingParameters, getViewTransforms))
        .def("setLooks", &ConfigMergingParameters::setLooks, "strategy"_a,
            DOC(ConfigMergingParameters, setLooks))
        .def("getLooks", &ConfigMergingParameters::getLooks,
            DOC(ConfigMergingParameters, getLooks))
        .def("setColorspaces", &ConfigMergingParameters::setColorspaces, "strategy"_a,
            DOC(ConfigMergingParameters, setColorspaces))
        .def("getColorspaces", &ConfigMergingParameters::getColorspaces,
            DOC(ConfigMergingParameters, getColorspaces))
        .def("setNamedTransforms", &ConfigMergingParameters::setNamedTransforms, "strategy"_a,
            DOC(ConfigMergingParameters, setNamedTransforms))
        .def("getNamedTransforms", &ConfigMergingParameters::getNamedTransforms,
            DOC(ConfigMergingParameters, getNamedTransforms));

    enumMergeStrategies
        .value("STRATEGY_PREFER_INPUT", ConfigMergingParameters::STRATEGY_PREFER_INPUT,
               DOC(ConfigMergingParameters, MergeStrategies, STRATEGY_PREFER_INPUT))
        .value("STRATEGY_PREFER_BASE", ConfigMergingParameters::STRATEGY_PREFER_BASE,
               DOC(ConfigMergingParameters, MergeStrategies, STRATEGY_PREFER_BASE))
        .value("STRATEGY_INPUT_ONLY", ConfigMergingParameters::STRATEGY_INPUT_ONLY,
               DOC(ConfigMergingParameters, MergeStrategies, STRATEGY_INPUT_ONLY))
        .value("STRATEGY_BASE_ONLY", ConfigMergingParameters::STRATEGY_BASE_ONLY,
               DOC(ConfigMergingParameters, MergeStrategies, STRATEGY_BASE_ONLY))
        .value("STRATEGY_REMOVE", ConfigMergingParameters::STRATEGY_REMOVE,
               DOC(ConfigMergingParameters, MergeStrategies, STRATEGY_REMOVE))
        .value("STRATEGY_UNSPECIFIED", ConfigMergingParameters::STRATEGY_UNSPECIFIED,
               DOC(ConfigMergingParameters, MergeStrategies, STRATEGY_UNSPECIFIED))
        .export_values();

    defRepr(clsConfigMergingParameters);

    // ConfigMerger binding
    auto clsConfigMerger = py::class_<ConfigMerger, ConfigMergerRcPtr>(
             m.attr("ConfigMerger"));

    clsConfigMerger
        .def_static("Create", &ConfigMerger::Create,
            DOC(ConfigMerger, Create))
        .def_static("CreateFromFile", &ConfigMerger::CreateFromFile, "filepath"_a,
            DOC(ConfigMerger, CreateFromFile))
        .def("__deepcopy__", [](const ConstConfigMergerRcPtr & self, py::dict)
            {
                return self->createEditableCopy();
            },
            "memo"_a)

        .def("setSearchPath", &ConfigMerger::setSearchPath, "path"_a,
            DOC(ConfigMerger, setSearchPath))
        .def("addSearchPath", &ConfigMerger::addSearchPath, "path"_a,
            DOC(ConfigMerger, addSearchPath))
        .def("getNumSearchPaths", &ConfigMerger::getNumSearchPaths,
            DOC(ConfigMerger, getNumSearchPaths))
        .def("getSearchPath", &ConfigMerger::getSearchPath, "index"_a,
            DOC(ConfigMerger, getSearchPath))

        .def("setWorkingDir", &ConfigMerger::setWorkingDir, "dirname"_a,
            DOC(ConfigMerger, setWorkingDir))
        .def("getWorkingDir", &ConfigMerger::getWorkingDir,
            DOC(ConfigMerger, getWorkingDir))
        .def("getParams", &ConfigMerger::getParams, "index"_a,
            DOC(ConfigMerger, getParams))
        .def("getNumConfigMergingParameters", &ConfigMerger::getNumConfigMergingParameters,
            DOC(ConfigMerger, getNumConfigMergingParameters))
        .def("addParams", &ConfigMerger::addParams, "params"_a,
            DOC(ConfigMerger, addParams))
        .def("mergeConfigs", &ConfigMerger::mergeConfigs,
            DOC(ConfigMerger, mergeConfigs))
        .def("getNumMergedConfigs", &ConfigMerger::getNumMergedConfigs,
            DOC(ConfigMerger, getNumMergedConfigs))
        .def("getMergedConfig", 
            py::overload_cast<>(&ConfigMerger::getMergedConfig, py::const_),
            DOC(ConfigMerger, getMergedConfig))
        .def("getMergedConfig", 
            py::overload_cast<int>(&ConfigMerger::getMergedConfig, py::const_),
            "index"_a,
            DOC(ConfigMerger, getMergedConfig))
        .def("serialize", [](const ConfigMerger & self) {
                std::ostringstream os;
                self.serialize(os);
                return os.str();
            }, DOC(ConfigMerger, serialize))
        .def("setVersion", &ConfigMerger::setVersion, "major"_a, "minor"_a,
            DOC(ConfigMerger, setVersion))
        .def("getMajorVersion", &ConfigMerger::getMajorVersion,
            DOC(ConfigMerger, getMajorVersion))
        .def("getMinorVersion", &ConfigMerger::getMinorVersion,
            DOC(ConfigMerger, getMinorVersion));

    defRepr(clsConfigMerger);

    // ConfigMergingHelpers namespace functions
    mConfigMergingHelpers
        .def("MergeConfigs", 
             py::overload_cast<const ConfigMergingParametersRcPtr &,
                               const ConstConfigRcPtr &,
                               const ConstConfigRcPtr &>(
                &ConfigMergingHelpers::MergeConfigs),
             "params"_a, "baseConfig"_a, "inputConfig"_a,
             DOC(ConfigMergingHelpers, MergeConfigs))
        .def("MergeColorSpace",
             &ConfigMergingHelpers::MergeColorSpace,
             "params"_a, "baseConfig"_a, "colorspace"_a,
             DOC(ConfigMergingHelpers, MergeColorSpace));
}

} // namespace OCIO_NAMESPACE
