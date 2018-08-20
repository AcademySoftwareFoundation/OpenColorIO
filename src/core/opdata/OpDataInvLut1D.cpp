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

#include "OpDataInvLut1D.h"


namespace
{
// Names of each inv lut1d op styles
static const char kEXACT[] = "exact";
static const char kFAST[]  = "fast";
}

OCIO_NAMESPACE_ENTER
{
namespace OpData
{
InvLut1D::InvStyle InvLut1D::GetInvStyle(const char * str)
{
    if(str && *str)
    {
        if(0==Platform::Strcasecmp(str, kEXACT))
        {
            return EXACT;
        }
        else if(0==Platform::Strcasecmp(str, kFAST))
        {
            return FAST;
        }

        std::string err("Unknown LUT 1D inverse style: ");
        err += str;
        throw Exception(err.c_str());
    }

    throw Exception("Invalid LUT 1D inverse style");
    return EXACT;
}

const char * InvLut1D::GetInvStyleName(InvLut1D::InvStyle invStyle)
{
    switch(invStyle)
    {
        case EXACT:
        {
            return kEXACT;
            break;
        }
        case FAST:
        {
            return kFAST;
            break;
        }
    }

    throw Exception("Invalid LUT 1D inverse style");
    return "";
}

InvLut1D::InvLut1D()
    :   Lut1D(2)
    ,   m_invStyle(InvLut1D::FAST)
    ,   m_origInDepth(BIT_DEPTH_UNKNOWN)
{
}

InvLut1D::InvLut1D(BitDepth inBitDepth, BitDepth outBitDepth, HalfFlags halfFlags)
    :   Lut1D(inBitDepth, outBitDepth, halfFlags)
    ,   m_invStyle(InvLut1D::FAST)
    ,   m_origInDepth(BIT_DEPTH_UNKNOWN)
{
    m_origInDepth = getInputBitDepth();
}

InvLut1D::InvLut1D(const Lut1D & fwdLut1D)
    :   Lut1D(fwdLut1D)
    ,   m_invStyle(InvLut1D::FAST)
    ,   m_origInDepth(BIT_DEPTH_UNKNOWN)
{
    // Swap input/output bitdepths
    // Note: Call for Op::set*BitDepth() in order to only swap the bitdepths
    //       without any rescaling
    const BitDepth in = getInputBitDepth();
    OpData::setInputBitDepth(getOutputBitDepth());
    OpData::setOutputBitDepth(in);

    initializeFromLut1D();
}

void InvLut1D::initializeFromLut1D()
{
    // This routine is to be called (e.g. in XML reader) once the base forward Lut1D
    // has been created and then sets up what is needed for the invLut1D.

    // Note that if the original LUT had a half domain, the invLut needs to as
    // well so that the appropriate evaluation algorithm is called.

    m_origInDepth = getInputBitDepth();

    prepareArray();
}

InvLut1D::~InvLut1D()
{
}

OpData * InvLut1D::clone(CloneType) const
{
    return new InvLut1D(*this);
}

void InvLut1D::inverse(OpDataVec & ops) const
{
    std::auto_ptr<Lut1D> invOp(new Lut1D(*this));

    // Swap input/output bitdepths
    // Note: Call for Op::set*BitDepth() in order to only swap 
    //       the bitdepths without any rescaling

    invOp->OpData::setInputBitDepth(getOutputBitDepth());
    invOp->OpData::setOutputBitDepth(getInputBitDepth());

    // Validation is currently deferred until the finalize call.

    ops.append(invOp.release());
}

const std::string& InvLut1D::getOpTypeName() const
{
    static const std::string type("Inverse LUT 1D");
    return type;
}

void InvLut1D::setInputBitDepth(BitDepth in)
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

void InvLut1D::setOutputBitDepth(BitDepth out)
{
    // (Analogous to set IN depth on the original LUT.)

    // Note: bypass Lut1DOp::setOutputBitDepth() override
    OpData::setOutputBitDepth(out);
}

void InvLut1D::setInvStyle(InvStyle style)
{
    m_invStyle = style;
}

bool InvLut1D::hasExtendedDomain() const
{
    // The forward LUT is allowed to have entries outside the outDepth (e.g. a 10i
    // LUT is allowed to have values on [-20,1050] if it wants).  This is called
    // an extended range LUT and helps maximize accuracy by allowing clamping to
    // happen (if necessary) after the interpolation.  The implication is
    // that the inverse LUT needs to evaluate over an extended domain.  Since
    // this potentially requires a slower rendering method for the Fast style,
    // this method allows the renderers to determine if this is necessary.

    // Note that it is the range (output) of the forward LUT that determines the
    // need for an extended domain on the inverse LUT.  Whether the forward LUT
    // has a half domain does not matter.  E.g., a Lustre float-conversion LUT
    // has a half domain but outputs integers within [0,65535] so the inverse 
    // actually wants a normal 16i domain.

    const unsigned length = getArray().getLength();
    const unsigned maxChannels = getArray().getMaxColorComponents();
    const unsigned activeChannels = getArray().getNumColorComponents();
    const Array::Values& values = getArray().getValues();

    // As noted elsewhere, the InBitDepth describes the scaling of the LUT entries.
    const float normalMin = 0.0f;
    const float normalMax = GetBitDepthMaxValue(getInputBitDepth());

    unsigned minInd = 0;
    unsigned maxInd = length - 1;
    if(isInputHalfDomain())
    {
        minInd = 64511u;  // last value before -inf
        maxInd = 31743u;  // last value before +inf
    }

    // prepareArray has made the LUT either non-increasing or non-decreasing,
    // so the min and max values will be either the first or last LUT entries.
    for(unsigned c = 0; c < activeChannels; ++c)
    {
        if(m_componentProperties[c].isIncreasing)
        {
            if( values[ minInd * maxChannels + c] < normalMin || 
                values[ maxInd * maxChannels + c] > normalMax )
            {
                return true;
            }
        }
        else
        {
            if( values[ minInd * maxChannels + c] > normalMax || 
                values[ maxInd * maxChannels + c] < normalMin )
            {
                return true;
            }
        }
    }

    return false;
}

// NB: The half domain includes pos/neg infinity and NaNs.  
// The prepareArray makes the LUT monotonic to ensure a unique inverse and
// determines an effective domain to handle flat spots at the ends nicely.
// It's not clear how the NaN part of the domain should be included in the
// monotonicity constraints, furthermore there are 2048 NaNs that could each
// potentially have different values.  For now, the inversion algorithm and
// the pre-processing ignore the NaN part of the domain.

void InvLut1D::prepareArray()
{
    // Note: The data allocated for the array is length*getMaxColorComponents()
    const unsigned length = getArray().getLength();
    const unsigned maxChannels = getArray().getMaxColorComponents();
    const unsigned activeChannels = getArray().getNumColorComponents();
    Array::Values& values = getArray().getValues();

    for(unsigned c = 0; c < activeChannels; ++c)
    {
        // Determine if the LUT is overall increasing or decreasing.
        // The heuristic used is to compare first and last entries.
        // (Note flat LUTs (arbitrarily) have isIncreasing == false.)
        unsigned lowInd = c;
        unsigned highInd = (length-1) * maxChannels + c;
        if(isInputHalfDomain())
        {
            // For half-domain LUTs, I am concerned that customer LUTs may not
            // correctly populate the whole domain, so using -HALF_MAX and +HALF_MAX
            // could potentially give unreliable results.  Just using 0 and 1 for now.
            lowInd  = 0u * maxChannels + c;      // 0.0
            highInd = 15360u * maxChannels + c;  // 15360 == 1.0
        }

        {
            m_componentProperties[c].isIncreasing
                = ( values[lowInd] < values[highInd] );
        }

        // Flatten reversals
        // (If the LUT has a reversal, there is not a unique inverse.
        //  Furthermore we require sorted values for the exact eval algorithm.)
        {
            bool isIncreasing = m_componentProperties[c].isIncreasing;

            if( ! isInputHalfDomain() )
            {
                float prevValue = values[c];
                for(unsigned idx = c + maxChannels; 
                        idx < length * maxChannels; 
                        idx += maxChannels)
                {
                    if( isIncreasing != (values[idx] > prevValue) )
                    {
                        values[idx] = prevValue;
                    }
                    else
                    {
                        prevValue = values[idx];
                    }
                }
            }
            else
            {
                // Do positive numbers.
                unsigned startInd = 0u * maxChannels + c; // 0 == +zero
                unsigned endInd = 31744u * maxChannels;   // 31744 == +infinity
                float prevValue = values[startInd];
                for(unsigned idx = startInd + maxChannels; 
                        idx <= endInd; 
                        idx += maxChannels)
                {
                    if( isIncreasing != (values[idx] > prevValue) ) 
                    {
                        values[idx] = prevValue;
                    }
                    else 
                    {
                        prevValue = values[idx];
                    }
                }

                // Do negative numbers.
                isIncreasing = ! isIncreasing;
                startInd = 32768u * maxChannels + c;      // 32768 == -zero
                endInd = 64512u * maxChannels;            // 64512 == -infinity
                prevValue = values[c];  // prev value for -0 is +0 (disallow overlaps)
                for(unsigned idx = startInd; idx <= endInd; idx += maxChannels)
                {
                    if( isIncreasing != (values[idx] > prevValue) )
                    {
                        values[idx] = prevValue;
                    }
                    else
                    {
                        prevValue = values[idx];
                    }
                }
            }
        }

        // Determine effective domain from the starting/ending flat spots
        // (If the LUT begins or ends with a flat spot, the inverse should be the
        //  value nearest the center of the LUT.)
        // For constant LUTs, the end domain == start domain == 0.
        {
            if(!isInputHalfDomain())
            {
                unsigned endDomain = length - 1;
                const float endValue = values[endDomain * maxChannels + c];
                while( endDomain > 0
                        && values[ (endDomain-1) * maxChannels + c] == endValue )
                {
                    --endDomain;
                }

                unsigned startDomain = 0;
                const float startValue = values[startDomain * maxChannels + c];
                // Note that this works for both increasing and decreasing LUTs
                // since there is no reqmt that startValue < endValue.
                while( startDomain < endDomain
                        &&  values[ (startDomain+1) * maxChannels + c] == startValue )
                {
                    ++startDomain;
                }

                m_componentProperties[c].startDomain = startDomain;
                m_componentProperties[c].endDomain = endDomain;
            }
            else
            {
                // Question: Should the value for infinity be considered for interpolation?
                // The advantage is that in theory, if infinity is mapped to some value
                // by the forward LUT, you could restore that value to infinity in the inverse.
                // This does seem to work in EXACT mode (e.g. CPURendererInvLUT1DHalf_fclut unit test).
                // The problem is that in FAST mode, there are Infs in the fast LUT
                // and these seem to make the value for both inf and 65504 a NaN.
                // Limiting the effective domain allows 65504 to invert correctly.
                unsigned endDomain = 31743u;    // +65504 = largest half value < inf
                const float endValue = values[endDomain * maxChannels + c];
                while( endDomain > 0
                        && values[ (endDomain-1) * maxChannels + c] == endValue )
                {
                    --endDomain;
                }

                unsigned startDomain = 0;       // positive zero
                const float startValue = values[startDomain * maxChannels + c];
                // Note that this works for both increasing and decreasing LUTs
                // since there is no reqmt that startValue < endValue.
                while( startDomain < endDomain
                        &&  values[ (startDomain+1) * maxChannels + c] == startValue )
                {
                    ++startDomain;
                }

                m_componentProperties[c].startDomain = startDomain;
                m_componentProperties[c].endDomain = endDomain;

                // Negative half of domain has its own start/end.
                unsigned negEndDomain = 64511u;  // -65504 = last value before neg inf
                const float negEndValue = values[negEndDomain * maxChannels + c];
                while( negEndDomain > 32768u     // negative zero
                        && values[ (negEndDomain-1) * maxChannels + c] == negEndValue )
                {
                    --negEndDomain;
                }

                unsigned negStartDomain = 32768u; // negative zero
                const float negStartValue = values[negStartDomain * maxChannels + c];
                while( negStartDomain < negEndDomain
                        && values[ (negStartDomain+1) * maxChannels + c] == negStartValue )
                {
                    ++negStartDomain;
                }

                m_componentProperties[c].negStartDomain = negStartDomain;
                m_componentProperties[c].negEndDomain = negEndDomain;
            }
        }
    }

    if(activeChannels == 1)
    {
        m_componentProperties[2] = m_componentProperties[1] = m_componentProperties[0];
    }
}
}
}
OCIO_NAMESPACE_EXIT



