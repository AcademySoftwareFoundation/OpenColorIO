// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "BitDepthUtils.h"
#include "fileformats/ctf/CTFReaderHelper.h"
#include "fileformats/ctf/CTFReaderUtils.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "Logging.h"
#include "MathUtils.h"
#include "ops/log/LogUtils.h"
#include "ParseUtils.h"
#include "Platform.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

CTFReaderTransformElt::CTFReaderTransformElt(const std::string & name,
                                             unsigned int xmlLineNumber,
                                             const std::string & xmlFile,
                                             bool isCLF)
    : XmlReaderContainerElt(name, xmlLineNumber, xmlFile)
    , m_isCLF(isCLF)
{
    m_transform = std::make_shared<CTFReaderTransform>();
}

const std::string & CTFReaderTransformElt::getIdentifier() const
{
    return m_transform->getID();
}

namespace
{
template <class T>
void PrintInStream(std::ostringstream & oss, const T &val)
{
    oss << val;
}

template <class T, class... Rest>
void PrintInStream(std::ostringstream & oss, const T &val, Rest... r)
{
    PrintInStream(oss, val);
    PrintInStream(oss, r...);
}

template <class T, class... Rest>
void ThrowM(const XmlReaderElement& elt, const T &val, Rest... r)
{
    std::ostringstream oss;
    PrintInStream(oss, val, r...);
    elt.throwMessage(oss.str());
}
}

void CTFReaderTransformElt::start(const char ** atts)
{
    bool isIdFound = false;
    bool isVersionFound = false;
    bool isCLFVersionFound = false;
    CTFVersion requestedVersion(0, 0);
    CTFVersion requestedCLFVersion(0, 0);

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_ID, atts[i]))
        {
            if (!atts[i + 1] || !*atts[i + 1])
            {
                throwMessage("Required attribute 'id' does not have a value.");
            }

            m_transform->setID(atts[i + 1]);
            isIdFound = true;
        }
        else if (0 == Platform::Strcasecmp(ATTR_NAME, atts[i]))
        {
            if (!atts[i + 1] || !*atts[i + 1])
            {
                throwMessage("If the attribute 'name' is present, it must have a value.");
            }

            m_transform->setName(atts[i + 1]);
        }
        else if (0 == Platform::Strcasecmp(ATTR_INVERSE_OF, atts[i]))
        {
            if (!atts[i + 1] || !*atts[i + 1])
            {
                throwMessage("If the attribute 'inverseOf' is present, it must have a value.");
            }

            m_transform->setInverseOfId(atts[i + 1]);
        }
        else if (0 == Platform::Strcasecmp(ATTR_VERSION, atts[i]))
        {
            if (isCLFVersionFound)
            {
                throwMessage("'compCLFversion' and 'Version' cannot both be present.");
            }
            if (isVersionFound)
            {
                throwMessage("'Version' can only be there once.");
            }

            const char* pVer = atts[i + 1];
            if (!pVer || !*pVer)
            {
                throwMessage("If the attribute 'version' is present, it must have a value.");
            }

            try
            {
                const std::string verString(pVer);
                CTFVersion::ReadVersion(verString, requestedVersion);
            }
            catch (Exception& ce)
            {
                throwMessage(ce.what());
            }

            isVersionFound = true;
        }
        else if (0 == Platform::Strcasecmp(ATTR_COMP_CLF_VERSION, atts[i]))
        {
            if (isCLFVersionFound)
            {
                throwMessage("'compCLFversion' can only be there once.");
            }
            if (isVersionFound)
            {
                throwMessage("'compCLFversion' and 'Version' cannot be both present.");
            }

            const char* pVer = atts[i + 1];
            if (!pVer || !*pVer)
            {
                throwMessage("Required attribute 'compCLFversion' does not have a value.");
            }

            try
            {
                std::string verString(pVer);
                CTFVersion::ReadVersion(verString, requestedCLFVersion);
            }
            catch (Exception& ce)
            {
                throwMessage(ce.what());
            }

            // Translate to CTF.

            const CTFVersion maxCLF(3, 0);
            if (maxCLF < requestedCLFVersion)
            {
                ThrowM(*this, "Unsupported transform file version '", pVer,
                       "' supplied.");
            }
            // We currently interpret CLF versions <= 2.0 as CTF version 1.7.
            if (requestedCLFVersion <= CTFVersion(2, 0))
            {
                requestedVersion = CTF_PROCESS_LIST_VERSION_1_7;
            }
            else
            {
                requestedVersion = CTF_PROCESS_LIST_VERSION_2_0;
            }

            isVersionFound = true;
            isCLFVersionFound = true;
            // Handle as CLF.
            m_isCLF = true;
        }
        else if (0 == Platform::Strcasecmp("xmlns", atts[i]))
        {
            // Ignore.
        }
        else
        {
            logParameterWarning(atts[i]);
        }

        i += 2;
    }

    // Check mandatory elements.
    if (!isIdFound)
    {
        throwMessage("Required attribute 'id' is missing.");
    }

    // Transform file format with no version means that
    // the CTF format is 1.2.
    if (!isVersionFound)
    {
        if (m_isCLF && !isCLFVersionFound)
        {
            throwMessage("Required attribute 'compCLFversion' is missing.");
        }
        setVersion(CTF_PROCESS_LIST_VERSION_1_2);
    }
    else
    {
        setVersion(requestedVersion);
        if (m_isCLF)
        {
            setCLFVersion(requestedCLFVersion);
        }
    }
}

void CTFReaderTransformElt::end()
{
}

void CTFReaderTransformElt::appendMetadata(const std::string & /*name*/, const std::string & value)
{
    getTransform()->getDescriptions().push_back(value);
}

const CTFReaderTransformPtr & CTFReaderTransformElt::getTransform() const
{
    return m_transform;
}

const char * CTFReaderTransformElt::getTypeName() const
{
    static const std::string n(TAG_PROCESS_LIST);
    return n.c_str();
}

void CTFReaderTransformElt::setVersion(const CTFVersion & ver)
{
    if (CTF_PROCESS_LIST_VERSION < ver)
    {
        ThrowM(*this, "Unsupported transform file version '", ver, "' supplied.");
    }
    getTransform()->setCTFVersion(ver);
}

void CTFReaderTransformElt::setCLFVersion(const CTFVersion & ver)
{
    getTransform()->setCLFVersion(ver);
}

const CTFVersion & CTFReaderTransformElt::getVersion() const
{
    return getTransform()->getCTFVersion();
}

const CTFVersion & CTFReaderTransformElt::getCLFVersion() const
{
    return getTransform()->getCLFVersion();
}

bool CTFReaderTransformElt::isCLF() const
{
    return getTransform()->isCLF();
}

//////////////////////////////////////////////////////////

CTFReaderArrayElt::CTFReaderArrayElt(const std::string & name,
                                     ContainerEltRcPtr pParent,
                                     unsigned int xmlLineNumber,
                                     const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
    , m_array(nullptr)
    , m_position(0)
{
}

CTFReaderArrayElt::~CTFReaderArrayElt()
{
    m_array = nullptr; // Not owned
}

void CTFReaderArrayElt::start(const char ** atts)
{
    bool isDimFound = false;

    unsigned i = 0;
    while (atts[i] && *atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_DIMENSION, atts[i]))
        {
            isDimFound = true;

            const char* dimStr = atts[i + 1];
            const size_t len = strlen(dimStr);

            CTFArrayMgt::Dimensions dims;
            try
            {
                dims = GetNumbers<unsigned>(dimStr, len);
            }
            catch (Exception& /*ce*/)
            {
                ThrowM(*this, "Illegal '", getTypeName(), "' array dimensions ",
                       TruncateString(dimStr, len), ".");
            }

            CTFArrayMgt* pArr = dynamic_cast<CTFArrayMgt*>(getParent().get());
            if (!pArr)
            {
                ThrowM(*this, "Parsing issue while parsing array dimensions of '",
                       getTypeName(), "' (", TruncateString(dimStr, len), ").");
            }
            else
            {
                const unsigned max = (const unsigned)(dims.empty() ? 0 : (dims.size() - 1));
                if (max == 0)
                {
                    ThrowM(*this, "Illegal '", getTypeName(), "' array dimensions ",
                           TruncateString(dimStr, len), ".");
                }

                m_array = pArr->updateDimension(dims);
                if (!m_array)
                {
                    ThrowM(*this, "'", getTypeName(), "' Illegal array dimensions ",
                           TruncateString(dimStr, len), ".");
                }
            }
        }
        else
        {
            logParameterWarning(atts[i]);
        }

        i += 2;
    }

    // Check mandatory elements.
    if (!isDimFound)
    {
        throwMessage("Missing 'dim' attribute.");
    }

    m_position = 0;
}

void CTFReaderArrayElt::end()
{
    // A known element (e.g. an array) in a dummy element,
    // no need to validate it.
    if (getParent()->isDummy()) return;

    CTFArrayMgt* pArr = dynamic_cast<CTFArrayMgt*>(getParent().get());
    pArr->endArray(m_position);
}

void CTFReaderArrayElt::setRawData(const char * s,
                                   size_t len,
                                   unsigned int/*xmlLine*/)
{
    const unsigned long maxValues = m_array->getNumValues();
    size_t pos(0);

    //
    // using GetNextNumber here instead of GetNumbers to leverage the loop
    // needed here to process each value from the strings.  This function
    // is the most used when reading in large transforms.
    //

    pos = FindNextTokenStart(s, len, 0);
    while (pos != len)
    {
        double data(0.);

        try
        {
            GetNextNumber(s, len, pos, data);
        }
        catch (Exception& /*ce*/)
        {
            ThrowM(*this, "Illegal values '", TruncateString(s, len),
                   "' in array of ", getTypeName(), ".");
        }

        if (m_position<maxValues)
        {
            m_array->setDoubleValue(m_position++, data);
        }
        else
        {
            const CTFReaderOpElt* p = static_cast<const CTFReaderOpElt*>(getParent().get());

            std::ostringstream arg;
            if (p->getOp()->getType() == OpData::Lut1DType)
            {
                arg << m_array->getLength();
                arg << "x" << m_array->getNumColorComponents();
            }
            else if (p->getOp()->getType() == OpData::Lut3DType)
            {
                arg << m_array->getLength() << "x" << m_array->getLength();
                arg << "x" << m_array->getLength();
                arg << "x" << m_array->getNumColorComponents();
            }
            else  // Matrix
            {
                arg << m_array->getLength();
                arg << "x" << m_array->getLength();
            }

            ThrowM(*this, "Expected ", arg.str(),
                   " Array, found too many values in array of '", getTypeName(), "'.");
        }
    }
}

const char * CTFReaderArrayElt::getTypeName() const
{
    return dynamic_cast<const CTFReaderOpElt*>(getParent().get())->getTypeName();
}

//////////////////////////////////////////////////////////

CTFArrayMgt::CTFArrayMgt()
    : m_completed(false)
{
}

CTFArrayMgt::~CTFArrayMgt()
{
}

//////////////////////////////////////////////////////////

CTFReaderIndexMapElt::CTFReaderIndexMapElt(const std::string & name,
                                           ContainerEltRcPtr pParent,
                                           unsigned int xmlLineNumber,
                                           const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
    , m_indexMap(nullptr)
    , m_position(0)
{
}

CTFReaderIndexMapElt::~CTFReaderIndexMapElt()
{
    m_indexMap = nullptr; // Not owned
}

void CTFReaderIndexMapElt::start(const char ** atts)
{
    bool isDimFound = false;

    unsigned long i = 0;
    while (atts[i] && *atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_DIMENSION, atts[i]))
        {
            isDimFound = true;

            const char* dimStr = atts[i + 1];
            const size_t len = strlen(dimStr);

            CTFIndexMapMgt::DimensionsIM dims;
            try
            {
                dims = GetNumbers<unsigned>(dimStr, len);
            }
            catch (Exception& /*ce*/)
            {
                ThrowM(*this, "Illegal '", getTypeName(),
                       "' IndexMap dimensions ", TruncateString(dimStr, len), ".");
            }

            CTFIndexMapMgt * pArr = dynamic_cast<CTFIndexMapMgt*>(getParent().get());
            if (!pArr)
            {
                ThrowM(*this, "Illegal '", getTypeName(),
                       "' IndexMap dimensions ", TruncateString(dimStr, len), ".");
            }
            else
            {
                if (dims.empty() || dims.size() != 1)
                {
                    ThrowM(*this, "Illegal '", getTypeName(),
                           "' IndexMap dimensions ", TruncateString(dimStr, len), ".");
                }

                m_indexMap = pArr->updateDimensionIM(dims);
                if (!m_indexMap)
                {
                    ThrowM(*this, "Illegal '", getTypeName(),
                           "' IndexMap dimensions ", TruncateString(dimStr, len), ".");
                }
            }
        }
        else
        {
            logParameterWarning(atts[i]);
        }

        i += 2;
    }

    // Check mandatory elements
    if (!isDimFound)
    {
        throwMessage("Required attribute 'dim' is missing.");
    }

    m_position = 0;
}

void CTFReaderIndexMapElt::end()
{
    // A known element (e.g. an IndexMap) in a dummy element,
    // no need to validate it.
    if (getParent()->isDummy()) return;

    CTFReaderOpElt * pOpElt = dynamic_cast<CTFReaderOpElt*>(getParent().get());
    if (pOpElt)
    {
        if (pOpElt->getTransform()->getCTFVersion() < CTF_PROCESS_LIST_VERSION_2_0)
        {
            CTFIndexMapMgt * pIMM = dynamic_cast<CTFIndexMapMgt*>(getParent().get());
            pIMM->endIndexMap(m_position);
        }
        else
        {
            std::ostringstream dbg;
            dbg << getXmlFile().c_str() << "(" << getXmlLineNumber() << "): ";
            dbg << "Element '" << getName() << "' is not valid since CLF 3 (or CTF 2).";
            LogWarning(dbg.str().c_str());
        }
    }
}

namespace
{
// Like FindDelim() but looks for whitespace or an ampersand (for IndexMap).
size_t FindIndexDelim(const char* str, size_t len, size_t pos)
{
    const char *ptr = str + pos;

    while (!IsSpace(*ptr) && !(*ptr == '@'))
    {
        ptr++; pos++;

        if (pos >= len)
        {
            return len;
        }
    }

    return pos;
}

// Like findNextTokenStart() but also ignores ampersands.
size_t FindNextTokenStart_IndexMap(const char* s, size_t len, size_t pos)
{
    const char *ptr = s + pos;

    if (pos == len)
    {
        return pos;
    }

    while (IsNumberDelimiter(*ptr) || (*ptr == '@'))
    {
        ptr++; pos++;

        if (pos >= len)
        {
            return len;
        }
    }

    return pos;
}


// Extract the next pair of IndexMap numbers contained in the string.
// - str the string to search
// - len length of the string
// - pos position to start the search at.
// - Returns the next in the string.  Note that pos gets updated to the
//         position of the next delimiter, or to std::string::npos
//         if the value returned is the last one in the string.
//
// This parses a pair of values from an IndexMap.
// Example: <IndexMap dim="6">64.5@0 1e-1@0.1 0.1@-0.2 1 @2 2 @3 940 @ 2</IndexMap>
void GetNextIndexPair(const char *s, size_t len, size_t& pos, float& num1, float& num2)
{
    // s might not be null terminated.
    // Set pos to how much leading white space there is.
    pos = FindNextTokenStart(s, len, pos);

    if (pos != len)
    {
        // Note, the len may here include the @ at the end of the string for ParseNumber
        // (e.g. "10.5@") but it does not cause the sscanf to fail.
        // Set pos to advance over the numbers we just parsed.
        // Note that we stop either at white space or an ampersand.
        size_t endPos = FindIndexDelim(s, len, pos);
        if (endPos == len)
        {
            std::ostringstream oss;
            oss << "GetNextIndexPair: First number of a pair is the end of the string '"
                << TruncateString(s, len) << "'.";
            throw Exception(oss.str().c_str());
        }

        // Extract a number at pos.
        ParseNumber(s, pos, endPos, num1);

        // Set pos to the start of the next number, advancing over white space or an @.
        pos = FindNextTokenStart_IndexMap(s, len, endPos);

        endPos = FindDelim(s, len, pos);

        // Extract the other half of the index pair.
        // Set pos to advance over the numbers we just parsed.
        ParseNumber(s, pos, endPos, num2);

        pos = endPos;
        if (pos != len)
        {
            pos = FindNextTokenStart(s, len, pos);
        }
    }
}
}

void CTFReaderIndexMapElt::setRawData(const char * s,
                                      size_t len,
                                      unsigned int /*xmlLine*/)
{
    size_t maxValues = m_indexMap->getDimension();
    size_t pos(0);

    pos = FindNextTokenStart(s, len, 0);
    while (pos != len)
    {
        float data1(0.f);
        float data2(0.f);

        try
        {
            GetNextIndexPair(s, len, pos, data1, data2);
        }
        catch (Exception& /*ce*/)
        {
            ThrowM(*this, "Illegal values '", TruncateString(s, len),
                   "' in '", getTypeName(), "' IndexMap.");
        }

        if (m_position<maxValues)
        {
            m_indexMap->setPair(m_position++, data1, data2);
        }
        else
        {
            ThrowM(*this, "Expected ", m_indexMap->getDimension(),
                   " entries, found too many values in '",
                   getTypeName(), "' IndexMap.");
        }
    }
}

