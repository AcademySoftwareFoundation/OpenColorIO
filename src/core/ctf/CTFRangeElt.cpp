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

#include "CTFRangeElt.h"
#include "CTFReaderUtils.h"
#include "../opdata/OpDataRange.h"
#include "../opdata/OpDataMatrix.h"
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

RangeElt::RangeElt()
    : OpElt()
    , m_range(new OpData::Range)
{
}

RangeElt::~RangeElt()
{
    // Do not delete  m_range
    m_range = 0x0;
}

void RangeElt::start(const char **atts)
{
    OpElt::start(atts);
}

void RangeElt::end()
{
    OpElt::end();

    // Validate the end result

    m_range->validate();
}

OpData::OpData* RangeElt::getOp() const
{
    return m_range;
}

//////////////////////////////////////////////////////////


RangeElt_1_7::RangeElt_1_7()
    : RangeElt()
{
}

RangeElt_1_7::~RangeElt_1_7()
{
}

void RangeElt_1_7::start(const char **atts)
{
    RangeElt::start(atts);

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

void RangeElt_1_7::end()
{
    OpElt::end();
    m_range->validate();

    // Adding support for the noClamp style introduced in the CLF spec.
    // We convert our RangeOp (which always clamps) into an equivalent MatrixOp.
    if (m_isNoClamp)
    {
        std::auto_ptr<OpData::OpData> pMtx(m_range->convertToMatrix());

        // This code assumes that the current Range is at the end of the opList.
        // In other words, that this Op's end() method will be called before
        // any other Op's start().
        // Also, since replace() deletes m_range, we assume that no other methods
        // will be called on this RangeElt instance.
        const unsigned len = (unsigned)m_transform->getOps().size();
        const unsigned pos = len - 1;

        // Insert new op at idx (will adjust bitdepth as needed)
        m_transform->getOps().replace(pMtx.release(), pos);
    }
}

//////////////////////////////////////////////////////////

RangeValueElt::RangeValueElt(const std::string& name,
                             ContainerElt* pParent,
                             unsigned xmlLineNumber,
                             const std::string& xmlFile)
    : PlainElt(name, pParent, xmlLineNumber, xmlFile)
{
}

RangeValueElt::~RangeValueElt()
{
}

void RangeValueElt::start(const char **)
{
}

void RangeValueElt::end()
{
}

void RangeValueElt::setRawData(const char* s, size_t len, unsigned)
{
    RangeElt* pRange
        = dynamic_cast<RangeElt*>(getParent());

    std::vector<float> data;

    try
    {
        data = GetNumbers<float>(s, len);
    }
    catch (Exception& /*ce*/)
    {
        std::ostringstream oss;
        oss << "Illegal '";
        oss << getTypeName();
        oss << "' values ";
        oss << TruncateString(s, len);
        Throw(oss.str());
    }

    if (data.size() != 1)
    {
        Throw("Range element: non-single value.");
    }

    if (0 == Platform::Strcasecmp(getName().c_str(), CTF::TAG_MIN_IN_VALUE))
    {
        pRange->getRange()->setMinInValue(data[0]);
    }
    else if (0 == Platform::Strcasecmp(getName().c_str(), CTF::TAG_MAX_IN_VALUE))
    {
        pRange->getRange()->setMaxInValue(data[0]);
    }
    else if (0 == Platform::Strcasecmp(getName().c_str(), CTF::TAG_MIN_OUT_VALUE))
    {
        pRange->getRange()->setMinOutValue(data[0]);
    }
    else if (0 == Platform::Strcasecmp(getName().c_str(), CTF::TAG_MAX_OUT_VALUE))
    {
        pRange->getRange()->setMaxOutValue(data[0]);
    }
}


} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

