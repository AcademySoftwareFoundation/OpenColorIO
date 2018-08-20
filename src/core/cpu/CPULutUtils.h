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


#ifndef INCLUDED_OCIO_CPU_INVLUTUTILS
#define INCLUDED_OCIO_CPU_INVLUTUTILS


#include <OpenColorIO/OpenColorIO.h>


#include "../opdata/OpDataLut1D.h"
#include "../opdata/OpDataInvLut3D.h"



OCIO_NAMESPACE_ENTER
{
    namespace InvLutUtil
    {
        // Make a Lut1DOp to be used for the FAST renderer style.
        // - lut is the invLut1DOp
        // - forGPU set true if the Lut1D is intended for the GPU
        // Return the compute 1d LUT
        OpData::OpDataLut1DRcPtr makeFastLut1D(const OpData::OpDataLut1DRcPtr & lut, bool forGPU);

        // Make a Lut3DOp to be used for the FAST renderer style.
        // - lut is the invLut3DOp
        // Return the compute 3d LUT
        OpData::OpDataLut3DRcPtr makeFastLut3D(const OpData::OpDataInvLut3DRcPtr & lut);
    }
}
OCIO_NAMESPACE_EXIT


#endif