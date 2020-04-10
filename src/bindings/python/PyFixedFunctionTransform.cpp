// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

namespace {
    std::vector<double> getParamsStdVec(FixedFunctionTransformRcPtr & p) {
        std::vector<double> params;
        params.resize(p->getNumParams());
        p->getParams(params.data());
        return params;
    }
}

void bindPyFixedFunctionTransform(py::module & m)
{
    FixedFunctionTransformRcPtr DEFAULT = FixedFunctionTransform::Create();

    py::class_<FixedFunctionTransform, 
               FixedFunctionTransformRcPtr /* holder */, 
               Transform /* base */>(m, "FixedFunctionTransform")
        .def(py::init(&FixedFunctionTransform::Create))
        .def(py::init([](FixedFunctionStyle style, 
                         const std::vector<double> & params,
                         TransformDirection dir) 
            {
                FixedFunctionTransformRcPtr p = FixedFunctionTransform::Create();
                p->setStyle(style);
                p->setParams(params.data(), params.size());
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             "style"_a = DEFAULT->getStyle(), 
             "params"_a = getParamsStdVec(DEFAULT),
             "dir"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (FixedFunctionTransform::*)()) 
             &FixedFunctionTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (FixedFunctionTransform::*)() const) 
             &FixedFunctionTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("equals", &FixedFunctionTransform::equals, "other"_a)
        .def("getStyle", &FixedFunctionTransform::getStyle)
        .def("setStyle", &FixedFunctionTransform::setStyle, "style"_a)
        .def("getParams", [](FixedFunctionTransformRcPtr self)
            {
                return getParamsStdVec(self);
            })
        .def("setParams", [](FixedFunctionTransformRcPtr self, const std::vector<double> & params)
            { 
                self->setParams(params.data(), params.size());
            }, 
             "params"_a);
}

} // namespace OCIO_NAMESPACE
