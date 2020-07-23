// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

void bindPyRangeTransform(py::module & m)
{
    RangeTransformRcPtr DEFAULT = RangeTransform::Create();

    auto cls = py::class_<RangeTransform, 
                          RangeTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "RangeTransform", DS(RangeTransform))
        .def(py::init(&RangeTransform::Create), DS(RangeTransform, Create))
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
             DS(RangeTransform, RangeTransform),
             "minInValue"_a = DEFAULT->getMinInValue(),
             "maxInValue"_a = DEFAULT->getMaxInValue(),
             "minOutValue"_a = DEFAULT->getMinOutValue(),
             "maxOutValue"_a = DEFAULT->getMaxOutValue(),
             "direction"_a = DEFAULT->getDirection())

        .def("getStyle", &RangeTransform::getStyle, DS(RangeTransform, getStyle))
        .def("setStyle", &RangeTransform::setStyle, DS(RangeTransform, setStyle), "style"_a)
        .def("getFormatMetadata", 
             (FormatMetadata & (RangeTransform::*)()) &RangeTransform::getFormatMetadata,
             DS(RangeTransform, getFormatMetadata),
             py::return_value_policy::reference_internal)
        .def("getFormatMetadata", 
             (const FormatMetadata & (RangeTransform::*)() const) 
             &RangeTransform::getFormatMetadata,
             DS(RangeTransform, getFormatMetadata),
             py::return_value_policy::reference_internal)
        .def("equals", &RangeTransform::equals, DS(RangeTransform, equals), "other"_a)
        .def("getFileInputBitDepth", &RangeTransform::getFileInputBitDepth,
             DS(RangeTransform, getFileInputBitDepth))
        .def("setFileInputBitDepth", &RangeTransform::setFileInputBitDepth,
             DS(RangeTransform, setFileInputBitDepth),
             "bitDepth"_a)
        .def("getFileOutputBitDepth", &RangeTransform::getFileOutputBitDepth,
             DS(RangeTransform, getFileOutputBitDepth))
        .def("setFileOutputBitDepth", &RangeTransform::setFileOutputBitDepth,
             DS(RangeTransform, setFileOutputBitDepth),
             "bitDepth"_a)
        .def("getMinInValue", &RangeTransform::getMinInValue,
             DS(RangeTransform, getMinInValue))
        .def("setMinInValue", &RangeTransform::setMinInValue,
             DS(RangeTransform, setMinInValue),
             "value"_a)
        .def("hasMinInValue", &RangeTransform::hasMinInValue,
             DS(RangeTransform, hasMinInValue))
        .def("unsetMinInValue", &RangeTransform::unsetMinInValue,
             DS(RangeTransform, unsetMinInValue))
        .def("getMaxInValue", &RangeTransform::getMaxInValue,
             DS(RangeTransform, getMaxInValue))
        .def("setMaxInValue", &RangeTransform::setMaxInValue,
             DS(RangeTransform, setMaxInValue),
             "value"_a)
        .def("hasMaxInValue", &RangeTransform::hasMaxInValue,
             DS(RangeTransform, hasMaxInValue))
        .def("unsetMaxOutValue", &RangeTransform::unsetMaxOutValue,
             DS(RangeTransform, unsetMaxOutValue))
        .def("getMinOutValue", &RangeTransform::getMinOutValue,
             DS(RangeTransform, getMinOutValue))
        .def("setMinOutValue", &RangeTransform::setMinOutValue,
             DS(RangeTransform, setMinOutValue),
             "value"_a)
        .def("hasMinOutValue", &RangeTransform::hasMinOutValue,
             DS(RangeTransform, hasMinOutValue))
        .def("unsetMinOutValue", &RangeTransform::unsetMinOutValue,
             DS(RangeTransform, unsetMinOutValue))
        .def("getMaxOutValue", &RangeTransform::getMaxOutValue,
             DS(RangeTransform, getMaxOutValue))
        .def("setMaxOutValue", &RangeTransform::setMaxOutValue,
             DS(RangeTransform, setMaxOutValue),
             "value"_a)
        .def("hasMaxOutValue", &RangeTransform::hasMaxOutValue,
             DS(RangeTransform, hasMaxOutValue))
        .def("unsetMaxOutValue", &RangeTransform::unsetMaxOutValue,
             DS(RangeTransform, unsetMaxOutValue));

    defStr(cls);
}

} // namespace OCIO_NAMESPACE
