// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

void bindPyGPUProcessor(py::module & m)
{
    py::class_<GPUProcessor, GPUProcessorRcPtr /* holder */>(m, "GPUProcessor", DS(GPUProcessor))
        .def("isNoOp", &GPUProcessor::isNoOp, DS(GPUProcessor, isNoOp))
        .def("hasChannelCrosstalk", &GPUProcessor::hasChannelCrosstalk, DS(GPUProcessor, hasChannelCrosstalk))
        .def("getCacheID", &GPUProcessor::getCacheID, DS(GPUProcessor, getCacheID))
        .def("getDynamicProperty", &GPUProcessor::getDynamicProperty, DS(GPUProcessor, getDynamicProperty), "type"_a)
        .def("extractGpuShaderInfo", 
             (void (GPUProcessor::*)(GpuShaderDescRcPtr &) const) 
             &GPUProcessor::extractGpuShaderInfo,
             DS(GPUProcessor, extractGpuShaderInfo),
             "shaderDesc"_a)
        .def("extractGpuShaderInfo", 
             (void (GPUProcessor::*)(GpuShaderCreatorRcPtr &) const) 
             &GPUProcessor::extractGpuShaderInfo,
             DS(GPUProcessor, extractGpuShaderInfo),
             "shaderCreator"_a);
}

} // namespace OCIO_NAMESPACE