#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"
#include "../MathUtils.h"

namespace
{
    const char uid[] = "uid";
    const char name[] = "name";
    const OCIO::OpData::Descriptions desc;
};

// Validate the input and output bitdepths and half domain
void checkInverse_bitDepths_domain(
  const OCIO::BitDepth refInputBitDepth,
  const OCIO::BitDepth refOutputBitDepth,
  const OCIO::Interpolation refInterpolationAlgo,
  const OCIO::OpData::Lut1D::HalfFlags refHalfFlags,
  const OCIO::BitDepth expectedInvInputBitDepth,
  const OCIO::BitDepth expectedInvOutputBitDepth,
  const OCIO::Interpolation expectedInvInterpolationAlgo,
  const OCIO::OpData::Lut1D::HalfFlags expectedInvHalfFlags)
{
    OCIO::OpData::Lut1D refLut1d(
        refInputBitDepth, refOutputBitDepth, 
        uid, name, desc,
        refInterpolationAlgo,
        refHalfFlags);

    // Get inverse of reference lut1d operation 
    OCIO::OpData::OpDataVec invOps;
    OIIO_CHECK_NO_THROW(refLut1d.inverse(invOps));
    OIIO_CHECK_EQUAL(invOps.size(), 1u);

    const OCIO::OpData::InvLut1D * invLut1d
        = dynamic_cast<const OCIO::OpData::InvLut1D*>(invOps.get(0));
    OIIO_CHECK_ASSERT(invLut1d);

    OIIO_CHECK_EQUAL(invLut1d->getInputBitDepth(),
                     expectedInvInputBitDepth);

    OIIO_CHECK_EQUAL(invLut1d->getOutputBitDepth(),
                     expectedInvOutputBitDepth);

    OIIO_CHECK_EQUAL(invLut1d->getInterpolation(),
                     expectedInvInterpolationAlgo);

    OIIO_CHECK_EQUAL(invLut1d->getHalfFlags(),
                     expectedInvHalfFlags);

    OIIO_CHECK_EQUAL(invLut1d->getHueAdjust(),
                     OCIO::OpData::Lut1D::HUE_NONE);
}


