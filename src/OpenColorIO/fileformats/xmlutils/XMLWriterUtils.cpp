// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/xmlutils/XMLWriterUtils.h"
#include "ParseUtils.h"

namespace OCIO_NAMESPACE
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

    m_stream << ">\n";
}

void XmlFormatter::writeStartTag(const std::string & tagName)
{
    Attributes atts;
    writeStartTag(tagName, atts);
}

void XmlFormatter::writeEndTag(const std::string & tagName)
{
    writeIndent();
    m_stream << "</" << tagName << ">\n";
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
    m_stream << "</" << tagName << ">\n";
}

// Write the content using escaped characters if needed.
void XmlFormatter::writeContent(const std::string & content)
{
    writeIndent();
    writeString(content);
    m_stream << "\n";
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
    m_stream << " />\n";
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

} // namespace OCIO_NAMESPACE
