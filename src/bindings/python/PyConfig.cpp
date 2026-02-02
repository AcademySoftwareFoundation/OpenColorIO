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
    IT_ACTIVE_DISPLAYS_LIST,
    IT_ACTIVE_VIEWS_LIST,
    IT_LOOK_NAME,
    IT_LOOK,
    IT_VIEW_TRANSFORM_NAME,
    IT_VIEW_TRANSFORM,
    IT_NAMED_TRANSFORM_NAME,
    IT_NAMED_TRANSFORM,
    IT_ACTIVE_NAMED_TRANSFORM_NAME,
    IT_ACTIVE_NAMED_TRANSFORM,
    IT_DISPLAY_ALL,
    IT_DISPLAY_VIEW_TYPE,
    IT_VIRTUAL_DISPLAY_VIEW,
};

using EnvironmentVarNameIterator       = PyIterator<ConfigRcPtr, IT_ENVIRONMENT_VAR_NAME>;
using SearchPathIterator               = PyIterator<ConfigRcPtr, IT_SEARCH_PATH>;
using ColorSpaceNameIterator           = PyIterator<ConfigRcPtr, 
                                                    IT_COLOR_SPACE_NAME, 
                                                    SearchReferenceSpaceType,
                                                    ColorSpaceVisibility>;
using ColorSpaceIterator               = PyIterator<ConfigRcPtr, 
                                                    IT_COLOR_SPACE, 
                                                    SearchReferenceSpaceType,
                                                    ColorSpaceVisibility>;
using ActiveColorSpaceNameIterator     = PyIterator<ConfigRcPtr, IT_ACTIVE_COLOR_SPACE_NAME>;
using ActiveColorSpaceIterator         = PyIterator<ConfigRcPtr, IT_ACTIVE_COLOR_SPACE>;
using RoleNameIterator                 = PyIterator<ConfigRcPtr, IT_ROLE_NAME>;
using RoleColorSpaceIterator           = PyIterator<ConfigRcPtr, IT_ROLE_COLOR_SPACE>;
using DisplayIterator                  = PyIterator<ConfigRcPtr, IT_DISPLAY>;
using DisplayAllIterator               = PyIterator<ConfigRcPtr, IT_DISPLAY_ALL>;
using SharedViewIterator               = PyIterator<ConfigRcPtr, IT_SHARED_VIEW>;
using VirtualViewIterator              = PyIterator<ConfigRcPtr, IT_VIRTUAL_DISPLAY_VIEW, 
                                                    ViewType>;
using ViewIterator                     = PyIterator<ConfigRcPtr, IT_DISPLAY_VIEW, std::string>;
using ViewForColorSpaceIterator        = PyIterator<ConfigRcPtr, IT_DISPLAY_VIEW_COLORSPACE,
                                                    std::string, std::string>;
using ViewForViewTypeIterator          = PyIterator<ConfigRcPtr, IT_DISPLAY_VIEW_TYPE, 
                                                    ViewType, std::string>;
using ActiveDisplaysListIterator       = PyIterator<ConfigRcPtr, IT_ACTIVE_DISPLAYS_LIST>;
using ActiveViewsListIterator          = PyIterator<ConfigRcPtr, IT_ACTIVE_VIEWS_LIST>;
using LookNameIterator                 = PyIterator<ConfigRcPtr, IT_LOOK_NAME>;
using LookIterator                     = PyIterator<ConfigRcPtr, IT_LOOK>;
using ViewTransformNameIterator        = PyIterator<ConfigRcPtr, IT_VIEW_TRANSFORM_NAME>;
using ViewTransformIterator            = PyIterator<ConfigRcPtr, IT_VIEW_TRANSFORM>;
using NamedTransformNameIterator       = PyIterator<ConfigRcPtr, IT_NAMED_TRANSFORM_NAME,
                                                    NamedTransformVisibility>;
using NamedTransformIterator           = PyIterator<ConfigRcPtr, IT_NAMED_TRANSFORM,
                                                    NamedTransformVisibility>;
using ActiveNamedTransformNameIterator = PyIterator<ConfigRcPtr, IT_ACTIVE_NAMED_TRANSFORM_NAME>;
using ActiveNamedTransformIterator     = PyIterator<ConfigRcPtr, IT_ACTIVE_NAMED_TRANSFORM>;

} // namespace

