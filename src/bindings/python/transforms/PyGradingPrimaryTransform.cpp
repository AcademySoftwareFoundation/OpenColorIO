// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyGradingPrimaryTransform(py::module & m)
{
    GradingPrimaryTransformRcPtr DEFAULT = GradingPrimaryTransform::Create(GRADING_LOG);

    auto clsGradingPrimaryTransform = 
        py::class_<GradingPrimaryTransform, GradingPrimaryTransformRcPtr, Transform>(
            m.attr("GradingPrimaryTransform"))

        .def(py::init([](const GradingPrimary & values,
                         GradingStyle style,
                         bool dynamic, 
                         TransformDirection dir) 
            {
                GradingPrimaryTransformRcPtr p = GradingPrimaryTransform::Create(style);
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
             DOC(GradingPrimaryTransform, Create))
        .def(py::init([](GradingStyle style, bool dynamic, TransformDirection dir)
            {
                GradingPrimaryTransformRcPtr p = GradingPrimaryTransform::Create(style);
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
             DOC(GradingPrimaryTransform, Create))

        .def("getFormatMetadata", 
             (FormatMetadata & (GradingPrimaryTransform::*)()) 
             &GradingPrimaryTransform::getFormatMetadata,
             py::return_value_policy::reference_internal, 
             DOC(GradingPrimaryTransform, getFormatMetadata))
        .def("getStyle", &GradingPrimaryTransform::getStyle, 
             DOC(GradingPrimaryTransform, getStyle))
        .def("setStyle", &GradingPrimaryTransform::setStyle, "style"_a, 
             DOC(GradingPrimaryTransform, setStyle))
        .def("getValue", &GradingPrimaryTransform::getValue, 
             DOC(GradingPrimaryTransform, getValue))
        .def("setValue", &GradingPrimaryTransform::setValue, "values"_a, 
             DOC(GradingPrimaryTransform, setValue))
        .def("isDynamic", &GradingPrimaryTransform::isDynamic, 
             DOC(GradingPrimaryTransform, isDynamic))
        .def("makeDynamic", &GradingPrimaryTransform::makeDynamic, 
             DOC(GradingPrimaryTransform, makeDynamic))
        .def("makeNonDynamic", &GradingPrimaryTransform::makeNonDynamic, 
             DOC(GradingPrimaryTransform, makeNonDynamic));

    defRepr(clsGradingPrimaryTransform);
}

} // namespace OCIO_NAMESPACE
