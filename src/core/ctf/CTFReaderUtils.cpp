/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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

#include "CTFReaderUtils.h"
#include "CTFReaderVersion.h"
#include "CTFElement.h"
#include "../Platform.h"
#include <sstream>
#include <algorithm>

OCIO_NAMESPACE_ENTER
{

// Private namespace to the OpData sub-directory
namespace CTF
{

// XML tags

// Process List tag name
const char TAG_PROCESS_LIST[] = "ProcessList";

// Info tag name
const char TAG_INFO[] = "Info";

// Description tag name
const char TAG_DESCRIPTION[] = "Description";

// Input Descriptor tag name
const char TAG_INPUT_DESCRIPTOR[] = "InputDescriptor";

// Output Descriptor tag name
const char TAG_OUTPUT_DESCRIPTOR[] = "OutputDescriptor";

// Matrix tag name
const char TAG_MATRIX[] = "Matrix";

// Array tag name
const char TAG_ARRAY[] = "Array";

// LUT1D tag name
const char TAG_LUT1D[] = "LUT1D";

// INVLUT1D tag name
const char TAG_INVLUT1D[] = "InverseLUT1D";

// Index map tag name
const char TAG_INDEX_MAP[] = "IndexMap";

// Range tag name
const char TAG_RANGE[] = "Range";

// Range value tag name
const char TAG_MIN_IN_VALUE[] = "minInValue";

// Range value tag name
const char TAG_MAX_IN_VALUE[] = "maxInValue";

// Range value tag name
const char TAG_MIN_OUT_VALUE[] = "minOutValue";

// Range value tag name
const char TAG_MAX_OUT_VALUE[] = "maxOutValue";

// CDL tag name
const char TAG_CDL[] = "ASC_CDL";

// SOPNode tag name
const char TAG_SOPNODE[] = "SOPNode";

// Slope tag name
const char TAG_SLOPE[] = "Slope";

// Offset tag name
const char TAG_OFFSET[] = "Offset";

// Power tag name
const char TAG_POWER[] = "Power";

// SatNode tag name
const char TAG_SATNODE[] = "SatNode";

// Saturation tag name
const char TAG_SATURATION[] = "Saturation";

// LUT3D tag name
const char TAG_LUT3D[] = "LUT3D";

// INVLUT3D tag name
const char TAG_INVLUT3D[] = "InverseLUT3D";


// id attribute
const char ATTR_ID[] = "id";

// name attribute
const char ATTR_NAME[] = "name";

// inverseOf attribute
const char ATTR_INVERSE_OF[] = "inverseOf";

// Version attribute.
const char ATTR_VERSION[] = "version";

// Version attribute.
const char ATTR_COMP_CLF_VERSION[] = "compCLFversion";

// inBitDepth attribute
const char ATTR_IN_BIT_DEPTH[] = "inBitDepth";

// outBitDepth attribute
const char ATTR_OUT_BIT_DEPTH[] = "outBitDepth";

// Array dimension attribute
const char ATTR_DIMENSION[] = "dim";

// LUT interpolation attribute
const char ATTR_INTERPOLATION[] = "interpolation";

// half domain attribute
const char ATTR_HALF_DOMAIN[] = "halfDomain";

// raw halfs attribute
const char ATTR_RAW_HALFS[] = "rawHalfs";

// hue adjust attribute
const char ATTR_HUE_ADJUST[] = "hueAdjust";

// Range style
const char ATTR_RANGE_STYLE[] = "style";

// CDL style
const char ATTR_CDL_STYLE[] = "style";




// Private namespace for the xml reader utils
namespace Reader
{


bool IsNotSpace(char c)
{
    return !Reader::IsSpace(c);
}




// Trim from start
static inline void LTrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        IsNotSpace));
}

// Trim from end
static inline void RTrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
        IsNotSpace).base(), s.end());
}

// Trim from both ends
void Trim(std::string &s)
{
    LTrim(s);
    RTrim(s);
}


ElementStack::ElementStack()
{
}

ElementStack::~ElementStack()
{
    clear();
}


unsigned ElementStack::size() const
{
    return (const unsigned)m_elms.size();
}

bool ElementStack::empty() const
{
    return m_elms.empty();
}

void ElementStack::push_back(Element* pElt)
{
    m_elms.push_back(pElt);
}

void ElementStack::pop_back()
{
    m_elms.pop_back();
}

Element* ElementStack::back() const
{
    return m_elms.back();
}

Element* ElementStack::front() const
{
    return m_elms.front();
}

void ElementStack::clear()
{
    const unsigned max = (const unsigned)m_elms.size();
    for (unsigned i = 0; i<max; ++i)
    {
        delete m_elms[i];
    }

    m_elms.clear();
}

std::string ElementStack::dump() const
{
    std::ostringstream errMsg;
    errMsg << "CTF stack is ";

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
        errMsg << m_elms[i]->getXmLineNumber();
        errMsg << "] ";
    }

    return errMsg.str();
}



} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

//////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"
#include "../MathUtils.h"

