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
    IT_VIEW_TRANSFORM_CATEGORY = 0
};

using ViewTransformCategoryIterator = PyIterator<ViewTransformRcPtr, 
                                                 IT_VIEW_TRANSFORM_CATEGORY>;

std::vector<std::string> getCategoriesStdVec(const ViewTransformRcPtr & p) {
    std::vector<std::string> categories;
    categories.reserve(p->getNumCategories());
    for (int i = 0; i < p->getNumCategories(); i++)
    {
        categories.push_back(p->getCategory(i));
    }
    return categories;
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
                         const std::vector<std::string> & categories) 
            {
                ViewTransformRcPtr p = ViewTransform::Create(referenceSpace);
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
        .def("getFamily", &ViewTransform::getFamily,
             DOC(ViewTransform, getFamily))
        .def("setFamily", &ViewTransform::setFamily, "family"_a,
             DOC(ViewTransform, setFamily))
        .def("getDescription", &ViewTransform::getDescription,
             DOC(ViewTransform, getDescription))
        .def("setDescription", &ViewTransform::setDescription, "description"_a,
             DOC(ViewTransform, setDescription))
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
}

} // namespace OCIO_NAMESPACE
