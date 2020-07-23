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

    auto cls = py::class_<ColorSpace, ColorSpaceRcPtr /* holder */>(m, "ColorSpace", DS(ColorSpace))
        .def(py::init([]() { return ColorSpace::Create(); }), DS(ColorSpace, Create))
        .def(py::init([](ReferenceSpaceType referenceSpace) 
            { 
                return ColorSpace::Create(referenceSpace); 
            }), 
             DS(ColorSpace, ColorSpace),
             "referenceSpace"_a)
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
             DS(ColorSpace, ColorSpace),
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
             "categories"_a = getCategoriesStdVec(DEFAULT))

        .def("getName", &ColorSpace::getName, DS(ColorSpace, getName))
        .def("setName", &ColorSpace::setName, DS(ColorSpace, setName), "name"_a)
        .def("getFamily", &ColorSpace::getFamily, DS(ColorSpace, getFamily))
        .def("setFamily", &ColorSpace::setFamily, DS(ColorSpace, setFamily), "family"_a)
        .def("getEncoding", &ColorSpace::getEncoding, DS(ColorSpace, getEncoding))
        .def("setEncoding", &ColorSpace::setEncoding, DS(ColorSpace, setEncoding), "encoding"_a)
        .def("getEqualityGroup", &ColorSpace::getEqualityGroup, DS(ColorSpace, getEqualityGroup))
        .def("setEqualityGroup", &ColorSpace::setEqualityGroup, DS(ColorSpace, setEqualityGroup), "equalityGroup"_a)
        .def("getDescription", &ColorSpace::getDescription, DS(ColorSpace, getDescription))
        .def("setDescription", &ColorSpace::setDescription, DS(ColorSpace, setDescription), "description"_a)
        .def("getBitDepth", &ColorSpace::getBitDepth, DS(ColorSpace, getBitDepth))
        .def("setBitDepth", &ColorSpace::setBitDepth, DS(ColorSpace, setBitDepth), "bitDepth"_a)

        // Categories
        .def("hasCategory", &ColorSpace::hasCategory, DS(ColorSpace, hasCategory), "category"_a)
        .def("addCategory", &ColorSpace::addCategory, DS(ColorSpace, addCategory), "category"_a)
        .def("removeCategory", &ColorSpace::removeCategory, DS(ColorSpace, removeCategory), "category"_a)
        .def("getCategories", [](ColorSpaceRcPtr & self) 
            { 
                return ColorSpaceCategoryIterator(self); 
            })
        .def("clearCategories", &ColorSpace::clearCategories, DS(ColorSpace, clearCategories))

        // Data
        .def("isData", &ColorSpace::isData, DS(ColorSpace, isData))
        .def("setIsData", &ColorSpace::setIsData, DS(ColorSpace, setIsData), "isData"_a)
        .def("getReferenceSpaceType", &ColorSpace::getReferenceSpaceType, DS(ColorSpace, getReferenceSpaceType))

        // Allocation
        .def("getAllocation", &ColorSpace::getAllocation, DS(ColorSpace, getAllocation))
        .def("setAllocation", &ColorSpace::setAllocation, DS(ColorSpace, setAllocation), "allocation"_a)
        .def("getAllocationVars", [](ColorSpaceRcPtr & self) 
            { 
                return getAllocationVarsStdVec(self); 
            },
             DS(ColorSpace, getAllocationVars))
        .def("setAllocationVars", [](ColorSpaceRcPtr self, const std::vector<float> & vars)
            { 
                if (vars.size() < 2 || vars.size() > 3)
                {
                    throw Exception("vars must be a float array, size 2 or 3");
                }
                self->setAllocationVars((int)vars.size(), vars.data());
            }, 
             DS(ColorSpace, setAllocationVars),
             "vars"_a)

        // Transform
        .def("getTransform", &ColorSpace::getTransform, DS(ColorSpace, getTransform), "direction"_a)
        .def("setTransform", &ColorSpace::setTransform, DS(ColorSpace, setTransform), "transform"_a, "direction"_a);

    defStr(cls);

    py::class_<ColorSpaceCategoryIterator>(cls, "ColorSpaceCategoryIterator")
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
