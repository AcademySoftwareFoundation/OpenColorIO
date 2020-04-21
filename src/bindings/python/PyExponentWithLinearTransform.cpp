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

    py::class_<ExponentWithLinearTransform, 
               ExponentWithLinearTransformRcPtr /* holder */, 
               Transform /* base */>(m, "ExponentWithLinearTransform")
        .def(py::init(&ExponentWithLinearTransform::Create))
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
             "dir"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (ExponentWithLinearTransform::*)()) 
             &ExponentWithLinearTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (ExponentWithLinearTransform::*)() const) 
             &ExponentWithLinearTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("equals", &ExponentWithLinearTransform::equals, "other"_a)
        .def("getGamma", [](ExponentWithLinearTransformRcPtr self)
            {
                std::array<double, 4> values;
                self->getGamma(*reinterpret_cast<double(*)[4]>(values.data()));
                return values;
            })
        .def("setGamma", [](ExponentWithLinearTransformRcPtr self, 
                            const std::array<double, 4> & values)
            { 
                self->setGamma(*reinterpret_cast<const double(*)[4]>(values.data()));
            }, 
             "values"_a)
        .def("getOffset", [](ExponentWithLinearTransformRcPtr self)
            {
                std::array<double, 4> values;
                self->getOffset(*reinterpret_cast<double(*)[4]>(values.data()));
                return values;
            })
        .def("setOffset", [](ExponentWithLinearTransformRcPtr self, 
                             const std::array<double, 4> & values)
            { 
                self->setOffset(*reinterpret_cast<const double(*)[4]>(values.data()));
            }, 
             "values"_a)
        .def("getNegativeStyle", &ExponentWithLinearTransform::getNegativeStyle)
        .def("setNegativeStyle", &ExponentWithLinearTransform::setNegativeStyle, "style"_a);
}

} // namespace OCIO_NAMESPACE
