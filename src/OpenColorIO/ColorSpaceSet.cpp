/*
Copyright (c) 2018 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"
#include "pystring/pystring.h"


OCIO_NAMESPACE_ENTER
{
    
class ColorSpaceSet::Impl
{
public:
    Impl() { }
    ~Impl() { }

    Impl(const Impl &) = delete;
    
    Impl & operator= (const Impl & rhs)
    {
        if(this!=&rhs)
        {
            clear();

            for(auto & cs: rhs.m_colorSpaces)
            {
                m_colorSpaces.push_back(cs->createEditableCopy());
            }
        }
        return *this;
    }

    bool operator== (const Impl & rhs)
    {
        if(this==&rhs) return true;

        if(m_colorSpaces.size()!=rhs.m_colorSpaces.size())
        {
            return false;
        }

        for(auto & cs : m_colorSpaces)
        {
            // NB: Only the names are compared.
            if(-1==rhs.getIndex(cs->getName()))
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
        if(index<0 || index>=size())
        {
            return ColorSpaceRcPtr();
        }

        return m_colorSpaces[index];
    }

    const char * getName(int index) const 
    {
        if(index<0 || index>=size())
        {
            return nullptr;
        }

        return m_colorSpaces[index]->getName();
    }

    ConstColorSpaceRcPtr getByName(const char * csName) const 
    {
        if(csName && *csName)
        {
            const std::string str = pystring::lower(csName);
            for(auto & cs: m_colorSpaces)
            {
                if(pystring::lower(cs->getName())==str)
                {
                    return cs;
                }
            }
        }
        return ColorSpaceRcPtr();
    }

    int getIndex(const char * csName) const 
    {
        if(csName && *csName)
        {
            const std::string str = pystring::lower(csName);
            for(size_t idx = 0; idx<m_colorSpaces.size(); ++idx)
            {
                if(pystring::lower(m_colorSpaces[idx]->getName())==str)
                {
                    return static_cast<int>(idx);
                }
            }
        }

        return -1;
    }

    void add(const ConstColorSpaceRcPtr & cs)
    {
        const std::string csName = pystring::lower(cs->getName());
        if(csName.empty())
        {
            throw Exception("Cannot add a color space with an empty name.");
        }

        for(auto & entry: m_colorSpaces)
        {
            if(pystring::lower(entry->getName())==csName)
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
        for(auto & cs : rhs.m_colorSpaces)
        {
            add(cs);
        }
    }

    void remove(const char * csName)
    {
        const std::string name = pystring::lower(csName);
        if(name.empty()) return;

        for(auto itr = m_colorSpaces.begin(); itr != m_colorSpaces.end(); ++itr)
        {
            if(pystring::lower((*itr)->getName())==name)
            {
                m_colorSpaces.erase(itr);
                return;
            }
        }
    }

    void remove(const Impl & rhs)
    {
        for(auto & cs : rhs.m_colorSpaces)
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

int ColorSpaceSet::getIndexForColorSpace(const char * name) const
{
    return m_impl->getIndex(name);
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

    for(int idx=0; idx<rcss->getNumColorSpaces(); ++idx)
    {
        ConstColorSpaceRcPtr tmp = rcss->getColorSpaceByIndex(idx);
        if(-1!=lcss->getIndexForColorSpace(tmp->getName()))
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

    for(int idx=0; idx<lcss->getNumColorSpaces(); ++idx)
    {
        ConstColorSpaceRcPtr tmp = lcss->getColorSpaceByIndex(idx);

        if(-1==rcss->getIndexForColorSpace(tmp->getName()))
        {
            css->addColorSpace(tmp);
        }
    }

    return css;
}

}
OCIO_NAMESPACE_EXIT




///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(ColorSpaceSet, basic)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ConstColorSpaceSetRcPtr css1;
    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 0);

    // No category.

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    OIIO_CHECK_ASSERT(!cs1->hasCategory("linear"));
    OIIO_CHECK_ASSERT(!cs1->hasCategory("rendering"));
    OIIO_CHECK_ASSERT(!cs1->hasCategory("log"));

    // Having categories to filter with.

    OCIO::ColorSpaceRcPtr cs2 = OCIO::ColorSpace::Create();
    cs2->setName("cs2");
    cs2->addCategory("linear");
    cs2->addCategory("rendering");
    OIIO_CHECK_ASSERT(cs2->hasCategory("linear"));
    OIIO_CHECK_ASSERT(cs2->hasCategory("rendering"));
    OIIO_CHECK_ASSERT(!cs2->hasCategory("log"));

    OIIO_CHECK_NO_THROW(cs2->addCategory("log"));
    OIIO_CHECK_ASSERT(cs2->hasCategory("log"));
    OIIO_CHECK_NO_THROW(cs2->removeCategory("log"));
    OIIO_CHECK_ASSERT(!cs2->hasCategory("log"));

    // Update config.

    OIIO_CHECK_NO_THROW(config->addColorSpace(cs1));
    OIIO_CHECK_NO_THROW(config->addColorSpace(cs2));

    // Search some color spaces based on criteria.

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 2);
    OIIO_CHECK_EQUAL(config->getNumColorSpaces(), 2);

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces(""));
    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 2);

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces("log"));
    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 0);

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces("linear"));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs2"));

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces("LinEar"));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs2"));

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces(" LinEar "));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs2"));
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceByIndex(0)->getName()), std::string("cs2"));

    // Test some faulty requests.

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces("lin ear"));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 0);

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces("[linear]"));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 0);

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces("linear log"));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 0);

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces("linearlog"));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 0);

    // Empty the config.

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces("linear"));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 1);

    OIIO_CHECK_NO_THROW(config->clearColorSpaces());
    OIIO_CHECK_EQUAL(config->getNumColorSpaces(), 0);
    // But existing sets are preserved.
    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 1);

    OCIO::ConstColorSpaceSetRcPtr css2;
    OIIO_CHECK_NO_THROW(css2 = config->getColorSpaces(nullptr));
    OIIO_CHECK_EQUAL(css2->getNumColorSpaces(), 0);
}

OIIO_ADD_TEST(ColorSpaceSet, decoupled_sets)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    OIIO_CHECK_NO_THROW(cs1->addCategory("linear"));
    OIIO_CHECK_ASSERT(cs1->hasCategory("linear"));
    OIIO_CHECK_NO_THROW(config->addColorSpace(cs1));

    OCIO::ConstColorSpaceSetRcPtr css1;
    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs1"));

    OCIO::ConstColorSpaceSetRcPtr css2;
    OIIO_CHECK_NO_THROW(css2 = config->getColorSpaces("linear"));
    OIIO_CHECK_EQUAL(css2->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css2->getColorSpaceNameByIndex(0)), std::string("cs1"));

    // Change the original color space.

    cs1->setName("new_cs1");

    // Check that color spaces in existing sets are not changed.
    OIIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(0)), std::string("cs1"));

    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs1"));

    OIIO_CHECK_EQUAL(css2->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css2->getColorSpaceNameByIndex(0)), std::string("cs1"));

    // Change the color space from the config instance.

    OIIO_CHECK_ASSERT(!cs1->isData());
    config->clearColorSpaces();
    config->addColorSpace(cs1);
    cs1->setIsData(true);

    OIIO_CHECK_EQUAL(std::string(cs1->getName()), std::string("new_cs1"));
    OIIO_CHECK_ASSERT(cs1->isData());
    OIIO_CHECK_EQUAL(std::string(config->getColorSpaceNameByIndex(0)), std::string("new_cs1"));
    // NB: ColorSpace would need to be re-added to the config to reflect the change to isData.
    OIIO_CHECK_ASSERT(!config->getColorSpace("new_cs1")->isData());

    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OIIO_CHECK_ASSERT(!css1->getColorSpace("cs1")->isData());

    OIIO_CHECK_EQUAL(css2->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css2->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OIIO_CHECK_ASSERT(!css2->getColorSpace("cs1")->isData());
}

OIIO_ADD_TEST(ColorSpaceSet, order_validation)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ConstColorSpaceSetRcPtr css1;
    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 0);

    // Create some color spaces.

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    cs1->addCategory("linear");
    cs1->addCategory("rendering");

    OCIO::ColorSpaceRcPtr cs2 = OCIO::ColorSpace::Create();
    cs2->setName("cs2");
    cs2->addCategory("rendering");
    cs2->addCategory("linear");

    OCIO::ColorSpaceRcPtr cs3 = OCIO::ColorSpace::Create();
    cs3->setName("cs3");
    cs3->addCategory("rendering");

    // Add the color spaces.

    OIIO_CHECK_NO_THROW(config->addColorSpace(cs1));
    OIIO_CHECK_NO_THROW(config->addColorSpace(cs2));
    OIIO_CHECK_NO_THROW(config->addColorSpace(cs3));

    // Check the color space order for the category "linear".

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces("linear"));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 2);

    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(1)), std::string("cs2"));

    // Check the color space order for the category "rendering".

    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces("rendering"));
    OIIO_REQUIRE_EQUAL(css1->getNumColorSpaces(), 3);

    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(1)), std::string("cs2"));
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpaceNameByIndex(2)), std::string("cs3"));
}

OIIO_ADD_TEST(ColorSpaceSet, operations_on_set)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    // No category.

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    OIIO_CHECK_NO_THROW(config->addColorSpace(cs1));

    // Having categories to filter with.

    OCIO::ColorSpaceRcPtr cs2 = OCIO::ColorSpace::Create();
    cs2->setName("cs2");
    cs2->addCategory("linear");
    cs2->addCategory("rendering");
    OIIO_CHECK_NO_THROW(config->addColorSpace(cs2));

    OCIO::ColorSpaceRcPtr cs3 = OCIO::ColorSpace::Create();
    cs3->setName("cs3");
    cs3->addCategory("log");
    cs3->addCategory("rendering");
    OIIO_CHECK_NO_THROW(config->addColorSpace(cs3));


    // Recap. of the existing color spaces:
    // cs1  -> name="cs1" i.e. no category
    // cs2  -> name="cs2", categories=[rendering, linear]
    // cs3  -> name="cs3", categories=[rendering, log]


    OCIO::ConstColorSpaceSetRcPtr css1;
    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 3);

    OCIO::ConstColorSpaceSetRcPtr css2;
    OIIO_CHECK_NO_THROW(css2 = config->getColorSpaces("linear"));
    OIIO_CHECK_EQUAL(css2->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css2->getColorSpaceNameByIndex(0)), std::string("cs2"));

    OCIO::ConstColorSpaceSetRcPtr css3;
    OIIO_CHECK_NO_THROW(css3 = config->getColorSpaces("log"));
    OIIO_CHECK_EQUAL(css3->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css3->getColorSpaceNameByIndex(0)), std::string("cs3"));


    // Recap. of the existing color space sets:
    // ccs1 -> {cs1, cs2, cs3}
    // ccs2 -> {cs2}
    // css3 -> {cs3}


    // Test the union.

    OCIO::ConstColorSpaceSetRcPtr css4 = css2 || css3;
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 2); // {cs2, cs3}

    css4 = css1 || css2;
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 3); // no duplication i.e. all color spaces

    // Test the intersection.

    css4 = css2 && css3;
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 0);

    css4 = css2 && css1;
    OIIO_REQUIRE_EQUAL(css4->getNumColorSpaces(), 1); // {cs2}
    OIIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(0)), std::string("cs2"));
    OIIO_CHECK_EQUAL(std::string(css4->getColorSpaceByIndex(0)->getName()), std::string("cs2"));

    // Test the difference.

    css4 = css1 - css3;
    OIIO_REQUIRE_EQUAL(css4->getNumColorSpaces(), 2); // {cs1, cs2}
    OIIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OIIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(1)), std::string("cs2"));

    css4 = css1 - css2;
    OIIO_REQUIRE_EQUAL(css4->getNumColorSpaces(), 2); // {cs1, cs3}
    OIIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(0)), std::string("cs1"));
    OIIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(1)), std::string("cs3"));

    // Test with several embedded operations.

    css4 = css1 - (css2 || css3);
    OIIO_REQUIRE_EQUAL(css4->getNumColorSpaces(), 1); // {cs1}
    OIIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(0)), std::string("cs1"));

    OCIO::ColorSpaceSetRcPtr css5;
    OIIO_CHECK_NO_THROW(css5 = config->getColorSpaces("rendering"));
    OIIO_CHECK_EQUAL(css5->getNumColorSpaces(), 2); // {cs2, cs3}
    // Manipulate the result with few tests.
    OIIO_CHECK_NO_THROW(css5->addColorSpace(cs1));
    OIIO_CHECK_EQUAL(css5->getNumColorSpaces(), 3); // {cs1, cs2, cs3}
    OIIO_CHECK_NO_THROW(css5->removeColorSpace("cs2"));
    OIIO_CHECK_NO_THROW(css5->removeColorSpace("cs1"));
    OIIO_CHECK_EQUAL(css5->getNumColorSpaces(), 1);
    OIIO_CHECK_EQUAL(std::string(css5->getColorSpaceNameByIndex(0)), std::string("cs3"));
    OIIO_CHECK_NO_THROW(css5->clearColorSpaces());
    OIIO_CHECK_EQUAL(css5->getNumColorSpaces(), 0);

    OIIO_CHECK_NO_THROW(css5 = config->getColorSpaces("rendering"));
    OIIO_REQUIRE_EQUAL(css5->getNumColorSpaces(), 2); // {cs2, cs3}
    OIIO_CHECK_EQUAL(std::string(css5->getColorSpaceNameByIndex(0)), std::string("cs2"));
    OIIO_CHECK_EQUAL(std::string(css5->getColorSpaceNameByIndex(1)), std::string("cs3"));

    css4 = (css1  - css5)  // ( {cs1, cs2, cs3} - {cs2, cs3} ) --> {cs1}
           && 
           (css2 || css3); // ( {cs2} || {cs3} )               --> {cs2, cs3}

    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 0);
}

#endif // OCIO_UNIT_TEST
