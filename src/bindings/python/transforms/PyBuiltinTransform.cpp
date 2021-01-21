// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyBuiltinTransform(py::module & m)
{
    BuiltinTransformRcPtr DEFAULT = BuiltinTransform::Create();

    auto clsBuiltinTransform = 
        py::class_<BuiltinTransform, BuiltinTransformRcPtr, Transform>(
            m.attr("BuiltinTransform"))

        .def(py::init(&BuiltinTransform::Create), 
             DOC(BuiltinTransform, Create))
        .def(py::init([](const std::string & style, TransformDirection dir) 
            {
                BuiltinTransformRcPtr p = BuiltinTransform::Create();
                if (!style.empty()) { p->setStyle(style.c_str()); }
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             "style"_a = DEFAULT->getStyle(),
             "direction"_a = DEFAULT->getDirection(), 
             DOC(BuiltinTransform, Create))

        .def("setStyle", &BuiltinTransform::setStyle, "style"_a.none(false), 
             DOC(BuiltinTransform, setStyle))
        .def("getStyle", &BuiltinTransform::getStyle, 
             DOC(BuiltinTransform, getStyle))
        .def("getDescription", &BuiltinTransform::getDescription, 
             DOC(BuiltinTransform, getDescription));

    defRepr(clsBuiltinTransform);
}

} // namespace OCIO_NAMESPACE
