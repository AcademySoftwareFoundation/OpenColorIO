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


#ifndef INCLUDED_OCIO_PRIVATE_TYPES_H
#define INCLUDED_OCIO_PRIVATE_TYPES_H

#include <OpenColorIO/OpenColorIO.h>

#include <map>
#include <set>
#include <vector>

OCIO_NAMESPACE_ENTER
{
    // Stl types of OCIO classes
    typedef std::map<std::string, std::string> StringMap;
    typedef std::map<std::string, bool> StringBoolMap;
    typedef std::vector<std::string> StringVec;
    typedef std::set<std::string> StringSet;
    
    typedef std::vector<ConstTransformRcPtr> ConstTransformVec;
    typedef std::vector<LookRcPtr> LookVec;
    
    typedef std::vector<TransformDirection> TransformDirectionVec;


    // Specify the method to use when inverting a Lut1D or Lut3D.  The EXACT
    // method is slower, and only available on the CPU, but it calculates an
    // exact inverse.  The exact inverse is based on the use of LINEAR forward
    // interpolation for Lut1D and TETRAHEDRAL forward interpolation for Lut3D.
    // The FAST method bakes the inverse into another forward LUT (using the
    // exact method).  For Lut1D, a half-domain LUT is used and so this is
    // quite accurate even for scene-linear values, but for Lut3D the baked
    // version is more of an approximation.  The DEFAULT is the FAST method
    // since it is the only one available on both CPU and GPU.  The BEST is
    // the EXACT method.
    //
    enum LutInversionQuality
    {
        LUT_INVERSION_EXACT = 0,
        LUT_INVERSION_FAST,

        LUT_INVERSION_DEFAULT = 254,
        LUT_INVERSION_BEST = 255
    };

}
OCIO_NAMESPACE_EXIT

#endif