OIIO_ADD_TEST(OpDataInvLut1D, inverse_bitDepth_domain)
{
    checkInverse_bitDepths_domain(
        // reference
        OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10,
        OCIO::INTERP_CUBIC, 
        OCIO::OpData::Lut1D::LUT_STANDARD,
        // expected
        OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT8,
        OCIO::INTERP_CUBIC, 
        OCIO::OpData::Lut1D::LUT_STANDARD);

    checkInverse_bitDepths_domain(
        // reference
        OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_F32,
        OCIO::INTERP_DEFAULT,
        OCIO::OpData::Lut1D::LUT_STANDARD,
        // expected
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT8,
        OCIO::INTERP_DEFAULT,
        OCIO::OpData::Lut1D::LUT_STANDARD);

    checkInverse_bitDepths_domain(
        // reference
        OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT12,
        OCIO::INTERP_LINEAR,
        OCIO::OpData::Lut1D::LUT_STANDARD,
        // expected
        OCIO::BIT_DEPTH_UINT12, OCIO::BIT_DEPTH_F16,
        OCIO::INTERP_LINEAR,
        OCIO::OpData::Lut1D::LUT_STANDARD);

    checkInverse_bitDepths_domain(
        // reference
        OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32,
        OCIO::INTERP_BEST,
        OCIO::OpData::Lut1D::LUT_STANDARD,
        // expected
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F16,
        OCIO::INTERP_BEST,
        OCIO::OpData::Lut1D::LUT_STANDARD);

    checkInverse_bitDepths_domain(
        // reference
        OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT12,
        OCIO::INTERP_BEST,
        OCIO::OpData::Lut1D::LUT_INPUT_HALF_CODE,
        // expected
        OCIO::BIT_DEPTH_UINT12, OCIO::BIT_DEPTH_F16,
        OCIO::INTERP_BEST,
        OCIO::OpData::Lut1D::LUT_INPUT_HALF_CODE);
}

