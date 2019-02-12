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

#include "fileformats/ctf/CTFReaderHelper.h"
#include "fileformats/ctf/CTFReaderUtils.h"
#include "fileformats/xmlutils/XMLReaderUtils.h"
#include "Platform.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
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
                throwMessage("Required attribute 'id' does not have a value. ");
            }

            m_transform->setID(atts[i + 1]);
            isIdFound = true;
        }
        else if (0 == Platform::Strcasecmp(ATTR_NAME, atts[i]))
        {
            if (!atts[i + 1] || !*atts[i + 1])
            {
                throwMessage("If the attribute 'name' is present, it must have a value. ");
            }

            m_transform->setName(atts[i + 1]);
        }
        else if (0 == Platform::Strcasecmp(ATTR_INVERSE_OF, atts[i]))
        {
            if (!atts[i + 1] || !*atts[i + 1])
            {
                throwMessage("If the attribute 'inverseOf' is present, it must have a value. ");
            }

            m_transform->setInverseOfId(atts[i + 1]);
        }
        else if (0 == Platform::Strcasecmp(ATTR_VERSION, atts[i]))
        {
            if (isCLFVersionFound)
            {
                throwMessage("'compCLFversion' and 'Version' cannot both be present. ");
            }
            if (isVersionFound)
            {
                throwMessage("'Version' can only be there once. ");
            }

            const char* pVer = atts[i + 1];
            if (!pVer || !*pVer)
            {
                throwMessage("If the attribute 'version' is present, it must have a value. ");
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
                throwMessage("'compCLFversion' can only be there once. ");
            }
            if (isVersionFound)
            {
                throwMessage("'compCLFversion' and 'Version' cannot be both present. ");
            }

            const char* pVer = atts[i + 1];
            if (!pVer || !*pVer)
            {
                throwMessage("Required attribute 'compCLFversion' does not have a value. ");
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
            // We currently interpret CLF versions <= 2.0 as CTF version 1.7.
            CTFVersion maxCLF(2, 0);
            if (maxCLF < requestedCLFVersion)
            {
                ThrowM(*this, "Unsupported transform file version '", pVer,
                       "' supplied. ");
            }

            requestedVersion = CTF_PROCESS_LIST_VERSION_1_7;

            isVersionFound = true;
            isCLFVersionFound = true;
        }

        i += 2;
    }

    // Check mandatory elements.
    if (!isIdFound)
    {
        throwMessage("Required attribute 'id' is missing. ");
    }

    // Transform file format with no version means that
    // the CTF format is 1.2.
    if (!isVersionFound)
    {
        if (m_isCLF && !isCLFVersionFound)
        {
            throwMessage("Required attribute 'compCLFversion' is missing. ");
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
    try
    {
        m_transform->validate();
    }
    catch (Exception& e)
    {
        throwMessage(e.what());
    }
}

void CTFReaderTransformElt::appendDescription(const std::string & desc)
{
    getTransform()->getDescriptions() += desc;
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
        ThrowM(*this, "Unsupported transform file version '", ver, "' supplied. ");
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
                ThrowM(*this, "Illegal '", getTypeName(), "' dimensions ",
                       TruncateString(dimStr, len));
            }

            CTFArrayMgt* pArr = dynamic_cast<CTFArrayMgt*>(getParent().get());
            if (!pArr)
            {
                ThrowM(*this, "Parsing issue while parsing dimensions of '",
                       getTypeName(), "' (", TruncateString(dimStr, len), ").");
            }
            else
            {
                const unsigned max =
                    (const unsigned)(dims.empty() ? 0 : (dims.size() - 1));
                if (max == 0)
                {
                    ThrowM(*this, "Illegal '", getTypeName(), "' dimensions ",
                           TruncateString(dimStr, len));
                }

                m_array = pArr->updateDimension(dims);
                if (!m_array)
                {
                    ThrowM(*this, "'", getTypeName(), "' Illegal dimensions ",
                           TruncateString(dimStr, len));
                }
            }
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
                   "' in ", getTypeName());
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
                   " Array, found too many values in '", getTypeName(), "'.");
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
                       "' IndexMap dimensions ", TruncateString(dimStr, len));
            }

            CTFIndexMapMgt * pArr = dynamic_cast<CTFIndexMapMgt*>(getParent().get());
            if (!pArr)
            {
                ThrowM(*this, "Illegal '", getTypeName(),
                       "' IndexMap dimensions ", TruncateString(dimStr, len));
            }
            else
            {
                if (dims.empty() || dims.size() != 1)
                {
                    ThrowM(*this, "Illegal '", getTypeName(),
                           "' IndexMap dimensions ", TruncateString(dimStr, len));
                }

                m_indexMap = pArr->updateDimensionIM(dims);
                if (!m_indexMap)
                {
                    ThrowM(*this, "Illegal '", getTypeName(),
                           "' IndexMap dimensions ", TruncateString(dimStr, len));
                }
            }
        }

        i += 2;
    }

    // Check mandatory elements
    if (!isDimFound)
    {
        throwMessage("Required attribute 'dim' is missing. ");
    }

    m_position = 0;
}