void bindPyConfig(py::module & m)
{
    auto clsConfig = 
        py::class_<Config, ConfigRcPtr>(
            m.attr("Config"));

    auto clsEnvironmentVarNameIterator = 
        py::class_<EnvironmentVarNameIterator>(
            clsConfig, "EnvironmentVarNameIterator");

    auto clsSearchPathIterator = 
        py::class_<SearchPathIterator>(
            clsConfig, "SearchPathIterator");

    auto clsColorSpaceNameIterator = 
        py::class_<ColorSpaceNameIterator>(
            clsConfig, "ColorSpaceNameIterator");

    auto clsColorSpaceIterator = 
        py::class_<ColorSpaceIterator>(
            clsConfig, "ColorSpaceIterator");

    auto clsActiveColorSpaceNameIterator = 
        py::class_<ActiveColorSpaceNameIterator>(
            clsConfig, "ActiveColorSpaceNameIterator");

    auto clsActiveColorSpaceIterator = 
        py::class_<ActiveColorSpaceIterator>(
            clsConfig, "ActiveColorSpaceIterator");

    auto clsRoleNameIterator = 
        py::class_<RoleNameIterator>(
            clsConfig, "RoleNameIterator");

    auto clsRoleColorSpaceIterator = 
        py::class_<RoleColorSpaceIterator>(
            clsConfig, "RoleColorSpaceIterator");

    auto clsDisplayIterator = 
        py::class_<DisplayIterator>(
            clsConfig, "DisplayIterator");

    auto clsDisplayAllIterator = 
        py::class_<DisplayAllIterator>(
            clsConfig, "DisplayAllIterator");

    auto clsSharedViewIterator = 
        py::class_<SharedViewIterator>(
            clsConfig, "SharedViewIterator");

    auto clsVirtualViewIterator = 
        py::class_<VirtualViewIterator>(
            clsConfig, "VirtualViewIterator");

    auto clsViewIterator = 
        py::class_<ViewIterator>(
            clsConfig, "ViewIterator");

    auto clsViewForColorSpaceIterator = 
        py::class_<ViewForColorSpaceIterator>(
            clsConfig, "ViewForColorSpaceIterator");

    auto clsViewForViewTypeIterator =
        py::class_<ViewForViewTypeIterator>(
            clsConfig, "ViewForViewTypeIterator");

    auto clsActiveDisplaysListIterator = 
        py::class_<ActiveDisplaysListIterator>(
            clsConfig, "ActiveDisplaysListIterator");

    auto clsActiveViewsListIterator = 
        py::class_<ActiveViewsListIterator>(
            clsConfig, "ActiveViewsListIterator");

    auto clsLookNameIterator = 
        py::class_<LookNameIterator>(
            clsConfig, "LookNameIterator");

    auto clsLookIterator = 
        py::class_<LookIterator>(
            clsConfig, "LookIterator");

    auto clsViewTransformNameIterator = 
        py::class_<ViewTransformNameIterator>(
            clsConfig, "ViewTransformNameIterator");

    auto clsViewTransformIterator = 
        py::class_<ViewTransformIterator>(
            clsConfig, "ViewTransformIterator");

    auto clsNamedTransformNameIterator =
        py::class_<NamedTransformNameIterator>(
            clsConfig, "NamedTransformNameIterator");

    auto clsNamedTransformIterator =
        py::class_<NamedTransformIterator>(
            clsConfig, "NamedTransformIterator");

    auto clsActiveNamedTransformNameIterator =
        py::class_<ActiveNamedTransformNameIterator>(
            clsConfig, "ActiveNamedTransformNameIterator");

    auto clsActiveNamedTransformIterator =
        py::class_<ActiveNamedTransformIterator>(
            clsConfig, "ActiveNamedTransformIterator");

    clsConfig
        .def(py::init(&Config::Create), 
             DOC(Config, Create))

        .def("__deepcopy__", [](const ConstConfigRcPtr & self, py::dict)
            {
                return self->createEditableCopy();
            },
            "memo"_a)

        .def_static("CreateRaw", &Config::CreateRaw, 
                    DOC(Config, CreateRaw))
        .def_static("CreateFromEnv", &Config::CreateFromEnv, 
                    DOC(Config, CreateFromEnv))
        .def_static("CreateFromFile", &Config::CreateFromFile, "fileName"_a, 
                    DOC(Config, CreateFromFile))
        .def_static("CreateFromStream", [](const std::string & str) 
            {
                std::istringstream is(str);
                return Config::CreateFromStream(is);
            }, 
             "str"_a, 
             DOC(Config, CreateFromStream))

        .def_static("CreateFromBuiltinConfig", &Config::CreateFromBuiltinConfig, 
                    DOC(Config, CreateFromBuiltinConfig))
        .def_static("CreateFromConfigIOProxy", &Config::CreateFromConfigIOProxy,
                    DOC(Config, CreateFromConfigIOProxy))
        .def("getMajorVersion", &Config::getMajorVersion, 
             DOC(Config, getMajorVersion))
        .def("setMajorVersion", &Config::setMajorVersion, "major"_a, 
             DOC(Config, setMajorVersion))
        .def("getMinorVersion", &Config::getMinorVersion, 
             DOC(Config, getMinorVersion))
        .def("setMinorVersion", &Config::setMinorVersion, "minor"_a, 
             DOC(Config, setMinorVersion))
        .def("setVersion", &Config::setVersion, "major"_a, "minor"_a,
             DOC(Config, setVersion))
        .def("upgradeToLatestVersion", &Config::upgradeToLatestVersion, 
             DOC(Config, upgradeToLatestVersion))
        .def("validate", &Config::validate, 
             DOC(Config, validate))
        .def("getName", &Config::getName, 
             DOC(Config, getName))
        .def("setName", &Config::setName, "name"_a.none(false), 
             DOC(Config, setName))
        .def("getFamilySeparator", &Config::getFamilySeparator, 
             DOC(Config, getFamilySeparator))
        .def("setFamilySeparator", &Config::setFamilySeparator, "separator"_a, 
             DOC(Config, setFamilySeparator))
        .def("getDescription", &Config::getDescription, 
             DOC(Config, getDescription))
        .def("setDescription", &Config::setDescription, "description"_a, 
             DOC(Config, setDescription))
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
            }, 
             DOC(Config, serialize))
        .def("getCacheID", (const char * (Config::*)() const) &Config::getCacheID, 
             DOC(Config, getCacheID))
        .def("getCacheID", 
             (const char * (Config::*)(const ConstContextRcPtr &) const) &Config::getCacheID, 
             "context"_a, 
             DOC(Config, getCacheID))

        // Resources
        .def("getCurrentContext", &Config::getCurrentContext, 
             DOC(Config, getCurrentContext))
        .def("addEnvironmentVar", &Config::addEnvironmentVar, "name"_a, "defaultValue"_a, 
             DOC(Config, addEnvironmentVar))
        .def("getEnvironmentVarNames", [](ConfigRcPtr & self) 
            {
                return EnvironmentVarNameIterator(self);
            })
        .def("getEnvironmentVarDefault", &Config::getEnvironmentVarDefault, "name"_a, 
             DOC(Config, getEnvironmentVarDefault))
        .def("clearEnvironmentVars", &Config::clearEnvironmentVars, 
             DOC(Config, clearEnvironmentVars))
        .def("setEnvironmentMode", &Config::setEnvironmentMode, "mode"_a, 
             DOC(Config, setEnvironmentMode))
        .def("getEnvironmentMode", &Config::getEnvironmentMode, 
             DOC(Config, getEnvironmentMode))
        .def("loadEnvironment", &Config::loadEnvironment, 
             DOC(Config, loadEnvironment))
        .def("getSearchPath", (const char * (Config::*)() const) &Config::getSearchPath, 
             DOC(Config, getSearchPath))
        .def("setSearchPath", &Config::setSearchPath, "path"_a, 
             DOC(Config, setSearchPath))
        .def("getSearchPaths", [](ConfigRcPtr & self) 
            { 
                return SearchPathIterator(self); 
            })
        .def("clearSearchPaths", &Config::clearSearchPaths, 
             DOC(Config, clearSearchPaths))
        .def("addSearchPath", &Config::addSearchPath, "path"_a, 
             DOC(Config, addSearchPath))
        .def("getWorkingDir", &Config::getWorkingDir, 
             DOC(Config, getWorkingDir))
        .def("setWorkingDir", &Config::setWorkingDir, "dirName"_a, 
             DOC(Config, setWorkingDir))
        .def("getConfigIOProxy", &Config::getConfigIOProxy, 
            DOC(Config, getConfigIOProxy))
        .def("setConfigIOProxy", &Config::setConfigIOProxy, "ciop"_a, 
            DOC(Config, setConfigIOProxy))

        // ColorSpaces
        .def("getColorSpaces", &Config::getColorSpaces, "category"_a, 
             DOC(Config, getColorSpaces))
        .def("getColorSpace", &Config::getColorSpace, "name"_a, 
             DOC(Config, getColorSpace))
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
        .def("getCanonicalName", &Config::getCanonicalName, "name"_a,
             DOC(Config, getCanonicalName))
        .def("addColorSpace", &Config::addColorSpace, "colorSpace"_a, 
             DOC(Config, addColorSpace))
        .def("removeColorSpace", &Config::removeColorSpace, "name"_a, 
             DOC(Config, removeColorSpace))
        .def("isColorSpaceLinear", &Config::isColorSpaceLinear, "colorSpace"_a, "referenceSpaceType"_a, 
             DOC(Config, isColorSpaceLinear))
        .def("isColorSpaceUsed", &Config::isColorSpaceUsed, "name"_a, 
             DOC(Config, isColorSpaceUsed))
        .def("clearColorSpaces", &Config::clearColorSpaces, 
             DOC(Config, clearColorSpaces))
        .def("parseColorSpaceFromString", &Config::parseColorSpaceFromString, "str"_a, 
             DOC(Config, parseColorSpaceFromString))
        .def("isStrictParsingEnabled", &Config::isStrictParsingEnabled, 
             DOC(Config, isStrictParsingEnabled))
        .def("setStrictParsingEnabled", &Config::setStrictParsingEnabled, "enabled"_a,
             DOC(Config, setStrictParsingEnabled))
        .def("setInactiveColorSpaces", &Config::setInactiveColorSpaces, "inactiveColorSpaces"_a,
             DOC(Config, setInactiveColorSpaces))
        .def("getInactiveColorSpaces", &Config::getInactiveColorSpaces, 
             DOC(Config, getInactiveColorSpaces))
        .def("isInactiveColorSpace", &Config::isInactiveColorSpace, "colorspace"_a,
             DOC(Config, isInactiveColorSpace))      
        .def_static("IdentifyBuiltinColorSpace", [](const ConstConfigRcPtr & srcConfig,
                                                    const ConstConfigRcPtr & builtinConfig,
                                                    const char * builtinColorSpaceName)
            {
                return Config::IdentifyBuiltinColorSpace(srcConfig,
                                                         builtinConfig,
                                                         builtinColorSpaceName);
            },
                    "srcConfig"_a, "builtinConfig"_a, "builtinColorSpaceName"_a,
                    DOC(Config, IdentifyBuiltinColorSpace))

        .def_static("IdentifyInterchangeSpace", [](const ConstConfigRcPtr & srcConfig,
                                                   const char * srcColorSpaceName,
                                                   const ConstConfigRcPtr & builtinConfig,
                                                   const char * builtinColorSpaceName)
            {
                const char * srcInterchangePtr = nullptr;
                const char * builtinInterchangePtr = nullptr;

                Config::IdentifyInterchangeSpace(&srcInterchangePtr,
                                                 &builtinInterchangePtr,
                                                 srcConfig,
                                                 srcColorSpaceName,
                                                 builtinConfig,
                                                 builtinColorSpaceName);
                // Return the tuple by value which copies the strings values.
                return std::make_tuple(std::string(srcInterchangePtr), std::string(builtinInterchangePtr));
            },
                "srcConfig"_a, "srcColorSpaceName"_a, "builtinConfig"_a, "builtinColorSpaceName"_a,
                DOC(Config, IdentifyInterchangeSpace))

        // Roles
        .def("setRole", &Config::setRole, "role"_a, "colorSpaceName"_a, 
             DOC(Config, setRole))
        .def("hasRole", &Config::hasRole, "role"_a, 
             DOC(Config, hasRole))
        .def("getRoleNames", [](ConfigRcPtr & self) 
            { 
                return RoleNameIterator(self); 
            })
        .def("getRoles", [](ConfigRcPtr & self) 
            { 
                return RoleColorSpaceIterator(self); 
            })
        .def("getRoleColorSpace", 
             (const char * (Config::*)(const char *) const) &Config::getRoleColorSpace, 
             "roleName"_a,
             DOC(Config, getRoleColorSpace)) 

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
             "description"_a = "", 
             DOC(Config, addSharedView))
        .def("removeSharedView", &Config::removeSharedView, "view"_a, 
             DOC(Config, removeSharedView))
        .def("clearSharedViews", &Config::clearSharedViews,
             DOC(Config, clearSharedViews))
        .def("getSharedViews", [](ConfigRcPtr & self) 
            { 
                return SharedViewIterator(self); 
            })
        .def("getDefaultDisplay", &Config::getDefaultDisplay, 
             DOC(Config, getDefaultDisplay))
        .def("getDisplays", [](ConfigRcPtr & self) 
            { 
                return DisplayIterator(self); 
            })
        .def("getDisplaysAll", [](ConfigRcPtr & self) 
            { 
                return DisplayAllIterator(self); 
            })
        .def("getDefaultView",
             (const char * (Config::*)(const char *) const)
             &Config::getDefaultView, "display"_a, 
             DOC(Config, getDefaultView))
        .def("getDefaultView",
             (const char * (Config::*)(const char *, const char *) const)
             &Config::getDefaultView, "display"_a, "colorSpacename"_a,
             DOC(Config, getDefaultView))
        .def("getViews", [](ConfigRcPtr & self, const std::string & display)
             {
                 return ViewIterator(self, display);
             },
             "display"_a)
        .def("getViews", [](ConfigRcPtr & self, ViewType type, const std::string & display)
             {
                 return ViewForViewTypeIterator(self, type, display);
             },
             "type"_a, "display"_a)
        .def("getViews", [](ConfigRcPtr & self, 
                            const std::string & display, 
                            const std::string & colorSpaceName)
             {
                 return ViewForColorSpaceIterator(self, display, colorSpaceName);
             },
             "display"_a, "colorSpaceName"_a)
        .def("getDisplayViewTransformName", &Config::getDisplayViewTransformName, 
             "display"_a, "view"_a, 
             DOC(Config, getDisplayViewTransformName))
        .def("getDisplayViewColorSpaceName", &Config::getDisplayViewColorSpaceName, 
             "display"_a, "view"_a, 
             DOC(Config, getDisplayViewColorSpaceName))
        .def("getDisplayViewLooks", &Config::getDisplayViewLooks, "display"_a, "view"_a, 
             DOC(Config, getDisplayViewLooks))
        .def("getDisplayViewRule", &Config::getDisplayViewRule, "display"_a, "view"_a, 
             DOC(Config, getDisplayViewRule))
        .def("getDisplayViewDescription", &Config::getDisplayViewDescription, 
             "display"_a, "view"_a, 
             DOC(Config, getDisplayViewDescription))
        .def("hasView", &Config::hasView,
             "display"_a, "view"_a,
             DOC(Config, hasView))
        .def("addDisplayView", 
             (void (Config::*)(const char *, const char *, const char *, const char *)) 
             &Config::addDisplayView, 
             "display"_a, "view"_a, "colorSpaceName"_a, 
             "looks"_a = "", 
             DOC(Config, addDisplayView))
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
             "description"_a = "", 
             DOC(Config, addDisplayView))
        .def("isViewShared", &Config::isViewShared, "display"_a, "view"_a,
             DOC(Config, isViewShared))
        .def("addDisplaySharedView", &Config::addDisplaySharedView, "display"_a, "view"_a, 
             DOC(Config, addDisplaySharedView))
        .def("removeDisplayView", &Config::removeDisplayView, "display"_a, "view"_a, 
             DOC(Config, removeDisplayView))
        .def("clearDisplays", &Config::clearDisplays, 
             DOC(Config, clearDisplays))
        .def_static("AreViewsEqual", [](const ConstConfigRcPtr & first,
                                        const ConstConfigRcPtr & second,
                                        const char * dispName,
                                        const char * viewName)
            {
                return Config::AreViewsEqual(first, second, dispName, viewName);
            },
                    "first"_a, "second"_a, "dispName"_a, "viewName"_a,
                    DOC(Config, AreViewsEqual))

        // Virtual Display
        .def("hasVirtualView", &Config::hasVirtualView, "view"_a,
             DOC(Config, hasVirtualView))
        .def("addVirtualDisplayView", &Config::addVirtualDisplayView, 
             "view"_a, "viewTransformName"_a, "colorSpaceName"_a, 
             "looks"_a = "",
             "ruleName"_a = "", 
             "description"_a = "", 
             DOC(Config, addVirtualDisplayView))
        .def("isVirtualViewShared", &Config::isVirtualViewShared, "view"_a,
             DOC(Config, isVirtualViewShared))
        .def("addVirtualDisplaySharedView", &Config::addVirtualDisplaySharedView, "sharedView"_a,
             DOC(Config, addVirtualDisplaySharedView))
        .def("getVirtualDisplayViews", [](ConfigRcPtr & self, ViewType type)
             {
                 return VirtualViewIterator(self, type);
             },
             "display"_a)
        .def("getVirtualDisplayViewTransformName", &Config::getVirtualDisplayViewTransformName, 
             "view"_a,
             DOC(Config, getVirtualDisplayViewTransformName))
        .def("getVirtualDisplayViewColorSpaceName", &Config::getVirtualDisplayViewColorSpaceName, 
             "view"_a,
             DOC(Config, getVirtualDisplayViewColorSpaceName))
        .def("getVirtualDisplayViewLooks", &Config::getVirtualDisplayViewLooks, "view"_a,
             DOC(Config, getVirtualDisplayViewLooks))
        .def("getVirtualDisplayViewRule", &Config::getVirtualDisplayViewRule, "view"_a,
             DOC(Config, getVirtualDisplayViewRule))
        .def("getVirtualDisplayViewDescription", &Config::getVirtualDisplayViewDescription, 
             "view"_a,
             DOC(Config, getVirtualDisplayViewDescription))
        .def("removeVirtualDisplayView", &Config::removeVirtualDisplayView, "view"_a,
             DOC(Config, removeVirtualDisplayView))
        .def("clearVirtualDisplay", &Config::clearVirtualDisplay,
             DOC(Config, clearVirtualDisplay))
        .def("instantiateDisplayFromMonitorName", &Config::instantiateDisplayFromMonitorName, 
             "monitorName"_a,
             DOC(Config, instantiateDisplayFromMonitorName))
        .def("instantiateDisplayFromICCProfile", &Config::instantiateDisplayFromICCProfile, 
             "ICCProfileFilepath"_a,
             DOC(Config, instantiateDisplayFromICCProfile))
        .def("isDisplayTemporary", [](ConfigRcPtr & self, const std::string & display) -> bool
             {
                 for (int i = 0; i < self->getNumDisplaysAll(); i++)
                 {
                     std::string other(self->getDisplayAll(i));
                     if (StringUtils::Compare(display, other))
                     {
                         return self->isDisplayTemporary(i);
                     }
                 }
                 return false;
             },
             "display"_a)
        .def("setDisplayTemporary", [](ConfigRcPtr & self, const std::string & display, bool isTemporary)
             {
                 for (int i = 0; i < self->getNumDisplaysAll(); i++)
                 {
                     std::string other(self->getDisplayAll(i));
                     if (StringUtils::Compare(display, other))
                     {
                         self->setDisplayTemporary(i, isTemporary);
                     }
                 }
             },
             "display"_a, "isTemporary"_a)
        .def_static("AreVirtualViewsEqual", [](const ConstConfigRcPtr & first,
                                               const ConstConfigRcPtr & second,
                                               const char * viewName)
            {
                return Config::AreVirtualViewsEqual(first, second, viewName);
            },
                    "first"_a, "second"_a, "viewName"_a,
                    DOC(Config, AreVirtualViewsEqual))

        // Active Displays and Views
        .def("setActiveDisplays", &Config::setActiveDisplays, "displays"_a, 
             DOC(Config, setActiveDisplays))
        .def("getActiveDisplays", [](ConfigRcPtr & self) 
            { 
                return ActiveDisplaysListIterator(self); 
            })
        .def("addActiveDisplay", &Config::addActiveDisplay, "display"_a, 
             DOC(Config, addActiveDisplay))
        .def("removeActiveDisplay", &Config::removeActiveDisplay, "display"_a, 
             DOC(Config, removeActiveDisplay))
        .def("clearActiveDisplays", &Config::clearActiveDisplays, 
             DOC(Config, clearActiveDisplays))
        .def("getNumActiveDisplays", &Config::getNumActiveDisplays, 
             DOC(Config, getNumActiveDisplays))

        .def("setActiveViews", &Config::setActiveViews, "views"_a, 
             DOC(Config, setActiveViews))
        .def("getActiveViews", [](ConfigRcPtr & self) 
            { 
                return ActiveViewsListIterator(self); 
            })
        .def("addActiveView", &Config::addActiveView, "view"_a, 
             DOC(Config, addActiveView))
        .def("removeActiveView", &Config::removeActiveView, "view"_a, 
             DOC(Config, removeActiveView))
        .def("clearActiveViews", &Config::clearActiveViews, 
             DOC(Config, clearActiveViews))
        .def("getNumActiveViews", &Config::getNumActiveViews, 
             DOC(Config, getNumActiveViews))

        // Luma
        .def("getDefaultLumaCoefs", [](ConfigRcPtr & self)
            {
                std::array<double, 3> rgb;
                self->getDefaultLumaCoefs(rgb.data());
                return rgb;
            }, 
             DOC(Config, getDefaultLumaCoefs))
        .def("setDefaultLumaCoefs", [](ConfigRcPtr & self, const std::array<double, 3> & rgb)
            {
                self->setDefaultLumaCoefs(rgb.data());
            }, 
             "rgb"_a, 
             DOC(Config, setDefaultLumaCoefs))

        // Look
        .def("getLook", &Config::getLook, "name"_a, 
             DOC(Config, getLook))
        .def("getLookNames", [](ConfigRcPtr & self) 
            { 
                return LookNameIterator(self); 
            })
        .def("getLooks", [](ConfigRcPtr & self) 
            { 
                return LookIterator(self); 
            })
        .def("addLook", &Config::addLook, "look"_a, 
             DOC(Config, addLook))
        .def("clearLooks", &Config::clearLooks, 
             DOC(Config, clearLooks))

        // View Transforms
        .def("getViewTransform", &Config::getViewTransform, "name"_a, 
             DOC(Config, getViewTransform))
        .def("getViewTransformNames", [](ConfigRcPtr & self) 
            { 
                return ViewTransformNameIterator(self); 
            })
        .def("getViewTransforms", [](ConfigRcPtr & self) 
            { 
                return ViewTransformIterator(self); 
            })
        .def("addViewTransform", &Config::addViewTransform, "viewTransform"_a,
             DOC(Config, addViewTransform))
        .def("getDefaultSceneToDisplayViewTransform", 
             &Config::getDefaultSceneToDisplayViewTransform, 
             DOC(Config, getDefaultSceneToDisplayViewTransform))
        .def("getDefaultViewTransformName",
            &Config::getDefaultViewTransformName,
            DOC(Config, getDefaultViewTransformName))
        .def("setDefaultViewTransformName",
            &Config::setDefaultViewTransformName, "name"_a.none(false),
            DOC(Config, setDefaultViewTransformName))
        .def("clearViewTransforms", &Config::clearViewTransforms, 
             DOC(Config, clearViewTransforms))

        // Named Transforms
        .def("getNamedTransform", &Config::getNamedTransform, "name"_a)

        .def("getNamedTransformNames", [](ConfigRcPtr & self,
                                          NamedTransformVisibility visibility)
            {
                return NamedTransformNameIterator(self, visibility);
            }, "visibility"_a)
        .def("getNamedTransforms", [](ConfigRcPtr & self,
                                      NamedTransformVisibility visibility)
            {
                return NamedTransformIterator(self, visibility);
            }, "visibility"_a)

        .def("getNamedTransformNames", [](ConfigRcPtr & self)
        {
                return ActiveNamedTransformNameIterator(self);
        })
        .def("getNamedTransforms", [](ConfigRcPtr & self)
        {
                return ActiveNamedTransformIterator(self);
        })
        .def("addNamedTransform", &Config::addNamedTransform, "namedTransform"_a)
        .def("clearNamedTransforms", &Config::clearNamedTransforms)

        // Viewing Rules
        .def("getViewingRules", &Config::getViewingRules, 
             DOC(Config, getViewingRules))
        .def("setViewingRules", &Config::setViewingRules, "ViewingRules"_a, 
             DOC(Config, setViewingRules))

        // File Rules
        .def("getFileRules", &Config::getFileRules, 
             DOC(Config, getFileRules))
        .def("setFileRules", &Config::setFileRules, "fileRules"_a, 
             DOC(Config, setFileRules))
        .def("getColorSpaceFromFilepath",
            [](ConfigRcPtr & self, const std::string & filePath)
            {
                size_t ruleIndex = 0;
                std::string csName = self->getColorSpaceFromFilepath(filePath.c_str(), ruleIndex);
                return py::make_tuple(csName, ruleIndex);
            }, "filePath"_a, 
            DOC(Config, getColorSpaceFromFilepath))
        .def("filepathOnlyMatchesDefaultRule", &Config::filepathOnlyMatchesDefaultRule, 
             "filePath"_a, 
             DOC(Config, filepathOnlyMatchesDefaultRule))

        // Processors
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstColorSpaceRcPtr &, 
                                              const ConstColorSpaceRcPtr &) const) 
             &Config::getProcessor, 
             "srcColorSpace"_a, "dstColorSpace"_a, 
             DOC(Config, getProcessor))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstContextRcPtr &, 
                                              const ConstColorSpaceRcPtr &, 
                                              const ConstColorSpaceRcPtr &) const) 
             &Config::getProcessor, 
             "context"_a, "srcColorSpace"_a, "dstColorSpace"_a, 
             DOC(Config, getProcessor, 2))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const char *, const char *) const) 
             &Config::getProcessor, 
             "srcColorSpaceName"_a, "dstColorSpaceName"_a, 
             DOC(Config, getProcessor, 3))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstContextRcPtr &, 
                                              const char *, 
                                              const char *) const) 
             &Config::getProcessor, 
             "context"_a, "srcColorSpaceName"_a, "dstColorSpaceName"_a, 
             DOC(Config, getProcessor, 4))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const char *,
                                              const char *,
                                              const char *,
                                              TransformDirection) const) 
             &Config::getProcessor, 
             "srcColorSpaceName"_a, "display"_a, "view"_a, "direction"_a, 
             DOC(Config, getProcessor, 5))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstContextRcPtr &, 
                                              const char *, 
                                              const char *, 
                                              const char *,
                                              TransformDirection) const) 
             &Config::getProcessor, 
             "context"_a, "srcColorSpaceName"_a, "display"_a, "view"_a, "direction"_a, 
             DOC(Config, getProcessor, 6))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstNamedTransformRcPtr &, 
                                              TransformDirection) const) 
             &Config::getProcessor, 
             "namedTransform"_a, "direction"_a, 
             DOC(Config, getProcessor, 7))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstContextRcPtr &, 
                                              const ConstNamedTransformRcPtr &, 
                                              TransformDirection) const) 
             &Config::getProcessor, 
             "context"_a, "namedTransform"_a, "direction"_a, 
             DOC(Config, getProcessor, 8))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const char *, 
                                              TransformDirection) const) 
             &Config::getProcessor, 
             "namedTransformName"_a, "direction"_a,
             DOC(Config, getProcessor, 9))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstContextRcPtr &, 
                                              const char *,
                                              TransformDirection) const)
             &Config::getProcessor, 
             "context"_a, "namedTransformName"_a, "direction"_a,
             DOC(Config, getProcessor, 10))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstTransformRcPtr &) const) 
             &Config::getProcessor, 
             "transform"_a, 
             DOC(Config, getProcessor, 11))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstTransformRcPtr &, 
                                              TransformDirection) const) 
             &Config::getProcessor, 
             "transform"_a, "direction"_a, 
             DOC(Config, getProcessor, 12))
        .def("getProcessor", 
             (ConstProcessorRcPtr (Config::*)(const ConstContextRcPtr &, 
                                              const ConstTransformRcPtr &, 
                                              TransformDirection) const) 
             &Config::getProcessor, 
             "context"_a, "transform"_a, "direction"_a, 
             DOC(Config, getProcessor, 13))
        .def_static("GetProcessorToBuiltinColorSpace", [](const ConstConfigRcPtr & srcConfig,
                                                          const char * srcColorSpaceName,
                                                          const char * builtinColorSpaceName)
            {
                return Config::GetProcessorToBuiltinColorSpace(srcConfig,
                                                               srcColorSpaceName,
                                                               builtinColorSpaceName);
            },
                    "srcConfig"_a, "srcColorSpaceName"_a, "builtinColorSpaceName"_a,
                    DOC(Config, GetProcessorToBuiltinColorSpace))
        .def_static("GetProcessorFromBuiltinColorSpace", [](const char * builtinColorSpaceName,
                                                          ConstConfigRcPtr srcConfig,
                                                          const char * srcColorSpaceName)
            {
                return Config::GetProcessorFromBuiltinColorSpace(builtinColorSpaceName,
                                                                 srcConfig,
                                                                 srcColorSpaceName);
            },
                    "builtinColorSpaceName"_a, "srcConfig"_a, "srcColorSpaceName"_a,
                    DOC(Config, GetProcessorFromBuiltinColorSpace))
        .def_static("GetProcessorFromConfigs", [](const ConstConfigRcPtr & srcConfig,
                                                  const char * srcColorSpaceName,
                                                  const ConstConfigRcPtr & dstConfig,
                                                  const char * dstColorSpaceName)
            {
                return Config::GetProcessorFromConfigs(srcConfig, srcColorSpaceName,
                                                       dstConfig, dstColorSpaceName);
            },
                    "srcConfig"_a, "srcColorSpaceName"_a, "dstConfig"_a, "dstColorSpaceName"_a, 
                    DOC(Config, GetProcessorFromConfigs))
        .def_static("GetProcessorFromConfigs", [](const ConstContextRcPtr & srcContext,
                                                  const ConstConfigRcPtr & srcConfig,
                                                  const char * srcColorSpaceName,
                                                  const ConstContextRcPtr & dstContext,
                                                  const ConstConfigRcPtr & dstConfig,
                                                  const char * dstColorSpaceName)
            {
                return Config::GetProcessorFromConfigs(srcContext, srcConfig, srcColorSpaceName,
                                                       dstContext, dstConfig, dstColorSpaceName);
            },
                    "srcContext"_a, "srcConfig"_a, "srcColorSpaceName"_a, 
                    "dstContext"_a, "dstConfig"_a, "dstColorSpaceName"_a, 
                    DOC(Config, GetProcessorFromConfigs, 2))
        .def_static("GetProcessorFromConfigs", [](const ConstConfigRcPtr & srcConfig,
                                                  const char * srcColorSpaceName,
                                                  const char * srcInterchangeName,
                                                  const ConstConfigRcPtr & dstConfig,
                                                  const char * dstColorSpaceName,
                                                  const char * dstInterchangeName)
            {
                return Config::GetProcessorFromConfigs(srcConfig, 
                                                       srcColorSpaceName, 
                                                       srcInterchangeName,
                                                       dstConfig, 
                                                       dstColorSpaceName, 
                                                       dstInterchangeName);
            }, 
                    "srcConfig"_a, "srcColorSpaceName"_a, "srcInterchangeName"_a, 
                    "dstConfig"_a, "dstColorSpaceName"_a, "dstInterchangeName"_a, 
                    DOC(Config, GetProcessorFromConfigs, 3))
        .def_static("GetProcessorFromConfigs", [](const ConstContextRcPtr & srcContext,
                                                  const ConstConfigRcPtr & srcConfig,
                                                  const char * srcColorSpaceName,
                                                  const char * srcInterchangeName,
                                                  const ConstContextRcPtr & dstContext,
                                                  const ConstConfigRcPtr & dstConfig,
                                                  const char * dstColorSpaceName,
                                                  const char * dstInterchangeName)
            {
                return Config::GetProcessorFromConfigs(srcContext, 
                                                       srcConfig, 
                                                       srcColorSpaceName, 
                                                       srcInterchangeName,
                                                       dstContext, 
                                                       dstConfig, 
                                                       dstColorSpaceName, 
                                                       dstInterchangeName);
            }, 
                    "srcContext"_a, "srcConfig"_a, "srcColorSpaceName"_a, "srcInterchangeName"_a, 
                    "dstContext"_a, "dstConfig"_a, "dstColorSpaceName"_a, "dstInterchangeName"_a, 
                    DOC(Config, GetProcessorFromConfigs, 4))
        .def_static("GetProcessorFromConfigs", [](const ConstConfigRcPtr & srcConfig,
                                                  const char * srcColorSpaceName,
                                                  const ConstConfigRcPtr & dstConfig,
                                                  const char * dstDisplay,
                                                  const char * dstView,
                                                  TransformDirection direction)
            {
                return Config::GetProcessorFromConfigs(srcConfig, srcColorSpaceName,
                                                       dstConfig, dstDisplay, dstView, direction);
            },
                    "srcConfig"_a, "srcColorSpaceName"_a, "dstConfig"_a, "dstDisplay"_a, "dstView"_a, "direction"_a,
                    DOC(Config, GetProcessorFromConfigs, 5))
        .def_static("GetProcessorFromConfigs", [](const ConstContextRcPtr & srcContext,
                                                  const ConstConfigRcPtr & srcConfig,
                                                  const char * srcColorSpaceName,
                                                  const ConstContextRcPtr & dstContext,
                                                  const ConstConfigRcPtr & dstConfig,
                                                  const char * dstDisplay,
                                                  const char * dstView,
                                                  TransformDirection direction)
            {
                return Config::GetProcessorFromConfigs(srcContext, srcConfig, srcColorSpaceName,
                                                       dstContext, dstConfig, dstDisplay, dstView, direction);
            },
                    "srcContext"_a, "srcConfig"_a, "srcColorSpaceName"_a,
                    "dstContext"_a, "dstConfig"_a, "dstView"_a, "dstDisplay"_a, "direction"_a,
                    DOC(Config, GetProcessorFromConfigs, 6))
        .def_static("GetProcessorFromConfigs", [](const ConstConfigRcPtr & srcConfig,
                                                  const char * srcColorSpaceName,
                                                  const char * srcInterchangeName,
                                                  const ConstConfigRcPtr & dstConfig,
                                                  const char * dstDisplay,
                                                  const char * dstView,
                                                  const char * dstInterchangeName,
                                                  TransformDirection direction)
            {
                return Config::GetProcessorFromConfigs(srcConfig,
                                                       srcColorSpaceName,
                                                       srcInterchangeName,
                                                       dstConfig,
                                                       dstDisplay,
                                                       dstView,
                                                       dstInterchangeName,
                                                       direction);
            },
                    "srcConfig"_a, "srcColorSpaceName"_a, "srcInterchangeName"_a,
                    "dstConfig"_a, "dstDisplay"_a, "dstView"_a, "dstInterchangeName"_a, "direction"_a,
                    DOC(Config, GetProcessorFromConfigs, 7))
        .def_static("GetProcessorFromConfigs", [](const ConstContextRcPtr & srcContext,
                                                  const ConstConfigRcPtr & srcConfig,
                                                  const char * srcColorSpaceName,
                                                  const char * srcInterchangeName,
                                                  const ConstContextRcPtr & dstContext,
                                                  const ConstConfigRcPtr & dstConfig,
                                                  const char * dstDisplay,
                                                  const char * dstView,
                                                  const char * dstInterchangeName,
                                                  TransformDirection direction)
            {
                return Config::GetProcessorFromConfigs(srcContext,
                                                       srcConfig,
                                                       srcColorSpaceName,
                                                       srcInterchangeName,
                                                       dstContext,
                                                       dstConfig,
                                                       dstDisplay,
                                                       dstView,
                                                       dstInterchangeName,
                                                       direction);
            },
                    "srcContext"_a, "srcConfig"_a, "srcColorSpaceName"_a, "srcInterchangeName"_a,
                    "dstContext"_a, "dstConfig"_a, "dstDisplay"_a, "dstView"_a, "dstInterchangeName"_a, "direction"_a,
                    DOC(Config, GetProcessorFromConfigs, 8))
        .def("setProcessorCacheFlags", &Config::setProcessorCacheFlags, "flags"_a, 
             DOC(Config, setProcessorCacheFlags))
        .def("clearProcessorCache", &Config::clearProcessorCache, 
             DOC(Config, setProcessorCacheFlags))

        // Archiving
        .def("isArchivable", &Config::isArchivable, DOC(Config, isArchivable))
        .def("archive", [](ConfigRcPtr & self, const char * filepath) 
            {
                std::ofstream f(filepath, std::ofstream::out | std::ofstream::binary);
                self->archive(f);
                f.close(); 
            }, 
            DOC(Config, archive))

        // Conversion to string
        .def("__str__", [](ConfigRcPtr & self)
            {
                std::ostringstream os;
                os << (*self);
                return os.str();
            })
        .def("__repr__", [](ConfigRcPtr & self)
            {
                std::ostringstream os;
                os << "<Config name=";
                std::string name{ self->getName() };
                if (!name.empty())
                {
                    os << name;
                }
                os << ", description=";
                std::string desc{ self->getDescription() };
                if (!desc.empty())
                {
                    os << desc;
                }
                os << ", ocio_profile_version=" << self->getMajorVersion();
                const unsigned int minor = self->getMinorVersion();
                if (minor)
                {
                    os << "." << minor;
                }
                os << ", active_colorspaces=" << self->getNumColorSpaces();
                os << ", active_displays=" << self->getNumDisplays();
                os << ">";
                return os.str();
            });

    clsEnvironmentVarNameIterator
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

    clsSearchPathIterator
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

    clsColorSpaceNameIterator
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

    clsColorSpaceIterator
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

    clsActiveColorSpaceNameIterator
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

    clsActiveColorSpaceIterator
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

    clsRoleNameIterator
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

    clsRoleColorSpaceIterator
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

    clsDisplayIterator
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

    clsDisplayAllIterator
        .def("__len__", [](DisplayAllIterator & it) { return it.m_obj->getNumDisplaysAll(); })
        .def("__getitem__", [](DisplayAllIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumDisplaysAll());
                return it.m_obj->getDisplayAll(i);
            })
        .def("__iter__", [](DisplayAllIterator & it) -> DisplayAllIterator & { return it; })
        .def("__next__", [](DisplayAllIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumDisplaysAll());
                return it.m_obj->getDisplayAll(i);
            });

    clsSharedViewIterator
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

    clsVirtualViewIterator
        .def("__len__", [](VirtualViewIterator & it)
            { 
                return it.m_obj->getVirtualDisplayNumViews(std::get<0>(it.m_args)); 
            })
        .def("__getitem__", [](VirtualViewIterator & it, int i)
            { 
                it.checkIndex(i, it.m_obj->getVirtualDisplayNumViews(std::get<0>(it.m_args)));
                return it.m_obj->getVirtualDisplayView(std::get<0>(it.m_args), i);
            })
        .def("__iter__", [](VirtualViewIterator & it) -> VirtualViewIterator & { return it; })
        .def("__next__", [](VirtualViewIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getVirtualDisplayNumViews(std::get<0>(it.m_args)));
                return it.m_obj->getVirtualDisplayView(std::get<0>(it.m_args), i);
            });

    clsViewIterator
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

    clsViewForColorSpaceIterator
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
        .def("__iter__", [](ViewForColorSpaceIterator & it) -> ViewForColorSpaceIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ViewForColorSpaceIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumViews(std::get<0>(it.m_args).c_str(),
                                                           std::get<1>(it.m_args).c_str()));
                return it.m_obj->getView(std::get<0>(it.m_args).c_str(),
                                         std::get<1>(it.m_args).c_str(), i);
            });

    clsViewForViewTypeIterator
        .def("__len__", [](ViewForViewTypeIterator & it)
                        { return it.m_obj->getNumViews(std::get<0>(it.m_args),
                                                       std::get<1>(it.m_args).c_str()); })
        .def("__getitem__", [](ViewForViewTypeIterator & it, int i)
            { 
                it.checkIndex(i, it.m_obj->getNumViews(std::get<0>(it.m_args),
                                                       std::get<1>(it.m_args).c_str()));
                return it.m_obj->getView(std::get<0>(it.m_args),
                                         std::get<1>(it.m_args).c_str(), i);
            })
        .def("__iter__", [](ViewForViewTypeIterator & it) -> ViewForViewTypeIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ViewForViewTypeIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumViews(std::get<0>(it.m_args),
                                                           std::get<1>(it.m_args).c_str()));
                return it.m_obj->getView(std::get<0>(it.m_args),
                                         std::get<1>(it.m_args).c_str(), i);
            });

    clsActiveDisplaysListIterator
        .def("__len__", [](ActiveDisplaysListIterator & it) { return it.m_obj->getNumActiveDisplays(); })
        .def("__getitem__", [](ActiveDisplaysListIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumActiveDisplays());
                return it.m_obj->getActiveDisplay(i);
            })
        .def("__iter__", [](ActiveDisplaysListIterator & it) -> ActiveDisplaysListIterator & { return it; })
        .def("__next__", [](ActiveDisplaysListIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumActiveDisplays());
                return it.m_obj->getActiveDisplay(i);
            });

    clsActiveViewsListIterator
        .def("__len__", [](ActiveViewsListIterator & it) { return it.m_obj->getNumActiveViews(); })
        .def("__getitem__", [](ActiveViewsListIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumActiveViews());
                return it.m_obj->getActiveView(i);
            })
        .def("__iter__", [](ActiveViewsListIterator & it) -> ActiveViewsListIterator & { return it; })
        .def("__next__", [](ActiveViewsListIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumActiveViews());
                return it.m_obj->getActiveView(i);
            });

    clsLookNameIterator
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

    clsLookIterator
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

    clsViewTransformNameIterator
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

    clsViewTransformIterator
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
        .def("__iter__", [](ViewTransformIterator & it) -> ViewTransformIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ViewTransformIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumViewTransforms());
                const char * name = it.m_obj->getViewTransformNameByIndex(i);
                return it.m_obj->getViewTransform(name);
            });

    clsNamedTransformNameIterator
        .def("__len__", [](NamedTransformNameIterator & it)
            {
                return it.m_obj->getNumNamedTransforms(std::get<0>(it.m_args));
            })
        .def("__getitem__", [](NamedTransformNameIterator & it, int i)
            {
                it.checkIndex(i, it.m_obj->getNumNamedTransforms(std::get<0>(it.m_args)));
                return it.m_obj->getNamedTransformNameByIndex(std::get<0>(it.m_args), i);
            })
        .def("__iter__", [](NamedTransformNameIterator & it) -> NamedTransformNameIterator &
            {
                return it;
            })
        .def("__next__", [](NamedTransformNameIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumNamedTransforms(std::get<0>(it.m_args)));
                return it.m_obj->getNamedTransformNameByIndex(std::get<0>(it.m_args), i);
            });

    clsNamedTransformIterator
        .def("__len__", [](NamedTransformIterator & it)
            {
                return it.m_obj->getNumNamedTransforms(std::get<0>(it.m_args));
            })
        .def("__getitem__", [](NamedTransformIterator & it, int i)
            {
                it.checkIndex(i, it.m_obj->getNumNamedTransforms(std::get<0>(it.m_args)));
                const char * name = it.m_obj->getNamedTransformNameByIndex(std::get<0>(it.m_args), i);
                return it.m_obj->getNamedTransform(name);
            })
        .def("__iter__", [](NamedTransformIterator & it) -> NamedTransformIterator &
            {
                return it;
            })
        .def("__next__", [](NamedTransformIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumNamedTransforms(std::get<0>(it.m_args)));
                const char * name = it.m_obj->getNamedTransformNameByIndex(std::get<0>(it.m_args), i);
                return it.m_obj->getNamedTransform(name);
            });

    clsActiveNamedTransformNameIterator
        .def("__len__", [](ActiveNamedTransformNameIterator & it)
            {
                return it.m_obj->getNumNamedTransforms();
            })
        .def("__getitem__", [](ActiveNamedTransformNameIterator & it, int i)
            {
                it.checkIndex(i, (int)it.m_obj->getNumNamedTransforms());
                return it.m_obj->getNamedTransformNameByIndex(i);
            })
        .def("__iter__", [](ActiveNamedTransformNameIterator & it) -> ActiveNamedTransformNameIterator &
            {
                return it;
            })
        .def("__next__", [](ActiveNamedTransformNameIterator & it)
            {
                int i = it.nextIndex((int)it.m_obj->getNumNamedTransforms());
                return it.m_obj->getNamedTransformNameByIndex(i);
            });

    clsActiveNamedTransformIterator
        .def("__len__", [](ActiveNamedTransformIterator & it)
            {
                return it.m_obj->getNumNamedTransforms();
            })
        .def("__getitem__", [](ActiveNamedTransformIterator & it, int i)
            {
                it.checkIndex(i, (int)it.m_obj->getNumNamedTransforms());
                const char * name = it.m_obj->getNamedTransformNameByIndex(i);
                return it.m_obj->getNamedTransform(name);
            })
        .def("__iter__", [](ActiveNamedTransformIterator & it) -> ActiveNamedTransformIterator &
            {
                return it;
            })
        .def("__next__", [](ActiveNamedTransformIterator & it)
            {
                int i = it.nextIndex((int)it.m_obj->getNumNamedTransforms());
                const char * name = it.m_obj->getNamedTransformNameByIndex(i);
                return it.m_obj->getNamedTransform(name);
            });

    m.def("GetCurrentConfig", &GetCurrentConfig, 
          DOC(PyOpenColorIO, GetCurrentConfig));
    m.def("SetCurrentConfig", &SetCurrentConfig, "config"_a, 
          DOC(PyOpenColorIO, SetCurrentConfig));

    m.def("ExtractOCIOZArchive", &ExtractOCIOZArchive, 
          DOC(PyOpenColorIO, ExtractOCIOZArchive));

    m.def("ResolveConfigPath", &ResolveConfigPath, 
          DOC(PyOpenColorIO, ResolveConfigPath));
}

} // namespace OCIO_NAMESPACE
