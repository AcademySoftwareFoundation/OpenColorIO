// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstring>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ContextVariableUtils.h"


namespace OCIO_NAMESPACE
{
LookRcPtr Look::Create()
{
    return LookRcPtr(new Look(), &deleter);
}

void Look::deleter(Look* c)
{
    delete c;
}

class Look::Impl
{
public:
    std::string m_name;
    std::string m_processSpace;
    std::string m_description;
    TransformRcPtr m_transform;
    TransformRcPtr m_inverseTransform;

    Impl()
    { }

    Impl(const Impl &) = delete;

    ~Impl()
    { }

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_name = rhs.m_name;
            m_processSpace = rhs.m_processSpace;
            m_description = rhs.m_description;

            m_transform = rhs.m_transform?
                rhs.m_transform->createEditableCopy() : rhs.m_transform;

            m_inverseTransform = rhs.m_inverseTransform?
                rhs.m_inverseTransform->createEditableCopy()
                : rhs.m_inverseTransform;
        }
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////

Look::Look()
: m_impl(new Look::Impl)
{
}

Look::~Look()
{
    delete m_impl;
    m_impl = NULL;
}

LookRcPtr Look::createEditableCopy() const
{
    LookRcPtr cs = Look::Create();
    *cs->m_impl = *m_impl;
    return cs;
}

const char * Look::getName() const
{
    return getImpl()->m_name.c_str();
}

void Look::setName(const char * name)
{
    getImpl()->m_name = name;
}

const char * Look::getProcessSpace() const
{
    return getImpl()->m_processSpace.c_str();
}

void Look::setProcessSpace(const char * processSpace)
{
    getImpl()->m_processSpace = processSpace;
}

ConstTransformRcPtr Look::getTransform() const
{
    return getImpl()->m_transform;
}

void Look::setTransform(const ConstTransformRcPtr & transform)
{
    getImpl()->m_transform = transform->createEditableCopy();
}

ConstTransformRcPtr Look::getInverseTransform() const
{
    return getImpl()->m_inverseTransform;
}

void Look::setInverseTransform(const ConstTransformRcPtr & transform)
{
    getImpl()->m_inverseTransform = transform->createEditableCopy();
}

const char * Look::getDescription() const
{
    return getImpl()->m_description.c_str();
}

void Look::setDescription(const char * description)
{
    getImpl()->m_description = description;
}

bool CollectContextVariables(const Config & config,
                             const Context & context,
                             TransformDirection direction,
                             const Look & look,
                             ContextRcPtr & usedContext)
{
    bool foundContextVars = false;

    switch (direction)
    {
    case TRANSFORM_DIR_FORWARD:
    {
        ConstTransformRcPtr tr = look.getTransform();
        if (tr)
        {
            if (CollectContextVariables(config, context, tr, usedContext))
            {
                foundContextVars = true;
            }
        }
        else
        {
            tr = look.getInverseTransform();
            if (tr && CollectContextVariables(config, context, tr, usedContext))
            {
                foundContextVars = true;
            }
        }
        break;
    }
    case TRANSFORM_DIR_INVERSE:
    {
        ConstTransformRcPtr tr = look.getInverseTransform();
        if (tr)
        {
            if (CollectContextVariables(config, context, tr, usedContext))
            {
                foundContextVars = true;
            }
        }
        else
        {
            tr = look.getTransform();
            if (tr && CollectContextVariables(config, context, tr, usedContext))
            {
                foundContextVars = true;
            }
        }
        break;
    }
    }

    const char * ps = look.getProcessSpace();
    if (ps)
    {
        ConstColorSpaceRcPtr cs = config.getColorSpace(ps);
        if (cs)
        {
            ConstTransformRcPtr to = cs->getTransform(COLORSPACE_DIR_TO_REFERENCE);
            if (to && CollectContextVariables(config, context, to, usedContext))
            {
                foundContextVars = true;
            }

            ConstTransformRcPtr from = cs->getTransform(COLORSPACE_DIR_FROM_REFERENCE);
            if (from && CollectContextVariables(config, context, from, usedContext))
            {
                foundContextVars = true;
            }
        }
    }

    return foundContextVars;
}

std::ostream& operator<< (std::ostream& os, const Look& look)
{
    os << "<Look";
    os << " name=" << look.getName();
    os << ", processSpace=" << look.getProcessSpace();

    std::string desc{ look.getDescription() };
    if (!desc.empty())
    {
        os << ", description=" << desc;
    }

    if(look.getTransform())
    {
        os << ",\n    transform=";
        os << "\n        " << *look.getTransform();
    }

    if(look.getInverseTransform())
    {
        os << ",\n    inverseTransform=";
        os << "\n        " << *look.getInverseTransform();
    }

    os << ">";

    return os;
}
} // namespace OCIO_NAMESPACE
