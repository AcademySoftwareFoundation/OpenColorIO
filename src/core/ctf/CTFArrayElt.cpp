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

#include "CTFArrayElt.h"
#include "CTFArrayMgt.h"
#include "CTFOpElt.h"
#include "CTFReaderUtils.h"
#include "../opdata/OpDataArray.h"
#include "../Platform.h"
#include <sstream>

OCIO_NAMESPACE_ENTER
{

// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{

ArrayElt::ArrayElt(const std::string& name,
                   ContainerElt* pParent,
                   unsigned xmlLineNumber,
                   const std::string& xmlFile)
    : PlainElt(name, pParent, xmlLineNumber, xmlFile)
    , m_array(0x0)
    , m_position(0)
{
}

ArrayElt::~ArrayElt()
{
    m_array = 0x0; // Not owned
}

void ArrayElt::start(const char **atts)
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

            ArrayMgt::Dimensions dims;
            try
            {
                dims = GetNumbers<unsigned>(dimStr, len);
            }
            catch (Exception& /*ce*/)
            {
                std::ostringstream oss;
                oss << "Illegal '";
                oss << getTypeName();
                oss << "' dimensions ";
                oss << TruncateString(dimStr, len);
                Throw(oss.str());
            }

            ArrayMgt* pArr = dynamic_cast<ArrayMgt*>(getParent());
            if (!pArr)
            {
                std::ostringstream oss;
                oss << "Parsing issue while parsing dimensions of '";
                oss << getTypeName();
                oss << "' (";
                oss << TruncateString(dimStr, len);
                oss << ").";
                Throw(oss.str());
            }
            else
            {
                const unsigned max =
                    (const unsigned)(dims.empty() ? 0 : (dims.size() - 1));
                if (max == 0)
                {
                    std::ostringstream oss;
                    oss << "Illegal '";
                    oss << getTypeName();
                    oss << "' dimensions ";
                    oss << TruncateString(dimStr, len);
                    Throw(oss.str());
                }

                m_array = pArr->updateDimension(dims);
                if (!m_array)
                {
                    std::ostringstream oss;
                    oss << "Illegal '";
                    oss << getTypeName();
                    oss << "' dimensions ";
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
        Throw("Missing 'dim' attribute.");
    }

    m_position = 0;
}

void ArrayElt::end()
{
    // A known element (e.g. an array) in a dummy element,
    // no need to validate it.
    if (getParent()->isDummy()) return;

    ArrayMgt* pArr = dynamic_cast<ArrayMgt*>(getParent());
    pArr->finalize(m_position);
}

void ArrayElt::setRawData(const char* s, size_t len, unsigned /*xmlLine*/)
{
    const unsigned maxValues = m_array->getNumValues();
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
            std::ostringstream oss;
            oss << "Illegal values '";
            oss << TruncateString(s, len);
            oss << "' in ";
            oss << getTypeName();
            Throw(oss.str());
        }

        if (m_position<maxValues)
        {
            m_array->setDoubleValue(m_position++, data);
        }
        else
        {
            const OpElt* p = static_cast<const OpElt*>(getParent());

            std::ostringstream arg;
            if (p->getOp()->getOpType() == OpData::OpData::Lut1DType)
            {
                arg << m_array->getLength();
                arg << "x" << m_array->getNumColorComponents();
            }
            else if (p->getOp()->getOpType() == OpData::OpData::Lut3DType)
            {
                arg << m_array->getLength() << "x";
                arg << m_array->getLength();
                arg << "x" << m_array->getLength();
                arg << "x" << m_array->getNumColorComponents();
            }
            else  // Matrix
            {
                arg << m_array->getLength();
                arg << "x" << m_array->getLength();
            }

            std::ostringstream oss;
            oss << "Expected " << arg.str();
            oss << " Array, found additional values in '";
            oss << getTypeName();
            oss << "'.";
            Throw(oss.str());
        }
    }
}

const std::string& ArrayElt::getTypeName() const
{
    return dynamic_cast<const OpElt*>(getParent())->getTypeName();
}

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

