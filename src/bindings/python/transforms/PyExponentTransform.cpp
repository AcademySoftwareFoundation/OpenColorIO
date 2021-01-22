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

    auto clsExponentTransform = 
        py::class_<ExponentTransform, ExponentTransformRcPtr, Transform>(
             m.attr("ExponentTransform"))

        .def(py::init(&ExponentTransform::Create), 
             DOC(ExponentTransform, Create))
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
             "value"_a = DEFAULT_VALUE,
             "negativeStyle"_a = DEFAULT->getNegativeStyle(),
             "direction"_a = DEFAULT->getDirection(), 
             DOC(ExponentTransform, Create))

        .def("getFormatMetadata", 
             (FormatMetadata & (ExponentTransform::*)()) &ExponentTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(ExponentTransform, getFormatMetadata))
        .def("equals", &ExponentTransform::equals, "other"_a, 
             DOC(ExponentTransform, equals))
        .def("getValue", [](ExponentTransformRcPtr self)
            {
                std::array<double, 4> vec4;
                self->getValue(*reinterpret_cast<double(*)[4]>(vec4.data()));
                return vec4;
            }, 
             DOC(ExponentTransform, getValue))
        .def("setValue", [](ExponentTransformRcPtr self, const std::array<double, 4> & vec4)
            { 
                self->setValue(*reinterpret_cast<const double(*)[4]>(vec4.data()));
            }, 
             "value"_a, 
             DOC(ExponentTransform, setValue))
        .def("getNegativeStyle", &ExponentTransform::getNegativeStyle, 
             DOC(ExponentTransform, getNegativeStyle))
        .def("setNegativeStyle", &ExponentTransform::setNegativeStyle, "style"_a, 
             DOC(ExponentTransform, setNegativeStyle));

    defRepr(clsExponentTransform);
}

} // namespace OCIO_NAMESPACE
