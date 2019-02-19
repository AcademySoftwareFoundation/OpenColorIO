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

#include "ops/Metadata.h"

OCIO_NAMESPACE_ENTER
{
// Find a given name on a list of metadata items.
// The template parameter ITERATOR can be used to return
// a constant or modifiable iterator.
// - items the list of metadata items
// - name the metadata item name to search
// Return the iterator to the item found, or items.end() otherwise
template<typename ITERATOR, typename CONTAINER>
    ITERATOR findItem(CONTAINER& items, const std::string& name)
{
    ITERATOR end = items.end();
    for (ITERATOR it = items.begin(); it != end; ++it)
    {
        if (it->getName() == name)
        {
            return it;
        }
    }
    return end;
}

Metadata::Metadata(const std::string& name)
    :   m_name(name)
{
    if (name.empty())
    {
        throw Exception("Metadata with empty name.");
    }
}

Metadata::Metadata(const Metadata& other)
    :   m_name(other.m_name),
        m_value(other.m_value),
        m_attributes(other.m_attributes),
        m_items(other.m_items)
{
}

const std::string& Metadata::getName() const
{
    return m_name;
}

const std::string& Metadata::getValue() const
{
    if (!isLeaf())
    {
        std::stringstream os;
        os << "Metadata should be a leaf '";
        os << getName().c_str();
        os << "'.";
        throw Exception(os.str().c_str());
    }
    return m_value;
}

const Metadata::Attributes& Metadata::getAttributes() const
{
    return m_attributes;
}

void Metadata::addAttribute(const Attribute& attribute)
{
    // If this attribute already exists, overwrite the value. 
    // Otherwise, add the new attribute. This ensures that we do not 
    // have the same attribute twice.
    Attributes::iterator it = m_attributes.begin();
    const Attributes::iterator itEnd = m_attributes.end();

    for (; it != itEnd; ++it)
    {
        if (it->first == attribute.first)
        {
            it->second = attribute.second;
            return;
        }
    }

    m_attributes.push_back(attribute);
}

Metadata::MetadataList Metadata::getItems() const
{
    if (isLeaf())
    {
        std::ostringstream os;
        os << "Metadata should be a container '";
        os << getName().c_str();
        os << "'.";
        throw Exception(os.str().c_str());
    }
    return m_items;
}

Metadata::NameList Metadata::getItemsNames() const
{
    NameList names;
    for (MetadataList::const_iterator it = m_items.begin(), 
                                        end = m_items.end(); it != end; ++it)
    {
        names.push_back(it->getName());
    }
    return names;
}

bool Metadata::isLeaf() const
{
    return m_items.empty();
}

bool Metadata::isEmpty() const
{
    return m_value.empty() && m_items.empty();
}

void Metadata::clear()
{
    m_attributes.clear();
    m_value = "";
    m_items.clear();
}

void Metadata::remove(const std::string & itemName)
{
    MetadataList::iterator 
        it = findItem<MetadataList::iterator>(m_items, itemName);
    if (it == m_items.end())
    {
        std::ostringstream os;
        os << "Metadata element not found '";
        os << itemName.c_str();
        os << "'.";
        throw Exception(os.str().c_str());
    }
    m_items.erase(it);
}

Metadata& Metadata::operator[](const std::string & itemName)
{
    MetadataList::iterator 
        it = findItem<MetadataList::iterator>(m_items, itemName);
    if (it == m_items.end())
    {
        m_items.push_back(Metadata(itemName));
        m_value = "";
        return m_items.back();
    }
    return *it;
}

const Metadata& Metadata::operator[](const std::string & itemName) const
{
    MetadataList::const_iterator 
        it = findItem<MetadataList::const_iterator>(m_items, itemName);
    if (it == m_items.end()) {
        std::ostringstream os;
        os << "Metadata element not found '";
        os << itemName.c_str();
        os << "'.";
        throw Exception(os.str().c_str());
    }
    return *it;
}

Metadata& Metadata::operator=(const std::string & value)
{
    m_value = value;
    m_items.clear();
    return *this;
}

Metadata& Metadata::operator=(const Metadata & rhs)
{
    if (this != &rhs)
    {
        m_name       = rhs.m_name;
        m_value      = rhs.m_value;
        m_attributes = rhs.m_attributes;
        m_items      = rhs.m_items;
    }
    return *this;
}
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(Metadata, test_accessors)
{
    OCIO::Metadata info("Info");

    // Make sure that we can add attributes and that existing attributes will get
    // overwritten.
    info.addAttribute(OCIO::Metadata::Attribute("version", "1.0"));

    const OCIO::Metadata::Attributes& atts1 = info.getAttributes();
    OIIO_CHECK_EQUAL(atts1.size(), 1);
    OIIO_CHECK_EQUAL(atts1[0].first, "version");
    OIIO_CHECK_EQUAL(atts1[0].second, "1.0");

    info.addAttribute(OCIO::Metadata::Attribute("version", "2.0"));

    const OCIO::Metadata::Attributes& atts2 = info.getAttributes();
    OIIO_CHECK_EQUAL(atts2.size(), 1);
    OIIO_CHECK_EQUAL(atts2[0].first, "version");
    OIIO_CHECK_EQUAL(atts2[0].second, "2.0");

    info["Copyright"] = "Copyright 2013 Autodesk";
    info["Release"] = "2015";

    // Add input color space metadata.
    OCIO::Metadata& inCS = info["InputColorSpace"];
    inCS["Description"] = "Input color space description";
    inCS["Profile"] = "Input color space profile";

    // Add output color space metadata.
    // Use an alternative method to add metadata.
    info["OutputColorSpace"]["Description"] = "Output color space description";
    info["OutputColorSpace"]["Profile"] = "Output color space profile";

    // Add category.
    // Assign value diretly to the metadata item.
    OCIO::Metadata& cat = info["Category"];
    OCIO::Metadata& catName = cat["Name"];
    OCIO::Metadata& catImp = cat["Importance"];
    catName = "Color space category name";
    catImp = "High";

    {
        const OCIO::Metadata cinfo = info;
        OIIO_CHECK_EQUAL(cinfo["Copyright"].getValue(), "Copyright 2013 Autodesk");
        OIIO_CHECK_EQUAL(cinfo["Release"].getValue(), "2015");
        OIIO_CHECK_EQUAL(cinfo["InputColorSpace"]["Description"].getValue(), 
                         "Input color space description");
        OIIO_CHECK_EQUAL(cinfo["InputColorSpace"]["Profile"].getValue(), 
                         "Input color space profile");
        OIIO_CHECK_EQUAL(cinfo["OutputColorSpace"]["Description"].getValue(), 
                         "Output color space description");
        OIIO_CHECK_EQUAL(cinfo["OutputColorSpace"]["Profile"].getValue(), 
                         "Output color space profile");
        OIIO_CHECK_EQUAL(cinfo["Category"]["Name"].getValue(),
                         "Color space category name");
        OIIO_CHECK_EQUAL(cinfo["Category"]["Importance"].getValue(), "High");
    }

    info["Extra"]["Item1"]["Item1a"] = "Extra:Item1:Item1a";
    info["Extra"]["Item1"]["Item1b"] = "Extra:Item1:Item1b";
    info["Extra"]["Item2"]["Item2a"] = "Extra:Item2:Item2a";
    info["Extra"]["Item2"]["Item2b"] = "Extra:Item2:Item2b";
    info["Extra"]["Item2"]["Item2c"] = "Extra:Item2:Item2c";

    {
        const OCIO::Metadata cinfo = info;
        OIIO_CHECK_ASSERT(!cinfo["Extra"].isLeaf());
        OIIO_CHECK_ASSERT(cinfo["Extra"].getItems().size() == 2);
    }

    // This should clear subelements of 'Extra' and make it a leaf metadata.
    info["Extra"] = "Blah";

    {
        const OCIO::Metadata& cinfo = info;
        OIIO_CHECK_ASSERT(cinfo["Extra"].isLeaf());
        OIIO_CHECK_EQUAL(cinfo["Extra"].getValue(), "Blah");
    }

    // This should clear the (leaf) value of 'Extra'
    info["Extra"]["Item3"]["Item3a"] = "Extra:Item3:Item3a";
    info["Extra"]["Item3"]["Item3b"] = "Extra:Item3:Item3b";
    info["Extra"]["Item3"]["Item3c"] = "Extra:Item3:Item3c";

    {
        const OCIO::Metadata& cinfo = info;
        OIIO_CHECK_ASSERT(!cinfo["Extra"].isLeaf());
        OIIO_CHECK_EQUAL(cinfo["Extra"].getItems().size(), 1);
        OIIO_CHECK_EQUAL(cinfo["Extra"]["Item3"].getItems().size(), 3);
    }

    // Remove a subelement.
    info["Extra"]["Item3"].remove("Item3b");

    {
        const OCIO::Metadata& cinfo = info;
        OIIO_CHECK_EQUAL(cinfo["Extra"]["Item3"].getItems().size(), 2);
        OIIO_CHECK_EQUAL(cinfo["Extra"]["Item3"]["Item3a"].getValue(), 
                         "Extra:Item3:Item3a");
        OIIO_CHECK_EQUAL(cinfo["Extra"]["Item3"]["Item3c"].getValue(),
                         "Extra:Item3:Item3c");
    }

    // Clearing a leaf metadata
    // This should make 'Item3a' an empty leaf metadata.
    info["Extra"]["Item3"]["Item3a"].clear();

    {
        const OCIO::Metadata& cinfo = info;
        OIIO_CHECK_ASSERT(cinfo["Extra"]["Item3"]["Item3a"].isLeaf());
        OIIO_CHECK_EQUAL(cinfo["Extra"]["Item3"]["Item3a"].getValue(), "");
    }

    // Clearing a non-leaf metadata.
    // This should remove all subelements of 'Item3' and make it an empty leaf metadata.
    info["Extra"]["Item3"].clear();

    {
        const OCIO::Metadata& cinfo = info;
        OIIO_CHECK_ASSERT(cinfo["Extra"]["Item3"].isLeaf());
        OIIO_CHECK_EQUAL(cinfo["Extra"]["Item3"].getValue(), "");
    }

    // Create a separate metadata structure and use it to replace an element.
    //

    // Change input profile and description.
    OCIO::Metadata newInCS("NewInputColorSpace");
    newInCS["Profile"] = "New input color space profile";
    newInCS["Description"] = "New input color space description";

    info["InputColorSpace"] = newInCS;
    {
        const OCIO::Metadata& cinfo = info;
        OIIO_CHECK_EQUAL(cinfo["NewInputColorSpace"]["Profile"].getValue(),
                         "New input color space profile");
        OIIO_CHECK_EQUAL(cinfo["NewInputColorSpace"]["Description"].getValue(),
                         "New input color space description");
    }

    // Check exceptions.
    const OCIO::Metadata& cinfo = info;

    OIIO_CHECK_THROW_WHAT(cinfo["OutputColorSpace"].getValue(), 
                          OCIO::Exception, 
                          "Metadata should be a leaf 'OutputColorSpace'");

    OIIO_CHECK_THROW_WHAT(cinfo["OutputColorSpace"]["Profile"].getItems(),
                          OCIO::Exception,
                          "Metadata should be a container 'Profile'");

    OIIO_CHECK_THROW_WHAT(cinfo["OutputColorSpace"]["WrongName"],
                          OCIO::Exception,
                          "Metadata element not found 'WrongName'");

    OIIO_CHECK_THROW_WHAT(info["OutputColorSpace"].remove("WrongName"),
                          OCIO::Exception,
                          "Metadata element not found 'WrongName'");
    
}

#endif
