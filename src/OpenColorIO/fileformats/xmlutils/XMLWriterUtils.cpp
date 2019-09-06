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

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/xmlutils/XMLWriterUtils.h"
#include "ParseUtils.h"

OCIO_NAMESPACE_ENTER
{

XmlFormatter::XmlFormatter(std::ostream& stream)
 : m_stream(stream)
{

}

XmlFormatter::~XmlFormatter()
{

}

void XmlFormatter::incrementIndent()
{
    ++m_indentLevel;
}

void XmlFormatter::decrementIndent()
{
    --m_indentLevel;
}

void XmlFormatter::writeStartTag(const std::string & tagName,
                                 const Attributes & attributes)
{
    writeIndent();
    m_stream << "<" << tagName;

    for (Attributes::const_iterator it(attributes.begin()); it != attributes.end(); it++)
    {
        m_stream << " " << (*it).first << "=\"";
        writeString((*it).second);
        m_stream << "\"";
    }

    m_stream << ">" << std::endl;
}

void XmlFormatter::writeStartTag(const std::string & tagName)
{
    Attributes atts;
    writeStartTag(tagName, atts);
}

void XmlFormatter::writeEndTag(const std::string & tagName)
{
    writeIndent();
    m_stream << "</" << tagName << ">" << std::endl;
}

void XmlFormatter::writeContentTag(const std::string & tagName,
                                   const std::string & content)
{
    Attributes atts;
    writeContentTag(tagName, atts, content);
}

void XmlFormatter::writeContentTag(const std::string & tagName,
                                   const Attributes & attributes,
                                   const std::string & content)
{
    writeIndent();
    m_stream << "<" << tagName;
    for (Attributes::const_iterator it(attributes.begin()); it != attributes.end(); it++)
    {
        m_stream << " " << (*it).first << "=\"";
        writeString((*it).second);
        m_stream << "\"";
    }
    m_stream << ">";
    writeString(content);
    m_stream << "</" << tagName << ">" << std::endl;
}

// Write the content using escaped characters if needed.
void XmlFormatter::writeContent(const std::string & content)
{
    writeIndent();
    writeString(content);
    m_stream << std::endl;
}

void XmlFormatter::writeEmptyTag(const std::string & tagName,
                                 const Attributes & attributes)
{
    writeIndent();
    m_stream << "<" << tagName;

    for (Attributes::const_iterator it(attributes.begin());
        it != attributes.end(); it++)
    {
        m_stream << " " << (*it).first << "=\"";
        writeString((*it).second);
        m_stream << "\"";
    }

    // Note we close the tag, no end tag is needed.
    m_stream << " />" << std::endl;
}

std::ostream & XmlFormatter::getStream()
{
    return m_stream;
}

void XmlFormatter::writeIndent()
{
    for (int i = 0; i < m_indentLevel; ++i)
    {
        m_stream << "    ";
    }
}

void XmlFormatter::writeString(const std::string & content)
{
    m_stream << ConvertSpecialCharToXmlToken(content);
}

XmlScopeIndent::XmlScopeIndent(XmlFormatter & formatter)
    : m_formatter(formatter)
{
    m_formatter.incrementIndent();
}

XmlScopeIndent::~XmlScopeIndent()
{
    m_formatter.decrementIndent();
}


XmlElementWriter::XmlElementWriter(XmlFormatter & formatter)
    : m_formatter(formatter)
{
}

XmlElementWriter::~XmlElementWriter()
{
}

}
OCIO_NAMESPACE_EXIT
