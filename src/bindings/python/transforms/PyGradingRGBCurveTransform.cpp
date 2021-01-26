// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyGradingRGBCurveTransform(py::module & m)
{
    GradingRGBCurveTransformRcPtr DEFAULT = GradingRGBCurveTransform::Create(GRADING_LOG);

    auto clsGradingRGBCurveTransform = 
        py::class_<GradingRGBCurveTransform, GradingRGBCurveTransformRcPtr, Transform>(
            m.attr("GradingRGBCurveTransform"))

        .def(py::init([](const ConstGradingRGBCurveRcPtr & values,
                         GradingStyle style,
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
             "values"_a,
             "style"_a = DEFAULT->getStyle(),
             "dynamic"_a = DEFAULT->isDynamic(),
             "dir"_a = DEFAULT->getDirection(),
             DOC(GradingRGBCurveTransform, Create))
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
             "dir"_a = DEFAULT->getDirection(),
             DOC(GradingRGBCurveTransform, Create))

        .def("getFormatMetadata", 
             (FormatMetadata & (GradingRGBCurveTransform::*)()) 
             &GradingRGBCurveTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(GradingRGBCurveTransform, getFormatMetadata))
        .def("getStyle", &GradingRGBCurveTransform::getStyle, 
             DOC(GradingRGBCurveTransform, getStyle))
        .def("setStyle", &GradingRGBCurveTransform::setStyle, "style"_a, 
             DOC(GradingRGBCurveTransform, setStyle))
        .def("getValue", &GradingRGBCurveTransform::getValue, 
             DOC(GradingRGBCurveTransform, getValue))
        .def("setValue", &GradingRGBCurveTransform::setValue, "values"_a, 
             DOC(GradingRGBCurveTransform, setValue))
        .def("getSlope", &GradingRGBCurveTransform::getSlope, "channel"_a, "index"_a,
             DOC(GradingRGBCurveTransform, getSlope))
        .def("setSlope", &GradingRGBCurveTransform::setSlope, "channel"_a, "index"_a, "slope"_a, 
             DOC(GradingRGBCurveTransform, setSlope))
        .def("slopesAreDefault", &GradingRGBCurveTransform::slopesAreDefault, "channel"_a,
             DOC(GradingRGBCurveTransform, slopesAreDefault))
        .def("getBypassLinToLog", &GradingRGBCurveTransform::getBypassLinToLog, 
             DOC(GradingRGBCurveTransform, getBypassLinToLog))
        .def("setBypassLinToLog", &GradingRGBCurveTransform::setBypassLinToLog, "bypass"_a, 
             DOC(GradingRGBCurveTransform, setBypassLinToLog))
        .def("isDynamic", &GradingRGBCurveTransform::isDynamic, 
             DOC(GradingRGBCurveTransform, isDynamic))
        .def("makeDynamic", &GradingRGBCurveTransform::makeDynamic, 
             DOC(GradingRGBCurveTransform, makeDynamic))
        .def("makeNonDynamic", &GradingRGBCurveTransform::makeNonDynamic, 
             DOC(GradingRGBCurveTransform, makeNonDynamic));

    defRepr(clsGradingRGBCurveTransform);
}

} // namespace OCIO_NAMESPACE
