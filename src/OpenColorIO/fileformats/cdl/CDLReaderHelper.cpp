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

#include <algorithm>
#include <sstream>

#include "fileformats/cdl/CDLReaderHelper.h"
#include "Logging.h"
#include "ParseUtils.h"
#include "Platform.h"

OCIO_NAMESPACE_ENTER
{

const char ATTR_ID[] = "id";

XmlReaderElement::XmlReaderElement(const std::string& name,
                                   unsigned int xmlLineNumber,
                                   const std::string& xmlFile)
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


void XmlReaderElement::setContext(const std::string& name,
                                  unsigned xmlLineNumber,
                                  const std::string& xmlFile)
{
    m_name = name;
    m_xmlLineNumber = xmlLineNumber;
    m_xmlFile = xmlFile;
}

void XmlReaderElement::throwMessage(const std::string & error) const
{
    std::ostringstream os;
    os << "Error parsing file (";
    os << getXmlFile().c_str() << "). ";
    os << "Error is: " << error.c_str();
    os << " At line (" << getXmlLineNumber() << ")";
    throw Exception(os.str().c_str());
}

const std::string& XmlReaderDummyElt::DummyParent::getIdentifier() const
{
    static const std::string identifier = "Unknown";
    return identifier;
}

XmlReaderDummyElt::XmlReaderDummyElt(const std::string& name,
                                     ConstElementRcPtr pParent,
                                     unsigned xmlLineNumber,
                                     const std::string& xmlFile,
                                     const char* msg)
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
    oss << (msg ? msg : "");
    oss << ": " << getXmlFile().c_str();

    LogDebug(oss.str());
}

const std::string& XmlReaderDummyElt::getIdentifier() const
{
    static std::string emptyName;
    return emptyName;
}

void XmlReaderDescriptionElt::end()
{
    if (m_changed)
    {
        // Note: eXpat automatically replaces escaped characters with 
        //       their original values.
        getParent()->appendDescription(m_description);
    }
}

XmlReaderColorCorrectionElt::XmlReaderColorCorrectionElt(
    const std::string& name,
    ContainerEltRcPtr pParent,
    unsigned xmlLocation,
    const std::string& xmlFile)
    : XmlReaderComplexElt(name, pParent, xmlLocation, xmlFile)
    , m_transformList()
    , m_transform(CDLTransform::Create())
{
}

void XmlReaderColorCorrectionElt::start(const char** atts)
{
    unsigned i = 0;
    while (atts[i])
    {
        if (0 == strcmp(ATTR_ID, atts[i]))
        {
            if (atts[i + 1])
            {
                // Note: eXpat automatically replaces escaped characters with 
                //       their original values.
                m_transform->setID(atts[i + 1]);
            }
            else
            {
                throwMessage("Missing attribute value for id");
            }
        }

        i += 2;
    }
}

void XmlReaderColorCorrectionElt::end()
{
    m_transform->validate();
    m_transformList->push_back(m_transform);
}

void XmlReaderColorCorrectionElt::setCDLTransformList(CDLTransformVecRcPtr pTransformList)
{
    m_transformList = pTransformList;
}

void XmlReaderColorCorrectionElt::appendDescription(const std::string& desc)
{
    // TODO: OCIO only keeps the description on the SOP
    //m_transform->setDescription(desc.c_str());
}


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

std::string XmlReaderElementStack::dump() const
{
    std::ostringstream errMsg;
    errMsg << "CDL parsing stack is ";

    const unsigned max = (const unsigned)m_elms.size();
    for (unsigned i = 0; i<max; ++i)
    {
        errMsg << "[";
        errMsg << m_elms[i]->getName();
        if (!m_elms[i]->getIdentifier().empty())
        {
            errMsg << "=";
            errMsg << m_elms[i]->getIdentifier();
        }
        errMsg << " at line=";
        errMsg << m_elms[i]->getXmlLineNumber();
        errMsg << "] ";
    }

    return errMsg.str();
}

XmlReaderSOPValueElt::XmlReaderSOPValueElt(
    const std::string& name,
    ContainerEltRcPtr pParent,
    unsigned int xmlLineNumber,
    const std::string& xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

XmlReaderSOPValueElt::~XmlReaderSOPValueElt()
{
}

void XmlReaderSOPValueElt::start(const char **atts)
{
    m_contentData = "";
}

// Is c a 'space' character
// Note: Do not use the std::isspace which is very slow.
inline bool IsSpace(char c)
{
    // Note \n is unix while \r\n is windows line feed
    return c == ' ' || c == '\n' || c == '\t' || c == '\r' ||
           c == '\v' || c == '\f';
}

bool IsNotSpace(char c)
{
    return !IsSpace(c);
}

// Is the character a valid number delimiter?
// - c is the character to test
// returns true if the character is a valid delimiter
inline bool IsNumberDelimiter(char c)
{
    return IsSpace(c) || c == ',';
}

// Trim from start
static inline void LTrim(std::string &s)
{
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), IsNotSpace));
}

