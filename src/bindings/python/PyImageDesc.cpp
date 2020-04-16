// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyImageDesc.h"

namespace OCIO_NAMESPACE
{

void bindPyImageDesc(py::module & m)
{
    m.attr("AutoStride") = AutoStride;

    py::class_<PyImageDesc>(m, "ImageDesc")
        .def(py::init<>())

        .def("__str__", [](const PyImageDesc & self)
            { 
                std::ostringstream os;
                os << self.m_img;
                return os.str();
            })

        .def("getBitDepth", &PyImageDesc::getBitDepth)
        .def("getWidth", &PyImageDesc::getWidth)
        .def("getHeight", &PyImageDesc::getHeight)
        .def("getXStrideBytes", &PyImageDesc::getXStrideBytes)
        .def("getYStrideBytes", &PyImageDesc::getYStrideBytes)
        .def("isRGBAPacked", &PyImageDesc::isRGBAPacked)
        .def("isFloat", &PyImageDesc::isFloat);

    // Subclasses
    bindPyPackedImageDesc(m);
    bindPyPlanarImageDesc(m);
}

} // namespace OCIO_NAMESPACE
