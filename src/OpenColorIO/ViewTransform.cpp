// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <ostream>

#include <OpenColorIO/OpenColorIO.h>

#include "Categories.h"

namespace OCIO_NAMESPACE
{

class ViewTransform::Impl : public CategoriesManager
{
public:
    std::string m_name;
    std::string m_family;
    std::string m_description;
    ReferenceSpaceType m_referenceSpaceType{ REFERENCE_SPACE_SCENE };

    TransformRcPtr m_toRefTransform;
    TransformRcPtr m_fromRefTransform;

    Impl() = delete;
    explicit Impl(ReferenceSpaceType referenceSpace)
        : CategoriesManager()
        , m_referenceSpaceType(referenceSpace)
    {
    }

    Impl(const Impl &) = delete;
    ~Impl() = default;

    Impl & operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            *dynamic_cast<CategoriesManager*>(this)
                = *dynamic_cast<const CategoriesManager*>(&rhs);
 
            m_name        = rhs.m_name;
            m_family      = rhs.m_family;
            m_description = rhs.m_description;

            m_referenceSpaceType = rhs.m_referenceSpaceType;

            m_toRefTransform = rhs.m_toRefTransform ? rhs.m_toRefTransform->createEditableCopy() :
                                                      rhs.m_toRefTransform;

            m_fromRefTransform = rhs.m_fromRefTransform ?
                                 rhs.m_fromRefTransform->createEditableCopy() :
                                 rhs.m_fromRefTransform;
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
    getImpl()->m_name = name;
}

const char * ViewTransform::getFamily() const noexcept
{
    return getImpl()->m_family.c_str();
}

void ViewTransform::setFamily(const char * family)
{
    getImpl()->m_family = family;
}

const char * ViewTransform::getDescription() const noexcept
{
    return getImpl()->m_description.c_str();
}

void ViewTransform::setDescription(const char * description)
{
    getImpl()->m_description = description;
}

bool ViewTransform::hasCategory(const char * category) const
{
    return getImpl()->hasCategory(category);
}

void ViewTransform::addCategory(const char * category)
{
    getImpl()->addCategory(category);
}
void ViewTransform::removeCategory(const char * category)
{
    getImpl()->removeCategory(category);
}

int ViewTransform::getNumCategories() const
{
    return getImpl()->getNumCategories();
}

const char * ViewTransform::getCategory(int index) const
{
    return getImpl()->getCategory(index);
}

void ViewTransform::clearCategories()
{
    getImpl()->clearCategories();
}


ReferenceSpaceType ViewTransform::getReferenceSpaceType() const noexcept
{
    return getImpl()->m_referenceSpaceType;
}

ConstTransformRcPtr ViewTransform::getTransform(ViewTransformDirection dir) const
{
    if (dir == VIEWTRANSFORM_DIR_TO_REFERENCE)
    {
        return getImpl()->m_toRefTransform;
    }
    else if (dir == VIEWTRANSFORM_DIR_FROM_REFERENCE)
    {
        return getImpl()->m_fromRefTransform;
    }

    throw Exception("View transform: Unspecified ViewTransformDirection.");
}

void ViewTransform::setTransform(const ConstTransformRcPtr & transform, ViewTransformDirection dir)
{
    TransformRcPtr transformCopy;
    if (transform)
    {
        transformCopy = transform->createEditableCopy();
    }

    if (dir == VIEWTRANSFORM_DIR_TO_REFERENCE)
    {
        getImpl()->m_toRefTransform = transformCopy;
    }
    else if (dir == VIEWTRANSFORM_DIR_FROM_REFERENCE)
    {
        getImpl()->m_fromRefTransform = transformCopy;
    }
    else
    {
        throw Exception("View transform: Unspecified ViewTransformDirection.");
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
    os << ">";

    if (vt.getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE))
    {
        os << "\n    " << vt.getName() << " --> Reference";
        os << "\n\t" << *vt.getTransform(VIEWTRANSFORM_DIR_TO_REFERENCE);
    }

    if (vt.getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE))
    {
        os << "\n    Reference --> " << vt.getName();
        os << "\n\t" << *vt.getTransform(VIEWTRANSFORM_DIR_FROM_REFERENCE);
    }
    return os;
}

} // namespace OCIO_NAMESPACE

