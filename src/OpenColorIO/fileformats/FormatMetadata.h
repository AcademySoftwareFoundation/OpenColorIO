// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_OPS_METADATA_H
#define INCLUDED_OCIO_OPS_METADATA_H

#include <list>
#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"

namespace OCIO_NAMESPACE
{


// ROOT is simply a placeholder name  for the top-level element, since each
// instance needs a name string.  (At the file level in CLF/CTF, the actual
// name would be ProcessList.  At the op level it would be the process node
// name such as Matrix.)  Doesn't get written to the XML.
static constexpr char METADATA_ROOT[] = "ROOT";

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

    FormatMetadataImpl();
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
    FormatMetadata & addChildElement(const char * name, const char * value) override;

    // Reset the contents of a metadata element. The value,
    // list of attributes and sub-elements are cleared.
    // The name is preserved.
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

} // namespace OCIO_NAMESPACE

#endif
