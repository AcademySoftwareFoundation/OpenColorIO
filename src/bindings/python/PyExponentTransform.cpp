// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyExponentTransform(py::module & m)
{
    ExponentTransformRcPtr DEFAULT = ExponentTransform::Create();

    std::array<double, 4> DEFAULT_VALUE;
    DEFAULT->getValue(*reinterpret_cast<double(*)[4]>(DEFAULT_VALUE.data()));

    auto cls = py::class_<ExponentTransform, 
                          ExponentTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "ExponentTransform", DS(ExponentTransform))
        .def(py::init(&ExponentTransform::Create), DS(ExponentTransform, Create))
        .def(py::init([](const std::array<double, 4> & vec4,
                         NegativeStyle negativeStyle, 
                         TransformDirection dir)
            {
                ExponentTransformRcPtr p = ExponentTransform::Create();
                p->setValue(*reinterpret_cast<const double(*)[4]>(vec4.data()));
                p->setNegativeStyle(negativeStyle);
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             DS(ExponentTransform, ExponentTransform),
             "value"_a = DEFAULT_VALUE,
             "negativeStyle"_a = DEFAULT->getNegativeStyle(),
             "direction"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (ExponentTransform::*)()) &ExponentTransform::getFormatMetadata,
             DS(ExponentTransform, getFormatMetadata),
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (ExponentTransform::*)() const) 
             &ExponentTransform::getFormatMetadata,
             DS(ExponentTransform, getFormatMetadata),
             py::return_value_policy::reference_internal)
        .def("equals", &ExponentTransform::equals, DS(ExponentTransform, equals), "other"_a)
        .def("getValue", [](ExponentTransformRcPtr self)
            {
                std::array<double, 4> vec4;
                self->getValue(*reinterpret_cast<double(*)[4]>(vec4.data()));
                return vec4;
            },
             DS(ExponentTransform, getValue))
        .def("setValue", [](ExponentTransformRcPtr self, const std::array<double, 4> & vec4)
            { 
                self->setValue(*reinterpret_cast<const double(*)[4]>(vec4.data()));
            }, 
             DS(ExponentTransform, setValue),
             "value"_a)
        .def("getNegativeStyle", &ExponentTransform::getNegativeStyle, DS(ExponentTransform, getNegativeStyle))
        .def("setNegativeStyle", &ExponentTransform::setNegativeStyle, DS(ExponentTransform, setNegativeStyle), "style"_a);

    defStr(cls);
}

} // namespace OCIO_NAMESPACE
