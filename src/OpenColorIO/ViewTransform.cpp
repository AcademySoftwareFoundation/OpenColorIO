// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <ostream>

#include <OpenColorIO/OpenColorIO.h>

#include "TokensManager.h"

namespace
{
const std::array<const std::string, 1> knownInterchangeNames = {
    "amf_transform_ids" };
}

namespace OCIO_NAMESPACE
{

class ViewTransform::Impl
{
public:
    std::string m_name;
    std::string m_family;
    std::string m_description;
    ReferenceSpaceType m_referenceSpaceType{ REFERENCE_SPACE_SCENE };
    std::map<std::string, std::string> m_interchangeAttribs;

    TransformRcPtr m_toRefTransform;
    TransformRcPtr m_fromRefTransform;

    TokensManager m_categories;

    Impl() = delete;
    explicit Impl(ReferenceSpaceType referenceSpace)
        : m_referenceSpaceType(referenceSpace)
    {
    }

    Impl(const Impl &) = delete;
    ~Impl() = default;

    Impl & operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_name        = rhs.m_name;
            m_family      = rhs.m_family;
            m_description = rhs.m_description;

            m_referenceSpaceType = rhs.m_referenceSpaceType;
            m_interchangeAttribs = rhs.m_interchangeAttribs;

            m_toRefTransform = rhs.m_toRefTransform ? rhs.m_toRefTransform->createEditableCopy() :
                                                      rhs.m_toRefTransform;

            m_fromRefTransform = rhs.m_fromRefTransform ?
                                 rhs.m_fromRefTransform->createEditableCopy() :
                                 rhs.m_fromRefTransform;

            m_categories = rhs.m_categories;
        }
        return *this;
    }
};


ViewTransformRcPtr ViewTransform::Create(ReferenceSpaceType referenceSpace)
{
    return ViewTransformRcPtr(new ViewTransform(referenceSpace), &deleter);
}

void ViewTransform::deleter(ViewTransform * v)
{
    delete v;
}

ViewTransformRcPtr ViewTransform::createEditableCopy() const
{
    ViewTransformRcPtr cs = ViewTransform::Create(getImpl()->m_referenceSpaceType);
    *cs->m_impl = *m_impl;
    return cs;
}

ViewTransform::ViewTransform(ReferenceSpaceType referenceSpace)
    : m_impl(new ViewTransform::Impl(referenceSpace))
{
}

ViewTransform::~ViewTransform()
{
    delete m_impl;
    m_impl = NULL;
}

const char * ViewTransform::getName() const noexcept
{
    return getImpl()->m_name.c_str();
}

void ViewTransform::setName(const char * name) noexcept
{
    getImpl()->m_name = name ? name : "";
}

const char * ViewTransform::getFamily() const noexcept
{
    return getImpl()->m_family.c_str();
}

void ViewTransform::setFamily(const char * family)
{
    getImpl()->m_family = family ? family : "";
}

const char * ViewTransform::getDescription() const noexcept
{
    return getImpl()->m_description.c_str();
}

void ViewTransform::setDescription(const char * description)
{
    getImpl()->m_description = description ? description : "";
}

const char * ViewTransform::getInterchangeAttribute(const char* attrName) const
{
    std::string name = attrName ? attrName : "";

    for (auto& key : knownInterchangeNames)
    {
        // do case-insensitive comparison.
        if (StringUtils::Compare(key, name))
        {
            auto it = m_impl->m_interchangeAttribs.find(key);
            if (it != m_impl->m_interchangeAttribs.end())
            {
                return it->second.c_str();
            }
            return "";
        }
    }

    std::ostringstream oss;
    oss << "Unknown attribute name '" << name << "'.";
    throw Exception(oss.str().c_str());
}

