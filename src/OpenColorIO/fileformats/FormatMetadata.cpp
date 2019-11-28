// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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

FormatMetadata::FormatMetadata()
{
}

FormatMetadata::~FormatMetadata()
{
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
        throw Exception("FormatMetadata has to have a non empty name.");
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

void FormatMetadataImpl::setName(const std::string & name)
{
    if (name.empty())
    {
        throw Exception("FormatMetadata has to have a non empty name.");
    }

    m_name = name;
}

void FormatMetadataImpl::setValue(const std::string & value)
{
    m_value = value;
}

const FormatMetadataImpl::Attributes & FormatMetadataImpl::getAttributes() const
{
    return m_attributes;
}

void FormatMetadataImpl::addAttribute(const Attribute & attribute)
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

FormatMetadataImpl::Elements & FormatMetadataImpl::getChildrenElements()
{
    return m_elements;
}

const FormatMetadataImpl::Elements & FormatMetadataImpl::getChildrenElements() const
{
    return m_elements;
}


namespace
{
void Combine(std::string & first, const std::string & second)
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

int FormatMetadataImpl::getFirstChildIndex(const std::string & name) const
{
    int i = 0;
    for (auto & it : m_elements)
    {
        if (0 == Platform::Strcasecmp(name.c_str(), it.getName()))
        {
            return i;
        }
        ++i;
    }
    return -1;
}

int FormatMetadataImpl::findNamedAttribute(const std::string & name) const
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

const std::string & FormatMetadataImpl::getAttributeValue(const std::string & name) const
{
    for (auto & it : m_attributes)
    {
        if (0 == Platform::Strcasecmp(name.c_str(), it.first.c_str()))
        {
            return it.second;
        }
    }
    static const std::string emptyString("");
    return emptyString;
}

const char * FormatMetadataImpl::getName() const
{
    return m_name.c_str();
}

void FormatMetadataImpl::setName(const char * name)
{
    m_name = name;
    if (m_name.empty())
    {
        throw Exception("FormatMetadata has to have a non empty name.");
    }
}

const char * FormatMetadataImpl::getValue() const
{
    return m_value.c_str();
}

void FormatMetadataImpl::setValue(const char * value)
{
    m_value = value;
}

int FormatMetadataImpl::getNumAttributes() const
{
    return (int)m_attributes.size();
}

const char * FormatMetadataImpl::getAttributeName(int i) const
{
    if (i >= 0 && i < getNumAttributes())
    {
        return m_attributes[i].first.c_str();
    }
    return nullptr;
}

const char * FormatMetadataImpl::getAttributeValue(int i) const
{
    if (i >= 0 && i < getNumAttributes())
    {
        return m_attributes[i].second.c_str();
    }
    return nullptr;
}

void FormatMetadataImpl::addAttribute(const char * name, const char * value)
{
    Attribute attrib(name, value);
    addAttribute(attrib);
}

int FormatMetadataImpl::getNumChildrenElements() const
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

FormatMetadata & FormatMetadataImpl::addChildElement(const char * name, const char * value)
{
    m_elements.emplace_back(name, value);
    return m_elements.back();
}

void FormatMetadataImpl::clear()
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

} // namespace OCIO_NAMESPACE

