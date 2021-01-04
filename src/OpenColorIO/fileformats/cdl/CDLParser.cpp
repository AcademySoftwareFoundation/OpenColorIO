// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include "expat.h"
#include "fileformats/cdl/CDLParser.h"
#include "fileformats/cdl/CDLReaderHelper.h"
#include "fileformats/xmlutils/XMLReaderHelper.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "transforms/CDLTransform.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

class CDLParser::Impl
{
public:
    Impl() = delete;
    explicit Impl(const std::string & fileName);
    Impl(const Impl &) = delete;
    Impl & operator=(const Impl &) = delete;

    ~Impl();

    // Parse a CDL stream.
    void parse(std::istream & istream);

    const CDLParsingInfoRcPtr & getCDLParsingInfo() const;

    unsigned int getXmlLocation() const;

    const std::string& getXmlFilename() const;

    ElementRcPtr getBackElement() const;

    // Check is the last element on the stack is an instance of the type T.
    template<typename T>
    bool isBackElementInstanceOf() const;

    // Create a dummy element.
    DummyEltRcPtr createDummyElement(const std::string& name,
                                     const std::string& msg) const;

    // Create an element of type T.
    template<typename T>
    OCIO_SHARED_PTR<T> createElement(const std::string& name) const;

    bool isCC() const
    {
        return m_isCC;
    }
    bool isCCC() const
    {
        return m_isCCC;
    }

protected:
    // Parse a line.
    void parse(const std::string & buffer, bool lastLine);

    std::string loadHeader(std::istream & istream);

    void throwMessage(const std::string & error) const;

    // Start the parsing of one element in the CDL schema.
    static void StartElementHandlerCDL(void *userData,
                                       const XML_Char *name,
                                       const XML_Char **atts);

    // Start the parsing of one element in the CCC schema.
    static void StartElementHandlerCCC(void *userData,
                                       const XML_Char *name,
                                       const XML_Char **atts);

    // Start the parsing of one element in the CC schema.
    static void StartElementHandlerCC(void *userData,
                                      const XML_Char *name,
                                      const XML_Char **atts);

    // Verify if the data for starting a CDL/CCC/CC element is valid.
    static bool IsValidStartElement(CDLParser::Impl* pImpl,
                                    const XML_Char *name);

    // Helper methods to handle the start of specific elements. 

    // Handle the start of a ColorDecisionList element.
    static bool HandleColorDecisionListStartElement(CDLParser::Impl* pImpl,
                                                    const XML_Char *name);

    // Handle the start of a ColorDecision element.
    static bool HandleColorDecisionStartElement(CDLParser::Impl* pImpl,
                                                const XML_Char *name);

    // Handle the start of a ColorCorrectionCollection element.
    static bool HandleColorCorrectionCollectionStartElement(
        CDLParser::Impl* pImpl, const XML_Char *name);

    // Handle the start of a ColorCorrection element in the CDL schema.
    static bool HandleColorCorrectionCDLStartElement(CDLParser::Impl* pImpl,
                                                     const XML_Char *name);

    // Handle the start of a ColorCorrection element in the CCC schema.
    static bool HandleColorCorrectionCCCStartElement(CDLParser::Impl* pImpl,
                                                     const XML_Char *name);

    // Handle the start of a ColorCorrection element in the CC schema.
    static bool HandleColorCorrectionCCStartElement(CDLParser::Impl* pImpl,
                                                    const XML_Char *name);

    // Handle the start of a SOPNode element.
    static bool HandleSOPNodeStartElement(CDLParser::Impl* pImpl,
                                          const XML_Char *name);

    // Handle the start of a SatNode element.
    static bool HandleSatNodeStartElement(CDLParser::Impl* pImpl,
                                          const XML_Char *name);

    // Handle the start of a terminal/leaf element.
    // Specific element handled are:
    // Slope, Offset, Power, and Saturation.
    static bool HandleTerminalStartElement(CDLParser::Impl* pImpl,
                                           const XML_Char *name);

