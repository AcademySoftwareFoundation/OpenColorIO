// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

class ColorSpaceSet::Impl
{
public:
    Impl() = default;
    ~Impl() = default;

    Impl(const Impl &) = delete;

    Impl & operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            clear();

            for (auto & cs: rhs.m_colorSpaces)
            {
                m_colorSpaces.push_back(cs->createEditableCopy());
            }
        }
        return *this;
    }

    bool operator== (const Impl & rhs)
    {
        if (this == &rhs) return true;

        if (m_colorSpaces.size() != rhs.m_colorSpaces.size())
        {
            return false;
        }

        for (auto & cs : m_colorSpaces)
        {
            // NB: Only the names are compared.
            if (!rhs.isPresent(cs->getName()))
            {
                return false;
            }
        }

        return true;
    }

    int size() const 
    { 
        return static_cast<int>(m_colorSpaces.size()); 
    }

    ConstColorSpaceRcPtr get(int index) const 
    {
        if (index < 0 || index >= size())
        {
            return ColorSpaceRcPtr();
        }

        return m_colorSpaces[index];
    }

    const char * getName(int index) const 
    {
        if (index < 0 || index >= size())
        {
            return nullptr;
        }

        return m_colorSpaces[index]->getName();
    }

    ConstColorSpaceRcPtr getByName(const char * csName) const 
    {
        return get(getIndex(csName));
    }

    int getIndex(const char * csName) const 
    {
        // Search for name and aliases.
        if (csName && *csName)
        {
            const std::string str = StringUtils::Lower(csName);
            for (size_t idx = 0; idx < m_colorSpaces.size(); ++idx)
            {
                if (StringUtils::Compare(m_colorSpaces[idx]->getName(), str))
                {
                    return static_cast<int>(idx);
                }
                const size_t numAliases = m_colorSpaces[idx]->getNumAliases();
                for (size_t aidx = 0; aidx < numAliases; ++aidx)
                {
                    if (StringUtils::Compare(m_colorSpaces[idx]->getAlias(aidx), str))
                    {
                        return static_cast<int>(idx);
                    }
                }
            }
        }

        return -1;
    }

    bool isPresent(const char * csName) const
    {
        return -1 != getIndex(csName);
    }

    void add(const ConstColorSpaceRcPtr & cs)
    {
        const char * csName = cs->getName();
        if (!*csName)
        {
            throw Exception("Cannot add a color space with an empty name.");
        }

        auto entryIdx = getIndex(csName);
        size_t replaceIdx = (size_t)-1;
        if (entryIdx != -1)
        {
            // If getIndex succeeds but the csName is not the name of the matching color space, it
            // means that csName must be an alias name.  Color space will be replaced only when
            // canonical names match.
            if (!StringUtils::Compare(m_colorSpaces[entryIdx]->getName(), csName))
            {
                std::ostringstream os;
                os << "Cannot add '" << csName << "' color space, existing color space, '";
                os << m_colorSpaces[entryIdx]->getName() << "' is using this name as an alias.";
                throw Exception(os.str().c_str());
            }
            // There is a color space with the same name that will be replaced (if new color space
            // can be used).
            replaceIdx = entryIdx;
        }

        const size_t numAliases = cs->getNumAliases();
        for (size_t aidx = 0; aidx < numAliases; ++aidx)
        {
            const char * alias = cs->getAlias(aidx);
            entryIdx = getIndex(alias);
            // Is an alias of the color space already used by a color space?
            // Skip existing colorspace that might be replaced.
            if (entryIdx != -1 && static_cast<int>(replaceIdx) != entryIdx)
            {
                std::ostringstream os;
                os << "Cannot add '" << csName << "' color space, it has '" << alias;
                os << "' alias and existing color space, '";
                os << m_colorSpaces[entryIdx]->getName() << "' is using the same alias.";
                throw Exception(os.str().c_str());
            }
        }
        if (replaceIdx != (size_t)-1)
        {
            // The color space replaces the existing one.
            m_colorSpaces[replaceIdx] = cs->createEditableCopy();
            return;
        }

        m_colorSpaces.push_back(cs->createEditableCopy());
    }

    void add(const Impl & rhs)
    {
        for (auto & cs : rhs.m_colorSpaces)
        {
            add(cs);
        }
    }

    void remove(const char * csName)
    {
        const std::string name = StringUtils::Lower(csName);
        if (name.empty()) return;

        for (auto itr = m_colorSpaces.begin(); itr != m_colorSpaces.end(); ++itr)
        {
            if (StringUtils::Lower((*itr)->getName())==name)
            {
                m_colorSpaces.erase(itr);
                return;
            }
        }
    }

    void remove(const Impl & rhs)
    {
        for (auto & cs : rhs.m_colorSpaces)
        {
            remove(cs->getName());
        }
    }

    void clear()
    {
        m_colorSpaces.clear();
    }

