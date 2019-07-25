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

#ifndef INCLUDED_OCIO_OPS_METADATA_H
#define INCLUDED_OCIO_OPS_METADATA_H

#include <list>
#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"

OCIO_NAMESPACE_ENTER
{

static constexpr const char * METADATA_INFO = "Info";
static constexpr const char * METADATA_ROOT = "ROOT";
static constexpr const char * METADATA_NAME = "name";
static constexpr const char * METADATA_ID = "id";
static constexpr const char * METADATA_DESCRIPTION = "Description";


// ROOT is simply a placeholder name  for the top-level element, since each
// instance needs a name string.  (At the file level in CLF/CTF, the actual
// name would be ProcessList.  At the op level it would be the process node
// name such as Matrix.)  Doesn't get written to the XML.

// This class provides a hierarchical metadata container, similar to an XML Element.
// It contains:
// -- a name string (e.g. "Description")
// -- a value string (e.g. "updated viewing LUT")
// -- a list of attributes (name, value) string pairs (e.g. "version", "1.5")
// -- and a list of child sub-elements, which are also FormatMetadataImpl objects.
// 
// Root "ProcessList" metadata for CLF/CTF files may include attributes such as
// "name" and "id" and sub - elements such as "Info", "Description",
// "InputDescriptor", and "OutputDescriptor". This class is also used to hold
// the metadata within individual ops in a CLF/CTF file, which similarly may
// contain items such as name or id attributes and Description elements. 
// (It does not hold the actual LUT or parameter values.)
class FormatMetadataImpl : public FormatMetadata
{
public:
    typedef std::vector<FormatMetadataImpl> Elements;
    typedef std::pair<std::string, std::string> Attribute;
    typedef std::vector<Attribute> Attributes;

    FormatMetadataImpl() = delete;
    FormatMetadataImpl(const std::string & name);
    FormatMetadataImpl(const std::string & name,
                       const std::string & value);

    FormatMetadataImpl(const FormatMetadataImpl & other);
    FormatMetadataImpl(const FormatMetadata & other);
    ~FormatMetadataImpl();

    void setName(const std::string & name);
    void setValue(const std::string & value);

    const Attributes & getAttributes() const;

    // Retrieve the vector of elements under the metadata.
    Elements & getChildrenElements();
    const Elements & getChildrenElements() const;

    // Merge rhs into this. Expected to be used on root FormatMetadataImpl for ops.
    void combine(const FormatMetadataImpl & rhs);

    FormatMetadataImpl & operator=(const FormatMetadataImpl & rhs);
    bool operator==(const FormatMetadataImpl & rhs) const;

    // If child with a matching name exists, returns its index,
    // else returns -1.
    int getFirstChildIndex(const std::string & name) const;

    // If attribute with a matching name does not exists, it will return an empty string.
    const std::string & getAttributeValue(const std::string & name) const;

    //
    // FormatMetadata interface implementation.
    //

    const char * getName() const override;
    void setName(const char *) override;
    const char * getValue() const override;
    void setValue(const char *) override;
    int getNumAttributes() const override;
    const char * getAttributeName(int i) const override;
    const char * getAttributeValue(int i) const override;
    void addAttribute(const char * name, const char * value) override;

    int getNumChildrenElements() const override;
    FormatMetadata & getChildElement(int i) override;
    const FormatMetadata & getChildElement(int i) const override;
    FormatMetadata & addChildElement(const char * name) override;
    FormatMetadata & addChildElement(const char * name, const char * value) override;

    // Reset the contents of a metadata element. The name, value,
    // list of attributes and sub-elements are cleared.
    void clear() override;

    FormatMetadata & operator=(const FormatMetadata & rhs) override;

protected:
    // If the attribute already exists, the existing attribute's
    // value will be overwritten.
    void addAttribute(const Attribute & attribute);

    int findNamedAttribute(const std::string & name) const;

private:
    std::string      m_name;       // The element name
    std::string      m_value;      // The element value
    Attributes       m_attributes; // The element's list of attributes
    Elements         m_elements;   // The list of sub-elements
};

}
OCIO_NAMESPACE_EXIT

#endif
