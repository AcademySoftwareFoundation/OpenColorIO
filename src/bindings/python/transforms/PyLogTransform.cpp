// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyLogTransform(py::module & m)
{
    LogTransformRcPtr DEFAULT = LogTransform::Create();

    auto clsLogTransform = 
        py::class_<LogTransform, LogTransformRcPtr, Transform>(
            m.attr("LogTransform"))

        .def(py::init(&LogTransform::Create), 
             DOC(LogTransform, Create))

        .def(py::init([](double base, TransformDirection dir) 
            {
                LogTransformRcPtr p = LogTransform::Create();
                p->setBase(base);
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "base"_a = DEFAULT->getBase(),
             "direction"_a = DEFAULT->getDirection(), 
             DOC(LogTransform, Create))

        .def("getFormatMetadata", 
             (FormatMetadata & (LogTransform::*)()) &LogTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(LogTransform, getFormatMetadata))
        .def("equals", &LogTransform::equals, "other"_a, 
             DOC(LogTransform, equals))
        .def("getBase", &LogTransform::getBase, 
             DOC(LogTransform, getBase))
        .def("setBase", &LogTransform::setBase, "base"_a, 
             DOC(LogTransform, setBase));

    defRepr(clsLogTransform);
}

} // namespace OCIO_NAMESPACE
