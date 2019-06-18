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


#ifndef INCLUDED_OCIO_OPTOOLS_H
#define INCLUDED_OCIO_OPTOOLS_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


OCIO_NAMESPACE_ENTER
{

void EvalTransform(const float * in, float * out,
                   long numPixels,
                   OpRcPtrVec & ops);

const char * GetInvQualityName(LutInversionQuality invStyle);

// Allow us to temporarily manipulate the inversion quality without
// cloning the object.
template <class LutType>
class LutStyleGuard
{
public:
    LutStyleGuard() = delete;
    LutStyleGuard(const LutStyleGuard &) = delete;
    LutStyleGuard & operator=(const LutStyleGuard &) = delete;
    LutStyleGuard & operator=(LutStyleGuard &&) = delete;

    LutStyleGuard(const LutType & lut)
        : m_prevQuality(lut.getInversionQuality())
        , m_lut(const_cast<LutType &>(lut))
    {
        m_lut.setInversionQuality(LUT_INVERSION_BEST);
    }

    ~LutStyleGuard()
    {
        if (m_prevQuality != LUT_INVERSION_BEST)
        {
            m_lut.setInversionQuality(m_prevQuality);
        }
    }

private:
    LutInversionQuality m_prevQuality;
    LutType &           m_lut;
};


}
OCIO_NAMESPACE_EXIT

#endif
