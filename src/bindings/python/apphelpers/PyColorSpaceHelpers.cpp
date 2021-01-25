// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{

enum ParametersIterator
{
    IT_PARAMETERS_ADDED_COLORSPACE = 0
};

using AddedColorSpaceIterator = PyIterator<ColorSpaceMenuParametersRcPtr,
                                           IT_PARAMETERS_ADDED_COLORSPACE>;

enum MenuIterator
{
    IT_MENU_HIERARCHY_LEVEL = 0
};

using ColorSpaceLevelIterator = PyIterator<ColorSpaceMenuHelperRcPtr,
                                           IT_MENU_HIERARCHY_LEVEL, size_t>;

} // namespace

void bindPyColorSpaceMenuHelpers(py::module & m)
{
    auto mColorSpaceHelpers = m.def_submodule("ColorSpaceHelpers")
        .def("AddColorSpace", &ColorSpaceHelpers::AddColorSpace,
             "config"_a.none(false),
             "name"_a.none(false),
             "transformFilePath"_a.none(false),
             "categories"_a = "",
             "connectionColorSpaceName"_a.none(false),
             DOC(ColorSpaceHelpers, AddColorSpace));

    auto clsColorSpaceMenuParameters = 
        py::class_<ColorSpaceMenuParameters, ColorSpaceMenuParametersRcPtr>(
            m.attr("ColorSpaceMenuParameters"));

    auto clsAddedColorSpaceIterator = 
        py::class_<AddedColorSpaceIterator>(
            clsColorSpaceMenuParameters, "AddedColorSpaceIterator");

    auto clsColorSpaceMenuHelper =
        py::class_<ColorSpaceMenuHelper, ColorSpaceMenuHelperRcPtr>(
            m.attr("ColorSpaceMenuHelper"));

    auto clsColorSpaceLevelIterator = 
        py::class_<ColorSpaceLevelIterator>(
            clsColorSpaceMenuHelper, "ColorSpaceLevelIterator");

    clsColorSpaceMenuParameters
        .def(py::init(&ColorSpaceMenuParameters::Create),
             "config"_a.none(false),
             DOC(ColorSpaceMenuParameters, Create))
        .def(py::init([](ConstConfigRcPtr config,
                         const std::string & role,
                         bool includeColorSpaces,
                         SearchReferenceSpaceType searchReferenceSpaceType,
                         bool includeNamedTransforms,
                         const std::string & appCategories,
                         const std::string & encodings,
                         const std::string & userCategories,
                         bool includeRoles)
             {
                 ColorSpaceMenuParametersRcPtr p = ColorSpaceMenuParameters::Create(config);
                 if (!role.empty())
                 {
                     p->setRole(role.c_str());
                 }
                 if (!appCategories.empty())
                 {
                     p->setAppCategories(appCategories.c_str());
                 }
                 if (!userCategories.empty())
                 {
                     p->setUserCategories(userCategories.c_str());
                 }
                 if (!encodings.empty())
                 {
                     p->setEncodings(encodings.c_str());
                 }
                 p->setSearchReferenceSpaceType(searchReferenceSpaceType);
                 p->setIncludeColorSpaces(includeColorSpaces);
                 p->setIncludeRoles(includeRoles);
                 p->setIncludeNamedTransforms(includeNamedTransforms);
                 return p;
            }),
             "config"_a.none(false),
             "role"_a.none(false) = "",
             "includeColorSpaces"_a = true,
             "searchReferenceSpaceType"_a = SEARCH_REFERENCE_SPACE_ALL,
             "includeNamedTransforms"_a = false,
             "appCategories"_a.none(false) = "",
             "encodings"_a.none(false) = "",
             "userCategories"_a.none(false) = "",
             "includeRoles"_a = false,
             DOC(ColorSpaceMenuParameters, Create))

        .def("getConfig", &ColorSpaceMenuParameters::getConfig,
             DOC(ColorSpaceMenuParameters, getConfig))
        .def("setConfig", &ColorSpaceMenuParameters::setConfig, "config"_a.none(false),
             DOC(ColorSpaceMenuParameters, setConfig))
        .def("getRole", &ColorSpaceMenuParameters::getRole,
             DOC(ColorSpaceMenuParameters, getRole))
        .def("setRole", &ColorSpaceMenuParameters::setRole, "role"_a.none(false),
             DOC(ColorSpaceMenuParameters, setRole))
        .def("getIncludeColorSpaces", &ColorSpaceMenuParameters::getIncludeColorSpaces,
             DOC(ColorSpaceMenuParameters, getIncludeColorSpaces))
        .def("setIncludeColorSpaces", &ColorSpaceMenuParameters::setIncludeColorSpaces,
             "includeColorSpaces"_a = true,
             DOC(ColorSpaceMenuParameters, setIncludeColorSpaces))
        .def("getSearchReferenceSpaceType", &ColorSpaceMenuParameters::getSearchReferenceSpaceType,
             DOC(ColorSpaceMenuParameters, getSearchReferenceSpaceType))
        .def("setSearchReferenceSpaceType", &ColorSpaceMenuParameters::setSearchReferenceSpaceType,
             "searchReferenceSpaceType"_a.none(false),
             DOC(ColorSpaceMenuParameters, setSearchReferenceSpaceType))
        .def("getIncludeNamedTransforms", &ColorSpaceMenuParameters::getIncludeNamedTransforms,
             DOC(ColorSpaceMenuParameters, getIncludeNamedTransforms))
        .def("setIncludeNamedTransforms", &ColorSpaceMenuParameters::setIncludeNamedTransforms,
             "includeNamedTransforms"_a = true,
             DOC(ColorSpaceMenuParameters, setIncludeNamedTransforms))
        .def("getEncodings", &ColorSpaceMenuParameters::getEncodings,
             DOC(ColorSpaceMenuParameters, getEncodings))
        .def("setEncodings", &ColorSpaceMenuParameters::setEncodings, "encodings"_a.none(false),
             DOC(ColorSpaceMenuParameters, setEncodings))
        .def("getAppCategories", &ColorSpaceMenuParameters::getAppCategories,
             DOC(ColorSpaceMenuParameters, getAppCategories))
        .def("setAppCategories", &ColorSpaceMenuParameters::setAppCategories,
             "appCategories"_a.none(false),
             DOC(ColorSpaceMenuParameters, setAppCategories))
        .def("getUserCategories", &ColorSpaceMenuParameters::getUserCategories,
             DOC(ColorSpaceMenuParameters, getUserCategories))
        .def("setUserCategories", &ColorSpaceMenuParameters::setUserCategories,
             "categories"_a.none(false),
             DOC(ColorSpaceMenuParameters, setUserCategories))
        .def("getIncludeRoles", &ColorSpaceMenuParameters::getIncludeRoles,
             DOC(ColorSpaceMenuParameters, getIncludeRoles))
        .def("setIncludeRoles", &ColorSpaceMenuParameters::setIncludeRoles,
             "includeRoles"_a = true,
             DOC(ColorSpaceMenuParameters, setIncludeRoles))
        .def("addColorSpace", &ColorSpaceMenuParameters::addColorSpace,
             "colorSpace"_a.none(false),
             DOC(ColorSpaceMenuParameters, addColorSpace))
        .def("getAddedColorSpaces", [](ColorSpaceMenuParametersRcPtr & self)
            {
                return AddedColorSpaceIterator(self);
            })
        .def("clearAddedColorSpaces", &ColorSpaceMenuParameters::clearAddedColorSpaces,
             DOC(ColorSpaceMenuParameters, clearAddedColorSpaces));

    defRepr(clsColorSpaceMenuParameters);

    clsAddedColorSpaceIterator
        .def("__len__", [](AddedColorSpaceIterator & it)
            {
                return it.m_obj->getNumAddedColorSpaces();
            })
        .def("__getitem__", [](AddedColorSpaceIterator & it, int i)
            {
                it.checkIndex(i, (int)it.m_obj->getNumAddedColorSpaces());
                return it.m_obj->getAddedColorSpace(i);
            })
        .def("__iter__", [](AddedColorSpaceIterator & it) -> AddedColorSpaceIterator &
            {
                return it;
            })
        .def("__next__", [](AddedColorSpaceIterator & it)
            {
                int i = it.nextIndex((int)it.m_obj->getNumAddedColorSpaces());
                return it.m_obj->getAddedColorSpace(i);
            });

    clsColorSpaceMenuHelper
        .def(py::init(&ColorSpaceMenuHelper::Create), "parameters"_a.none(false),
             DOC(ColorSpaceMenuHelper, Create))
        .def("getNumColorSpaces", &ColorSpaceMenuHelper::getNumColorSpaces,
             DOC(ColorSpaceMenuHelper, getNumColorSpaces))
        .def("getName", &ColorSpaceMenuHelper::getName, "index"_a,
             DOC(ColorSpaceMenuHelper, getName))
        .def("getUIName", &ColorSpaceMenuHelper::getUIName, "index"_a,
             DOC(ColorSpaceMenuHelper, getUIName))
        .def("getIndexFromName", &ColorSpaceMenuHelper::getIndexFromName, "name"_a.none(false),
             DOC(ColorSpaceMenuHelper, getIndexFromName))
        .def("getIndexFromUIName", &ColorSpaceMenuHelper::getIndexFromUIName, "name"_a.none(false),
             DOC(ColorSpaceMenuHelper, getIndexFromUIName))
        .def("getDescription", &ColorSpaceMenuHelper::getDescription, "index"_a,
             DOC(ColorSpaceMenuHelper, getDescription))
        .def("getFamily", &ColorSpaceMenuHelper::getFamily, "index"_a,
             DOC(ColorSpaceMenuHelper, getFamily))
        .def("getHierarchyLevels",
             [](ColorSpaceMenuHelperRcPtr & self, size_t index)
            {
                 return ColorSpaceLevelIterator(self, index);
            }, 
             "index"_a)
        .def("getNameFromUIName", &ColorSpaceMenuHelper::getNameFromUIName, "name"_a.none(false),
             DOC(ColorSpaceMenuHelper, getNameFromUIName))
        .def("getUINameFromName", &ColorSpaceMenuHelper::getUINameFromName, "name"_a.none(false),
             DOC(ColorSpaceMenuHelper, getUINameFromName));

    defRepr(clsColorSpaceMenuHelper);

    clsColorSpaceLevelIterator
        .def("__len__", [](ColorSpaceLevelIterator & it)
             {
                 return it.m_obj->getNumHierarchyLevels(std::get<0>(it.m_args));
             })
        .def("__getitem__", [](ColorSpaceLevelIterator & it, int i)
             {
                 it.checkIndex(i, static_cast<int>(it.m_obj->getNumHierarchyLevels(std::get<0>(it.m_args))));
                 return it.m_obj->getHierarchyLevel(std::get<0>(it.m_args), static_cast<size_t>(i));
             })
        .def("__iter__", [](ColorSpaceLevelIterator & it) -> ColorSpaceLevelIterator &
             {
                 return it;
             })
        .def("__next__", [](ColorSpaceLevelIterator & it)
             {
                 int i = it.nextIndex(static_cast<int>(it.m_obj->getNumHierarchyLevels(std::get<0>(it.m_args))));
                 return it.m_obj->getHierarchyLevel(std::get<0>(it.m_args), static_cast<size_t>(i));
             });
}

} // namespace OCIO_NAMESPACE