void CTFReaderIndexMapElt::end()
{
    // A known element (e.g. an IndexMap) in a dummy element,
    // no need to validate it.
    if (getParent()->isDummy()) return;

    CTFIndexMapMgt * pIMM = dynamic_cast<CTFIndexMapMgt*>(getParent().get());
    pIMM->endIndexMap(m_position);
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
    // Set pos to how much leading white space there is.
    pos = FindNextTokenStart(s, len, pos);

    if (pos != len)
    {
        // Extract a number at pos.
        // Note, the len may here include the @ at the end of the string for ParseNumber
        // (e.g. "10.5@") but it does not cause the sscanf to fail.
        ParseNumber(s + pos, len - pos, num1);

        // Set pos to advance over the numbers we just parsed.
        // Note that we stop either at white space or an ampersand.
        pos = FindIndexDelim(s, len, pos);

        // Set pos to the start of the next number, advancing over white space or an @.
        pos = FindNextTokenStart_IndexMap(s, len, pos);

        // Extract the other half of the index pair.
        ParseNumber(s + pos, len - pos, num2);

        // Set pos to advance over the numbers we just parsed.
        pos = FindDelim(s, len, pos);

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
                   "' in '", getTypeName(), "' IndexMap");
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
    , m_metadata(name)
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
            m_metadata.addAttribute(Metadata::Attribute(atts[i], atts[i + 1]));
        }
        i += 2;
    }
}

void CTFReaderMetadataElt::end()
{
    CTFReaderMetadataElt* pMetadataElt = dynamic_cast<CTFReaderMetadataElt*>(getParent().get());
    if (pMetadataElt)
    {
        pMetadataElt->getMetadata()[getName()] = m_metadata;
    }
}

const std::string & CTFReaderMetadataElt::getIdentifier() const
{
    return getName();
}

void CTFReaderMetadataElt::setRawData(const char * str, size_t len, unsigned int)
{
    m_metadata = m_metadata.getValue() + std::string(str, len);
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

    // Let the base class store the attributes in the Metadata element.
    CTFReaderMetadataElt::start(atts);
}

void CTFReaderInfoElt::end()
{
    CTFReaderTransformElt* pTransformElt = dynamic_cast<CTFReaderTransformElt*>(getParent().get());
    if (pTransformElt)
    {
        pTransformElt->getTransform()->getInfo() = m_metadata;
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

void CTFReaderOpElt::appendDescription(const std::string & desc)
{
    getOp()->getDescriptions() += desc;
}

void CTFReaderOpElt::start(const char ** atts)
{
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
                ThrowM(*this, "inBitDepth unknown value (", atts[i + 1], ")");
            }
            getOp()->setInputBitDepth(bitdepth);
            bitDepthFound |= INPUT_BIT_DEPTH;
        }
        else if (0 == Platform::Strcasecmp(ATTR_BITDEPTH_OUT, atts[i]))
        {
            BitDepth bitdepth = GetBitDepth(atts[i + 1]);
            if (bitdepth == BIT_DEPTH_UNKNOWN)
            {
                ThrowM(*this, "outBitDepth unknown value (", atts[i + 1], ")");
            }
            getOp()->setOutputBitDepth(bitdepth);
            bitDepthFound |= OUTPUT_BIT_DEPTH;
        }
        /* TODO: CTF
        else if (0 == Platform::Strcasecmp(ATTR_BYPASS, atts[i]))
        {
        std::string s(atts[i + 1]);
        boost::algorithm::Trim(s);
        bool bypass = false;
        if (0 == Foundation::Utils::Strcasecmp(s.c_str(), "true"))
        {
        bypass = true;
        }
        else
        {
        XML_THROW1(ILLEGAL_BYPASS_VALUE, atts[i + 1]);
        }
        getOp()->getBypass()->setValue(bypass);
        }*/

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
}

void CTFReaderOpElt::end()
{
}

