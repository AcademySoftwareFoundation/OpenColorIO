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

#include "CTFInvLut3DElt.h"
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
InvLut3DElt::InvLut3DElt()
    :   m_pInvLut(new OpData::InvLut3D)
{
}

InvLut3DElt::~InvLut3DElt()
{
    // Do not delete m_pInvLut (caller owns it now).
    m_pInvLut = 0x0;
}

void InvLut3DElt::start(const char **atts)
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
                    OpData::Lut3D::getInterpolation(atts[i + 1]));
            }
            catch (const std::exception& e)
            {
                Throw(e.what());
            }
        }

        i += 2;
    }
}

void InvLut3DElt::end()
{
    OpElt::end();
    m_pInvLut->validate();
}

OpData::ArrayBase * InvLut3DElt::updateDimension(const Dimensions & dims)
{
    if(dims.size()!=4)
    {
        return 0x0;
    }

    const size_t max = (dims.empty() ? 0 : (dims.size()-1));
    const unsigned numColorComponents = dims[max];

    if (dims[3] != 3 || dims[1] != dims[0] || dims[2] != dims[0])
    {
        return 0x0;
    }

    OpData::Array* pArray = &m_pInvLut->getArray();
    pArray->resize(dims[0], numColorComponents);
    return pArray;
}

void InvLut3DElt::finalize(unsigned position)
{
    OpData::Array* pArray = &m_pInvLut->getArray();

    if(pArray->getNumValues()!=position)
    {
        std::ostringstream arg;
        arg << "Expected ";
        arg << pArray->getLength() << "x";
        arg << pArray->getLength() << "x";
        arg << pArray->getLength() << "x";
        arg << pArray->getNumColorComponents();
        arg << " Array values, found ";
        arg << position << ". ";
        Throw(arg.str());
    }

    pArray->validate();

    // At this point, we have created the complete Lut3D base class.
    // Now finish initializing as an InvLut3D.
    m_pInvLut->initializeFromLut3D();

    setCompleted(true);
}

} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

