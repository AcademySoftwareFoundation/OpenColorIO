// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


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
