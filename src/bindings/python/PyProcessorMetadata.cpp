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
    auto cls = py::class_<ProcessorMetadata, ProcessorMetadataRcPtr /* holder */>(m, "ProcessorMetadata")
        .def(py::init(&ProcessorMetadata::Create))

        .def("getFiles", [](ProcessorMetadataRcPtr & self) { return FileIterator(self); })
        .def("getLooks", [](ProcessorMetadataRcPtr & self) { return LookIterator(self); })
        .def("addFile", &ProcessorMetadata::addFile, "fname"_a)
        .def("addLook", &ProcessorMetadata::addLook, "look"_a);

    py::class_<FileIterator>(cls, "FileIterator")
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

    py::class_<LookIterator>(cls, "LookIterator")
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
