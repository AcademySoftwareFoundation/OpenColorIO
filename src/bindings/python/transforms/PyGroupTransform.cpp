// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <fstream>
#include <sstream>

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{
namespace 
{

enum GroupTransformIterator
{
    IT_TRANSFORM = 0,
    IT_WRITE_FORMAT
};

using TransformIterator = PyIterator<GroupTransformRcPtr, IT_TRANSFORM>;
using WriteFormatIterator = PyIterator<GroupTransformRcPtr, IT_WRITE_FORMAT>;

} // namespace

void bindPyGroupTransform(py::module & m)
{
    GroupTransformRcPtr DEFAULT = GroupTransform::Create();

    auto clsGroupTransform = 
        py::class_<GroupTransform, GroupTransformRcPtr, Transform>(
            m.attr("GroupTransform"));

    auto clsTransformIterator = 
        py::class_<TransformIterator>(
            clsGroupTransform, "TransformIterator");

    auto clsWriteFormatIterator = 
        py::class_<WriteFormatIterator>(
            clsGroupTransform, "WriteFormatIterator");

    clsGroupTransform
        .def_static("GetWriteFormats", []()
            {
                return WriteFormatIterator(nullptr);
            })
        .def(py::init(&GroupTransform::Create),
             DOC(GroupTransform, Create))
        .def(py::init([](const std::vector<TransformRcPtr> transforms, 
                         TransformDirection dir)
            {
                GroupTransformRcPtr p = GroupTransform::Create();
                for(auto & t : transforms)
                {
                    p->appendTransform(t);
                }
                p->setDirection(dir);
                p->validate();
                return p;
            }), 
             "transforms"_a = std::vector<ConstTransformRcPtr>{},
             "direction"_a = DEFAULT->getDirection(),
             DOC(GroupTransform, Create))

        .def("__iter__", [](GroupTransformRcPtr & self) 
            { 
                return TransformIterator(self); 
            })
        .def("__len__", &GroupTransform::getNumTransforms,
             DOC(GroupTransform, getNumTransforms))
        .def("__getitem__", 
             (TransformRcPtr & (GroupTransform::*)(int)) &GroupTransform::getTransform, 
             "index"_a,
             DOC(GroupTransform, getTransform))
             
        .def("getFormatMetadata", 
             (FormatMetadata & (GroupTransform::*)()) &GroupTransform::getFormatMetadata,
             py::return_value_policy::reference_internal,
             DOC(GroupTransform, getFormatMetadata))
        .def("appendTransform", &GroupTransform::appendTransform, "transform"_a,
             DOC(GroupTransform, appendTransform))
        .def("prependTransform", &GroupTransform::prependTransform, "transform"_a,
             DOC(GroupTransform, prependTransform))
        .def("write", [](GroupTransformRcPtr & self,
                         const ConstConfigRcPtr & config,
                         const ConstContextRcPtr & context,
                         const std::string & formatName, 
                         const std::string & fileName) 
            {
                std::ofstream f(fileName.c_str());
                self->write(config, context, formatName.c_str(), f);
                f.close();
            }, 
             "config"_a, "context"_a, "formatName"_a, "fileName"_a,
             DOC(GroupTransform, write))
        .def("write", [](GroupTransformRcPtr & self,
                         const ConstConfigRcPtr & config,
                         const ConstContextRcPtr & context,
                         const std::string & formatName)
            {
                std::ostringstream os;
                self->write(config, context, formatName.c_str(), os);
                return os.str();
            }, 
            "config"_a, "context"_a, "formatName"_a,
             DOC(GroupTransform, write));

    defStr(clsGroupTransform);

    clsTransformIterator
        .def("__len__", [](TransformIterator & it) { return it.m_obj->getNumTransforms(); })
        .def("__getitem__", [](TransformIterator & it, int i) 
            { 
                // GroupTransform provides index check with exception
                return it.m_obj->getTransform(i);
            })
        .def("__iter__", [](TransformIterator & it) -> TransformIterator & { return it; })
        .def("__next__", [](TransformIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumTransforms());
                return it.m_obj->getTransform(i);
            });

    clsWriteFormatIterator
        .def("__len__", [](WriteFormatIterator & it)
            {
                return GroupTransform::GetNumWriteFormats();
            })
        .def("__getitem__", [](WriteFormatIterator & it, int i)
            {
                it.checkIndex(i, GroupTransform::GetNumWriteFormats());
                return py::make_tuple(GroupTransform::GetFormatNameByIndex(i),
                                      GroupTransform::GetFormatExtensionByIndex(i));
            })
        .def("__iter__", [](WriteFormatIterator & it) -> WriteFormatIterator &
            {
                return it;
            })
        .def("__next__", [](WriteFormatIterator & it)
            {
                int i = it.nextIndex(GroupTransform::GetNumWriteFormats());
                return py::make_tuple(GroupTransform::GetFormatNameByIndex(i),
                                      GroupTransform::GetFormatExtensionByIndex(i));
            });
}

} // namespace OCIO_NAMESPACE
