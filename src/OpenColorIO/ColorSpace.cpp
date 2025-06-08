// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "TokensManager.h"
#include "Platform.h"
#include "PrivateTypes.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

class ColorSpace::Impl
{
public:
    std::string m_name;
    std::string m_family;
    std::string m_equalityGroup;
    std::string m_description;
    std::string m_encoding;
    std::string m_interopID;
    std::string m_AMFTransformIDs;
    std::string m_ICCProfileName;
    StringUtils::StringVec m_aliases;

    BitDepth m_bitDepth{ BIT_DEPTH_UNKNOWN };
    bool m_isData{ false };

    ReferenceSpaceType m_referenceSpaceType{ REFERENCE_SPACE_SCENE };

    Allocation m_allocation{ ALLOCATION_UNIFORM };
    std::vector<float> m_allocationVars;

    TransformRcPtr m_toRefTransform;
    TransformRcPtr m_fromRefTransform;

    bool m_toRefSpecified{ false };
    bool m_fromRefSpecified{ false };

    TokensManager m_categories;

    Impl() = delete;
    explicit Impl(ReferenceSpaceType referenceSpace)
        : m_referenceSpaceType(referenceSpace)
    {
    }

    Impl(const Impl &) = delete;

    ~Impl() = default;

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_name = rhs.m_name;
            m_aliases = rhs.m_aliases;
            m_family = rhs.m_family;
            m_equalityGroup = rhs.m_equalityGroup;
            m_description = rhs.m_description;
            m_encoding = rhs.m_encoding;
            m_interopID = rhs.m_interopID;
            m_AMFTransformIDs = rhs.m_AMFTransformIDs;
            m_ICCProfileName = rhs.m_ICCProfileName;
            m_bitDepth = rhs.m_bitDepth;
            m_isData = rhs.m_isData;
            m_referenceSpaceType = rhs.m_referenceSpaceType;
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

};


///////////////////////////////////////////////////////////////////////////

ColorSpaceRcPtr ColorSpace::Create()
{
    return ColorSpaceRcPtr(new ColorSpace(REFERENCE_SPACE_SCENE), &deleter);
}

void ColorSpace::deleter(ColorSpace* c)
{
    delete c;
}

ColorSpaceRcPtr ColorSpace::Create(ReferenceSpaceType referenceSpace)
{
    auto cs = ColorSpaceRcPtr(new ColorSpace(referenceSpace), &deleter);
    return cs;
}

ColorSpace::ColorSpace(ReferenceSpaceType referenceSpace)
: m_impl(new ColorSpace::Impl(referenceSpace))
{
}

