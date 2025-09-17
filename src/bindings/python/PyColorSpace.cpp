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
    IT_COLOR_SPACE_CATEGORY = 0,
    IT_COLOR_SPACE_ALIAS
};

using ColorSpaceCategoryIterator = PyIterator<ColorSpaceRcPtr, IT_COLOR_SPACE_CATEGORY>;
using ColorSpaceAliasIterator = PyIterator<ColorSpaceRcPtr, IT_COLOR_SPACE_ALIAS>;

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

std::vector<std::string> getAliasesStdVec(const ColorSpaceRcPtr & p)
{
    std::vector<std::string> aliases;
    aliases.reserve(p->getNumAliases());
    for (int i = 0; i < p->getNumCategories(); i++)
    {
        aliases.push_back(p->getAlias(i));
    }
    return aliases;
}

} // namespace

void bindPyColorSpace(py::module & m)
{
    ColorSpaceRcPtr DEFAULT = ColorSpace::Create();

    auto clsColorSpace = 
        py::class_<ColorSpace, ColorSpaceRcPtr>(
            m.attr("ColorSpace"));

    auto clsColorSpaceCategoryIterator = 
        py::class_<ColorSpaceCategoryIterator>(
            clsColorSpace, "ColorSpaceCategoryIterator");

    auto clsColorSpaceAliasIterator = 
        py::class_<ColorSpaceAliasIterator>(
            clsColorSpace, "ColorSpaceAliasIterator");

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
                         const std::vector<std::string> & aliases,
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
                         const std::vector<std::string> & categories,
                         const std::string & interopID
            ) 
            {
                ColorSpaceRcPtr p = ColorSpace::Create(referenceSpace);
                if (!aliases.empty())
                {
                    p->clearAliases();
                    for (size_t i = 0; i < aliases.size(); i++)
                    {
                        p->addAlias(aliases[i].c_str());
                    }
                }
                // Setting the name will remove alias named the same, so set name after.
                if (!name.empty())           { p->setName(name.c_str()); }
                if (!family.empty())         { p->setFamily(family.c_str()); }
                if (!encoding.empty())       { p->setEncoding(encoding.c_str()); }
                if (!equalityGroup.empty())  { p->setEqualityGroup(equalityGroup.c_str()); }
                if (!description.empty())    { p->setDescription(description.c_str()); }
                if (!interopID.empty())      { p->setInteropID(interopID.c_str()); }
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
             "name"_a.none(false) = DEFAULT->getName(),
             "aliases"_a = getAliasesStdVec(DEFAULT),
             "family"_a.none(false) = DEFAULT->getFamily(),
             "encoding"_a.none(false) = DEFAULT->getEncoding(),
             "equalityGroup"_a.none(false) = DEFAULT->getEqualityGroup(),
             "description"_a.none(false) = DEFAULT->getDescription(),
             "bitDepth"_a = DEFAULT->getBitDepth(),
             "isData"_a = DEFAULT->isData(),
             "allocation"_a = DEFAULT->getAllocation(), 
             "allocationVars"_a = getAllocationVarsStdVec(DEFAULT),
             "toReference"_a = DEFAULT->getTransform(COLORSPACE_DIR_TO_REFERENCE),
             "fromReference"_a = DEFAULT->getTransform(COLORSPACE_DIR_FROM_REFERENCE),
             "categories"_a = getCategoriesStdVec(DEFAULT),
             "interopID"_a = DEFAULT->getInteropID(),
             DOC(ColorSpace, Create, 2))

        .def("__deepcopy__", [](const ConstColorSpaceRcPtr & self, py::dict)
            {
                return self->createEditableCopy();
            },
            "memo"_a)

        .def("getName", &ColorSpace::getName, 
             DOC(ColorSpace, getName))
        .def("setName", &ColorSpace::setName, "name"_a.none(false), 
             DOC(ColorSpace, setName))

        // Aliases.
        .def("hasAlias", &ColorSpace::hasAlias, "alias"_a,
             DOC(ColorSpace, hasAlias))
        .def("addAlias", &ColorSpace::addAlias, "alias"_a.none(false), 
             DOC(ColorSpace, addAlias))
        .def("removeAlias", &ColorSpace::removeAlias, "alias"_a.none(false), 
             DOC(ColorSpace, removeAlias))
        .def("getAliases", [](ColorSpaceRcPtr & self) 
            { 
                return ColorSpaceAliasIterator(self); 
            })
        .def("clearAliases", &ColorSpace::clearAliases, 
             DOC(ColorSpace, clearAliases))

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
        .def("getInteropID", &ColorSpace::getInteropID,
             DOC(ColorSpace, getInteropID))
        .def("setInteropID", &ColorSpace::setInteropID, "interopID"_a,
             DOC(ColorSpace, setInteropID))
        .def("getInterchangeAttribute", &ColorSpace::getInterchangeAttribute, "attrName"_a,
             DOC(ColorSpace, getInterchangeAttribute))
        .def("setInterchangeAttribute", &ColorSpace::setInterchangeAttribute, "attrName"_a, "attrValue"_a,
            DOC(ColorSpace, setInterchangeAttribute))
        .def("getInterchangeAttributes", &ColorSpace:: getInterchangeAttributes,
            DOC(ColorSpace, getInterchangeAttributes))
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

    defRepr(clsColorSpace);

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

    clsColorSpaceAliasIterator
        .def("__len__", [](ColorSpaceAliasIterator & it) 
            { 
                return it.m_obj->getNumAliases(); 
            })
        .def("__getitem__", [](ColorSpaceAliasIterator & it, int i)
            { 
                it.checkIndex(i, (int)it.m_obj->getNumAliases());
                return it.m_obj->getAlias(i);
            })
        .def("__iter__", [](ColorSpaceAliasIterator & it) -> ColorSpaceAliasIterator &
            { 
                return it; 
            })
        .def("__next__", [](ColorSpaceAliasIterator & it)
            {
                int i = it.nextIndex((int)it.m_obj->getNumAliases());
                return it.m_obj->getAlias(i);
            });

}

} // namespace OCIO_NAMESPACE
