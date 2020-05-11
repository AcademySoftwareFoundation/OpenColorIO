// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <vector>

#include "PyOpenColorIO.h"
#include "PyUtils.h"
#include "PyImageDesc.h"

namespace OCIO_NAMESPACE
{

void bindPyCPUProcessor(py::module & m)
{
    py::class_<CPUProcessor, CPUProcessorRcPtr /* holder */>(m, "CPUProcessor")
        .def("isNoOp", &CPUProcessor::isNoOp)
        .def("isIdentity", &CPUProcessor::isIdentity)
        .def("hasChannelCrosstalk", &CPUProcessor::hasChannelCrosstalk)
        .def("getCacheID", &CPUProcessor::getCacheID)
        .def("getInputBitDepth", &CPUProcessor::getInputBitDepth)
        .def("getOutputBitDepth", &CPUProcessor::getOutputBitDepth)
        .def("getDynamicProperty", &CPUProcessor::getDynamicProperty, "type"_a)

        .def("apply", [](CPUProcessorRcPtr & self, PyImageDesc & imgDesc) 
            {
                self->apply((*imgDesc.m_img));
            },
             "imgDesc"_a,
             py::call_guard<py::gil_scoped_release>())
        .def("apply", [](CPUProcessorRcPtr & self, 
                         PyImageDesc & srcImgDesc, 
                         PyImageDesc & dstImgDesc)
            {
                self->apply((*srcImgDesc.m_img), (*dstImgDesc.m_img));
            },
             "srcImgDesc"_a, "dstImgDesc"_a,
             py::call_guard<py::gil_scoped_release>())
        .def("applyRGB", [](CPUProcessorRcPtr & self, py::buffer & pixel) 
            {
                py::buffer_info info = pixel.request();
                checkBufferType(info, py::dtype("float32"));
                checkBufferDivisible(info, 3);

                py::gil_scoped_release release;

                self->applyRGB(static_cast<float *>(info.ptr));
                return pixel;
            },
             "pixel"_a)
        .def("applyRGB", [](CPUProcessorRcPtr & self, std::vector<float> & pixel) 
            {
                checkVectorDivisible(pixel, 3);
                self->applyRGB(pixel.data());
                return pixel;
            },
             "pixel"_a,
             py::call_guard<py::gil_scoped_release>())
        .def("applyRGBA", [](CPUProcessorRcPtr & self, py::buffer & pixel) 
            {
                py::buffer_info info = pixel.request();
                checkBufferType(info, py::dtype("float32"));
                checkBufferDivisible(info, 4);

                py::gil_scoped_release release;

                self->applyRGBA(static_cast<float *>(info.ptr));
                return pixel;
            },
             "pixel"_a)
        .def("applyRGBA", [](CPUProcessorRcPtr & self, std::vector<float> & pixel) 
            {
                checkVectorDivisible(pixel, 4);
                self->applyRGBA(pixel.data());
                return pixel;
            },
             "pixel"_a,
             py::call_guard<py::gil_scoped_release>());
}

} // namespace OCIO_NAMESPACE
