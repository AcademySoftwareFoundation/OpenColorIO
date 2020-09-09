// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyDynamicProperty.h"
#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyDynamicProperty(py::module & m)
{
    py::class_<PyDynamicProperty>(m, "DynamicProperty")
        .def("getType", &PyDynamicProperty::getType)
        .def("getDouble", &PyDynamicProperty::getDouble)
        .def("setDouble", &PyDynamicProperty::setDouble, "val"_a)
        .def("getGradingPrimary", &PyDynamicProperty::getGradingPrimary)
        .def("setGradingPrimary", &PyDynamicProperty::setGradingPrimary, "val"_a)
        .def("getGradingRGBCurve", &PyDynamicProperty::getGradingRGBCurve)
        .def("setGradingRGBCurve", &PyDynamicProperty::setGradingRGBCurve, "val"_a)
        .def("getGradingTone", &PyDynamicProperty::getGradingTone)
        .def("setGradingTone", &PyDynamicProperty::setGradingTone, "val"_a);
}

} // namespace OCIO_NAMESPACE