    // Default handler for the start of an unknown element.
    static bool HandleUnknownStartElement(CDLParser::Impl* pImpl,
                                          const XML_Char *name);

    // End the parsing of one element.
    static void EndElementHandler(void *userData,
                                  const XML_Char *name);

    // Handle of strings within an element.
    static void CharacterDataHandler(void *userData,
                                     const XML_Char *s, int len);

    // Check if a description tag matches the required schema.
    static bool IsValidDescriptionTag(const std::string& currentId,
                                      const std::string& parentId);

    // Reset the parsing.
    void reset();

    // Use the buffer string to detect the input schema and
    // initialize the related element handlers.
    void initializeHandlers(const char* buffer);

    // Validate if the parsed file or buffer was successful.
    void validateParsing() const;

private:
    XML_Parser m_parser;
    XmlReaderElementStack m_elms;
    CDLParsingInfoRcPtr m_parsingInfo;
    unsigned int m_lineNumber;
    std::string m_fileName;
    bool m_isCC;
    bool m_isCCC;
};

CDLParser::Impl::Impl(const std::string & fileName)
    : m_parser(XML_ParserCreate(NULL))
    , m_lineNumber(0)
    , m_fileName(fileName)
    , m_isCC(false)
    , m_isCCC(false)
{
}

CDLParser::Impl::~Impl()
{
    XML_ParserFree(m_parser);
    reset();
}

std::string CDLParser::Impl::loadHeader(std::istream & istream)
{
    const unsigned int limit = 5 * 1024; // 5 kilobytes.
    char line[limit + 1];

    std::string header;
    unsigned int sizeProcessed = 0;
    while (istream.good() && (sizeProcessed < limit))
    {
        istream.getline(line, limit);
        header += std::string(line) + " ";
        sizeProcessed += (unsigned int)strlen(line);
    }

    istream.clear();
    istream.seekg(0, istream.beg);

    return header;
}

void CDLParser::Impl::parse(std::istream & istream)
{
    reset();

    const std::string header(loadHeader(istream));
    initializeHandlers(header.c_str());

    std::string line;
    m_lineNumber = 0;
    while (istream.good())
    {
        std::getline(istream, line);
        line.push_back('\n');
        ++m_lineNumber;

        parse(line, !istream.good());
    }

    validateParsing();
}

void CDLParser::Impl::throwMessage(const std::string & error) const
{
    std::ostringstream os;
    os << "Error parsing ";
    if (m_isCC)
    {
        os << CDL_TAG_COLOR_CORRECTION;
    }
    else if (m_isCCC)
    {
        os << CDL_TAG_COLOR_CORRECTION_COLLECTION;
    }
    else
    {
        os << CDL_TAG_COLOR_DECISION_LIST;
    }
    os << " (";
    os << m_fileName.c_str() << "). ";
    os << "Error is: " << error.c_str();
    os << ". At line (" << m_lineNumber << ")";
    throw Exception(os.str().c_str());
}

void CDLParser::Impl::parse(const std::string & buffer, bool lastLine)
{
    const int done = lastLine?1:0;

    if (XML_STATUS_ERROR == XML_Parse(m_parser,
                                      buffer.c_str(),
                                      (int)buffer.size(),
                                      done))
    {
        XML_Error eXpatErrorCode = XML_GetErrorCode(m_parser);
        if (eXpatErrorCode == XML_ERROR_TAG_MISMATCH)
        {
            if (!m_elms.empty())
            {
                // It could be an Op or an Attribute.
                std::string error("XML parsing error "
                                  "(no closing tag for '");
                error += m_elms.back()->getName().c_str();
                error += "'). ";
                throwMessage(error);
            }
            else
            {
                // Completely lost, something went wrong,
                // but nothing detected with the stack.
                static const std::string error("XML parsing error "
                                               "(unbalanced element tags). ");
                throwMessage(error);
            }
        }
        else
        {
            std::string error("XML parsing error: ");
            error += XML_ErrorString(XML_GetErrorCode(m_parser));
            throwMessage(error);
        }
    }

}

