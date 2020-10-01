// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "PyOpenColorIO.h"
#include "PyUtils.h"

namespace OCIO_NAMESPACE
{

namespace 
{

enum ColorSpaceIterator
{
    IT_COLOR_SPACE_CATEGORY = 0
};

using ColorSpaceCategoryIterator = PyIterator<ColorSpaceRcPtr, IT_COLOR_SPACE_CATEGORY>;

std::vector<float> getAllocationVarsStdVec(const ColorSpaceRcPtr & p) {
    std::vector<float> vars;
    vars.resize(p->getAllocationNumVars());
    p->getAllocationVars(vars.data());
    return vars;
}

std::vector<std::string> getCategoriesStdVec(const ColorSpaceRcPtr & p) {
    std::vector<std::string> categories;
    categories.reserve(p->getNumCategories());
    for (int i = 0; i < p->getNumCategories(); i++)
    {
        categories.push_back(p->getCategory(i));
    }
    return categories;
}

} // namespace

void bindPyColorSpace(py::module & m)
{
    ColorSpaceRcPtr DEFAULT = ColorSpace::Create();

    auto clsColorSpace = py::class_<ColorSpace, ColorSpaceRcPtr /* holder */>(
        m, "ColorSpace", 
        DOC(ColorSpace));

    auto clsColorSpaceCategoryIterator = py::class_<ColorSpaceCategoryIterator>(
        clsColorSpace, "ColorSpaceCategoryIterator");

    clsColorSpace
        .def(py::init([]() 
            {
                 return ColorSpace::Create(); 
            }), 
             DOC(ColorSpace, Create))
        .def(py::init([](ReferenceSpaceType referenceSpace) 
            { 
                return ColorSpace::Create(referenceSpace); 
            }), 
             "referenceSpace"_a,
             DOC(ColorSpace, Create, 2))
        .def(py::init([](ReferenceSpaceType referenceSpace,
                         const std::string & name,
                         const std::string & family,
                         const std::string & encoding,
                         const std::string & equalityGroup,
                         const std::string & description,
                         BitDepth bitDepth,
                         bool isData,
                         Allocation allocation, 
                         const std::vector<float> & allocationVars,
                         const TransformRcPtr & toReference,
                         const TransformRcPtr & fromReference,
                         const std::vector<std::string> & categories) 
            {
                ColorSpaceRcPtr p = ColorSpace::Create(referenceSpace);
                if (!name.empty())          { p->setName(name.c_str()); }
                if (!family.empty())        { p->setFamily(family.c_str()); }
                if (!encoding.empty())      { p->setEncoding(encoding.c_str()); }
                if (!equalityGroup.empty()) { p->setEqualityGroup(equalityGroup.c_str()); }
                if (!description.empty())   { p->setDescription(description.c_str()); }
                p->setBitDepth(bitDepth);
                p->setIsData(isData);
                p->setAllocation(allocation);
                if (!allocationVars.empty())
                {
                    if (allocationVars.size() < 2 || allocationVars.size() > 3)
                    {
                        throw Exception("vars must be a float array, size 2 or 3");
                    }
                    p->setAllocationVars((int)allocationVars.size(), allocationVars.data());
                }
                if (toReference)
                { 
                    p->setTransform(toReference, COLORSPACE_DIR_TO_REFERENCE); 
                }
                if (fromReference) 
                { 
                    p->setTransform(fromReference, COLORSPACE_DIR_FROM_REFERENCE); 
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
             "encoding"_a = DEFAULT->getEncoding(),
             "equalityGroup"_a = DEFAULT->getEqualityGroup(),
             "description"_a = DEFAULT->getDescription(),
             "bitDepth"_a = DEFAULT->getBitDepth(),
             "isData"_a = DEFAULT->isData(),
             "allocation"_a = DEFAULT->getAllocation(), 
             "allocationVars"_a = getAllocationVarsStdVec(DEFAULT),
             "toReference"_a = DEFAULT->getTransform(COLORSPACE_DIR_TO_REFERENCE),
             "fromReference"_a = DEFAULT->getTransform(COLORSPACE_DIR_FROM_REFERENCE),
             "categories"_a = getCategoriesStdVec(DEFAULT),
             DOC(ColorSpace, Create, 2))

        .def("getName", &ColorSpace::getName, 
             DOC(ColorSpace, getName))
        .def("setName", &ColorSpace::setName, "name"_a, 
             DOC(ColorSpace, setName))
        .def("getFamily", &ColorSpace::getFamily, 
             DOC(ColorSpace, getFamily))
        .def("setFamily", &ColorSpace::setFamily, "family"_a, 
             DOC(ColorSpace, setFamily))
        .def("getEncoding", &ColorSpace::getEncoding, 
             DOC(ColorSpace, getEncoding))
        .def("setEncoding", &ColorSpace::setEncoding, "encoding"_a, 
             DOC(ColorSpace, setEncoding))
        .def("getEqualityGroup", &ColorSpace::getEqualityGroup, 
             DOC(ColorSpace, getEqualityGroup))
        .def("setEqualityGroup", &ColorSpace::setEqualityGroup, "equalityGroup"_a, 
             DOC(ColorSpace, setEqualityGroup))
        .def("getDescription", &ColorSpace::getDescription, 
             DOC(ColorSpace, getDescription))
        .def("setDescription", &ColorSpace::setDescription, "description"_a, 
             DOC(ColorSpace, setDescription))
        .def("getBitDepth", &ColorSpace::getBitDepth, 
             DOC(ColorSpace, getBitDepth))
        .def("setBitDepth", &ColorSpace::setBitDepth, "bitDepth"_a, 
             DOC(ColorSpace, setBitDepth))

        // Categories
        .def("hasCategory", &ColorSpace::hasCategory, "category"_a, 
             DOC(ColorSpace, hasCategory))
        .def("addCategory", &ColorSpace::addCategory, "category"_a, 
             DOC(ColorSpace, addCategory))
        .def("removeCategory", &ColorSpace::removeCategory, "category"_a, 
             DOC(ColorSpace, removeCategory))
        .def("getCategories", [](ColorSpaceRcPtr & self) 
            { 
                return ColorSpaceCategoryIterator(self); 
            })
        .def("clearCategories", &ColorSpace::clearCategories, 
             DOC(ColorSpace, clearCategories))

        // Data
        .def("isData", &ColorSpace::isData, 
             DOC(ColorSpace, isData))
        .def("setIsData", &ColorSpace::setIsData, "isData"_a, 
             DOC(ColorSpace, setIsData))
        .def("getReferenceSpaceType", &ColorSpace::getReferenceSpaceType, 
             DOC(ColorSpace, getReferenceSpaceType))

        // Allocation
        .def("getAllocation", &ColorSpace::getAllocation, 
             DOC(ColorSpace, getAllocation))
        .def("setAllocation", &ColorSpace::setAllocation, "allocation"_a, 
             DOC(ColorSpace, setAllocation))
        .def("getAllocationVars", [](ColorSpaceRcPtr & self) 
            { 
                return getAllocationVarsStdVec(self); 
            })
        .def("setAllocationVars", [](ColorSpaceRcPtr self, const std::vector<float> & vars)
            { 
                if (vars.size() < 2 || vars.size() > 3)
                {
                    throw Exception("vars must be a float array, size 2 or 3");
                }
                self->setAllocationVars((int)vars.size(), vars.data());
            }, 
             "vars"_a,
             DOC(ColorSpace, setAllocationVars))

        // Transform
        .def("getTransform", &ColorSpace::getTransform, "direction"_a, 
             DOC(ColorSpace, getTransform))
        .def("setTransform", &ColorSpace::setTransform, "transform"_a, "direction"_a, 
             DOC(ColorSpace, setTransform));

    defStr(clsColorSpace);

    clsColorSpaceCategoryIterator
        .def("__len__", [](ColorSpaceCategoryIterator & it) 
            { 
                return it.m_obj->getNumCategories(); 
            })
        .def("__getitem__", [](ColorSpaceCategoryIterator & it, int i) 
            { 
                it.checkIndex(i, it.m_obj->getNumCategories());
                return it.m_obj->getCategory(i);
            })
        .def("__iter__", [](ColorSpaceCategoryIterator & it) -> ColorSpaceCategoryIterator & 
            { 
                return it; 
            })
        .def("__next__", [](ColorSpaceCategoryIterator & it)
            {
                int i = it.nextIndex(it.m_obj->getNumCategories());
                return it.m_obj->getCategory(i);
            });
}

} // namespace OCIO_NAMESPACE
