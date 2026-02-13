// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include <pybind11/pybind11.h>

#include "PyOpenColorIO.h"
#include "docstrings.h"


namespace py = pybind11;
using namespace pybind11::literals;


namespace OCIO_NAMESPACE
{

void bindPyGPUProcessor(py::module & m)
{
    auto clsGPUProcessor = 
        py::class_<GPUProcessor, GPUProcessorRcPtr>(
            m.attr("GPUProcessor"))

        .def("isNoOp", &GPUProcessor::isNoOp, 
             DOC(GPUProcessor, isNoOp))
        .def("hasChannelCrosstalk", &GPUProcessor::hasChannelCrosstalk, 
             DOC(GPUProcessor, hasChannelCrosstalk))
        .def("getCacheID", &GPUProcessor::getCacheID, 
             DOC(GPUProcessor, getCacheID))
        .def("extractGpuShaderInfo", 
             (void (GPUProcessor::*)(GpuShaderDescRcPtr &) const) 
             &GPUProcessor::extractGpuShaderInfo,
             "shaderDesc"_a, 
             DOC(GPUProcessor, extractGpuShaderInfo));
}

} // namespace OCIO_NAMESPACE
