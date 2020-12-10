// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <vector>

#include "PyDynamicProperty.h"
#include "PyOpenColorIO.h"
#include "PyUtils.h"
#include "PyImageDesc.h"

namespace OCIO_NAMESPACE
{

void bindPyCPUProcessor(py::module & m)
{
    auto clsCPUProcessor = 
        py::class_<CPUProcessor, CPUProcessorRcPtr /* holder */>(
            m, "CPUProcessor", 
            DOC(CPUProcessor))

        .def("isNoOp", &CPUProcessor::isNoOp, 
             DOC(CPUProcessor, isNoOp))
        .def("isIdentity", &CPUProcessor::isIdentity, 
             DOC(CPUProcessor, isIdentity))
        .def("hasChannelCrosstalk", &CPUProcessor::hasChannelCrosstalk, 
             DOC(CPUProcessor, hasChannelCrosstalk))
        .def("getCacheID", &CPUProcessor::getCacheID, 
             DOC(CPUProcessor, getCacheID))
        .def("getInputBitDepth", &CPUProcessor::getInputBitDepth, 
             DOC(CPUProcessor, getInputBitDepth))
        .def("getOutputBitDepth", &CPUProcessor::getOutputBitDepth, 
             DOC(CPUProcessor, getOutputBitDepth))
        .def("getDynamicProperty", [](CPUProcessorRcPtr & self, DynamicPropertyType type) 
            {
                return PyDynamicProperty(self->getDynamicProperty(type));
            }, "type"_a, 
             DOC(CPUProcessor, getDynamicProperty))

        .def("apply", [](CPUProcessorRcPtr & self, PyImageDesc & imgDesc) 
            {
                self->apply((*imgDesc.m_img));
            },
             "imgDesc"_a,
             py::call_guard<py::gil_scoped_release>(), 
             DOC(CPUProcessor, apply))
        .def("apply", [](CPUProcessorRcPtr & self, 
                         PyImageDesc & srcImgDesc, 
                         PyImageDesc & dstImgDesc)
            {
                self->apply((*srcImgDesc.m_img), (*dstImgDesc.m_img));
            },
             "srcImgDesc"_a, "dstImgDesc"_a,
             py::call_guard<py::gil_scoped_release>(),
             DOC(CPUProcessor, apply, 2))
        .def("applyRGB", [](CPUProcessorRcPtr & self, py::buffer & pixel) 
            {
                py::buffer_info info = pixel.request();
                checkBufferType(info, py::dtype("float32"));
                checkBufferDivisible(info, 3);

                py::gil_scoped_release release;

                self->applyRGB(static_cast<float *>(info.ptr));
                return pixel;
            },
             "pixel"_a, 
             DOC(CPUProcessor, applyRGB))
        .def("applyRGB", [](CPUProcessorRcPtr & self, std::vector<float> & pixel) 
            {
                checkVectorDivisible(pixel, 3);
                self->applyRGB(pixel.data());
                return pixel;
            },
             "pixel"_a,
             py::call_guard<py::gil_scoped_release>(), 
             DOC(CPUProcessor, applyRGB))
        .def("applyRGBA", [](CPUProcessorRcPtr & self, py::buffer & pixel) 
            {
                py::buffer_info info = pixel.request();
                checkBufferType(info, py::dtype("float32"));
                checkBufferDivisible(info, 4);

                py::gil_scoped_release release;

                self->applyRGBA(static_cast<float *>(info.ptr));
                return pixel;
            },
             "pixel"_a, 
             DOC(CPUProcessor, applyRGBA))
        .def("applyRGBA", [](CPUProcessorRcPtr & self, std::vector<float> & pixel) 
            {
                checkVectorDivisible(pixel, 4);
                self->applyRGBA(pixel.data());
                return pixel;
            },
             "pixel"_a,
             py::call_guard<py::gil_scoped_release>(), 
             DOC(CPUProcessor, applyRGBA));
}

} // namespace OCIO_NAMESPACE
