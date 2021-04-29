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

    auto clsFileTransform = 
        py::class_<FileTransform, FileTransformRcPtr, Transform>(
            m.attr("FileTransform"));

    auto clsFormatIterator = 
        py::class_<FormatIterator>(
            clsFileTransform, "FormatIterator");

     clsFileTransform
        .def(py::init(&FileTransform::Create), 
             DOC(FileTransform, Create))
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
             "direction"_a = DEFAULT->getDirection(), 
             DOC(FileTransform, Create))

        .def_static("getFormats", []() 
            { 
                return FormatIterator(nullptr); 
            })

        .def("getSrc", &FileTransform::getSrc, 
             DOC(FileTransform, getSrc))
        .def("setSrc", &FileTransform::setSrc, "src"_a.none(false), 
             DOC(FileTransform, setSrc))
        .def("getCCCId", &FileTransform::getCCCId, 
             DOC(FileTransform, getCCCId))
        .def("setCCCId", &FileTransform::setCCCId, "cccId"_a.none(false), 
             DOC(FileTransform, setCCCId))
        .def("getCDLStyle", &FileTransform::getCDLStyle, 
             DOC(FileTransform, getCDLStyle))
        .def("setCDLStyle", &FileTransform::setCDLStyle, "style"_a, 
             DOC(FileTransform, setCDLStyle))
        .def("getInterpolation", &FileTransform::getInterpolation, 
             DOC(FileTransform, getInterpolation))
        .def("setInterpolation", &FileTransform::setInterpolation, "interpolation"_a, 
             DOC(FileTransform, setInterpolation));

    defRepr(clsFileTransform);

    clsFormatIterator
        .def("__len__", [](FormatIterator & /* it */) 
            { 
                return FileTransform::GetNumFormats(); 
            })
        .def("__getitem__", [](FormatIterator & it, int i) 
            { 
                it.checkIndex(i, FileTransform::GetNumFormats());
                return py::make_tuple(FileTransform::GetFormatNameByIndex(i), 
                                      FileTransform::GetFormatExtensionByIndex(i));
            })
        .def("__iter__", [](FormatIterator & it) -> FormatIterator & 
            { 
                return it; 
            })
        .def("__next__", [](FormatIterator & it)
            {
                int i = it.nextIndex(FileTransform::GetNumFormats());
                return py::make_tuple(FileTransform::GetFormatNameByIndex(i), 
                                      FileTransform::GetFormatExtensionByIndex(i));
            });
}

} // namespace OCIO_NAMESPACE