private:
    typedef std::vector<ColorSpaceRcPtr> ColorSpaceVec;
    ColorSpaceVec m_colorSpaces;
};


///////////////////////////////////////////////////////////////////////////

ColorSpaceSetRcPtr ColorSpaceSet::Create()
{
    return ColorSpaceSetRcPtr(new ColorSpaceSet(), &deleter);
}

void ColorSpaceSet::deleter(ColorSpaceSet* c)
{
    delete c;
}


///////////////////////////////////////////////////////////////////////////



ColorSpaceSet::ColorSpaceSet()
    :   m_impl(new ColorSpaceSet::Impl)
{
}

ColorSpaceSet::~ColorSpaceSet()
{
    delete m_impl;
    m_impl = nullptr;
}

ColorSpaceSetRcPtr ColorSpaceSet::createEditableCopy() const
{
    ColorSpaceSetRcPtr css = ColorSpaceSet::Create();
    *css->m_impl = *m_impl; // Deep Copy.
    return css;
}

bool ColorSpaceSet::operator==(const ColorSpaceSet & css) const
{
    return *m_impl == *css.m_impl;
}

bool ColorSpaceSet::operator!=(const ColorSpaceSet & css) const
{
    return !( *m_impl == *css.m_impl );
}

int ColorSpaceSet::getNumColorSpaces() const
{
    return m_impl->size();    
}

const char * ColorSpaceSet::getColorSpaceNameByIndex(int index) const
{
    return m_impl->getName(index);
}

ConstColorSpaceRcPtr ColorSpaceSet::getColorSpaceByIndex(int index) const
{
    return m_impl->get(index);
}

ConstColorSpaceRcPtr ColorSpaceSet::getColorSpace(const char * name) const
{
    return m_impl->getByName(name);
}

int ColorSpaceSet::getColorSpaceIndex(const char * name) const
{
    return m_impl->getIndex(name);
}

bool ColorSpaceSet::hasColorSpace(const char * name) const
{
    return m_impl->isPresent(name);
}

void ColorSpaceSet::addColorSpace(const ConstColorSpaceRcPtr & cs)
{
    return m_impl->add(cs);
}

void ColorSpaceSet::addColorSpaces(const ConstColorSpaceSetRcPtr & css)
{
    return m_impl->add(*css->m_impl);
}

void ColorSpaceSet::removeColorSpace(const char * name)
{
    return m_impl->remove(name);
}

void ColorSpaceSet::removeColorSpaces(const ConstColorSpaceSetRcPtr & css)
{
    return m_impl->remove(*css->m_impl);
}

void ColorSpaceSet::clearColorSpaces()
{
    m_impl->clear();
}

ConstColorSpaceSetRcPtr operator||(const ConstColorSpaceSetRcPtr & lcss, 
                                   const ConstColorSpaceSetRcPtr & rcss)
{
    ColorSpaceSetRcPtr css = lcss->createEditableCopy();
    css->addColorSpaces(rcss);
    return css;    
}

ConstColorSpaceSetRcPtr operator&&(const ConstColorSpaceSetRcPtr & lcss, 
                                   const ConstColorSpaceSetRcPtr & rcss)
{
    ColorSpaceSetRcPtr css = ColorSpaceSet::Create();

    for (int idx = 0; idx < rcss->getNumColorSpaces(); ++idx)
    {
        ConstColorSpaceRcPtr tmp = rcss->getColorSpaceByIndex(idx);
        if (lcss->hasColorSpace(tmp->getName()))
        {
            css->addColorSpace(tmp);
        }
    }

    return css;
}

ConstColorSpaceSetRcPtr operator-(const ConstColorSpaceSetRcPtr & lcss, 
                                  const ConstColorSpaceSetRcPtr & rcss)
{
    ColorSpaceSetRcPtr css = ColorSpaceSet::Create();

    for (int idx = 0; idx < lcss->getNumColorSpaces(); ++idx)
    {
        ConstColorSpaceRcPtr tmp = lcss->getColorSpaceByIndex(idx);

        if (!rcss->hasColorSpace(tmp->getName()))
        {
            css->addColorSpace(tmp);
        }
    }

    return css;
}

} // namespace OCIO_NAMESPACE

