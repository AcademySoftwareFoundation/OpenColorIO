// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace
{

enum ProcessorMetadataIterator
{
    IT_FILE = 0,
    IT_LOOK
};

using FileIterator = PyIterator<ProcessorMetadataRcPtr, IT_FILE>;
using LookIterator = PyIterator<ProcessorMetadataRcPtr, IT_LOOK>;

} // namespace

void bindPyProcessorMetadata(py::module & m)
{
    auto clsProcessorMetadata = 
        py::class_<ProcessorMetadata, ProcessorMetadataRcPtr>(
            m.attr("ProcessorMetadata"));

    auto clsFileIterator =
        py::class_<FileIterator>(
            clsProcessorMetadata, "FileIterator");

    auto clsLookIterator =
        py::class_<LookIterator>(
            clsProcessorMetadata, "LookIterator");

    clsProcessorMetadata
        .def(py::init(&ProcessorMetadata::Create),
             DOC(ProcessorMetadata, Create))

        .def("getFiles", [](ProcessorMetadataRcPtr & self) 
            { 
                return FileIterator(self); 
            })
        .def("getLooks", [](ProcessorMetadataRcPtr & self) 
            { 
                return LookIterator(self); 
            })
        .def("addFile", &ProcessorMetadata::addFile, "fileName"_a,
             DOC(ProcessorMetadata, addFile))
        .def("addLook", &ProcessorMetadata::addLook, "look"_a,
             DOC(ProcessorMetadata, addLook));

    clsFileIterator
        .def("__len__", [](FileIterator & it) { return it.m_obj->getNumFiles(); })
        .def("__getitem__", [](FileIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumFiles());
                return it.m_obj->getFile(i);
            })
        .def("__iter__", [](FileIterator & it) -> FileIterator & { return it; })
        .def("__next__", [](FileIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumFiles());
                return it.m_obj->getFile(i);
            });

    clsLookIterator
        .def("__len__", [](LookIterator & it) { return it.m_obj->getNumLooks(); })
        .def("__getitem__", [](LookIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumLooks());
                return it.m_obj->getLook(i);
            })
        .def("__iter__", [](LookIterator & it) -> LookIterator & { return it; })
        .def("__next__", [](LookIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumLooks());
                return it.m_obj->getLook(i);
            });
}

} // namespace OCIO_NAMESPACE
