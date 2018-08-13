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


#ifndef INCLUDED_OCIO_CTF_INVLUT3DELT_H
#define INCLUDED_OCIO_CTF_INVLUT3DELT_H

#include "CTFOpElt.h"
#include "CTFArrayMgt.h"
#include "CTFIndexMapMgt.h"

#include "../opdata/OpDataInvLut3D.h"


OCIO_NAMESPACE_ENTER
{
    // Private namespace to the CTF sub-directory
    namespace CTF
    {
        // Private namespace for the xml reader utils
        namespace Reader
        {
            // Class for Color::InvLut1D
            class InvLut3DElt : public OpElt, public ArrayMgt
            {
            public:
                // Constructor
                InvLut3DElt();
                // Destructor
                ~InvLut3DElt();

                // Start the parsing of the element
                virtual void start(const char **atts);

                // End the parsing of the element
                virtual void end();

                // Get the associated Color::Op
                // Return The Op
                OpData::OpData * getOp() const { return m_pInvLut; }

                // Get the associated Color::InvLut1DOp
                // Return The Color::invLut1DOp
                OpData::InvLut3D * getLut() const { return m_pInvLut; }

                // Update the array dimensions
                // - dims are the array dimensions
                // returns the resized array
                virtual OpData::ArrayBase * updateDimension(const Dimensions & dims);

                // Finalize the array data origanization
                // - position is the position of the last value found
                virtual void finalize(unsigned position);

            protected:
                // Convert the interpolation string to its enum's value
                // - str is the interpolation algorithm name
                // Return the enum's value
                Interpolation getInterpolation(const std::string & str);
        
            private:
                OpData::InvLut3D * m_pInvLut;
            };

        } // exit Reader namespace
    } // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

#endif

