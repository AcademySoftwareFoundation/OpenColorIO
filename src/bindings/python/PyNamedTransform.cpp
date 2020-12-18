// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

enum NamedTransformIterator
{
    IT_NAMED_TRANSFORM_DEFINITION_CATEGORY = 0
};

using NamedTransformCategoryIterator = PyIterator<NamedTransformRcPtr,
                                                  IT_NAMED_TRANSFORM_DEFINITION_CATEGORY>;

std::vector<std::string> getCategoriesStdVec(const NamedTransformRcPtr & p)
{
    std::vector<std::string> categories;
    categories.reserve(p->getNumCategories());
    for (int i = 0; i < p->getNumCategories(); i++)
    {
        categories.push_back(p->getCategory(i));
    }
    return categories;
}

void bindPyNamedTransform(py::module & m)
{
    NamedTransformRcPtr DEFAULT = NamedTransform::Create();

    auto cls = py::class_<NamedTransform, NamedTransformRcPtr /* holder */>(
        m, "NamedTransform", DOC(NamedTransform))
        .def(py::init([]() { return NamedTransform::Create(); }), DOC(NamedTransform, Create))
        .def(py::init([](const std::string & name,
                         const std::string & family,
                         const std::string & description,
                         const ConstTransformRcPtr & forwardTransform,
                         const ConstTransformRcPtr & inverseTransform,
                         const std::vector<std::string> & categories)
            {
                NamedTransformRcPtr p = NamedTransform::Create();
                if (!name.empty())
                {
                    p->setName(name.c_str());
                }
                if (!family.empty())
                {
                    p->setFamily(family.c_str());
                }
                if (!description.empty())
                {
                    p->setDescription(description.c_str());
                }
                if (forwardTransform)
                {
                    p->setTransform(forwardTransform, TRANSFORM_DIR_FORWARD);
                }
                if (inverseTransform)
                {
                    p->setTransform(inverseTransform, TRANSFORM_DIR_INVERSE);
                }
                if (!categories.empty())
                {
                    p->clearCategories();
                    for (const auto & cat : categories)
                    {
                        p->addCategory(cat.c_str());
                    }
                }
                return p;
            }),
            "name"_a.none(false) = "",
            "family"_a.none(false) = "",
            "description"_a.none(false) = "",
            "forwardTransform"_a = ConstTransformRcPtr(),
            "inverseTransform"_a = ConstTransformRcPtr(),
            "categories"_a = getCategoriesStdVec(DEFAULT),
            DOC(NamedTransform, Create))

        .def("getName", &NamedTransform::getName,
            DOC(NamedTransform, getName))
        .def("setName", &NamedTransform::setName, "name"_a.none(false),
            DOC(NamedTransform, setName))
        .def("getFamily", &NamedTransform::getFamily,
            DOC(NamedTransform, getFamily))
        .def("setFamily", &NamedTransform::setFamily, "family"_a.none(false),
            DOC(NamedTransform, setFamily))
        .def("getDescription", &NamedTransform::getDescription,
            DOC(NamedTransform, getDescription))
        .def("setDescription", &NamedTransform::setDescription, "description"_a.none(false),
            DOC(NamedTransform, setDescription))
        .def("getEncoding", &NamedTransform::getEncoding,
            DOC(NamedTransform, getEncoding))
        .def("setEncoding", &NamedTransform::setEncoding, "encodig"_a.none(false),
            DOC(NamedTransform, setEncoding))

        // Transform
        .def("getTransform", &NamedTransform::getTransform, "direction"_a,
            DOC(NamedTransform, getTransform))
        .def("setTransform", &NamedTransform::setTransform, "transform"_a, "direction"_a,
            DOC(NamedTransform, setTransform))

        // Categories
        .def("hasCategory", &NamedTransform::hasCategory, "category"_a,
            DOC(NamedTransform, hasCategory))
        .def("addCategory", &NamedTransform::addCategory, "category"_a,
            DOC(NamedTransform, addCategory))
        .def("removeCategory", &NamedTransform::removeCategory, "category"_a,
            DOC(NamedTransform, removeCategory))
        .def("getCategories", [](NamedTransformRcPtr & self)
            {
                return NamedTransformCategoryIterator(self);
            })
        .def("clearCategories", &NamedTransform::clearCategories,
            DOC(NamedTransform, clearCategories));

    defStr(cls);

    py::class_<NamedTransformCategoryIterator>(cls, "NamedTransformCategoryIterator")
        .def("__len__", [](NamedTransformCategoryIterator & it)
            {
                return it.m_obj->getNumCategories();
            })
        .def("__getitem__", [](NamedTransformCategoryIterator & it, int i)
            {
                it.checkIndex(i, it.m_obj->getNumCategories());
                return it.m_obj->getCategory(i);
            })
        .def("__iter__", [](NamedTransformCategoryIterator & it) -> NamedTransformCategoryIterator &
            {
                return it;
            })
        .def("__next__", [](NamedTransformCategoryIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumCategories());
                return it.m_obj->getCategory(i);
            });
}

} // namespace OCIO_NAMESPACE