const char * CTFReaderIndexMapElt::getTypeName() const
{
    return dynamic_cast<const CTFReaderOpElt*>(getParent().get())->getTypeName();
}

//////////////////////////////////////////////////////////

CTFReaderMetadataElt::CTFReaderMetadataElt(const std::string & name,
                                           ContainerEltRcPtr pParent,
                                           unsigned int xmlLineNumber,
                                           const std::string & xmlFile)
    : XmlReaderComplexElt(name, pParent, xmlLineNumber, xmlFile)
    , m_metadata(name, "")
{
}

CTFReaderMetadataElt::~CTFReaderMetadataElt()
{
}

void CTFReaderMetadataElt::start(const char ** atts)
{
    unsigned i = 0;
    while (atts[i] && *atts[i])
    {
        if (atts[i + 1] && *atts[i + 1])
        {
            m_metadata.addAttribute(atts[i], atts[i + 1]);
        }
        i += 2;
    }
}

void CTFReaderMetadataElt::end()
{
    CTFReaderMetadataElt* pMetadataElt = dynamic_cast<CTFReaderMetadataElt*>(getParent().get());
    if (pMetadataElt)
    {
        pMetadataElt->getMetadata().getChildrenElements().push_back(m_metadata);
    }
}

const std::string & CTFReaderMetadataElt::getIdentifier() const
{
    return getName();
}

void CTFReaderMetadataElt::setRawData(const char * str, size_t len, unsigned int)
{
    std::string newValue{ m_metadata.getElementValue() + std::string(str, len) };
    m_metadata.setElementValue(newValue.c_str());
}

//////////////////////////////////////////////////////////

CTFReaderInfoElt::CTFReaderInfoElt(const std::string & name,
                                   ContainerEltRcPtr pParent,
                                   unsigned int xmlLineNumber,
                                   const std::string & xmlFile)
    : CTFReaderMetadataElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderInfoElt::~CTFReaderInfoElt()
{
}

namespace
{
void validateInfoElementVersion(const char * versionAttr,
                                const char * versionValue)
{
    // There are 3 rules for an <Info> element version attribute to be valid :
    //
    // 1- Not exist. No version means version 1.0. It will always be valid.
    // 2- Be of the following format : MAJOR.MINOR ( i.e '3.0' )
    // 3- The major version should be equal or smaller then the current major version.
    //
    // Note: The minor version is not taken into account when validating the
    // version. The minor version is only for tracking purposes.
    // Note: <Info> is not part of CLF.
    // 
    if (versionAttr && *versionAttr &&
        0 == Platform::Strcasecmp(ATTR_VERSION, versionAttr))
    {
        if (!versionValue || !*versionValue)
        {
            throw Exception("CTF reader. Invalid Info element version attribute.");
        }

        int fver = (int)CTF_INFO_ELEMENT_VERSION;
        if (0 == sscanf(versionValue, "%d", &fver))
        {
            std::ostringstream oss;
            oss << "CTF reader. Invalid Info element version attribute: ";
            oss << versionValue << " .";
            throw Exception(oss.str().c_str());
        }

        // Always compare with 'ints' so we do not include minor versions
        // in the test. An info version of 3.9 should be supported in a SynColor
        // with the current version of 3.0.
        //
        if (fver > (int)CTF_INFO_ELEMENT_VERSION)
        {
            std::ostringstream oss;
            oss << "CTF reader. Unsupported Info element version attribute: ";
            oss << versionValue << " .";
            throw Exception(oss.str().c_str());
        }
    }
}
}
void CTFReaderInfoElt::start(const char ** atts)
{
    // Validate the version number. The version should be the first attribute.
    // Might throw.
    validateInfoElementVersion(atts[0], atts[1]);

    // Let the base class store the attributes in the FormatMetadataImpl element.
    CTFReaderMetadataElt::start(atts);
}

void CTFReaderInfoElt::end()
{
    CTFReaderTransformElt* pTransformElt = dynamic_cast<CTFReaderTransformElt*>(getParent().get());
    if (pTransformElt)
    {
        pTransformElt->getTransform()->getInfoMetadata() = getMetadata();
    }
}

//////////////////////////////////////////////////////////

CTFReaderOpElt::CTFReaderOpElt()
    : XmlReaderContainerElt("", 0, "")
{
}

CTFReaderOpElt::~CTFReaderOpElt()
{
}

void CTFReaderOpElt::setContext(const std::string & name,
                                const CTFReaderTransformPtr & pTransform,
                                unsigned xmlLineNumber,
                                const std::string & xmlFile)
{
    XmlReaderElement::setContext(name, xmlLineNumber, xmlFile);

    m_transform = pTransform;

    if (!pTransform.get())
    {
        throwMessage("ProcessList tag missing.");
    }
}

const std::string & CTFReaderOpElt::getIdentifier() const
{
    return getOp()->getID();
}

void CTFReaderOpElt::appendMetadata(const std::string & name, const std::string & value)
{
    FormatMetadataImpl item(name, value);
    getOp()->getFormatMetadata().getChildrenElements().push_back(item);
}

void CTFReaderOpElt::start(const char ** atts)
{
    std::ostringstream dbg;
    dbg << getXmlFile().c_str() << "(" << getXmlLineNumber() << "): ";
    dbg << "Parsing '" << getName() << "'.";
    LogDebug(dbg.str().c_str());

    // Add a pointer to an empty op of the appropriate child class to the
    // end of the opvec.  No data is copied since the parameters of the op
    // have not been filled in yet.
    m_transform->getOps().push_back(getOp());

    enum BitDepthFlags
    {
        NO_BIT_DEPTH = 0x00,
        INPUT_BIT_DEPTH = 0x01,
        OUTPUT_BIT_DEPTH = 0x02,
        ALL_BIT_DEPTH = (INPUT_BIT_DEPTH | OUTPUT_BIT_DEPTH)
    };

    unsigned bitDepthFound = NO_BIT_DEPTH;

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_ID, atts[i]))
        {
            getOp()->setID(atts[i + 1]);
        }
        else if (0 == Platform::Strcasecmp(ATTR_NAME, atts[i]))
        {
            getOp()->setName(atts[i + 1]);
        }
        else if (0 == Platform::Strcasecmp(ATTR_BITDEPTH_IN, atts[i]))
        {
            BitDepth bitdepth = GetBitDepth(atts[i + 1]);
            if (bitdepth == BIT_DEPTH_UNKNOWN)
            {
                ThrowM(*this, "inBitDepth unknown value (", atts[i + 1], ").");
            }
            m_inBitDepth = bitdepth;
            bitDepthFound |= INPUT_BIT_DEPTH;
        }
        else if (0 == Platform::Strcasecmp(ATTR_BITDEPTH_OUT, atts[i]))
        {
            BitDepth bitdepth = GetBitDepth(atts[i + 1]);
            if (bitdepth == BIT_DEPTH_UNKNOWN)
            {
                ThrowM(*this, "outBitDepth unknown value (", atts[i + 1], ").");
            }
            m_outBitDepth = bitdepth;
            bitDepthFound |= OUTPUT_BIT_DEPTH;
        }

        i += 2;
    }

    // Check mandatory attributes.
    if ((bitDepthFound&INPUT_BIT_DEPTH) == NO_BIT_DEPTH)
    {
        throwMessage("inBitDepth is missing.");
    }
    else if ((bitDepthFound&OUTPUT_BIT_DEPTH) == NO_BIT_DEPTH)
    {
        throwMessage("outBitDepth is missing.");
    }

    // Ensure bit-depths consistency between ops in the file.

    const BitDepth prevOutBD = m_transform->getPreviousOutBitDepth();
    m_transform->setPreviousOutBitDepth(m_outBitDepth);
    if (prevOutBD != BIT_DEPTH_UNKNOWN) // Unknown for first op.
    {
        if (prevOutBD != m_inBitDepth)
        {
            const std::string inBD(BitDepthToString(m_inBitDepth));
            const std::string prevBD(BitDepthToString(prevOutBD));
            std::ostringstream os;
            os << "Bit-depth mismatch between ops. Previous op output ";
            os << "bit-depth is: '" << prevBD;
            os << "' and this op input bit-depth is '" << inBD;
            os << "'. ";
            throwMessage(os.str());
        }
    }

    validateXmlParameters(atts);
}

void CTFReaderOpElt::end()
{
}

const char * CTFReaderOpElt::getTypeName() const
{
    return getName().c_str();
}

void CTFReaderOpElt::validateXmlParameters(const char ** atts) const noexcept
{
    unsigned i = 0;
    while (atts[i])
    {
        if (!isOpParameterValid(atts[i]))
        {
            logParameterWarning(atts[i]);
        }

        i += 2;
    }
}

bool CTFReaderOpElt::isOpParameterValid(const char * att) const noexcept
{
    return 0 == Platform::Strcasecmp(ATTR_ID, att) ||
           0 == Platform::Strcasecmp(ATTR_NAME, att) ||
           0 == Platform::Strcasecmp(ATTR_BITDEPTH_IN, att) ||
           0 == Platform::Strcasecmp(ATTR_BITDEPTH_OUT, att) ||
           // Allow bypass attribute in CTF.
           (0 == Platform::Strcasecmp(ATTR_BYPASS, att) && !m_transform->isCLF());
}

//------------------------------------------------------------------------------
//
// These macros are used to define which Op implementation to use
// depending of the selected version.
//


//
// Versioning of file formats is a topic that needs careful consideration.
// Any format will have one day to change some part of its structure in order
// to support new features. In our case, the Color Transform XML format
// will evolve to support new Ops and potentially extend some existing Ops.
//
// The two design decisions related to the versioning are that first,
// the CTF Reader has to be fully backward compatible (it means to read
// any existing versions) and second, only the latest version will be written.
//
// The macros below provides a mechanism to support versioning at the Op level.
//
// At the design point of view, any new Op's version should be located
// in this file only; factory method already exists to handle the creation
// of the right Op reader instance based on the requested version.
// Please refer to CTFReaderOpElt::GetReader() to see that the element reader
// is instantiated following the requested version.
// For example:
//  - ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderMatrixElt, 1_2) imposes that
//    any xml versions lower or equal to 1.2 shall use the CTFReaderMatrixElt reader.
//  - ADD_DEFAULT_READER(CTFReaderMatrixElt_1_3) imposes that any xml versions shall
//    use the CTFReaderMatrixElt_1_3 reader.
//  - As these two macros are in the same scope, the 1.2 check will be
//    performed before the 1.3 one; hence, CTFReaderMatrixElt_1_3 is only used for
//    versions greater than 1.2.
//

// Note: Refer to CTFReaderOpElt::GetReader() to have a usage and
//       implicit variables used by the macros below.

// Add a reader to all versions lower or equal to the specified one.
#define ADD_READER_FOR_VERSIONS_UP_TO(READER, VERS)  \
  if(version <= CTF_PROCESS_LIST_VERSION_##VERS)    \
  { pOp = std::make_shared<READER>(); break; }


// Add a default reader for all existing versions.
#define ADD_DEFAULT_READER(READER)                  \
  if(version <= CTF_PROCESS_LIST_VERSION)           \
  { pOp = std::make_shared<READER>(); break; }

// Add a reader to all versions higher or equal to the specified one.
#define ADD_READER_FOR_VERSIONS_STARTING_AT(READER, VERS) \
  if(version >= CTF_PROCESS_LIST_VERSION_##VERS &&        \
     version <= CTF_PROCESS_LIST_VERSION)                 \
  { pOp = std::make_shared<READER>(); break; }


// Add a reader for a range of versions.
#define ADD_READER_FOR_VERSIONS_BETWEEN(READER, VERS1, VERS2) \
  if(version >= CTF_PROCESS_LIST_VERSION_##VERS1 &&           \
     version <= CTF_PROCESS_LIST_VERSION_##VERS2)             \
  { pOp = std::make_shared<READER>(); break; }

CTFReaderOpEltRcPtr CTFReaderOpElt::GetReader(CTFReaderOpElt::Type type,
                                              const CTFVersion & version, bool isCLF)
{
    CTFReaderOpEltRcPtr pOp;

    switch (type)
    {
    case CTFReaderOpElt::ACESType:
    {
        ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderACESElt, 1_5);
        break;
    }
    case CTFReaderOpElt::CDLType:
    {
        // Note: CLF style name support was not added until version 1.7, but
        // no point creating a separate version just for that.
        ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderCDLElt, 1_3);
        break;
    }
    case CTFReaderOpElt::ExposureContrastType:
    {
        if (!isCLF)
        {
            ADD_DEFAULT_READER(CTFReaderExposureContrastElt);
        }
        break;
    }
    case CTFReaderOpElt::FixedFunctionType:
    {
        if (!isCLF)
        {
            ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderFixedFunctionElt, 2_0);
        }
        break;
    }
    case CTFReaderOpElt::FunctionType:
    {
        if (!isCLF)
        {
            ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderFunctionElt, 1_6);
        }
        break;
    }
    case CTFReaderOpElt::GammaType:
    {
        if (!isCLF)
        {
            // If the version is 1.4 or less, then use GammaElt.
            // This reader forces the alpha transformation to be the identity.
            ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderGammaElt, 1_4);
            // CTF 1.5 and onwards handles alpha component.
            ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderGammaElt_1_5, 1_8);
            // CTF v2 and CLF v3 add styles and changes names.
            ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderGammaElt_CTF_2_0, 2_0);
        }
        else
        {
            // Introduced with CLF v3. CLF does not handle alpha component.
            ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderGammaElt_CLF_3_0, 2_0);
        }
        break;
    }
    case CTFReaderOpElt::GradingPrimaryType:
    {
        if (!isCLF)
        {
            ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderGradingPrimaryElt, 2_0);
        }
        break;
    }
    case CTFReaderOpElt::GradingRGBCurveType:
    {
        if (!isCLF)
        {
            ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderGradingRGBCurveElt, 2_0);
        }
        break;
    }
    case CTFReaderOpElt::GradingToneType:
    {
        if (!isCLF)
        {
            ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderGradingToneElt, 2_0);
        }
        break;
    }
    case CTFReaderOpElt::InvLut1DType:
    {
        if (!isCLF)
        {
            ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderInvLut1DElt, 1_3);
        }
        break;
    }
    case CTFReaderOpElt::InvLut3DType:
    {
        if (!isCLF)
        {
            ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderInvLut3DElt, 1_6);
        }
        break;
    }
    case CTFReaderOpElt::LogType:
    {
        if (!isCLF)
        {
            ADD_READER_FOR_VERSIONS_BETWEEN(CTFReaderLogElt, 1_3, 1_8);
        }
        // CLF v3 (CTF v2) adds log support and adds new camera styles.
        ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderLogElt_2_0, 2_0);
        break;
    }
    case CTFReaderOpElt::Lut1DType:
    {
        ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderLut1DElt, 1_3);
        // Adding hue_adjust attribute.
        ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderLut1DElt_1_4, 1_4);
        // Adding basic IndexMap element.
        ADD_DEFAULT_READER(CTFReaderLut1DElt_1_7);
        break;
    }
    case CTFReaderOpElt::Lut3DType:
    {
        ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderLut3DElt, 1_6);
        // Adding basic IndexMap element.
        ADD_DEFAULT_READER(CTFReaderLut3DElt_1_7);
        break;
    }
    case CTFReaderOpElt::MatrixType:
    {
        // If the version is 1.2 or less, then use MatrixElt.
        ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderMatrixElt, 1_2);
        // If the version is 1.3 or more, then use MatrixElt_1_3.
        ADD_DEFAULT_READER(CTFReaderMatrixElt_1_3);
        break;
    }
    case CTFReaderOpElt::RangeType:
    {
        ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderRangeElt, 1_6);
        // Adding noClamp style.
        ADD_DEFAULT_READER(CTFReaderRangeElt_1_7);
        break;
    }
    case CTFReaderOpElt::ReferenceType:
    {
        ADD_DEFAULT_READER(CTFReaderReferenceElt);
        break;
    }
    case CTFReaderOpElt::NoType:
    {
        break;
    }
    }

    return pOp;
}

BitDepth CTFReaderOpElt::GetBitDepth(const std::string & strBD)
{
    const std::string str = StringUtils::Lower(strBD);
    if (str == "8i") return BIT_DEPTH_UINT8;
    else if (str == "10i") return BIT_DEPTH_UINT10;
    else if (str == "12i") return BIT_DEPTH_UINT12;
    else if (str == "16i") return BIT_DEPTH_UINT16;
    else if (str == "16f") return BIT_DEPTH_F16;
    else if (str == "32f") return BIT_DEPTH_F32;
    return BIT_DEPTH_UNKNOWN;
}

//////////////////////////////////////////////////////////

CTFReaderACESElt::CTFReaderACESElt()
    : CTFReaderOpElt()
    , m_fixedFunction(std::make_shared<FixedFunctionOpData>(FixedFunctionOpData::ACES_RED_MOD_03_FWD))
{
}

CTFReaderACESElt::~CTFReaderACESElt()
{
}