ColorSpace::~ColorSpace()
{
    delete m_impl;
    m_impl = nullptr;
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

void ColorSpace::setName(const char * name) noexcept
{
    getImpl()->m_name = name ? name : "";
    // Name can no longer be an alias.
    StringUtils::Remove(getImpl()->m_aliases, getImpl()->m_name);
}

size_t ColorSpace::getNumAliases() const noexcept
{
    return getImpl()->m_aliases.size();
}

const char * ColorSpace::getAlias(size_t idx) const noexcept
{
    if (idx < getImpl()->m_aliases.size())
    {
        return getImpl()->m_aliases[idx].c_str();
    }
    return "";
}

bool ColorSpace::hasAlias(const char * alias) const noexcept
{
    for (size_t idx = 0; idx < getImpl()->m_aliases.size(); ++idx)
    {
        if (0 == Platform::Strcasecmp(getImpl()->m_aliases[idx].c_str(), alias))
        {
            return true;
        }
    }
    return false;
}

void ColorSpace::addAlias(const char * alias) noexcept
{
    if (alias && *alias)
    {
        if (!StringUtils::Compare(alias, getImpl()->m_name))
        {
            if (!StringUtils::Contain(getImpl()->m_aliases, alias))
            {
                getImpl()->m_aliases.push_back(alias);
            }
        }
    }
}

void ColorSpace::removeAlias(const char * name) noexcept
{
    if (name && *name)
    {
        const std::string alias{ name };
        StringUtils::Remove(getImpl()->m_aliases, alias);
    }
}

void ColorSpace::clearAliases() noexcept
{
    getImpl()->m_aliases.clear();
}

const char * ColorSpace::getFamily() const noexcept
{
    return getImpl()->m_family.c_str();
}

void ColorSpace::setFamily(const char * family)
{
    getImpl()->m_family = family ? family : "";
}

const char * ColorSpace::getEqualityGroup() const noexcept
{
    return getImpl()->m_equalityGroup.c_str();
}

void ColorSpace::setEqualityGroup(const char * equalityGroup)
{
    getImpl()->m_equalityGroup = equalityGroup ? equalityGroup : "";
}

const char * ColorSpace::getDescription() const noexcept
{
    return getImpl()->m_description.c_str();
}

void ColorSpace::setDescription(const char * description)
{
    getImpl()->m_description = description ? description : "";
}

const char * ColorSpace::getInteropID() const noexcept
{
    return getImpl()->m_interopID.c_str();
}

void ColorSpace::setInteropID(const char * interopID)
{
    getImpl()->m_interopID = interopID ? interopID : "";
}

const char * ColorSpace::getAMFTransformIDs() const noexcept
{
    return getImpl()->m_AMFTransformIDs.c_str();
}

void ColorSpace::setAMFTransformIDs(const char * amfTransformIDs)
{
    getImpl()->m_AMFTransformIDs = amfTransformIDs ? amfTransformIDs : "";
}

const char * ColorSpace::getICCProfileName() const noexcept
{
    return getImpl()->m_ICCProfileName.c_str();
}

void ColorSpace::setICCProfileName(const char * iccProfileName)
{
    getImpl()->m_ICCProfileName = iccProfileName ? iccProfileName : "";
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
    return getImpl()->m_categories.hasToken(category);
}

void ColorSpace::addCategory(const char * category)
{
    getImpl()->m_categories.addToken(category);
}

void ColorSpace::removeCategory(const char * category)
{
    getImpl()->m_categories.removeToken(category);
}

int ColorSpace::getNumCategories() const
{
    return getImpl()->m_categories.getNumTokens();
}

const char * ColorSpace::getCategory(int index) const
{
    return getImpl()->m_categories.getToken(index);
}

void ColorSpace::clearCategories()
{
    getImpl()->m_categories.clearTokens();
}

const char * ColorSpace::getEncoding() const noexcept
{
    return getImpl()->m_encoding.c_str();
}

void ColorSpace::setEncoding(const char * encoding)
{
    getImpl()->m_encoding = encoding ? encoding : "";
}

bool ColorSpace::isData() const noexcept
{
    return getImpl()->m_isData;
}

void ColorSpace::setIsData(bool val) noexcept
{
    getImpl()->m_isData = val;
}

ReferenceSpaceType ColorSpace::getReferenceSpaceType() const noexcept
{
    return getImpl()->m_referenceSpaceType;
}

Allocation ColorSpace::getAllocation() const noexcept
{
    return getImpl()->m_allocation;
}

void ColorSpace::setAllocation(Allocation allocation) noexcept
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

ConstTransformRcPtr ColorSpace::getTransform(ColorSpaceDirection dir) const noexcept
{
    switch (dir)
    {
    case COLORSPACE_DIR_TO_REFERENCE:
        return getImpl()->m_toRefTransform;
    case COLORSPACE_DIR_FROM_REFERENCE:
        return getImpl()->m_fromRefTransform;
    }
    return ConstTransformRcPtr();
}

void ColorSpace::setTransform(const ConstTransformRcPtr & transform,
                                ColorSpaceDirection dir)
{
    TransformRcPtr transformCopy;
    if(transform) transformCopy = transform->createEditableCopy();

    switch (dir)
    {
    case COLORSPACE_DIR_TO_REFERENCE:
        getImpl()->m_toRefTransform = transformCopy;
        break;
    case COLORSPACE_DIR_FROM_REFERENCE:
        getImpl()->m_fromRefTransform = transformCopy;
        break;
    }
}

std::ostream & operator<< (std::ostream & os, const ColorSpace & cs)
{
    const int numVars(cs.getAllocationNumVars());
    std::vector<float> vars(numVars);
    if (numVars > 0)
    {
        cs.getAllocationVars(&vars[0]);
    }

    os << "<ColorSpace referenceSpaceType=";

    const auto refType = cs.getReferenceSpaceType();
    switch (refType)
    {
    case REFERENCE_SPACE_SCENE:
        os << "scene, ";
        break;
    case REFERENCE_SPACE_DISPLAY:
        os << "display, ";
        break;
    }
    os << "name=" << cs.getName() << ", ";
    const auto numAliases = cs.getNumAliases();
    if (numAliases == 1)
    {
        os << "alias= " << cs.getAlias(0) << ", ";
    }
    else if (numAliases > 1)
    {
        os << "aliases=[" << cs.getAlias(0);
        for (size_t aidx = 1; aidx < numAliases; ++aidx)
        {
            os << ", " << cs.getAlias(aidx);
        }
        os << "], ";
    }

    std::string str;
    
    str = cs.getInteropID();
    if (!str.empty())
    {
        os << "interop_id=" << str << ", ";
    }
    str = cs.getFamily();
    if (!str.empty())
    {
        os << "family=" << str << ", ";
    }
    str = cs.getEqualityGroup();
    if (!str.empty())
    {
        os << "equalityGroup=" << str << ", ";
    }
    const auto bd = cs.getBitDepth();
    if (bd != BIT_DEPTH_UNKNOWN)
    {
        os << "bitDepth=" << BitDepthToString(bd) << ", ";
    }
    os << "isData=" << BoolToString(cs.isData());
    if (numVars > 0)
    {
        os << ", allocation=" << AllocationToString(cs.getAllocation()) << ", ";
        os << "vars=" << vars[0];
        for (int i = 1; i < numVars; ++i)
        {
            os << " " << vars[i];
        }
    }
    if (cs.getNumCategories())
    {
        StringUtils::StringVec categories;
        for (int i = 0; i < cs.getNumCategories(); ++i)
        {
            categories.push_back(cs.getCategory(i));
        }
        os << ", categories=" << StringUtils::Join(categories, ',');
    }
    str = cs.getEncoding();
    if (!str.empty())
    {
        os << ", encoding=" << str;
    }
    str = cs.getDescription();
    if (!str.empty())
    {
        os << ", description=" << str;
    }
    str = cs.getAMFTransformIDs();
    if (!str.empty())
    {
        os << ", amf_transform_ids=" << str;
    }
    str = cs.getICCProfileName();
    if (!str.empty())
    {
        os << ", icc_profile_name=" << str;
    }
    if(cs.getTransform(COLORSPACE_DIR_TO_REFERENCE))
    {
        os << ",\n    " << cs.getName() << " --> Reference";
        os << "\n        " << *cs.getTransform(COLORSPACE_DIR_TO_REFERENCE);
    }
    if(cs.getTransform(COLORSPACE_DIR_FROM_REFERENCE))
    {
        os << ",\n    Reference --> " << cs.getName();
        os << "\n        " << *cs.getTransform(COLORSPACE_DIR_FROM_REFERENCE);
    }
    os << ">";
    return os;
}
} // namespace OCIO_NAMESPACE