// Trim from end
static inline void RTrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), IsNotSpace).base(),
            s.end());
}

// Trim from both ends
void Trim(std::string &s)
{
    LTrim(s);
    RTrim(s);
}

// Find the position of the next character to start scanning at
// Delimiters checked are spaces, commas, tabs and newlines.
// - str string to search
// - len is the length of the string
// - start position in string to start the search at
// returns the position of the next delimiter, or len if there are none.
inline size_t FindNextTokenStart(const char* s, size_t len, size_t pos)
{
    const char *ptr = s + pos;

    if (pos == len)
    {
        return pos;
    }

    while (IsNumberDelimiter(*ptr))
    {
        ptr++; pos++;

        if (pos >= len)
        {
            return len;
        }
    }

    return pos;
}

inline size_t FindDelim(const char* str, size_t len, size_t pos)
{
    const char *ptr = str + pos;

    while (!IsNumberDelimiter(*ptr))
    {
        if ((pos + 1) >= len)
        {
            return len;
        }
        ptr++; pos++;
    }

    return pos;
}

// Get first number from a string (specialized for floats)
// str should start with number.
inline void ParseNumber(const char *str, size_t len, float& value)
{
    const size_t end = FindDelim(str, len, 0);
    if (end == 0)
    {
        throw Exception("ParseNumber: string should not start with space.");
    }

    // Perhaps the float is not the only one
    len = end;

    //
    // First check whether the string is a float value.  If there is no
    // match, only then do the infinity string comparison.
    //

    // Note: Always avoid to copy the string
    //
    // Note: As str is a string (and not a raw buffer), the physical length
    //       is always at least len+1.
    //       So the code could manipulate str[len] content.
    //
    char* p = const_cast<char*>(str);
    char t = '\0';
    const bool resizeString = len != strlen(str);
    if (resizeString)
    {
        t = str[len];
        p[len] = '\0';
    }
    const int matches = sscanf(p, "%f", &value);
    if (resizeString)
    {
        p[len] = t;
    }

    if (matches == 0)
    {
        //
        // Did not get a float value match.  See if infinity is present.
        // Only C99 nan and infinity representations are recognized.
        //

        if (((Platform::Strncasecmp(str, "INF", 3) == 0) && (end == 3)) ||
            ((Platform::Strncasecmp(str, "INFINITY", 8) == 0) && (end == 8)))
        {
            value = std::numeric_limits<float>::infinity();
        }
        else if
            (((Platform::Strncasecmp(str, "-INF", 4) == 0) && (end == 4)) ||
            ((Platform::Strncasecmp(str, "-INFINITY", 9) == 0) && (end == 9)))
        {
            value = -std::numeric_limits<float>::infinity();
        }
        else if (Platform::Strncasecmp(str, "NAN", 3) == 0 ||
            Platform::Strncasecmp(str, "-NAN", 4) == 0)
        {
            value = std::numeric_limits<float>::quiet_NaN();
        }
        else
        {
            // No inifity match, bad value in file.
            std::ostringstream ss;
            ss << "ParserNumber: Characters '" << str << "' are illegal.";
            throw Exception(ss.str().c_str());
        }
    }
    else if (matches == -1)
    {
        throw Exception("ParseNumber: error while scanning.");
    }
}

// Extract the next number contained in the string.
// - str the string th search
// - prev position to start the search at.
// returns the next in the string.  Note that prev gets updated to the
//         position of the next delimiter, or to std::string::npos
//         if the value returned is the last one in the string.
template<typename T>
void GetNextNumber(const char *s, size_t len, size_t& pos, T& num)
{
    pos = FindNextTokenStart(s, len, pos);

    if (pos != len)
    {
        ParseNumber(s + pos, len - pos, num);

        pos = FindDelim(s, len, pos);
        if (pos != len)
        {
            pos = FindNextTokenStart(s, len, pos);
        }
    }
}

// This method tokenizes a string like "0 1 2" of integers or floats
// returns the numbers extracted from the string
template<typename T>
std::vector<T> GetNumbers(const char* str, size_t len)
{
    std::vector<T> numbers;

    size_t pos = FindNextTokenStart(str, len, 0);
    while (pos != len)
    {
        T num(0);
        GetNextNumber(str, len, pos, num);
        numbers.push_back(num);
    }

    return numbers;
}

