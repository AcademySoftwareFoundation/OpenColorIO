// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{
namespace 
{

enum ViewTransformIterator
{
    IT_VIEW_TRANSFORM_CATEGORY = 0,
    IT_VIEW_TRANSFORM_ALIAS
};

using ViewTransformCategoryIterator = PyIterator<ViewTransformRcPtr, 
                                                 IT_VIEW_TRANSFORM_CATEGORY>;
using ViewTransformAliasIterator = PyIterator<ViewTransformRcPtr, IT_VIEW_TRANSFORM_ALIAS>;

std::vector<std::string> getCategoriesStdVec(const ViewTransformRcPtr & p) {
    std::vector<std::string> categories;
    categories.reserve(p->getNumCategories());
    for (int i = 0; i < p->getNumCategories(); i++)
    {
        categories.push_back(p->getCategory(i));
    }
    return categories;
}

std::vector<std::string> getAliasesStdVec(const ViewTransformRcPtr & p)
{
    std::vector<std::string> aliases;
    aliases.reserve(p->getNumAliases());
    for (size_t i = 0; i < p->getNumAliases(); i++)
    {
        aliases.push_back(p->getAlias(i));
    }
    return aliases;
}

} // namespace

void bindPyViewTransform(py::module & m)
{
    ViewTransformRcPtr DEFAULT = ViewTransform::Create(REFERENCE_SPACE_SCENE);

    auto clsViewTransform = 
        py::class_<ViewTransform, ViewTransformRcPtr>(
            m.attr("ViewTransform"));

    auto clsViewTransformCategoryIterator = 
        py::class_<ViewTransformCategoryIterator>(
            clsViewTransform, "ViewTransformCategoryIterator");

    auto clsViewTransformAliasIterator =
        py::class_<ViewTransformAliasIterator>(
            clsViewTransform, "ViewTransformAliasIterator");

    clsViewTransform
        .def(py::init([](ReferenceSpaceType referenceSpace) 
            { 
                return ViewTransform::Create(referenceSpace); 
            }), 
             "referenceSpace"_a,
             DOC(ViewTransform, Create))
        .def(py::init([](ReferenceSpaceType referenceSpace,
                         const std::string & name,
                         const std::string & family,
                         const std::string & description,
                         const TransformRcPtr & toReference,
                         const TransformRcPtr & fromReference,
                         const std::vector<std::string> & categories, 
                         const std::vector<std::string> & aliases)
            {
                ViewTransformRcPtr p = ViewTransform::Create(referenceSpace);
                if (!aliases.empty())
                {
                    p->clearAliases();
                    for (size_t i = 0; i < aliases.size(); i++)
                    {
                        p->addAlias(aliases[i].c_str());
                    }
                }
                // Setting the name will remove alias named the same, so set name after.
                if (!name.empty())          { p->setName(name.c_str()); }
                if (!family.empty())        { p->setFamily(family.c_str()); }
                if (!description.empty())   { p->setDescription(description.c_str()); }
                if (toReference)
                { 
                    p->setTransform(toReference, VIEWTRANSFORM_DIR_TO_REFERENCE); 
                }
                if (fromReference) 
                { 
                    p->setTransform(fromReference, VIEWTRANSFORM_DIR_FROM_REFERENCE); 
                }
                if (!categories.empty())
                {
                    p->clearCategories();
                    for (size_t i = 0; i < categories.size(); i++)
                    {
                        p->addCategory(categories[i].c_str());
                    }
                }
                return p;
            }), 
             "referenceSpace"_a = DEFAULT->getReferenceSpaceType(),
             "name"_a = DEFAULT->getName(),
             "family"_a = DEFAULT->getFamily(),
             "description"_a = DEFAULT->getDescription(),
             "toReference"_a = DEFAULT->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE),
             "fromReference"_a = DEFAULT->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE),
             "categories"_a = getCategoriesStdVec(DEFAULT),
             "aliases"_a = getAliasesStdVec(DEFAULT),
             DOC(ViewTransform, Create))

        .def("__deepcopy__", [](const ConstViewTransformRcPtr & self, py::dict)
            {
                return self->createEditableCopy();
            },
            "memo"_a)

        .def("getName", &ViewTransform::getName,
             DOC(ViewTransform, getName))
        .def("setName", &ViewTransform::setName, "name"_a,
             DOC(ViewTransform, setName))

        // Aliases.
        .def("hasAlias", &ViewTransform::hasAlias, "alias"_a,
             DOC(ViewTransform, hasAlias))
        .def("addAlias", &ViewTransform::addAlias, "alias"_a.none(false),
             DOC(ViewTransform, addAlias))
        .def("removeAlias", &ViewTransform::removeAlias, "alias"_a.none(false),
             DOC(ViewTransform, removeAlias))
        .def("getAliases", [](ViewTransformRcPtr & self)
            {
                return ViewTransformAliasIterator(self);
            })
        .def("clearAliases", &ViewTransform::clearAliases,
             DOC(ViewTransform, clearAliases))

        .def("getFamily", &ViewTransform::getFamily,
             DOC(ViewTransform, getFamily))
        .def("setFamily", &ViewTransform::setFamily, "family"_a,
             DOC(ViewTransform, setFamily))
        .def("getDescription", &ViewTransform::getDescription,
             DOC(ViewTransform, getDescription))
        .def("setDescription", &ViewTransform::setDescription, "description"_a,
             DOC(ViewTransform, setDescription))
        .def("getInterchangeAttribute", &ViewTransform::getInterchangeAttribute, "attrName"_a,
             DOC(ViewTransform, getInterchangeAttribute))
        .def("setInterchangeAttribute", &ViewTransform::setInterchangeAttribute, "attrName"_a, "attrValue"_a,
            DOC(ViewTransform, setInterchangeAttribute))
        .def("getInterchangeAttributes", &ViewTransform:: getInterchangeAttributes,
            DOC(ViewTransform, getInterchangeAttributes))
        .def("hasCategory", &ViewTransform::hasCategory, "category"_a,
             DOC(ViewTransform, hasCategory))
        .def("addCategory", &ViewTransform::addCategory, "category"_a,
             DOC(ViewTransform, addCategory))
        .def("removeCategory", &ViewTransform::removeCategory, "category"_a,
             DOC(ViewTransform, removeCategory))
        .def("getCategories", [](ViewTransformRcPtr & self) 
            { 
                return ViewTransformCategoryIterator(self); 
            })
        .def("clearCategories", &ViewTransform::clearCategories,
             DOC(ViewTransform, clearCategories))
        .def("getReferenceSpaceType", &ViewTransform::getReferenceSpaceType,
             DOC(ViewTransform, getReferenceSpaceType))
        .def("getTransform", &ViewTransform::getTransform, "direction"_a,
             DOC(ViewTransform, getTransform))
        .def("setTransform", &ViewTransform::setTransform, "transform"_a, "direction"_a,
             DOC(ViewTransform, setTransform));

    defRepr(clsViewTransform);

    clsViewTransformCategoryIterator
        .def("__len__", [](ViewTransformCategoryIterator & it) 
            { 
                return it.m_obj->getNumCategories(); 
            })
        .def("__getitem__", [](ViewTransformCategoryIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumCategories());
                return it.m_obj->getCategory(i);
            })
        .def("__iter__", [](ViewTransformCategoryIterator & it) -> ViewTransformCategoryIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ViewTransformCategoryIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumCategories());
                return it.m_obj->getCategory(i);
            });

    clsViewTransformAliasIterator
        .def("__len__", [](ViewTransformAliasIterator & it)
            {
                return it.m_obj->getNumAliases();
            })
        .def("__getitem__", [](ViewTransformAliasIterator & it, int i)
            {
                it.checkIndex(i, (int)it.m_obj->getNumAliases());
                return it.m_obj->getAlias(i);
            })
        .def("__iter__", [](ViewTransformAliasIterator & it) -> ViewTransformAliasIterator &
            {
                return it;
            })
        .def("__next__", [](ViewTransformAliasIterator & it)
            {
                int i = it.nextIndex((int)it.m_obj->getNumAliases());
                return it.m_obj->getAlias(i);
            });
}

} // namespace OCIO_NAMESPACE