const char * CTFReaderOpElt::getTypeName() const
{
    OpDataRcPtr op = getOp();
    auto & r = *op;
    return typeid(r).name();
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
// any existing versions) and second, only the lastest version will be written
// (TODO: write to be implemented).
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

CTFReaderOpEltRcPtr CTFReaderOpElt::GetReader(CTFReaderOpElt::Type type, const CTFVersion & version)
{
    CTFReaderOpEltRcPtr pOp;

    switch (type)
    {
    case CTFReaderOpElt::MatrixType:
    {
        // If the version is 1.2 or less, then use MatrixElt.
        ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderMatrixElt, 1_2);
        // If the version is 1.3 or more, then use MatrixElt_1_3.
        ADD_DEFAULT_READER(CTFReaderMatrixElt_1_3);
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
    case CTFReaderOpElt::InvLut1DType:
    {
        ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderInvLut1DElt, 1_3);
        break;
    }
    case CTFReaderOpElt::Lut3DType:
    {
        ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderLut3DElt, 1_6);
        // Adding basic IndexMap element.
        ADD_DEFAULT_READER(CTFReaderLut3DElt_1_7);
        break;
    }
    case CTFReaderOpElt::InvLut3DType:
    {
        ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderInvLut3DElt, 1_6);
        break;
    }
    case CTFReaderOpElt::RangeType:
    {
        ADD_READER_FOR_VERSIONS_UP_TO(CTFReaderRangeElt, 1_6);
        // Adding noClamp style.
        ADD_DEFAULT_READER(CTFReaderRangeElt_1_7);
        break;
    }
    /* TODO: CTF
    case Color::Op::Reference:
    {
    ADD_READER_FOR_VERSIONS_UP_TO(ReferenceElt, 1_6);
    // Adding inverted attribute
    ADD_DEFAULT_READER(ReferenceElt_1_7);
    break;
    }
    case Color::Op::Gamma:
    {
    // If the version is 1.4 or less, then use GammaElt
    // This reader forces the alpha transformation to be the identity.
    ADD_READER_FOR_VERSIONS_UP_TO(GammaElt, 1_4);
    // If the version is 1.5 or more, then use GammaElt_1_5
    ADD_DEFAULT_READER(GammaElt_1_5);
    break;
    }
    case Color::Op::ExposureContrast:
    {
    ADD_DEFAULT_READER(ExposureContrastElt);
    break;
    }
    case Color::Op::Log:
    {
    ADD_READER_FOR_VERSIONS_STARTING_AT(LogElt, 1_3);
    break;
    }*/
    case CTFReaderOpElt::CDLType:
    {
        // Note: CLF style name support was not added until version 1.7, but
        // no point creating a separate version just for that.
        ADD_READER_FOR_VERSIONS_STARTING_AT(CTFReaderCDLElt, 1_3);
        break;
    }
    /* TODO: CTF
    case Color::Op::Dither:
    {
    ADD_READER_FOR_VERSIONS_STARTING_AT(DitherElt, 1_5);
    break;
    }
    case Color::Op::GamutMap:
    {
    ADD_READER_FOR_VERSIONS_STARTING_AT(GamutMapElt, 1_4);
    break;
    }
    case Color::Op::HueVector:
    {
    ADD_READER_FOR_VERSIONS_STARTING_AT(HueVectorElt, 1_4);
    break;
    }
    case OpData::OpData::ACESType:
    {
    ADD_READER_FOR_VERSIONS_STARTING_AT(ACESElt, 1_5);
    break;
    }
    case Color::Op::Function:
    {
    ADD_READER_FOR_VERSIONS_STARTING_AT(FunctionElt, 1_8);
    break;
    }*/
    }

    return pOp;
}

BitDepth CTFReaderOpElt::GetBitDepth(const std::string & strBD)
{
    const std::string str = pystring::lower(strBD);
    if (str == "8i") return BIT_DEPTH_UINT8;
    else if (str == "10i") return BIT_DEPTH_UINT10;
    else if (str == "12i") return BIT_DEPTH_UINT12;
    else if (str == "16i") return BIT_DEPTH_UINT16;
    else if (str == "16f") return BIT_DEPTH_F16;
    else if (str == "32f") return BIT_DEPTH_F32;
    return BIT_DEPTH_UNKNOWN;
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
        if (0 == Platform::Strcasecmp(ATTR_CDL_STYLE, atts[i]))
        {
            // Unrecognized CDL styles will throw an exception.
            m_cdl->setStyle(CDLOpData::GetStyle(atts[i + 1]));
            isStyleFound = true;
        }
    }

    if (!isStyleFound)
    {
        throwMessage("CTF/CLF CDL parsing. Required attribute 'style' is missing. ");
    }
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

    // Validate the end result.
    m_matrix->validate();
}