OIIO_ADD_TEST(OpDataInvLut1D, inverse_hueadjust)
{
    OCIO::OpData::Lut1D refLut1dOp(OCIO::BIT_DEPTH_F32, 
                                   OCIO::BIT_DEPTH_F32,
                                   uid, name, desc,
                                   OCIO::INTERP_BEST, 
                                   OCIO::OpData::Lut1D::LUT_STANDARD);

    refLut1dOp.setHueAdjust(OCIO::OpData::Lut1D::HUE_DW3);

    // Get inverse of reference lut1d operation 
    OCIO::OpData::OpDataVec invOps;
    OIIO_CHECK_NO_THROW(refLut1dOp.inverse(invOps));
    OIIO_CHECK_EQUAL(invOps.size(), 1u);

    const OCIO::OpData::InvLut1D * invLut1dOp
        = dynamic_cast<const OCIO::OpData::InvLut1D*>(invOps.get(0));
    OIIO_CHECK_ASSERT(invLut1dOp);

    OIIO_CHECK_EQUAL(invLut1dOp->getHueAdjust(),
                     OCIO::OpData::Lut1D::HUE_DW3);
}

OIIO_ADD_TEST(OpDataInvLut1D, isInverse)
{
    // Create forward LUT.
    OCIO::OpData::Lut1D L1(OCIO::BIT_DEPTH_UINT8, 
                           OCIO::BIT_DEPTH_UINT10, 
                           uid, name, desc, 
                           OCIO::INTERP_LINEAR,
                           OCIO::OpData::Lut1D::LUT_STANDARD,
                           5);
    // Make it not an identity.
    OCIO::OpData::Array & array = L1.getArray();
    OCIO::OpData::Array::Values & values = array.getValues();
    values[0] = 20.f;
    OIIO_CHECK_ASSERT( ! L1.isIdentity() );

    // Create an inverse LUT with same basics.
    OCIO::OpData::InvLut1D L2(L1);
    L2.setId("foo");
    OIIO_CHECK_ASSERT( ! (L1 == L2) );

    // Check isInverse.
    OIIO_CHECK_ASSERT( L1.isInverse(L2) );
    OIIO_CHECK_ASSERT( L2.isInverse(L1) );

    // TODO: This should pass, since the arrays are actually the same if you
    //       normalize for the scaling difference.
    //L1.setOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    //OIIO_CHECK_ASSERT( L1.isInverse(L2) );
    //OIIO_CHECK_ASSERT( L2.isInverse(L1) );

    // Catch the situation where the arrays are the same even though the
    // bit-depths don't match and hence the arrays effectively aren't the same.
    L1.setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);  // restore original
    OIIO_CHECK_ASSERT( L1.isInverse(L2) );
    L1.OpData::setOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_ASSERT( ! L1.isInverse(L2) );
    OIIO_CHECK_ASSERT( ! L2.isInverse(L1) );
}

