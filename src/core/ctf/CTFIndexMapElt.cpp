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

#include "CTFIndexMapElt.h"
#include "CTFReaderUtils.h"
#include "CTFContainerElt.h"
#include "CTFOpElt.h"
#include "../opdata/OpDataIndexMapping.h"
#include <sstream>
#include "../Platform.h"

OCIO_NAMESPACE_ENTER
{

// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

IndexMapElt::IndexMapElt(const std::string& name,
                         ContainerElt* pParent,
                         unsigned xmlLineNumber,
                         const std::string& xmlFile)
    : PlainElt(name, pParent, xmlLineNumber, xmlFile)
    , m_indexMap(0x0)
    , m_position(0)
{
}

IndexMapElt::~IndexMapElt()
{
    m_indexMap = 0x0; // Not owned
}

void IndexMapElt::start(const char **atts)
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

            IndexMapMgt::DimensionsIM dims;
            try
            {
                dims = GetNumbers<unsigned>(dimStr, len);
            }
            catch (Exception& /*ce*/)
            {
                std::ostringstream oss;
                oss << "Illegal '";
                oss << getTypeName();
                oss << "' IndexMap dimensions ";
                oss << TruncateString(dimStr, len);
                Throw(oss.str());
            }

            IndexMapMgt* pArr = dynamic_cast<IndexMapMgt*>(getParent());
            if (!pArr)
            {
                std::ostringstream oss;
                oss << "Illegal '";
                oss << getTypeName();
                oss << "' IndexMap dimensions ";
                oss << TruncateString(dimStr, len);
                Throw(oss.str());
            }
            else
            {
                if (dims.empty() || dims.size() != 1)
                {
                    std::ostringstream oss;
                    oss << "Illegal '";
                    oss << getTypeName();
                    oss << "' IndexMap dimensions ";
                    oss << TruncateString(dimStr, len);
                    Throw(oss.str());
                }

                m_indexMap = pArr->updateDimensionIM(dims);
                if (!m_indexMap)
                {
                    std::ostringstream oss;
                    oss << "Illegal '";
                    oss << getTypeName();
                    oss << "' IndexMap dimensions ";
                    oss << TruncateString(dimStr, len);
                    Throw(oss.str());
                }
            }
        }

        i += 2;
    }

    // Check mandatory elements
    if (!isDimFound)
    {
        Throw("Required attribute 'dim' is missing. ");
    }

    m_position = 0;
}

void IndexMapElt::end()
{
    // A known element (e.g. an IndexMap) in a dummy element,
    // no need to validate it.
    if (getParent()->isDummy()) return;

    IndexMapMgt* pIMM = dynamic_cast<IndexMapMgt*>(getParent());
    pIMM->finalizeIM(m_position);
}

// Like findDelim() but looks for whitespace or an ampersand (for IndexMap).
size_t FindIndexDelim(const char* str, size_t len, size_t pos)
{
    const char *ptr = str + pos;

    while (!Reader::IsSpace(*ptr) && !(*ptr == '@'))
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

    while (Reader::IsNumberDelimiter(*ptr) || (*ptr == '@'))
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
        CTF::Reader::ParseNumber(s + pos, len - pos, num1);

        // Set pos to advance over the numbers we just parsed.
        // Note that we stop either at white space or an ampersand.
        pos = FindIndexDelim(s, len, pos);

        // Set pos to the start of the next number, advancing over white space or an @.
        pos = FindNextTokenStart_IndexMap(s, len, pos);

        // Extract the other half of the index pair.
        CTF::Reader::ParseNumber(s + pos, len - pos, num2);

        // Set pos to advance over the numbers we just parsed.
        pos = CTF::Reader::FindDelim(s, len, pos);

        if (pos != len)
        {
            pos = FindNextTokenStart(s, len, pos);
        }
    }
}

void IndexMapElt::setRawData(const char* s, size_t len, unsigned /*xmlLine*/)
{
    const unsigned maxValues = m_indexMap->getDimension();
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
            std::ostringstream oss;
            oss << "Illegal values '";
            oss << TruncateString(s, len);
            oss << "' in '";
            oss << getTypeName();
            oss << "' IndexMap";
            Throw(oss.str());
        }

        if (m_position<maxValues)
        {
            m_indexMap->setPair(m_position++, data1, data2);
        }
        else
        {
            std::ostringstream oss;
            oss << "Expected " << m_indexMap->getDimension();
            oss << " entries, found additional values in '";
            oss << getTypeName();
            oss << "' IndexMap.";
            Throw(oss.str());
        }
    }
}

const std::string& IndexMapElt::getTypeName() const
{
    return dynamic_cast<const OpElt*>(getParent())->getTypeName();
}


} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

