// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "CategoryHelpers.h"

#include "PyOpenColorIO_apphelpers.h"

namespace OCIO_NAMESPACE
{

void bindPyCategoryHelpers(py::module & m)
{
    m.def("ExtractCategories", &ExtractCategories, "categories"_a);
    m.def("FindColorSpaceNames", &FindColorSpaceNames, "config"_a, "categories"_a);
    m.def("FindAllColorSpaceNames", &FindAllColorSpaceNames, "config"_a);
    m.def("FindColorSpaceInfos", &FindColorSpaceInfos, "config"_a, "categories"_a);
    m.def("FindAllColorSpaceInfos", &FindAllColorSpaceInfos, "config"_a);
    m.def("GetRoleInfo", &GetRoleInfo, "config"_a, "role"_a);
    m.def("getColorSpaceInfosFromCategories", &getColorSpaceInfosFromCategories, 
          "config"_a, 
          "role"_a = "", 
          "categories"_a = "");
}

} // namespace OCIO_NAMESPACE
