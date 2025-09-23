// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyBuiltinConfigRegistry.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

enum BuiltinConfigRegistryIterator
{
    IT_BUILTIN_CONFIG_NAME = 0,
    IT_BUILTIN_CONFIG
};
using BuiltinConfigNameIterator = PyIterator<PyBuiltinConfigRegistry, IT_BUILTIN_CONFIG_NAME>;
using BuiltinConfigIterator = PyIterator<PyBuiltinConfigRegistry, IT_BUILTIN_CONFIG>;

void bindPyBuiltinConfigRegistry(py::module & m)
{
    auto clsBuiltinConfigRegistry =
        py::class_<PyBuiltinConfigRegistry>(m.attr("BuiltinConfigRegistry"));

    auto clsBuiltinConfigNameIterator = 
        py::class_<BuiltinConfigNameIterator>(
            clsBuiltinConfigRegistry, "BuiltinConfigNameIterator");

    auto clsBuiltinConfigIterator= 
        py::class_<BuiltinConfigIterator>(
            clsBuiltinConfigRegistry, "BuiltinConfigIterator");

    clsBuiltinConfigRegistry
        .def(py::init<>(), DOC(BuiltinConfigRegistry, Get))
        .def("__iter__", [](PyBuiltinConfigRegistry & self)
            { 
                return BuiltinConfigNameIterator(self); 
            },
             DOC(BuiltinConfigRegistry, getBuiltinConfigName))
        .def("__len__", [](PyBuiltinConfigRegistry & self) 
            { 
                return self.getNumBuiltinConfigs(); 
            },
            DOC(BuiltinConfigRegistry, getNumBuiltinConfigs))
        .def("__getitem__", [](PyBuiltinConfigRegistry & self, const std::string & name) 
            { 
                return self.getBuiltinConfigByName(name.c_str());
            },
            DOC(BuiltinConfigRegistry, getBuiltinConfigByName))
        .def("__contains__", [](PyBuiltinConfigRegistry & self, const std::string & name) 
            {
                for (size_t i = 0; i < self.getNumBuiltinConfigs(); i++)
                {
                    if (StringUtils::Compare(
                            std::string(self.getBuiltinConfigName(i)), 
                            std::string(name)))
                    {
                        return true;
                    }
                }
                return false;
            })
        .def("getBuiltinConfigs", [](PyBuiltinConfigRegistry & self) 
            {
                return BuiltinConfigIterator(self);
            });

    clsBuiltinConfigIterator
        .def("__len__", [](BuiltinConfigIterator & it) { return it.m_obj.getNumBuiltinConfigs(); })
        .def("__getitem__", [](BuiltinConfigIterator & it, int i) 
            { 
                return py::make_tuple(it.m_obj.getBuiltinConfigName(i),
                                      it.m_obj.getBuiltinConfigUIName(i),
                                      it.m_obj.isBuiltinConfigRecommended(i),
                                      StringUtils::Compare(
                                          std::string("ocio://") + std::string(it.m_obj.getBuiltinConfigName(i)), 
                                          std::string(ResolveConfigPath("ocio://default"))));
            })
        .def("__iter__", [](BuiltinConfigIterator & it) -> BuiltinConfigIterator & { return it; })
        .def("__next__", [](BuiltinConfigIterator & it)
            {
                int i = it.nextIndex((int)it.m_obj.getNumBuiltinConfigs());
                return py::make_tuple(it.m_obj.getBuiltinConfigName(i),
                                      it.m_obj.getBuiltinConfigUIName(i),
                                      it.m_obj.isBuiltinConfigRecommended(i),
                                      StringUtils::Compare(
                                          std::string("ocio://") + std::string(it.m_obj.getBuiltinConfigName(i)), 
                                          std::string(ResolveConfigPath("ocio://default"))));
            });

    clsBuiltinConfigNameIterator
        .def("__len__", [](BuiltinConfigNameIterator & it) { return it.m_obj.getNumBuiltinConfigs(); })
        .def("__getitem__", [](BuiltinConfigNameIterator & it, int i) 
            { 
                return it.m_obj.getBuiltinConfigName(i);
            })
        .def("__iter__", [](BuiltinConfigNameIterator & it) -> BuiltinConfigNameIterator & { return it; })
        .def("__next__", [](BuiltinConfigNameIterator & it)
            {
                int i = it.nextIndex((int)it.m_obj.getNumBuiltinConfigs());
                return it.m_obj.getBuiltinConfigName(i);
            });
}
}