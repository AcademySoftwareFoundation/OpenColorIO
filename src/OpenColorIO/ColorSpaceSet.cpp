// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
        if (csName && *csName)
        {
            const std::string str = StringUtils::Lower(csName);
            for (size_t idx = 0; idx < m_colorSpaces.size(); ++idx)
            {
                if (StringUtils::Lower(m_colorSpaces[idx]->getName()) == str)
                {
                    return static_cast<int>(idx);
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
        const std::string csName = StringUtils::Lower(cs->getName());
        if (csName.empty())
        {
            throw Exception("Cannot add a color space with an empty name.");
        }

        for (auto & entry : m_colorSpaces)
        {
            if (StringUtils::Lower(entry->getName()) == csName)
            {
                // The color space replaces the existing one.
                entry = cs->createEditableCopy();
                return;
            }
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
    *css->m_impl = *m_impl;
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

