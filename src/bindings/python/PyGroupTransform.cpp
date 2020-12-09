// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{
namespace 
{

enum GroupTransformIterator
{
    IT_TRANSFORM = 0
};

using TransformIterator = PyIterator<GroupTransformRcPtr, IT_TRANSFORM>;

} // namespace

void bindPyGroupTransform(py::module & m)
{
    GroupTransformRcPtr DEFAULT = GroupTransform::Create();

    auto clsGroupTransform = 
        py::class_<GroupTransform, GroupTransformRcPtr /* holder */, Transform /* base */>(
            m, "GroupTransform", 
            DOC(GroupTransform));

    auto clsTransformIterator = 
        py::class_<TransformIterator>(
            clsGroupTransform, "TransformIterator");

    clsGroupTransform
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
             DOC(GroupTransform, prependTransform));

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
}

} // namespace OCIO_NAMESPACE
