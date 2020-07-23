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

    auto cls = py::class_<ViewTransform, ViewTransformRcPtr /* holder */>(m, "ViewTransform", DS(ViewTransform))
        .def(py::init([](ReferenceSpaceType referenceSpace) 
            { 
                return ViewTransform::Create(referenceSpace); 
            }), 
             DS(ViewTransform, Create),
             "referenceSpace"_a)
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
             DS(ViewTransform, ViewTransform),
             "referenceSpace"_a = DEFAULT->getReferenceSpaceType(),
             "name"_a = DEFAULT->getName(),
             "family"_a = DEFAULT->getFamily(),
             "description"_a = DEFAULT->getDescription(),
             "toReference"_a = DEFAULT->getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE),
             "fromReference"_a = DEFAULT->getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE),
             "categories"_a = getCategoriesStdVec(DEFAULT))  

        .def("getName", &ViewTransform::getName, DS(ViewTransform, getName))
        .def("setName", &ViewTransform::setName, DS(ViewTransform, setName), "name"_a)
        .def("getFamily", &ViewTransform::getFamily, DS(ViewTransform, getFamily))
        .def("setFamily", &ViewTransform::setFamily, DS(ViewTransform, setFamily), "family"_a)
        .def("getDescription", &ViewTransform::getDescription, DS(ViewTransform, getDescription))
        .def("setDescription", &ViewTransform::setDescription, DS(ViewTransform, setDescription), "description"_a)
        .def("hasCategory", &ViewTransform::hasCategory, DS(ViewTransform, hasCategory), "category"_a)
        .def("addCategory", &ViewTransform::addCategory, DS(ViewTransform, addCategory), "category"_a)
        .def("removeCategory", &ViewTransform::removeCategory, DS(ViewTransform, removeCategory), "category"_a)
        .def("getCategories", [](ViewTransformRcPtr & self) 
            { 
                return ViewTransformCategoryIterator(self); 
            })
        .def("clearCategories", &ViewTransform::clearCategories, DS(ViewTransform, clearCategories))
        .def("getReferenceSpaceType", &ViewTransform::getReferenceSpaceType, DS(ViewTransform, getReferenceSpaceType))
        .def("getTransform", &ViewTransform::getTransform, DS(ViewTransform, getTransform), "direction"_a)
        .def("setTransform", &ViewTransform::setTransform, DS(ViewTransform, setTransform), "transform"_a, "direction"_a);

    defStr(cls);

    py::class_<ViewTransformCategoryIterator>(cls, "ViewTransformCategoryIterator")
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
