// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyGradingToneTransform(py::module & m)
{
    GradingToneTransformRcPtr DEFAULT = GradingToneTransform::Create(GRADING_LOG);

    auto cls = py::class_<GradingToneTransform,
                          GradingToneTransformRcPtr /* holder */,
                          Transform /* base */>(m, "GradingToneTransform")
        .def(py::init([](GradingStyle style,
                         const GradingTone & values,
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
             "style"_a = DEFAULT->getStyle(),
             "values"_a,
             "dynamic"_a = DEFAULT->isDynamic(),
             "dir"_a = DEFAULT->getDirection())
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
            "dir"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (GradingToneTransform::*)()) &GradingToneTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (GradingToneTransform::*)() const)
             &GradingToneTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getStyle", &GradingToneTransform::getStyle)
        .def("setStyle", &GradingToneTransform::setStyle, "style"_a)
        .def("getValue", &GradingToneTransform::getValue)
        .def("setValue", &GradingToneTransform::setValue, "values"_a)
        .def("isDynamic", &GradingToneTransform::isDynamic)
        .def("makeDynamic", &GradingToneTransform::makeDynamic)
        .def("makeNonDynamic", &GradingToneTransform::makeNonDynamic);

    defStr(cls);

}

} // namespace OCIO_NAMESPACE
