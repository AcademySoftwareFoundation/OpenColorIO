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

#ifndef INCLUDED_OCIO_FILEFORMATS_CTF_CTFREADERUTILS_H
#define INCLUDED_OCIO_FILEFORMATS_CTF_CTFREADERUTILS_H

#include <OpenColorIO/OpenColorIO.h>

OCIO_NAMESPACE_ENTER
{
    
Interpolation GetInterpolation1D(const char * str);
Interpolation GetInterpolation3D(const char * str);

static constexpr const char * TAG_PROCESS_LIST = "ProcessList";
static constexpr const char * TAG_INPUT_DESCRIPTOR = "InputDescriptor";
static constexpr const char * TAG_OUTPUT_DESCRIPTOR = "OutputDescriptor";
static constexpr const char * TAG_ARRAY = "Array";
static constexpr const char * TAG_INDEX_MAP = "IndexMap";
static constexpr const char * TAG_INFO = "Info";
static constexpr const char * TAG_CDL = "ASC_CDL";
static constexpr const char * TAG_MATRIX = "Matrix";
static constexpr const char * TAG_LUT1D = "LUT1D";
static constexpr const char * TAG_INVLUT1D = "InverseLUT1D";
static constexpr const char * TAG_LUT3D = "LUT3D";
static constexpr const char * TAG_INVLUT3D = "InverseLUT3D";
static constexpr const char * TAG_RANGE = "Range";
static constexpr const char * TAG_MIN_IN_VALUE = "minInValue";
static constexpr const char * TAG_MAX_IN_VALUE = "maxInValue";
static constexpr const char * TAG_MIN_OUT_VALUE = "minOutValue";
static constexpr const char * TAG_MAX_OUT_VALUE = "maxOutValue";

static constexpr const char * ATTR_BITDEPTH_IN = "inBitDepth";
static constexpr const char * ATTR_BITDEPTH_OUT = "outBitDepth";
static constexpr const char * ATTR_CDL_STYLE = "style";
static constexpr const char * ATTR_COMP_CLF_VERSION = "compCLFversion";
static constexpr const char * ATTR_DIMENSION = "dim";
static constexpr const char * ATTR_HALF_DOMAIN = "halfDomain";
static constexpr const char * ATTR_HUE_ADJUST = "hueAdjust";
static constexpr const char * ATTR_INTERPOLATION = "interpolation";
static constexpr const char * ATTR_INVERSE_OF = "inverseOf";
static constexpr const char * ATTR_NAME = "name";
static constexpr const char * ATTR_RANGE_STYLE = "style";
static constexpr const char * ATTR_RAW_HALFS = "rawHalfs";
static constexpr const char * ATTR_VERSION = "version";

}
OCIO_NAMESPACE_EXIT

#endif