// This method truncates a string (mainly used for display purpose)
// - pStr string to search
// - len is the length of the string
// returns the truncated string
inline std::string TruncateString(const char* pStr, size_t len)
{
    static const unsigned MAX_SIZE = 17;

    std::string s(pStr, len);

    if (s.size()>MAX_SIZE)
    {
        s.resize(MAX_SIZE);
        s += "...";
    }

    return s;
}

// Find the position of the first the non-whitespace character.
// Whitespaces are defined as spaces, tabs or newlines here.
// - str string to check.
// - len is the length of the string.
// returns the position of the first non-whitespace character or
//         std::string::npos if the string only has whitespaces or is empty.
inline size_t FindFirstNonWhiteSpace(const char* str, size_t len)
{
    const char *ptr = str;
    size_t pos = 0;

    for (;;)
    {
        if (!IsSpace(*ptr))
        {
            return pos;
        }
        if (pos == len)
        {
            return len;
        }
        ptr++; pos++;
    }
}

// Find the position of the last the non-whitespace character.
// Whitespaces are defined as spaces, tabs or newlines here.
// - str string to check.
// - len is the length of the string.
// returns the position of the last non-whitespace character or
//         std::string::npos if the string only has whitespaces or is empty.
inline size_t FindLastNonWhiteSpace(const char* str, size_t len)
{
    size_t pos = len - 1;
    const char *ptr = str + pos;

    for (;;)
    {
        if (!IsSpace(*ptr))
        {
            return pos;
        }
        if (pos == 0)
        {
            return 0;
        }

        ptr--; pos--;
    }
}


// Get start (first non space character) and
// end (just after the last non space character)
void FindSubString(const char* str, size_t length,
    size_t& start,
    size_t& end)
{
    if (!str || !*str)
    {
        start = 0;
        end = 0;
        return; // nothing to Trim.
    }

    start = FindFirstNonWhiteSpace(str, length);
    if (start == length)
    {
        // str only contains spaces, tabs or newlines.
        // Return an empty string.
        start = 0;
        end = 0;
        return;
    }

    // It is guaranteed here that end will not be 'npos'.
    // At worst, it will equal start
    end = FindLastNonWhiteSpace(str, length);

    // end-start should give the number of valid characters
    if (!IsSpace(str[end])) ++end;
}

void XmlReaderSOPValueElt::end()
{
    Trim(m_contentData);

    std::vector<float> data;

    try
    {
        data = GetNumbers<float>(m_contentData.c_str(), m_contentData.size());
    }
    catch (Exception&)
    {
        const std::string s = TruncateString(m_contentData.c_str(), m_contentData.size());
        std::ostringstream oss;
        oss << "Illegal values '";
        oss << s;
        oss << "' in ";
        oss << getTypeName();
        throwMessage(oss.str());
    }

    if (data.size() != 3)
    {
        throwMessage("SOPNode: 3 values required.");
    }

    XmlReaderSOPNodeBaseElt* pSOPNodeElt = dynamic_cast<XmlReaderSOPNodeBaseElt*>(getParent().get());
    CDLTransformRcPtr pCDL = pSOPNodeElt->getCDL();

    if (0 == strcmp(getName().c_str(), TAG_SLOPE))
    {
        pCDL->setSlope(&data[0]);
        pSOPNodeElt->setIsSlopeInit(true);
    }
    else if (0 == strcmp(getName().c_str(), TAG_OFFSET))
    {
        pCDL->setOffset(data.data());
        pSOPNodeElt->setIsOffsetInit(true);
    }
    else if (0 == strcmp(getName().c_str(), TAG_POWER))
    {
        pCDL->setPower(data.data());
        pSOPNodeElt->setIsPowerInit(true);
    }
}

void XmlReaderSOPValueElt::setRawData(const char* str, size_t len, unsigned int xmlLine)
{
    m_contentData += std::string(str, len) + " ";
}

XmlReaderSaturationElt::XmlReaderSaturationElt(const std::string& name,
                                               ContainerEltRcPtr pParent,
                                               unsigned int xmlLineNumber,
                                               const std::string& xmlFile)
    : XmlReaderPlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

XmlReaderSaturationElt::~XmlReaderSaturationElt()
{
}

void XmlReaderSaturationElt::start(const char** /* atts */)
{
    m_contentData = "";
}