void CTFReaderACESElt::start(const char **atts)
{
    CTFReaderOpElt::start(atts);

    bool isStyleFound = false;

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            // We need a valid style to parse the parameters.
            // This will throw on unrecognized styles.
            try
            {
                m_fixedFunction->setStyle(FixedFunctionOpData::GetStyle(atts[i + 1]));
                isStyleFound = true;
            }
            catch (Exception& ce)
            {
                throwMessage(ce.what());
            }
        }

        i += 2;
    }
    if (!isStyleFound)
    {
        throwMessage("style parameter for FixedFunction is missing.");
    }
}

bool CTFReaderACESElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) || 
           0 == Platform::Strcasecmp(ATTR_STYLE, att);
}

void CTFReaderACESElt::end()
{
    CTFReaderOpElt::end();

    m_fixedFunction->validate();
}

const OpDataRcPtr CTFReaderACESElt::getOp() const
{
    return m_fixedFunction;
}

CTFReaderACESParamsElt::CTFReaderACESParamsElt(
    const std::string & name,
    ContainerEltRcPtr pParent,
    unsigned int xmlLineNumber,
    const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderACESParamsElt::~CTFReaderACESParamsElt()
{
}

void CTFReaderACESParamsElt::start(const char **atts)
{
    // Attributes we want to extract.
    double gamma = std::numeric_limits<double>::quiet_NaN();

    CTFReaderACESElt * pFixedFunction
        = dynamic_cast<CTFReaderACESElt*>(getParent().get());

    // Try extracting the attributes.
    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_GAMMA, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], gamma);
        }
        else
        {
            logParameterWarning(atts[i]);
        }

        i += 2;
    }

    const auto style = pFixedFunction->getFixedFunction()->getStyle();
    if (style == FixedFunctionOpData::REC2100_SURROUND_FWD ||
        style == FixedFunctionOpData::REC2100_SURROUND_INV)
    {
        if (pFixedFunction->getFixedFunction()->getParams().size())
        {
            ThrowM(*this, "ACES FixedFunction element with style ",
                   FixedFunctionOpData::ConvertStyleToString(style, false),
                   " expects only 1 gamma parameter.");
        }
        FixedFunctionOpData::Params params;
        if (IsNan(gamma))
        {
            ThrowM(*this, "Missing required parameter ", ATTR_GAMMA,
                   "for ACES FixedFunction element with style ",
                   FixedFunctionOpData::ConvertStyleToString(style, false), ".");
        }
        params.push_back(gamma);
        // Assign the parameters to the object.
        pFixedFunction->getFixedFunction()->setParams(params);
    }
    else
    {
        ThrowM(*this, "ACES FixedFunction element with style ",
               FixedFunctionOpData::ConvertStyleToString(style, false),
               " does not take any parameter.");
    }
}

void CTFReaderACESParamsElt::end()
{
}

void CTFReaderACESParamsElt::setRawData(const char * str, size_t len, unsigned int xmlLine)
{
}

//////////////////////////////////////////////////////////

CTFReaderCDLElt::CTFReaderCDLElt()
    : CTFReaderOpElt()
    , m_cdl(std::make_shared<CDLOpData>())
{
    // CDL op is already initialized to identity.
}

CTFReaderCDLElt::~CTFReaderCDLElt()
{
}

void CTFReaderCDLElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    bool isStyleFound = false;
    for (unsigned i = 0; atts[i]; i += 2)
    {
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            // Unrecognized CDL styles will throw an exception.
            m_cdl->setStyle(CDLOpData::GetStyle(atts[i + 1]));
            isStyleFound = true;
        }
    }

    // Although the OCIO default for CDL style is no-clamp, the default specified in
    // the CLF v3 spec is ASC.
    if (!isStyleFound)
    {
        m_cdl->setStyle(CDLOpData::CDL_V1_2_FWD);
    }
}

bool CTFReaderCDLElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_STYLE, att);
}

void CTFReaderCDLElt::end()
{
    CTFReaderOpElt::end();

    // Validate the end result.
    m_cdl->validate();
}

const OpDataRcPtr CTFReaderCDLElt::getOp() const
{
    return m_cdl;
}

const CDLOpDataRcPtr & CTFReaderCDLElt::getCDL() const
{
    return m_cdl;
}

//////////////////////////////////////////////////////////

CTFReaderSatNodeElt::CTFReaderSatNodeElt(const std::string & name,
                                         ContainerEltRcPtr pParent,
                                         unsigned xmlLineNumber,
                                         const std::string & xmlFile)
    : XmlReaderSatNodeBaseElt(name, pParent, xmlLineNumber, xmlFile)
{
}

const CDLOpDataRcPtr & CTFReaderSatNodeElt::getCDL() const
{
    return static_cast<CTFReaderCDLElt*>(getParent().get())->getCDL();
}

//////////////////////////////////////////////////////////

CTFReaderSOPNodeElt::CTFReaderSOPNodeElt(const std::string & name,
                                         ContainerEltRcPtr pParent,
                                         unsigned int xmlLineNumber,
                                         const std::string & xmlFile)
    : XmlReaderSOPNodeBaseElt(name, pParent, xmlLineNumber, xmlFile)
{
}

const CDLOpDataRcPtr & CTFReaderSOPNodeElt::getCDL() const
{
    return static_cast<CTFReaderCDLElt*>(getParent().get())->getCDL();
}

//////////////////////////////////////////////////////////

CTFReaderFixedFunctionElt::CTFReaderFixedFunctionElt()
    : CTFReaderOpElt()
    , m_fixedFunction(std::make_shared<FixedFunctionOpData>(FixedFunctionOpData::ACES_RED_MOD_03_FWD))
{
}

CTFReaderFixedFunctionElt::~CTFReaderFixedFunctionElt()
{
}

void CTFReaderFixedFunctionElt::start(const char **atts)
{
    CTFReaderOpElt::start(atts);

    bool isStyleFound = false;

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            // We need a valid style to parse the parameters.
            // This will throw on unrecognized styles.
            try
            {
                m_fixedFunction->setStyle(FixedFunctionOpData::GetStyle(atts[i + 1]));
                isStyleFound = true;
            }
            catch (Exception& ce)
            {
                throwMessage(ce.what());
            }
        }
        else if (0 == Platform::Strcasecmp(ATTR_PARAMS, atts[i]))
        {
            std::vector<double> data;
            const char* paramsStr = atts[i + 1];
            const size_t len = paramsStr ? strlen(paramsStr) : 0;
            try
            {
                data = GetNumbers<double>(paramsStr, len);
            }
            catch (Exception& /*ce*/)
            {
                ThrowM(*this, "Illegal '", getTypeName(), "' params ",
                       TruncateString(paramsStr, len), ".");
            }
            m_fixedFunction->setParams(data);
        }

        i += 2;
    }
    if (!isStyleFound)
    {
        throwMessage("Style parameter for FixedFunction is missing.");
    }
}

bool CTFReaderFixedFunctionElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_STYLE, att) ||
           0 == Platform::Strcasecmp(ATTR_PARAMS, att);
}

void CTFReaderFixedFunctionElt::end()
{
    CTFReaderOpElt::end();

    m_fixedFunction->validate();
}

const OpDataRcPtr CTFReaderFixedFunctionElt::getOp() const
{
    return m_fixedFunction;
}

//////////////////////////////////////////////////////////

CTFReaderFunctionElt::CTFReaderFunctionElt()
    : CTFReaderOpElt()
    , m_fixedFunction(std::make_shared<FixedFunctionOpData>(FixedFunctionOpData::ACES_RED_MOD_03_FWD))
{
}

CTFReaderFunctionElt::~CTFReaderFunctionElt()
{
}

void CTFReaderFunctionElt::start(const char **atts)
{
    CTFReaderOpElt::start(atts);

    bool isStyleFound = false;

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            // This will throw on unrecognized styles.
            try
            {
                m_fixedFunction->setStyle(FixedFunctionOpData::GetStyle(atts[i + 1]));
                isStyleFound = true;
            }
            catch (Exception& ce)
            {
                throwMessage(ce.what());
            }
        }

        i += 2;
    }
    if (!isStyleFound)
    {
        throwMessage("Style parameter for FixedFunction is missing.");
    }
}

bool CTFReaderFunctionElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_STYLE, att);
}

void CTFReaderFunctionElt::end()
{
    CTFReaderOpElt::end();

    m_fixedFunction->validate();
}

const OpDataRcPtr CTFReaderFunctionElt::getOp() const
{
    return m_fixedFunction;
}

//////////////////////////////////////////////////////////

CTFReaderDynamicParamElt::CTFReaderDynamicParamElt(const std::string & name,
                                                   ContainerEltRcPtr pParent,
                                                   unsigned int xmlLineNumber,
                                                   const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderDynamicParamElt::~CTFReaderDynamicParamElt()
{
}

void CTFReaderDynamicParamElt::start(const char ** atts)
{
    ContainerEltRcPtr container = getParent();

    // Try extracting the attributes
    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_PARAM, atts[i]))
        {
            if (0 == Platform::Strcasecmp(TAG_DYN_PROP_EXPOSURE, atts[i + 1]))
            {
                CTFReaderExposureContrastElt* pEC =
                    dynamic_cast<CTFReaderExposureContrastElt*>(container.get());
                if (!pEC)
                {
                    ThrowM(*this, "Dynamic parameter '", atts[i + 1],
                           "' is not supported in '",
                           container->getName().c_str(), "'.");
                }

                ExposureContrastOpDataRcPtr pECOp = pEC->getExposureContrast();
                pECOp->getExposureProperty()->makeDynamic();
            }
            else if (0 == Platform::Strcasecmp(TAG_DYN_PROP_CONTRAST, atts[i + 1]))
            {
                CTFReaderExposureContrastElt* pEC =
                    dynamic_cast<CTFReaderExposureContrastElt*>(container.get());
                if (!pEC)
                {
                    ThrowM(*this, "Dynamic parameter '", atts[i + 1],
                           "' is not supported in '",
                           container->getName().c_str(), "'.");
                }

                ExposureContrastOpDataRcPtr pECOp = pEC->getExposureContrast();
                pECOp->getContrastProperty()->makeDynamic();
            }
            else if (0 == Platform::Strcasecmp(TAG_DYN_PROP_GAMMA, atts[i + 1]))
            {
                CTFReaderExposureContrastElt* pEC =
                    dynamic_cast<CTFReaderExposureContrastElt*>(container.get());
                if (!pEC)
                {
                    ThrowM(*this, "Dynamic parameter '", atts[i + 1],
                           "' is not supported in '",
                           container->getName().c_str(), "'.");
                }

                ExposureContrastOpDataRcPtr pECOp = pEC->getExposureContrast();
                pECOp->getGammaProperty()->makeDynamic();
            }
            else if (0 == Platform::Strcasecmp(TAG_DYN_PROP_PRIMARY, atts[i + 1]))
            {
                CTFReaderGradingPrimaryElt* pGP =
                    dynamic_cast<CTFReaderGradingPrimaryElt*>(container.get());
                if (!pGP)
                {
                    ThrowM(*this, "Dynamic parameter '", atts[i + 1],
                           "' is not supported in '", container->getName().c_str(), "'.");
                }

                GradingPrimaryOpDataRcPtr pGPOp = pGP->getGradingPrimary();
                pGPOp->getDynamicPropertyInternal()->makeDynamic();
            }
            else if (0 == Platform::Strcasecmp(TAG_DYN_PROP_RGBCURVE, atts[i + 1]))
            {
                CTFReaderGradingRGBCurveElt* pGC =
                    dynamic_cast<CTFReaderGradingRGBCurveElt*>(container.get());
                if (!pGC)
                {
                    ThrowM(*this, "Dynamic parameter '", atts[i + 1],
                           "' is not supported in '", container->getName().c_str(), "'.");
                }

                GradingRGBCurveOpDataRcPtr pGCOp = pGC->getGradingRGBCurve();
                pGCOp->getDynamicPropertyInternal()->makeDynamic();
            }
            else if (0 == Platform::Strcasecmp(TAG_DYN_PROP_TONE, atts[i + 1]))
            {
                CTFReaderGradingToneElt* pGT =
                    dynamic_cast<CTFReaderGradingToneElt*>(container.get());
                if (!pGT)
                {
                    ThrowM(*this, "Dynamic parameter '", atts[i + 1],
                        "' is not supported in '", container->getName().c_str(), "'.");
                }

                GradingToneOpDataRcPtr pGTOp = pGT->getGradingTone();
                pGTOp->getDynamicPropertyInternal()->makeDynamic();
            }
            else if (0 == Platform::Strcasecmp(TAG_DYN_PROP_LOOK, atts[i + 1]))
            {
                // The LOOK_SWITCH was used in CTF files from SynColor but is not supported yet
                // in OCIO.  Ignoring it is similar to setting LOOK_SWITCH to 'on' but this is
                // harmless since the defaultLook is also ignored.
                std::ostringstream dbg;
                dbg << getXmlFile().c_str() << "(" << getXmlLineNumber() << "): ";
                dbg << "Dynamic parameter '" << atts[i + 1] << "' on '";
                dbg << getParent()->getName() <<"' is ignored.";
                LogWarning(dbg.str().c_str());
            }
            else
            {
                ThrowM(*this, "Dynamic parameter '", atts[i + 1],
                       "' is not valid in '",
                       container->getName().c_str(), "'.");
            }
        }

        i += 2;
    }

}

void CTFReaderDynamicParamElt::end()
{
}

void CTFReaderDynamicParamElt::setRawData(const char * str, size_t len, unsigned int xmlLine)
{
}

//////////////////////////////////////////////////////////

CTFReaderExposureContrastElt::CTFReaderExposureContrastElt()
    : CTFReaderOpElt()
    , m_ec(std::make_shared<ExposureContrastOpData>())
{
}

CTFReaderExposureContrastElt::~CTFReaderExposureContrastElt()
{
}

void CTFReaderExposureContrastElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    bool isStyleFound = false;

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            ExposureContrastOpData::Style style = ExposureContrastOpData::STYLE_LINEAR;
            try
            {
                style = ExposureContrastOpData::ConvertStringToStyle(atts[i + 1]);

            }
            catch (Exception& ce)
            {
                ThrowM(*this, "ExposureContrast element: ", ce.what());
            }

            m_ec->setStyle(style);
            isStyleFound = true;
        }

        i += 2;
    }
    if (!isStyleFound)
    {
        throwMessage("ExposureContrast element: style missing.");
    }

}

bool CTFReaderExposureContrastElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_STYLE, att);
}

void CTFReaderExposureContrastElt::end()
{
    CTFReaderOpElt::end();

    // Validate the end result.
    m_ec->validate();
}

const OpDataRcPtr CTFReaderExposureContrastElt::getOp() const
{
    return m_ec;
}

CTFReaderECParamsElt::CTFReaderECParamsElt(const std::string & name,
                                           ContainerEltRcPtr pParent,
                                           unsigned int xmlLineNumber,
                                           const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderECParamsElt::~CTFReaderECParamsElt()
{
}

void CTFReaderECParamsElt::start(const char ** atts)
{
    // Attributes we want to extract
    double exposure = std::numeric_limits<double>::quiet_NaN();
    double contrast = std::numeric_limits<double>::quiet_NaN();
    double gamma = std::numeric_limits<double>::quiet_NaN();
    double pivot = std::numeric_limits<double>::quiet_NaN();
    double logExposureStep = std::numeric_limits<double>::quiet_NaN();
    double logMidGray = std::numeric_limits<double>::quiet_NaN();

    // Try extracting the attributes.
    unsigned i = 0;
    while (atts[i] && *atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_EXPOSURE, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], exposure);
        }
        else if (0 == Platform::Strcasecmp(ATTR_CONTRAST, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], contrast);
        }
        else if (0 == Platform::Strcasecmp(ATTR_GAMMA, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], gamma);
        }
        else if (0 == Platform::Strcasecmp(ATTR_PIVOT, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], pivot);
        }
        else if (0 == Platform::Strcasecmp(ATTR_LOGEXPOSURESTEP, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], logExposureStep);
        }
        else if (0 == Platform::Strcasecmp(ATTR_LOGMIDGRAY, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], logMidGray);
        }
        else
        {
            logParameterWarning(atts[i]);
        }

        i += 2;
    }

    CTFReaderExposureContrastElt * pEC
        = dynamic_cast<CTFReaderExposureContrastElt*>(getParent().get());

    if (IsNan(exposure))
    {
        throwMessage("ExposureContrast element: exposure missing.");
    }
    if (IsNan(contrast))
    {
        throwMessage("ExposureContrast element: contrast missing.");
    }
    if (IsNan(pivot))
    {
        throwMessage("ExposureContrast element: pivot missing.");
    }

    // Assign the parameters to the object
    pEC->getExposureContrast()->setExposure(exposure);
    pEC->getExposureContrast()->setContrast(contrast);

    // Gamma wasn't always part of the spec, therefore it's optional; use the
    // default value if not present.
    if (!IsNan(gamma))
    {
        pEC->getExposureContrast()->setGamma(gamma);
    }

    pEC->getExposureContrast()->setPivot(pivot);

    if (!IsNan(logExposureStep))
    {
        pEC->getExposureContrast()->setLogExposureStep(logExposureStep);
    }
    if (!IsNan(logMidGray))
    {
        pEC->getExposureContrast()->setLogMidGray(logMidGray);
    }
}

void CTFReaderECParamsElt::end()
{
}

