// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyDynamicProperty.h"
#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyDynamicProperty(py::module & m)
{
    auto clsDynamicProperty = 
        py::class_<PyDynamicProperty>(
            m.attr("DynamicProperty"))

        .def("getType", &PyDynamicProperty::getType, 
             DOC(DynamicProperty, getType))
        .def("getDouble", &PyDynamicProperty::getDouble, 
             DOC(DynamicPropertyValue, AsDouble))
        .def("setDouble", &PyDynamicProperty::setDouble, "val"_a, 
             DOC(DynamicPropertyValue, AsDouble))
        .def("getGradingPrimary", &PyDynamicProperty::getGradingPrimary, 
             DOC(DynamicPropertyValue, AsGradingPrimary))
        .def("setGradingPrimary", &PyDynamicProperty::setGradingPrimary, "val"_a, 
             DOC(DynamicPropertyValue, AsGradingPrimary))
        .def("getGradingRGBCurve", &PyDynamicProperty::getGradingRGBCurve, 
             DOC(DynamicPropertyValue, AsGradingRGBCurve))
        .def("setGradingRGBCurve", &PyDynamicProperty::setGradingRGBCurve, "val"_a, 
             DOC(DynamicPropertyValue, AsGradingRGBCurve))
        .def("getGradingTone", &PyDynamicProperty::getGradingTone, 
             DOC(DynamicPropertyValue, AsGradingTone))
        .def("setGradingTone", &PyDynamicProperty::setGradingTone, "val"_a, 
             DOC(DynamicPropertyValue, AsGradingTone));
}

} // namespace OCIO_NAMESPACE
