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

#include "fileformats/xmlutils/XMLReaderHelper.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "Logging.h"
#include "ParseUtils.h"
#include "Platform.h"

OCIO_NAMESPACE_ENTER
{
XmlReaderElement::XmlReaderElement(const std::string & name,
                                   unsigned int xmlLineNumber,
                                   const std::string & xmlFile)
    : m_name(name)
    , m_xmlLineNumber(xmlLineNumber)
    , m_xmlFile(xmlFile)
{
}

XmlReaderElement::~XmlReaderElement()
{
}

const std::string& XmlReaderElement::getXmlFile() const
{
    static const std::string emptyName("File name not specified");
    return m_xmlFile.empty() ? emptyName : m_xmlFile;
}

void XmlReaderElement::setContext(const std::string & name,
                                  unsigned int xmlLineNumber,
                                  const std::string  & xmlFile)
{
    m_name = name;
    m_xmlLineNumber = xmlLineNumber;
    m_xmlFile = xmlFile;
}

void XmlReaderElement::throwMessage(const std::string & error) const
{
    std::ostringstream os;
    os << "Error parsing file (" << getXmlFile().c_str() << "). ";
    os << "Error is: " << error.c_str();
    os << " At line (" << getXmlLineNumber() << ")";
    throw Exception(os.str().c_str());
}

///////////////////////////////////////////////////////////////////////////////

const std::string & XmlReaderDummyElt::DummyParent::getIdentifier() const
{
    static const std::string identifier = "Unknown";
    return identifier;
}

XmlReaderDummyElt::XmlReaderDummyElt(const std::string & name,
                                     ElementRcPtr pParent,
                                     unsigned int xmlLineNumber,
                                     const std::string & xmlFile,
                                     const char * msg)
    : XmlReaderPlainElt(name,
                        std::make_shared<DummyParent>(pParent),
                        xmlLineNumber,
                        xmlFile)
{
    std::ostringstream oss;
    oss << "Ignore element '" << getName().c_str();
    oss << "' (line " << getXmlLineNumber() << ") ";
    oss << "where its parent is '" << getParent()->getName().c_str();
    oss << "' (line " << getParent()->getXmlLineNumber() << ") ";
    oss << (msg ? msg : "") << ": " << getXmlFile().c_str();

    LogDebug(oss.str());
}

const std::string & XmlReaderDummyElt::getIdentifier() const
{
    static std::string emptyName;
    return emptyName;
}

///////////////////////////////////////////////////////////////////////////////

void XmlReaderDescriptionElt::end()
{
    if (m_changed)
    {
        // Note: eXpat automatically replaces escaped characters with 
        //       their original values.
        getParent()->appendDescription(m_description);
    }
}

///////////////////////////////////////////////////////////////////////////////

XmlReaderElementStack::XmlReaderElementStack()
{
}

XmlReaderElementStack::~XmlReaderElementStack()
{
    clear();
}

unsigned XmlReaderElementStack::size() const
{
    return (const unsigned)m_elms.size();
}

bool XmlReaderElementStack::empty() const
{
    return m_elms.empty();
}

void XmlReaderElementStack::push_back(ElementRcPtr pElt)
{
    m_elms.push_back(pElt);
}

void XmlReaderElementStack::pop_back()
{
    m_elms.pop_back();
}

ElementRcPtr XmlReaderElementStack::back() const
{
    return m_elms.back();
}

ElementRcPtr XmlReaderElementStack::front() const
{
    return m_elms.front();
}

void XmlReaderElementStack::clear()
{
    m_elms.clear();
}

///////////////////////////////////////////////////////////////////////////////

XmlReaderSOPValueElt::XmlReaderSOPValueElt(
    const std::string & name,
    ContainerEltRcPtr pParent,
    unsigned int xmlLineNumber,
    const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

XmlReaderSOPValueElt::~XmlReaderSOPValueElt()
{
}

void XmlReaderSOPValueElt::start(const char ** atts)
{
    m_contentData = "";
}

void XmlReaderSOPValueElt::end()
{
    Trim(m_contentData);

    std::vector<double> data;

    try
    {
        data = GetNumbers<double>(m_contentData.c_str(), m_contentData.size());
    }
    catch (Exception&)
    {
        const std::string s = TruncateString(m_contentData.c_str(),
                                             m_contentData.size());
        std::ostringstream oss;
        oss << "Illegal values '" << s << "' in " << getTypeName();
        throwMessage(oss.str());
    }

    if (data.size() != 3)
    {
        throwMessage("SOPNode: 3 values required.");
    }

    XmlReaderSOPNodeBaseElt* pSOPNodeElt = 
        dynamic_cast<XmlReaderSOPNodeBaseElt*>(getParent().get());
    CDLOpDataRcPtr pCDL = pSOPNodeElt->getCDL();

    if (0 == strcmp(getName().c_str(), TAG_SLOPE))
    {
        pCDL->setSlopeParams(CDLOpData::ChannelParams(data[0], data[1], data[2]));
        pSOPNodeElt->setIsSlopeInit(true);
    }
    else if (0 == strcmp(getName().c_str(), TAG_OFFSET))
    {
        pCDL->setOffsetParams(CDLOpData::ChannelParams(data[0], data[1], data[2]));
        pSOPNodeElt->setIsOffsetInit(true);
    }
    else if (0 == strcmp(getName().c_str(), TAG_POWER))
    {
        pCDL->setPowerParams(CDLOpData::ChannelParams(data[0], data[1], data[2]));
        pSOPNodeElt->setIsPowerInit(true);
    }
}

void XmlReaderSOPValueElt::setRawData(const char * str,
                                      size_t len,
                                      unsigned int xmlLine)
{
    m_contentData += std::string(str, len) + " ";
}

///////////////////////////////////////////////////////////////////////////////

XmlReaderSaturationElt::XmlReaderSaturationElt(const std::string & name,
                                               ContainerEltRcPtr pParent,
                                               unsigned int xmlLineNumber,
                                               const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

XmlReaderSaturationElt::~XmlReaderSaturationElt()
{
}

void XmlReaderSaturationElt::start(const char ** /* atts */)
{
    m_contentData = "";
}

void XmlReaderSaturationElt::end()
{
    Trim(m_contentData);

    std::vector<double> data;

    try
    {
        data = GetNumbers<double>(m_contentData.c_str(), m_contentData.size());
    }
    catch (Exception& /* ce */)
    {
        const std::string s = TruncateString(m_contentData.c_str(),
                                             m_contentData.size());
        std::ostringstream oss;
        oss << "Illegal values '" << s << "' in " << getTypeName();
        throwMessage(oss.str());
    }

    if (data.size() != 1)
    {
        throwMessage("SatNode: non-single value. ");
    }

    XmlReaderSatNodeBaseElt* pSatNodeElt =
        dynamic_cast<XmlReaderSatNodeBaseElt*>(getParent().get());
    CDLOpDataRcPtr pCDL = pSatNodeElt->getCDL();

    if (0 == strcmp(getName().c_str(), TAG_SATURATION))
    {
        pCDL->setSaturation(data[0]);
    }
}

void XmlReaderSaturationElt::setRawData(const char* str, size_t len, unsigned int)
{
    m_contentData += std::string(str, len) + " ";
}

}
OCIO_NAMESPACE_EXIT
