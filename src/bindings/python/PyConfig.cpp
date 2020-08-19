// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <fstream>
#include <sstream>

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{
namespace
{

enum ConfigIterator
{
    IT_ENVIRONMENT_VAR_NAME = 0,
    IT_SEARCH_PATH,
    IT_COLOR_SPACE_NAME,
    IT_COLOR_SPACE,
    IT_ACTIVE_COLOR_SPACE_NAME,
    IT_ACTIVE_COLOR_SPACE,
    IT_ROLE_NAME,
    IT_ROLE_COLOR_SPACE,
    IT_DISPLAY,
    IT_SHARED_VIEW,
    IT_DISPLAY_VIEW,
    IT_DISPLAY_VIEW_COLORSPACE,
    IT_LOOK_NAME,
    IT_LOOK,
    IT_VIEW_TRANSFORM_NAME,
    IT_VIEW_TRANSFORM
};

using EnvironmentVarNameIterator   = PyIterator<ConfigRcPtr, IT_ENVIRONMENT_VAR_NAME>;
using SearchPathIterator           = PyIterator<ConfigRcPtr, IT_SEARCH_PATH>;
using ColorSpaceNameIterator       = PyIterator<ConfigRcPtr, 
                                                IT_COLOR_SPACE_NAME, 
                                                SearchReferenceSpaceType,
                                                ColorSpaceVisibility>;
using ColorSpaceIterator           = PyIterator<ConfigRcPtr, 
                                                IT_COLOR_SPACE, 
                                                SearchReferenceSpaceType,
                                                ColorSpaceVisibility>;
using ActiveColorSpaceNameIterator = PyIterator<ConfigRcPtr, IT_ACTIVE_COLOR_SPACE_NAME>;
using ActiveColorSpaceIterator     = PyIterator<ConfigRcPtr, IT_ACTIVE_COLOR_SPACE>;
using RoleNameIterator             = PyIterator<ConfigRcPtr, IT_ROLE_NAME>;
using RoleColorSpaceIterator       = PyIterator<ConfigRcPtr, IT_ROLE_COLOR_SPACE>;
using DisplayIterator              = PyIterator<ConfigRcPtr, IT_DISPLAY>;
using SharedViewIterator           = PyIterator<ConfigRcPtr, IT_SHARED_VIEW>;
using ViewIterator                 = PyIterator<ConfigRcPtr, IT_DISPLAY_VIEW, std::string>;
using ViewForColorSpaceIterator    = PyIterator<ConfigRcPtr, IT_DISPLAY_VIEW_COLORSPACE,
                                                std::string, std::string>;
using LookNameIterator             = PyIterator<ConfigRcPtr, IT_LOOK_NAME>;
using LookIterator                 = PyIterator<ConfigRcPtr, IT_LOOK>;
using ViewTransformNameIterator    = PyIterator<ConfigRcPtr, IT_VIEW_TRANSFORM_NAME>;
using ViewTransformIterator        = PyIterator<ConfigRcPtr, IT_VIEW_TRANSFORM>;

} // namespace

void bindPyConfig(py::module & m)
{
    auto cls = py::class_<Config, ConfigRcPtr /* holder */>(m, "Config")
        .def(py::init(&Config::Create))

        .def_static("CreateRaw", &Config::CreateRaw)
        .def_static("CreateFromEnv", &Config::CreateFromEnv)
        .def_static("CreateFromFile", &Config::CreateFromFile, "fileName"_a)
        .def_static("CreateFromStream", [](const std::string & str) 
            {
                std::istringstream is(str);
                return Config::CreateFromStream(is);
            }, 
             "str"_a)
                    
        .def("getMajorVersion", &Config::getMajorVersion)
        .def("setMajorVersion", &Config::setMajorVersion, "major"_a)
        .def("getMinorVersion", &Config::getMinorVersion)
        .def("setMinorVersion", &Config::setMinorVersion, "minor"_a)
        .def("upgradeToLatestVersion", &Config::upgradeToLatestVersion)
        .def("validate", &Config::validate)
        .def("getFamilySeparator", &Config::getFamilySeparator)
        .def("setFamilySeparator", &Config::setFamilySeparator, "separator"_a)
        .def("getDescription", &Config::getDescription)
        .def("setDescription", &Config::setDescription, "description"_a)
        .def("serialize", [](ConfigRcPtr & self, const std::string & fileName) 
            {
                std::ofstream f(fileName.c_str());
                self->serialize(f);
                f.close();
            }, 
             "fileName"_a)
        .def("serialize", [](ConfigRcPtr & self) 
            {
                std::ostringstream os;
                self->serialize(os);
                return os.str();
            })
        .def("getCacheID", (const char * (Config::*)() const) &Config::getCacheID)
        .def("getCacheID", 
             (const char * (Config::*)(const ConstContextRcPtr &) const) &Config::getCacheID, 
             "context"_a)

        // Resources
        .def("getCurrentContext", &Config::getCurrentContext)
        .def("addEnvironmentVar", &Config::addEnvironmentVar, "name"_a, "defaultValue"_a)
        .def("getEnvironmentVarNames", [](ConfigRcPtr & self) 
            {
                return EnvironmentVarNameIterator(self);
            })
        .def("getEnvironmentVarDefault", &Config::getEnvironmentVarDefault, "name"_a)
        .def("clearEnvironmentVars", &Config::clearEnvironmentVars)
        .def("setEnvironmentMode", &Config::setEnvironmentMode, "mode"_a)
        .def("getEnvironmentMode", &Config::getEnvironmentMode)
        .def("loadEnvironment", &Config::loadEnvironment)
        .def("getSearchPath", (const char * (Config::*)() const) &Config::getSearchPath)
        .def("setSearchPath", &Config::setSearchPath, "path"_a)
        .def("getSearchPaths", [](ConfigRcPtr & self) { return SearchPathIterator(self); })
        .def("clearSearchPaths", &Config::clearSearchPaths)
        .def("addSearchPath", &Config::addSearchPath, "path"_a)
        .def("getWorkingDir", &Config::getWorkingDir)
        .def("setWorkingDir", &Config::setWorkingDir, "dirName"_a)

        // ColorSpaces
        .def("getColorSpaces", &Config::getColorSpaces, "category"_a)
        .def("getColorSpace", &Config::getColorSpace, "name"_a)
        .def("getColorSpaceNames", [](ConfigRcPtr & self, 
                                      SearchReferenceSpaceType searchReferenceType, 
                                      ColorSpaceVisibility visibility) 
            {
                return ColorSpaceNameIterator(self, searchReferenceType, visibility);
            },
             "searchReferenceType"_a, "visibility"_a)
        .def("getColorSpaces", [](ConfigRcPtr & self, 
                                  SearchReferenceSpaceType searchReferenceType, 
                                  ColorSpaceVisibility visibility) 
            {
                return ColorSpaceIterator(self, searchReferenceType, visibility);
            },
             "searchReferenceType"_a, "visibility"_a)
        .def("getColorSpaceNames", [](ConfigRcPtr & self) 
            {
                return ActiveColorSpaceNameIterator(self);
            })
        .def("getColorSpaces", [](ConfigRcPtr & self) 
            {
                return ActiveColorSpaceIterator(self);
            })
        .def("addColorSpace", &Config::addColorSpace, "colorSpace"_a)
        .def("removeColorSpace", &Config::removeColorSpace, "name"_a)
        .def("isColorSpaceUsed", &Config::isColorSpaceUsed, "name"_a)
        .def("clearColorSpaces", &Config::clearColorSpaces)
        .def("parseColorSpaceFromString", &Config::parseColorSpaceFromString, "str"_a)
        .def("isStrictParsingEnabled", &Config::isStrictParsingEnabled)
        .def("setInactiveColorSpaces", &Config::setInactiveColorSpaces, "inactiveColorSpaces"_a)
        .def("getInactiveColorSpaces", &Config::getInactiveColorSpaces)

        // Roles
        .def("setRole", &Config::setRole, "role"_a, "colorSpaceName"_a)
        .def("hasRole", &Config::hasRole, "role"_a)
        .def("getRoleNames", [](ConfigRcPtr & self) { return RoleNameIterator(self); })
        .def("getRoles", [](ConfigRcPtr & self) { return RoleColorSpaceIterator(self); })

        // Display/View Registration
        .def("addSharedView",
             (void (Config::*)(const char *,
                               const char *,
                               const char *,
                               const char *,
                               const char *,
                               const char *)) &Config::addSharedView,
             "view"_a, "viewTransformName"_a, "colorSpaceName"_a, 
             "looks"_a = "",
             "ruleName"_a = "", 
             "description"_a = "")
        .def("removeSharedView", &Config::removeSharedView, "view"_a)
        .def("getSharedViews", [](ConfigRcPtr & self) { return SharedViewIterator(self); })
        .def("getDefaultDisplay", &Config::getDefaultDisplay)
        .def("getDisplays", [](ConfigRcPtr & self) { return DisplayIterator(self); })
        .def("getDefaultView", &Config::getDefaultView, "display"_a)
        .def("getViews", [](ConfigRcPtr & self, const std::string & display)
             {
                 return ViewIterator(self, display);
             },
             "display"_a)
        .def("getViews",
             [](ConfigRcPtr & self, const std::string & display, const std::string & colorSpaceName)
             {
                 return ViewForColorSpaceIterator(self, display, colorSpaceName);
             },
             "display"_a, "colorSpaceName"_a)
        .def("getDisplayViewTransformName", &Config::getDisplayViewTransformName, 
             "display"_a, "view"_a)
        .def("getDisplayViewColorSpaceName", &Config::getDisplayViewColorSpaceName, "display"_a, "view"_a)
        .def("getDisplayViewLooks", &Config::getDisplayViewLooks, "display"_a, "view"_a)
        .def("getDisplayViewRule", &Config::getDisplayViewRule, "display"_a, "view"_a)
        .def("getDisplayViewDescription", &Config::getDisplayViewDescription, "display"_a, "view"_a)
        .def("addDisplayView", 
             (void (Config::*)(const char *, const char *, const char *, const char *)) 
             &Config::addDisplayView, 
             "display"_a, "view"_a, "colorSpaceName"_a, 
             "looks"_a = "")
        .def("addDisplayView", 
             (void (Config::*)(const char *, 
                               const char *, 
                               const char *, 
                               const char *, 
                               const char *,
                               const char *,
                               const char *)) &Config::addDisplayView, 
             "display"_a, "view"_a, "viewTransform"_a, "displayColorSpaceName"_a, 
             "looks"_a = "",
             "ruleName"_a = "", 
             "description"_a = "")
        .def("addDisplaySharedView", &Config::addDisplaySharedView, "display"_a, "view"_a)
        .def("removeDisplayView", &Config::removeDisplayView, "display"_a, "view"_a)
        .def("clearDisplays", &Config::clearDisplays)
        .def("setActiveDisplays", &Config::setActiveDisplays, "displays"_a)
        .def("getActiveDisplays", &Config::getActiveDisplays)
        .def("setActiveViews", &Config::setActiveViews, "views"_a)
        .def("getActiveViews", &Config::getActiveViews)

        // Luma
        .def("getDefaultLumaCoefs", [](ConfigRcPtr & self)
            {
                std::array<double, 3> rgb;
                self->getDefaultLumaCoefs(rgb.data());
                return rgb;
            })
        .def("setDefaultLumaCoefs", [](ConfigRcPtr & self, const std::array<double, 3> & rgb)
            {
                self->setDefaultLumaCoefs(rgb.data());
            }, 
             "rgb"_a)

        // Look
        .def("getLook", &Config::getLook, "name"_a)
        .def("getLookNames", [](ConfigRcPtr & self) { return LookNameIterator(self); })
        .def("getLooks", [](ConfigRcPtr & self) { return LookIterator(self); })
        .def("addLook", &Config::addLook, "look"_a)
        .def("clearLooks", &Config::clearLooks)

        // View Transforms
        .def("getViewTransform", &Config::getViewTransform, "name"_a)
        .def("getViewTransformNames", [](ConfigRcPtr & self) 
            { 
                return ViewTransformNameIterator(self); 
            })
        .def("getViewTransforms", [](ConfigRcPtr & self) 
            { 
                return ViewTransformIterator(self); 
            })
        .def("addViewTransform", &Config::addViewTransform, "viewTransform"_a)
        .def("getDefaultSceneToDisplayViewTransform", 
             &Config::getDefaultSceneToDisplayViewTransform)
        .def("clearViewTransforms", &Config::clearViewTransforms)

        // Viewing Rules
        .def("getViewingRules", &Config::getViewingRules)
        .def("setViewingRules", &Config::setViewingRules, "ViewingRules"_a)

        // File Rules
        .def("getFileRules", &Config::getFileRules)
        .def("setFileRules", &Config::setFileRules, "fileRules"_a)
        .def("getColorSpaceFromFilepath",
             (const char * (Config::*)(const char *) const) &Config::getColorSpaceFromFilepath, 
             "filePath"_a)
        .def("getColorSpaceFromFilepath",
             (const char * (Config::*)(const char *, size_t &) const) 
             &Config::getColorSpaceFromFilepath, 
             "filePath"_a, "ruleIndex"_a)
        .def("filepathOnlyMatchesDefaultRule", &Config::filepathOnlyMatchesDefaultRule, 
             "filePath"_a)

        // Processors
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstColorSpaceRcPtr &, 
                                              const ConstColorSpaceRcPtr &) const) 
             &Config::getProcessor, 
             "srcColorSpace"_a, "dstColorSpace"_a)
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstContextRcPtr &, 
                                              const ConstColorSpaceRcPtr &, 
                                              const ConstColorSpaceRcPtr &) const) 
             &Config::getProcessor, 
             "context"_a, "srcColorSpace"_a, "dstColorSpace"_a)
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const char *, const char *) const) 
             &Config::getProcessor, 
             "srcColorSpaceName"_a, "dstColorSpaceName"_a)
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstContextRcPtr &, 
                                              const char *, 
                                              const char *) const) 
             &Config::getProcessor, 
             "context"_a, "srcColorSpaceName"_a, "dstColorSpaceName"_a)
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const char *, const char *, const char *) const) 
             &Config::getProcessor, 
             "srcColorSpaceName"_a, "display"_a, "view"_a)
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstContextRcPtr &, 
                                              const char *, 
                                              const char *, 
                                              const char *) const) 
             &Config::getProcessor, 
             "context"_a, "srcColorSpaceName"_a, "display"_a, "view"_a)
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstTransformRcPtr &) const) 
             &Config::getProcessor, 
             "transform"_a)
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstTransformRcPtr &, 
                                              TransformDirection) const) 
             &Config::getProcessor, 
             "transform"_a, "direction"_a)
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstContextRcPtr &, 
                                              const ConstTransformRcPtr &, 
                                              TransformDirection) const) 
             &Config::getProcessor, 
             "context"_a, "transform"_a, "direction"_a)

        .def_static("GetProcessor", [](const ConstConfigRcPtr & srcConfig,
                                       const char * srcColorSpaceName,
                                       const ConstConfigRcPtr & dstConfig,
                                       const char * dstColorSpaceName)
            {
                return Config::GetProcessor(srcConfig, srcColorSpaceName,
                                            dstConfig, dstColorSpaceName);
            },
                    "srcConfig"_a, "srcColorSpaceName"_a, "dstConfig"_a, "dstColorSpaceName"_a)
        .def_static("GetProcessor", [](const ConstContextRcPtr & srcContext,
                                       const ConstConfigRcPtr & srcConfig,
                                       const char * srcColorSpaceName,
                                       const ConstContextRcPtr & dstContext,
                                       const ConstConfigRcPtr & dstConfig,
                                       const char * dstColorSpaceName)
            {
                return Config::GetProcessor(srcContext, srcConfig, srcColorSpaceName,
                                            dstContext, dstConfig, dstColorSpaceName);
            },
                    "srcContext"_a, "srcConfig"_a, "srcColorSpaceName"_a, 
                    "dstContext"_a, "dstConfig"_a, "dstColorSpaceName"_a)
        .def_static("GetProcessor", [](const ConstConfigRcPtr & srcConfig,
                                       const char * srcColorSpaceName,
                                       const char * srcInterchangeName,
                                       const ConstConfigRcPtr & dstConfig,
                                       const char * dstColorSpaceName,
                                       const char * dstInterchangeName)
            {
                return Config::GetProcessor(srcConfig, srcColorSpaceName, srcInterchangeName,
                                            dstConfig, dstColorSpaceName, dstInterchangeName);
            }, 
                    "srcConfig"_a, "srcColorSpaceName"_a, "srcInterchangeName"_a, 
                    "dstConfig"_a, "dstColorSpaceName"_a, "dstInterchangeName"_a)
        .def_static("GetProcessor", [](const ConstContextRcPtr & srcContext,
                                       const ConstConfigRcPtr & srcConfig,
                                       const char * srcColorSpaceName,
                                       const char * srcInterchangeName,
                                       const ConstContextRcPtr & dstContext,
                                       const ConstConfigRcPtr & dstConfig,
                                       const char * dstColorSpaceName,
                                       const char * dstInterchangeName)
            {
                return Config::GetProcessor(srcContext, srcConfig, srcColorSpaceName, srcInterchangeName,
                                            dstContext, dstConfig, dstColorSpaceName, dstInterchangeName);
            }, 
                    "srcContext"_a, "srcConfig"_a, "srcColorSpaceName"_a, "srcInterchangeName"_a, 
                    "dstContext"_a, "dstConfig"_a, "dstColorSpaceName"_a, "dstInterchangeName"_a);

    defStr(cls);

    py::class_<EnvironmentVarNameIterator>(cls, "EnvironmentVarNameIterator")
        .def("__len__", [](EnvironmentVarNameIterator & it) 
            { 
                return it.m_obj->getNumEnvironmentVars(); 
            })
        .def("__getitem__", [](EnvironmentVarNameIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumEnvironmentVars());
                return it.m_obj->getEnvironmentVarNameByIndex(i);
            })
        .def("__iter__", [](EnvironmentVarNameIterator & it) -> EnvironmentVarNameIterator & 
            { 
                return it; 
            })
        .def("__next__", [](EnvironmentVarNameIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumEnvironmentVars());
                return it.m_obj->getEnvironmentVarNameByIndex(i);
            });

    py::class_<SearchPathIterator>(cls, "SearchPathIterator")
        .def("__len__", [](SearchPathIterator & it) 
            { 
                return it.m_obj->getNumSearchPaths(); 
            })
        .def("__getitem__", [](SearchPathIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumSearchPaths());
                return it.m_obj->getSearchPath(i);
            })
        .def("__iter__", [](SearchPathIterator & it) -> SearchPathIterator & { return it; })
        .def("__next__", [](SearchPathIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumSearchPaths());
                return it.m_obj->getSearchPath(i);
            });

    py::class_<ColorSpaceNameIterator>(cls, "ColorSpaceNameIterator")
        .def("__len__", [](ColorSpaceNameIterator & it) 
            { 
                return it.m_obj->getNumColorSpaces(std::get<0>(it.m_args), std::get<1>(it.m_args)); 
            })
        .def("__getitem__", [](ColorSpaceNameIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumColorSpaces(std::get<0>(it.m_args), 
                                                             std::get<1>(it.m_args)));
                return it.m_obj->getColorSpaceNameByIndex(std::get<0>(it.m_args), 
                                                          std::get<1>(it.m_args), 
                                                          i);
            })
        .def("__iter__", [](ColorSpaceNameIterator & it) -> ColorSpaceNameIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ColorSpaceNameIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumColorSpaces(std::get<0>(it.m_args), 
                                                                 std::get<1>(it.m_args)));
                return it.m_obj->getColorSpaceNameByIndex(std::get<0>(it.m_args), 
                                                          std::get<1>(it.m_args), 
                                                          i);
            });

    py::class_<ColorSpaceIterator>(cls, "ColorSpaceIterator")
        .def("__len__", [](ColorSpaceIterator & it) 
            { 
                return it.m_obj->getNumColorSpaces(std::get<0>(it.m_args), std::get<1>(it.m_args)); 
            })
        .def("__getitem__", [](ColorSpaceIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumColorSpaces(std::get<0>(it.m_args), 
                                                             std::get<1>(it.m_args)));
                const char * name = it.m_obj->getColorSpaceNameByIndex(std::get<0>(it.m_args), 
                                                                       std::get<1>(it.m_args), 
                                                                       i);
                return it.m_obj->getColorSpace(name);
            })
        .def("__iter__", [](ColorSpaceIterator & it) -> ColorSpaceIterator & { return it; })
        .def("__next__", [](ColorSpaceIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumColorSpaces(std::get<0>(it.m_args), 
                                                                 std::get<1>(it.m_args)));
                const char * name = it.m_obj->getColorSpaceNameByIndex(std::get<0>(it.m_args), 
                                                                       std::get<1>(it.m_args), 
                                                                       i);
                return it.m_obj->getColorSpace(name);
            });

    py::class_<ActiveColorSpaceNameIterator>(cls, "ActiveColorSpaceNameIterator")
        .def("__len__", [](ActiveColorSpaceNameIterator & it) 
            { 
                return it.m_obj->getNumColorSpaces(); 
            })
        .def("__getitem__", [](ActiveColorSpaceNameIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumColorSpaces());
                return it.m_obj->getColorSpaceNameByIndex(i);
            })
        .def("__iter__", [](ActiveColorSpaceNameIterator & it) -> ActiveColorSpaceNameIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ActiveColorSpaceNameIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumColorSpaces());
                return it.m_obj->getColorSpaceNameByIndex(i);
            });

    py::class_<ActiveColorSpaceIterator>(cls, "ActiveColorSpaceIterator")
        .def("__len__", [](ActiveColorSpaceIterator & it) 
            { 
                return it.m_obj->getNumColorSpaces(); 
            })
        .def("__getitem__", [](ActiveColorSpaceIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumColorSpaces());
                const char * name = it.m_obj->getColorSpaceNameByIndex(i);
                return it.m_obj->getColorSpace(name);
            })
        .def("__iter__", [](ActiveColorSpaceIterator & it) -> ActiveColorSpaceIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ActiveColorSpaceIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumColorSpaces());
                const char * name = it.m_obj->getColorSpaceNameByIndex(i);
                return it.m_obj->getColorSpace(name);
            });

    py::class_<RoleNameIterator>(cls, "RoleNameIterator")
        .def("__len__", [](RoleNameIterator & it) { return it.m_obj->getNumRoles(); })
        .def("__getitem__", [](RoleNameIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumRoles());
                return it.m_obj->getRoleName(i);
            })
        .def("__iter__", [](RoleNameIterator & it) -> RoleNameIterator & { return it; })
        .def("__next__", [](RoleNameIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumRoles());
                return it.m_obj->getRoleName(i);
            });

    py::class_<RoleColorSpaceIterator>(cls, "RoleColorSpaceIterator")
        .def("__len__", [](RoleColorSpaceIterator & it) { return it.m_obj->getNumRoles(); })
        .def("__getitem__", [](RoleColorSpaceIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumRoles());
                return py::make_tuple(it.m_obj->getRoleName(i), 
                                      it.m_obj->getRoleColorSpace(i));
            })
        .def("__iter__", [](RoleColorSpaceIterator & it) -> RoleColorSpaceIterator & 
            { 
                return it; 
            })
        .def("__next__", [](RoleColorSpaceIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumRoles());
                return py::make_tuple(it.m_obj->getRoleName(i), 
                                      it.m_obj->getRoleColorSpace(i));
            });

    py::class_<DisplayIterator>(cls, "DisplayIterator")
        .def("__len__", [](DisplayIterator & it) { return it.m_obj->getNumDisplays(); })
        .def("__getitem__", [](DisplayIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumDisplays());
                return it.m_obj->getDisplay(i);
            })
        .def("__iter__", [](DisplayIterator & it) -> DisplayIterator & { return it; })
        .def("__next__", [](DisplayIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumDisplays());
                return it.m_obj->getDisplay(i);
            });

    py::class_<SharedViewIterator>(cls, "SharedViewIterator")
        .def("__len__", [](SharedViewIterator & it) { return it.m_obj->getNumViews(VIEW_SHARED,
                                                                                   nullptr); })
        .def("__getitem__", [](SharedViewIterator & it, int i)
            { 
                it.checkIndex(i, it.m_obj->getNumViews(VIEW_SHARED, nullptr));
                return it.m_obj->getView(VIEW_SHARED, nullptr, i);
            })
        .def("__iter__", [](SharedViewIterator & it) -> SharedViewIterator & { return it; })
        .def("__next__", [](SharedViewIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumViews(VIEW_SHARED, nullptr));
                return it.m_obj->getView(VIEW_SHARED, nullptr, i);
            });

    py::class_<ViewIterator>(cls, "ViewIterator")
        .def("__len__", [](ViewIterator & it)
                        { return it.m_obj->getNumViews(std::get<0>(it.m_args).c_str()); })
        .def("__getitem__", [](ViewIterator & it, int i)
            { 
                it.checkIndex(i, it.m_obj->getNumViews(std::get<0>(it.m_args).c_str()));
                return it.m_obj->getView(std::get<0>(it.m_args).c_str(), i);
            })
        .def("__iter__", [](ViewIterator & it) -> ViewIterator & { return it; })
        .def("__next__", [](ViewIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumViews(std::get<0>(it.m_args).c_str()));
                return it.m_obj->getView(std::get<0>(it.m_args).c_str(), i);
            });

    py::class_<ViewForColorSpaceIterator>(cls, "ViewForColorSpaceIterator")
        .def("__len__", [](ViewForColorSpaceIterator & it)
                        { return it.m_obj->getNumViews(std::get<0>(it.m_args).c_str(),
                                                       std::get<1>(it.m_args).c_str()); })
        .def("__getitem__", [](ViewForColorSpaceIterator & it, int i)
            { 
                it.checkIndex(i, it.m_obj->getNumViews(std::get<0>(it.m_args).c_str(),
                                                       std::get<1>(it.m_args).c_str()));
                return it.m_obj->getView(std::get<0>(it.m_args).c_str(),
                                         std::get<1>(it.m_args).c_str(), i);
            })
        .def("__iter__", [](ViewForColorSpaceIterator & it) -> ViewForColorSpaceIterator & { return it; })
        .def("__next__", [](ViewForColorSpaceIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumViews(std::get<0>(it.m_args).c_str(),
                                                           std::get<1>(it.m_args).c_str()));
                return it.m_obj->getView(std::get<0>(it.m_args).c_str(),
                                         std::get<1>(it.m_args).c_str(), i);
            });

    py::class_<LookNameIterator>(cls, "LookNameIterator")
        .def("__len__", [](LookNameIterator & it) { return it.m_obj->getNumLooks(); })
        .def("__getitem__", [](LookNameIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumLooks());
                return it.m_obj->getLookNameByIndex(i);
            })
        .def("__iter__", [](LookNameIterator & it) -> LookNameIterator & { return it; })
        .def("__next__", [](LookNameIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumLooks());
                return it.m_obj->getLookNameByIndex(i);
            });

    py::class_<LookIterator>(cls, "LookIterator")
        .def("__len__", [](LookIterator & it) { return it.m_obj->getNumLooks(); })
        .def("__getitem__", [](LookIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumLooks());
                const char * name = it.m_obj->getLookNameByIndex(i);
                return it.m_obj->getLook(name);
            })
        .def("__iter__", [](LookIterator & it) -> LookIterator & { return it; })
        .def("__next__", [](LookIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumLooks());
                const char * name = it.m_obj->getLookNameByIndex(i);
                return it.m_obj->getLook(name);
            });

    py::class_<ViewTransformNameIterator>(cls, "ViewTransformNameIterator")
        .def("__len__", [](ViewTransformNameIterator & it) 
            { 
                return it.m_obj->getNumViewTransforms(); 
            })
        .def("__getitem__", [](ViewTransformNameIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumViewTransforms());
                return it.m_obj->getViewTransformNameByIndex(i);
            })
        .def("__iter__", [](ViewTransformNameIterator & it) -> ViewTransformNameIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ViewTransformNameIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumViewTransforms());
                return it.m_obj->getViewTransformNameByIndex(i);
            });

    py::class_<ViewTransformIterator>(cls, "ViewTransformIterator")
        .def("__len__", [](ViewTransformIterator & it) 
            { 
                return it.m_obj->getNumViewTransforms(); 
            })
        .def("__getitem__", [](ViewTransformIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumViewTransforms());
                const char * name = it.m_obj->getViewTransformNameByIndex(i);
                return it.m_obj->getViewTransform(name);
            })
        .def("__iter__", [](ViewTransformIterator & it) -> ViewTransformIterator & { return it; })
        .def("__next__", [](ViewTransformIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumViewTransforms());
                const char * name = it.m_obj->getViewTransformNameByIndex(i);
                return it.m_obj->getViewTransform(name);
            });

    m.def("GetCurrentConfig", &GetCurrentConfig);
    m.def("SetCurrentConfig", &SetCurrentConfig, "config"_a);
}

} // namespace OCIO_NAMESPACE
