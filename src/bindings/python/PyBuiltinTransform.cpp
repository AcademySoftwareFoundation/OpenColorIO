// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyBuiltinTransform(py::module & m)
{
    BuiltinTransformRcPtr DEFAULT = BuiltinTransform::Create();

    py::class_<BuiltinTransform, 
               BuiltinTransformRcPtr /* holder */, 
               Transform /* base */>(m, "BuiltinTransform")
        .def(py::init(&BuiltinTransform::Create))
        .def(py::init([](const std::string & style, TransformDirection dir) 
            {
                BuiltinTransformRcPtr p = BuiltinTransform::Create();
                if (!style.empty()) { p->setStyle(style.c_str()); }
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             "style"_a = DEFAULT->getStyle(),
             "dir"_a = DEFAULT->getDirection())

        .def("setStyle",       &BuiltinTransform::setStyle, "style"_a.none(false))
        .def("getStyle",       &BuiltinTransform::getStyle)
        .def("getDescription", &BuiltinTransform::getDescription);
}

} // namespace OCIO_NAMESPACE