const CDLParsingInfoRcPtr & CDLParser::Impl::getCDLParsingInfo() const
{
    return m_parsingInfo;
}

bool FindRootElement(const std::string& header, const std::string& tag)
{
    // Find root element tag at beginning of the file.
    const std::string pattern = std::string("<") + tag;
    return strstr(header.c_str(), pattern.c_str()) != 0x0;
}

void CDLParser::Impl::initializeHandlers(const char* buffer)
{
    XML_SetUserData(m_parser, this);
    XML_SetCharacterDataHandler(m_parser, CharacterDataHandler);

    if (FindRootElement(buffer, CDL_TAG_COLOR_DECISION_LIST))
    {
        XML_SetElementHandler(m_parser,
                              StartElementHandlerCDL,
                              EndElementHandler);
    }
    else if (FindRootElement(buffer, CDL_TAG_COLOR_CORRECTION_COLLECTION))
    {
        XML_SetElementHandler(m_parser,
                              StartElementHandlerCCC,
                              EndElementHandler);
        m_isCCC = true;
    }
    else if (FindRootElement(buffer, CDL_TAG_COLOR_CORRECTION))
    {
        XML_SetElementHandler(m_parser,
                              StartElementHandlerCC,
                              EndElementHandler);
        m_isCC = true;

        // If parsing a CC, initialize the TransformList explicitly.
        m_parsingInfo = std::make_shared<CDLParsingInfo>();
    }
    else
    {
        throwMessage("Missing CDL tag");
    }
}

void CDLParser::Impl::validateParsing() const
{
    if (!m_elms.empty())
    {
        std::string error("CDL parsing error (no closing tag for '");
        error += m_elms.back()->getName().c_str();
        error += ")";
        throwMessage(error);
    }

    const CDLTransformVec& pTransformList = getCDLParsingInfo()->m_transforms;
    for (size_t i = 0; i<pTransformList.size(); ++i)
    {
        const CDLTransformRcPtr& pTransform = pTransformList.at(i);
        if (pTransform.use_count() == 0)
        {
            static const std::string error("CDL parsing error: "
                                           "Invalid transform");
            throwMessage(error);
        }
    }
}

unsigned int CDLParser::Impl::getXmlLocation() const
{
    return m_lineNumber;
}

const std::string& CDLParser::Impl::getXmlFilename() const
{
    static const std::string emptyName("File name not specified");
    return m_fileName.empty() ? emptyName : m_fileName;
}

void CDLParser::Impl::reset()
{
    if (m_parsingInfo)
    {
        m_parsingInfo->m_transforms.clear();
    }

    m_elms.clear();

    m_lineNumber = 0;
    m_fileName = "";
    m_isCC = false;
    m_isCCC = false;
}

DummyEltRcPtr CDLParser::Impl::createDummyElement(
    const std::string& name, const std::string& msg) const
{
    return std::make_shared<XmlReaderDummyElt>(name,
                                               getBackElement(),
                                               getXmlLocation(),
                                               getXmlFilename(),
                                               msg.c_str());
}

template<typename T>
OCIO_SHARED_PTR<T> CDLParser::Impl::createElement(const std::string& name) const
{
    auto pContainer = std::dynamic_pointer_cast<XmlReaderContainerElt>(getBackElement());
    return std::make_shared<T>(name,
                               pContainer,
                               getXmlLocation(),
                               getXmlFilename());
}

ElementRcPtr CDLParser::Impl::getBackElement() const
{
    return m_elms.size() ? m_elms.back() : ElementRcPtr();
}

template<typename T>
bool CDLParser::Impl::isBackElementInstanceOf() const
{
    return dynamic_cast<T*>(getBackElement().get()) != 0x0;
}

