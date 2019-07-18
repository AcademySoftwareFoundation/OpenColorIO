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

namespace
{

static const std::string errBDNotSupported("Bit depth is not supported: ");

};

OCIO_NAMESPACE_ENTER
{
    float GetBitDepthMaxValue(BitDepth in)
    {
        switch(in)
        {
            case BIT_DEPTH_UINT8:
                return (float)BitDepthInfo<BIT_DEPTH_UINT8>::maxValue;
            case BIT_DEPTH_UINT10:
                return (float)BitDepthInfo<BIT_DEPTH_UINT10>::maxValue;
            case BIT_DEPTH_UINT12:
                return (float)BitDepthInfo<BIT_DEPTH_UINT12>::maxValue;
            case BIT_DEPTH_UINT16:
                return (float)BitDepthInfo<BIT_DEPTH_UINT16>::maxValue;
            case BIT_DEPTH_F16:
                return (float)BitDepthInfo<BIT_DEPTH_F16>::maxValue;
            case BIT_DEPTH_F32:
                return (float)BitDepthInfo<BIT_DEPTH_F32>::maxValue;

            case BIT_DEPTH_UNKNOWN:
            case BIT_DEPTH_UINT14:
            case BIT_DEPTH_UINT32:
            default:
            {
                std::string err(errBDNotSupported);
                err += BitDepthToString(in);
                throw Exception(err.c_str());
            }
        }
    }


    bool IsFloatBitDepth(BitDepth in)
    {
        switch(in)
        {
            case BIT_DEPTH_UINT8:
                return BitDepthInfo<BIT_DEPTH_UINT8>::isFloat;
            case BIT_DEPTH_UINT10:
                return BitDepthInfo<BIT_DEPTH_UINT10>::isFloat;
            case BIT_DEPTH_UINT12:
                return BitDepthInfo<BIT_DEPTH_UINT12>::isFloat;
            case BIT_DEPTH_UINT16:
                return BitDepthInfo<BIT_DEPTH_UINT16>::isFloat;
            case BIT_DEPTH_F16:
                return BitDepthInfo<BIT_DEPTH_F16>::isFloat;
            case BIT_DEPTH_F32:
                return BitDepthInfo<BIT_DEPTH_F32>::isFloat;

            case BIT_DEPTH_UNKNOWN:
            case BIT_DEPTH_UINT14:
            case BIT_DEPTH_UINT32:
            default:
            {
                std::string err(errBDNotSupported);
                err += BitDepthToString(in);
                throw Exception(err.c_str());
            }
        }
    }
}
OCIO_NAMESPACE_EXIT



///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "UnitTest.h"

OCIO_ADD_TEST(BitDepthUtils, GetBitDepthMaxValue)
{
    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT8), 255.0f);
    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT16), 65535.0f);

    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F16), 1.0f);
    OCIO_CHECK_EQUAL(OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32), 1.0f);

    OCIO_CHECK_THROW_WHAT(
        OCIO::GetBitDepthMaxValue((OCIO::BitDepth)42), OCIO::Exception, "not supported");
}

OCIO_ADD_TEST(BitDepthUtils, IsFloatBitDepth)
{
    OCIO_CHECK_ASSERT(!OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT8));
    OCIO_CHECK_ASSERT(!OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT10));
    OCIO_CHECK_ASSERT(!OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT12));
    OCIO_CHECK_ASSERT(!OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT16));
    
    OCIO_CHECK_ASSERT(OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_F16));
    OCIO_CHECK_ASSERT(OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_F32));

    OCIO_CHECK_THROW_WHAT(
        OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT14), OCIO::Exception, "not supported");

    OCIO_CHECK_THROW_WHAT(
        OCIO::IsFloatBitDepth(OCIO::BIT_DEPTH_UINT32), OCIO::Exception, "not supported");

    OCIO_CHECK_THROW_WHAT(
        OCIO::IsFloatBitDepth((OCIO::BitDepth)42), OCIO::Exception, "not supported");
}

#endif