// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <iostream>
#include <memory>

#include "PyOpenColorIO.h"
#include "PyUtils.h"

#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/pytypes.h>

PYBIND11_MAKE_OPAQUE(std::vector<uint8_t>);

namespace OCIO_NAMESPACE
{
struct PyConfigIOProxy : ConfigIOProxy
{
    using ConfigIOProxy::ConfigIOProxy;

    std::vector<uint8_t> getLutData(const char * filepath) const override
    {
        PYBIND11_OVERRIDE_PURE(
            std::vector<uint8_t>,           // Return type.
            ConfigIOProxy,                  // Parent class.
            getLutData,                     // Name of function in C++ (must match Python name).
            filepath                        // Argument.
        );
    }

    std::string getConfigData() const override
    {
        PYBIND11_OVERRIDE_PURE(
            std::string,                    // Return type.
            ConfigIOProxy,                  // Parent class.
            getConfigData,                  // Name of function in C++ (must match Python name).
        );
    }

    std::string getFastLutFileHash(const char * filepath) const override
    {
        PYBIND11_OVERRIDE_PURE(
            std::string,                    // Return type.
            ConfigIOProxy,                  // Parent class.
            getFastLutFileHash,             // Name of function in C++ (must match Python name).
            filepath                        // Argument.
        );
    }
};

void bindPyConfigIOProxy(py::module & m)
{
    py::bind_vector<std::vector<uint8_t>>(m, "vector_of_uint8_t");
    py::implicitly_convertible<py::list, std::vector<uint8_t>>();
    py::implicitly_convertible<py::bytearray, std::vector<uint8_t>>();

    py::class_<ConfigIOProxy, std::shared_ptr<ConfigIOProxy>, PyConfigIOProxy>(m, "PyConfigIOProxy")
        .def(py::init())
        .def("getLutData", &ConfigIOProxy::getLutData)
        .def("getConfigData", &ConfigIOProxy::getConfigData)
        .def("getFastLutFileHash", &ConfigIOProxy::getFastLutFileHash);
}

}