void SetLutArray(OCIO::OpData::Lut1D & op,
                 unsigned dimension,
                 unsigned channels, 
                 const float * data)
{
    OCIO::OpData::Array & refArray = op.getArray();
    refArray.resize(dimension, channels);
    // The data allocated for the array is dimension * getMaxColorComponents(),
    // not dimension * channels.

    const unsigned maxChannels = refArray.getMaxColorComponents();
    OCIO::OpData::Array::Values & values = refArray.getValues();
    if(channels == maxChannels)
    {
        memcpy(&(values[0]), data, dimension * channels * sizeof(float));
    }
    else
    {
        // Set the red component, fill other with zero values
        for(unsigned i = 0; i < dimension; ++i)
        {
            values[i*maxChannels+0] = data[i];
            values[i*maxChannels+1] = 0.f;
            values[i*maxChannels+2] = 0.f;
        }
    }
}

// Validate the overall increase/decrease and effective domain
void CheckInverse_IncreasingEffectiveDomain(
    unsigned dimension, unsigned channels, const float* fwdArrayData,
    bool expIncreasingR, unsigned expStartDomainR, unsigned expEndDomainR,
    bool expIncreasingG, unsigned expStartDomainG, unsigned expEndDomainG,
    bool expIncreasingB, unsigned expStartDomainB, unsigned expEndDomainB)
{
    OCIO::OpData::Lut1D refLut1dOp(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                   uid, name, desc,
                                   OCIO::INTERP_BEST,
                                   OCIO::OpData::Lut1D::LUT_STANDARD);

    SetLutArray(refLut1dOp, dimension, channels, fwdArrayData);

    OCIO::OpData::InvLut1D invLut1dOp( refLut1dOp );

    const OCIO::OpData::InvLut1D::ComponentProperties & 
        redProperties = invLut1dOp.getRedProperties();

    const OCIO::OpData::InvLut1D::ComponentProperties & 
        greenProperties = invLut1dOp.getGreenProperties();

    const OCIO::OpData::InvLut1D::ComponentProperties & 
        blueProperties = invLut1dOp.getBlueProperties();

    OIIO_CHECK_EQUAL(redProperties.isIncreasing, expIncreasingR);
    OIIO_CHECK_EQUAL(redProperties.startDomain, expStartDomainR);
    OIIO_CHECK_EQUAL(redProperties.endDomain, expEndDomainR);

    OIIO_CHECK_EQUAL(greenProperties.isIncreasing, expIncreasingG);
    OIIO_CHECK_EQUAL(greenProperties.startDomain, expStartDomainG);
    OIIO_CHECK_EQUAL(greenProperties.endDomain, expEndDomainG);

    OIIO_CHECK_EQUAL(blueProperties.isIncreasing, expIncreasingB);
    OIIO_CHECK_EQUAL(blueProperties.startDomain, expStartDomainB);
    OIIO_CHECK_EQUAL(blueProperties.endDomain, expEndDomainB);
}

