// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

void bindPyGPUProcessor(py::module & m)
{
    auto clsGPUProcessor = 
        py::class_<GPUProcessor, GPUProcessorRcPtr /* holder */>(
            m, "GPUProcessor", 
            DOC(GPUProcessor))

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
             DOC(GPUProcessor, extractGpuShaderInfo))
        .def("extractGpuShaderInfo", 
             (void (GPUProcessor::*)(GpuShaderCreatorRcPtr &) const) 
             &GPUProcessor::extractGpuShaderInfo,
             "shaderCreator"_a, 
             DOC(GPUProcessor, extractGpuShaderInfo, 2));
}

} // namespace OCIO_NAMESPACE
