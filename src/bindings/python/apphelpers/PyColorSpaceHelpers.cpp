// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <memory>

#include "ColorSpaceHelpers.h"

#include "PyOpenColorIO_apphelpers.h"

namespace OCIO_NAMESPACE
{

namespace
{

std::vector<std::string> stringsToVec(ConstStringsRcPtr s)
{
    std::vector<std::string> v;
    v.reserve(s->getNumString());
    for (size_t i = 0; i < s->getNumString(); i++)
    {
        v.push_back(s->getString(i));
    }
    return v;
}

} // namespace

void bindPyColorSpaceHelpers(py::module & m)
{
    py::class_<ColorSpaceInfo, ColorSpaceInfoRcPtr /* holder */>(m, "ColorSpaceInfo")
        .def(py::init([](ConstConfigRcPtr config,
                         const char * name,
                         const char * family,
                         const char * description)
            { 
                return ColorSpaceInfo::Create(config, name, family, description);
            }), 
             "config"_a, "name"_a, "family"_a, "description"_a)
        .def(py::init([](ConstConfigRcPtr config,
                         const char * name,
                         const char * uiName,
                         const char * family,
                         const char * description) 
            { 
                return ColorSpaceInfo::Create(config, name, uiName, family, description);
            }), 
             "config"_a, "name"_a, "uiName"_a, "family"_a, "description"_a)
        .def(py::init([](ConstConfigRcPtr config, ConstColorSpaceRcPtr colorSpace) 
            { 
                return ColorSpaceInfo::Create(config, colorSpace);
            }), 
             "config"_a, "colorSpace"_a)

        .def("getName", &ColorSpaceInfo::getName)
        .def("getUIName", &ColorSpaceInfo::getUIName)
        .def("getFamily", &ColorSpaceInfo::getFamily)
        .def("getDescription", &ColorSpaceInfo::getDescription)
        .def("getHierarchyLevels", [](ColorSpaceInfoRcPtr & self)
            {
                ConstStringsRcPtr s = self->getHierarchyLevels();
                return stringsToVec(s);
            })
        .def("getDescriptions", [](ColorSpaceInfoRcPtr & self)
            {
                ConstStringsRcPtr s = self->getDescriptions();
                return stringsToVec(s);
            });

    py::class_<ColorSpaceMenuHelper, 
               ColorSpaceMenuHelperRcPtr /* holder */>(m, "ColorSpaceMenuHelper")
        .def(py::init([](ConstConfigRcPtr config,
                         const char * role,
                         const char * categories) 
            { 
                return ColorSpaceMenuHelper::Create(config, role, categories);
            }), 
             "config"_a, "role"_a, "categories"_a)

        .def("getNameFromUIName", &ColorSpaceMenuHelper::getNumColorSpaces)
        .def("getColorSpaceName", &ColorSpaceMenuHelper::getColorSpaceName, "idx"_a)
        .def("getColorSpaceUIName", &ColorSpaceMenuHelper::getColorSpaceUIName, "idx"_a)
        .def("getColorSpace", &ColorSpaceMenuHelper::getColorSpace, "idx"_a)
        .def("getNameFromUIName", &ColorSpaceMenuHelper::getNameFromUIName, "uiName"_a)
        .def("getUINameFromName", &ColorSpaceMenuHelper::getUINameFromName, "name"_a)
        .def("addColorSpaceToMenu", &ColorSpaceMenuHelper::addColorSpaceToMenu, "colorSpace"_a)
        .def("refresh", &ColorSpaceMenuHelper::refresh, "config"_a);

    m.def("GetProcessor", &ColorSpaceHelpers::GetProcessor, 
          "config"_a, "inputColorSpaceName"_a, "outputColorSpaceName"_a);
          
    // NOTE: 'connectionColorSpaceName' and 'categories' have reversed order from C++ API since 
    //       'categories' can be null and Python kwargs must follow args.
    m.def("AddColorSpace", [](ConfigRcPtr & config,
                              const ColorSpaceInfo & colorSpaceInfo,
                              FileTransformRcPtr & userTransform,
                              const char * connectionColorSpaceName,
                              const char * categories)
         {
            ColorSpaceHelpers::AddColorSpace(config, 
                                             colorSpaceInfo, 
                                             userTransform, 
                                             categories,
                                             connectionColorSpaceName);
         },
          "config"_a, 
          "colorSpaceInfo"_a, 
          "userTransform"_a, 
          "connectionColorSpaceName"_a,
          "categories"_a = "");
}

} // namespace OCIO_NAMESPACE
