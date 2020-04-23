// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <fstream>
#include <sstream>

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{

enum ProcessorIterator
{
    IT_WRITE_FORMAT = 0,
    IT_TRANSFORM_FORMAT_METADATA
};

using WriteFormatIterator             = PyIterator<ProcessorRcPtr, IT_WRITE_FORMAT>;
using TransformFormatMetadataIterator = PyIterator<ProcessorRcPtr, 
                                                   IT_TRANSFORM_FORMAT_METADATA>;

} // namespace

void bindPyProcessor(py::module & m)
{
    auto cls = py::class_<Processor, ProcessorRcPtr /* holder */>(m, "Processor")
        .def_static("getWriteFormats", []() { return WriteFormatIterator(nullptr); })

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
        .def("write", [](ProcessorRcPtr & self, 
                         const std::string & formatName, 
                         const std::string & fileName) 
            {
                std::ofstream f(fileName.c_str());
                self->write(formatName.c_str(), f);
                f.close();
            }, 
             "formatName"_a, "fileName"_a)
        .def("write", [](ProcessorRcPtr & self, const std::string & formatName) 
            {
                std::ostringstream os;
                self->write(formatName.c_str(), os);
                return os.str();
            }, 
             "formatName"_a)
        .def("getDynamicProperty", &Processor::getDynamicProperty, "type"_a)
        .def("hasDynamicProperty", &Processor::hasDynamicProperty, "type"_a)
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

    py::class_<WriteFormatIterator>(cls, "WriteFormatIterator")
        .def("__len__", [](WriteFormatIterator & it) 
            { 
                return Processor::getNumWriteFormats(); 
            })
        .def("__getitem__", [](WriteFormatIterator & it, int i) 
            { 
                it.checkIndex(i, Processor::getNumWriteFormats());
                return py::make_tuple(Processor::getFormatNameByIndex(i), 
                                      Processor::getFormatExtensionByIndex(i));
            })
        .def("__iter__", [](WriteFormatIterator & it) -> WriteFormatIterator & { return it; })
        .def("__next__", [](WriteFormatIterator & it)
            {
                int i = it.nextIndex(Processor::getNumWriteFormats());
                return py::make_tuple(Processor::getFormatNameByIndex(i), 
                                      Processor::getFormatExtensionByIndex(i));
            });
}

} // namespace OCIO_NAMESPACE
