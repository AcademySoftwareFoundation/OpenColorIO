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


#include <vector>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"


OCIO_NAMESPACE_ENTER
{
    
class ColorSpaceSet::Impl
{
public:
    
    Impl()
    {
    }
    
    ~Impl()
    {
    }
    
    Impl & operator= (const Impl & rhs)
    {
        if(this!=&rhs)
        {
            m_colorSpaces = rhs.m_colorSpaces;
        }
        return *this;
    }

    bool operator== (const Impl & rhs)
    {
        if(this==&rhs) return true;

        return m_colorSpaces == rhs.m_colorSpaces;
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
            return "";
        }

        return m_colorSpaces[index]->getName();
    }

    ConstColorSpaceRcPtr getByName(const char * name) const 
    {
        return ColorSpaceRcPtr();
    }

    int getIndex(const char * name) const 
    {
        return -1;
    }

    void add(const ConstColorSpaceRcPtr & cs) {}

    void add(const Impl & rhs) {}

    void remove(const ConstColorSpaceRcPtr & cs) {}

    void remove(const Impl & rhs) {}

    void clear() { m_colorSpaces.clear(); }

private:
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

bool ColorSpaceSet::operator==(const ConstColorSpaceSetRcPtr & css) const
{
    return *m_impl == *css->m_impl;
}

bool ColorSpaceSet::operator!=(const ConstColorSpaceSetRcPtr & css) const
{
    return !( *m_impl == *css->m_impl );
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

void ColorSpaceSet::removeColorSpace(const ConstColorSpaceRcPtr & cs)
{
    return m_impl->remove(cs);
}

void ColorSpaceSet::removeColorSpaces(const ConstColorSpaceSetRcPtr & css)
{
    return m_impl->remove(*css->m_impl);
}

void ColorSpaceSet::clearColorSpaces()
{
    m_impl->clear();
}

static ConstColorSpaceSetRcPtr operator||(const ConstColorSpaceSetRcPtr & lcss, 
                                          const ConstColorSpaceSetRcPtr & rcss)
{
    return lcss;    
}

static ConstColorSpaceSetRcPtr operator&&(const ConstColorSpaceSetRcPtr & lcss, 
                                          const ConstColorSpaceSetRcPtr & rcss)
{
    return lcss;    
}

static ConstColorSpaceSetRcPtr operator-(const ConstColorSpaceSetRcPtr & lcss, 
                                         const ConstColorSpaceSetRcPtr & rcss)
{
    return lcss;    
}

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(ColorSpaceSet, basic)
{
/*
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ConstColorSpaceSetRcPtr css1;
    OIIO_CHECK_NO_THROW(css1 = config->getColorSpaces(nullptr));
    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 0);

    // No category

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    OIIO_CHECK_ASSERT(!cs1->hasCategory("linear"));
    OIIO_CHECK_ASSERT(!cs1->hasCategory("rendering"));
    OIIO_CHECK_ASSERT(!cs1->hasCategory("log"));

    // Having categories to filter with

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

    // Update config

    OIIO_CHECK_NO_THROW(config->addColorSpace(cs1));
    OIIO_CHECK_NO_THROW(config->addColorSpace(cs2));

    // Search some color spaces based on criteria

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
    OIIO_CHECK_EQUAL(std::string(css1->getColorSpace(0)->getName()), std::string("cs2"));

    // Empty the config

    OIIO_CHECK_NO_THROW(config->clearColorSpaces());
    OIIO_CHECK_EQUAL(config->getNumColorSpaces(), 0);
    // ... but existing sets are preserved
    OIIO_CHECK_EQUAL(css1->getNumColorSpaces(), 1);

    OCIO::ConstColorSpaceSetRcPtr css2;
    OIIO_CHECK_NO_THROW(css2 = config->getColorSpaces("linear"));
    OIIO_CHECK_EQUAL(css2->getNumColorSpaces(), 0);

    OIIO_CHECK_NO_THROW(css2 = config->getColorSpaces(nullptr));
    OIIO_CHECK_EQUAL(css2->getNumColorSpaces(), 0);
*/
}


OIIO_ADD_TEST(ColorSpaceSet, operations)
{
/*
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    // No category

    OCIO::ColorSpaceRcPtr cs1 = OCIO::ColorSpace::Create();
    cs1->setName("cs1");
    OIIO_CHECK_NO_THROW(config->addColorSpace(cs1));

    // Having categories to filter with

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

    // cs1  -> none
    // cs2  -> [rendering, linear]
    // cs3  -> [rendering, log]

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

    // Union

    OCIO::ConstColorSpaceSetRcPtr css4 = css2 || css3;
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 2); // cs2 & cs3

    css4 = css1 || css2;
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 3); // no duplication i.e. all color spaces

    // Intersection

    css4 = css2 && css3;
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 0);

    css4 = css2 && css1;
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 1); // cs2
    OIIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(0)), std::string("cs2"));
    OIIO_CHECK_EQUAL(std::string(css4->getColorSpace(0)->getName()), std::string("cs2"));

    // Difference

    css4 = css1 - css3;
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 2); // cs1 & cs2

    css4 = css1 - css2;
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 2); // cs1 & cs3

    // Several operations

    css4 = css1 - (css2 || css3);
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 1); // cs1
    OIIO_CHECK_EQUAL(std::string(css4->getColorSpaceNameByIndex(0)), std::string("cs1"));

    OCIO::ConstColorSpaceSetRcPtr css5;
    OIIO_CHECK_NO_THROW(css5 = config->getColorSpaces("rendering"));
    OIIO_CHECK_EQUAL(css5->getNumColorSpaces(), 2); // cs2 & cs3

    css4 = (css1 - css5) && (css2 || css3);
    OIIO_CHECK_EQUAL(css4->getNumColorSpaces(), 0);
*/
}


#endif // OCIO_UNIT_TEST