bool CDLParser::Impl::IsValidDescriptionTag(const std::string& currentId,
                                            const std::string& parentId)
{
    const char* currId = currentId.c_str();
    const char* parId = parentId.c_str();

    const bool isDesc = (0 == strcmp(currId, TAG_DESCRIPTION));
    const bool isInputViewingDesc =
        (0 == strcmp(currId, METADATA_INPUT_DESCRIPTION)) ||
        (0 == strcmp(currId, METADATA_VIEWING_DESCRIPTION));
    const bool isSOPSat = (0 == strcmp(parId, TAG_SOPNODE)) ||
                          (0 == strcmp(parId, TAG_SATNODE)) ||
                          (0 == strcmp(parId, TAG_SATNODEALT));

    return isDesc || (isInputViewingDesc && !isSOPSat);
}

void CDLParser::Impl::StartElementHandlerCDL(void *userData,
                                             const XML_Char *name,
                                             const XML_Char **atts)
{
    CDLParser::Impl* pImpl = (CDLParser::Impl*)userData;
    if (IsValidStartElement(pImpl, name))
    {
        if (HandleColorDecisionListStartElement(pImpl, name) ||
            HandleColorDecisionStartElement(pImpl, name) ||
            HandleColorCorrectionCDLStartElement(pImpl, name) ||
            HandleSOPNodeStartElement(pImpl, name) ||
            HandleSatNodeStartElement(pImpl, name) ||
            HandleTerminalStartElement(pImpl, name) ||
            HandleUnknownStartElement(pImpl, name))
        {
            pImpl->m_elms.back()->start(atts);
        }
    }
}

void CDLParser::Impl::StartElementHandlerCCC(void *userData,
                                             const XML_Char *name,
                                             const XML_Char **atts)
{
    CDLParser::Impl* pImpl = (CDLParser::Impl*)userData;
    if (IsValidStartElement(pImpl, name))
    {
        if (HandleColorCorrectionCollectionStartElement(pImpl, name) ||
            HandleColorCorrectionCCCStartElement(pImpl, name) ||
            HandleSOPNodeStartElement(pImpl, name) ||
            HandleSatNodeStartElement(pImpl, name) ||
            HandleTerminalStartElement(pImpl, name) ||
            HandleUnknownStartElement(pImpl, name))
        {
            pImpl->m_elms.back()->start(atts);
        }
    }
}

void CDLParser::Impl::StartElementHandlerCC(void *userData,
                                            const XML_Char *name,
                                            const XML_Char **atts)
{
    CDLParser::Impl* pImpl = (CDLParser::Impl*)userData;
    if (IsValidStartElement(pImpl, name))
    {
        if (HandleColorCorrectionCCStartElement(pImpl, name) ||
            HandleSOPNodeStartElement(pImpl, name) ||
            HandleSatNodeStartElement(pImpl, name) ||
            HandleTerminalStartElement(pImpl, name) ||
            HandleUnknownStartElement(pImpl, name))
        {
            pImpl->m_elms.back()->start(atts);
        }
    }
}

bool CDLParser::Impl::IsValidStartElement(CDLParser::Impl* pImpl,
                                          const XML_Char *name)
{
    if (!pImpl)
    {
        throw Exception("Internal CDL parsing error.");
    }

    if (!name || !*name)
    {
        pImpl->throwMessage("Internal parsing error");
    }

    return true;
}

bool CDLParser::Impl::HandleColorDecisionListStartElement(CDLParser::Impl* pImpl,
                                                          const XML_Char *name)
{
    // Handle the ColorDecisionList element.
    if ((0 == strcmp(name, CDL_TAG_COLOR_DECISION_LIST)))
    {
        ElementRcPtr pElt;
        if (!pImpl->m_parsingInfo || pImpl->m_parsingInfo->m_transforms.empty())
        {
            pElt = std::make_shared<CDLReaderColorDecisionListElt>(
                name,
                pImpl->getXmlLocation(),
                pImpl->getXmlFilename());

            // Bind the reader's CDLTransformList to the one in the
            // ColorDecisionList element.
            CDLReaderColorDecisionListElt* pCDLElt =
                dynamic_cast<CDLReaderColorDecisionListElt*>(pElt.get());
            pImpl->m_parsingInfo = pCDLElt->getCDLParsingInfo();
        }
        else
        {
            pElt = pImpl->createDummyElement(name, ": The ColorDecisionList "
                                                   "already exists");
        }

        pImpl->m_elms.push_back(pElt);
        return true;
    }
    return false;
}

