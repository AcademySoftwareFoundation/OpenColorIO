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

    auto cls = py::class_<FixedFunctionTransform, 
                          FixedFunctionTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "FixedFunctionTransform", DS(FixedFunctionTransform))
        .def(py::init(&FixedFunctionTransform::Create), DS(FixedFunctionTransform, Create))
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
             DS(FixedFunctionTransform, FixedFunctionTransform),
             "style"_a = DEFAULT->getStyle(), 
             "params"_a = getParamsStdVec(DEFAULT),
             "direction"_a = DEFAULT->getDirection())

        .def("getFormatMetadata", 
             (FormatMetadata & (FixedFunctionTransform::*)()) 
             &FixedFunctionTransform::getFormatMetadata,
             DS(FixedFunctionTransform, getFormatMetadata),
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (FixedFunctionTransform::*)() const) 
             &FixedFunctionTransform::getFormatMetadata,
             DS(FixedFunctionTransform, getFormatMetadata),
             py::return_value_policy::reference_internal)
        .def("equals", &FixedFunctionTransform::equals, DS(FixedFunctionTransform, equals), "other"_a)
        .def("getStyle", &FixedFunctionTransform::getStyle, DS(FixedFunctionTransform, getStyle))
        .def("setStyle", &FixedFunctionTransform::setStyle, DS(FixedFunctionTransform, setStyle), "style"_a)
        .def("getParams", [](FixedFunctionTransformRcPtr self)
            {
                return getParamsStdVec(self);
            },
             DS(FixedFunctionTransform, getParams))
        .def("setParams", [](FixedFunctionTransformRcPtr self, const std::vector<double> & params)
            { 
                self->setParams(params.data(), params.size());
            }, 
             DS(FixedFunctionTransform, setParams),
             "params"_a);

    defStr(cls);
}

} // namespace OCIO_NAMESPACE
