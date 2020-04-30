// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

void bindPyBuiltinTransformRegistry(py::module & m)
{
    py::class_<BuiltinTransformRegistry, BuiltinTransformRegistryRcPtr>(m, "BuiltinTransformRegistry")
        .def(py::init([]() { return BuiltinTransformRegistry::Get(); }))

        .def("getNumBuiltins", &BuiltinTransformRegistry::getNumBuiltins)
        .def("getBuiltinStyle", &BuiltinTransformRegistry::getBuiltinStyle)
        .def("getBuiltinDescription", &BuiltinTransformRegistry::getBuiltinDescription);
}

} // namespace OCIO_NAMESPACE