bool CDLParser::Impl::HandleColorDecisionStartElement(CDLParser::Impl* pImpl,
                                                      const XML_Char *name)
{
    // Handle the ColorDecision element, if parsing a CDL.
    if ((0 == strcmp(name, CDL_TAG_COLOR_DECISION)))
    {
        ElementRcPtr pElt;
        if (pImpl->isBackElementInstanceOf<CDLReaderColorDecisionListElt>())
        {
            pElt = pImpl->createElement<CDLReaderColorDecisionElt>(name);
        }
        else
        {
            pElt = pImpl->createDummyElement(name, ": ColorDecision must be "
                                                   "under a ColorDecisionList");
        }

        pImpl->m_elms.push_back(pElt);
        return true;
    }
    return false;
}

bool CDLParser::Impl::HandleColorCorrectionCollectionStartElement(
    CDLParser::Impl* pImpl, const XML_Char *name)
{
    // Handle the ColorCorrectionCollection element.
    if ((0 == strcmp(name, CDL_TAG_COLOR_CORRECTION_COLLECTION)))
    {
        ElementRcPtr pElt;
        if (!pImpl->m_parsingInfo || pImpl->m_parsingInfo->m_transforms.empty())
        {
            pElt = std::make_shared<CDLReaderColorCorrectionCollectionElt>(
                name, pImpl->getXmlLocation(), pImpl->getXmlFilename());

            // Bind the reader's CDLTransformList to the one in the
            // ColorCorrectionCollection element.
            CDLReaderColorCorrectionCollectionElt* pCCCElt =
                dynamic_cast<CDLReaderColorCorrectionCollectionElt*>(pElt.get());
            pImpl->m_parsingInfo = pCCCElt->getCDLParsingInfo();
        }
        else
        {
            pElt = pImpl->createDummyElement(
                name,
                ": The ColorCorrectionCollection already exists");
        }

        pImpl->m_elms.push_back(pElt);
        return true;
    }
    return false;
}

bool CDLParser::Impl::HandleColorCorrectionCDLStartElement(CDLParser::Impl* pImpl, const XML_Char *name)
{
    // Handle the ColorCorrection element
    if (0 == strcmp(name, CDL_TAG_COLOR_CORRECTION))
    {
        ElementRcPtr pElt;

        // If parsing a CDL, make sure the ColorCorrection is under
        // a ColorDecision element.
        if (pImpl->isBackElementInstanceOf<CDLReaderColorDecisionElt>())
        {
            pElt = pImpl->createElement<CDLReaderColorCorrectionElt>(name);

            // Bind the ColorCorrection element's CDLTransformList to the
            // one in the ColorDecisionList element.
            CDLReaderColorCorrectionElt* pCCElt =
                dynamic_cast<CDLReaderColorCorrectionElt*>(pElt.get());
            CDLReaderColorDecisionElt* pCDElt =
                dynamic_cast<CDLReaderColorDecisionElt*>(pCCElt->getParent().get());
            CDLReaderColorDecisionListElt* pCDLElt =
                dynamic_cast<CDLReaderColorDecisionListElt*>(pCDElt->getParent().get());

            pCCElt->setCDLParsingInfo(pCDLElt->getCDLParsingInfo());
        }
        else
        {
            pElt = pImpl->createDummyElement(
                name,
                ": ColorCorrection must be under a ColorDecision (CDL), "
                "ColorCorrectionCollection (CCC), or must be the root "
                "element (CC)");
        }

        pImpl->m_elms.push_back(pElt);
        return true;
    }
    return false;
}

