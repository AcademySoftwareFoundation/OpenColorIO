// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"

namespace OCIO_NAMESPACE
{

void bindPyGPUProcessor(py::module & m)
{
    py::class_<GPUProcessor, GPUProcessorRcPtr /* holder */>(m, "GPUProcessor")
        .def("isNoOp", &GPUProcessor::isNoOp)
        .def("hasChannelCrosstalk", &GPUProcessor::hasChannelCrosstalk)
        .def("getCacheID", &GPUProcessor::getCacheID)
        .def("getDynamicProperty", &GPUProcessor::getDynamicProperty, "type"_a)
        .def("extractGpuShaderInfo", 
             (void (GPUProcessor::*)(GpuShaderDescRcPtr &) const) 
             &GPUProcessor::extractGpuShaderInfo,
             "shaderDesc"_a)
        .def("extractGpuShaderInfo", 
             (void (GPUProcessor::*)(GpuShaderCreatorRcPtr &) const) 
             &GPUProcessor::extractGpuShaderInfo,
             "shaderCreator"_a);
}

} // namespace OCIO_NAMESPACE
