// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyDynamicProperty(py::module & m)
{
    py::class_<DynamicProperty, DynamicPropertyRcPtr /* holder */>(m, "DynamicProperty", DS(DynamicProperty))
        .def("getType", &DynamicProperty::getType, DS(DynamicProperty, getType))
        .def("getValueType", &DynamicProperty::getValueType, DS(DynamicProperty, getValueType))
        .def("getDoubleValue", &DynamicProperty::getDoubleValue, DS(DynamicProperty, getDoubleValue))
        .def("setValue", &DynamicProperty::setValue, DS(DynamicProperty, setValue), "value"_a)
        .def("isDynamic", &DynamicProperty::isDynamic, DS(DynamicProperty, isDynamic));
}

} // namespace OCIO_NAMESPACE
