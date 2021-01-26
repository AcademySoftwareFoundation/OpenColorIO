// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyDynamicProperty.h"
#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{

enum ProcessorIterator
{
    IT_TRANSFORM_FORMAT_METADATA = 0
};

using TransformFormatMetadataIterator = PyIterator<ProcessorRcPtr, IT_TRANSFORM_FORMAT_METADATA>;

} // namespace

void bindPyProcessor(py::module & m)
{
    auto clsProcessor = 
        py::class_<Processor, ProcessorRcPtr>(
            m.attr("Processor"));

    auto clsTransformFormatMetadataIterator = 
        py::class_<TransformFormatMetadataIterator>(
            clsProcessor, "TransformFormatMetadataIterator");

    clsProcessor
        .def("isNoOp", &Processor::isNoOp,
             DOC(Processor, isNoOp))
        .def("hasChannelCrosstalk", &Processor::hasChannelCrosstalk,
             DOC(Processor, hasChannelCrosstalk))
        .def("getCacheID", &Processor::getCacheID,
             DOC(Processor, getCacheID))
        .def("getProcessorMetadata", &Processor::getProcessorMetadata,
             DOC(Processor, getProcessorMetadata))
        .def("getFormatMetadata", &Processor::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(Processor, getFormatMetadata))
        .def("getTransformFormatMetadata", [](ProcessorRcPtr & self) 
            {
                return TransformFormatMetadataIterator(self);
            })
        .def("createGroupTransform", &Processor::createGroupTransform,
             DOC(Processor, createGroupTransform))
        .def("getDynamicProperty", [](ProcessorRcPtr & self, DynamicPropertyType type)
            {
                return PyDynamicProperty(self->getDynamicProperty(type));
            },
            "type"_a,
             DOC(Processor, getDynamicProperty))
        .def("hasDynamicProperty",
             (bool (Processor::*)(DynamicPropertyType) const noexcept)
             &Processor::hasDynamicProperty,
             "type"_a,
             DOC(Processor, hasDynamicProperty))
        .def("isDynamic", &Processor::isDynamic,
             DOC(Processor, isDynamic))
        .def("getOptimizedProcessor",
             (ConstProcessorRcPtr(Processor::*)(OptimizationFlags) const)
             &Processor::getOptimizedProcessor, "oFlags"_a,
             DOC(Processor, getOptimizedProcessor))
        .def("getOptimizedProcessor",
             (ConstProcessorRcPtr(Processor::*)(BitDepth, BitDepth, OptimizationFlags) const)
             &Processor::getOptimizedProcessor,
             "inBitDepth"_a, "outBitDepth"_a, "oFlags"_a,
             DOC(Processor, getOptimizedProcessor))

        // GPU Renderer
        .def("getDefaultGPUProcessor", &Processor::getDefaultGPUProcessor,
             DOC(Processor, getDefaultGPUProcessor))
        .def("getOptimizedGPUProcessor", &Processor::getOptimizedGPUProcessor, "oFlags"_a,
             DOC(Processor, getOptimizedGPUProcessor))

        // CPU Renderer
        .def("getDefaultCPUProcessor", &Processor::getDefaultCPUProcessor,
             DOC(Processor, getDefaultCPUProcessor))
        .def("getOptimizedCPUProcessor", 
             (ConstCPUProcessorRcPtr (Processor::*)(OptimizationFlags) const) 
             &Processor::getOptimizedCPUProcessor, 
             "oFlags"_a,
             DOC(Processor, getOptimizedCPUProcessor))
        .def("getOptimizedCPUProcessor", 
             (ConstCPUProcessorRcPtr (Processor::*)(BitDepth, BitDepth, OptimizationFlags) const) 
             &Processor::getOptimizedCPUProcessor, 
             "inBitDepth"_a, "outBitDepth"_a, "oFlags"_a,
             DOC(Processor, getOptimizedCPUProcessor));

    clsTransformFormatMetadataIterator
        .def("__len__", [](TransformFormatMetadataIterator & it) 
            { 
                return it.m_obj->getNumTransforms(); 
            })
        .def("__getitem__", 
             [](TransformFormatMetadataIterator & it, int i) -> const FormatMetadata &
            { 
                it.checkIndex(i, it.m_obj->getNumTransforms());
                return it.m_obj->getTransformFormatMetadata(i);
            }, 
             py::return_value_policy::reference_internal)
        .def("__iter__", 
             [](TransformFormatMetadataIterator & it) -> TransformFormatMetadataIterator & 
            { 
                return it; 
            })
        .def("__next__", [](TransformFormatMetadataIterator & it) -> const FormatMetadata &
            {
                int i = it.nextIndex(it.m_obj->getNumTransforms());
                return it.m_obj->getTransformFormatMetadata(i);
            }, 
             py::return_value_policy::reference_internal);
}

} // namespace OCIO_NAMESPACE
