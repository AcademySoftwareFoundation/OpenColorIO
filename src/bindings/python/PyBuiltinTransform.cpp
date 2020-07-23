// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyBuiltinTransform(py::module & m)
{
    BuiltinTransformRcPtr DEFAULT = BuiltinTransform::Create();

    auto cls = py::class_<BuiltinTransform, 
                          BuiltinTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "BuiltinTransform", DS(BuiltinTransform))
        .def(py::init(&BuiltinTransform::Create), DS(BuiltinTransform, Create))
        .def(py::init([](const std::string & style, TransformDirection dir) 
            {
                BuiltinTransformRcPtr p = BuiltinTransform::Create();
                if (!style.empty()) { p->setStyle(style.c_str()); }
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             DS(BuiltinTransform, BuiltinTransform),
             "style"_a = DEFAULT->getStyle(),
             "direction"_a = DEFAULT->getDirection())

        .def("setStyle",       &BuiltinTransform::setStyle, DS(BuiltinTransform, setStyle), "style"_a.none(false))
        .def("getStyle",       &BuiltinTransform::getStyle, DS(BuiltinTransform, getStyle))
        .def("getDescription", &BuiltinTransform::getDescription, DS(BuiltinTransform, getDescription));

    defStr(cls);
}

} // namespace OCIO_NAMESPACE
