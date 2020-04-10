// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyRangeTransform(py::module & m)
{
    RangeTransformRcPtr DEFAULT = RangeTransform::Create();

    py::class_<RangeTransform, 
               RangeTransformRcPtr /* holder */, 
               Transform /* base */>(m, "RangeTransform")
        .def(py::init(&RangeTransform::Create))
        .def(py::init([](double minInValue,
                         double maxInValue,
                         double minOutValue,
                         double maxOutValue, 
                         TransformDirection dir) 
            {
                RangeTransformRcPtr p = RangeTransform::Create();
                p->setMinInValue(minInValue);
                p->setMaxInValue(maxInValue);
                p->setMinOutValue(minOutValue);
                p->setMaxOutValue(maxOutValue);
                p->setDirection(dir);
                p->validate();
                return p;
            }),
             "minInValue"_a = DEFAULT->getMinInValue(),
             "maxInValue"_a = DEFAULT->getMaxInValue(),
             "minOutValue"_a = DEFAULT->getMinOutValue(),
             "maxOutValue"_a = DEFAULT->getMaxOutValue(),
             "dir"_a = DEFAULT->getDirection())

        .def("getStyle", &RangeTransform::getStyle)
        .def("setStyle", &RangeTransform::setStyle, "style"_a)
        .def("getFormatMetadata", 
             (FormatMetadata & (RangeTransform::*)()) &RangeTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (RangeTransform::*)() const) 
             &RangeTransform::getFormatMetadata,
             py::return_value_policy::reference_internal)
        .def("equals", &RangeTransform::equals, "other"_a)
        .def("getFileInputBitDepth", &RangeTransform::getFileInputBitDepth)
        .def("setFileInputBitDepth", &RangeTransform::setFileInputBitDepth, "bitDepth"_a)
        .def("getFileOutputBitDepth", &RangeTransform::getFileOutputBitDepth)
        .def("setFileOutputBitDepth", &RangeTransform::setFileOutputBitDepth, "bitDepth"_a)
        .def("getMinInValue", &RangeTransform::getMinInValue)
        .def("setMinInValue", &RangeTransform::setMinInValue, "val"_a)
        .def("hasMinInValue", &RangeTransform::hasMinInValue)
        .def("unsetMinInValue", &RangeTransform::unsetMinInValue)
        .def("getMaxInValue", &RangeTransform::getMaxInValue)
        .def("setMaxInValue", &RangeTransform::setMaxInValue, "val"_a)
        .def("hasMaxInValue", &RangeTransform::hasMaxInValue)
        .def("unsetMaxOutValue", &RangeTransform::unsetMaxOutValue)
        .def("getMinOutValue", &RangeTransform::getMinOutValue)
        .def("setMinOutValue", &RangeTransform::setMinOutValue, "val"_a)
        .def("hasMinOutValue", &RangeTransform::hasMinOutValue)
        .def("unsetMinOutValue", &RangeTransform::unsetMinOutValue)
        .def("getMaxOutValue", &RangeTransform::getMaxOutValue)
        .def("setMaxOutValue", &RangeTransform::setMaxOutValue, "val"_a)
        .def("hasMaxOutValue", &RangeTransform::hasMaxOutValue)
        .def("unsetMaxOutValue", &RangeTransform::unsetMaxOutValue);
}

} // namespace OCIO_NAMESPACE
