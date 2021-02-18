// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyExponentWithLinearTransform(py::module & m)
{
    ExponentWithLinearTransformRcPtr DEFAULT = ExponentWithLinearTransform::Create();

    std::array<double, 4> DEFAULT_GAMMA;
    DEFAULT->getGamma(*reinterpret_cast<double(*)[4]>(DEFAULT_GAMMA.data()));

    std::array<double, 4> DEFAULT_OFFSET;
    DEFAULT->getOffset(*reinterpret_cast<double(*)[4]>(DEFAULT_OFFSET.data()));

    auto clsExponentWithLinearTransform = 
        py::class_<ExponentWithLinearTransform, ExponentWithLinearTransformRcPtr, Transform>(
            m.attr("ExponentWithLinearTransform"))

        .def(py::init(&ExponentWithLinearTransform::Create), 
             DOC(ExponentWithLinearTransform, Create))
        .def(py::init([](const std::array<double, 4> & gamma,
                         const std::array<double, 4> & offset,
                         NegativeStyle negativeStyle, 
                         TransformDirection dir) 
            {
                ExponentWithLinearTransformRcPtr p = ExponentWithLinearTransform::Create();
                p->setGamma(*reinterpret_cast<const double(*)[4]>(gamma.data()));
                p->setOffset(*reinterpret_cast<const double(*)[4]>(offset.data()));
                p->setNegativeStyle(negativeStyle);
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "gamma"_a = DEFAULT_GAMMA,
             "offset"_a = DEFAULT_OFFSET,
             "negativeStyle"_a = DEFAULT->getNegativeStyle(),
             "direction"_a = DEFAULT->getDirection(), 
             DOC(ExponentWithLinearTransform, Create))

        .def("getFormatMetadata", 
             (FormatMetadata & (ExponentWithLinearTransform::*)()) 
             &ExponentWithLinearTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(ExponentWithLinearTransform, getFormatMetadata))
        .def("equals", &ExponentWithLinearTransform::equals, "other"_a, 
             DOC(ExponentWithLinearTransform, equals))
        .def("getGamma", [](ExponentWithLinearTransformRcPtr self)
            {
                std::array<double, 4> values;
                self->getGamma(*reinterpret_cast<double(*)[4]>(values.data()));
                return values;
            }, 
             DOC(ExponentWithLinearTransform, getGamma))
        .def("setGamma", [](ExponentWithLinearTransformRcPtr self, 
                            const std::array<double, 4> & values)
            { 
                self->setGamma(*reinterpret_cast<const double(*)[4]>(values.data()));
            }, 
             "values"_a, 
             DOC(ExponentWithLinearTransform, setGamma))
        .def("getOffset", [](ExponentWithLinearTransformRcPtr self)
            {
                std::array<double, 4> values;
                self->getOffset(*reinterpret_cast<double(*)[4]>(values.data()));
                return values;
            }, 
             DOC(ExponentWithLinearTransform, getOffset))
        .def("setOffset", [](ExponentWithLinearTransformRcPtr self, 
                             const std::array<double, 4> & values)
            { 
                self->setOffset(*reinterpret_cast<const double(*)[4]>(values.data()));
            }, 
             "values"_a, 
             DOC(ExponentWithLinearTransform, setOffset))
        .def("getNegativeStyle", &ExponentWithLinearTransform::getNegativeStyle, 
             DOC(ExponentWithLinearTransform, getNegativeStyle))
        .def("setNegativeStyle", &ExponentWithLinearTransform::setNegativeStyle, "style"_a, 
             DOC(ExponentWithLinearTransform, setNegativeStyle));

    defRepr(clsExponentWithLinearTransform);
}

} // namespace OCIO_NAMESPACE