bool CDLParser::Impl::HandleColorCorrectionCCCStartElement(CDLParser::Impl* pImpl, const XML_Char *name)
{
    if (0 == strcmp(name, CDL_TAG_COLOR_CORRECTION))
    {
        ElementRcPtr pElt;

        // If parsing a CCC, make sure the ColorCorrection is under
        // a ColorCorrectionCollection element.
        if (pImpl->isBackElementInstanceOf<CDLReaderColorCorrectionCollectionElt>())
        {
            pElt = pImpl->createElement<CDLReaderColorCorrectionElt>(name);

            // Bind the ColorCorrection element's CDLTransformList to the one
            // in the ColorCorrectionCollection element.
            CDLReaderColorCorrectionElt* pCCElt =
                dynamic_cast<CDLReaderColorCorrectionElt*>(pElt.get());
            CDLReaderColorCorrectionCollectionElt* pCCCElt =
                dynamic_cast<CDLReaderColorCorrectionCollectionElt*>(pCCElt->getParent().get());

            pCCElt->setCDLParsingInfo(pCCCElt->getCDLParsingInfo());
        }
        else
        {
            pElt = pImpl->createDummyElement(
                name,
                ": ColorCorrection must be under a ColorDecision (CDL), "
                "ColorCorrectionCollection (CCC), or must be the root "
                "element (CC)");
        }

        pImpl->m_elms.push_back(pElt);
        return true;
    }
    return false;
}

bool CDLParser::Impl::HandleColorCorrectionCCStartElement(
    CDLParser::Impl* pImpl, const XML_Char *name)
{
    // Handle the ColorCorrection element.
    if (0 == strcmp(name, CDL_TAG_COLOR_CORRECTION))
    {
        ElementRcPtr pElt;

        // If parsing a CC, make sure it is the only CDLTransform.
        if (!pImpl->m_parsingInfo ||
            pImpl->m_parsingInfo->m_transforms.empty())
        {
            pElt = pImpl->createElement<CDLReaderColorCorrectionElt>(name);

            // Bind the ColorCorrection element's CDLTransformList to the
            // one explicitly created by the reader.
            CDLReaderColorCorrectionElt* pCCElt =
                dynamic_cast<CDLReaderColorCorrectionElt*>(pElt.get());

            pCCElt->setCDLParsingInfo(pImpl->getCDLParsingInfo());
        }
        else
        {
            pElt = pImpl->createDummyElement(
                name,
                ": ColorCorrection must be under a ColorDecision (CDL), "
                "ColorCorrectionCollection (CCC), or must be the root "
                "element (CC)");
        }

        pImpl->m_elms.push_back(pElt);
        return true;
    }
    return false;
}

bool CDLParser::Impl::HandleSOPNodeStartElement(CDLParser::Impl* pImpl,
                                                const XML_Char *name)
{
    if (0 == strcmp(name, TAG_SOPNODE))
    {
        ElementRcPtr pElt;
        if (pImpl->isBackElementInstanceOf<CDLReaderColorCorrectionElt>())
        {
            pElt = pImpl->createElement<CDLReaderSOPNodeCCElt>(name);
        }
        else
        {
            pElt = pImpl->createDummyElement(name,
                                             ": SOPNode must be under "
                                             "a ColorCorrection");
        }

        pImpl->m_elms.push_back(pElt);
        return true;
    }
    return false;
}

bool CDLParser::Impl::HandleSatNodeStartElement(CDLParser::Impl* pImpl,
                                                const XML_Char *name)
{
    if (0 == strcmp(name, TAG_SATNODE) ||
        0 == strcmp(name, TAG_SATNODEALT))
    {
        ElementRcPtr pElt;
        if (pImpl->isBackElementInstanceOf<CDLReaderColorCorrectionElt>())
        {
            pElt = pImpl->createElement<CDLReaderSatNodeCCElt>(name);
        }
        else
        {
            pElt = pImpl->createDummyElement(name,
                                             ": SatNode must be under "
                                             "a ColorCorrection");
        }

        pImpl->m_elms.push_back(pElt);
        return true;
    }
    return false;
}