OIIO_ADD_TEST(OpDataInvLut1D, inverse_increasing_effectiveDomain)
{
    {
        const float fwdData[] = { 0.1f, 0.8f, 0.1f,    // 0
                                  0.1f, 0.7f, 0.1f,
                                  0.1f, 0.6f, 0.1f,    // 2
                                  0.2f, 0.5f, 0.1f,    // 3
                                  0.3f, 0.4f, 0.2f,
                                  0.4f, 0.3f, 0.3f,
                                  0.5f, 0.1f, 0.4f,    // 6
                                  0.6f, 0.1f, 0.5f,    // 7
                                  0.7f, 0.1f, 0.5f,
                                  0.8f, 0.1f, 0.5f };  // 9

        CheckInverse_IncreasingEffectiveDomain(
          10, 3, fwdData,
          true,  2, 9,   // increasing, flat [0, 2]
          false, 0, 6,   // decreasing, flat [6, 9]
          true,  3, 7);  // increasing, flat [0, 3] and [7, 9]
    }

    {
        const float fwdData[] = { 0.3f,    // 0
                                  0.3f,
                                  0.3f,    // 2
                                  0.4f,
                                  0.5f,
                                  0.6f,
                                  0.7f,
                                  0.8f,    // 7
                                  0.8f,
                                  0.8f };  // 9

        CheckInverse_IncreasingEffectiveDomain(
          10, 1, fwdData,
          true, 2, 7,   // increasing, flat [0->2] and [7->9]
          true, 2, 7,
          true, 2, 7);
    }

    {
        const float fwdData[] = { 0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f,
                                  0.5f };

        CheckInverse_IncreasingEffectiveDomain(
          10, 1, fwdData,
          false, 0, 0,
          false, 0, 0,
          false, 0, 0);
    }

    {
        const float fwdData[] = { 0.8f,     // 0
                                  0.9f,     // reversal
                                  0.8f,     // 2
                                  0.5f,
                                  0.4f,
                                  0.3f,
                                  0.2f,
                                  0.1f,     // 7
                                  0.1f,
                                  0.2f };   // reversal

        CheckInverse_IncreasingEffectiveDomain(
          10, 1, fwdData,
          false, 2, 7,
          false, 2, 7,
          false, 2, 7);
    }
}

// Validate the flatten algorithms
void CheckInverse_Flatten(unsigned dimension,
                          unsigned channels, 
                          const float * fwdArrayData,
                          const float * expInvArrayData)
{
    OCIO::OpData::Lut1D refLut1dOp(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                   uid, name, desc,
                                   OCIO::INTERP_BEST, 
                                   OCIO::OpData::Lut1D::LUT_STANDARD);

    SetLutArray(refLut1dOp, dimension, channels, fwdArrayData);

    OCIO::OpData::InvLut1D invLut1dOp( refLut1dOp );

    const OCIO::OpData::Array::Values & 
        invValues = invLut1dOp.getArray().getValues();

    for(unsigned i = 0; i < dimension * channels; ++i)
    {
        OIIO_CHECK_EQUAL( invValues[i], expInvArrayData[i] );
    }
}

