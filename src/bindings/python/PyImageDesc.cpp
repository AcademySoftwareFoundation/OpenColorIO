// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyImageDesc.h"

namespace OCIO_NAMESPACE
{

void bindPyImageDesc(py::module & m)
{
    m.attr("AutoStride") = AutoStride;

    auto clsImageDesc = 
        py::class_<PyImageDesc>(
            m.attr("ImageDesc"))

        .def(py::init<>(), 
             DOC(ImageDesc, ImageDesc))

        .def("__repr__", [](const PyImageDesc & self)
            { 
                std::ostringstream os;
                os << self.m_img;
                return os.str();
            })

        .def("getBitDepth", &PyImageDesc::getBitDepth, 
             DOC(ImageDesc, getBitDepth))
        .def("getWidth", &PyImageDesc::getWidth,
             DOC(ImageDesc, getWidth))
        .def("getHeight", &PyImageDesc::getHeight,
             DOC(ImageDesc, getHeight))
        .def("getXStrideBytes", &PyImageDesc::getXStrideBytes,
             DOC(ImageDesc, getXStrideBytes))
        .def("getYStrideBytes", &PyImageDesc::getYStrideBytes,
             DOC(ImageDesc, getYStrideBytes))
        .def("isRGBAPacked", &PyImageDesc::isRGBAPacked,
             DOC(ImageDesc, isRGBAPacked))
        .def("isFloat", &PyImageDesc::isFloat,
             DOC(ImageDesc, isFloat));

    // Subclasses
    bindPyPackedImageDesc(m);
    bindPyPlanarImageDesc(m);
}

} // namespace OCIO_NAMESPACE
