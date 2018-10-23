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


#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"

OCIO_NAMESPACE_ENTER
{
    static const std::string errBDNotSupported("Bit depth is not supported: ");

    float GetBitDepthMaxValue(BitDepth in)
    {
        switch(in)
        {
            case BIT_DEPTH_UINT8:
                return 255.0f;

            case BIT_DEPTH_UINT10:
                return 1023.0f;

            case BIT_DEPTH_UINT12:
                return 4095.0f;

            case BIT_DEPTH_UINT16:
                return 65535.0f;

            case BIT_DEPTH_F16:
            case BIT_DEPTH_F32:
                return 1.0f;

            case BIT_DEPTH_UNKNOWN:
            case BIT_DEPTH_UINT14:
            case BIT_DEPTH_UINT32:
            default:
            {
                std::string err(errBDNotSupported);
                err += BitDepthToString(in);
                throw Exception(err.c_str());
                break;
            }
        }
    }


    bool IsFloatBitDepth(BitDepth in)
    {
        switch(in)
        {
            case BIT_DEPTH_UINT8:
            case BIT_DEPTH_UINT10:
            case BIT_DEPTH_UINT12:
            case BIT_DEPTH_UINT14:
            case BIT_DEPTH_UINT16:
            case BIT_DEPTH_UINT32:
                return false;

            case BIT_DEPTH_F16:
            case BIT_DEPTH_F32:
                return true;

            case BIT_DEPTH_UNKNOWN:
            default:
            {
                std::string err(errBDNotSupported);
                err += BitDepthToString(in);
                throw Exception(err.c_str());
                break;
            }
        }
    }
}
OCIO_NAMESPACE_EXIT



///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

OCIO_NAMESPACE_USING

#include "unittest.h"

OIIO_ADD_TEST(BitDepthUtils, GetBitDepthMaxValue)
{
    OIIO_CHECK_EQUAL(GetBitDepthMaxValue(BIT_DEPTH_UINT8), 255.0f);
    OIIO_CHECK_EQUAL(GetBitDepthMaxValue(BIT_DEPTH_UINT16), 65535.0f);

    OIIO_CHECK_EQUAL(GetBitDepthMaxValue(BIT_DEPTH_F16), 1.0f);
    OIIO_CHECK_EQUAL(GetBitDepthMaxValue(BIT_DEPTH_F32), 1.0f);

    OIIO_CHECK_THROW_WHAT(
        GetBitDepthMaxValue((BitDepth)42), Exception, "not supported");
}

OIIO_ADD_TEST(BitDepthUtils, IsFloatBitDepth)
{
    OIIO_CHECK_ASSERT(!IsFloatBitDepth(BIT_DEPTH_UINT8));
    OIIO_CHECK_ASSERT(!IsFloatBitDepth(BIT_DEPTH_UINT10));
    OIIO_CHECK_ASSERT(!IsFloatBitDepth(BIT_DEPTH_UINT12));
    OIIO_CHECK_ASSERT(!IsFloatBitDepth(BIT_DEPTH_UINT14));
    OIIO_CHECK_ASSERT(!IsFloatBitDepth(BIT_DEPTH_UINT16));
    OIIO_CHECK_ASSERT(!IsFloatBitDepth(BIT_DEPTH_UINT32));
    
    OIIO_CHECK_ASSERT(IsFloatBitDepth(BIT_DEPTH_F16));
    OIIO_CHECK_ASSERT(IsFloatBitDepth(BIT_DEPTH_F32));

    OIIO_CHECK_THROW_WHAT(
        IsFloatBitDepth((BitDepth)42), Exception, "not supported");
}

#endif