void CTFReaderECParamsElt::setRawData(const char * str, size_t len, unsigned int xmlLine)
{
}

//////////////////////////////////////////////////////////

CTFReaderGammaElt::CTFReaderGammaElt()
    : m_gamma(std::make_shared<GammaOpData>())
{
}

CTFReaderGammaElt::~CTFReaderGammaElt()
{
}

void CTFReaderGammaElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);
    bool isStyleFound = false;

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            // We need a valid style to parse the parameters.
            // This will throw on unrecognized styles.
            const auto style = GammaOpData::ConvertStringToStyle(atts[i + 1]);
            if (!isValid(style))
            {

                std::ostringstream oss;
                oss << "Style not handled: '" << atts[i + 1] << "' for ";

                if (m_transform->isCLF())
                {
                    oss << "CLF file version '" << m_transform->getCLFVersion();
                }
                else
                {
                    oss << "CTF file version '" << m_transform->getCTFVersion();
                }
                oss << "'.";

                throwMessage(oss.str());
            }
            m_gamma->setStyle(style);
            isStyleFound = true;

            // Set default parameters for all channels.
            const GammaOpData::Params params = GammaOpData::getIdentityParameters(m_gamma->getStyle());
            m_gamma->setParams(params);
        }

        i += 2;
    }
    if (!isStyleFound)
    {
        throwMessage("Missing parameter 'style'.");
    }
}

bool CTFReaderGammaElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_STYLE, att);
}

void CTFReaderGammaElt::end()
{
    CTFReaderOpElt::end();

    // Validate the end result.
    try
    {
        getGamma()->validateParameters();
    }
    catch (Exception & ce)
    {
        ThrowM(*this, "Invalid parameters: ", ce.what(), ".");
    }
}

const OpDataRcPtr CTFReaderGammaElt::getOp() const
{
    return m_gamma;
}

CTFReaderGammaParamsEltRcPtr CTFReaderGammaElt::createGammaParamsElt(
    const std::string & name,
    ContainerEltRcPtr pParent,
    unsigned int xmlLineNumber,
    const std::string & xmlFile) const
{
    return std::make_shared<CTFReaderGammaParamsElt>(name, pParent, xmlLineNumber, xmlFile);
}

bool CTFReaderGammaElt::isValid(const GammaOpData::Style style) const noexcept
{
    switch (style)
    {
    case GammaOpData::BASIC_FWD:
    case GammaOpData::BASIC_REV:
    case GammaOpData::MONCURVE_FWD:
    case GammaOpData::MONCURVE_REV:
        return true;
    case GammaOpData::BASIC_MIRROR_FWD:
    case GammaOpData::BASIC_MIRROR_REV:
    case GammaOpData::BASIC_PASS_THRU_FWD:
    case GammaOpData::BASIC_PASS_THRU_REV:
    case GammaOpData::MONCURVE_MIRROR_FWD:
    case GammaOpData::MONCURVE_MIRROR_REV:
        return false;
    }
    return false;
}

CTFReaderGammaParamsEltRcPtr CTFReaderGammaElt_1_5::createGammaParamsElt(
    const std::string & name,
    ContainerEltRcPtr pParent,
    unsigned int xmlLineNumber,
    const std::string & xmlFile) const
{
    CTFReaderGammaParamsEltRcPtr res =
        std::make_shared<CTFReaderGammaParamsElt_1_5>(name, pParent, xmlLineNumber, xmlFile);
    return res;
}

bool CTFReaderGammaElt_CTF_2_0::isValid(const GammaOpData::Style style) const noexcept
{
    switch (style)
    {
    case GammaOpData::BASIC_FWD:
    case GammaOpData::BASIC_REV:
    case GammaOpData::MONCURVE_FWD:
    case GammaOpData::MONCURVE_REV:
    case GammaOpData::BASIC_MIRROR_FWD:
    case GammaOpData::BASIC_MIRROR_REV:
    case GammaOpData::BASIC_PASS_THRU_FWD:
    case GammaOpData::BASIC_PASS_THRU_REV:
    case GammaOpData::MONCURVE_MIRROR_FWD:
    case GammaOpData::MONCURVE_MIRROR_REV:
        return true;
    }
    return false;
}

CTFReaderGammaParamsEltRcPtr CTFReaderGammaElt_CLF_3_0::createGammaParamsElt(
    const std::string & name,
    ContainerEltRcPtr pParent,
    unsigned int xmlLineNumber,
    const std::string & xmlFile) const
{
    CTFReaderGammaParamsEltRcPtr res =
        std::make_shared<CTFReaderGammaParamsElt>(name, pParent, xmlLineNumber, xmlFile);
    return res;
}

//////////////////////////////////////////////////////////

CTFReaderGammaParamsElt::CTFReaderGammaParamsElt(const std::string & name,
                                                 ContainerEltRcPtr pParent,
                                                 unsigned int xmlLineNumber,
                                                 const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderGammaParamsElt::~CTFReaderGammaParamsElt()
{
}

void CTFReaderGammaParamsElt::start(const char ** atts)
{
    // Attributes we want to extract.
    int chan = -1;
    double gamma = std::numeric_limits<double>::quiet_NaN();
    double offset = std::numeric_limits<double>::quiet_NaN();

    // Try extracting the attributes.
    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_CHAN, atts[i]))
        {
            chan = getChannelNumber(atts[i + 1]);

            // Chan is optional but, if present, must be legal.
            if (chan == -1)
            {
                ThrowM(*this, "Invalid channel: ", atts[i + 1], ".");
            }
        }
        else if (0 == Platform::Strcasecmp(ATTR_GAMMA, atts[i]) ||
                 0 == Platform::Strcasecmp(ATTR_EXPONENT, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], gamma);
        }
        else if (0 == Platform::Strcasecmp(ATTR_OFFSET, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], offset);
        }
        else
        {
            logParameterWarning(atts[i]);
        }

        i += 2;
    }

    // Validate the attributes are appropriate for the gamma style and set
    // the parameters (numeric validation is done by GammaOp::validate).

    CTFReaderGammaElt * pGamma
        = dynamic_cast<CTFReaderGammaElt*>(getParent().get());

    GammaOpData::Params params;

    const GammaOpData::Style style = pGamma->getGamma()->getStyle();
    switch (style)
    {
    case GammaOpData::BASIC_FWD:
    case GammaOpData::BASIC_REV:
    case GammaOpData::BASIC_MIRROR_FWD:
    case GammaOpData::BASIC_MIRROR_REV:
    case GammaOpData::BASIC_PASS_THRU_FWD:
    case GammaOpData::BASIC_PASS_THRU_REV:
    {
        if (IsNan(gamma))
        {
            ThrowM(*this, "Missing required gamma parameter for style: ",
                   GammaOpData::ConvertStyleToString(style),
                   ".");
        }
        params.push_back(gamma);

        if (!IsNan(offset))
        {
            ThrowM(*this, "Illegal offset parameter for style: ",
                   GammaOpData::ConvertStyleToString(style),
                   ".");
        }
        break;
    }

    case GammaOpData::MONCURVE_FWD:
    case GammaOpData::MONCURVE_REV:
    case GammaOpData::MONCURVE_MIRROR_FWD:
    case GammaOpData::MONCURVE_MIRROR_REV:
    {
        if (IsNan(gamma))
        {
            ThrowM(*this, "Missing required gamma parameter for style: ",
                   GammaOpData::ConvertStyleToString(style),
                   ".");
        }
        params.push_back(gamma);

        if (IsNan(offset))
        {
            ThrowM(*this, "Missing required offset parameter for style: ",
                   GammaOpData::ConvertStyleToString(style),
                   ".");
        }
        params.push_back(offset);
        break;
    }
    }

    // Assign the parameters to the object.
    switch (chan)
    {
    case -1:
        pGamma->getGamma()->setParams(params);
        break;
    case 0:
        pGamma->getGamma()->setRedParams(params);
        break;
    case 1:
        pGamma->getGamma()->setGreenParams(params);
        break;
    case 2:
        pGamma->getGamma()->setBlueParams(params);
        break;
    case 3:
        pGamma->getGamma()->setAlphaParams(params);
        break;
    }
}

void CTFReaderGammaParamsElt::end()
{
}

void CTFReaderGammaParamsElt::setRawData(const char * , size_t , unsigned int )
{
}

int CTFReaderGammaParamsElt::getChannelNumber(const char * name) const
{
    // Version prior to 1.3 only supports R, G and B channels.

    int chan = -1;
    if (0 == Platform::Strcasecmp("R", name))
    {
        chan = 0;
    }
    else if (0 == Platform::Strcasecmp("G", name))
    {
        chan = 1;
    }
    else if (0 == Platform::Strcasecmp("B", name))
    {
        chan = 2;
    }
    return chan;
}

CTFReaderGammaParamsElt_1_5::CTFReaderGammaParamsElt_1_5(const std::string & name,
                                                         ContainerEltRcPtr pParent,
                                                         unsigned int xmlLineNumber,
                                                         const std::string & xmlFile)
    : CTFReaderGammaParamsElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderGammaParamsElt_1_5::~CTFReaderGammaParamsElt_1_5()
{
}

int CTFReaderGammaParamsElt_1_5::getChannelNumber(const char * name) const
{
    // Version equal or greater than 1.5 supports R, G, B and A channels.

    int chan = -1;
    if (0 == Platform::Strcasecmp("A", name))
    {
        chan = 3;
    }
    else
    {
        chan = CTFReaderGammaParamsElt::getChannelNumber(name);
    }
    return chan;
}

//////////////////////////////////////////////////////////

CTFReaderGradingPrimaryElt::CTFReaderGradingPrimaryElt()
    : m_gradingPrimaryOpData(std::make_shared<GradingPrimaryOpData>(GRADING_LOG))
{
}

bool CTFReaderGradingPrimaryElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_STYLE, att);
}

void CTFReaderGradingPrimaryElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    bool isStyleFound = false;

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            try
            {
                GradingStyle style;
                TransformDirection dir;
                ConvertStringToGradingStyleAndDir(atts[i + 1], style, dir);
                m_gradingPrimaryOpData->setStyle(style);
                m_gradingPrimaryOpData->setDirection(dir);
                // Initialize default values from the style.
                const GradingPrimary values(style);
                m_gradingPrimary = values;
            }
            catch (Exception &)
            {
                ThrowM(*this, "Required attribute 'style' '", atts[i + 1], "' is invalid.");
            }
            isStyleFound = true;
        }

        i += 2;
    }

    if (!isStyleFound)
    {
        ThrowM(*this, "Required attribute 'style' is missing.");
    }
}

void CTFReaderGradingPrimaryElt::end()
{
    CTFReaderOpElt::end();

    m_gradingPrimaryOpData->setValue(m_gradingPrimary);
    m_gradingPrimaryOpData->validate();
}

const OpDataRcPtr CTFReaderGradingPrimaryElt::getOp() const
{
    return m_gradingPrimaryOpData;
}

//////////////////////////////////////////////////////////

CTFReaderGradingPrimaryParamElt::CTFReaderGradingPrimaryParamElt(const std::string & name,
                                                 ContainerEltRcPtr pParent,
                                                 unsigned int xmlLineNumber,
                                                 const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderGradingPrimaryParamElt::~CTFReaderGradingPrimaryParamElt()
{
}

void CTFReaderGradingPrimaryParamElt::parseRGBMAttrValues(const char ** atts,
                                                          GradingRGBM & rgbm) const
{
    bool rgbFound = false;
    bool masterFound = false;
    unsigned i = 0;
    while (atts[i] && *atts[i])
    {
        const size_t len = strlen(atts[i + 1]);
        std::vector<double> data;

        try
        {
            data = GetNumbers<double>(atts[i + 1], len);
        }
        catch (Exception & ce)
        {
            ThrowM(*this, "Illegal '", getTypeName(), "' values ",
                   TruncateString(atts[i + 1], len), " [", ce.what(), "].");
        }

        if (0 == Platform::Strcasecmp(ATTR_RGB, atts[i]))
        {
            if (data.size() != 3)
            {
                ThrowM(*this, "Illegal number of 'rgb' values for '", getTypeName(), "': '",
                       TruncateString(atts[i + 1], len), "'.");
            }

            rgbm.m_red = data[0];
            rgbm.m_green = data[1];
            rgbm.m_blue = data[2];

            rgbFound = true;
        }
        else if (0 == Platform::Strcasecmp(ATTR_MASTER, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'Master' for '", getTypeName(), "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'");
            }

            rgbm.m_master = data[0];
            masterFound = true;
        }
        else
        {
            ThrowM(*this, "Illegal attribute for '", getName().c_str(), "': '", atts[i], "'.");
        }

        i += 2;
    }

    if (!rgbFound)
    {
        ThrowM(*this, "Missing 'rgb' attribute for '", getName().c_str(), "'.");
    }

    if (!masterFound)
    {
        ThrowM(*this, "Missing 'master' attribute for '", getName().c_str(), "'.");
    }
}

void CTFReaderGradingPrimaryParamElt::parseBWAttrValues(const char ** atts,
                                                        double & black,
                                                        double & white) const
{
    bool blackFound = false;
    bool whiteFound = false;
    unsigned i = 0;
    while (atts[i] && *atts[i])
    {
        const size_t len = strlen(atts[i + 1]);
        std::vector<double> data;

        try
        {
            data = GetNumbers<double>(atts[i + 1], len);
        }
        catch (Exception & ce)
        {
            ThrowM(*this, "Illegal '", getTypeName(), "' values ",
                TruncateString(atts[i + 1], len), " [", ce.what(), "].");
        }

        if (0 == Platform::Strcasecmp(ATTR_PRIMARY_BLACK, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'Black' for '", getTypeName(), "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'.");
            }

            black = data[0];
            blackFound = true;
        }
        else if (0 == Platform::Strcasecmp(ATTR_PRIMARY_WHITE, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'White' for '", getTypeName(), "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'.");
            }

            white = data[0];
            whiteFound = true;
        }
        else
        {
            ThrowM(*this, "Illegal attribute for '", getName().c_str(), "': '", atts[i], "'.");
        }

        i += 2;
    }

    if (!blackFound && !whiteFound)
    {
        ThrowM(*this, "Missing 'black' or 'white' attribute for '", getName().c_str(), "'.");
    }
}

void CTFReaderGradingPrimaryParamElt::parsePivotAttrValues(const char ** atts,
                                                           double & contrast,
                                                           double & black,
                                                           double & white) const
{
    bool contrastFound = false;
    bool blackFound = false;
    bool whiteFound = false;
    unsigned i = 0;
    while (atts[i] && *atts[i])
    {
        const size_t len = strlen(atts[i + 1]);
        std::vector<double> data;

        try
        {
            data = GetNumbers<double>(atts[i + 1], len);
        }
        catch (Exception & ce)
        {
            ThrowM(*this, "Illegal '", getTypeName(), "' values ",
                TruncateString(atts[i + 1], len), " [", ce.what(), "].");
        }

        if (0 == Platform::Strcasecmp(ATTR_PRIMARY_BLACK, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'Black' for '", getTypeName(), "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'.");
            }

            black = data[0];
            blackFound = true;
        }
        else if (0 == Platform::Strcasecmp(ATTR_PRIMARY_WHITE, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'White' for '", getTypeName(), "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'.");
            }

            white = data[0];
            whiteFound = true;
        }
        else if (0 == Platform::Strcasecmp(ATTR_PRIMARY_CONTRAST, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'Contrast' for '", getTypeName(), "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'.");
            }

            contrast = data[0];
            contrastFound = true;
        }
        else
        {
            ThrowM(*this, "Illegal attribute for '", getName().c_str(), "': '", atts[i], "'.");
        }

        i += 2;
    }

    if (!contrastFound && !whiteFound && !blackFound)
    {
        ThrowM(*this, "Missing 'contrast', 'black' or 'white' attribute for '",
               getName().c_str(), "'.");
    }
}

void CTFReaderGradingPrimaryParamElt::parseScalarAttrValue(const char ** atts,
                                                           const char * tag,
                                                           double & value) const
{
    bool found = false;
    unsigned i = 0;
    while (atts[i] && *atts[i])
    {
        const size_t len = strlen(atts[i + 1]);
        std::vector<double> data;

        try
        {
            data = GetNumbers<double>(atts[i + 1], len);
        }
        catch (Exception & ce)
        {
            ThrowM(*this, "Illegal '", getTypeName(), "' values ",
                   TruncateString(atts[i + 1], len), " [", ce.what(), "].");
        }

        if (0 == Platform::Strcasecmp(tag, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'", tag, "' for '", getTypeName(), "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'.");
            }

            value = data[0];
            found = true;
        }
        else
        {
            ThrowM(*this, "Illegal attribute for '", getName().c_str(), "': '", atts[i], "'.");
        }

        i += 2;
    }

    if (!found)
    {
        ThrowM(*this, "Missing attribute for '", getName().c_str(), "'.");
    }
}

void CTFReaderGradingPrimaryParamElt::start(const char ** atts)
{
    auto gp = dynamic_cast<CTFReaderGradingPrimaryElt *>(getParent().get());

    GradingPrimary & gpValues = gp->getValue();

    // Read all primary controls even those that are not used by the current style.
    if (0 == Platform::Strcasecmp(TAG_PRIMARY_BRIGHTNESS, getName().c_str()))
    {
        parseRGBMAttrValues(atts, gpValues.m_brightness);
    }
    else if (0 == Platform::Strcasecmp(TAG_PRIMARY_CONTRAST, getName().c_str()))
    {
        parseRGBMAttrValues(atts, gpValues.m_contrast);
    }
    else if (0 == Platform::Strcasecmp(TAG_PRIMARY_GAMMA, getName().c_str()))
    {
        parseRGBMAttrValues(atts, gpValues.m_gamma);
    }
    else if (0 == Platform::Strcasecmp(TAG_PRIMARY_PIVOT, getName().c_str()))
    {
        parsePivotAttrValues(atts, gpValues.m_pivot, gpValues.m_pivotBlack, gpValues.m_pivotWhite);
    }
    else if (0 == Platform::Strcasecmp(TAG_PRIMARY_SATURATION, getName().c_str()))
    {
        parseScalarAttrValue(atts, ATTR_MASTER, gpValues.m_saturation);
    }
    else if (0 == Platform::Strcasecmp(TAG_PRIMARY_OFFSET, getName().c_str()))
    {
        parseRGBMAttrValues(atts, gpValues.m_offset);
    }
    else if (0 == Platform::Strcasecmp(TAG_PRIMARY_EXPOSURE, getName().c_str()))
    {
        parseRGBMAttrValues(atts, gpValues.m_exposure);
    }
    else if (0 == Platform::Strcasecmp(TAG_PRIMARY_LIFT, getName().c_str()))
    {
        parseRGBMAttrValues(atts, gpValues.m_lift);
    }
    else if (0 == Platform::Strcasecmp(TAG_PRIMARY_GAIN, getName().c_str()))
    {
        parseRGBMAttrValues(atts, gpValues.m_gain);
    }
    else if (0 == Platform::Strcasecmp(TAG_PRIMARY_CLAMP, getName().c_str()))
    {
        parseBWAttrValues(atts, gpValues.m_clampBlack, gpValues.m_clampWhite);
    }
}

void CTFReaderGradingPrimaryParamElt::end()
{
}

void CTFReaderGradingPrimaryParamElt::setRawData(const char*, size_t, unsigned)
{
}

//////////////////////////////////////////////////////////

CTFReaderGradingRGBCurveElt::CTFReaderGradingRGBCurveElt()
    : m_gradingRGBCurve(std::make_shared<GradingRGBCurveOpData>(GRADING_LOG))
{
}

bool CTFReaderGradingRGBCurveElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_STYLE, att) ||
           0 == Platform::Strcasecmp(ATTR_BYPASS_LIN_TO_LOG, att);
}

void CTFReaderGradingRGBCurveElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    bool isStyleFound = false;

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            try
            {
                GradingStyle style;
                TransformDirection dir;
                ConvertStringToGradingStyleAndDir(atts[i + 1], style, dir);
                m_gradingRGBCurve->setStyle(style);
                m_gradingRGBCurve->setDirection(dir);

                // Initialize loading curve with corresponding style.
                m_loadingRGBCurve = GradingRGBCurve::Create(style);
            }
            catch (Exception &)
            {
                ThrowM(*this, "Required attribute 'style' '", atts[i + 1], "' is invalid.");
            }
            isStyleFound = true;
        }
        else if (0 == Platform::Strcasecmp(ATTR_BYPASS_LIN_TO_LOG, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                std::ostringstream oss;
                oss << "Unknown bypassLinToLog value: '" << atts[i + 1];
                oss << "' while parsing RGBCurve.";
                throwMessage(oss.str());
            }

            m_gradingRGBCurve->setBypassLinToLog(true);
        }

        i += 2;
    }

    if (!isStyleFound)
    {
        ThrowM(*this, "Required attribute 'style' is missing.");
    }
}

void CTFReaderGradingRGBCurveElt::end()
{
    CTFReaderOpElt::end();

    // Set the loaded data.
    m_gradingRGBCurve->setValue(m_loadingRGBCurve);
    // Validate the end result.
    m_gradingRGBCurve->validate();
}

const OpDataRcPtr CTFReaderGradingRGBCurveElt::getOp() const
{
    return m_gradingRGBCurve;
}

//////////////////////////////////////////////////////////

CTFReaderGradingCurveElt::CTFReaderGradingCurveElt(const std::string & name,
                                                   ContainerEltRcPtr pParent,
                                                   unsigned int xmlLineNumber,
                                                   const std::string & xmlFile)
    : XmlReaderComplexElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderGradingCurveElt::~CTFReaderGradingCurveElt()
{
}

namespace
{
RGBCurveType GetRGBCurveType(const std::string & name)
{
    if (0 == Platform::Strcasecmp(TAG_RGB_CURVE_RED, name.c_str()))
    {
        return RGB_RED;
    }
    else if (0 == Platform::Strcasecmp(TAG_RGB_CURVE_GREEN, name.c_str()))
    {
        return RGB_GREEN;
    }
    else if (0 == Platform::Strcasecmp(TAG_RGB_CURVE_BLUE, name.c_str()))
    {
        return RGB_BLUE;
    }
    else if (0 == Platform::Strcasecmp(TAG_RGB_CURVE_MASTER, name.c_str()))
    {
        return RGB_MASTER;
    }
    std::ostringstream err;
    err << "Invalid curve name '" << name << "'.";
    throw Exception(err.str().c_str());
}
}

void CTFReaderGradingCurveElt::start(const char ** atts)
{
    try
    {
        const RGBCurveType type = GetRGBCurveType(getName());
        auto pRGBCurveElt = dynamic_cast<CTFReaderGradingRGBCurveElt*>(getParent().get());
        m_curve = pRGBCurveElt->getLoadingRGBCurve()->getCurve(type);
    }
    catch (Exception& ce)
    {
        throwMessage(ce.what());
    }
}

void CTFReaderGradingCurveElt::end()
{
}

//////////////////////////////////////////////////////////

CTFReaderGradingCurvePointsElt::CTFReaderGradingCurvePointsElt(const std::string & name,
                                                              ContainerEltRcPtr pParent,
                                                              unsigned int xmlLineNumber,
                                                              const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderGradingCurvePointsElt::~CTFReaderGradingCurvePointsElt()
{
}

void CTFReaderGradingCurvePointsElt::start(const char ** atts)
{
}

void CTFReaderGradingCurvePointsElt::end()
{
    if (m_data.size() % 2 != 0)
    {
        throwMessage("Control points element: odd number of values.");
    }

    auto pCurve = dynamic_cast<CTFReaderGradingCurveElt*>(getParent().get());
    const size_t numPts = m_data.size() / 2;
    auto curve = pCurve->getCurve();
    curve->setNumControlPoints(numPts);
    for (size_t p = 0; p < numPts; ++p)
    {
        auto & ctPt = curve->getControlPoint(p);
        ctPt.m_x = m_data[2 * p];
        ctPt.m_y = m_data[2 * p + 1];
    }
}

void CTFReaderGradingCurvePointsElt::setRawData(const char* s, size_t len, unsigned int xmlLine)
{
    std::vector<float> data;

    try
    {
        data = GetNumbers<float>(s, len);
    }
    catch (Exception & ce)
    {
        ThrowM(*this, "Illegal '", getTypeName(), "' values ",
               TruncateString(s, len), " [", ce.what(), "]");
    }
    m_data.insert(m_data.end(), data.begin(), data.end());
}

//////////////////////////////////////////////////////////

CTFReaderGradingCurveSlopesElt::CTFReaderGradingCurveSlopesElt(const std::string & name,
                                                               ContainerEltRcPtr pParent,
                                                               unsigned int xmlLineNumber,
                                                               const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderGradingCurveSlopesElt::~CTFReaderGradingCurveSlopesElt()
{
}

void CTFReaderGradingCurveSlopesElt::start(const char ** atts)
{
}

void CTFReaderGradingCurveSlopesElt::end()
{
    auto pCurve = dynamic_cast<CTFReaderGradingCurveElt*>(getParent().get());
    const size_t numVals = m_data.size();
    auto curve = pCurve->getCurve();
    const size_t numCtPnts = curve->getNumControlPoints();
    if (numVals != numCtPnts)
    {
        throwMessage("Number of slopes must match number of control points.");
    }
    for (size_t i = 0; i < numVals; ++i)
    {
        curve->setSlope(i, m_data[i]);
    }
}

void CTFReaderGradingCurveSlopesElt::setRawData(const char* s, size_t len, unsigned int xmlLine)
{
    std::vector<float> data;

    try
    {
        data = GetNumbers<float>(s, len);
    }
    catch (Exception & ce)
    {
        ThrowM(*this, "Illegal '", getTypeName(), "' values ",
               TruncateString(s, len), " [", ce.what(), "]");
    }
    m_data.insert(m_data.end(), data.begin(), data.end());
}

//////////////////////////////////////////////////////////

CTFReaderGradingToneElt::CTFReaderGradingToneElt()
    : m_gradingTone(std::make_shared<GradingToneOpData>(GRADING_LOG))
{
}

bool CTFReaderGradingToneElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
        0 == Platform::Strcasecmp(ATTR_STYLE, att);
}

void CTFReaderGradingToneElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    bool isStyleFound = false;

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            try
            {
                GradingStyle style;
                TransformDirection dir;
                ConvertStringToGradingStyleAndDir(atts[i + 1], style, dir);
                m_gradingTone->setStyle(style);
                m_gradingTone->setDirection(dir);
                // Initialize default values from the style.
                const GradingTone values(style);
                m_gradingTone->setValue(values);
            }
            catch (Exception &)
            {
                ThrowM(*this, "Required attribute 'style' '", atts[i + 1], "' is invalid.");
            }
            isStyleFound = true;
        }

        i += 2;
    }

    if (!isStyleFound)
    {
        ThrowM(*this, "Required attribute 'style' is missing.");
    }
}

void CTFReaderGradingToneElt::end()
{
    CTFReaderOpElt::end();

    // Validate the end result
    m_gradingTone->validate();
}

const OpDataRcPtr CTFReaderGradingToneElt::getOp() const
{
    return m_gradingTone;
}

//////////////////////////////////////////////////////////

CTFReaderGradingToneParamElt::CTFReaderGradingToneParamElt(const std::string & name,
    ContainerEltRcPtr pParent,
    unsigned int xmlLineNumber,
    const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderGradingToneParamElt::~CTFReaderGradingToneParamElt()
{
}

void CTFReaderGradingToneParamElt::parseRGBMSWAttrValues(const char ** atts,
                                                         GradingRGBMSW & rgbm,
                                                         bool center, bool pivot) const
{
    bool rgbFound = false;
    bool masterFound = false;
    bool startFound = false;
    bool widthFound = false;
    unsigned i = 0;
    while (atts[i] && *atts[i])
    {
        const size_t len = strlen(atts[i + 1]);
        std::vector<double> data;

        try
        {
            data = GetNumbers<double>(atts[i + 1], len);
        }
        catch (Exception & ce)
        {
            ThrowM(*this, "Illegal '", getTypeName(), "' values ",
                   TruncateString(atts[i + 1], len), " [", ce.what(), "].");
        }

        if (0 == Platform::Strcasecmp(ATTR_RGB, atts[i]))
        {
            if (data.size() != 3)
            {
                ThrowM(*this, "Illegal number of 'rgb' values for '", getTypeName(), "': '",
                       TruncateString(atts[i + 1], len), "'.");
            }

            rgbm.m_red = data[0];
            rgbm.m_green = data[1];
            rgbm.m_blue = data[2];

            rgbFound = true;
        }
        else if (0 == Platform::Strcasecmp(ATTR_MASTER, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'Master' for '", getTypeName(), "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'");
            }

            rgbm.m_master = data[0];
            masterFound = true;
        }
        else if (0 == Platform::Strcasecmp(center ? ATTR_CENTER : ATTR_START, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'", center ? ATTR_CENTER : ATTR_START, "' for '", getTypeName(),
                       "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'");
            }

            rgbm.m_start = data[0];
            startFound = true;
        }
        else if (0 == Platform::Strcasecmp(pivot ? ATTR_PIVOT : ATTR_WIDTH, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'", pivot ? ATTR_PIVOT : ATTR_WIDTH, "' for '", getTypeName(),
                       "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'");
            }

            rgbm.m_width = data[0];
            widthFound = true;
        }
        else
        {
            ThrowM(*this, "Illegal attribute for '", getName().c_str(), "': '", atts[i], "'.");
        }

        i += 2;
    }

    if (!rgbFound)
    {
        ThrowM(*this, "Missing 'rgb' attribute for '", getName().c_str(), "'.");
    }
    if (!masterFound)
    {
        ThrowM(*this, "Missing 'master' attribute for '", getName().c_str(), "'.");
    }
    if (!startFound)
    {
        ThrowM(*this, "Missing '", center ? ATTR_CENTER : ATTR_START, "' attribute for '",
               getName().c_str(), "'.");
    }
    if (!widthFound)
    {
        ThrowM(*this, "Missing '", pivot ? ATTR_PIVOT : ATTR_WIDTH, "' attribute for '",
               getName().c_str(), "'.");
    }
}

void CTFReaderGradingToneParamElt::parseScalarAttrValue(const char ** atts,
                                                        const char * tag,
                                                        double & value) const
{
    bool found = false;
    unsigned i = 0;
    while (atts[i] && *atts[i])
    {
        const size_t len = strlen(atts[i + 1]);
        std::vector<double> data;

        try
        {
            data = GetNumbers<double>(atts[i + 1], len);
        }
        catch (Exception & ce)
        {
            ThrowM(*this, "Illegal '", getTypeName(), "' values ",
                   TruncateString(atts[i + 1], len), " [", ce.what(), "].");
        }

        if (0 == Platform::Strcasecmp(tag, atts[i]))
        {
            if (data.size() != 1)
            {
                ThrowM(*this, "'", tag, "' for '", getTypeName(), "' must be a single value: '",
                       TruncateString(atts[i + 1], len), "'.");
            }

            value = data[0];
            found = true;
        }
        else
        {
            ThrowM(*this, "Illegal attribute for '", getName().c_str(), "': '", atts[i], "'.");
        }

        i += 2;
    }

    if (!found)
    {
        ThrowM(*this, "Missing attribute for '", getName().c_str(), "'.");
    }
}

void CTFReaderGradingToneParamElt::start(const char ** atts)
{
    auto gt = dynamic_cast<CTFReaderGradingToneElt *>(getParent().get());

    auto gtValues = gt->getGradingTone()->getValue();

    if (0 == Platform::Strcasecmp(TAG_TONE_BLACKS, getName().c_str()))
    {
        parseRGBMSWAttrValues(atts, gtValues.m_blacks, false, false);
    }
    else if (0 == Platform::Strcasecmp(TAG_TONE_SHADOWS, getName().c_str()))
    {
        parseRGBMSWAttrValues(atts, gtValues.m_shadows, false, true);
    }
    else if (0 == Platform::Strcasecmp(TAG_TONE_MIDTONES, getName().c_str()))
    {
        parseRGBMSWAttrValues(atts, gtValues.m_midtones, true, false);
    }
    else if (0 == Platform::Strcasecmp(TAG_TONE_HIGHLIGHTS, getName().c_str()))
    {
        parseRGBMSWAttrValues(atts, gtValues.m_highlights, false, true);
    }
    else if (0 == Platform::Strcasecmp(TAG_TONE_WHITES, getName().c_str()))
    {
        parseRGBMSWAttrValues(atts, gtValues.m_whites, false, false);
    }
    else if (0 == Platform::Strcasecmp(TAG_TONE_SCONTRAST, getName().c_str()))
    {
        parseScalarAttrValue(atts, ATTR_MASTER, gtValues.m_scontrast);
    }
    else
    {
        ThrowM(*this, "Invalid element '", getName().c_str(), "'.");
    }

    gt->getGradingTone()->setValue(gtValues);
}

void CTFReaderGradingToneParamElt::end()
{
}

void CTFReaderGradingToneParamElt::setRawData(const char*, size_t, unsigned)
{
}

//////////////////////////////////////////////////////////

CTFReaderInvLut1DElt::CTFReaderInvLut1DElt()
    : m_invLut(std::make_shared<Lut1DOpData>(2, TRANSFORM_DIR_INVERSE))
{
}

CTFReaderInvLut1DElt::~CTFReaderInvLut1DElt()
{
}

void CTFReaderInvLut1DElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    // The interpolation attribute is optional in CLF/CTF.  The INTERP_DEFAULT
    // enum indicates that the value was not specified in the file.  When 
    // writing, this means no interpolation attribute will be added.
    m_invLut->setInterpolation(INTERP_DEFAULT);

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_INTERPOLATION, atts[i]))
        {
            try
            {
                Interpolation interp = GetInterpolation1D(atts[i + 1]);
                m_invLut->setInterpolation(interp);
            }
            catch (const std::exception & e)
            {
                throwMessage(e.what());
            }
        }

        if (0 == Platform::Strcasecmp(ATTR_HALF_DOMAIN, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                std::ostringstream oss;
                oss << "Unknown halfDomain value: '" << atts[i + 1];
                oss << "' while parsing InvLut1D.";
                throwMessage(oss.str());
            }

            m_invLut->setInputHalfDomain(true);
        }

        if (0 == Platform::Strcasecmp(ATTR_RAW_HALFS, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                std::ostringstream oss;
                oss << "Unknown rawHalfs value: '" << atts[i + 1];
                oss << "' while parsing InvLut1D.";
                throwMessage(oss.str());
            }

            m_invLut->setOutputRawHalfs(true);
        }

        if (0 == Platform::Strcasecmp(ATTR_HUE_ADJUST, atts[i]))
        {
            if (0 != Platform::Strcasecmp("dw3", atts[i + 1]))
            {
                std::ostringstream oss;
                oss << "Unknown hueAdjust value: '" << atts[i + 1];
                oss << "' while parsing InvLut1D.";
                throwMessage(oss.str());
            }

            m_invLut->setHueAdjust(HUE_DW3);
        }

        i += 2;
    }
}

