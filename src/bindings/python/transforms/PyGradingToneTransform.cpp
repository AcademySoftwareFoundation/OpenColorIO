// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyGradingToneTransform(py::module & m)
{
    GradingToneTransformRcPtr DEFAULT = GradingToneTransform::Create(GRADING_LOG);

    auto clsGradingToneTransform = 
        py::class_<GradingToneTransform, GradingToneTransformRcPtr, Transform>(
            m.attr("GradingToneTransform"))

        .def(py::init([](const GradingTone & values,
                         GradingStyle style,
                         bool dynamic, 
                         TransformDirection dir) 
            {
                GradingToneTransformRcPtr p = GradingToneTransform::Create(style);
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
             DOC(GradingToneTransform, Create))
        .def(py::init([](GradingStyle style, bool dynamic, TransformDirection dir)
            {
                GradingToneTransformRcPtr p = GradingToneTransform::Create(style);
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
             DOC(GradingToneTransform, Create))

        .def("getFormatMetadata", 
             (FormatMetadata & (GradingToneTransform::*)()) 
             &GradingToneTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(GradingToneTransform, getFormatMetadata))
        .def("getStyle", &GradingToneTransform::getStyle, 
             DOC(GradingToneTransform, getStyle))
        .def("setStyle", &GradingToneTransform::setStyle, "style"_a, 
             DOC(GradingToneTransform, setStyle))
        .def("getValue", &GradingToneTransform::getValue, 
             DOC(GradingToneTransform, getValue))
        .def("setValue", &GradingToneTransform::setValue, "values"_a, 
             DOC(GradingToneTransform, setValue))
        .def("isDynamic", &GradingToneTransform::isDynamic, 
             DOC(GradingToneTransform, isDynamic))
        .def("makeDynamic", &GradingToneTransform::makeDynamic, 
             DOC(GradingToneTransform, makeDynamic))
        .def("makeNonDynamic", &GradingToneTransform::makeNonDynamic, 
             DOC(GradingToneTransform, makeNonDynamic));

    defRepr(clsGradingToneTransform);
}

} // namespace OCIO_NAMESPACE
