// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "PyOpenColorIO.h"
#include "PyUtils.h"

#include <sstream>

namespace OCIO_NAMESPACE
{

namespace
{

enum FormatMetadataIterator
{
    IT_ATTRIBUTE_NAME = 0,
    IT_ATTRIBUTE,
    IT_CONST_CHILD_ELEMENT,
    IT_CHILD_ELEMENT
};

using AttributeNameIterator     = PyIterator<const FormatMetadata &, IT_ATTRIBUTE_NAME>;
using AttributeIterator         = PyIterator<const FormatMetadata &, IT_ATTRIBUTE>;
using ConstChildElementIterator = PyIterator<const FormatMetadata &, IT_CONST_CHILD_ELEMENT>;
using ChildElementIterator      = PyIterator<FormatMetadata &, IT_CHILD_ELEMENT>;

} // namespace

void bindPyFormatMetadata(py::module & m)
{
    auto clsFormatMetadata = 
        py::class_<FormatMetadata>(
            m.attr("FormatMetadata"));

    auto clsAttributeNameIterator = 
        py::class_<AttributeNameIterator>(
            clsFormatMetadata, "AttributeNameIterator");

    auto clsAttributeIterator = 
        py::class_<AttributeIterator>(
            clsFormatMetadata, "AttributeIterator");

    auto clsConstChildElementIterator = 
        py::class_<ConstChildElementIterator>(
            clsFormatMetadata, "ConstChildElementIterator");

    auto clsChildElementIterator = 
        py::class_<ChildElementIterator>(
            clsFormatMetadata, "ChildElementIterator");

    clsFormatMetadata
        .def("__iter__", [](const FormatMetadata & self) 
            { 
                return AttributeNameIterator(self); 
            })
        .def("__len__", &FormatMetadata::getNumAttributes, 
             DOC(FormatMetadata, getNumAttributes))
        .def("__getitem__", [](const FormatMetadata & self, const std::string & name) 
            {
                for (int i = 0; i < self.getNumAttributes(); i++)
                {
                    const char * attrName = self.getAttributeName(i);
                    if (StringUtils::Compare(std::string(attrName), name))
                    {
                        return self.getAttributeValue(i);
                    }
                }

                std::ostringstream os;
                os << " '" << name << "'";
                throw py::key_error(os.str());
            },
             "name"_a.none(false), 
             DOC(FormatMetadata, getAttributeValue))
        .def("__setitem__", &FormatMetadata::addAttribute, "name"_a.none(false), "value"_a.none(false), 
             DOC(FormatMetadata, addAttribute))
        .def("__contains__", [](const FormatMetadata & self, const std::string & name) -> bool
            {
                for (int i = 0; i < self.getNumAttributes(); i++)
                {
                    const char * attrName = self.getAttributeName(i);
                    if (StringUtils::Compare(std::string(attrName), name))
                    {
                        return true;
                    }
                }
                return false;
            },
             "name"_a.none(false))
        .def("__repr__", [](const FormatMetadata & self)
            {
                std::ostringstream oss;
                oss << self;
                return oss.str();
            })

        .def("getElementName", &FormatMetadata::getElementName, 
             DOC(FormatMetadata, getElementName))
        .def("setElementName", &FormatMetadata::setElementName, "name"_a.none(false), 
             DOC(FormatMetadata, setElementName))
        .def("getElementValue", &FormatMetadata::getElementValue, 
             DOC(FormatMetadata, getElementValue))
        .def("setElementValue", &FormatMetadata::setElementValue, "value"_a.none(false), 
             DOC(FormatMetadata, setElementValue))
        .def("getAttributes", [](const FormatMetadata & self) 
            { 
                return AttributeIterator(self); 
            })
        .def("getChildElements", [](FormatMetadata & self) 
            { 
                return ChildElementIterator(self); 
            })
        .def("addChildElement", &FormatMetadata::addChildElement, 
             "name"_a.none(false), "value"_a.none(false), 
             DOC(FormatMetadata, addChildElement))
        .def("getName", &FormatMetadata::getName,
             DOC(FormatMetadata, getName))
        .def("setName", &FormatMetadata::setName, "name"_a.none(false),
             DOC(FormatMetadata, setName))
        .def("getID", &FormatMetadata::getID,
             DOC(FormatMetadata, getID))
        .def("setID", &FormatMetadata::setID, "id"_a.none(false),
             DOC(FormatMetadata, setID))
        .def("clear", &FormatMetadata::clear, 
             DOC(FormatMetadata, clear));

    clsAttributeNameIterator
        .def("__len__", [](AttributeNameIterator & it) { return it.m_obj.getNumAttributes(); })
        .def("__getitem__", [](AttributeNameIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj.getNumAttributes());
                return it.m_obj.getAttributeName(i);
            })
        .def("__iter__", [](AttributeNameIterator & it) -> AttributeNameIterator & { return it; })
        .def("__next__", [](AttributeNameIterator & it)
            {
                int i = it.nextIndex(it.m_obj.getNumAttributes());
                return it.m_obj.getAttributeName(i);
            });

    clsAttributeIterator
        .def("__len__", [](AttributeIterator & it) { return it.m_obj.getNumAttributes(); })
        .def("__getitem__", [](AttributeIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj.getNumAttributes());
                return py::make_tuple(it.m_obj.getAttributeName(i), 
                                      it.m_obj.getAttributeValue(i));
            })
        .def("__iter__", [](AttributeIterator & it) -> AttributeIterator & { return it; })
        .def("__next__", [](AttributeIterator & it)
            {
                int i = it.nextIndex(it.m_obj.getNumAttributes());
                return py::make_tuple(it.m_obj.getAttributeName(i), 
                                      it.m_obj.getAttributeValue(i));
            });

    clsConstChildElementIterator
        .def("__len__", [](ConstChildElementIterator & it) 
            { 
                return it.m_obj.getNumChildrenElements(); 
            })
        .def("__getitem__", [](ConstChildElementIterator & it, int i) -> const FormatMetadata &
            { 
                // FormatMetadata provides index check with exception
                return it.m_obj.getChildElement(i);
            }, 
             py::return_value_policy::reference_internal)
        .def("__iter__", [](ConstChildElementIterator & it) -> ConstChildElementIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ConstChildElementIterator & it) -> const FormatMetadata &
            {
                int i = it.nextIndex(it.m_obj.getNumChildrenElements());
                return it.m_obj.getChildElement(i);
            }, 
             py::return_value_policy::reference_internal);

    clsChildElementIterator
        .def("__len__", [](ChildElementIterator & it) 
            { 
                return it.m_obj.getNumChildrenElements(); 
            })
        .def("__getitem__", [](ChildElementIterator & it, int i) -> FormatMetadata &
            { 
                // FormatMetadata provides index check with exception
                return it.m_obj.getChildElement(i);
            }, 
             py::return_value_policy::reference_internal)
        .def("__iter__", [](ChildElementIterator & it) -> ChildElementIterator & { return it; })
        .def("__next__", [](ChildElementIterator & it) -> FormatMetadata &
            {
                int i = it.nextIndex(it.m_obj.getNumChildrenElements());
                return it.m_obj.getChildElement(i);
            }, 
             py::return_value_policy::reference_internal);
}

} // namespace OCIO_NAMESPACE
