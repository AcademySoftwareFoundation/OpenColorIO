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

#include "CTFInvLut1DElt.h"
#include "CTFReaderUtils.h"
#include <sstream>

#include "../Platform.h"
#include "../Logging.h"
#include "../MathUtils.h"

#include "../opdata/OpDataRange.h"

OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{
InvLut1DElt::InvLut1DElt()
    :   m_pInvLut(new OpData::InvLut1D)
{
}

InvLut1DElt::~InvLut1DElt()
{
    // Do not delete m_pInvLut (caller owns it now).
    m_pInvLut = 0x0;
}

void InvLut1DElt::start(const char **atts)
{
    OpElt::start(atts);

    // As the 'interpolation' element is optional,
    // set the value to default behavior
    m_pInvLut->setInterpolation(INTERP_DEFAULT);

    unsigned i = 0;
    while(atts[i])
    {
        if(0==Platform::Strcasecmp(ATTR_INTERPOLATION, atts[i]))
        {
            try
            {
                m_pInvLut->setInterpolation(
                    OpData::Lut1D::getInterpolation(atts[i + 1]));
            }
            catch (const std::exception & e)
            {
                Throw(e.what());
            }
        }

        if(0==Platform::Strcasecmp(ATTR_HALF_DOMAIN, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i+1]))
            {
                std::ostringstream oss;
                oss << "Unknown halfDomain value: '" << atts[i + 1];
                oss << "' while parsing InvLut1D. ";
                Throw(oss.str());
            }

            m_pInvLut->setInputHalfDomain(true);
        }

        if(0==Platform::Strcasecmp(ATTR_RAW_HALFS, atts[i]))
        {
            if (0 != Platform::Strcasecmp("true", atts[i+1]))
            {
                std::ostringstream oss;
                oss << "Unknown rawHalfs value: '" << atts[i + 1];
                oss << "' while parsing InvLut1D. ";
                Throw(oss.str());
            }

            m_pInvLut->setOutputRawHalfs(true);
        }

        if(0==Platform::Strcasecmp(ATTR_HUE_ADJUST, atts[i]))
        {
            if (0 != Platform::Strcasecmp("dw3", atts[i+1]))
            {
                std::ostringstream oss;
                oss << "Unknown hueAdjust value: '" << atts[i + 1];
                oss << "' while parsing InvLut1D. ";
                Throw(oss.str());
            }

            m_pInvLut->setHueAdjust(OpData::Lut1D::HUE_DW3);
        }

        i += 2;
    }
}

void InvLut1DElt::end()
{
    OpElt::end();
    m_pInvLut->validate();
}

OpData::ArrayBase * InvLut1DElt::updateDimension(const Dimensions & dims)
{
    if(dims.size()!=2)
    {
        return 0x0;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size()-1));
    const unsigned numColorComponents = dims[max];

    if(dims[1]!=3 && dims[1]!=1)
    {
        return 0x0;
    }

    OpData::Array* pArray = &m_pInvLut->getArray();
    pArray->resize(dims[0], numColorComponents);
    return pArray;
}

void InvLut1DElt::finalize(unsigned position)
{
    OpData::Array* pArray = &m_pInvLut->getArray();

    // Convert half bits to float values if needed
    if(m_pInvLut->isOutputRawHalfs())
    {
        const size_t maxValues = pArray->getNumValues();
        for(size_t i=0; i<maxValues; ++i)
        {
            pArray->getValues()[i] 
                = ConvertHalfBitsToFloat((unsigned short)pArray->getValues()[i]);
        }
    }

    if(pArray->getNumValues()!=position)
    {
        const unsigned numColorComponents = pArray->getNumColorComponents();
        const unsigned maxColorComponents = pArray->getMaxColorComponents();

        const unsigned dimensions = pArray->getLength();

        if( numColorComponents != 1 || position != dimensions )
        {
            std::ostringstream arg;
            arg << "Expected ";
            arg << dimensions;
            arg << "x" << numColorComponents;
            arg << " Array values, found ";
            arg << position << ". ";
            Throw(arg.str());
        }

        // Convert a 1D LUT to a 3by1D LUT
        // (duplicate values from the Red to the Green and Blue)
        const unsigned numLuts = maxColorComponents;

        // \todo Should improve Lut1DOp so that the copy is unnecessary.
        for(signed i=(dimensions-1); i>=0; --i)
        {
            for(unsigned j=0; j<numLuts; ++j)
            {
                pArray->getValues()[(i*numLuts)+j]
                    = pArray->getValues()[i];
            }
        }
    }

    pArray->validate();

    // At this point, we have created the complete Lut1D base class.
    // Now finish initializing as an InvLut1D.
    m_pInvLut->initializeFromLut1D();

    setCompleted(true);
}

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

