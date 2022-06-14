// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyBuiltinConfigRegistry.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{
void bindPyBuiltinConfigRegistry(py::module & m)
{
    auto clsBuiltinConfigRegistry =
        py::class_<PyBuiltinConfigRegistry>(m.attr("BuiltinConfigRegistry"));

    clsBuiltinConfigRegistry
        .def(py::init<>(), DOC(BuiltinConfigRegistry, Get))
        .def("__len__", [](PyBuiltinConfigRegistry & self) { return self.getNumBuiltinConfigs(); },
             DOC(BuiltinConfigRegistry, getNumBuiltinConfigs))
        .def("getNumBuiltinConfigs", [](PyBuiltinConfigRegistry & self) 
            { 
                return self.getNumBuiltinConfigs(); 
            },
            DOC(BuiltinConfigRegistry, getNumBuiltinConfigs))
        .def("getBuiltinConfigName", [](PyBuiltinConfigRegistry & self, size_t configIndex) 
            { 
                return self.getBuiltinConfigName(configIndex); 
            },
            DOC(BuiltinConfigRegistry, getBuiltinConfigName))
        .def("getBuiltinConfigUIName", [](PyBuiltinConfigRegistry & self, size_t configIndex) 
            { 
                return self.getBuiltinConfigUIName(configIndex); 
            },
            DOC(BuiltinConfigRegistry, getBuiltinConfigUIName))
        .def("getBuiltinConfig", [](PyBuiltinConfigRegistry & self, size_t configIndex) 
            { 
                return self.getBuiltinConfig(configIndex); 
            },
            DOC(BuiltinConfigRegistry, getBuiltinConfig))
        .def("getBuiltinConfigByName", [](PyBuiltinConfigRegistry & self, const char * configName) 
            { 
                return self.getBuiltinConfigByName(configName); 
            },
            DOC(BuiltinConfigRegistry, getBuiltinConfigByName))
        .def("isBuiltinConfigRecommended", [](PyBuiltinConfigRegistry & self, size_t configIndex) 
            { 
                return self.isBuiltinConfigRecommended(configIndex); 
            },
            DOC(BuiltinConfigRegistry, isBuiltinConfigRecommended))
        .def("getDefaultBuiltinConfigName", [](PyBuiltinConfigRegistry & self) 
            { 
                return self.getDefaultBuiltinConfigName(); 
            },
            DOC(BuiltinConfigRegistry, getDefaultBuiltinConfigName));
}
}