bool CTFReaderInvLut1DElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_INTERPOLATION, att) ||
           0 == Platform::Strcasecmp(ATTR_HALF_DOMAIN, att) ||
           0 == Platform::Strcasecmp(ATTR_RAW_HALFS, att) ||
           0 == Platform::Strcasecmp(ATTR_HUE_ADJUST, att);
}

void CTFReaderInvLut1DElt::end()
{
    CTFReaderOpElt::end();

    // Array values for inverse LUTs are scaled based on the inBitDepth
    // (rather than outBD), normalize them.
    const float scale = 1.0f / (float)GetBitDepthMaxValue(m_inBitDepth);
    m_invLut->scale(scale);

    // Record the original array scaling present in the file.  This is used by
    // a heuristic involved with LUT inversion.  The bit-depth of ops is
    // typically changed after the file is read, hence the need to store it now.
    // For an inverse LUT, it's the input bit-depth that describes the array scaling.
    m_invLut->setFileOutputBitDepth(m_inBitDepth);
    m_invLut->validate();
}

const OpDataRcPtr CTFReaderInvLut1DElt::getOp() const
{
    return m_invLut;
}

ArrayBase * CTFReaderInvLut1DElt::updateDimension(const Dimensions & dims)
{
    if (dims.size() != 2)
    {
        return nullptr;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size() - 1));
    const unsigned int numColorComponents = dims[max];

    if (dims[1] != 3 && dims[1] != 1)
    {
        return nullptr;
    }

    Array * pArray = &m_invLut->getArray();
    pArray->resize(dims[0], numColorComponents);
    return pArray;
}

void CTFReaderInvLut1DElt::endArray(unsigned int position)
{
    Array * pArray = &m_invLut->getArray();

    // Convert half bits to float values if needed.
    if (m_invLut->isOutputRawHalfs())
    {
        const size_t maxValues = pArray->getNumValues();
        for (size_t i = 0; i<maxValues; ++i)
        {
            pArray->getValues()[i]
                = ConvertHalfBitsToFloat((unsigned short)pArray->getValues()[i]);
        }
    }

    if (pArray->getNumValues() != position)
    {
        const unsigned long numColorComponents = pArray->getNumColorComponents();
        const unsigned long maxColorComponents = pArray->getMaxColorComponents();

        const unsigned long dimensions = pArray->getLength();

        if (numColorComponents != 1 || position != dimensions)
        {
            std::ostringstream arg;
            arg << "Expected " << dimensions << "x" << numColorComponents;
            arg << " Array values, found " << position << ".";
            throwMessage(arg.str());
        }

        // Convert a 1D LUT to a 3by1D LUT
        // (duplicate values from the Red to the Green and Blue).
        const unsigned long numLuts = maxColorComponents;

        // TODO: Should improve Lut1DOp so that the copy is unnecessary.
        for (long i = (dimensions - 1); i >= 0; --i)
        {
            for (unsigned long j = 0; j<numLuts; ++j)
            {
                pArray->getValues()[(i*numLuts) + j]
                    = pArray->getValues()[i];
            }
        }
    }

    pArray->validate();

    // At this point, we have created the complete Lut1D base class.
    // Finalize will finish initializing as an inverse Lut1D.

    setCompleted(true);
}

//////////////////////////////////////////////////////////

CTFReaderInvLut3DElt::CTFReaderInvLut3DElt()
    : m_invLut(std::make_shared<Lut3DOpData>(2, TRANSFORM_DIR_INVERSE))
{
}

CTFReaderInvLut3DElt::~CTFReaderInvLut3DElt()
{
}

void CTFReaderInvLut3DElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    // The interpolation attribute is optional in CLF/CTF.  The INTERP_DEFAULT
    // enum indicates that the value was not specified in the file.  When 
    // writing, this means no interpolation attribute will be added.
    m_invLut->setInterpolation(INTERP_DEFAULT);

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_INTERPOLATION, atts[i]))
        {
            try
            {
                Interpolation interp = GetInterpolation3D(atts[i + 1]);
                m_invLut->setInterpolation(interp);
            }
            catch (const std::exception& e)
            {
                throwMessage(e.what());
            }
        }

        i += 2;
    }
}

bool CTFReaderInvLut3DElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_INTERPOLATION, att);
}

void CTFReaderInvLut3DElt::end()
{
    CTFReaderOpElt::end();

    // Array values for inverse LUTs are scaled based on the inBitDepth
    // (rather than outBD), normalize them.
    const float scale = 1.0f / (float)GetBitDepthMaxValue(m_inBitDepth);
    m_invLut->scale(scale);

    // For an inverse LUT, it's the input bit-depth that describes the array scaling.
    m_invLut->setFileOutputBitDepth(m_inBitDepth);
    m_invLut->validate();
}

const OpDataRcPtr CTFReaderInvLut3DElt::getOp() const
{
    return m_invLut;
}

ArrayBase * CTFReaderInvLut3DElt::updateDimension(const Dimensions & dims)
{
    if (dims.size() != 4)
    {
        return nullptr;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size() - 1));
    const unsigned int numColorComponents = dims[max];

    if (dims[3] != 3 || dims[1] != dims[0] || dims[2] != dims[0])
    {
        return nullptr;
    }

    Array * pArray = &m_invLut->getArray();
    pArray->resize(dims[0], numColorComponents);
    return pArray;
}

void CTFReaderInvLut3DElt::endArray(unsigned int position)
{
    Array * pArray = &m_invLut->getArray();

    if (pArray->getNumValues() != position)
    {
        const unsigned long len = pArray->getLength();
        std::ostringstream arg;
        arg << "Expected " << len << "x" << len << "x" << len << "x";
        arg << pArray->getNumColorComponents();
        arg << " Array values, found " << position << ".";
        throwMessage(arg.str());
    }

    pArray->validate();

    // At this point, we have created the complete Lut3D
    // Finalize will finish initializing as an inverse Lut3D.

    setCompleted(true);
}

//////////////////////////////////////////////////////////

CTFReaderLogElt::CTFReaderLogElt()
    : CTFReaderOpElt()
    , m_log(std::make_shared<LogOpData>(2.0, TRANSFORM_DIR_FORWARD))
{
}

CTFReaderLogElt::~CTFReaderLogElt()
{
}

void CTFReaderLogElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    bool isStyleFound = false;
    for (unsigned i = 0; atts[i]; i += 2)
    {
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            try
            {
                m_ctfParams.m_style = LogUtil::ConvertStringToStyle(atts[i + 1]);
            }
            catch (Exception &)
            {
                ThrowM(*this, "Required attribute 'style' '", atts[i + 1], "' is invalid.");
            }
            isStyleFound = true;
        }
    }

    if (!isStyleFound)
    {
        throwMessage("CTF/CLF Log parsing. Required attribute 'style' is missing.");
    }
}

bool CTFReaderLogElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_STYLE, att);
}

void CTFReaderLogElt::end()
{
    CTFReaderOpElt::end();

    // When style is log2, log10 (or anti), there are no params.
    double base = 2.0;
    LogOpData::Params rParams, gParams, bParams;
    const auto dir = LogUtil::GetLogDirection(m_ctfParams.m_style);

    try
    {
        // This handles all log styles.
        LogUtil::ConvertLogParameters(m_ctfParams, base, rParams, gParams, bParams);
    }
    catch (Exception& ce)
    {
        ThrowM(*this, "Parameters are not valid: '", ce.what(), "'.");
    }

    m_log->setBase(base);
    m_log->setDirection(dir);
    m_log->setRedParams(rParams);
    m_log->setGreenParams(gParams);
    m_log->setBlueParams(bParams);

    // Validate the end result.
    try
    {
        m_log->validate();
    }
    catch (Exception& ce)
    {
        ThrowM(*this, "Log is not valid: '", ce.what(), "'.");
    }
}

const OpDataRcPtr CTFReaderLogElt::getOp() const
{
    return m_log;
}

void CTFReaderLogElt::setBase(double base)
{
    if (m_baseSet)
    {
        const double curBase = m_log->getBase();
        if (curBase != base)
        {
            ThrowM(*this, "Log base has to be the same on all components: ",
                   "Current base: ", curBase, ", new base: ", base, ".");
        }
    }
    else
    {
        m_baseSet = true;
        m_log->setBase(base);
    }
}

// CLF v3 added support for a Log op that is based on the OCIO-type params (log/lin slope/offset,
// etc.).  CTF had historically supported a LogOp with Cineon-type params (refWhite, gamma, etc.).
// Here is what the various versions support:
// CTF pre v2 -- Cineon-type only.
// CTF v2 and later -- OCIO-type preferred but Cineon-type still supported.
// CLF pre v3 -- No Log support.
// CLF v3 and onward -- OCIO-type.

CTFReaderLogParamsEltRcPtr CTFReaderLogElt::createLogParamsElt(
    const std::string & name,
    ContainerEltRcPtr pParent,
    unsigned int xmlLineNumber,
    const std::string & xmlFile) const
{
    return std::make_shared<CTFReaderLogParamsElt>(name, pParent, xmlLineNumber, xmlFile);
}

void CTFReaderLogElt_2_0::end()
{
    CTFReaderOpElt::end();

    const auto dir = LogUtil::GetLogDirection(getCTFParams().m_style);
    getLog()->setDirection(dir);

    if (getCTFParams().getType() == LogUtil::CTFParams::CINEON)
    {
        double base = 2.0;
        LogOpData::Params rParams, gParams, bParams;

        try
        {
            // This handles all log styles.
            LogUtil::ConvertLogParameters(getCTFParams(), base, rParams, gParams, bParams);
        }
        catch (Exception& ce)
        {
            ThrowM(*this, "Parameters are not valid: '", ce.what(), "'.");
        }

        getLog()->setBase(base);
        getLog()->setRedParams(rParams);
        getLog()->setGreenParams(gParams);
        getLog()->setBlueParams(bParams);
    }
    else
    {
        if (!isBaseSet())
        {
            if (getCTFParams().m_style == LogUtil::LOG2 
                || getCTFParams().m_style == LogUtil::ANTI_LOG2)
            {
                setBase(2.0);
            }
            else if (getCTFParams().m_style == LogUtil::LOG10
                     || getCTFParams().m_style == LogUtil::ANTI_LOG10)
            {
                setBase(10.0);
            }
        }
    }

    // Validate the end result.
    try
    {
        getLog()->validate();
    }
    catch (Exception& ce)
    {
        ThrowM(*this, "Log is not valid: '", ce.what(), "'.");
    }
}

CTFReaderLogParamsEltRcPtr CTFReaderLogElt_2_0::createLogParamsElt(
    const std::string & name,
    ContainerEltRcPtr pParent,
    unsigned int xmlLineNumber,
    const std::string & xmlFile) const
{
    return std::make_shared<CTFReaderLogParamsElt_2_0>(name, pParent, xmlLineNumber, xmlFile);
}

CTFReaderLogParamsElt::CTFReaderLogParamsElt(const std::string & name,
                                             ContainerEltRcPtr pParent,
                                             unsigned int xmlLineNumber,
                                             const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderLogParamsElt::~CTFReaderLogParamsElt()
{
}

bool CTFReaderLogParamsElt::parseCineon(const char ** atts, unsigned i,
                                        double & gamma, double & refWhite,
                                        double & refBlack, double & highlight, double & shadow)
{
    if (0 == Platform::Strcasecmp(ATTR_GAMMA, atts[i]))
    {
        parseScalarAttribute(atts[i], atts[i + 1], gamma);
        return true;
    }
    else if (0 == Platform::Strcasecmp(ATTR_REFWHITE, atts[i]))
    {
        parseScalarAttribute(atts[i], atts[i + 1], refWhite);
        return true;
    }
    else if (0 == Platform::Strcasecmp(ATTR_REFBLACK, atts[i]))
    {
        parseScalarAttribute(atts[i], atts[i + 1], refBlack);
        return true;
    }
    else if (0 == Platform::Strcasecmp(ATTR_HIGHLIGHT, atts[i]))
    {
        parseScalarAttribute(atts[i], atts[i + 1], highlight);
        return true;
    }
    else if (0 == Platform::Strcasecmp(ATTR_SHADOW, atts[i]))
    {
        parseScalarAttribute(atts[i], atts[i + 1], shadow);
        return true;
    }
    return false;
}

void CTFReaderLogParamsElt::setCineon(LogUtil::CTFParams & legacyParams, int chan,
                                      double gamma, double refWhite,
                                      double refBlack, double highlight, double shadow)
{
    // Validate the attributes are appropriate for the log style and set
    // the parameters (numeric validation is done by LogOpData::validate).

    // Legacy parameters are set on the CTFReaderLogElt and will be
    // transferred to the op on the end() call.
    LogUtil::CTFParams::Params ctfValues(5);

    if (IsNan(gamma))
    {
        ThrowM(*this, "Required attribute '", ATTR_GAMMA, "' is missing.");
    }
    ctfValues[LogUtil::CTFParams::gamma] = gamma;

    if (IsNan(refWhite))
    {
        ThrowM(*this, "Required attribute '", ATTR_REFWHITE, "' is missing.");
    }
    ctfValues[LogUtil::CTFParams::refWhite] = refWhite;

    if (IsNan(refBlack))
    {
        ThrowM(*this, "Required attribute '", ATTR_REFBLACK, "' is missing.");
    }
    ctfValues[LogUtil::CTFParams::refBlack] = refBlack;

    if (IsNan(highlight))
    {
        ThrowM(*this, "Required attribute '", ATTR_HIGHLIGHT, "' is missing.");
    }
    ctfValues[LogUtil::CTFParams::highlight] = highlight;

    if (IsNan(shadow))
    {
        ThrowM(*this, "Required attribute '", ATTR_SHADOW, "' is missing.");
    }
    ctfValues[LogUtil::CTFParams::shadow] = shadow;

    // Assign the parameters to the object.

    switch (chan)
    {
    case -1:
        legacyParams.m_params[LogUtil::CTFParams::red  ] = ctfValues;
        legacyParams.m_params[LogUtil::CTFParams::green] = ctfValues;
        legacyParams.m_params[LogUtil::CTFParams::blue ] = ctfValues;
        break;
    case 0:
        legacyParams.m_params[LogUtil::CTFParams::red  ] = ctfValues;
        break;
    case 1:
        legacyParams.m_params[LogUtil::CTFParams::green] = ctfValues;
        break;
    case 2:
        legacyParams.m_params[LogUtil::CTFParams::blue ] = ctfValues;
        break;
    }
}

void CTFReaderLogParamsElt::start(const char ** atts)
{
    CTFReaderLogElt * pLogElt = dynamic_cast<CTFReaderLogElt*>(getParent().get());

    LogUtil::CTFParams & legacyParams = pLogElt->getCTFParams();

    // Attributes we want to extract.

    int chan = -1;

    // Legacy Log/Lin parameters:
    double gamma     = std::numeric_limits<double>::quiet_NaN();
    double refWhite  = std::numeric_limits<double>::quiet_NaN();
    double refBlack  = std::numeric_limits<double>::quiet_NaN();
    double highlight = std::numeric_limits<double>::quiet_NaN();
    double shadow    = std::numeric_limits<double>::quiet_NaN();

    // Try extracting the attributes.
    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_CHAN, atts[i]))
        {
            if (0 == Platform::Strcasecmp("R", atts[i + 1]))
            {
                chan = 0;
            }
            else if (0 == Platform::Strcasecmp("G", atts[i + 1]))
            {
                chan = 1;
            }
            else if (0 == Platform::Strcasecmp("B", atts[i + 1]))
            {
                chan = 2;
            }
            // Chan is optional but, if present, must be legal.
            else
            {
                std::ostringstream arg;
                arg << "Illegal channel attribute value '";
                arg << atts[i + 1] << "'.";

                throwMessage(arg.str());
            }
        }
        else if (!parseCineon(atts, i, gamma, refWhite, refBlack, highlight, shadow))
        {
            logParameterWarning(atts[i]);
        }

        i += 2;
    }

    setCineon(legacyParams, chan, gamma, refWhite, refBlack, highlight, shadow);
}

void CTFReaderLogParamsElt::end()
{
}

void CTFReaderLogParamsElt::setRawData(const char *, size_t, unsigned int)
{
}

