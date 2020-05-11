// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyDynamicProperty(py::module & m)
{
    py::class_<DynamicProperty, DynamicPropertyRcPtr /* holder */>(m, "DynamicProperty")
        .def("getType", &DynamicProperty::getType)
        .def("getValueType", &DynamicProperty::getValueType)
        .def("getDoubleValue", &DynamicProperty::getDoubleValue)
        .def("setValue", &DynamicProperty::setValue, "value"_a)
        .def("isDynamic", &DynamicProperty::isDynamic);
}

} // namespace OCIO_NAMESPACE
