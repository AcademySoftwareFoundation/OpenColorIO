// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "NamedTransform.h"

namespace OCIO_NAMESPACE
{
NamedTransformRcPtr NamedTransform::Create()
{
    return NamedTransformRcPtr(new NamedTransformImpl(), &NamedTransformImpl::Deleter);
}

void NamedTransformImpl::Deleter(NamedTransform * t)
{
    delete static_cast<NamedTransformImpl *>(t);
}

NamedTransformRcPtr NamedTransformImpl::createEditableCopy() const
{
    auto copy = std::make_shared<NamedTransformImpl>();
    copy->m_name = m_name;
    copy->m_aliases = m_aliases;
    copy->m_description = m_description;
    copy->m_family = m_family;
    copy->m_categories = m_categories;
    copy->m_encoding = m_encoding;
    if (m_forwardTransform)
    {
        copy->m_forwardTransform = m_forwardTransform->createEditableCopy();
    }
    if (m_inverseTransform)
    {
        copy->m_inverseTransform = m_inverseTransform->createEditableCopy();
    }
    return copy;
}

const char * NamedTransformImpl::getName() const noexcept
{
    return m_name.c_str();
}

void NamedTransformImpl::setName(const char * name) noexcept
{
    m_name = name ? name : "";
    // Name can no longer be an alias.
    StringUtils::Remove(m_aliases, m_name);
}

size_t NamedTransformImpl::getNumAliases() const noexcept
{
    return m_aliases.size();
}

const char * NamedTransformImpl::getAlias(size_t idx) const noexcept
{
    if (idx < m_aliases.size())
    {
        return m_aliases[idx].c_str();
    }
    return "";
}

void NamedTransformImpl::addAlias(const char * alias) noexcept
{
    if (alias && *alias)
    {
        if (!StringUtils::Compare(alias, m_name))
        {
            if (!StringUtils::Contain(m_aliases, alias))
            {
                m_aliases.push_back(alias);
            }
        }
    }
}

void NamedTransformImpl::removeAlias(const char * name) noexcept
{
    if (name && *name)
    {
        const std::string alias{ name };
        StringUtils::Remove(m_aliases, alias);
    }
}

void NamedTransformImpl::clearAliases() noexcept
{
    m_aliases.clear();
}

const char * NamedTransformImpl::getFamily() const noexcept
{
    return m_family.c_str();
}

void NamedTransformImpl::setFamily(const char * family) noexcept
{
    m_family = family ? family : "";
}

const char * NamedTransformImpl::getDescription() const noexcept
{
    return m_description.c_str();
}

void NamedTransformImpl::setDescription(const char * description) noexcept
{
    m_description = description ? description : "";
}

bool NamedTransformImpl::hasCategory(const char * category) const noexcept
{
    return m_categories.hasToken(category);
}

void NamedTransformImpl::addCategory(const char * category) noexcept
{
    m_categories.addToken(category);
}

void NamedTransformImpl::removeCategory(const char * category) noexcept
{
    m_categories.removeToken(category);
}

int NamedTransformImpl::getNumCategories() const noexcept
{
    return m_categories.getNumTokens();
}

const char * NamedTransformImpl::getCategory(int index) const noexcept
{
    return m_categories.getToken(index);
}

void NamedTransformImpl::clearCategories() noexcept
{
    m_categories.clearTokens();
}

const char * NamedTransformImpl::getEncoding() const noexcept
{
    return m_encoding.c_str();
}

void NamedTransformImpl::setEncoding(const char * encoding) noexcept
{
    m_encoding = encoding ? encoding : "";
}

ConstTransformRcPtr NamedTransformImpl::getTransform(TransformDirection dir) const
{
    if (dir == TRANSFORM_DIR_FORWARD)
    {
        return m_forwardTransform;
    }
    else if (dir == TRANSFORM_DIR_INVERSE)
    {
        return m_inverseTransform;
    }
    throw Exception("Named transform: Unspecified TransformDirection.");
}

ConstTransformRcPtr NamedTransformImpl::GetTransform(const ConstNamedTransformRcPtr & nt,
                                                     TransformDirection dir)
{
    if (nt)
    {
        const auto forwardTransform = nt->getTransform(TRANSFORM_DIR_FORWARD);
        const auto inverseTransform = nt->getTransform(TRANSFORM_DIR_INVERSE);
        if (dir == TRANSFORM_DIR_FORWARD)
        {
            // If forward does not exist, inverse the inverse transform.
            if (!forwardTransform)
            {
                auto transform = inverseTransform->createEditableCopy();
                const auto dir = GetInverseTransformDirection(transform->getDirection());
                transform->setDirection(dir);
                return transform;
            }
            else
            {
                return forwardTransform;
            }
        }
        else if (dir == TRANSFORM_DIR_INVERSE)
        {
            if (!inverseTransform)
            {
                // Inverse the forward transform.
                auto transform = forwardTransform->createEditableCopy();
                const auto dir = GetInverseTransformDirection(transform->getDirection());
                transform->setDirection(dir);
                return transform;
            }
            else
            {
                return inverseTransform;
            }
        }
    }
    throw Exception("Named transform: Unspecified TransformDirection.");
}

void NamedTransformImpl::setTransform(const ConstTransformRcPtr & transform, TransformDirection dir)
{
    if (dir == TRANSFORM_DIR_FORWARD)
    {
        if (!transform)
        {
            m_forwardTransform = ConstTransformRcPtr();
        }
        else
        {
            m_forwardTransform = transform->createEditableCopy();
        }
    }
    else if (dir == TRANSFORM_DIR_INVERSE)
    {
        if (!transform)
        {
            m_inverseTransform = ConstTransformRcPtr();
        }
        else
        {
            m_inverseTransform = transform->createEditableCopy();
        }
    }
    else
    {
        throw Exception("Named transform: Unspecified TransformDirection.");
    }
}

std::ostream & operator<< (std::ostream & os, const NamedTransform & t)
{
    os << "<NamedTransform ";
    const std::string strName{ t.getName() };
    os << "name=" << strName;
    const auto numAliases = t.getNumAliases();
    if (numAliases == 1)
    {
        os << "alias= " << t.getAlias(0) << ", ";
    }
    else if (numAliases > 1)
    {
        os << "aliases=[" << t.getAlias(0);
        for (size_t aidx = 1; aidx < numAliases; ++aidx)
        {
            os << ", " << t.getAlias(aidx);
        }
        os << "], ";
    }
    const std::string strFamily{ t.getFamily() };
    if (!strFamily.empty())
    {
        os << ", family=" << strFamily;
    }
    if (t.getNumCategories())
    {
        StringUtils::StringVec categories;
        for (int i = 0; i < t.getNumCategories(); ++i)
        {
            categories.push_back(t.getCategory(i));
        }
        os << ", categories=[" << StringUtils::Join(categories, ',') << "]";
    }
    const std::string desc{ t.getDescription() };
    if (!desc.empty())
    {
        os << ", description=" << desc;
    }
    const std::string enc{ t.getEncoding() };
    if (!enc.empty())
    {
        os << ", encoding=" << enc;
    }
    if (t.getTransform(TRANSFORM_DIR_FORWARD))
    {
        os << ",\n    forward=";
        os << "\n        " << *t.getTransform(TRANSFORM_DIR_FORWARD);
    }
    if (t.getTransform(TRANSFORM_DIR_INVERSE))
    {
        os << ",\n    inverse=";
        os << "\n        " << *t.getTransform(TRANSFORM_DIR_INVERSE);
    }
    os << ">";
    return os;
}

ConstTransformRcPtr GetTransform(const ConstNamedTransformRcPtr & src,
                                 const ConstNamedTransformRcPtr & dst)
{
    if (src && dst)
    {
        // Both are named transforms.
        auto group = GroupTransform::Create();
        auto srcTransform = NamedTransformImpl::GetTransform(src, TRANSFORM_DIR_FORWARD);
        group->appendTransform(srcTransform->createEditableCopy());
        auto dstTransform = NamedTransformImpl::GetTransform(dst, TRANSFORM_DIR_INVERSE);
        group->appendTransform(dstTransform->createEditableCopy());
        return group;
    }
    else if (src)
    {
        // Src is a named transform, ignore dst color space.
        return NamedTransformImpl::GetTransform(src, TRANSFORM_DIR_FORWARD);
    }
    else if (dst)
    {
        // Dst is a named transform, ignore src color space.
        return NamedTransformImpl::GetTransform(dst, TRANSFORM_DIR_INVERSE);
    }
    throw Exception("GetTransform: one of the parameters has to be not null.");
}

} // namespace OCIO_NAMESPACE
