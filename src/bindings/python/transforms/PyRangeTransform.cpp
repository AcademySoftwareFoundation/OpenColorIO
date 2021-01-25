// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyRangeTransform(py::module & m)
{
    RangeTransformRcPtr DEFAULT = RangeTransform::Create();

    auto clsRangeTransform = 
        py::class_<RangeTransform, RangeTransformRcPtr, Transform>(
            m.attr("RangeTransform"))

        .def(py::init(&RangeTransform::Create),
             DOC(RangeTransform, Create))
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
             "direction"_a = DEFAULT->getDirection(),
             DOC(RangeTransform, Create))

        .def("getStyle", &RangeTransform::getStyle,
             DOC(RangeTransform, getStyle))
        .def("setStyle", &RangeTransform::setStyle, "style"_a,
             DOC(RangeTransform, setStyle))
        .def("getFormatMetadata", 
             (FormatMetadata & (RangeTransform::*)()) &RangeTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(RangeTransform, getFormatMetadata))
        .def("equals", &RangeTransform::equals, "other"_a,
             DOC(RangeTransform, equals))
        .def("getFileInputBitDepth", &RangeTransform::getFileInputBitDepth,
             DOC(RangeTransform, getFileInputBitDepth))
        .def("setFileInputBitDepth", &RangeTransform::setFileInputBitDepth, "bitDepth"_a,
             DOC(RangeTransform, setFileInputBitDepth))
        .def("getFileOutputBitDepth", &RangeTransform::getFileOutputBitDepth,
             DOC(RangeTransform, getFileOutputBitDepth))
        .def("setFileOutputBitDepth", &RangeTransform::setFileOutputBitDepth, "bitDepth"_a,
             DOC(RangeTransform, setFileOutputBitDepth))
        .def("getMinInValue", &RangeTransform::getMinInValue,
             DOC(RangeTransform, getMinInValue))
        .def("setMinInValue", &RangeTransform::setMinInValue, "value"_a,
             DOC(RangeTransform, setMinInValue))
        .def("hasMinInValue", &RangeTransform::hasMinInValue,
             DOC(RangeTransform, hasMinInValue))
        .def("unsetMinInValue", &RangeTransform::unsetMinInValue,
             DOC(RangeTransform, unsetMinInValue))
        .def("getMaxInValue", &RangeTransform::getMaxInValue,
             DOC(RangeTransform, getMaxInValue))
        .def("setMaxInValue", &RangeTransform::setMaxInValue, "value"_a,
             DOC(RangeTransform, setMaxInValue))
        .def("hasMaxInValue", &RangeTransform::hasMaxInValue,
             DOC(RangeTransform, hasMaxInValue))
        .def("unsetMaxInValue", &RangeTransform::unsetMaxInValue,
             DOC(RangeTransform, unsetMaxInValue))
        .def("getMinOutValue", &RangeTransform::getMinOutValue,
             DOC(RangeTransform, getMinOutValue))
        .def("setMinOutValue", &RangeTransform::setMinOutValue, "value"_a,
             DOC(RangeTransform, setMinOutValue))
        .def("hasMinOutValue", &RangeTransform::hasMinOutValue,
             DOC(RangeTransform, hasMinOutValue))
        .def("unsetMinOutValue", &RangeTransform::unsetMinOutValue,
             DOC(RangeTransform, unsetMinOutValue))
        .def("getMaxOutValue", &RangeTransform::getMaxOutValue,
             DOC(RangeTransform, getMaxOutValue))
        .def("setMaxOutValue", &RangeTransform::setMaxOutValue, "value"_a,
             DOC(RangeTransform, setMaxOutValue))
        .def("hasMaxOutValue", &RangeTransform::hasMaxOutValue,
             DOC(RangeTransform, hasMaxOutValue))
        .def("unsetMaxOutValue", &RangeTransform::unsetMaxOutValue,
             DOC(RangeTransform, unsetMaxOutValue));

    defRepr(clsRangeTransform);
}

} // namespace OCIO_NAMESPACE