void CTFReaderLogParamsElt_2_0::start(const char ** atts)
{
    CTFReaderLogElt * pLogElt = dynamic_cast<CTFReaderLogElt*>(getParent().get());
    LogUtil::CTFParams & legacyParams = pLogElt->getCTFParams();
    const LogUtil::LogStyle style = legacyParams.m_style;

    const bool cameraStyle = style == LogUtil::CAMERA_LIN_TO_LOG ||
                             style == LogUtil::CAMERA_LOG_TO_LIN;

    bool allowCineon = !cameraStyle && !pLogElt->getTransform()->isCLF();

    // Attributes we want to extract.

    int chan = -1;

    // Log/Lin parameters (CTF v2, CLF v3 and onward):
    double linSideSlope  = std::numeric_limits<double>::quiet_NaN();
    double linSideOffset = std::numeric_limits<double>::quiet_NaN();
    double logSideSlope  = std::numeric_limits<double>::quiet_NaN();
    double logSideOffset = std::numeric_limits<double>::quiet_NaN();
    double base          = std::numeric_limits<double>::quiet_NaN();
    double linSideBreak  = std::numeric_limits<double>::quiet_NaN();
    double linearSlope   = std::numeric_limits<double>::quiet_NaN();

    // Legacy Log/Lin parameters (if allowed for CTF v2 onward:
    double gamma     = std::numeric_limits<double>::quiet_NaN();
    double refWhite  = std::numeric_limits<double>::quiet_NaN();
    double refBlack  = std::numeric_limits<double>::quiet_NaN();
    double highlight = std::numeric_limits<double>::quiet_NaN();
    double shadow    = std::numeric_limits<double>::quiet_NaN();

    // Try extracting the attributes.
    unsigned i = 0;
    bool validType = true;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_CHAN, atts[i]))
        {
            if (0 == Platform::Strcasecmp("R", atts[i + 1]))
            {
                chan = 0;
            }
            else if (0 == Platform::Strcasecmp("G", atts[i + 1]))
            {
                chan = 1;
            }
            else if (0 == Platform::Strcasecmp("B", atts[i + 1]))
            {
                chan = 2;
            }
            // Chan is optional but, if present, must be legal.
            else
            {
                std::ostringstream arg;
                arg << "Illegal channel attribute value '";
                arg << atts[i + 1] << "'.";

                throwMessage(arg.str());
            }
        }
        else if (0 == Platform::Strcasecmp(ATTR_LINSIDESLOPE, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], linSideSlope);
            validType = legacyParams.setType(LogUtil::CTFParams::CLF);
        }
        else if (0 == Platform::Strcasecmp(ATTR_LINSIDEOFFSET, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], linSideOffset);
            validType = legacyParams.setType(LogUtil::CTFParams::CLF);
        }
        else if (0 == Platform::Strcasecmp(ATTR_LOGSIDESLOPE, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], logSideSlope);
            validType = legacyParams.setType(LogUtil::CTFParams::CLF);
        }
        else if (0 == Platform::Strcasecmp(ATTR_LOGSIDEOFFSET, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], logSideOffset);
            validType = legacyParams.setType(LogUtil::CTFParams::CLF);
        }
        else if (0 == Platform::Strcasecmp(ATTR_BASE, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], base);
            validType = legacyParams.setType(LogUtil::CTFParams::CLF);
        }
        else if (0 == Platform::Strcasecmp(ATTR_LINEARSLOPE, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], linearSlope);
            validType = legacyParams.setType(LogUtil::CTFParams::CLF);
        }
        else if (0 == Platform::Strcasecmp(ATTR_LINSIDEBREAK, atts[i]))
        {
            parseScalarAttribute(atts[i], atts[i + 1], linSideBreak);
            validType = legacyParams.setType(LogUtil::CTFParams::CLF);
        }
        else if (allowCineon && parseCineon(atts, i, gamma, refWhite, refBlack, highlight, shadow))
        {
            validType = legacyParams.setType(LogUtil::CTFParams::CINEON);
        }
        else
        {
            logParameterWarning(atts[i]);
        }

        if (!validType)
        {
            ThrowM(*this, "CLF type and Cineon types parameters can not be mixed.");
        }

        i += 2;
    }

    if (legacyParams.getType() == LogUtil::CTFParams::CINEON)
    {
        setCineon(legacyParams, chan, gamma, refWhite, refBlack, highlight, shadow);
        return;
    }

    // Validate the attributes are appropriate for the log style and set
    // the parameters (numeric validation is done by LogOpData::validate).

    // New parameters are set directly on the op.
    LogOpData::Params newParams(4);

    newParams[LIN_SIDE_SLOPE ] = IsNan(linSideSlope)  ? 1.0 : linSideSlope;
    newParams[LIN_SIDE_OFFSET] = IsNan(linSideOffset) ? 0.0 : linSideOffset;
    newParams[LOG_SIDE_SLOPE ] = IsNan(logSideSlope)  ? 1.0 : logSideSlope;
    newParams[LOG_SIDE_OFFSET] = IsNan(logSideOffset) ? 0.0 : logSideOffset;

    if (!IsNan(base))
    {
        pLogElt->setBase(base);
    }

    // The LogOpData does not have a style member, so some validation needs to happen here.

    if (!IsNan(linSideBreak))
    {
        if (!cameraStyle)
        {
            ThrowM(*this, "Parameter '", ATTR_LINSIDEBREAK, "' is only allowed for style '",
                   LogUtil::ConvertStyleToString(LogUtil::CAMERA_LOG_TO_LIN), "' or '",
                   LogUtil::ConvertStyleToString(LogUtil::CAMERA_LIN_TO_LOG), "'.");
        }
        newParams.push_back(linSideBreak);
    }
    else if (cameraStyle)
    {
        ThrowM(*this, "Parameter '", ATTR_LINSIDEBREAK, "' should be defined for style '",
               LogUtil::ConvertStyleToString(LogUtil::CAMERA_LOG_TO_LIN), "' or '",
               LogUtil::ConvertStyleToString(LogUtil::CAMERA_LIN_TO_LOG), "'. ");
    }

    if (!IsNan(linearSlope))
    {
        if (!cameraStyle)
        {
            ThrowM(*this, "Parameter '", ATTR_LINEARSLOPE, "' is only allowed for style '",
                   LogUtil::ConvertStyleToString(LogUtil::CAMERA_LOG_TO_LIN), "' or '",
                   LogUtil::ConvertStyleToString(LogUtil::CAMERA_LIN_TO_LOG), "'. ");
        }
        newParams.push_back(linearSlope);
    }

    auto logOp = OCIO_DYNAMIC_POINTER_CAST<LogOpData>(pLogElt->getOp());

    switch (chan)
    {
    case -1:
        logOp->setRedParams(newParams);
        logOp->setGreenParams(newParams);
        logOp->setBlueParams(newParams);
        break;
    case 0:
        logOp->setRedParams(newParams);
        break;
    case 1:
        logOp->setGreenParams(newParams);
        break;
    case 2:
        logOp->setBlueParams(newParams);
        break;
    }
}

//////////////////////////////////////////////////////////

CTFReaderLut1DElt::CTFReaderLut1DElt()
    : CTFReaderOpElt()
    , CTFArrayMgt()
    , CTFIndexMapMgt()
    , m_lut(std::make_shared<Lut1DOpData>(2))
    , m_indexMapping(0)
{
}

CTFReaderLut1DElt::~CTFReaderLut1DElt()
{
}

void CTFReaderLut1DElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    // The interpolation attribute is optional in CLF/CTF.  The INTERP_DEFAULT
    // enum indicates that the value was not specified in the file.  When 
    // writing, this means no interpolation attribute will be added.
    m_lut->setInterpolation(INTERP_DEFAULT);

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_INTERPOLATION, atts[i]))
        {
            try
            {
                Interpolation interp = GetInterpolation1D(atts[i + 1]);
                m_lut->setInterpolation(interp);
            }
            catch (const std::exception& e)
            {
                throwMessage(e.what());
            }
        }

        if (0 == Platform::Strcasecmp(ATTR_HALF_DOMAIN, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                ThrowM(*this, "Illegal 'halfDomain' attribute '", atts[i + 1],
                       "' while parsing Lut1D.");
            }

            m_lut->setInputHalfDomain(true);
        }

        if (0 == Platform::Strcasecmp(ATTR_RAW_HALFS, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                ThrowM(*this, "Illegal 'rawHalfs' attribute '", atts[i + 1],
                       "' while parsing Lut1D.");
            }

            m_lut->setOutputRawHalfs(true);
        }

        i += 2;
    }
}

bool CTFReaderLut1DElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_INTERPOLATION, att) ||
           0 == Platform::Strcasecmp(ATTR_HALF_DOMAIN, att) ||
           0 == Platform::Strcasecmp(ATTR_RAW_HALFS, att);
}

void CTFReaderLut1DElt::end()
{
    CTFReaderOpElt::end();

    const float scale = 1.0f / (float)GetBitDepthMaxValue(m_outBitDepth);
    m_lut->scale(scale);

    // Record the original array scaling present in the file.  This is used by
    // a heuristic involved with LUT inversion.  The bit-depth of ops is
    // typically changed after the file is read, hence the need to store it now.
    m_lut->setFileOutputBitDepth(m_outBitDepth);
    m_lut->validate();
}

const OpDataRcPtr CTFReaderLut1DElt::getOp() const
{
    return m_lut;
}

ArrayBase * CTFReaderLut1DElt::updateDimension(const Dimensions & dims)
{
    if (dims.size() != 2)
    {
        return nullptr;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size() - 1));
    const unsigned numColorComponents = dims[max];

    if (dims[1] != 3 && dims[1] != 1)
    {
        return nullptr;
    }

    Array * pArray = &m_lut->getArray();
    pArray->resize(dims[0], numColorComponents);
    return pArray;
}

void CTFReaderLut1DElt::endArray(unsigned int position)
{
    Array * pArray = &m_lut->getArray();

    // Convert half bits to float values if needed.
    if (m_lut->isOutputRawHalfs())
    {
        const size_t maxValues = pArray->getNumValues();
        for (size_t i = 0; i<maxValues; ++i)
        {
            pArray->getValues()[i]
                = ConvertHalfBitsToFloat((unsigned short)pArray->getValues()[i]);
        }
    }

    if (pArray->getNumValues() != position)
    {
        const unsigned numColorComponents = pArray->getNumColorComponents();
        const unsigned maxColorComponents = 3;

        const unsigned dimensions = pArray->getLength();

        if (numColorComponents != 1 || position != dimensions)
        {
            ThrowM(*this, "Expected ", dimensions, "x", numColorComponents,
                   " Array values, found ", position, ".");
        }

        // Convert a 1D LUT to a 3by1D LUT
        // (duplicate values from the Red to the Green and Blue).
        const unsigned numLuts = maxColorComponents;

        // TODO: Should improve Lut1DOp so that the copy is unnecessary.
        for (signed i = (dimensions - 1); i >= 0; --i)
        {
            for (unsigned j = 0; j<numLuts; ++j)
            {
                pArray->getValues()[(i*numLuts) + j] = pArray->getValues()[i];
            }
        }
    }

    pArray->validate();

    setCompleted(true);
}

IndexMapping * CTFReaderLut1DElt::updateDimensionIM(const DimensionsIM & dims)
{
    if (dims.size() != 1)
    {
        return nullptr;
    }

    const unsigned int numComponents = dims[0];

    if (dims[0] == 0)
    {
        return nullptr;
    }

    m_indexMapping.resize(numComponents);
    return &m_indexMapping;
}

void CTFReaderLut1DElt::endIndexMap(unsigned int position)
{
    if (m_indexMapping.getDimension() != position)
    {
        ThrowM(*this, "Expected ", m_indexMapping.getDimension(),
               " IndexMap values, found ", position, ".");
    }

    m_indexMapping.validate();
    setCompletedIM(true);
}

//////////////////////////////////////////////////////////

void CTFReaderLut1DElt_1_4::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    // The interpolation attribute is optional in CLF/CTF.  The INTERP_DEFAULT
    // enum indicates that the value was not specified in the file.  When 
    // writing, this means no interpolation attribute will be added.
    m_lut->setInterpolation(INTERP_DEFAULT);

    unsigned int i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_INTERPOLATION, atts[i]))
        {
            try
            {
                Interpolation interp = GetInterpolation1D(atts[i + 1]);
                m_lut->setInterpolation(interp);
            }
            catch (const std::exception& e)
            {
                throwMessage(e.what());
            }
        }

        if (0 == Platform::Strcasecmp(ATTR_HALF_DOMAIN, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                ThrowM(*this, "Illegal 'halfDomain' attribute '", atts[i + 1],
                       "' while parsing Lut1D.");
            }

            m_lut->setInputHalfDomain(true);
        }

        if (0 == Platform::Strcasecmp(ATTR_RAW_HALFS, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                ThrowM(*this, "Illegal 'rawHalfs' attribute '", atts[i + 1],
                       "' while parsing Lut1D.");
            }

            m_lut->setOutputRawHalfs(true);
        }

        // This was added in v1.4.
        if (0 == Platform::Strcasecmp(ATTR_HUE_ADJUST, atts[i]))
        {
            if (0 != Platform::Strcasecmp("dw3", atts[i + 1]))
            {
                ThrowM(*this, "Illegal 'hueAdjust' attribute '", atts[i + 1],
                       "' while parsing Lut1D.");
            }

            m_lut->setHueAdjust(HUE_DW3);
        }

        i += 2;
    }

}

bool CTFReaderLut1DElt_1_4::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_INTERPOLATION, att) ||
           0 == Platform::Strcasecmp(ATTR_HALF_DOMAIN, att) ||
           0 == Platform::Strcasecmp(ATTR_RAW_HALFS, att) ||
           0 == Platform::Strcasecmp(ATTR_HUE_ADJUST, att);
}

//////////////////////////////////////////////////////////

void CTFReaderLut1DElt_1_7::end()
{
    CTFReaderOpElt::end();

    const float scale = 1.0f / (float)GetBitDepthMaxValue(m_outBitDepth);
    m_lut->scale(scale);

    m_lut->setFileOutputBitDepth(m_outBitDepth);
    m_lut->validate();

    // The LUT renderers do not currently support an indexMap, however for
    // compliance with the CLF spec we want to support the case of a single 2-entry
    // indexMap by converting it into a RangeOp which we insert before the LUT.
    if (isCompletedIM())
    {
        // This will throw if the indexMap is not length 2.
        RangeOpDataRcPtr pRng = std::make_shared<RangeOpData>(
            m_indexMapping,
            m_lut->getArray().getLength(),
            m_inBitDepth);

        // Insert the range before the LUT that was appended to m_transform
        // in OpElt::start.
        // This code assumes that the current LUT is at the end of the opList.
        // In other words, that this LUT's end() method will be called before
        // any other Op's start().
        const size_t len = m_transform->getOps().size();
        const size_t pos = len - 1;
        m_transform->getOps().insert(m_transform->getOps().begin() + pos, pRng);
    }
}

//////////////////////////////////////////////////////////

CTFReaderLut3DElt::CTFReaderLut3DElt()
    : CTFReaderOpElt()
    , CTFArrayMgt()
    , CTFIndexMapMgt()
    , m_lut(std::make_shared<Lut3DOpData>(2))
    , m_indexMapping(0)
{
}

CTFReaderLut3DElt::~CTFReaderLut3DElt()
{
}

void CTFReaderLut3DElt::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    // The interpolation attribute is optional in CLF/CTF.  The INTERP_DEFAULT
    // enum indicates that the value was not specified in the file.  When 
    // writing, this means no interpolation attribute will be added.
    m_lut->setInterpolation(INTERP_DEFAULT);

    unsigned i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_INTERPOLATION, atts[i]))
        {
            try
            {
                Interpolation interp = GetInterpolation3D(atts[i + 1]);
                m_lut->setInterpolation(interp);
            }
            catch (const std::exception& e)
            {
                throwMessage(e.what());
            }
        }

        i += 2;
    }
}

bool CTFReaderLut3DElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_INTERPOLATION, att);
}

void CTFReaderLut3DElt::end()
{
    CTFReaderOpElt::end();

    const float scale = 1.0f / (float)GetBitDepthMaxValue(m_outBitDepth);
    m_lut->scale(scale);

    m_lut->setFileOutputBitDepth(m_outBitDepth);
    m_lut->validate();
}

const OpDataRcPtr CTFReaderLut3DElt::getOp() const
{
    return m_lut;
}

ArrayBase * CTFReaderLut3DElt::updateDimension(const Dimensions & dims)
{
    if (dims.size() != 4)
    {
        return nullptr;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size() - 1));
    const unsigned numColorComponents = dims[max];

    if (dims[3] != 3 || dims[1] != dims[0] || dims[2] != dims[0])
    {
        return nullptr;
    }

    Array* pArray = &m_lut->getArray();
    pArray->resize(dims[0], numColorComponents);

    return pArray;
}

void CTFReaderLut3DElt::endArray(unsigned int position)
{
    // NB: A CLF/CTF Lut3D Array stores the elements in blue-fastest order.
    Array* pArray = &m_lut->getArray();
    if (pArray->getNumValues() != position)
    {
        ThrowM(*this, "Expected ", pArray->getLength(), "x", pArray->getLength(),
               "x", pArray->getLength(), "x", pArray->getNumColorComponents(),
               " Array values, found ", position, ".");
    }

    pArray->validate();
    setCompleted(true);
}

