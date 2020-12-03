// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyGradingPrimaryTransform(py::module & m)
{
    GradingPrimaryTransformRcPtr DEFAULT = GradingPrimaryTransform::Create(GRADING_LOG);

    auto cls = py::class_<GradingPrimaryTransform,
                          GradingPrimaryTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "GradingPrimaryTransform")
        .def(py::init([](GradingStyle style,
                         const GradingPrimary & values,
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
             "style"_a = DEFAULT->getStyle(),
             "values"_a,
             "dynamic"_a = DEFAULT->isDynamic(),
             "dir"_a = DEFAULT->getDirection())
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
            "dir"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (GradingPrimaryTransform::*)()) &GradingPrimaryTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (GradingPrimaryTransform::*)() const)
             &GradingPrimaryTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getStyle", &GradingPrimaryTransform::getStyle)
        .def("setStyle", &GradingPrimaryTransform::setStyle, "style"_a)
        .def("getValue", &GradingPrimaryTransform::getValue)
        .def("setValue", &GradingPrimaryTransform::setValue, "values"_a)
        .def("isDynamic", &GradingPrimaryTransform::isDynamic)
        .def("makeDynamic", &GradingPrimaryTransform::makeDynamic)
        .def("makeNonDynamic", &GradingPrimaryTransform::makeNonDynamic);

    defStr(cls);

}

} // namespace OCIO_NAMESPACE