bool CDLParser::Impl::HandleTerminalStartElement(CDLParser::Impl* pImpl,
                                                 const XML_Char *name)
{
    auto pContainer = std::dynamic_pointer_cast<XmlReaderContainerElt>(pImpl->getBackElement());
    if (!pContainer)
    {
        pImpl->m_elms.push_back(pImpl->createDummyElement(name,
                                                          "Internal error"));
        return true;
    }
    else
    {
        const std::string containerId = pContainer->getIdentifier();

        // Handle Description, InputDescription and ViewingDescription elements
        // at their appropriate parent container.
        if (IsValidDescriptionTag(name, containerId))
        {
            auto pDescElt
                = pImpl->createElement<XmlReaderDescriptionElt>(name);

            pImpl->m_elms.push_back(pDescElt);
            return true;
        }

        // Handle Slope, Offset and Power elements.
        else if (0 == strcmp(name, TAG_SLOPE) ||
                 0 == strcmp(name, TAG_OFFSET) ||
                 0 == strcmp(name, TAG_POWER))
        {
            ElementRcPtr pElt;
            if (pImpl->isBackElementInstanceOf<CDLReaderSOPNodeCCElt>())
            {
                pElt = pImpl->createElement<XmlReaderSOPValueElt>(name);
            }
            else
            {
                pElt = pImpl->createDummyElement(
                    name,
                    ": Slope, Offset or Power tags "
                    "must be under SOPNode");
            }

            pImpl->m_elms.push_back(pElt);
            return true;
        }

        // Handle Saturation element.
        else if (0 == strcmp(name, TAG_SATURATION))
        {
            ElementRcPtr pElt;
            if (pImpl->isBackElementInstanceOf<CDLReaderSatNodeCCElt>())
            {
                pElt = pImpl->createElement<XmlReaderSaturationElt>(name);
            }
            else
            {
                pElt = pImpl->createDummyElement(
                    name,
                    ": Saturation tags must be under SatNode");
            }

            pImpl->m_elms.push_back(pElt);
            return true;
        }
    }
    return false;
}

bool CDLParser::Impl::HandleUnknownStartElement(CDLParser::Impl* pImpl,
                                                const XML_Char *name)
{
    pImpl->m_elms.push_back(pImpl->createDummyElement(name,
                                                      ": Unknown element"));
    return true;
}

void CDLParser::Impl::EndElementHandler(void *userData, const XML_Char *name)
{
    CDLParser::Impl* pImpl = (CDLParser::Impl*)userData;
    if (!pImpl)
    {
        throw Exception("Internal CDL parsing error.");
    }

    if (!name || !*name)
    {
        pImpl->throwMessage("Internal parsing error");
    }

    // Is the expected element present?
    ElementRcPtr pElt = pImpl->getBackElement();
    if (!pElt)
    {
        pImpl->throwMessage("Missing element");
    }

    // Is it the expected element?
    if (pElt->getName() != name)
    {
        std::ostringstream os;
        os << "Unexpected element (";
        os << name << "). ";
        os << "Expecting (";
        os << pElt->getName() << "). ";

        pImpl->throwMessage(os.str());
    }

    pImpl->m_elms.pop_back();

    if (!pElt->isContainer() && !pElt->isDummy())
    {
        // Is it a plain element?
        auto pPlainElt = std::dynamic_pointer_cast<XmlReaderPlainElt>(pElt);
        if (!pPlainElt)
        {
            std::ostringstream os;
            os << "Unexpected attribute (";
            os << name << ")";

            pImpl->throwMessage(os.str());
        }

        ElementRcPtr pParent = pImpl->getBackElement();

        // Is it at the right location in the stack?
        if ((!pParent || !pParent->isContainer() ||
             pParent != pPlainElt->getParent()))
        {
            std::ostringstream os;
            os << "Parsing error (";
            os << name << ")";

            pImpl->throwMessage(os.str());
        }
    }

    pElt->end();
}