const OpDataRcPtr CTFReaderMatrixElt::getOp() const
{
    return m_matrix;
}

ArrayBase * CTFReaderMatrixElt::updateDimension(const Dimensions & dims)
{
    if (dims.size() != 3)
    {
        return nullptr;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size() - 1));
    const unsigned numColorComponents = dims[max];

    if (dims[0] != dims[1] || dims[2] != 3)
    {
        return nullptr;
    }

    ArrayDouble * pArray = &m_matrix->getArray();
    pArray->resize(dims[0], numColorComponents);

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

    // Extract offsets.

    const ArrayDouble::Values & values = array.getValues();

    if (array.getLength() == 4)
    {
        m_matrix->setOffsetValue(0, values[3]);
        m_matrix->setOffsetValue(1, values[7]);
        m_matrix->setOffsetValue(2, values[11]);

        m_matrix->setArrayValue(3, 0.0f);
        m_matrix->setArrayValue(7, 0.0f);
        m_matrix->setArrayValue(11, 0.0f);
        m_matrix->setArrayValue(15, 1.0f);
    }

    // Array parsing is done.

    setCompleted(true);

    convert_1_2_to_Latest();

    array.validate();
}

void CTFReaderMatrixElt::convert_1_2_to_Latest()
{
    if (CTF_PROCESS_LIST_VERSION_1_2 < CTF_PROCESS_LIST_VERSION)
    {
        ArrayDouble & array = m_matrix->getArray();

        if (array.getLength() == 3)
        {
            const float offsets[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            m_matrix->setRGBAOffsets(offsets);
        }
        else if (array.getLength() == 4)
        {
            m_matrix->setOffsetValue(3, 0.0f);

            array = m_matrix->getArray();
            ArrayDouble::Values oldV = array.getValues();

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
        else if (array.getLength() != 4)
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
    if (dims.size() != 3)
    {
        return nullptr;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size() - 1));
    const unsigned numColorComponents = dims[max];

    if (!((dims[0] == 3 && dims[1] == 3 && dims[2] == 3) ||
          (dims[0] == 3 && dims[1] == 4 && dims[2] == 3) ||
          (dims[0] == 4 && dims[1] == 4 && dims[2] == 4) ||
          (dims[0] == 4 && dims[1] == 5 && dims[2] == 4)))
    {
        return nullptr;
    }

    ArrayDouble * pArray = &getMatrix()->getArray();
    pArray->resize(dims[1], numColorComponents);

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

            const float offsets[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
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

    array.validate();
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

    // As the 'interpolation' element is optional,
    // set the value to default behavior.
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
                       "' while parsing Lut1D. ");
            }

            m_lut->setInputHalfDomain(true);
        }

        if (0 == Platform::Strcasecmp(ATTR_RAW_HALFS, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                ThrowM(*this, "Illegal 'rawHalfs' attribute '", atts[i + 1],
                       "' while parsing Lut1D. ");
            }

            m_lut->setOutputRawHalfs(true);
        }

        i += 2;
    }
}

void CTFReaderLut1DElt::end()
{
    CTFReaderOpElt::end();
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
                   " Array values, found ", position, ". ");
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

    // Record the original array scaling present in the file.  This is used by
    // a heuristic involved with LUT inversion.  The bit-depth of ops is
    // typically changed after the file is read, hence the need to store it now.
    m_lut->setFileBitDepth(m_lut->getOutputBitDepth());

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
               " IndexMap values, found ", position, ". ");
    }

    m_indexMapping.validate();
    setCompletedIM(true);
}

//////////////////////////////////////////////////////////

void CTFReaderLut1DElt_1_4::start(const char ** atts)
{
    CTFReaderOpElt::start(atts);

    // As the 'interpolation' element is optional,
    // set the value to default behavior.
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
                       "' while parsing Lut1D. ");
            }

            m_lut->setInputHalfDomain(true);
        }

        if (0 == Platform::Strcasecmp(ATTR_RAW_HALFS, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i + 1]))
            {
                ThrowM(*this, "Illegal 'rawHalfs' attribute '", atts[i + 1],
                       "' while parsing Lut1D. ");
            }

            m_lut->setOutputRawHalfs(true);
        }

        // This was added in v1.4.
        if (0 == Platform::Strcasecmp(ATTR_HUE_ADJUST, atts[i]))
        {
            if (0 != Platform::Strcasecmp("dw3", atts[i + 1]))
            {
                ThrowM(*this, "Illegal 'hueAdjust' attribute '", atts[i + 1],
                       "' while parsing Lut1D. ");
            }

            m_lut->setHueAdjust(Lut1DOpData::HUE_DW3);
        }

        i += 2;
    }

}

