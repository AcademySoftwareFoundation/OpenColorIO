// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "fileformats/xmlutils/XMLReaderHelper.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "Logging.h"
#include "ParseUtils.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
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
    os << "At line " << getXmlLineNumber() << ": ";
    os << error.c_str();
    throw Exception(os.str().c_str());
}

void XmlReaderElement::logParameterWarning(const char * param) const
{
    std::ostringstream oss;
    oss << getXmlFile().c_str() << "(" << getXmlLineNumber() << "): ";
    oss << "Unrecognized attribute '" << param << "' of '" << getName() << "'.";
    LogWarning(oss.str().c_str());
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
    oss << getXmlFile().c_str() << "(" << getXmlLineNumber() << "): ";
    oss << "Unrecognized element '" << getName();
    oss << "' where its parent is '" << getParent()->getName().c_str();
    oss << "' (" << getParent()->getXmlLineNumber() << ")";
    if (msg)
    {
        oss << ": " << msg;
    }
    oss << ".";

    LogWarning(oss.str());
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
        getParent()->appendMetadata(getIdentifier(), m_description);
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

} // namespace OCIO_NAMESPACE