OIIO_ADD_TEST(OpDataInvLut1D, inverse_flatten_test)
{
    {
        const float fwdData[] = { 0.10f, 0.90f, 0.25f,    // 0
                                  0.20f, 0.80f, 0.30f,
                                  0.30f, 0.70f, 0.40f,
                                  0.40f, 0.60f, 0.50f,
                                  0.35f, 0.50f, 0.60f,    // 4
                                  0.30f, 0.55f, 0.50f,    // 5
                                  0.45f, 0.60f, 0.40f,    // 6
                                  0.50f, 0.65f, 0.30f,    // 7
                                  0.60f, 0.45f, 0.20f,    // 8
                                  0.70f, 0.50f, 0.10f };  // 9
        // red is increasing, with a reversal [4, 5]
        // green is decreasing, with reversals [4, 5] and [9]
        // blue is decreasing, with reversals [0, 8]

        const float expInvData[] = { 0.10f, 0.90f, 0.25f,
                                     0.20f, 0.80f, 0.25f,
                                     0.30f, 0.70f, 0.25f,
                                     0.40f, 0.60f, 0.25f,
                                     0.40f, 0.50f, 0.25f,
                                     0.40f, 0.50f, 0.25f,
                                     0.45f, 0.50f, 0.25f,
                                     0.50f, 0.50f, 0.25f,
                                     0.60f, 0.45f, 0.20f,
                                     0.70f, 0.45f, 0.10f };

        CheckInverse_Flatten(10, 3, fwdData, expInvData);
    }
}

void SetLutArrayHalf(OCIO::OpData::Lut1D & op, unsigned channels)
{
    const unsigned dimension = 65536u;
    OCIO::OpData::Array & refArray = op.getArray();
    refArray.resize(dimension, channels);
    // The data allocated for the array is dimension * getMaxColorComponents(),
    // not dimension * channels.

    const unsigned maxChannels = refArray.getMaxColorComponents();
    OCIO::OpData::Array::Values & values = refArray.getValues();
    for (unsigned j = 0u; j < channels; j++)
    {
        for (unsigned i = 0u; i < 65536u; i++)
        {
            float f = OCIO::ConvertHalfBitsToFloat((unsigned short)i);
            if (j == 0)
            {
                if (i < 32768u)
                    f = 2.f * f - 0.1f;
                else                            // neg domain overlaps pos with reversal
                    f = 3.f * f + 0.1f;
                if (i >= 25000u && i < 32760u)  // flat spot at positive end
                    f = 10000.f;
                if (i >= 60000u)                // flat spot at neg end
                    f = -10000.f;
                if (i > 15000u && i < 20000u)   // reversal in positive side
                    f = 0.5f;
                if (i > 50000u && i < 55000u)   // reversal in negative side
                    f = -2.f;
            }
            else if (j == 1)
            {
                if (i < 32768u)                  // decreasing function
                    f = -0.5f * f + 0.02f;
                else                            // gap between pos & neg at zero
                    f = -0.4f * f + 0.05f;
                if (i >= 25000u && i < 32760u)  // flat spot at positive end
                    f = -400.f;
                if (i >= 60000u)                // flat spot at neg end
                    f = 2000.f;
                if (i > 15000u && i < 20000u)   // reversal in positive side
                    f = -0.1f;
                if (i > 50000u && i < 55000u)   // reversal in negative side
                    f = 1.4f;
            }
            else if (j == 2)
            {
                if (i < 32768u)
                    f = std::pow(f, 1.5f);
                else
                    f = -std::pow(-f, 0.9f);
                if (i <= 11878u || (i >= 32768u && i <= 44646u))  // flat spot around zero
                    f = -0.01f;
            }
            values[i * maxChannels + j] = f;
        }
    }
}

