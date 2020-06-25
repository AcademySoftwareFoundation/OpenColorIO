// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyBuiltinTransform(py::module & m)
{
    py::class_<BuiltinTransform, 
               BuiltinTransformRcPtr /* holder */, 
               Transform /* base */>(m, "BuiltinTransform")
        .def(py::init(&BuiltinTransform::Create))

        .def("setStyle",       &BuiltinTransform::setStyle, "style"_a)
        .def("getStyle",       &BuiltinTransform::getStyle)
        .def("getDescription", &BuiltinTransform::getDescription);
}

} // namespace OCIO_NAMESPACE
