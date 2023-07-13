// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{
namespace
{

enum ColorSpaceSetIterator
{
    IT_COLOR_SPACE_NAME = 0,
    IT_COLOR_SPACE
};

using ColorSpaceNameIterator = PyIterator<ColorSpaceSetRcPtr, IT_COLOR_SPACE_NAME>;
using ColorSpaceIterator     = PyIterator<ColorSpaceSetRcPtr, IT_COLOR_SPACE>;

} // namespace

void bindPyColorSpaceSet(py::module & m)
{
    ColorSpaceSetRcPtr DEFAULT = ColorSpaceSet::Create();

    auto clsColorSpaceSet = 
        py::class_<ColorSpaceSet, ColorSpaceSetRcPtr>(
            m.attr("ColorSpaceSet"));

    auto clsColorSpaceNameIterator = 
        py::class_<ColorSpaceNameIterator>(
            clsColorSpaceSet, "ColorSpaceNameIterator");

    auto clsColorSpaceIterator = 
        py::class_<ColorSpaceIterator>(
            clsColorSpaceSet, "ColorSpaceIterator");

    clsColorSpaceSet
        .def(py::init(&ColorSpaceSet::Create), 
             DOC(ColorSpaceSet, Create))

        .def("__deepcopy__", [](const ConstColorSpaceSetRcPtr & self, py::dict)
            {
                return self->createEditableCopy();
            },
            "memo"_a)

        .def("__eq__", &ColorSpaceSet::operator==, py::is_operator(), 
             DOC(ColorSpaceSet, operator, eq))
        .def("__ne__", &ColorSpaceSet::operator!=, py::is_operator(), 
             DOC(ColorSpaceSet, operator, ne))

        .def("__sub__", [](ConstColorSpaceSetRcPtr & self, ConstColorSpaceSetRcPtr & rcss)
            { 
                return self - rcss; 
            }, 
             py::is_operator(), 
             DOC(PyOpenColorIO, ConstColorSpaceSetRcPtr, operator, sub))
        .def("__or__", [](ConstColorSpaceSetRcPtr & self, ConstColorSpaceSetRcPtr & rcss) 
            { 
                return self || rcss; 
            }, 
             py::is_operator(), 
             DOC(PyOpenColorIO, ConstColorSpaceSetRcPtr, operator, lor))
        .def("__and__", [](ConstColorSpaceSetRcPtr & self, ConstColorSpaceSetRcPtr & rcss) 
            { 
                return self && rcss; 
            }, 
             py::is_operator(), 
             DOC(PyOpenColorIO, ConstColorSpaceSetRcPtr, operator, land))

        .def("getColorSpaceNames", [](ColorSpaceSetRcPtr & self) 
            { 
                return ColorSpaceNameIterator(self); 
            })
        .def("getColorSpaces", [](ColorSpaceSetRcPtr & self) 
            { 
                return ColorSpaceIterator(self); 
            })
        .def("getColorSpace", &ColorSpaceSet::getColorSpace, "name"_a, 
             DOC(ColorSpaceSet, getColorSpace))
        .def("hasColorSpace", &ColorSpaceSet::hasColorSpace, "name"_a, 
             DOC(ColorSpaceSet, hasColorSpace))
        .def("addColorSpace", &ColorSpaceSet::addColorSpace, "colorSpace"_a, 
             DOC(ColorSpaceSet, addColorSpace))
        .def("addColorSpaces", &ColorSpaceSet::addColorSpaces, "colorSpaces"_a, 
             DOC(ColorSpaceSet, addColorSpaces))
        .def("removeColorSpace", &ColorSpaceSet::removeColorSpace, "colorSpace"_a, 
             DOC(ColorSpaceSet, removeColorSpace))
        .def("removeColorSpaces", &ColorSpaceSet::removeColorSpaces, "colorSpaces"_a, 
             DOC(ColorSpaceSet, removeColorSpaces))
        .def("clearColorSpaces", &ColorSpaceSet::clearColorSpaces, 
             DOC(ColorSpaceSet, clearColorSpaces));

    clsColorSpaceNameIterator
        .def("__len__", [](ColorSpaceNameIterator & it) 
            { 
                return it.m_obj->getNumColorSpaces(); 
            })
        .def("__getitem__", [](ColorSpaceNameIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumColorSpaces());
                return it.m_obj->getColorSpaceNameByIndex(i);
            })
        .def("__iter__", [](ColorSpaceNameIterator & it) -> ColorSpaceNameIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ColorSpaceNameIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumColorSpaces());
                return it.m_obj->getColorSpaceNameByIndex(i);
            });

    clsColorSpaceIterator
        .def("__len__", [](ColorSpaceIterator & it) { return it.m_obj->getNumColorSpaces(); })
        .def("__getitem__", [](ColorSpaceIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumColorSpaces());
                return it.m_obj->getColorSpaceByIndex(i);
            })
        .def("__iter__", [](ColorSpaceIterator & it) -> ColorSpaceIterator & { return it; })
        .def("__next__", [](ColorSpaceIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumColorSpaces());
                return it.m_obj->getColorSpaceByIndex(i);
            });
}

} // namespace OCIO_NAMESPACE
