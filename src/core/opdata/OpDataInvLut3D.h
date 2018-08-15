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

// This class facilitates inversion of a 3d LUT.
//
// The class design inherits from the Lut3DOp class. The LUT data stored is the
// same as the forward LUT however it may be adjusted if necessary.
//
// Please be aware that although the in/out bit-depths are reversed from the
// forward LUT, the scaling of the LUT entries is not changed.

#ifndef INCLUDED_OCIO_OPDATA_INVLUT3D_H
#define INCLUDED_OCIO_OPDATA_INVLUT3D_H

#include <OpenColorIO/OpenColorIO.h>

#include "OpDataLut3D.h"


OCIO_NAMESPACE_ENTER
{
// Private namespace to the OpData sub-directory
namespace OpData
{
// The class represents an invert 3D LUT process node
class InvLut3D : public Lut3D
{
public:
    // Enumeration of the inverse 3D LUT styles.
    enum InvStyle
    {
        EXACT = 0,  // Exact, but slow, inverse processing
        FAST        // Fast, but approximate, inverse processing
    };

    // Constructor
    InvLut3D();

    // Constructor
    //  - fwdLut3DOp is the original 3D LUT forward operation
    InvLut3D(const Lut3D & fwdLut3D);

    // Used by XML reader.  After initializing members as a Lut3D,
    // this method is used to finish creating an InvLut3D.
    void initializeFromLut3D();

    // Destructor
    virtual ~InvLut3D();

    inline OpType getOpType() const { return OpData::InvLut3DType; }

    const std::string& getOpTypeName() const;

    OpData * clone(CloneType type) const;

    // Get the Op inverse
    virtual void inverse(OpDataVec & ops) const;

    inline InvStyle getInvStyle() const { return m_invStyle; }

    void setInvStyle(InvStyle style);

    void setInputBitDepth(BitDepth in);

    void setOutputBitDepth(BitDepth out);

private:
    // Inverse computation style
    InvStyle m_invStyle;
};

typedef OCIO_SHARED_PTR<InvLut3D> OpDataInvLut3DRcPtr;

}
}
OCIO_NAMESPACE_EXIT


#endif