OIIO_ADD_TEST(OpDataInvLut1D, inverse_half_domain)
{
    OCIO::OpData::Lut1D refLut1dOp(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                   uid, name, desc,
                                   OCIO::INTERP_BEST, 
                                   OCIO::OpData::Lut1D::LUT_INPUT_HALF_CODE);

    SetLutArrayHalf(refLut1dOp, 3u);

    OCIO::OpData::InvLut1D invLut1dOp( refLut1dOp );

    const OCIO::OpData::InvLut1D::ComponentProperties & 
        redProperties = invLut1dOp.getRedProperties();

    const OCIO::OpData::InvLut1D::ComponentProperties & 
        greenProperties = invLut1dOp.getGreenProperties();

    const OCIO::OpData::InvLut1D::ComponentProperties & 
        blueProperties = invLut1dOp.getBlueProperties();

    const OCIO::OpData::Array::Values & 
        invValues = invLut1dOp.getArray().getValues();

    // Check increasing/decreasing and start/end domain.
    OIIO_CHECK_EQUAL(redProperties.isIncreasing, true);
    OIIO_CHECK_EQUAL(redProperties.startDomain, 0u);
    OIIO_CHECK_EQUAL(redProperties.endDomain, 25000u);
    OIIO_CHECK_EQUAL(redProperties.negStartDomain, 44100u);  // -0.2/3 (flattened to remove overlap)
    OIIO_CHECK_EQUAL(redProperties.negEndDomain, 60000u);

    OIIO_CHECK_EQUAL(greenProperties.isIncreasing, false);
    OIIO_CHECK_EQUAL(greenProperties.startDomain, 0u);
    OIIO_CHECK_EQUAL(greenProperties.endDomain, 25000u);
    OIIO_CHECK_EQUAL(greenProperties.negStartDomain, 32768u);
    OIIO_CHECK_EQUAL(greenProperties.negEndDomain, 60000u);

    OIIO_CHECK_EQUAL(blueProperties.isIncreasing, true);
    OIIO_CHECK_EQUAL(blueProperties.startDomain, 11878u);
    OIIO_CHECK_EQUAL(blueProperties.endDomain, 31743u);      // see note in invLut1DOp.cpp
    OIIO_CHECK_EQUAL(blueProperties.negStartDomain, 44646u);
    OIIO_CHECK_EQUAL(blueProperties.negEndDomain, 64511u);

    // Check reversals are removed.
    half act, aim;
    act = invValues[16000 * 3];
    OIIO_CHECK_EQUAL( act.bits(), 15922 );  // halfToFloat(15000) * 2 - 0.1
    act = invValues[52000 * 3];
    OIIO_CHECK_EQUAL( act.bits(), 51567 );  // halfToFloat(50000) * 3 + 0.1
    act = invValues[16000 * 3 + 1];
    OIIO_CHECK_EQUAL( act.bits(), 46662 );  // halfToFloat(15000) * -0.5 + 0.02
    act = invValues[52000 * 3 + 1];
    OIIO_CHECK_EQUAL( act.bits(), 15885 );  // halfToFloat(50000) * -0.4 + 0.05

    bool reversal = false;
    for (unsigned i = 1; i < 31745; i++)
    {
        if (invValues[i * 3] < invValues[(i - 1) * 3])           // increasing red
            reversal = true;
    }
    OIIO_CHECK_ASSERT( ! reversal );
    // Check no overlap at +0 and -0.
    OIIO_CHECK_ASSERT( invValues[0] >= invValues[32768 * 3]);
    reversal = false;
    for (unsigned i = 1; i < 31745; i++)
    {
        if (invValues[i * 3 + 1] > invValues[(i - 1) * 3 + 1])   // decreasing grn
            reversal = true;
    }
    OIIO_CHECK_ASSERT( ! reversal );
    OIIO_CHECK_ASSERT( invValues[0 + 1] <= invValues[32768 * 3 + 1]);
    reversal = false;
    for (unsigned i = 1; i < 31745; i++)
    {
        if (invValues[i * 3 + 2] < invValues[(i - 1) * 3 + 2])   // increasing blu
            reversal = true;
    }
    OIIO_CHECK_ASSERT( ! reversal );
    OIIO_CHECK_ASSERT( invValues[0 + 2] >= invValues[32768 * 3 + 2]);
}


#endif