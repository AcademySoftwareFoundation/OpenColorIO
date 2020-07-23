// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyImageDesc.h"

namespace OCIO_NAMESPACE
{

void bindPyImageDesc(py::module & m)
{
    m.attr("AutoStride") = AutoStride;

    py::class_<PyImageDesc>(m, "ImageDesc", DS(ImageDesc))
        .def(py::init<>())

        .def("__str__", [](const PyImageDesc & self)
            { 
                std::ostringstream os;
                os << self.m_img;
                return os.str();
            })

        .def("getBitDepth", &PyImageDesc::getBitDepth, DS(ImageDesc, getBitDepth))
        .def("getWidth", &PyImageDesc::getWidth, DS(ImageDesc, getWidth))
        .def("getHeight", &PyImageDesc::getHeight, DS(ImageDesc, getHeight))
        .def("getXStrideBytes", &PyImageDesc::getXStrideBytes, DS(ImageDesc, getXStrideBytes))
        .def("getYStrideBytes", &PyImageDesc::getYStrideBytes, DS(ImageDesc, getYStrideBytes))
        .def("isRGBAPacked", &PyImageDesc::isRGBAPacked, DS(ImageDesc, isRGBAPacked))
        .def("isFloat", &PyImageDesc::isFloat, DS(ImageDesc, isFloat));

    // Subclasses
    bindPyPackedImageDesc(m);
    bindPyPlanarImageDesc(m);
}

} // namespace OCIO_NAMESPACE