OIIO_ADD_TEST(CTFReaderUtil, Trim)
{
    const std::string original1("    some text    ");
    const std::string original2(" \n \r some text  \t \v \f ");
    {
        std::string value(original1);
        OCIO::CTF::Reader::Trim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text", value.c_str()));
        value = original2;
        OCIO::CTF::Reader::Trim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text", value.c_str()));
    }

    {
        std::string value(original1);
        OCIO::CTF::Reader::RTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("    some text", value.c_str()));
        value = original2;
        OCIO::CTF::Reader::RTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp(" \n \r some text", value.c_str()));
    }

    {
        std::string value(original1);
        OCIO::CTF::Reader::LTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text    ", value.c_str()));
        value = original2;
        OCIO::CTF::Reader::LTrim(value);
        OIIO_CHECK_ASSERT(0 == strcmp("some text  \t \v \f ", value.c_str()));
    }
}

OIIO_ADD_TEST(CTFReaderUtil, ParseNumber)
{
    float data = 0.0f;
    {
        std::string buffer("1 0");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0 0");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0f 0");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0000 0");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1.0");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("1");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data);)
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("10.0e-1");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("0.1e+1");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, 1.0f);
    }
    {
        std::string buffer("-1 0");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0 0");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0f 0");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0000 0");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1.0");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-1");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data);)
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-10.0e-1");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("-0.1e+1");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -1.0f);
    }
    {
        std::string buffer("INF");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("INFINITY");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("-INF");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("-INFINITY");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -std::numeric_limits<float>::infinity());
    }
    {
        std::string buffer("NAN");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_ASSERT(OCIO::isnan(data));
    }
    {
        std::string buffer("-NAN");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_ASSERT(OCIO::isnan(data));
    }
    {
        std::string buffer("0.001");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-0.001");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer(".001");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-.001");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer(".01e-1");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, 0.001f);
    }
    {
        std::string buffer("-.01e-1");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1,");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1\n");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }
    {
        std::string buffer("-.01e-1\t");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           data));
        OIIO_CHECK_EQUAL(data, -0.001f);
    }

    {
        std::string buffer("XY");
        OIIO_CHECK_THROW_WHAT(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                             buffer.size(),
                                                             data),
                              OCIO::Exception,
                              "are illegal" );
    }
    {
        std::string buffer("            1");
        OIIO_CHECK_THROW_WHAT(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                             buffer.size(),
                                                             data),
                              OCIO::Exception,
                              "not start with space");
    }

    int dataInt = 0;
    {
        std::string buffer("       1");
        OIIO_CHECK_THROW_WHAT(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                             buffer.size(),
                                                             dataInt),
                              OCIO::Exception,
                              "not start with space");
    }
    {
        std::string buffer("42");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           dataInt));
        OIIO_CHECK_EQUAL(dataInt, 42);
    }

    double dataDouble = 0.0;
    {
        std::string buffer("1.234567890123456");
        OIIO_CHECK_NO_THROW(OCIO::CTF::Reader::ParseNumber(buffer.c_str(),
                                                           buffer.size(),
                                                           dataDouble));
        OIIO_CHECK_EQUAL(dataDouble, 1.234567890123456);
    }

}

OIIO_ADD_TEST(CTFReaderUtil, FindSubString)
{
    {
        //                  012345678901234
        std::string buffer("   new order   ");
        size_t start, end;
        OCIO::CTF::Reader::FindSubString(buffer.c_str(),
                                         buffer.size(),
                                         start,
                                         end );
        OIIO_CHECK_EQUAL(start, 3);
        OIIO_CHECK_EQUAL(end, 12);
    }
    {
        //                  012345678901234
        std::string buffer("new order   ");
        size_t start, end;
        OCIO::CTF::Reader::FindSubString(buffer.c_str(),
                                         buffer.size(),
                                         start,
                                         end );
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 9);
    }
    {
        //                  012345678901234
        std::string buffer("   new order");
        size_t start, end;
        OCIO::CTF::Reader::FindSubString(buffer.c_str(),
                                         buffer.size(),
                                         start,
                                         end );
        OIIO_CHECK_EQUAL(start, 3);
        OIIO_CHECK_EQUAL(end, 12);
    }
    {
        //                  012345678901234
        std::string buffer("new order");
        size_t start, end;
        OCIO::CTF::Reader::FindSubString(buffer.c_str(),
                                         buffer.size(),
                                         start,
                                         end );
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 9);
    }
    {
        std::string buffer("");
        size_t start, end;
        OCIO::CTF::Reader::FindSubString(buffer.c_str(),
                                         buffer.size(),
                                         start,
                                         end );
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 0);
    }
    {
        std::string buffer("      ");
        size_t start, end;
        OCIO::CTF::Reader::FindSubString(buffer.c_str(),
                                         buffer.size(),
                                         start,
                                         end );
        OIIO_CHECK_EQUAL(start, 0);
        OIIO_CHECK_EQUAL(end, 0);
    }
}

#endif // OCIO_UNIT_TEST