void XmlReaderSaturationElt::end()
{
    Trim(m_contentData);

    std::vector<float> data;

    try
    {
        data = GetNumbers<float>(m_contentData.c_str(), m_contentData.size());
    }
    catch (Exception& /* ce */)
    {
        const std::string s = TruncateString(m_contentData.c_str(), m_contentData.size());
        std::ostringstream oss;
        oss << "Illegal values '";
        oss << s;
        oss << "' in ";
        oss << getTypeName();
        throwMessage(oss.str());
    }

    if (data.size() != 1)
    {
        throwMessage("SatNode: non-single value. ");
    }

    XmlReaderSatNodeBaseElt* pSatNodeElt = dynamic_cast<XmlReaderSatNodeBaseElt*>(getParent().get());
    CDLTransformRcPtr pCDL = pSatNodeElt->getCDL();

    if (0 == strcmp(getName().c_str(), TAG_SATURATION))
    {
        pCDL->setSat(data[0]);
    }
}

void XmlReaderSaturationElt::setRawData(const char* str, size_t len, unsigned int)
{
    m_contentData += std::string(str, len) + " ";
}

}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

#include "MathUtils.h"

#define EXPECTED_PRECISION 1e-11f

OIIO_ADD_TEST(CDLReader, StringToFloat)
{
    float value = 0.0f;
    {
        const char str[] = "0.00001";
        const size_t len = strlen(str);

        OCIO::ParseNumber(str, len, value);
        OIIO_CHECK_CLOSE(value, 0.00001f, EXPECTED_PRECISION);
    }

    {
        const char str[] = "1e-5";
        const size_t len = strlen(str);

        OCIO::ParseNumber(str, len, value);
        OIIO_CHECK_CLOSE(value, 0.00001f, EXPECTED_PRECISION);
    }
}

OIIO_ADD_TEST(CDLReader, StringToFloatFailure)
{
    float value = 0.0f;
    const char str[] = "ABDSCSGFDS";
    const size_t len = strlen(str);

    OIIO_CHECK_THROW_WHAT(OCIO::ParseNumber(str, len, value),
                          OCIO::Exception,
                          "are illegal");
}


OIIO_ADD_TEST(CDLReader, Trim)
{
    const std::string original1("    some text    ");
    const std::string original2(" \n \r some text  \t \v \f ");
    {
        std::string value(original1);
        OCIO::Trim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text", value.c_str()));
        value = original2;
        OCIO::Trim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text", value.c_str()));
    }

    {
        std::string value(original1);
        OCIO::RTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("    some text", value.c_str()));
        value = original2;
        OCIO::RTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp(" \n \r some text", value.c_str()));
    }

    {
        std::string value(original1);
        OCIO::LTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text    ", value.c_str()));
        value = original2;
        OCIO::LTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text  \t \v \f ", value.c_str()));
    }
}

OIIO_ADD_TEST(CDLReader, ParseNumber)
{
    float data = 0.0f;
    {
        std::string buffer("1 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0f 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0000 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data);)
            OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("10.0e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("0.1e+1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("-1 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0f 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0000 0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data);)
            OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-10.0e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-0.1e+1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("INF");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("INFINITY");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("-INF");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("-INFINITY");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("NAN");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_ASSERT(OCIO::isnan(data));
    }
    {
        std::string buffer("-NAN");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_ASSERT(OCIO::isnan(data));
    }
    {
        std::string buffer("0.001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-0.001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer(".001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-.001");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer(".01e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-.01e-1");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1,");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1\n");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1\t");
        OIIO_CHECK_NO_THROW(OCIO::ParseNumber(buffer.c_str(),
                                              buffer.size(),
                                              data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }

    {
        std::string buffer("XY");
        OIIO_CHECK_THROW_WHAT(OCIO::ParseNumber(buffer.c_str(),
                                                buffer.size(),
                                                data),
                              OCIO::Exception,
                              "are illegal");
    }
    {
        std::string buffer("            1");
        OIIO_CHECK_THROW_WHAT(OCIO::ParseNumber(buffer.c_str(),
                                                buffer.size(),
                                                data),
                              OCIO::Exception,
                              "not start with space");
    }
}

OIIO_ADD_TEST(CDLReader, FindSubString)
{
    {
        //                  012345678901234
        std::string buffer("   new order   ");
        size_t start, end;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 3);
        OIIO_CHECK_EQUAL(end, 12);
    }
    {
        //                  012345678901234
        std::string buffer("new order   ");
        size_t start, end;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 9);
    }
    {
        //                  012345678901234
        std::string buffer("   new order");
        size_t start, end;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 3);
        OIIO_CHECK_EQUAL(end, 12);
    }
    {
        //                  012345678901234
        std::string buffer("new order");
        size_t start, end;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 9);
    }
    {
        std::string buffer("");
        size_t start, end;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 0);
    }
    {
        std::string buffer("      ");
        size_t start, end;
        OCIO::FindSubString(buffer.c_str(),
                            buffer.size(),
                            start,
                            end);
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 0);
    }
}


#endif