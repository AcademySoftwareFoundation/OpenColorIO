// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FormatMetadata.h"
#include "Op.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

// CLF XML elements described in S-2014-006.
const char * METADATA_DESCRIPTION = "Description";
const char * METADATA_INFO = "Info";
const char * METADATA_INPUT_DESCRIPTOR = "InputDescriptor";
const char * METADATA_OUTPUT_DESCRIPTOR = "OutputDescriptor";

// NAME and ID are CLF XML attributes described in S-2014-006.
const char * METADATA_NAME = "name";
const char * METADATA_ID = "id";

std::ostream & operator<< (std::ostream & os, const FormatMetadata & fd)
{
    const std::string name{ fd.getElementName() };
    os << "<" << name;
    const int numAtt = fd.getNumAttributes();
    if (numAtt)
    {
        for (int i = 0; i < numAtt; ++i)
        {
            os << " " << fd.getAttributeName(i) << "=\"" << fd.getAttributeValue(i) << "\"";
        }
    }
    os << ">";
    const std::string val{ fd.getElementValue() };
    if (!val.empty())
    {
        os << val;
    }
    const int numElt = fd.getNumChildrenElements();
    for (int i = 0; i < numElt; ++i)
    {
        os << fd.getChildElement(i);
    }
    os << "</" << name << ">";
    return os;
}

FormatMetadataImpl::FormatMetadataImpl()
    : FormatMetadata()
    , m_name(METADATA_ROOT)
    , m_value("")
{
}

FormatMetadataImpl::FormatMetadataImpl(const std::string & name, const std::string & value)
    : FormatMetadata()
    , m_name(name)
    , m_value(value)
{
    if (name.empty())
    {
        throw Exception("FormatMetadata has to have a non-empty name.");
    }
}

FormatMetadataImpl::FormatMetadataImpl(const FormatMetadataImpl & other)
    : FormatMetadata()
    , m_name(other.m_name)
    , m_value(other.m_value)
    , m_attributes(other.m_attributes)
    , m_elements(other.m_elements)
{
}

FormatMetadataImpl::FormatMetadataImpl(const FormatMetadata & other)
    : FormatMetadataImpl(dynamic_cast<const FormatMetadataImpl &>(other))
{
}

FormatMetadataImpl::~FormatMetadataImpl()
{
}

const FormatMetadataImpl::Attributes & FormatMetadataImpl::getAttributes() const noexcept
{
    return m_attributes;
}

void FormatMetadataImpl::addAttribute(const Attribute & attribute) noexcept
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

FormatMetadataImpl::Elements & FormatMetadataImpl::getChildrenElements() noexcept
{
    return m_elements;
}

const FormatMetadataImpl::Elements & FormatMetadataImpl::getChildrenElements() const noexcept
{
    return m_elements;
}


namespace
{
void Combine(std::string & first, const std::string & second) noexcept
{
    if (!second.empty())
    {
        if (!first.empty())
        {
            first += " + ";
        }
        first += second;
    }
}
}

void FormatMetadataImpl::combine(const FormatMetadataImpl & rhs)
{
    if (this == &rhs)
        return;

    if (m_name != rhs.m_name)
    {
        throw Exception("Only FormatMetadata with the same name can be combined.");
    }

    Combine(m_value, rhs.m_value);

    // XML attribute names must be unique, so any rhs attributes that use an
    // existing name get merged by combining the value strings.  New rhs
    // attributes simply get added.
    for (auto & attrib : rhs.m_attributes)
    {
        if (!attrib.second.empty())
        {
            const int attribIndex = findNamedAttribute(attrib.first);
            if (attribIndex != -1)
            {
                Combine(m_attributes[attribIndex].second,
                        attrib.second);
            }
            else
            {
                m_attributes.push_back(attrib);
            }
        }
    }

    // All child elements for rhs simply get added to the object.  Note that
    // the results may need to be cleaned up later if the schema for the given
    // file format does not want more than one element with a given name.
    for (auto & element : rhs.m_elements)
    {
        m_elements.push_back(element);
    }
}

FormatMetadataImpl & FormatMetadataImpl::operator=(const FormatMetadataImpl & rhs)
{
    if (this != &rhs)
    {
        m_name       = rhs.m_name;
        m_value      = rhs.m_value;
        m_attributes = rhs.m_attributes;
        m_elements   = rhs.m_elements;
    }
    return *this;
}

bool FormatMetadataImpl::operator==(const FormatMetadataImpl & rhs) const
{
    if (this != &rhs)
    {
        return m_name       == rhs.m_name &&
               m_value      == rhs.m_value &&
               m_attributes == rhs.m_attributes &&
               m_elements   == rhs.m_elements;
    }
    return true;
}

int FormatMetadataImpl::getFirstChildIndex(const std::string & name) const noexcept
{
    int i = 0;
    for (auto & it : m_elements)
    {
        if (0 == Platform::Strcasecmp(name.c_str(), it.getElementName()))
        {
            return i;
        }
        ++i;
    }
    return -1;
}

