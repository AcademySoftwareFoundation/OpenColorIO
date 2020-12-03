// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

namespace
{

enum FileTransformIterator
{
    IT_FORMAT = 0
};

using FormatIterator = PyIterator<FileTransformRcPtr, IT_FORMAT>;

} // namespace

void bindPyFileTransform(py::module & m)
{
    FileTransformRcPtr DEFAULT = FileTransform::Create();

    auto cls = py::class_<FileTransform, 
                          FileTransformRcPtr /* holder */, 
                          Transform /* base */>(m, "FileTransform")
        .def(py::init(&FileTransform::Create))
        .def(py::init([](const std::string & src, 
                         const std::string & id, 
                         Interpolation interp,
                         TransformDirection dir) 
            {
                FileTransformRcPtr p = FileTransform::Create();
                if (!src.empty()) { p->setSrc(src.c_str()); }
                if (!id.empty())  { p->setCCCId(id.c_str()); }
                p->setInterpolation(interp);
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             "src"_a = DEFAULT->getSrc(), 
             "cccId"_a = DEFAULT->getCCCId(),
             "interpolation"_a = DEFAULT->getInterpolation(),
             "direction"_a = DEFAULT->getDirection())

        .def_static("getFormats", []() { return FormatIterator(nullptr); })

        .def("getSrc", &FileTransform::getSrc)
        .def("setSrc", &FileTransform::setSrc, "src"_a.none(false))
        .def("getCCCId", &FileTransform::getCCCId)
        .def("setCCCId", &FileTransform::setCCCId, "cccId"_a.none(false))
        .def("getCDLStyle", &FileTransform::getCDLStyle)
        .def("setCDLStyle", &FileTransform::setCDLStyle, "style"_a)
        .def("getInterpolation", &FileTransform::getInterpolation)
        .def("setInterpolation", &FileTransform::setInterpolation, "interpolation"_a);

    defStr(cls);

    py::class_<FormatIterator>(cls, "FormatIterator")
        .def("__len__", [](FormatIterator & it) { return FileTransform::getNumFormats(); })
        .def("__getitem__", [](FormatIterator & it, int i) 
            { 
                it.checkIndex(i, FileTransform::getNumFormats());
                return py::make_tuple(FileTransform::getFormatNameByIndex(i), 
                                      FileTransform::getFormatExtensionByIndex(i));
            })
        .def("__iter__", [](FormatIterator & it) -> FormatIterator & { return it; })
        .def("__next__", [](FormatIterator & it)
            {
                int i = it.nextIndex(FileTransform::getNumFormats());
                return py::make_tuple(FileTransform::getFormatNameByIndex(i), 
                                      FileTransform::getFormatExtensionByIndex(i));
            });
}

} // namespace OCIO_NAMESPACE
