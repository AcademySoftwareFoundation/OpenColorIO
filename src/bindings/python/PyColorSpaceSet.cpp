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

    auto cls = py::class_<ColorSpaceSet, ColorSpaceSetRcPtr /* holder */>(m, "ColorSpaceSet")
        .def(py::init(&ColorSpaceSet::Create))

        .def("__eq__", &ColorSpaceSet::operator==, py::is_operator())
        .def("__ne__", &ColorSpaceSet::operator!=, py::is_operator())

        .def("__sub__", [](ConstColorSpaceSetRcPtr & self, ConstColorSpaceSetRcPtr & rcss)
            { 
                return self - rcss; 
            }, 
             py::is_operator())
        .def("__or__", [](ConstColorSpaceSetRcPtr & self, ConstColorSpaceSetRcPtr & rcss) 
            { 
                return self || rcss; 
            }, 
             py::is_operator())
        .def("__and__", [](ConstColorSpaceSetRcPtr & self, ConstColorSpaceSetRcPtr & rcss) 
            { 
                return self && rcss; 
            }, 
             py::is_operator())

        .def("getColorSpaceNames", [](ColorSpaceSetRcPtr & self) 
            { 
                return ColorSpaceNameIterator(self); 
            })
        .def("getColorSpaces", [](ColorSpaceSetRcPtr & self) 
            { 
                return ColorSpaceIterator(self); 
            })
        .def("getColorSpace", &ColorSpaceSet::getColorSpace, "name"_a)
        .def("addColorSpace", &ColorSpaceSet::addColorSpace, "cs"_a)
        .def("addColorSpaces", &ColorSpaceSet::addColorSpaces, "cs"_a)
        .def("removeColorSpace", &ColorSpaceSet::removeColorSpace, "cs"_a)
        .def("removeColorSpaces", &ColorSpaceSet::removeColorSpaces, "cs"_a)
        .def("clearColorSpaces", &ColorSpaceSet::clearColorSpaces);

    py::class_<ColorSpaceNameIterator>(cls, "ColorSpaceNameIterator")
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

    py::class_<ColorSpaceIterator>(cls, "ColorSpaceIterator")
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