int FormatMetadataImpl::findNamedAttribute(const std::string & name) const noexcept
{
    int i = 0;
    for (auto & it : m_attributes)
    {
        if (0 == Platform::Strcasecmp(name.c_str(), it.first.c_str()))
        {
            return i;
        }
        ++i;
    }
    return -1;
}

void FormatMetadataImpl::ValidateElementName(const std::string & name)
{
    if (name.empty())
    {
        throw Exception("FormatMetadata has to have a non-empty name.");
    }
    if (strcmp(name.c_str(), METADATA_ROOT) == 0)
    {
        throw Exception("'ROOT' is reversed for root FormatMetadata elements.");
    }
}

void FormatMetadataImpl::validateElementName(const std::string & name) const
{
    ValidateElementName(name);
    if (strcmp(m_name.c_str(), METADATA_ROOT) == 0)
    {
        throw Exception("FormatMetadata 'ROOT' element can't be renamed.");
    }
}

const char * FormatMetadataImpl::getElementName() const noexcept
{
    return m_name.c_str();
}

void FormatMetadataImpl::setElementName(const char * name)
{
    std::string nameStr{ name ? name : "" };
    validateElementName(nameStr);
    m_name = nameStr;
}

const char * FormatMetadataImpl::getElementValue() const noexcept
{
    return m_value.c_str();
}

void FormatMetadataImpl::setElementValue(const char * value)
{
    if (m_name == METADATA_ROOT)
    {
        throw Exception("FormatMetadata 'ROOT' can't have a value.");
    }
    m_value = std::string{ value ? value : "" };
}

int FormatMetadataImpl::getNumAttributes() const noexcept
{
    return (int)m_attributes.size();
}

const char * FormatMetadataImpl::getAttributeName(int i) const noexcept
{
    if (i >= 0 && i < getNumAttributes())
    {
        return m_attributes[i].first.c_str();
    }
    return "";
}

const char * FormatMetadataImpl::getAttributeValue(int i) const noexcept
{
    if (i >= 0 && i < getNumAttributes())
    {
        return m_attributes[i].second.c_str();
    }
    return "";
}

const char * FormatMetadataImpl::getAttributeValue(const char * name) const noexcept
{
    if (name && *name)
    {
        for (auto & it : m_attributes)
        {
            if (0 == Platform::Strcasecmp(name, it.first.c_str()))
            {
                return it.second.c_str();
            }
        }
    }
    return "";
}

const std::string & FormatMetadataImpl::getAttributeValueString(const char * name) const noexcept
{
    if (name && *name)
    {
        for (auto & it : m_attributes)
        {
            if (0 == Platform::Strcasecmp(name, it.first.c_str()))
            {
                return it.second;
            }
        }
    }
    static const std::string emptyString{ "" };
    return emptyString;
}

void FormatMetadataImpl::addAttribute(const char * name, const char * value)
{
    if (!name || !*name)
    {
        throw Exception("Attribute must have a non-empty name.");
    }
    Attribute attrib(name, value ? value : "");
    addAttribute(attrib);
}

int FormatMetadataImpl::getNumChildrenElements() const noexcept
{
    return (int)m_elements.size();
}

FormatMetadata & FormatMetadataImpl::getChildElement(int i)
{
    if (i >= 0 && i < getNumChildrenElements())
    {
        return m_elements[i];
    }
    throw Exception("Invalid index for metadata object.");
}

const FormatMetadata & FormatMetadataImpl::getChildElement(int i) const
{
    if (i >= 0 && i < getNumChildrenElements())
    {
        return m_elements[i];
    }
    throw Exception("Invalid index for metadata object.");
}

void FormatMetadataImpl::addChildElement(const char * name, const char * value)
{
    std::string nameStr(name ? name : "");
    ValidateElementName(nameStr);
    m_elements.emplace_back(nameStr, value ? value : "");
}

void FormatMetadataImpl::clear() noexcept
{
    m_attributes.clear();
    m_value = "";
    m_elements.clear();
}

FormatMetadata & FormatMetadataImpl::operator=(const FormatMetadata & rhs)
{
    if (this != &rhs)
    {
        const FormatMetadataImpl & metadata = dynamic_cast<const FormatMetadataImpl &>(rhs);
        *this = metadata;
    }
    return *this;
}

const char * FormatMetadataImpl::getName() const noexcept
{
    return getAttributeValueString(METADATA_NAME).c_str();
}

void FormatMetadataImpl::setName(const char * name) noexcept
{
    Attribute attrib(METADATA_NAME, name ? name : "");
    addAttribute(attrib);
}

const char * FormatMetadataImpl::getID() const noexcept
{
    return getAttributeValueString(METADATA_ID).c_str();
}

void FormatMetadataImpl::setID(const char * id) noexcept
{
    Attribute attrib(METADATA_ID, id ? id : "");
    addAttribute(attrib);
}

} // namespace OCIO_NAMESPACE

