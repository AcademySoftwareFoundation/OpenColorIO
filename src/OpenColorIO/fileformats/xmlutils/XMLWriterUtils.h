// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_FILEFORMATS_XML_XMLWRITERUTILS_H
#define INCLUDED_OCIO_FILEFORMATS_XML_XMLWRITERUTILS_H

#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

// Provides all services to write xml to an output stream.
class XmlFormatter final
{
public:

    using Attribute  = std::pair<std::string, std::string>;
    using Attributes = std::vector<Attribute>;

public:
    XmlFormatter() = delete;
    XmlFormatter(const XmlFormatter &) = delete;
    XmlFormatter& operator=(const XmlFormatter &) = delete;

    XmlFormatter(std::ostream& stream);
    ~XmlFormatter();

    void incrementIndent();
    void decrementIndent();

    // Write a start Element on a standalone line.
    void writeStartTag(const std::string & tagName,
                       const Attributes & attributes);

    // Write a start Element on a standalone line.
    void writeStartTag(const std::string & tagName);

    // Write an end Element on a standalone line.
    void writeEndTag(const std::string & tagName);

    // Write \<tagName\>content\</tagName\> on a standalone line.
    void writeContentTag(const std::string & tagName,
                         const std::string & content);

    // Write \<tagName\>content\</tagName\> on a standalone line.
    void writeContentTag(const std::string & tagName,
                         const Attributes & attributes,
                         const std::string & content);

    // Write the content using escaped characters if needed.
    void writeContent(const std::string & content);

    // Write an empty Element on a standalone line.
    // In XML parlance, an empty Element is an Element without content
    // and without children and which does not have a separate end tag.
    void writeEmptyTag(const std::string & tagName,
                       const Attributes & attributes);

    std::ostream & getStream();

private:
    std::ostream & m_stream;
    int m_indentLevel = 0;

    void writeIndent();
    void writeString(const std::string & content);
};

// Base class to write transform element xml writer.
class XmlElementWriter
{
public:
    XmlElementWriter() = delete;
    XmlElementWriter(XmlElementWriter &) = delete;
    XmlElementWriter& operator=(XmlElementWriter &) = delete;

    explicit XmlElementWriter(XmlFormatter & formatter);
    virtual ~XmlElementWriter();

    virtual void write() const = 0;

protected:
    XmlFormatter & m_formatter;
};

// The ScopeIndent object's role is to increment the formatter
// indentation at construction, and decrement it at destruction.
class XmlScopeIndent
{
public:
    XmlScopeIndent() = delete;
    XmlScopeIndent(XmlScopeIndent &) = delete;
    XmlScopeIndent & operator=(XmlScopeIndent &) = delete;

    explicit XmlScopeIndent(XmlFormatter & formatter);
    ~XmlScopeIndent();

private:
    XmlFormatter & m_formatter;
};

} // namespace OCIO_NAMESPACE

#endif
