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
            }, 
            "type"_a, 
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
        .def("applyRGB", [](CPUProcessorRcPtr & self, py::buffer & data) 
            {
                py::buffer_info info = data.request();
                checkBufferDivisible(info, 3);

                // Interpret as single row of RGB pixels
                BitDepth bitDepth = getBufferBitDepth(info);
                long numChannels = 3;
                long width = (long)info.size / numChannels;
                long height = 1;
                ptrdiff_t chanStrideBytes = (ptrdiff_t)info.itemsize;
                ptrdiff_t xStrideBytes = chanStrideBytes * numChannels;
                ptrdiff_t yStrideBytes = xStrideBytes * width;

                py::gil_scoped_release release;

                PackedImageDesc img(info.ptr, 
                                    width, height, 
                                    numChannels, 
                                    bitDepth, 
                                    chanStrideBytes, 
                                    xStrideBytes, 
                                    yStrideBytes);
                self->apply(img);
            },
             "data"_a, 
             DOC(CPUProcessor, applyRGB))
        .def("applyRGB", [](CPUProcessorRcPtr & self, std::vector<float> & data) 
            {
                checkVectorDivisible(data, 3);

                long numChannels = 3;
                long width = (long)data.size() / numChannels;
                long height = 1;

                PackedImageDesc img(&data[0], width, height, numChannels);
                self->apply(img);

                return data;
            },
             "data"_a,
             py::call_guard<py::gil_scoped_release>(), 
             DOC(CPUProcessor, applyRGB))
        .def("applyRGBA", [](CPUProcessorRcPtr & self, py::buffer & data) 
            {
                py::buffer_info info = data.request();
                checkBufferDivisible(info, 4);

                // Interpret as single row of RGBA pixels
                BitDepth bitDepth = getBufferBitDepth(info);
                long numChannels = 4;
                long width = (long)info.size / numChannels;
                long height = 1;
                ptrdiff_t chanStrideBytes = (ptrdiff_t)info.itemsize;
                ptrdiff_t xStrideBytes = chanStrideBytes * numChannels;
                ptrdiff_t yStrideBytes = xStrideBytes * width;

                py::gil_scoped_release release;

                PackedImageDesc img(info.ptr, 
                                    width, height, 
                                    numChannels, 
                                    bitDepth, 
                                    chanStrideBytes, 
                                    xStrideBytes, 
                                    yStrideBytes);
                self->apply(img);
            },
             "data"_a, 
             DOC(CPUProcessor, applyRGBA))
        .def("applyRGBA", [](CPUProcessorRcPtr & self, std::vector<float> & data) 
            {
                checkVectorDivisible(data, 4);

                long numChannels = 4;
                long width = (long)data.size() / numChannels;
                long height = 1;

                PackedImageDesc img(&data[0], width, height, numChannels);
                self->apply(img);

                return data;
            },
             "data"_a,
             py::call_guard<py::gil_scoped_release>(), 
             DOC(CPUProcessor, applyRGBA));
}

} // namespace OCIO_NAMESPACE
