// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyGradingRGBCurveTransform(py::module & m)
{
    GradingRGBCurveTransformRcPtr DEFAULT = GradingRGBCurveTransform::Create(GRADING_LOG);

    py::class_<GradingRGBCurveTransform,
               GradingRGBCurveTransformRcPtr /* holder */, 
               Transform /* base */>(m, "GradingRGBCurveTransform")
        .def(py::init([](GradingStyle style,
                         const ConstGradingRGBCurveRcPtr & values,
                         bool dynamic, 
                         TransformDirection dir) 
            {
                GradingRGBCurveTransformRcPtr p = GradingRGBCurveTransform::Create(style);
                p->setStyle(style);
                p->setValue(values);
                if (dynamic) { p->makeDynamic(); }
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "style"_a = DEFAULT->getStyle(),
             "values"_a,
             "dynamic"_a = DEFAULT->isDynamic(),
             "dir"_a = DEFAULT->getDirection())
        .def(py::init([](GradingStyle style, bool dynamic, TransformDirection dir)
            {
                GradingRGBCurveTransformRcPtr p = GradingRGBCurveTransform::Create(style);
                p->setStyle(style);
                if (dynamic)
                {
                    p->makeDynamic();
                }
                p->setDirection(dir);
                p->validate();
                return p;
            }),
            "style"_a = DEFAULT->getStyle(),
            "dynamic"_a = DEFAULT->isDynamic(),
            "dir"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (GradingRGBCurveTransform::*)()) &GradingRGBCurveTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (GradingRGBCurveTransform::*)() const)
             &GradingRGBCurveTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getStyle", &GradingRGBCurveTransform::getStyle)
        .def("setStyle", &GradingRGBCurveTransform::setStyle, "style"_a)
        .def("getValue", &GradingRGBCurveTransform::getValue)
        .def("setValue", &GradingRGBCurveTransform::setValue, "values"_a)
        .def("getBypassLinToLog", &GradingRGBCurveTransform::getBypassLinToLog)
        .def("setBypassLinToLog", &GradingRGBCurveTransform::setBypassLinToLog, "bypass"_a)
        .def("isDynamic", &GradingRGBCurveTransform::isDynamic)
        .def("makeDynamic", &GradingRGBCurveTransform::makeDynamic)
        .def("makeNonDynamic", &GradingRGBCurveTransform::makeNonDynamic);
}

} // namespace OCIO_NAMESPACE
