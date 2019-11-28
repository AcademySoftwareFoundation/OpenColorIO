// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"
#include "pystring/pystring.h"


namespace OCIO_NAMESPACE
{
ColorSpaceRcPtr ColorSpace::Create()
{
    return ColorSpaceRcPtr(new ColorSpace(), &deleter);
}

void ColorSpace::deleter(ColorSpace* c)
{
    delete c;
}

class ColorSpace::Impl
{
public:
    std::string m_name;
    std::string m_family;
    std::string m_equalityGroup;
    std::string m_description;

    BitDepth m_bitDepth;
    bool m_isData;

    Allocation m_allocation;
    std::vector<float> m_allocationVars;

    TransformRcPtr m_toRefTransform;
    TransformRcPtr m_fromRefTransform;

    bool m_toRefSpecified;
    bool m_fromRefSpecified;

    typedef StringVec Categories;
    Categories m_categories;

    Impl() :
        m_bitDepth(BIT_DEPTH_UNKNOWN),
        m_isData(false),
        m_allocation(ALLOCATION_UNIFORM),
        m_toRefSpecified(false),
        m_fromRefSpecified(false)
    { }

    Impl(const Impl &) = delete;

    ~Impl()
    { }

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_name = rhs.m_name;
            m_family = rhs.m_family;
            m_equalityGroup = rhs.m_equalityGroup;
            m_description = rhs.m_description;
            m_bitDepth = rhs.m_bitDepth;
            m_isData = rhs.m_isData;
            m_allocation = rhs.m_allocation;
            m_allocationVars = rhs.m_allocationVars;

            m_toRefTransform = rhs.m_toRefTransform?
                rhs.m_toRefTransform->createEditableCopy()
                : rhs.m_toRefTransform;

            m_fromRefTransform = rhs.m_fromRefTransform?
                rhs.m_fromRefTransform->createEditableCopy()
                : rhs.m_fromRefTransform;

            m_toRefSpecified = rhs.m_toRefSpecified;
            m_fromRefSpecified = rhs.m_fromRefSpecified;

            m_categories = rhs.m_categories;
        }
        return *this;
    }

    Categories::const_iterator findCategory(const char * category) const
    {
        if(!category || !*category) return m_categories.end();

        // NB: Categories are not case-sensitive and whitespace is stripped.
        const std::string ref(pystring::strip(pystring::lower(category)));

        for(auto itr = m_categories.begin(); itr!=m_categories.end(); ++itr)
        {
            if(pystring::strip(pystring::lower(*itr))==ref)
            {
                return itr;
            }
        }

        return m_categories.end();
    }

    void removeCategory(const char * category)
    {
        if(!category || !*category) return;

        // NB: Categories are not case-sensitive and whitespace is stripped.
        const std::string ref(pystring::strip(pystring::lower(category)));

        for(auto itr = m_categories.begin(); itr!=m_categories.end(); ++itr)
        {
            if(pystring::strip(pystring::lower(*itr))==ref)
            {
                m_categories.erase(itr);
                return;
            }
        }

        return;
    }
};


///////////////////////////////////////////////////////////////////////////



ColorSpace::ColorSpace()
: m_impl(new ColorSpace::Impl)
{
}

ColorSpace::~ColorSpace()
{
    delete m_impl;
    m_impl = NULL;
}

ColorSpaceRcPtr ColorSpace::createEditableCopy() const
{
    ColorSpaceRcPtr cs = ColorSpace::Create();
    *cs->m_impl = *m_impl;
    return cs;
}

const char * ColorSpace::getName() const noexcept
{
    return getImpl()->m_name.c_str();
}

void ColorSpace::setName(const char * name)
{
    getImpl()->m_name = name;
}
const char * ColorSpace::getFamily() const noexcept
{
    return getImpl()->m_family.c_str();
}

void ColorSpace::setFamily(const char * family)
{
    getImpl()->m_family = family;
}

const char * ColorSpace::getEqualityGroup() const noexcept
{
    return getImpl()->m_equalityGroup.c_str();
}

void ColorSpace::setEqualityGroup(const char * equalityGroup)
{
    getImpl()->m_equalityGroup = equalityGroup;
}

const char * ColorSpace::getDescription() const noexcept
{
    return getImpl()->m_description.c_str();
}

void ColorSpace::setDescription(const char * description)
{
    getImpl()->m_description = description;
}

BitDepth ColorSpace::getBitDepth() const noexcept
{
    return getImpl()->m_bitDepth;
}

