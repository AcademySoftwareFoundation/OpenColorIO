// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "PyTransform.h"

namespace OCIO_NAMESPACE
{

enum NamedTransformIterator
{
    IT_NAMED_TRANSFORM_DEFINITION_CATEGORY = 0,
    IT_NAMED_TRANSFORM_ALIAS
};

using NamedTransformCategoryIterator = PyIterator<NamedTransformRcPtr,
                                                  IT_NAMED_TRANSFORM_DEFINITION_CATEGORY>;

using NamedTransformAliasIterator = PyIterator<NamedTransformRcPtr,
                                               IT_NAMED_TRANSFORM_ALIAS>;

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

std::vector<std::string> getAliasesStdVec(const NamedTransformRcPtr & p)
{
    std::vector<std::string> aliases;
    aliases.reserve(p->getNumAliases());
    for (int i = 0; i < p->getNumCategories(); i++)
    {
        aliases.push_back(p->getAlias(i));
    }
    return aliases;
}

void bindPyNamedTransform(py::module & m)
{
    NamedTransformRcPtr DEFAULT = NamedTransform::Create();

    auto cls = py::class_<NamedTransform,
                          NamedTransformRcPtr /* holder */>(m, "NamedTransform")
        .def(py::init([]() { return NamedTransform::Create(); }))
        .def(py::init([](const std::string & name,
                         const std::vector<std::string> & aliases,
                         const std::string & family,
                         const std::string & description,
                         const ConstTransformRcPtr & forwardTransform,
                         const ConstTransformRcPtr & inverseTransform,
                         const std::vector<std::string> & categories)
            {
                NamedTransformRcPtr p = NamedTransform::Create();
                if (!aliases.empty())
                {
                    p->clearAliases();
                    for (size_t i = 0; i < aliases.size(); i++)
                    {
                        p->addAlias(aliases[i].c_str());
                    }
                }
                // Setting the name will remove alias named the same, so set name after.
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
            "name"_a = "",
            "aliases"_a = getAliasesStdVec(DEFAULT),
            "family"_a = "",
            "description"_a = "",
            "forwardTransform"_a = ConstTransformRcPtr(),
            "inverseTransform"_a = ConstTransformRcPtr(),
            "categories"_a = getCategoriesStdVec(DEFAULT))

        .def("getName", &NamedTransform::getName)
        .def("setName", &NamedTransform::setName, "name"_a)

        // Aliases.
        .def("addAlias", &NamedTransform::addAlias, "alias"_a.none(false))
        .def("removeAlias", &NamedTransform::removeAlias, "alias"_a.none(false))
        .def("getAliases", [](NamedTransformRcPtr & self)
            { 
                return NamedTransformAliasIterator(self);
            })
        .def("clearAliases", &NamedTransform::clearAliases)

        .def("getFamily", &NamedTransform::getFamily)
        .def("setFamily", &NamedTransform::setFamily, "family"_a)
        .def("getDescription", &NamedTransform::getDescription)
        .def("setDescription", &NamedTransform::setDescription, "description"_a)

        // Transform
        .def("getTransform", &NamedTransform::getTransform, "direction"_a)
        .def("setTransform", &NamedTransform::setTransform, "transform"_a, "direction"_a)

        // Categories
        .def("hasCategory", &NamedTransform::hasCategory, "category"_a)
        .def("addCategory", &NamedTransform::addCategory, "category"_a)
        .def("removeCategory", &NamedTransform::removeCategory, "category"_a)
        .def("getCategories", [](NamedTransformRcPtr & self)
            {
                return NamedTransformCategoryIterator(self);
            })
        .def("clearCategories", &NamedTransform::clearCategories);

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

    py::class_<NamedTransformAliasIterator>(cls, "NamedTransformAliasIterator")
        .def("__len__", [](NamedTransformAliasIterator & it)
            { 
                return it.m_obj->getNumAliases(); 
            })
        .def("__getitem__", [](NamedTransformAliasIterator & it, int i)
            { 
                it.checkIndex(i, (int)it.m_obj->getNumAliases());
                return it.m_obj->getAlias(i);
            })
        .def("__iter__", [](NamedTransformAliasIterator & it) -> NamedTransformAliasIterator &
            { 
                return it; 
            })
        .def("__next__", [](NamedTransformAliasIterator & it)
            {
                int i = it.nextIndex((int)it.m_obj->getNumAliases());
                return it.m_obj->getAlias(i);
            });

}

} // namespace OCIO_NAMESPACE
