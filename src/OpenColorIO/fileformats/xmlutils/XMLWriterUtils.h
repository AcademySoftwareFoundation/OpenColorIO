/*
Copyright (c) 2019 Autodesk Inc., et al.
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

#ifndef INCLUDED_OCIO_FILEFORMATS_XML_XMLWRITERUTILS_H
#define INCLUDED_OCIO_FILEFORMATS_XML_XMLWRITERUTILS_H

#include <list>
#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

OCIO_NAMESPACE_ENTER
{

// Provides all services to write xml to an output stream.
class XmlFormatter final
{
public:

    typedef std::pair<std::string, std::string> Attribute;
    typedef std::vector<Attribute> Attributes;

public:
    XmlFormatter() = delete;
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
    XmlElementWriter(XmlFormatter & formatter);
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
    XmlScopeIndent(XmlFormatter & formatter);

    ~XmlScopeIndent();

private:
    XmlFormatter & m_formatter;
};

}
OCIO_NAMESPACE_EXIT

#endif