void ColorSpace::setBitDepth(BitDepth bitDepth)
{
    getImpl()->m_bitDepth = bitDepth;
}

bool ColorSpace::hasCategory(const char * category) const
{
    return getImpl()->findCategory(category)
        != getImpl()->m_categories.end();
}

void ColorSpace::addCategory(const char * category)
{
    if(getImpl()->findCategory(category) 
        == getImpl()->m_categories.end())
    {
        getImpl()->m_categories.push_back(pystring::strip(category));
    }
}

void ColorSpace::removeCategory(const char * category)
{
    getImpl()->removeCategory(category);
}

int ColorSpace::getNumCategories() const
{
    return static_cast<int>(getImpl()->m_categories.size());
}

const char * ColorSpace::getCategory(int index) const
{
    if(index<0 || index>=(int)getImpl()->m_categories.size()) return nullptr;

    return getImpl()->m_categories[index].c_str();
}

void ColorSpace::clearCategories()
{
    getImpl()->m_categories.clear();
}

bool ColorSpace::isData() const
{
    return getImpl()->m_isData;
}

void ColorSpace::setIsData(bool val)
{
    getImpl()->m_isData = val;
}

Allocation ColorSpace::getAllocation() const
{
    return getImpl()->m_allocation;
}

void ColorSpace::setAllocation(Allocation allocation)
{
    getImpl()->m_allocation = allocation;
}

int ColorSpace::getAllocationNumVars() const
{
    return static_cast<int>(getImpl()->m_allocationVars.size());
}

void ColorSpace::getAllocationVars(float * vars) const
{
    if(!getImpl()->m_allocationVars.empty())
    {
        memcpy(vars,
            &getImpl()->m_allocationVars[0],
            getImpl()->m_allocationVars.size()*sizeof(float));
    }
}

void ColorSpace::setAllocationVars(int numvars, const float * vars)
{
    getImpl()->m_allocationVars.resize(numvars);

    if(!getImpl()->m_allocationVars.empty())
    {
        memcpy(&getImpl()->m_allocationVars[0],
            vars,
            numvars*sizeof(float));
    }
}

ConstTransformRcPtr ColorSpace::getTransform(ColorSpaceDirection dir) const
{
    if(dir == COLORSPACE_DIR_TO_REFERENCE)
        return getImpl()->m_toRefTransform;
    else if(dir == COLORSPACE_DIR_FROM_REFERENCE)
        return getImpl()->m_fromRefTransform;

    throw Exception("Unspecified ColorSpaceDirection");
}

void ColorSpace::setTransform(const ConstTransformRcPtr & transform,
                                ColorSpaceDirection dir)
{
    TransformRcPtr transformCopy;
    if(transform) transformCopy = transform->createEditableCopy();

    if(dir == COLORSPACE_DIR_TO_REFERENCE)
        getImpl()->m_toRefTransform = transformCopy;
    else if(dir == COLORSPACE_DIR_FROM_REFERENCE)
        getImpl()->m_fromRefTransform = transformCopy;
    else
        throw Exception("Unspecified ColorSpaceDirection");
}

std::ostream& operator<< (std::ostream& os, const ColorSpace& cs)
{
    int numVars(cs.getAllocationNumVars());
    std::vector<float> vars(numVars);
    cs.getAllocationVars(&vars[0]);

    os << "<ColorSpace ";
    os << "name=" << cs.getName() << ", ";
    os << "family=" << cs.getFamily() << ", ";
    os << "equalityGroup=" << cs.getEqualityGroup() << ", ";
    os << "bitDepth=" << BitDepthToString(cs.getBitDepth()) << ", ";
    os << "isData=" << BoolToString(cs.isData());
    if (numVars)
    {
        os << ", allocation=" << AllocationToString(cs.getAllocation()) << ", ";
        os << "vars=" << vars[0];
        for (int i = 1; i < numVars; ++i)
        {
            os << " " << vars[i];
        }
    }
    os << ">";

    if(cs.getTransform(COLORSPACE_DIR_TO_REFERENCE))
    {
        os << "\n    " << cs.getName() << " --> Reference";
        os << "\n\t" << *cs.getTransform(COLORSPACE_DIR_TO_REFERENCE);
    }

    if(cs.getTransform(COLORSPACE_DIR_FROM_REFERENCE))
    {
        os << "\n    Reference --> " << cs.getName();
        os << "\n\t" << *cs.getTransform(COLORSPACE_DIR_FROM_REFERENCE);
    }
    return os;
}
} // namespace OCIO_NAMESPACE

