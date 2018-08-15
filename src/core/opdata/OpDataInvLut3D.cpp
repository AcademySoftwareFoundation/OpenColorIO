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

#include "../Platform.h"
#include "../BitDepthUtils.h"
#include "../MathUtils.h"

#include "OpDataInvLut3D.h"

OCIO_NAMESPACE_ENTER
{
namespace OpData
{

InvLut3D::InvLut3D()
    :   Lut3D(2)
    ,   m_invStyle(InvLut3D::FAST)
{
}

InvLut3D::InvLut3D(const Lut3D & fwdLut3D)
    :   Lut3D(fwdLut3D)
    ,   m_invStyle(InvLut3D::FAST)
{
    // Swap input/output bitdepths
    // Note: Call for Op::set*BitDepth() in order to only swap the bitdepths
    //       without any rescaling
    const BitDepth in = getInputBitDepth();
    OpData::setInputBitDepth(getOutputBitDepth());
    OpData::setOutputBitDepth(in);

    initializeFromLut3D();
}

void InvLut3D::initializeFromLut3D()
{
    // This routine is to be called (e.g. in XML reader) once the base forward Lut3D
    // has been created and then sets up what is needed for the invLut3D.

    // TODO: Check LUT for invertibility.
}

InvLut3D::~InvLut3D()
{
}

OpData * InvLut3D::clone(CloneType) const
{
    return new InvLut3D(*this);
}

void InvLut3D::inverse(OpDataVec & ops) const
{
    // OpData validation will be done when transform finalize is called.

    std::auto_ptr<Lut3D> invOp(new Lut3D(*this));

    // Swap input/output bitdepths
    // Note: Call for Op::set*BitDepth() in order to only swap 
    //       the bitdepths without any rescaling

    invOp->OpData::setInputBitDepth(getOutputBitDepth());
    invOp->OpData::setOutputBitDepth(getInputBitDepth());
            
    // Validation is currently deferred until the finalize call.
            
    ops.append(invOp.release());
}

const std::string& InvLut3D::getOpTypeName() const
{
    static const std::string type("Inverse LUT 3D");
    return type;
}

void InvLut3D::setInputBitDepth(BitDepth in)
{
    // Recall that our array is for the LUT to be inverted, so this method
    // is similar to set OUT depth for the original LUT.

    // Scale factor is max_new_depth/max_old_depth
    const float scaleFactor
        = GetBitDepthMaxValue(in)
            / GetBitDepthMaxValue(getInputBitDepth());

    // Call parent to set the intputBitdepth
    OpData::setInputBitDepth(in);

    // Scale array by scaleFactor, 
    // don't scale if scaleFactor = 1.0f
    if (scaleFactor != 1.0f)
    {
        Array::Values & arrayVals = getArray().getValues();
        const size_t size = arrayVals.size();

        for (unsigned i = 0; i < size; i++)
        {
            arrayVals[i] *= scaleFactor;
        }
    }
}

void InvLut3D::setOutputBitDepth(BitDepth out)
{
    // (Analogous to set IN depth on the original LUT.)

    // Note: bypass Lut1DOp::setOutputBitDepth() override
    OpData::setOutputBitDepth(out);
}

void InvLut3D::setInvStyle(InvStyle style)
{
    m_invStyle = style;
}

}
}
OCIO_NAMESPACE_EXIT



#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"

namespace
{
    const char uid[] = "uid";
    const char name[] = "name";
    const OCIO::OpData::Descriptions desc;
};

void checkInverse_bitDepths_domain(
    const OCIO::BitDepth refInputBitDepth,
    const OCIO::BitDepth refOutputBitDepth,
    const OCIO::Interpolation refInterpolationAlgo,
    const OCIO::BitDepth expectedInvInputBitDepth,
    const OCIO::BitDepth expectedInvOutputBitDepth,
    const OCIO::Interpolation expectedInvInterpolationAlgo)
{
    OCIO::OpData::Lut3D refLut3d(
        refInputBitDepth, refOutputBitDepth,
        uid, name, desc,
        refInterpolationAlgo, 5);

    // Get inverse of reference lut1d operation 
    OCIO::OpData::OpDataVec invOps;
    OIIO_CHECK_NO_THROW(refLut3d.inverse(invOps));
    OIIO_CHECK_EQUAL(invOps.size(), 1u);

    const OCIO::OpData::InvLut3D * invLut3d
        = dynamic_cast<const OCIO::OpData::InvLut3D*>(invOps.get(0));
    OIIO_CHECK_ASSERT(invLut3d);

    OIIO_CHECK_EQUAL(invLut3d->getInputBitDepth(),
        expectedInvInputBitDepth);

    OIIO_CHECK_EQUAL(invLut3d->getOutputBitDepth(),
        expectedInvOutputBitDepth);

    OIIO_CHECK_EQUAL(invLut3d->getInterpolation(),
        expectedInvInterpolationAlgo);
}

OIIO_ADD_TEST(OpDataInvLut3D, inverse_bitDepth_domain)
{
    checkInverse_bitDepths_domain(
        // reference
        OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10,
        OCIO::INTERP_LINEAR,
        // expected
        OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT8,
        OCIO::INTERP_LINEAR);

    checkInverse_bitDepths_domain(
        // reference
        OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT10,
        OCIO::INTERP_TETRAHEDRAL,
        // expected
        OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_F16,
        OCIO::INTERP_TETRAHEDRAL);
}


OIIO_ADD_TEST(OpDataInvLut3D, isInverse_test)
{
    // Create forward LUT.
    OCIO::OpData::Lut3D L1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10,
        uid, name, desc, OCIO::INTERP_LINEAR, 5);
    // Make it not an identity.
    OCIO::OpData::Array& array = L1.getArray();
    OCIO::OpData::Array::Values& values = array.getValues();
    values[0] = 20.f;
    OIIO_CHECK_ASSERT(!L1.isIdentity());

    // Create an inverse LUT with same basics.
    OCIO::OpData::InvLut3D L2(L1);
    L2.setId("foo");
    OIIO_CHECK_ASSERT(!(L1 == L2));

    // Check isInverse.
    OIIO_CHECK_ASSERT(L1.isInverse(L2));
    OIIO_CHECK_ASSERT(L2.isInverse(L1));

    // TODO: This should pass, since the arrays are actually the same if you
    //       normalize for the scaling difference.
    L1.setOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    //   OIIO_CHECK_ASSERT( L1.isInverse(L2) );
    //   OIIO_CHECK_ASSERT( L2.isInverse(L1) );

    // Catch the situation where the arrays are the same even though the
    // bit-depths don't match and hence the arrays effectively aren't the same.
    L1.setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);  // restore original
    OIIO_CHECK_ASSERT(L1.isInverse(L2));
    L1.OpData::setOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_ASSERT(!L1.isInverse(L2));
    OIIO_CHECK_ASSERT(!L2.isInverse(L1));
}


#endif