//////////////////////////////////////////////////////////

void CTFReaderLut1DElt_1_7::end()
{
    CTFReaderOpElt::end();
    m_lut->validate();

    // The LUT renderers do not currently support an indexMap, however for
    // compliance with the CLF spec we want to support the case of a single 2-entry
    // indexMap by converting it into a RangeOp which we insert before the LUT.
    if (isCompletedIM())
    {
        // This will throw if the indexMap is not length 2.
        RangeOpDataRcPtr pRng = std::make_shared<RangeOpData>(
            m_indexMapping,
            m_lut->getInputBitDepth(),
            m_lut->getArray().getLength());

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

    // As the 'interpolation' element is optional,
    // set the value to default behavior.
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
                oss << "' while parsing InvLut1D. ";
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
                oss << "' while parsing InvLut1D. ";
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
                oss << "' while parsing InvLut1D. ";
                throwMessage(oss.str());
            }

            m_invLut->setHueAdjust(Lut1DOpData::HUE_DW3);
        }

        i += 2;
    }
}

void CTFReaderInvLut1DElt::end()
{
    CTFReaderOpElt::end();
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
            arg << " Array values, found " << position << ". ";
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

    // Record the original array scaling present in the file.  This is used by
    // a heuristic involved with LUT inversion.  The bit-depth of ops is
    // typically changed after the file is read, hence the need to store it now.
    m_invLut->setFileBitDepth(m_invLut->getInputBitDepth());

    // At this point, we have created the complete Lut1D base class.
    // Finalize will finish initializing as an inverse Lut1D.

    setCompleted(true);
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

    // As the 'interpolation' element is optional,
    // set the value to default behavior.
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

void CTFReaderLut3DElt::end()
{
    CTFReaderOpElt::end();
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
    Array* pArray = &m_lut->getArray();
    if (pArray->getNumValues() != position)
    {
        ThrowM(*this, "Expected ", pArray->getLength(), "x", pArray->getLength(),
               "x", pArray->getLength(), "x", pArray->getNumColorComponents(),
               " Array values, found ", position);
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
               " IndexMap values, found ", position, ". ");
    }

    m_indexMapping.validate();
    setCompletedIM(true);
}

//////////////////////////////////////////////////////////

// TODO: Factor duplicate code from Lut1D version.
void CTFReaderLut3DElt_1_7::end()
{
    CTFReaderOpElt::end();
    m_lut->validate();

    // The LUT renderers do not currently support an indexMap, however for
    // compliance with the CLF spec we want to support the case of a single 2-entry
    // indexMap by converting it into a RangeOp which we insert before the LUT.
    if (isCompletedIM())
    {
        // This will throw if the indexMap is not length 2.
        RangeOpDataRcPtr pRng = std::make_shared<RangeOpData>(
            m_indexMapping,
            m_lut->getInputBitDepth(),
            m_lut->getArray().getLength());

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

    // As the 'interpolation' element is optional,
    // set the value to default behavior.
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

void CTFReaderInvLut3DElt::end()
{
    CTFReaderOpElt::end();
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
        arg << " Array values, found " << position << ". ";
        throwMessage(arg.str());
    }

    pArray->validate();

    // At this point, we have created the complete Lut3D
    // Finalize will finish initializing as an inverse Lut3D.

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
        if (0 == Platform::Strcasecmp(ATTR_RANGE_STYLE, atts[i]))
        {
            m_isNoClamp = (0 == Platform::Strcasecmp("noClamp", atts[i + 1]));
        }

        i += 2;
    }
}

void CTFReaderRangeElt_1_7::end()
{
    CTFReaderRangeElt::end();

    // Adding support for the noClamp style introduced in the CLF spec.
    // We convert our RangeOp (which always clamps) into an equivalent MatrixOp.
    if (m_isNoClamp)
    {
        OpDataRcPtr pMtx = m_range->convertToMatrix();

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

void CTFReaderRangeValueElt::start(const char **)
{
}

void CTFReaderRangeValueElt::end()
{
}

void CTFReaderRangeValueElt::setRawData(const char * s, size_t len, unsigned int)
{
    CTFReaderRangeElt * pRange
        = dynamic_cast<CTFReaderRangeElt*>(getParent().get());

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

}
OCIO_NAMESPACE_EXIT

