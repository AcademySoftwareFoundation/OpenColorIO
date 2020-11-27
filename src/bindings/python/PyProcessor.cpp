// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


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
    auto cls = py::class_<Processor, ProcessorRcPtr /* holder */>(m, "Processor")

        .def("isNoOp", &Processor::isNoOp)
        .def("hasChannelCrosstalk", &Processor::hasChannelCrosstalk)
        .def("getCacheID", &Processor::getCacheID)
        .def("getProcessorMetadata", &Processor::getProcessorMetadata)
        .def("getFormatMetadata", &Processor::getFormatMetadata, 
             py::return_value_policy::reference_internal)
        .def("getTransformFormatMetadata", [](ProcessorRcPtr & self) 
            {
                return TransformFormatMetadataIterator(self);
            })
        .def("createGroupTransform", &Processor::createGroupTransform)
        .def("getDynamicProperty", &Processor::getDynamicProperty, "type"_a)
        .def("hasDynamicProperty",
             (bool (Processor::*)(DynamicPropertyType) const noexcept)
             &Processor::hasDynamicProperty,
             "type"_a)
        .def("hasDynamicProperty",
             (bool (Processor::*)() const noexcept)
             &Processor::hasDynamicProperty)
        .def("getOptimizedProcessor",
             (ConstProcessorRcPtr(Processor::*)(OptimizationFlags) const)
             &Processor::getOptimizedProcessor, "oFlags"_a)
        .def("getOptimizedProcessor",
             (ConstProcessorRcPtr(Processor::*)(BitDepth, BitDepth, OptimizationFlags) const)
             &Processor::getOptimizedProcessor,
             "inBitDepth"_a, "outBitDepth"_a, "oFlags"_a)

        // GPU Renderer
        .def("getDefaultGPUProcessor", &Processor::getDefaultGPUProcessor)
        .def("getOptimizedGPUProcessor", &Processor::getOptimizedGPUProcessor, "oFlags"_a)

        // CPU Renderer
        .def("getDefaultCPUProcessor", &Processor::getDefaultCPUProcessor)
        .def("getOptimizedCPUProcessor", 
             (ConstCPUProcessorRcPtr (Processor::*)(OptimizationFlags) const) 
             &Processor::getOptimizedCPUProcessor, 
             "oFlags"_a)
        .def("getOptimizedCPUProcessor", 
             (ConstCPUProcessorRcPtr (Processor::*)(BitDepth, BitDepth, OptimizationFlags) const) 
             &Processor::getOptimizedCPUProcessor, 
             "inBitDepth"_a, "outBitDepth"_a, "oFlags"_a);

    py::class_<TransformFormatMetadataIterator>(cls, "TransformFormatMetadataIterator")
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