IndexMapping * CTFReaderLut3DElt::updateDimensionIM(const DimensionsIM & dims)
{
    if (dims.size() != 1)
    {
        return nullptr;
    }

    const unsigned numComponents = dims[0];

    if (dims[0] == 0)
    {
        return nullptr;
    }

    m_indexMapping.resize(numComponents);
    return &m_indexMapping;
}

void CTFReaderLut3DElt::endIndexMap(unsigned int position)
{
    if (m_indexMapping.getDimension() != position)
    {
        ThrowM(*this, "Expected ", m_indexMapping.getDimension(),
               " IndexMap values, found ", position, ".");
    }

    m_indexMapping.validate();
    setCompletedIM(true);
}

//////////////////////////////////////////////////////////

// TODO: Factor duplicate code from Lut1D version.
void CTFReaderLut3DElt_1_7::end()
{
    CTFReaderOpElt::end();

    const float scale = 1.0f / (float)GetBitDepthMaxValue(m_outBitDepth);
    m_lut->scale(scale);

    m_lut->setFileOutputBitDepth(m_outBitDepth);
    m_lut->validate();

    // The LUT renderers do not currently support an indexMap, however for
    // compliance with the CLF spec we want to support the case of a single 2-entry
    // indexMap by converting it into a RangeOp which we insert before the LUT.
    if (isCompletedIM())
    {
        // This will throw if the indexMap is not length 2.
        RangeOpDataRcPtr pRng = std::make_shared<RangeOpData>(
            m_indexMapping,
            m_lut->getArray().getLength(),
            m_inBitDepth);

        // Insert the range before the LUT that was appended to m_transform
        // in OpElt::start.
        // This code assumes that the current LUT is at the end of the opList.
        // In other words, that this LUT's end() method will be called before
        // any other Op's start().
        const unsigned long len = (unsigned long)m_transform->getOps().size();
        const unsigned long pos = len - 1;
        m_transform->getOps().insert(m_transform->getOps().begin() + pos, pRng);
    }
}

//////////////////////////////////////////////////////////

CTFReaderMatrixElt::CTFReaderMatrixElt()
    : CTFReaderOpElt()
    , CTFArrayMgt()
    , m_matrix(std::make_shared<MatrixOpData>())
{
}

CTFReaderMatrixElt::~CTFReaderMatrixElt()
{
}

void CTFReaderMatrixElt::end()
{
    CTFReaderOpElt::end();

    m_matrix->scale(GetBitDepthMaxValue(m_inBitDepth), 1.0 / GetBitDepthMaxValue(m_outBitDepth));

    m_matrix->setFileInputBitDepth(m_inBitDepth);
    m_matrix->setFileOutputBitDepth(m_outBitDepth);

    // Validate the end result.
    m_matrix->validate();
}

const OpDataRcPtr CTFReaderMatrixElt::getOp() const
{
    return m_matrix;
}

ArrayBase * CTFReaderMatrixElt::updateDimension(const Dimensions & dims)
{
    // Only 3 3 3 and 4 4 3 are handled here (ctf version 1.2 or less).
    // Other formats are covered by CTFReaderMatrixElt_1_3::updateDimension.
    if (dims.size() != 3)
    {
        return nullptr;
    }

    const unsigned int numColorComponents = dims[2];
    const unsigned int size = dims[0];

    if (size != dims[1] || numColorComponents != 3)
    {
        return nullptr;
    }

    ArrayDouble * pArray = &m_matrix->getArray();
    pArray->resize(size, numColorComponents);

    return pArray;
}

void CTFReaderMatrixElt::endArray(unsigned int position)
{
    ArrayDouble & array = m_matrix->getArray();
    if (array.getNumValues() != position)
    {
        std::ostringstream arg;
        arg << "Expected " << array.getLength() << "x" << array.getLength();
        arg << " Array values, found " << position;

        throw Exception(arg.str().c_str());
    }

    // Array parsing is done.
    setCompleted(true);

    convert_1_2_to_Latest();
}

void CTFReaderMatrixElt::convert_1_2_to_Latest()
{
    if (CTF_PROCESS_LIST_VERSION_1_2 < CTF_PROCESS_LIST_VERSION)
    {
        ArrayDouble & array = m_matrix->getArray();

        if (array.getLength() == 3)
        {
            const double offsets[4] = { 0.0, 0.0, 0.0, 0.0 };
            m_matrix->setRGBAOffsets(offsets);
        }
        else if (array.getLength() == 4)
        {
            array = m_matrix->getArray();
            ArrayDouble::Values oldV = array.getValues();

            // Extract offsets.
            m_matrix->setOffsetValue(0, oldV[3]);
            m_matrix->setOffsetValue(1, oldV[7]);
            m_matrix->setOffsetValue(2, oldV[11]);
            m_matrix->setOffsetValue(3, 0.0);

            array.resize(3, 3);

            ArrayDouble::Values & v = array.getValues();
            v[0] = oldV[0];
            v[1] = oldV[1];
            v[2] = oldV[2];

            v[3] = oldV[4];
            v[4] = oldV[5];
            v[5] = oldV[6];

            v[6] = oldV[8];
            v[7] = oldV[9];
            v[8] = oldV[10];
        }
        else
        {
            std::ostringstream arg;
            arg << "MatrixElt: Expecting array dimension to be 3 or 4. Got: ";
            arg << array.getLength() << ".";

            throw Exception(arg.str().c_str());
        }
    }
}

ArrayBase * CTFReaderMatrixElt_1_3::updateDimension(const Dimensions & dims)
{
    // In CLF v3, the committee dropped the third dimension value since with all currently
    // supported cases it is the same as the first value.  There will probably be a lot of
    // files that use the old syntax with the new version number or vice versa, so we try
    // to be forgiving and handle either two or three values (for both CLF and CTF).
    // We decided to allow this even for old files back to CTF 1.3.
    const size_t size = dims.size();
    if (size != 3 && size != 2)
    {
        return nullptr;
    }

    if (!((dims[0] == 3 && dims[1] == 3) ||
          (dims[0] == 3 && dims[1] == 4) ||
          (dims[0] == 4 && dims[1] == 4) ||
          (dims[0] == 4 && dims[1] == 5)))
    {
        return nullptr;
    }

    if (size == 3)
    {
        if (dims[0] != dims[2])
        {
            return nullptr;
        }
    }

    if (IsDebugLoggingEnabled())
    {
        if (m_transform->getCTFVersion() < CTF_PROCESS_LIST_VERSION_2_0)
        {
            if (size == 2)
            {
                std::ostringstream oss;
                oss << getXmlFile().c_str() << "(" << getXmlLineNumber() << "): ";
                oss << "Matrix array dimension should have 3 numbers "
                       "for CTF before version 2.";
                LogDebug(oss.str());
            }
        }
        else
        {
            if (size == 3)
            {
                std::ostringstream oss;
                oss << getXmlFile().c_str() << "(" << getXmlLineNumber() << "): ";
                oss << "Matrix array dimension should have 2 numbers "
                       "for CTF from version 2.";
                LogDebug(oss.str());
            }
        }
    }

    ArrayDouble * pArray = &getMatrix()->getArray();
    pArray->resize(dims[1], dims[0]);

    return pArray;
}

void CTFReaderMatrixElt_1_3::endArray(unsigned int position)
{
    ArrayDouble & array = getMatrix()->getArray();

    const ArrayDouble::Values & values = array.getValues();

    // Note: Version 1.3 of Matrix Op supports 4 xml formats:
    //       1) 4x5x4, matrix with alpha and offsets
    //       2) 4x4x4, matrix with alpha and no offsets
    //       3) 3x4x3, matrix only with offsets and no alpha
    //       4) 3x3x3, matrix with no alpha and no offsets
    // Note: As of CLF v3 we drop the third digit, since it is redundant.

    if (array.getLength() == 3 && array.getNumColorComponents() == 3)
    {
        if (position != 9)
        {
            ThrowM(*this, "Expected 3x3 Array values, found ", position, ".");
        }
    }
    else if (array.getLength() == 4)
    {
        if (array.getNumColorComponents() == 3)
        {
            if (position != 12)
            {
                ThrowM(*this, "Expected 3x4 Array values, found ", position, ".");
            }

            MatrixOpDataRcPtr & pMatrix = getMatrix();

            pMatrix->setOffsetValue(0, values[3]);
            pMatrix->setOffsetValue(1, values[7]);
            pMatrix->setOffsetValue(2, values[11]);
            pMatrix->setOffsetValue(3, 0.0);

            ArrayDouble::Values oldV = array.getValues();

            array.setLength(3);

            ArrayDouble::Values & v = array.getValues();
            v[0] = oldV[0];
            v[1] = oldV[1];
            v[2] = oldV[2];

            v[3] = oldV[4];
            v[4] = oldV[5];
            v[5] = oldV[6];

            v[6] = oldV[8];
            v[7] = oldV[9];
            v[8] = oldV[10];
        }
        else
        {
            if (position != 16)
            {
                ThrowM(*this, "Expected 4x4 Array values, found ", position, ".");
            }

            MatrixOpDataRcPtr & pMatrix = getMatrix();

            const double offsets[4] = { 0., 0., 0., 0. };
            pMatrix->setRGBAOffsets(offsets);
        }
    }
    else
    {
        if (position != 20)
        {
            ThrowM(*this, "Expected 4x5 Array values, found ", position, ".");
        }

        MatrixOpDataRcPtr & pMatrix = getMatrix();

        pMatrix->setOffsetValue(0, values[4]);
        pMatrix->setOffsetValue(1, values[9]);
        pMatrix->setOffsetValue(2, values[14]);
        pMatrix->setOffsetValue(3, values[19]);

        ArrayDouble::Values oldV = array.getValues();

        array.resize(4, 4);

        ArrayDouble::Values & v = array.getValues();
        v[0] = oldV[0];
        v[1] = oldV[1];
        v[2] = oldV[2];
        v[3] = oldV[3];

        v[4] = oldV[5];
        v[5] = oldV[6];
        v[6] = oldV[7];
        v[7] = oldV[8];

        v[8] = oldV[10];
        v[9] = oldV[11];
        v[10] = oldV[12];
        v[11] = oldV[13];

        v[12] = oldV[15];
        v[13] = oldV[16];
        v[14] = oldV[17];
        v[15] = oldV[18];
    }

    // Array parsing is done.

    setCompleted(true);
}

//////////////////////////////////////////////////////////

CTFReaderRangeElt::CTFReaderRangeElt()
    : CTFReaderOpElt()
    , m_range(std::make_shared<RangeOpData>())
{
}

CTFReaderRangeElt::~CTFReaderRangeElt()
{
}

void CTFReaderRangeElt::end()
{
    CTFReaderOpElt::end();

    m_range->setFileInputBitDepth(m_inBitDepth);
    m_range->setFileOutputBitDepth(m_outBitDepth);

    m_range->normalize();

    // Validate the end result.
    m_range->validate();
}

const OpDataRcPtr CTFReaderRangeElt::getOp() const
{
    return m_range;
}

//////////////////////////////////////////////////////////

CTFReaderRangeElt_1_7::CTFReaderRangeElt_1_7()
    : CTFReaderRangeElt()
{
}

CTFReaderRangeElt_1_7::~CTFReaderRangeElt_1_7()
{
}

void CTFReaderRangeElt_1_7::start(const char ** atts)
{
    CTFReaderRangeElt::start(atts);

    m_isNoClamp = false;

    unsigned i = 0;
    while (atts[i])
    {
        // TODO: Refactor to use a base class attribute checking function
        //       instead of looping twice around the attribute list.
        if (0 == Platform::Strcasecmp(ATTR_STYLE, atts[i]))
        {
            m_isNoClamp = (0 == Platform::Strcasecmp("noClamp", atts[i + 1]));
        }

        i += 2;
    }
}

bool CTFReaderRangeElt_1_7::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_STYLE, att);
}

void CTFReaderRangeElt_1_7::end()
{
    CTFReaderRangeElt::end();

    // Adding support for the noClamp style introduced in the CLF spec.
    // We convert our RangeOp (which always clamps) into an equivalent MatrixOp.
    if (m_isNoClamp)
    {
        ConstOpDataRcPtr pMtx = m_range->convertToMatrix();

        // This code assumes that the current Range is at the end of the opList.
        // In other words, that this Op's end() method will be called before
        // any other Op's start().
        const size_t len = m_transform->getOps().size();
        const size_t pos = len - 1;

        // Replace the range appended to m_transform in OpElt::start
        // with the matrix.
        m_transform->getOps()[pos].swap(pMtx);
    }
}

//////////////////////////////////////////////////////////

CTFReaderRangeValueElt::CTFReaderRangeValueElt(const std::string & name,
                                               ContainerEltRcPtr pParent,
                                               unsigned int xmlLineNumber,
                                               const std::string & xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

CTFReaderRangeValueElt::~CTFReaderRangeValueElt()
{
}

void CTFReaderRangeValueElt::start(const char ** atts)
{
    unsigned i = 0;
    while (atts[i])
    {
        logParameterWarning(atts[i]);
        i += 2;
    }
}

void CTFReaderRangeValueElt::end()
{
}

void CTFReaderRangeValueElt::setRawData(const char * s, size_t len, unsigned int)
{
    CTFReaderRangeElt * pRange = dynamic_cast<CTFReaderRangeElt*>(getParent().get());

    std::vector<double> data;

    try
    {
        data = GetNumbers<double>(s, len);
    }
    catch (Exception & ce)
    {
        ThrowM(*this, "Illegal '", getTypeName(), "' values ",
               TruncateString(s, len), " [", ce.what(), "]");
    }

    if (data.size() != 1)
    {
        throwMessage("Range element: non-single value.");
    }

    if (0 == Platform::Strcasecmp(getName().c_str(), TAG_MIN_IN_VALUE))
    {
        pRange->getRange()->setMinInValue(data[0]);
    }
    else if (0 == Platform::Strcasecmp(getName().c_str(), TAG_MAX_IN_VALUE))
    {
        pRange->getRange()->setMaxInValue(data[0]);
    }
    else if (0 == Platform::Strcasecmp(getName().c_str(), TAG_MIN_OUT_VALUE))
    {
        pRange->getRange()->setMinOutValue(data[0]);
    }
    else if (0 == Platform::Strcasecmp(getName().c_str(), TAG_MAX_OUT_VALUE))
    {
        pRange->getRange()->setMaxOutValue(data[0]);
    }
}

//////////////////////////////////////////////////////////

CTFReaderReferenceElt::CTFReaderReferenceElt()
    : CTFReaderOpElt()
    , m_reference(std::make_shared<ReferenceOpData>())
{
}

CTFReaderReferenceElt::~CTFReaderReferenceElt()
{
}

void CTFReaderReferenceElt::start(const char **atts)
{
    CTFReaderOpElt::start(atts);

    std::string alias;
    std::string path;
    bool basePathFound = false;

    unsigned long i = 0;
    while (atts[i])
    {
        if (0 == Platform::Strcasecmp(ATTR_PATH, atts[i]))
        {
            path = atts[i + 1];
        }
        else if (0 == Platform::Strcasecmp(ATTR_BASE_PATH, atts[i]))
        {
            // Ignore BasePath for now: BasePath could be used to point to
            // a specific folder, but for OCIO all folders have to be
            // reacheable through the Context.
            // All paths might be absolute or relative.
            basePathFound = true;
        }
        else if (0 == Platform::Strcasecmp(ATTR_ALIAS, atts[i]))
        {
            // Most alias operators may be ignored, with the exception of
            // currentMonitor.  Since we cannot apply that transform here,
            // we need to throw.
            alias = atts[i + 1];
            if (0 == Platform::Strcasecmp(alias.c_str(), "currentMonitor"))
            {
                throwMessage("The 'currentMonitor' alias is not supported.");
            }
        }
        else if (0 == Platform::Strcasecmp(ATTR_IS_INVERTED, atts[i]))
        {
            if (0 == Platform::Strcasecmp("true", atts[i + 1]))
            {
                getReference()->setDirection(TRANSFORM_DIR_INVERSE);
            }
        }

        i += 2;
    }
    if (!alias.empty())
    {
        if (!path.empty())
        {
            throwMessage("alias & path attributes for Reference should not be "
                         "both defined.");
        }

        if (basePathFound)
        {
            throwMessage("alias & basepath attributes for Reference should not be "
                         "both defined.");
        }

        m_reference->setAlias(alias);
    }
    else
    {
        if (path.empty())
        {
            throwMessage("path attribute for Reference is missing.");
        }

        m_reference->setPath(path);
    }
}

bool CTFReaderReferenceElt::isOpParameterValid(const char * att) const noexcept
{
    return CTFReaderOpElt::isOpParameterValid(att) ||
           0 == Platform::Strcasecmp(ATTR_PATH, att) ||
           0 == Platform::Strcasecmp(ATTR_BASE_PATH, att) ||
           0 == Platform::Strcasecmp(ATTR_ALIAS, att) ||
           0 == Platform::Strcasecmp(ATTR_IS_INVERTED, att);
}

void CTFReaderReferenceElt::end()
{
    CTFReaderOpElt::end();

    m_reference->validate();
}

const OpDataRcPtr CTFReaderReferenceElt::getOp() const
{
    return m_reference;
}

} // namespace OCIO_NAMESPACE