void CDLParser::Impl::CharacterDataHandler(void *userData,
                                           const XML_Char *s,
                                           int len)
{
    CDLParser::Impl* pImpl = (CDLParser::Impl*)userData;
    if (!pImpl)
    {
        throw Exception("Internal CDL parsing error.");
    }

    if (len == 0) return;
    if (len<0 || !s || !*s)
    {
        pImpl->throwMessage("Empty attribute data");
    }
    // Parsing a single new line. This is valid.
    if (len == 1 && s[0] == '\n') return;

    ElementRcPtr pElt = pImpl->m_elms.back();
    if (!pElt)
    {
        std::ostringstream os;
        os << "Missing eng tag (";
        os << std::string(s, len) << ")";

        pImpl->throwMessage(os.str());
    }

    auto pDescElt = std::dynamic_pointer_cast<XmlReaderDescriptionElt>(pElt);
    if (pDescElt)
    {
        // For description we keep the all the text.
        pDescElt->setRawData(s, len, pImpl->getXmlLocation());
    }
    else
    {
        // Ignore white-spaces.
        size_t start = 0;
        size_t end = len;
        FindSubString(s, len, start, end);

        if (end>0)
        {
            if (pElt->isContainer())
            {
                std::ostringstream os;
                os << "Illegal attribute (";
                os << std::string(s, len) << ")";

                pImpl->throwMessage(os.str());
            }
            else
            {
                auto pPlainElt = std::dynamic_pointer_cast<XmlReaderPlainElt>(pElt);
                if (!pPlainElt)
                {
                    std::ostringstream os;
                    os << "Illegal attribute (";
                    os << std::string(s, len) << ")";

                    pImpl->throwMessage(os.str());
                }
                pPlainElt->setRawData(s + start,
                                      end - start,
                                      pImpl->getXmlLocation());
            }
        }
    }
}

CDLParser::CDLParser(const std::string& xmlFile)
    : m_impl(new Impl(xmlFile))
{
}

CDLParser::~CDLParser()
{
    delete m_impl;
    m_impl = NULL;
}

void CDLParser::parse(std::istream & istream) const
{
    m_impl->parse(istream);
}

void CDLParser::getCDLTransforms(CDLTransformMap & transformMap,
                                 CDLTransformVec & transformVec,
                                 FormatMetadataImpl & metadata) const
{
    const CDLTransformVec& pTransformList = m_impl->getCDLParsingInfo()->m_transforms;
    for (size_t i = 0; i < pTransformList.size(); ++i)
    {
        const CDLTransformImplRcPtr& pTransform = pTransformList.at(i);
        transformVec.push_back(pTransform);

        const std::string & id = pTransform->data().getID();
        if (!id.empty())
        {
            CDLTransformMap::iterator iter = transformMap.find(id);
            if (iter != transformMap.end())
            {
                std::ostringstream os;
                os << "Error loading ccc xml. ";
                os << "Duplicate elements with '" << id << "' found. ";
                os << "If id is specified, it must be unique.";
                throw Exception(os.str().c_str());
            }

            transformMap[id] = pTransform;
        }
    }
    metadata = m_impl->getCDLParsingInfo()->m_metadata;
}

void CDLParser::getCDLTransform(CDLTransformImplRcPtr & transform) const
{
    const CDLTransformVec& pTransformList = m_impl->getCDLParsingInfo()->m_transforms;
    if (pTransformList.empty())
    {
        throw Exception("No transform found.");
    }
    transform = pTransformList.at(0);
}

bool CDLParser::isCC() const
{
    return m_impl->isCC();
}

bool CDLParser::isCCC() const
{
    return m_impl->isCCC();
}

} // namespace OCIO_NAMESPACE