void ViewTransform::setInterchangeAttribute(const char* attrName, const char* value)
{
    std::string name = attrName ? attrName : "";

    for (auto& key : knownInterchangeNames)
    {
        // Do case-insensitive comparison.
        if (StringUtils::Compare(key, name))
        {
            // Use key instead of name for storing in correct capitalization.
            if (!value || !*value)
            {
                m_impl->m_interchangeAttribs.erase(key);
            } 
            else
            {
                m_impl->m_interchangeAttribs[key] = value;
            }
            return;
        }
    }

    std::ostringstream oss;
    oss << "Unknown attribute name '" << name << "'.";
    throw Exception(oss.str().c_str());
}

std::map<std::string, std::string> ViewTransform::getInterchangeAttributes() const noexcept
{
    return m_impl->m_interchangeAttribs;
}


bool ViewTransform::hasCategory(const char * category) const
{
    return getImpl()->m_categories.hasToken(category);
}

void ViewTransform::addCategory(const char * category)
{
    getImpl()->m_categories.addToken(category);
}
void ViewTransform::removeCategory(const char * category)
{
    getImpl()->m_categories.removeToken(category);
}

int ViewTransform::getNumCategories() const
{
    return getImpl()->m_categories.getNumTokens();
}

const char * ViewTransform::getCategory(int index) const
{
    return getImpl()->m_categories.getToken(index);
}

void ViewTransform::clearCategories()
{
    getImpl()->m_categories.clearTokens();
}


ReferenceSpaceType ViewTransform::getReferenceSpaceType() const noexcept
{
    return getImpl()->m_referenceSpaceType;
}

ConstTransformRcPtr ViewTransform::getTransform(ViewTransformDirection dir) const noexcept
{
    switch (dir)
    {
    case VIEWTRANSFORM_DIR_TO_REFERENCE:
        return getImpl()->m_toRefTransform;
    case VIEWTRANSFORM_DIR_FROM_REFERENCE:
        return getImpl()->m_fromRefTransform;
    }
    return ConstTransformRcPtr();
}

void ViewTransform::setTransform(const ConstTransformRcPtr & transform, ViewTransformDirection dir)
{
    TransformRcPtr transformCopy;
    if (transform)
    {
        transformCopy = transform->createEditableCopy();
    }

    switch (dir)
    {
    case VIEWTRANSFORM_DIR_TO_REFERENCE:
        getImpl()->m_toRefTransform = transformCopy; 
        break;
    case VIEWTRANSFORM_DIR_FROM_REFERENCE:
        getImpl()->m_fromRefTransform = transformCopy; 
        break;
    }
}


namespace
{
static constexpr char REFERENCE_SPACE_SCENE_STR[]{ "scene" };
static constexpr char REFERENCE_SPACE_DISPLAY_STR[]{ "display" };

const char * ReferenceSpaceTypeToString(ReferenceSpaceType reference)
{
    switch (reference)
    {
    case REFERENCE_SPACE_SCENE:   return REFERENCE_SPACE_SCENE_STR;
    case REFERENCE_SPACE_DISPLAY: return REFERENCE_SPACE_DISPLAY_STR;
    }

    // Default type is meaningless.
    throw Exception("Unknown reference type");
}
}

std::ostream & operator<< (std::ostream & os, const ViewTransform & vt)
{
    os << "<ViewTransform ";
    os << "name=" << vt.getName() << ", ";
    os << "family=" << vt.getFamily() << ", ";
    os << "referenceSpaceType=" << ReferenceSpaceTypeToString(vt.getReferenceSpaceType());
    const std::string desc{ vt.getDescription() };
    if (!desc.empty())
    {
        os << ", description=" << desc;
    }

    for (const auto& attr : vt.getInterchangeAttributes())
    {
        os << ", " << attr.first << "=" << attr.second;
    }

    if (vt.getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE))
    {
        os << ",\n    " << vt.getName() << " --> Reference";
        os << "\n        " << *vt.getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE);
    }
    if (vt.getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE))
    {
        os << ",\n    Reference --> " << vt.getName();
        os << "\n        " << *vt.getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE);
    }
    os << ">";
    return os;
}

} // namespace OCIO_NAMESPACE

