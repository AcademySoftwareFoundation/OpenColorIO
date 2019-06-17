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

#include <sstream>
#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "md5/md5.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut1D/Lut1DOpData.h"
#include "ops/Matrix/MatrixOps.h"
#include "ops/Range/RangeOpData.h"
#include "OpTools.h"

OCIO_NAMESPACE_ENTER
{
// Number of possible values for the Half domain.
static const unsigned long HALF_DOMAIN_REQUIRED_ENTRIES = 65536;

Lut1DOpData::Lut3by1DArray::Lut3by1DArray(BitDepth inBitDepth,
                                          BitDepth outBitDepth,
                                          HalfFlags halfFlags)
{
    resize(GetLutIdealSize(inBitDepth, halfFlags), getMaxColorComponents());
    fill(halfFlags, outBitDepth);
}

Lut1DOpData::Lut3by1DArray::Lut3by1DArray(BitDepth outBitDepth,
                                          HalfFlags halfFlags,
                                          unsigned long length)
{
    resize(length, getMaxColorComponents());
    fill(halfFlags, outBitDepth);
}

Lut1DOpData::Lut3by1DArray::~Lut3by1DArray()
{
}

void Lut1DOpData::Lut3by1DArray::fill(HalfFlags halfFlags,
                                      BitDepth outBitDepth)
{
    const unsigned long dim = getLength();
    const unsigned long maxChannels = getMaxColorComponents();

    Array::Values& values = getValues();
    if (Lut1DOpData::IsInputHalfDomain(halfFlags))
    {
        const float scaleFactor = GetBitDepthMaxValue(outBitDepth);

        for (unsigned long idx = 0; idx<dim; ++idx)
        {
            half htemp; htemp.setBits((unsigned short)idx);
            const float ftemp = htemp * scaleFactor;

            const unsigned long row = maxChannels * idx;
            for (unsigned long channel = 0; channel<maxChannels; ++channel)
            {
                values[channel + row] = ftemp;
            }
        }

    }
    else
    {
        const float stepValue
            = GetBitDepthMaxValue(outBitDepth) / ((float)dim - 1.0f);

        for (unsigned long idx = 0; idx<dim; ++idx)
        {
            const float ftemp = (float)idx * stepValue;

            const unsigned long row = maxChannels * idx;
            for (unsigned long channel = 0; channel<maxChannels; ++channel)
            {
                values[channel + row] = ftemp;
            }
        }
    }
}

void Lut1DOpData::Lut3by1DArray::scale(float scaleFactor)
{
    // Don't scale if scaleFactor = 1.0f.
    if (scaleFactor != 1.0f)
    {
        Array::Values& arrayVals = getValues();
        const size_t size = arrayVals.size();

        for (size_t i = 0; i < size; i++)
        {
            arrayVals[i] *= scaleFactor;
        }
    }
}

bool Lut1DOpData::Lut3by1DArray::isIdentity(HalfFlags halfFlags,
                                            BitDepth outBitDepth) const
{
    // An identity LUT does nothing except possibly bit-depth conversion.
    //
    // Note: OCIO v1 had a member variable so that this check would only need
    // to be calculated once.  However, there were cases where m_isNoOp could
    // get out of sync with the LUT contents.  It also required an unfinalize
    // method.  It's not clear that this is a performance bottleneck since for
    // most LUTs, we'll return false after only a few iterations of the loop.
    // So for now we've remove the member to store the result of this computation.
    //
    const unsigned long dim = getLength();
    const Array::Values& values = getValues();
    const unsigned long maxChannels = getMaxColorComponents();

    if (Lut1DOpData::IsInputHalfDomain(halfFlags))
    {
        const float scaleFactor = 1.0f / GetBitDepthMaxValue(outBitDepth);

        for (unsigned long idx = 0; idx<dim; ++idx)
        {
            half aimHalf;
            aimHalf.setBits((unsigned short)idx);

            const unsigned long row = maxChannels * idx;
            for (unsigned long channel = 0; channel<maxChannels; ++channel)
            {
                const half valHalf = scaleFactor * values[channel + row];
                // Must be different by at least two ULPs to not be an identity.
                if (HalfsDiffer(aimHalf, valHalf, 1))
                {
                    return false;
                }
            }
        }
    }
    else
    {
        //
        // Note: OCIO v1 allowed the ability to control the tolerance and
        // set whether it is absolute or relative. There was a TODO that
        // suggested making it automatic based on the bit-depth, and that
        // is what's done here.
        //
        // Note: LUTs that are approximately identity transforms and
        // contain a wide range of float values should use the
        // half-domain representation.  The contents of most LUTs using 
        // this branch will hence either not be close to an identity anyway
        // or will be in units that are roughly perceptually uniform and hence
        // it is more appropriate to use an absolute error based on the
        // bit-depth rather than a fully relative error that will be both
        // too sensitive near zero and too loose at the high end.
        //
        const float rel_tol = 1e-5f;
        const float abs_tol = GetBitDepthMaxValue(outBitDepth) * rel_tol;
        const float stepValue = GetBitDepthMaxValue(outBitDepth)
                                / ((float)dim - 1.0f);

        for (unsigned long idx = 0; idx<dim; ++idx)
        {
            const float aim = (float)idx * stepValue;

            const unsigned long row = maxChannels * idx;
            for (unsigned long channel = 0; channel<maxChannels; ++channel)
            {
                const float err = values[channel + row] - aim;

                if (fabs(err) > abs_tol)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

unsigned long Lut1DOpData::Lut3by1DArray::getNumValues() const
{
    return getLength() * getMaxColorComponents();
}

Lut1DOpData::Lut1DOpData(unsigned long dimension)
    : OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    , m_interpolation(INTERP_LINEAR)
    , m_array(getOutputBitDepth(), LUT_STANDARD, dimension)
    , m_halfFlags(LUT_STANDARD)
    , m_hueAdjust(HUE_NONE)
    , m_direction(TRANSFORM_DIR_FORWARD)
    , m_invQuality(LUT_INVERSION_FAST)
    , m_fileBitDepth(BIT_DEPTH_UNKNOWN)
{
}

Lut1DOpData::Lut1DOpData(unsigned long dimension, TransformDirection dir)
    : OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    , m_interpolation(INTERP_LINEAR)
    , m_array(getOutputBitDepth(), LUT_STANDARD, dimension)
    , m_halfFlags(LUT_STANDARD)
    , m_hueAdjust(HUE_NONE)
    , m_direction(dir)
    , m_invQuality(LUT_INVERSION_FAST)
    , m_fileBitDepth(BIT_DEPTH_UNKNOWN)
{
}

Lut1DOpData::Lut1DOpData(BitDepth inBitDepth,
                         BitDepth outBitDepth,
                         HalfFlags halfFlags)
    : OpData(inBitDepth, outBitDepth)
    , m_interpolation(INTERP_LINEAR)
    , m_array(getInputBitDepth(), getOutputBitDepth(), halfFlags)
    , m_halfFlags(halfFlags)
    , m_hueAdjust(HUE_NONE)
    , m_direction(TRANSFORM_DIR_FORWARD)
    , m_invQuality(LUT_INVERSION_FAST)
    , m_fileBitDepth(outBitDepth)
{
}

Lut1DOpData::Lut1DOpData(BitDepth inBitDepth, BitDepth outBitDepth,
                         const std::string& id,
                         const Descriptions& descriptions,
                         Interpolation interpolation,
                         HalfFlags halfFlags)
    : OpData(inBitDepth, outBitDepth, id, descriptions)
    , m_interpolation(interpolation)
    , m_array(getInputBitDepth(), getOutputBitDepth(), halfFlags)
    , m_halfFlags(halfFlags)
    , m_hueAdjust(HUE_NONE)
    , m_direction(TRANSFORM_DIR_FORWARD)
    , m_invQuality(LUT_INVERSION_FAST)
    , m_fileBitDepth(outBitDepth)
{
}

Lut1DOpData::Lut1DOpData(BitDepth inBitDepth, BitDepth outBitDepth,
                         const std::string& id,
                         const Descriptions& descriptions,
                         Interpolation interpolation,
                         HalfFlags halfFlags,
                         unsigned long dimension)
    : OpData(inBitDepth, outBitDepth, id, descriptions)
    , m_interpolation(interpolation)
    , m_array(getInputBitDepth(), halfFlags, dimension)
    , m_halfFlags(halfFlags)
    , m_hueAdjust(HUE_NONE)
    , m_direction(TRANSFORM_DIR_FORWARD)
    , m_invQuality(LUT_INVERSION_FAST)
    , m_fileBitDepth(outBitDepth)
{
}

Lut1DOpData::~Lut1DOpData()
{
}

Interpolation Lut1DOpData::getConcreteInterpolation() const
{
    // TODO: currently INTERP_NEAREST is not implemented in Lut1DOpCPU.
    // This is a regression from OCIO v1.
    // NB: To have the same interpolation support (i.e. same color processing)
    // between the CPU & GPU paths, the 'Nearest' interpolation is implemented
    // using the 'Linear' even if GPU path already support the 'Nearest'
    // interpolation.
    // NB: invalid interpolation will make validate() throw.
    return INTERP_LINEAR;
}

void Lut1DOpData::setInterpolation(Interpolation algo)
{
    m_interpolation = algo;
}

LutInversionQuality Lut1DOpData::getConcreteInversionQuality() const
{
    switch (m_invQuality)
    {
    case LUT_INVERSION_EXACT:
    case LUT_INVERSION_BEST:
        return LUT_INVERSION_EXACT;

    case LUT_INVERSION_FAST:
    case LUT_INVERSION_DEFAULT:
    default:
        return LUT_INVERSION_FAST;
    }
}

void Lut1DOpData::setInversionQuality(LutInversionQuality style)
{
    m_invQuality = style;
}

bool Lut1DOpData::isIdentity() const
{
    return m_array.isIdentity(m_halfFlags, getOutputBitDepth());
}

bool Lut1DOpData::hasChannelCrosstalk() const
{
    if (getHueAdjust() != Lut1DOpData::HUE_NONE)
    {
        // Returning !isIdentity() would be time consuming.
        return true;
    }
    else
    {
        return false;
    }
}

bool Lut1DOpData::isNoOp() const
{
    if (isInputHalfDomain())
    {
        return isIdentity();
    }
    else
    {
        return false;
    }
}

OpDataRcPtr Lut1DOpData::getIdentityReplacement() const
{
    const BitDepth inBD = getInputBitDepth();
    const BitDepth outBD = getOutputBitDepth();

    if (isInputHalfDomain())
    {
        return std::make_shared<MatrixOpData>(inBD, outBD);
    }
    else
    {
        return std::make_shared<RangeOpData>(inBD,
                                             outBD,
                                             0.,
                                             GetBitDepthMaxValue(inBD),
                                             0.,
                                             GetBitDepthMaxValue(outBD));
    }
}

void Lut1DOpData::setOutputBitDepth(BitDepth out)
{
    if (m_direction == TRANSFORM_DIR_FORWARD)
    {
        // Scale factor is max_new_depth/max_old_depth.
        const float scaleFactor
            = GetBitDepthMaxValue(out)
            / GetBitDepthMaxValue(getOutputBitDepth());

        // Scale array for the new bit-depth if scaleFactor != 1.
        m_array.scale(scaleFactor);
    }
    // Call parent to set the output bit depth.
    OpData::setOutputBitDepth(out);
}

void Lut1DOpData::setInputBitDepth(BitDepth in)
{
    if (m_direction == TRANSFORM_DIR_INVERSE)
    {
        // Recall that our array is for the LUT to be inverted, so this method
        // is similar to set OUT depth for the original LUT.

        // Scale factor is max_new_depth/max_old_depth.
        const float scaleFactor
            = GetBitDepthMaxValue(in)
            / GetBitDepthMaxValue(getInputBitDepth());

        // Scale array for the new bit-depth if scaleFactor != 1.
        m_array.scale(scaleFactor);
    }
    // Call parent to set the input bit depth.
    OpData::setInputBitDepth(in);
}

void Lut1DOpData::setInputHalfDomain(bool isHalfDomain)
{
    m_halfFlags = (isHalfDomain) ?
        ((HalfFlags)(m_halfFlags | LUT_INPUT_HALF_CODE)) :
        ((HalfFlags)(m_halfFlags & ~LUT_INPUT_HALF_CODE));
}

void Lut1DOpData::setOutputRawHalfs(bool isRawHalfs)
{
    m_halfFlags = (isRawHalfs) ?
        ((HalfFlags)(m_halfFlags | LUT_OUTPUT_HALF_CODE)) :
        ((HalfFlags)(m_halfFlags & ~LUT_OUTPUT_HALF_CODE));
}

namespace
{
bool IsValid(const Interpolation & interpolation)
{
    switch (interpolation)
    {
    case INTERP_BEST:
    case INTERP_DEFAULT:
    case INTERP_LINEAR:
    case INTERP_NEAREST:
        return true;
    case INTERP_CUBIC:
    case INTERP_TETRAHEDRAL:
    case INTERP_UNKNOWN:
    default:
        return false;
    }
}
}

void Lut1DOpData::validate() const
{
    OpData::validate();

    if (!IsValid(m_interpolation))
    {
        std::ostringstream oss;
        oss << "1D LUT does not support interpolation algorithm: ";
        oss << InterpolationToString(getInterpolation());
        oss << ".";
        throw Exception(oss.str().c_str());
    }

    try
    {
        getArray().validate();
    }
    catch (Exception& e)
    {
        std::ostringstream oss;
        oss << "1D LUT content array issue: ";
        oss << e.what();

        throw Exception(oss.str().c_str());
    }

    // If isHalfDomain is set, we need to make sure we have 65536 entries.
    if (isInputHalfDomain() && getArray().getLength()
        != HALF_DOMAIN_REQUIRED_ENTRIES)
    {
        std::ostringstream oss;
        oss << "1D LUT: ";
        oss << getArray().getLength();
        oss << " entries found, ";
        oss << HALF_DOMAIN_REQUIRED_ENTRIES;
        oss << " required for halfDomain 1D LUT.";

        throw Exception(oss.str().c_str());
    }
}

unsigned long Lut1DOpData::GetLutIdealSize(BitDepth incomingBitDepth)
{
    // Return the number of entries needed in order to do a lookup 
    // for the specified bit-depth.

    // For 32f, a look-up is impractical so in that case return 64k.

    switch (incomingBitDepth)
    {
    case BIT_DEPTH_UINT8:
    case BIT_DEPTH_UINT10:
    case BIT_DEPTH_UINT12:
    case BIT_DEPTH_UINT14:
    case BIT_DEPTH_UINT16:
        return (unsigned long)(GetBitDepthMaxValue(incomingBitDepth) + 1);

    case BIT_DEPTH_F16:
    case BIT_DEPTH_F32:
        break;

    case BIT_DEPTH_UNKNOWN:
    case BIT_DEPTH_UINT32:
    default:
    {
        std::string err("Bit-depth is not supported: ");
        err += BitDepthToString(incomingBitDepth);
        throw Exception(err.c_str());
        break;
    }

    }

    return 65536;
}

unsigned long Lut1DOpData::GetLutIdealSize(BitDepth inputBitDepth,
                                           HalfFlags halfFlags)
{
    // Returns the number of entries that fill() expects in order to make
    // an identity LUT.

    // For half domain always return 65536, since that is what fill() expects.
    // However note that if the inputBitDepth is, e.g. 10i, this might not be
    // the number of entries required for a look-up.

    const unsigned long size = HALF_DOMAIN_REQUIRED_ENTRIES;

    if (Lut1DOpData::IsInputHalfDomain(halfFlags))
    {
        return size;
    }

    return GetLutIdealSize(inputBitDepth);
}

bool Lut1DOpData::mayLookup(BitDepth incomingDepth) const
{
    if (isInputHalfDomain())
    {
        return incomingDepth == BIT_DEPTH_F16;
    }
    else  // not a half-domain LUT
    {
        if (!IsFloatBitDepth(incomingDepth))
        {
            return m_array.getLength()
                == (GetBitDepthMaxValue(incomingDepth) + 1);
        }
    }
    return false;
}

Lut1DOpDataRcPtr Lut1DOpData::MakeLookupDomain(BitDepth incomingDepth)
{
    // For integer in-depths, we need a standard domain.
    Lut1DOpData::HalfFlags domainType = Lut1DOpData::LUT_STANDARD;

    // For 16f in-depth, we need a half domain.
    // (Return same for 32f, even though a pure lookup wouldn't be appropriate.)
    if (IsFloatBitDepth(incomingDepth))
    {
        domainType = Lut1DOpData::LUT_INPUT_HALF_CODE;
    }

    // Note that in this case the domainType is always appropriate for
    // the incomingDepth, so it should be safe to rely on the constructor
    // and fill() to always return the correct length.
    // (E.g., we don't need to worry about 10i with a half domain.)
    return std::make_shared<Lut1DOpData>(incomingDepth,
                                         incomingDepth,
                                         "", Descriptions(),
                                         INTERP_LINEAR,
                                         domainType);
}

bool Lut1DOpData::haveEqualBasics(const Lut1DOpData & B) const
{
    // Question:  Should interpolation style be considered?
    return m_halfFlags == B.m_halfFlags &&
           m_hueAdjust == B.m_hueAdjust &&
           m_array == B.m_array;
}

bool Lut1DOpData::operator==(const OpData & other) const
{
    if (this == &other) return true;

    if (!OpData::operator==(other)) return false;

    const Lut1DOpData* lop = static_cast<const Lut1DOpData*>(&other);

    // NB: The m_invQuality is not currently included.
    if (m_direction != lop->m_direction
        || m_interpolation != lop->m_interpolation)
    {
        return false;
    }

    return haveEqualBasics(*lop);
}

void Lut1DOpData::setHueAdjust(Lut1DOpData::HueAdjust algo)
{
    m_hueAdjust = algo;
}

Lut1DOpDataRcPtr Lut1DOpData::clone() const
{
    return std::make_shared<Lut1DOpData>(*this);
}


bool Lut1DOpData::IsInverse(const Lut1DOpData * lutfwd,
                            const Lut1DOpData * lutinv)
{
    // Note: The inverse LUT 1D finalize modifies the array to make it
    // monotonic, hence, this could return false in unexpected cases.
    // However, one could argue that those LUTs should not be optimized
    // out as an identity anyway.

    // Need to check bit-depth because the array scaling is relative to it.
    // (For LUT it is the out-depth, for inverse LUT it is the in-depth.)
    // Note that we use MaxValue so that 16f and 32f are considered the same.
    if (GetBitDepthMaxValue(lutfwd->getOutputBitDepth())
        != GetBitDepthMaxValue(lutinv->getInputBitDepth()))
    {
        // Quick fail with array size.
        if (lutfwd->getArray().getValues().size()
            != lutinv->getArray().getValues().size())
        {
            return false;
        }
        // Harmonize array bit-depths to allow a proper array comparison.
        Lut1DOpDataRcPtr scaledLut = lutfwd->clone();
        scaledLut->setOutputBitDepth(lutinv->getInputBitDepth());

        // Test the core parts such as array, half domain, and hue adjust while
        // ignoring superficial differences such as in/out bit-depth.
        return scaledLut->haveEqualBasics(*lutinv);
    }
    else
    {
        return lutfwd->haveEqualBasics(*lutinv);
    }
}

bool Lut1DOpData::isInverse(ConstLut1DOpDataRcPtr & B) const
{
    if (m_direction == TRANSFORM_DIR_FORWARD
        && B->m_direction == TRANSFORM_DIR_INVERSE)
    {
        return IsInverse(this, B.get());
    }
    else if (m_direction == TRANSFORM_DIR_INVERSE
        && B->m_direction == TRANSFORM_DIR_FORWARD)
    {
        return IsInverse(B.get(), this);
    }
    return false;
}

bool Lut1DOpData::mayCompose(ConstLut1DOpDataRcPtr & B) const
{
    // NB: This does not check bypass or dynamic.
    return   getHueAdjust() == HUE_NONE  &&
             B->getHueAdjust() == HUE_NONE;
}

Lut1DOpDataRcPtr Lut1DOpData::inverse() const
{
    Lut1DOpDataRcPtr invLut = clone();

    invLut->m_direction = (m_direction == TRANSFORM_DIR_FORWARD) ?
        TRANSFORM_DIR_INVERSE : TRANSFORM_DIR_FORWARD;

    // Swap input/output bitdepths.
    // Note: Call for OpData::set*BitDepth() in order to only swap the
    // bitdepths without any rescaling.
    const BitDepth in = getInputBitDepth();
    invLut->OpData::setInputBitDepth(getOutputBitDepth());
    invLut->OpData::setOutputBitDepth(in);

    return invLut;
}

namespace
{
const char* GetHueAdjustName(Lut1DOpData::HueAdjust algo)
{
    switch (algo)
    {
    case Lut1DOpData::HUE_DW3:
    {
        return "dw3";
        break;
    }
    case Lut1DOpData::HUE_NONE:
    {
        return "none";
        break;
    }
    }
    throw Exception("1D LUT has an invalid hue adjust style.");
}

}

void Lut1DOpData::finalize()
{
    if (m_direction == TRANSFORM_DIR_INVERSE)
    {
        initializeFromForward();
    }

    AutoMutex lock(m_mutex);

    md5_state_t state;
    md5_byte_t digest[16];

    md5_init(&state);
    md5_append(&state,
        (const md5_byte_t *)&(getArray().getValues()[0]),
        (int)(getArray().getValues().size() * sizeof(float)));
    md5_finish(&state, digest);

    std::ostringstream cacheIDStream;
    cacheIDStream << GetPrintableHash(digest) << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";
    cacheIDStream << InterpolationToString(m_interpolation) << " ";
    cacheIDStream << BitDepthToString(getInputBitDepth()) << " ";
    cacheIDStream << BitDepthToString(getOutputBitDepth()) << " ";
    cacheIDStream << (isInputHalfDomain()?"half domain ":"standard domain ");
    cacheIDStream << GetHueAdjustName(m_hueAdjust);
    // NB: The m_invQuality is not currently included.

    m_cacheID = cacheIDStream.str();
}

namespace
{
//-----------------------------------------------------------------------------
//
// Functional composition is a concept from mathematics where two functions
// are combined into a single function.  This idea may be applied to ops
// where we generate a single op that has the same (or similar) effect as
// applying the two ops separately.  The motivation is faster processing.
//
// When composing LUTs, the algorithm produces a result which takes the
// domain of the first op into the range of the last op.  So the algorithm
// needs to render values through the ops.  In some cases the domain of
// the first op is sufficient, in other cases we need to create a new more
// finely sampled domain to try and make the result less lossy.


//-----------------------------------------------------------------------------
// Calculate a new LUT by evaluating a new domain (A) through a set of ops (B).
//
// Note1: The caller must ensure that B is separable (i.e., it has no channel
//        crosstalk).
//
// Note2: Unlike Compose(Lut1DOpDataRcPtr,Lut1DOpDataRcPtr, ), this function
//        does not try to resize the first LUT (A), so the caller needs to
//        create a suitable domain.
//
// Note3:  We do not attempt to propagate hueAdjust or bypass states.
//         These must be taken care of by the caller.
// 
// A is used as in/out parameter. As input is it the first LUT in the composition,
// as output it is the result of the composition.
void ComposeVec(Lut1DOpDataRcPtr & A, const OpRcPtrVec & B)
{
    if (B.size() == 0)
    {
        throw Exception("There is nothing to compose the 1D LUT with");
    }

    if (A->getOutputBitDepth() != B[0]->getInputBitDepth())
    {
        throw Exception("A bit-depth mismatch forbids the "
                        "composition of 1D LUTs");
    }

    OpRcPtrVec ops;

    // Insert an op to compensate for the bitdepth scaling of A.
    //
    // The values in A's array have a certain scaling which needs to be
    // normalized since ops will have an in-depth of 32f.
    // Although it may seem like we could use the constructor to create
    // a bit-depth conversion identity from A's out-depth to 32f,
    // this doesn't work.  When pM gets appended, the set depth call
    // would cause the scale factor to disappear.  In this case, the
    // append set depth and the finalize set depth cancel out and we
    // are left with our desired scale being applied.

    const float iScale = 1.f / GetBitDepthMaxValue(A->getOutputBitDepth());
    const float iScale4[4] = { iScale, iScale, iScale, iScale };
    CreateScaleOp(ops, iScale4, TRANSFORM_DIR_FORWARD);

    // Copy and append B.
    ops += B;

    // Insert an op to compensate for the bitdepth scaling of B.
    //
    // We render at 32f but need to create an array which may be inserted
    // into a LUT with B's output depth, so apply the scaling manually.
    // For the explanation of the bit-depths used, see the comment above.

    const BitDepth outputBD = B[B.size() - 1]->getOutputBitDepth();
    const float oScale = GetBitDepthMaxValue(outputBD);
    const float oScale4[4] = { oScale, oScale, oScale, oScale };
    CreateScaleOp(ops, oScale4, TRANSFORM_DIR_FORWARD);

    // Set up so that the eval directly fills in the array of the result LUT.

    const long numPixels = (long)A->getArray().getLength();

    // TODO: Could keep it one channel in some cases.
    A->getArray().resize(numPixels, 3);
    Array::Values& inValues = A->getArray().getValues();

    // Evaluate the transforms at 32f.
    // Note: If any ops are bypassed, that will be respected here.

    EvalTransform((const float*)(&inValues[0]),
                  (float*)(&inValues[0]),
                  numPixels,
                  ops);

    A->OpData::setOutputBitDepth(outputBD);

}
}

// Compose two Lut1DOpData.
//
// Note1: If either LUT uses hue adjust, composition will not give the same
// result as if they were applied sequentially.  However, we need to allow
// composition because the LUT 1D CPU renderer needs it to build the lookup
// table for the hue adjust renderer.  We could potentially do a lock object in
// that renderer to over-ride the hue adjust temporarily like in invLut1d.
// But for now, put the burdon on the caller to use Lut1DOpData::mayCompose first.
//
// Note2: Likewise ideally we would prohibit composition if hasMatchingBypass
// is false.  However, since the renderers may need to resample the LUTs,
// do not want to raise an exception or require the new domain to be dynamic.
// So again, it is up to the caller verify dynamic and bypass compatibility
// when calling this function in a more general context.
void Lut1DOpData::Compose(Lut1DOpDataRcPtr & A,
                          ConstLut1DOpDataRcPtr & B,
                          ComposeMethod compFlag)
{
    if (A->getOutputBitDepth() != B->getInputBitDepth())
    {
        throw Exception("A bit-depth mismatch forbids the composition of 1D LUTs");
    }

    OpRcPtrVec ops;

    unsigned long min_size = 0;
    BitDepth resampleDepth = BIT_DEPTH_UINT16;
    switch (compFlag)
    {
    case COMPOSE_RESAMPLE_NO:
    {
        min_size = 0;
        break;
    }
    case COMPOSE_RESAMPLE_INDEPTH:
    {
        // TODO: Composition of LUTs is a potentially lossy operation.
        //
        // We try to be safe by ensuring that the result will be finely
        // sampled enough to do a look-up for the current input bit-depth,
        // but it is possible that that bit-depth will need to be reset later.
        // In particular, if B is longer than this and the bit-depth is later
        // reset to be higher, we will have thrown away needed precision.
        // RESAMPLE_BIG is designed to avoid that problem but it has a
        // performance cost.
        resampleDepth = A->getInputBitDepth();
        min_size = GetLutIdealSize(resampleDepth,
                                   A->getHalfFlags());
        break;
    }
    case COMPOSE_RESAMPLE_BIG:
    {
        resampleDepth = BIT_DEPTH_UINT16;
        min_size = (unsigned long)GetBitDepthMaxValue(resampleDepth) + 1;
        break;
    }

    // TODO: May want to add another style which is the maximum of
    //       B size (careful of half domain), and in-depth ideal size.
    }

    const unsigned long Asz = A->getArray().getLength();
    const bool goodDomain = A->isInputHalfDomain() || (Asz >= min_size);
    const bool useOrigDomain = compFlag == COMPOSE_RESAMPLE_NO;

    if (!goodDomain && !useOrigDomain)
    {
        // Interpolate through both LUTs in this case (resample).
        CreateLut1DOp(ops, A, TRANSFORM_DIR_FORWARD);

        // Create identity with finer domain.

        // TODO: Should not need to create a new LUT object for this.
        //       Perhaps add a utility function to be shared with the constructor.
        A = std::make_shared<Lut1DOpData>(resampleDepth,
                                          A->getInputBitDepth(),
                                          A->getID(),
                                          A->getDescriptions(),
                                          A->getInterpolation(),
                                          // half case handled above
                                          Lut1DOpData::LUT_STANDARD);

    }

    Lut1DOpDataRcPtr bCloned = B->clone();
    CreateLut1DOp(ops, bCloned, TRANSFORM_DIR_FORWARD);

    // TODO:  May want to revisit metadata propagation.
    OpData::Descriptions newDesc;
    newDesc += "1D LUT from composition";

    // Create the result LUT by composing the domain through the desired ops.
    ComposeVec(A, ops);

    // Configure the metadata of the result LUT.
    A->setID(A->getID() + B->getID());
    newDesc += A->getDescriptions();
    newDesc += B->getDescriptions();
    A->getDescriptions() += newDesc;

    // See note above: Taking these from B since the common use case is for B to be 
    // the original LUT and A to be a new domain (e.g. used in LUT1D renderers).
    // TODO: Adjust domain in Lut1D renderer to be one channel.
    A->setHueAdjust(B->getHueAdjust());
}

// The domain to use for the FastLut is a challenging problem since we don't
// know the input and output color space of the LUT.  In particular, we don't
// know if a half or normal domain would be better.  For now, we use a
// heuristic which is based on the original input bit-depth of the inverse LUT
// (the output bit-depth of the forward LUT).  (We preserve the original depth
// as a member since typically by the time this routine is called, the depth
// has been reset to 32f.)  However, there are situations where the origDepth
// is not reliable (e.g. a user creates a transform in Custom mode and exports it).
// Ultimately, the goal is to replace this with an automated algorithm that
// computes the best domain based on analysis of the curvature of the LUT.
Lut1DOpDataRcPtr Lut1DOpData::MakeFastLut1DFromInverse(ConstLut1DOpDataRcPtr & lut, bool forGPU)
{
    if (lut->getDirection() != TRANSFORM_DIR_INVERSE)
    {
        throw Exception("MakeFastLut1DFromInverse expects an inverse 1D LUT");
    }

    if (lut->m_fileBitDepth == BIT_DEPTH_UNKNOWN)
    {
        throw Exception("MakeFastLut1DFromInverse expects a defined file bit-depth");
    }
    BitDepth depth(lut->m_fileBitDepth);

    // For typical LUTs (e.g. gamma tables from ICC monitor profiles)
    // we can use a smaller FastLUT on the GPU.
    // Currently allowing 16f to be subsampled for GPU but using 16i as a way
    // to indicate not to subsample certain LUTs (e.g. float-conversion LUTs).
    if (forGPU && depth != BIT_DEPTH_UINT16)
    {
        // GPU will always interpolate rather than look-up.
        // Use a smaller table for better efficiency.

        // TODO: Investigate performance/quality trade-off.

        depth = BIT_DEPTH_UINT12;
    }

    // But if the LUT has values outside [0,1], use a half-domain fastLUT.
    if (lut->hasExtendedDomain())
    {
        depth = BIT_DEPTH_F16;
    }

    // Make a domain for the composed 1D LUT.
    Lut1DOpDataRcPtr newDomainLut = MakeLookupDomain(depth);

    // Regardless of what depth is used to build the domain, set the in & out 
    // to the actual depth so that scaling is done correctly.
    newDomainLut->setInputBitDepth(lut->getInputBitDepth());
    newDomainLut->setOutputBitDepth(lut->getInputBitDepth());

    // Change inv style to INV_EXACT to avoid recursion.
    LutStyleGuard<Lut1DOpData> guard(*lut);

    Compose(newDomainLut, lut, COMPOSE_RESAMPLE_NO);
    return newDomainLut;
}

void Lut1DOpData::initializeFromForward()
{
    // This routine is to be called (e.g. in XML reader) once the base forward
    // Lut1D has been created and then sets up what is needed for the invLut1D.

    // Note that if the original LUT had a half domain, the invLut needs to as
    // well so that the appropriate evaluation algorithm is called.

    // NB: The file reader must call setOriginalBitDepth since some methods
    // need to know the original scaling of the LUT.
    prepareArray();
}

bool Lut1DOpData::hasExtendedDomain() const
{
    // The forward LUT is allowed to have entries outside the outDepth (e.g. a
    // 10i LUT is allowed to have values on [-20,1050] if it wants).  This is
    // called an extended range LUT and helps maximize accuracy by allowing
    // clamping to happen (if necessary) after the interpolation.
    // The implication is that the inverse LUT needs to evaluate over an
    // extended domain.  Since this potentially requires a slower rendering
    // method for the Fast style, this method allows the renderers to determine
    // if this is necessary.

    // Note that it is the range (output) of the forward LUT that determines
    // the need for an extended domain on the inverse LUT.  Whether the forward
    // LUT has a half domain does not matter.  E.g., a Lustre float-conversion
    // LUT has a half domain but outputs integers within [0,65535] so the
    // inverse actually wants a normal 16i domain.

    const unsigned long length = getArray().getLength();
    const unsigned long maxChannels = getArray().getMaxColorComponents();
    const unsigned long activeChannels = getArray().getNumColorComponents();
    const Array::Values& values = getArray().getValues();

    // As noted elsewhere, the InBitDepth describes the scaling of the LUT entries.
    const float normalMin = 0.0f;
    const float normalMax = GetBitDepthMaxValue(getInputBitDepth());

    unsigned long minInd = 0;
    unsigned long maxInd = length - 1;
    if (isInputHalfDomain())
    {
        minInd = 64511u;  // last value before -inf
        maxInd = 31743u;  // last value before +inf
    }

    // prepareArray has made the LUT either non-increasing or non-decreasing,
    // so the min and max values will be either the first or last LUT entries.
    for (unsigned long c = 0; c < activeChannels; ++c)
    {
        if (m_componentProperties[c].isIncreasing)
        {
            if (values[minInd * maxChannels + c] < normalMin ||
                values[maxInd * maxChannels + c] > normalMax)
            {
                return true;
            }
        }
        else
        {
            if (values[minInd * maxChannels + c] > normalMax ||
                values[maxInd * maxChannels + c] < normalMin)
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

void Lut1DOpData::prepareArray()
{
    // Note: Data allocated for the array is length*getMaxColorComponents().
    const unsigned long length = getArray().getLength();
    const unsigned long maxChannels = getArray().getMaxColorComponents();
    const unsigned long activeChannels = getArray().getNumColorComponents();
    Array::Values& values = getArray().getValues();

    for (unsigned long c = 0; c < activeChannels; ++c)
    {
        // Determine if the LUT is overall increasing or decreasing.
        // The heuristic used is to compare first and last entries.
        // (Note flat LUTs (arbitrarily) have isIncreasing == false.)
        unsigned long lowInd = c;
        unsigned long highInd = (length - 1) * maxChannels + c;
        if (isInputHalfDomain())
        {
            // For half-domain LUTs, I am concerned that customer LUTs may not
            // correctly populate the whole domain, so using -HALF_MAX and
            // +HALF_MAX could potentially give unreliable results.
            // Just using 0 and 1 for now.
            lowInd = 0u * maxChannels + c;       // 0.0
            highInd = 15360u * maxChannels + c;  // 15360 == 1.0
        }

        {
            m_componentProperties[c].isIncreasing
                = (values[lowInd] < values[highInd]);
        }

        // Flatten reversals.
        // (If the LUT has a reversal, there is not a unique inverse.
        // Furthermore we require sorted values for the exact eval algorithm.)
        {
            bool isIncreasing = m_componentProperties[c].isIncreasing;

            if (!isInputHalfDomain())
            {
                float prevValue = values[c];
                for (unsigned long idx = c + maxChannels;
                     idx < length * maxChannels;
                     idx += maxChannels)
                {
                    if (isIncreasing != (values[idx] > prevValue))
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
                unsigned long startInd = 0u * maxChannels + c; // 0 == +zero
                unsigned long endInd = 31744u * maxChannels;   // 31744 == +infinity
                float prevValue = values[startInd];
                for (unsigned long idx = startInd + maxChannels;
                     idx <= endInd;
                     idx += maxChannels)
                {
                    if (isIncreasing != (values[idx] > prevValue))
                    {
                        values[idx] = prevValue;
                    }
                    else
                    {
                        prevValue = values[idx];
                    }
                }

                // Do negative numbers.
                isIncreasing = !isIncreasing;
                startInd = 32768u * maxChannels + c;      // 32768 == -zero
                endInd = 64512u * maxChannels;            // 64512 == -infinity
                prevValue = values[c];  // prev value for -0 is +0 (disallow overlaps)
                for (unsigned long idx = startInd; idx <= endInd; idx += maxChannels)
                {
                    if (isIncreasing != (values[idx] > prevValue))
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

        // Determine effective domain from the starting/ending flat spots.
        // (If the LUT begins or ends with a flat spot, the inverse should be
        // the value nearest the center of the LUT.)
        // For constant LUTs, the end domain == start domain == 0.
        {
            if (!isInputHalfDomain())
            {
                unsigned long endDomain = length - 1;
                const float endValue = values[endDomain * maxChannels + c];
                while (endDomain > 0
                    && values[(endDomain - 1) * maxChannels + c] == endValue)
                {
                    --endDomain;
                }

                unsigned long startDomain = 0;
                const float startValue = values[startDomain * maxChannels + c];
                // Note that this works for both increasing and decreasing LUTs
                // since there is no reqmt that startValue < endValue.
                while (startDomain < endDomain
                    &&  values[(startDomain + 1) * maxChannels + c] == startValue)
                {
                    ++startDomain;
                }

                m_componentProperties[c].startDomain = startDomain;
                m_componentProperties[c].endDomain = endDomain;
            }
            else
            {
                // Question: Should the value for infinity be considered for
                // interpolation? The advantage is that in theory, if infinity
                // is mapped to some value by the forward LUT, you could
                // restore that value to infinity in the inverse.
                // This does seem to work in INV_EXACT mode (e.g.
                // CPURendererInvLUT1DHalf_fclut unit test).
                // TODO: Test to be ported CPURenderer_cases.cpp_inc
                // The problem is that in INV_FAST mode, there are Infs in the fast
                // LUT and these seem to make the value for both inf and 65504
                // a NaN. Limiting the effective domain allows 65504 to invert
                // correctly.
                unsigned long endDomain = 31743u;    // +65504 = largest half value < inf
                const float endValue = values[endDomain * maxChannels + c];
                while (endDomain > 0
                    && values[(endDomain - 1) * maxChannels + c] == endValue)
                {
                    --endDomain;
                }

                unsigned long startDomain = 0;       // positive zero
                const float startValue = values[startDomain * maxChannels + c];
                // Note that this works for both increasing and decreasing LUTs
                // since there is no reqmt that startValue < endValue.
                while (startDomain < endDomain
                    &&  values[(startDomain + 1) * maxChannels + c] == startValue)
                {
                    ++startDomain;
                }

                m_componentProperties[c].startDomain = startDomain;
                m_componentProperties[c].endDomain = endDomain;

                // Negative half of domain has its own start/end.
                unsigned long negEndDomain = 64511u;  // -65504 = last value before neg inf
                const float negEndValue = values[negEndDomain * maxChannels + c];
                while (negEndDomain > 32768u     // negative zero
                    && values[(negEndDomain - 1) * maxChannels + c] == negEndValue)
                {
                    --negEndDomain;
                }

                unsigned long negStartDomain = 32768u; // negative zero
                const float negStartValue = values[negStartDomain * maxChannels + c];
                while (negStartDomain < negEndDomain
                    && values[(negStartDomain + 1) * maxChannels + c] == negStartValue)
                {
                    ++negStartDomain;
                }

                m_componentProperties[c].negStartDomain = negStartDomain;
                m_componentProperties[c].negEndDomain = negEndDomain;
            }
        }
    }

    if (activeChannels == 1)
    {
        m_componentProperties[2] = m_componentProperties[1] = m_componentProperties[0];
    }
}



}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"


OIIO_ADD_TEST(Lut1DOpData, get_lut_ideal_size)
{
    OIIO_CHECK_EQUAL(OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_UINT8), 256);
    OIIO_CHECK_EQUAL(OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_UINT16), 65536);

    OIIO_CHECK_EQUAL(OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_F16), 65536);
    OIIO_CHECK_EQUAL(OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_F32), 65536);
}

OIIO_ADD_TEST(Lut1DOpData, constructor)
{
    OCIO::Lut1DOpData lut(2);

    OIIO_CHECK_ASSERT(lut.getType() == OCIO::OpData::Lut1DType);
    OIIO_CHECK_ASSERT(!lut.isNoOp());
    OIIO_CHECK_ASSERT(lut.isIdentity());
    OIIO_CHECK_EQUAL(lut.getArray().getLength(), 2);
    OIIO_CHECK_EQUAL(lut.getInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(lut.validate());
}

OIIO_ADD_TEST(Lut1DOpData, accessors)
{
    OCIO::Lut1DOpData l(17);
    l.setInputBitDepth(OCIO::BIT_DEPTH_UINT10);
    l.setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    l.setInterpolation(OCIO::INTERP_LINEAR);

    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_ASSERT(!l.isNoOp());
    OIIO_CHECK_ASSERT(l.isIdentity());
    OIIO_CHECK_NO_THROW(l.validate());

    OIIO_CHECK_EQUAL(l.getHueAdjust(), OCIO::Lut1DOpData::HUE_NONE);
    l.setHueAdjust(OCIO::Lut1DOpData::HUE_DW3);
    OIIO_CHECK_EQUAL(l.getHueAdjust(), OCIO::Lut1DOpData::HUE_DW3);

    // Note: Hue and Sat adjust do not affect identity status.
    OIIO_CHECK_ASSERT(l.isIdentity());

    l.getArray()[1] = 1.0f;
    OIIO_CHECK_ASSERT(!l.isNoOp());
    OIIO_CHECK_ASSERT(!l.isIdentity());
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_BEST);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_BEST);

    OIIO_CHECK_EQUAL(l.getArray().getLength(), 17);
    OIIO_CHECK_EQUAL(l.getArray().getNumValues(), 17 * 3);
    OIIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), 3);

    l.getArray().resize(65, 3);

    OIIO_CHECK_EQUAL(l.getArray().getLength(), 65);
    OIIO_CHECK_EQUAL(l.getArray().getNumValues(), 65 * 3);
    OIIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), 3);
    OIIO_CHECK_NO_THROW(l.validate());
}

OIIO_ADD_TEST(Lut1DOpData, identity_bitdepth)
{
    OCIO::Lut1DOpData l1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8,
                         "", OCIO::OpData::Descriptions(),
                         OCIO::INTERP_LINEAR,
                         OCIO::Lut1DOpData::LUT_STANDARD);
    OCIO::Lut1DOpData l2(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10,
                         "", OCIO::OpData::Descriptions(),
                         OCIO::INTERP_LINEAR,
                         OCIO::Lut1DOpData::LUT_STANDARD);

    OIIO_CHECK_ASSERT(!l1.isNoOp());
    OIIO_CHECK_ASSERT(l1.isIdentity());
    OIIO_CHECK_NO_THROW(l1.validate());

    OIIO_CHECK_ASSERT(!l2.isNoOp());
    OIIO_CHECK_ASSERT(l2.isIdentity());
    OIIO_CHECK_NO_THROW(l2.validate());

    const float coeff
        = OCIO::GetBitDepthMaxValue(l2.getOutputBitDepth())
        / OCIO::GetBitDepthMaxValue(l2.getInputBitDepth());

    // To validate the result, compute the expected results
    // not using the Lut1DOp algorithm.
    const float error = 1e-6f;
    for (unsigned long idx = 0; idx<l2.getArray().getLength(); ++idx)
    {
        OIIO_CHECK_CLOSE(l1.getArray().getValues()[idx * 3 + 0] * coeff,
                         l2.getArray().getValues()[idx * 3 + 0],
                         error);

        OIIO_CHECK_CLOSE(l1.getArray().getValues()[idx * 3 + 1] * coeff,
                         l2.getArray().getValues()[idx * 3 + 1],
                         error);

        OIIO_CHECK_CLOSE(l1.getArray().getValues()[idx * 3 + 2] * coeff,
                         l2.getArray().getValues()[idx * 3 + 2],
                         error);
    }

    OCIO::Lut1DOpData l3(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT8,
                         "", OCIO::OpData::Descriptions(),
                         OCIO::INTERP_LINEAR,
                         OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE);

    OIIO_CHECK_ASSERT(l3.isIdentity());
}

OIIO_ADD_TEST(Lut1DOpData, is_identity)
{
    OCIO::Lut1DOpData l1(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10,
                         "", OCIO::OpData::Descriptions(),
                         OCIO::INTERP_LINEAR,
                         OCIO::Lut1DOpData::LUT_STANDARD);

    OIIO_CHECK_ASSERT(l1.isIdentity());

    // The tolerance will be 1023*1e-5 = 0.01023.
    const unsigned long lastId = (unsigned long)l1.getArray().getValues().size() - 1;
    const float first = l1.getArray()[0];
    const float last = l1.getArray()[lastId];

    l1.getArray()[0] = first + 0.01f;
    l1.getArray()[lastId] = last + 0.01f;
    OIIO_CHECK_ASSERT(l1.isIdentity());

    l1.getArray()[0] = first + 0.02f;
    l1.getArray()[lastId] = last;
    OIIO_CHECK_ASSERT(!l1.isIdentity());

    l1.getArray()[0] = first;
    l1.getArray()[lastId] = last + 0.02f;
    OIIO_CHECK_ASSERT(!l1.isIdentity());


    OCIO::Lut1DOpData l2(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT10,
                         "", OCIO::OpData::Descriptions(),
                         OCIO::INTERP_LINEAR,
                         OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE);

    unsigned long id2 = 31700 * 3;
    const float first2 = l2.getArray()[0];
    const float last2 = l2.getArray()[id2];

    // U10 out bit-depth means values are scaled by 1023.f
    // (float)half(1) - (float)half(0) = 5.96046448e-08f
    // scaled -> 6.09755516e-5f
    const float ERROR_0 = 6.09755516e-5f;
    // (float)half(31701) - (float)half(31700) = 32.0f
    // scaled -> 32736.0f
    const float ERROR_31700 = 32736.0f;

    OIIO_CHECK_ASSERT(l2.isIdentity());

    l2.getArray()[0] = first2 + ERROR_0;
    l2.getArray()[id2] = last2 + ERROR_31700;

    OIIO_CHECK_ASSERT(l2.isIdentity());

    l2.getArray()[0] = first2 + 2 * ERROR_0;
    l2.getArray()[id2] = last2;

    OIIO_CHECK_ASSERT(!l2.isIdentity());

    l2.getArray()[0] = first2;
    l2.getArray()[id2] = last2 + 2 * ERROR_31700;

    OIIO_CHECK_ASSERT(!l2.isIdentity());
}

OIIO_ADD_TEST(Lut1DOpData, clone)
{
    OCIO::Lut1DOpData ref(20);
    ref.getArray()[1] = 0.5f;
    ref.setHueAdjust(OCIO::Lut1DOpData::HUE_DW3);

    OCIO::Lut1DOpDataRcPtr pClone = ref.clone();

    OIIO_CHECK_ASSERT(!pClone->isNoOp());
    OIIO_CHECK_ASSERT(!pClone->isIdentity());
    OIIO_CHECK_NO_THROW(pClone->validate());
    OIIO_CHECK_ASSERT(pClone->getArray() == ref.getArray());
    OIIO_CHECK_EQUAL(pClone->getHueAdjust(), OCIO::Lut1DOpData::HUE_DW3);
}

OIIO_ADD_TEST(Lut1DOpData, output_depth_scaling)
{
    OCIO::Lut1DOpData ref(OCIO::BIT_DEPTH_UINT8,
                          OCIO::BIT_DEPTH_UINT10,
                          OCIO::Lut1DOpData::LUT_STANDARD);

    // Get the array values and keep them to compare later.
    const OCIO::Array::Values initialArrValues = ref.getArray().getValues();
    const OCIO::BitDepth initialBitdepth = ref.getOutputBitDepth();

    const OCIO::BitDepth newBitdepth = OCIO::BIT_DEPTH_UINT16;

    const float factor = OCIO::GetBitDepthMaxValue(newBitdepth)
                         / OCIO::GetBitDepthMaxValue(initialBitdepth);

    ref.setOutputBitDepth(newBitdepth);
    // Now we need to make sure that the bitdepth was changed from the overriden
    // method.
    OIIO_CHECK_EQUAL(ref.getOutputBitDepth(), newBitdepth);

    // Now we need to check the scaling between the new array and initial array
    // matches the factor computed above.
    const OCIO::Array::Values& newArrValues = ref.getArray().getValues();

    // Sanity check first.
    OIIO_CHECK_EQUAL(initialArrValues.size(), newArrValues.size());

    float expectedValue = 0.0f;
    const float error = 1e-6f;
    for (unsigned long i = 0; i < newArrValues.size(); i++)
    {
        expectedValue = initialArrValues[i] * factor;
        OIIO_CHECK_CLOSE(expectedValue, newArrValues[i], error);
    }

}

OIIO_ADD_TEST(Lut1DOpData, equality_test)
{
    OCIO::Lut1DOpData l1(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10,
                         "", OCIO::OpData::Descriptions(),
                         OCIO::INTERP_LINEAR,
                         OCIO::Lut1DOpData::LUT_STANDARD);

    OCIO::Lut1DOpData l2(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10,
                         "", OCIO::OpData::Descriptions(),
                         OCIO::INTERP_NEAREST,
                         OCIO::Lut1DOpData::LUT_STANDARD);

    OIIO_CHECK_ASSERT(!(l1 == l2));

    OCIO::Lut1DOpData l3(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT10,
                         "", OCIO::OpData::Descriptions(),
                         OCIO::INTERP_LINEAR,
                         OCIO::Lut1DOpData::LUT_STANDARD);

    OIIO_CHECK_ASSERT(!(l1 == l3) && !(l3 == l2));

    OCIO::Lut1DOpData l4(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10,
                         "", OCIO::OpData::Descriptions(),
                         OCIO::INTERP_LINEAR,
                         OCIO::Lut1DOpData::LUT_STANDARD);

    OIIO_CHECK_ASSERT(l1 == l4);

    l1.setHueAdjust(OCIO::Lut1DOpData::HUE_DW3);

    OIIO_CHECK_ASSERT(!(l1 == l4));

    // Inversion quality does not affect forward ops equality.
    l4.setHueAdjust(OCIO::Lut1DOpData::HUE_DW3);
    l1.setInversionQuality(OCIO::LUT_INVERSION_BEST);

    OIIO_CHECK_ASSERT(l1 == l4);

    // Inversion quality does not affect inverse ops equality.
    // Even so applying the ops could lead to small differences.
    auto l5 = l1.inverse();
    auto l6 = l4.inverse();

    OIIO_CHECK_ASSERT(*l5 == *l6);
}

OIIO_ADD_TEST(Lut1DOpData, channel)
{
    OCIO::Lut1DOpDataRcPtr L1 = std::make_shared<OCIO::Lut1DOpData>(
        OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10,
        "", OCIO::OpData::Descriptions(),
        OCIO::INTERP_LINEAR,
        OCIO::Lut1DOpData::LUT_STANDARD,
        17);

    OCIO::ConstLut1DOpDataRcPtr L2 = std::make_shared<OCIO::Lut1DOpData>(
        OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10,
        "", OCIO::OpData::Descriptions(),
        OCIO::INTERP_LINEAR,
        OCIO::Lut1DOpData::LUT_STANDARD,
        20);

    // False: identity.
    OIIO_CHECK_ASSERT(!L1->hasChannelCrosstalk());
    OIIO_CHECK_ASSERT(L1->mayCompose(L2));

    L1->setHueAdjust(OCIO::Lut1DOpData::HUE_DW3);
    
    // True: hue restore is on, it's an identity LUT, but this is not
    // tested for efficency.
    OIIO_CHECK_ASSERT(L1->hasChannelCrosstalk());
    
    OIIO_CHECK_ASSERT(!L1->mayCompose(L2));

    OCIO::ConstLut1DOpDataRcPtr L1C = L1;
    OIIO_CHECK_ASSERT(!L2->mayCompose(L1C));

    L1->setHueAdjust(OCIO::Lut1DOpData::HUE_NONE);
    L1->getArray()[1] = 3.0f;
    // False: non-identity.
    OIIO_CHECK_ASSERT(!L1->hasChannelCrosstalk());

    L1->setHueAdjust(OCIO::Lut1DOpData::HUE_DW3);
    // True: non-identity w/hue restore.
    OIIO_CHECK_ASSERT(L1->hasChannelCrosstalk());
}

OIIO_ADD_TEST(Lut1DOpData, interpolation)
{
    OCIO::Lut1DOpData l(17);
    l.setInputBitDepth(OCIO::BIT_DEPTH_F32);
    l.setOutputBitDepth(OCIO::BIT_DEPTH_F32);

    l.setInterpolation(OCIO::INTERP_LINEAR);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_BEST);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_BEST);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_CUBIC);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_CUBIC);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "does not support interpolation algorithm");

    l.setInterpolation(OCIO::INTERP_DEFAULT);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_DEFAULT);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    // TODO: INTERP_NEAREST is currently implemented as INTERP_LINEAR.
    l.setInterpolation(OCIO::INTERP_NEAREST);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_NEAREST);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    // Invalid interpolation type do not get translated
    // by getConcreteInterpolation.
    l.setInterpolation(OCIO::INTERP_UNKNOWN);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_UNKNOWN);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "does not support interpolation algorithm");

    l.setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, " does not support interpolation algorithm");
}

OIIO_ADD_TEST(Lut1DOpData, inversion_quality)
{
    OCIO::Lut1DOpData l(17);
    l.setInputBitDepth(OCIO::BIT_DEPTH_F32);
    l.setOutputBitDepth(OCIO::BIT_DEPTH_F32);

    l.setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    OIIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_EXACT);
    OIIO_CHECK_EQUAL(l.getConcreteInversionQuality(), OCIO::LUT_INVERSION_EXACT);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInversionQuality(OCIO::LUT_INVERSION_FAST);
    OIIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_FAST);
    OIIO_CHECK_EQUAL(l.getConcreteInversionQuality(), OCIO::LUT_INVERSION_FAST);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInversionQuality(OCIO::LUT_INVERSION_DEFAULT);
    OIIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_DEFAULT);
    OIIO_CHECK_EQUAL(l.getConcreteInversionQuality(), OCIO::LUT_INVERSION_FAST);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInversionQuality(OCIO::LUT_INVERSION_BEST);
    OIIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_BEST);
    OIIO_CHECK_EQUAL(l.getConcreteInversionQuality(), OCIO::LUT_INVERSION_EXACT);
    OIIO_CHECK_NO_THROW(l.validate());
}

OIIO_ADD_TEST(Lut1DOpData, lut_1d_compose)
{
    OCIO::Lut1DOpDataRcPtr lut1 =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::BIT_DEPTH_F32,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::Lut1DOpData::LUT_STANDARD);

    lut1->getArray().resize(8, 3);
    float * values = &lut1->getArray().getValues()[0];

    values[0]  = 0.0f;      values[1]  = 0.0f;      values[2]  = 0.002333f;
    values[3]  = 0.0f;      values[4]  = 0.291341f; values[5]  = 0.015624f;
    values[6]  = 0.106521f; values[7]  = 0.334331f; values[8]  = 0.462431f;
    values[9]  = 0.515851f; values[10] = 0.474151f; values[11] = 0.624611f;
    values[12] = 0.658791f; values[13] = 0.527381f; values[14] = 0.685071f;
    values[15] = 0.908501f; values[16] = 0.707951f; values[17] = 0.886331f;
    values[18] = 0.926671f; values[19] = 0.846431f; values[20] = 1.0f;
    values[21] = 1.0f;      values[22] = 1.0f;      values[23] = 1.0f;

    OCIO::Lut1DOpDataRcPtr lut1Cloned = lut1->clone();

    OCIO::Lut1DOpDataRcPtr lut2 =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::BIT_DEPTH_F32,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::Lut1DOpData::LUT_STANDARD);

    lut2->getArray().resize(8, 3);
    values = &lut2->getArray().getValues()[0];

    values[0]  = 0.0f;        values[1]  = 0.0f;       values[2]  = 0.0023303f;
    values[3]  = 0.0f;        values[4]  = 0.0029134f; values[5]  = 0.015624f;
    values[6]  = 0.00010081f; values[7]  = 0.0059806f; values[8]  = 0.023362f;
    values[9]  = 0.0045628f;  values[10] = 0.024229f;  values[11] = 0.05822f;
    values[12] = 0.0082598f;  values[13] = 0.033831f;  values[14] = 0.074063f;
    values[15] = 0.028595f;   values[16] = 0.075003f;  values[17] = 0.13552f;
    values[18] = 0.69154f;    values[19] = 0.9213f;    values[20] = 1.0f;
    values[21] = 0.76038f;    values[22] = 1.0f;       values[23] = 1.0f;

    OCIO::ConstLut1DOpDataRcPtr lut2C = lut2;

    {
        OCIO::Lut1DOpData::Compose(lut1,
                                   lut2C,
                                   OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO);

        values = &lut1->getArray().getValues()[0];

        OIIO_CHECK_EQUAL(lut1->getArray().getLength(), 8);

        OIIO_CHECK_CLOSE(values[0], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[1], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[2], 0.00254739914f, 1e-6f);

        OIIO_CHECK_CLOSE(values[3], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[4], 0.00669934973f, 1e-6f);
        OIIO_CHECK_CLOSE(values[5], 0.00378420483f, 1e-6f);

        OIIO_CHECK_CLOSE(values[6], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[7], 0.0121908365f, 1e-6f);
        OIIO_CHECK_CLOSE(values[8], 0.0619750582f, 1e-6f);

        OIIO_CHECK_CLOSE(values[9], 0.00682150759f, 1e-6f);
        OIIO_CHECK_CLOSE(values[10], 0.0272925831f, 1e-6f);
        OIIO_CHECK_CLOSE(values[11], 0.096942015f, 1e-6f);

        OIIO_CHECK_CLOSE(values[12], 0.0206955168f, 1e-6f);
        OIIO_CHECK_CLOSE(values[13], 0.0308703855f, 1e-6f);
        OIIO_CHECK_CLOSE(values[14], 0.12295182f, 1e-6f);

        OIIO_CHECK_CLOSE(values[15], 0.716288447f, 1e-6f);
        OIIO_CHECK_CLOSE(values[16], 0.0731772855f, 1e-6f);
        OIIO_CHECK_CLOSE(values[17], 1.0f, 1e-6f);

        OIIO_CHECK_CLOSE(values[18], 0.725044191f, 1e-6f);
        OIIO_CHECK_CLOSE(values[19], 0.857842028f, 1e-6f);
        OIIO_CHECK_CLOSE(values[20], 1.0f, 1e-6f);
    }

    {
        OCIO::Lut1DOpData::Compose(lut1Cloned,
                                   lut2C,
                                   OCIO::Lut1DOpData::COMPOSE_RESAMPLE_INDEPTH);

        values = &lut1Cloned->getArray().getValues()[0];

        OIIO_CHECK_EQUAL(lut1Cloned->getArray().getLength(), 65536);

        OIIO_CHECK_CLOSE(values[0], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[1], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[2], 0.00254739914f, 1e-6f);

        OIIO_CHECK_CLOSE(values[3], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[4], 6.34463504e-07f, 1e-6f);
        OIIO_CHECK_CLOSE(values[5], 0.00254753046f, 1e-6f);

        OIIO_CHECK_CLOSE(values[6], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[7], 1.26915984e-06f, 1e-6f);
        OIIO_CHECK_CLOSE(values[8], 0.00254766271f, 1e-6f);

        OIIO_CHECK_CLOSE(values[9], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[10], 1.90362334e-06f, 1e-6f);
        OIIO_CHECK_CLOSE(values[11], 0.00254779495f, 1e-6f);

        OIIO_CHECK_CLOSE(values[12], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[13], 2.53855251e-06f, 1e-6f);
        OIIO_CHECK_CLOSE(values[14], 0.0025479272f, 1e-6f);

        OIIO_CHECK_CLOSE(values[15], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[16], 3.17324884e-06f, 1e-6f);
        OIIO_CHECK_CLOSE(values[17], 0.00254805945f, 1e-6f);

        OIIO_CHECK_CLOSE(values[300], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[301], 6.3463347e-05f, 1e-6f);
        OIIO_CHECK_CLOSE(values[302], 0.00256060902f, 1e-6f);

        OIIO_CHECK_CLOSE(values[900], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[901], 0.000190390972f, 1e-6f);
        OIIO_CHECK_CLOSE(values[902], 0.00258703064f, 1e-6f);

        OIIO_CHECK_CLOSE(values[2700], 0.0f, 1e-6f);
        OIIO_CHECK_CLOSE(values[2701], 0.000571172219f, 1e-6f);
        OIIO_CHECK_CLOSE(values[2702], 0.00266629551f, 1e-6f);
    }
}

OIIO_ADD_TEST(Lut1DOpData, lut_1d_compose_sc)
{
    OCIO::Lut1DOpDataRcPtr lut1 =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::BIT_DEPTH_UINT8,
                                            OCIO::BIT_DEPTH_UINT8,
                                            OCIO::Lut1DOpData::LUT_STANDARD);

    lut1->getArray().resize(2, 3);
    float * values = &lut1->getArray().getValues()[0];
    values[0] = 64.f;  values[1] = 64.f;   values[2] = 64.f;
    values[3] = 196.f; values[4] = 196.f;  values[5] = 196.f;

    OCIO::Lut1DOpDataRcPtr lut2 =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::BIT_DEPTH_UINT8,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::Lut1DOpData::LUT_STANDARD);

    lut2->getArray().resize(32, 3);
    values = &lut2->getArray().getValues()[0];

    values[0] = 0.0000000f;   values[1] = 0.0000000f;  values[2] = 0.0023303f;
    values[3] = 0.0000000f;   values[4] = 0.0001869f;  values[5] = 0.0052544f;
    values[6] = 0.0000000f;   values[7] = 0.0010572f;  values[8] = 0.0096338f;
    values[9] = 0.0000000f;  values[10] = 0.0029134f; values[11] = 0.0156240f;
    values[12] = 0.0001008f; values[13] = 0.0059806f; values[14] = 0.0233620f;
    values[15] = 0.0007034f; values[16] = 0.0104480f; values[17] = 0.0329680f;
    values[18] = 0.0021120f; values[19] = 0.0164810f; values[20] = 0.0445540f;
    values[21] = 0.0045628f; values[22] = 0.0242290f; values[23] = 0.0582200f;
    values[24] = 0.0082598f; values[25] = 0.0338310f; values[26] = 0.0740630f;
    values[27] = 0.0133870f; values[28] = 0.0454150f; values[29] = 0.0921710f;
    values[30] = 0.0201130f; values[31] = 0.0591010f; values[32] = 0.1126300f;
    values[33] = 0.0285950f; values[34] = 0.0750030f; values[35] = 0.1355200f;
    values[36] = 0.0389830f; values[37] = 0.0932290f; values[38] = 0.1609100f;
    values[39] = 0.0514180f; values[40] = 0.1138800f; values[41] = 0.1888800f;
    values[42] = 0.0660340f; values[43] = 0.1370600f; values[44] = 0.2195000f;
    values[45] = 0.0829620f; values[46] = 0.1628600f; values[47] = 0.2528300f;
    values[48] = 0.1023300f; values[49] = 0.1913800f; values[50] = 0.2889500f;
    values[51] = 0.1242500f; values[52] = 0.2227000f; values[53] = 0.3279000f;
    values[54] = 0.1488500f; values[55] = 0.2569100f; values[56] = 0.3697600f;
    values[57] = 0.1762300f; values[58] = 0.2940900f; values[59] = 0.4145900f;
    values[60] = 0.2065200f; values[61] = 0.3343300f; values[62] = 0.4624300f;
    values[63] = 0.2398200f; values[64] = 0.3777000f; values[65] = 0.5133400f;
    values[66] = 0.2762200f; values[67] = 0.4242800f; values[68] = 0.5673900f;
    values[69] = 0.3158500f; values[70] = 0.4741500f; values[71] = 0.6246100f;
    values[72] = 0.3587900f; values[73] = 0.5273800f; values[74] = 0.6850700f;
    values[75] = 0.4051500f; values[76] = 0.5840400f; values[77] = 0.7488100f;
    values[78] = 0.4550200f; values[79] = 0.6442100f; values[80] = 0.8158800f;
    values[81] = 0.5085000f; values[82] = 0.7079500f; values[83] = 0.8863300f;
    values[84] = 0.5656900f; values[85] = 0.7753400f; values[86] = 0.9602100f;
    values[87] = 0.6266700f; values[88] = 0.8464300f; values[89] = 1.0000000f;
    values[90] = 0.6915400f; values[91] = 0.9213000f; values[92] = 1.0000000f;
    values[93] = 0.7603800f; values[94] = 1.0000000f; values[95] = 1.0000000f;

    OCIO::ConstLut1DOpDataRcPtr lut2C = lut2;

    {
        OCIO::Lut1DOpDataRcPtr lComp = lut1->clone();
        OIIO_CHECK_NO_THROW(
            OCIO::Lut1DOpData::Compose(lComp, lut2C, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO));

        OIIO_CHECK_EQUAL(lComp->getArray().getLength(), 2);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[0], 0.00744791f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[1], 0.03172233f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[2], 0.07058375f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[3], 0.3513808f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[4], 0.51819527f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[5], 0.67463773f, 1e-6f);
    }

    {
        OCIO::Lut1DOpDataRcPtr lComp = lut1->clone();
        OIIO_CHECK_NO_THROW(
            OCIO::Lut1DOpData::Compose(lComp, lut2C, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_INDEPTH));

        OIIO_CHECK_EQUAL(lComp->getArray().getLength(), 256);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[0], 0.00744791f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[1], 0.03172233f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[2], 0.07058375f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[383], 0.28073114f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[384], 0.09914176f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[385], 0.1866852f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[765], 0.3513808f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[766], 0.51819527f, 1e-6f);
        OIIO_CHECK_CLOSE(lComp->getArray().getValues()[767], 0.67463773f, 1e-6f);
    }

    {
        OCIO::ConstLut1DOpDataRcPtr lut1C = lut1;
        OCIO::Lut1DOpDataRcPtr lComp = lut2->clone();
        OIIO_CHECK_THROW_WHAT(
            OCIO::Lut1DOpData::Compose(lComp, lut1C, OCIO::Lut1DOpData::COMPOSE_RESAMPLE_NO),
            OCIO::Exception, "bit-depth mismatch forbids the composition");

    }
}

namespace
{
    const char uid[] = "uid";
    const OCIO::OpData::Descriptions desc;
};

// Validate the input and output bit-depths and half domain.
void checkInverse_bitDepths_domain(
    const OCIO::BitDepth refInputBitDepth,
    const OCIO::BitDepth refOutputBitDepth,
    const OCIO::Interpolation refInterpolationAlgo,
    const OCIO::Lut1DOpData::HalfFlags refHalfFlags,
    const OCIO::BitDepth expectedInvInputBitDepth,
    const OCIO::BitDepth expectedInvOutputBitDepth,
    const OCIO::Interpolation expectedInvInterpolationAlgo,
    const OCIO::Lut1DOpData::HalfFlags expectedInvHalfFlags)
{
    OCIO::Lut1DOpData refLut1d(
        refInputBitDepth, refOutputBitDepth,
        uid, desc,
        refInterpolationAlgo,
        refHalfFlags);

    // Get inverse of reference 1D LUT operation.
    OCIO::Lut1DOpDataRcPtr invLut1d;
    OIIO_CHECK_NO_THROW(invLut1d = refLut1d.inverse());
    OIIO_REQUIRE_ASSERT(invLut1d);

    OIIO_CHECK_EQUAL(invLut1d->getInputBitDepth(),
                     expectedInvInputBitDepth);

    OIIO_CHECK_EQUAL(invLut1d->getOutputBitDepth(),
                     expectedInvOutputBitDepth);

    OIIO_CHECK_EQUAL(invLut1d->getInterpolation(),
                     expectedInvInterpolationAlgo);

    OIIO_CHECK_EQUAL(invLut1d->getHalfFlags(),
                     expectedInvHalfFlags);

    OIIO_CHECK_EQUAL(invLut1d->getHueAdjust(),
                     OCIO::Lut1DOpData::HUE_NONE);
}


OIIO_ADD_TEST(Lut1DOpData, inverse_bitDepth_domain)
{
    checkInverse_bitDepths_domain(
        // Reference
        OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_F32,
        OCIO::INTERP_DEFAULT,
        OCIO::Lut1DOpData::LUT_STANDARD,
        // Expected
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT8,
        OCIO::INTERP_DEFAULT,
        OCIO::Lut1DOpData::LUT_STANDARD);

    checkInverse_bitDepths_domain(
        // Reference
        OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT12,
        OCIO::INTERP_LINEAR,
        OCIO::Lut1DOpData::LUT_STANDARD,
        // Expected
        OCIO::BIT_DEPTH_UINT12, OCIO::BIT_DEPTH_F16,
        OCIO::INTERP_LINEAR,
        OCIO::Lut1DOpData::LUT_STANDARD);

    checkInverse_bitDepths_domain(
        // Reference
        OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32,
        OCIO::INTERP_BEST,
        OCIO::Lut1DOpData::LUT_STANDARD,
        // Expected
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F16,
        OCIO::INTERP_BEST,
        OCIO::Lut1DOpData::LUT_STANDARD);

    checkInverse_bitDepths_domain(
        // Reference
        OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT12,
        OCIO::INTERP_BEST,
        OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE,
        // Expected
        OCIO::BIT_DEPTH_UINT12, OCIO::BIT_DEPTH_F16,
        OCIO::INTERP_BEST,
        OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE);
}

OIIO_ADD_TEST(Lut1DOpData, inverse_hueadjust)
{
    OCIO::Lut1DOpData refLut1d(OCIO::BIT_DEPTH_F32,
                               OCIO::BIT_DEPTH_F32,
                               uid, desc,
                               OCIO::INTERP_BEST,
                               OCIO::Lut1DOpData::LUT_STANDARD);

    refLut1d.setHueAdjust(OCIO::Lut1DOpData::HUE_DW3);

    // Get inverse of reference lut1d operation.
    OCIO::Lut1DOpDataRcPtr invLut1d;
    OIIO_CHECK_NO_THROW(invLut1d = refLut1d.inverse());
    OIIO_REQUIRE_ASSERT(invLut1d);

    OIIO_CHECK_EQUAL(invLut1d->getHueAdjust(),
                     OCIO::Lut1DOpData::HUE_DW3);
}

OIIO_ADD_TEST(Lut1DOpData, is_inverse)
{
    // Create forward LUT.
    OCIO::Lut1DOpDataRcPtr L1 = std::make_shared<OCIO::Lut1DOpData>(
        OCIO::BIT_DEPTH_UINT8,
        OCIO::BIT_DEPTH_UINT10,
        uid, desc,
        OCIO::INTERP_LINEAR,
        OCIO::Lut1DOpData::LUT_STANDARD,
        5);
    
    // Make it not an identity.
    OCIO::Array & array = L1->getArray();
    OCIO::Array::Values & values = array.getValues();
    values[0] = 20.f;
    OIIO_CHECK_ASSERT(!L1->isIdentity());

    // Create an inverse LUT with same basics.
    OCIO::Lut1DOpDataRcPtr L2 = L1->inverse();
    OIIO_REQUIRE_ASSERT(L2);

    OIIO_CHECK_ASSERT(!(*L1 == *L2));

    OCIO::ConstLut1DOpDataRcPtr L1C = L1;
    OCIO::ConstLut1DOpDataRcPtr L2C = L2;

    // Check isInverse.
    OIIO_CHECK_ASSERT(L1->isInverse(L2C));
    OIIO_CHECK_ASSERT(L2->isInverse(L1C));

    // Arrays are the same if you normalize for the scaling difference.
    L1->setOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_ASSERT(L1->isInverse(L2C));
    OIIO_CHECK_ASSERT(L2->isInverse(L1C));

    // Catch the situation where the arrays are the same even though the
    // bit-depths don't match and hence the arrays effectively aren't the same.
    L1->setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);  // restore original
    OIIO_CHECK_ASSERT(L1->isInverse(L2C));
    L1->OpData::setOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_ASSERT(!L1->isInverse(L2C));
    OIIO_CHECK_ASSERT(!L2->isInverse(L1C));
}

void SetLutArray(OCIO::Lut1DOpData & op,
                 unsigned long dimension,
                 unsigned long channels,
                 const float * data)
{
    OCIO::Array & refArray = op.getArray();
    refArray.resize(dimension, channels);
    // The data allocated for the array is dimension * getMaxColorComponents(),
    // not dimension * channels.

    const unsigned long maxChannels = refArray.getMaxColorComponents();
    OCIO::Array::Values & values = refArray.getValues();
    if (channels == maxChannels)
    {
        memcpy(&(values[0]), data, dimension * channels * sizeof(float));
    }
    else
    {
        // Set the red component, fill other with zero values.
        for (unsigned long i = 0; i < dimension; ++i)
        {
            values[i*maxChannels + 0] = data[i];
            values[i*maxChannels + 1] = 0.f;
            values[i*maxChannels + 2] = 0.f;
        }
    }
}

// Validate the overall increase/decrease and effective domain.
void CheckInverse_IncreasingEffectiveDomain(
    unsigned long dimension, unsigned long channels, const float* fwdArrayData,
    bool expIncreasingR, unsigned long expStartDomainR, unsigned long expEndDomainR,
    bool expIncreasingG, unsigned long expStartDomainG, unsigned long expEndDomainG,
    bool expIncreasingB, unsigned long expStartDomainB, unsigned long expEndDomainB)
{
    OCIO::Lut1DOpData refLut1dOp(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                 uid, desc,
                                 OCIO::INTERP_BEST,
                                 OCIO::Lut1DOpData::LUT_STANDARD);

    SetLutArray(refLut1dOp, dimension, channels, fwdArrayData);

    OCIO::Lut1DOpDataRcPtr invLut1dOp = refLut1dOp.inverse();
    OIIO_REQUIRE_ASSERT(invLut1dOp);
    OIIO_CHECK_NO_THROW(invLut1dOp->finalize());

    const OCIO::Lut1DOpData::ComponentProperties &
        redProperties = invLut1dOp->getRedProperties();

    const OCIO::Lut1DOpData::ComponentProperties &
        greenProperties = invLut1dOp->getGreenProperties();

    const OCIO::Lut1DOpData::ComponentProperties &
        blueProperties = invLut1dOp->getBlueProperties();

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

OIIO_ADD_TEST(Lut1DOpData, inverse_increasing_effective_domain)
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

// Validate the flatten algorithms.
void CheckInverse_Flatten(unsigned long dimension,
                          unsigned long channels,
                          const float * fwdArrayData,
                          const float * expInvArrayData)
{
    OCIO::Lut1DOpData refLut1dOp(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                                 uid, desc,
                                 OCIO::INTERP_BEST,
                                 OCIO::Lut1DOpData::LUT_STANDARD);

    SetLutArray(refLut1dOp, dimension, channels, fwdArrayData);

    OCIO::Lut1DOpDataRcPtr invLut1dOp = refLut1dOp.inverse();
    OIIO_REQUIRE_ASSERT(invLut1dOp);
    invLut1dOp->finalize();

    const OCIO::Array::Values &
        invValues = invLut1dOp->getArray().getValues();

    for (unsigned long i = 0; i < dimension * channels; ++i)
    {
        OIIO_CHECK_EQUAL(invValues[i], expInvArrayData[i]);
    }
}

OIIO_ADD_TEST(Lut1DOpData, inverse_flatten_test)
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

void SetLutArrayHalf(const OCIO::Lut1DOpDataRcPtr & op, unsigned long channels)
{
    const unsigned long dimension = 65536u;
    OCIO::Array & refArray = op->getArray();
    refArray.resize(dimension, channels);
    // The data allocated for the array is dimension * getMaxColorComponents(),
    // not dimension * channels.

    const unsigned long maxChannels = refArray.getMaxColorComponents();
    OCIO::Array::Values & values = refArray.getValues();
    for (unsigned long j = 0u; j < channels; j++)
    {
        for (unsigned long i = 0u; i < 65536u; i++)
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
                if (i < 32768u)                 // decreasing function
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

OIIO_ADD_TEST(Lut1DOpData, inverse_half_domain)
{
    OCIO::Lut1DOpDataRcPtr refLut1dOp = std::make_shared<OCIO::Lut1DOpData>(
        OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
        uid, desc,
        OCIO::INTERP_BEST,
        OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE);

    SetLutArrayHalf(refLut1dOp, 3u);

    OCIO::Lut1DOpDataRcPtr invLut1dOp = refLut1dOp->inverse();
    OIIO_REQUIRE_ASSERT(invLut1dOp);
    invLut1dOp->finalize();

    const OCIO::Lut1DOpData::ComponentProperties &
        redProperties = invLut1dOp->getRedProperties();

    const OCIO::Lut1DOpData::ComponentProperties &
        greenProperties = invLut1dOp->getGreenProperties();

    const OCIO::Lut1DOpData::ComponentProperties &
        blueProperties = invLut1dOp->getBlueProperties();

    const OCIO::Array::Values &
        invValues = invLut1dOp->getArray().getValues();

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
    OIIO_CHECK_EQUAL(act.bits(), 15922);  // halfToFloat(15000) * 2 - 0.1
    act = invValues[52000 * 3];
    OIIO_CHECK_EQUAL(act.bits(), 51567);  // halfToFloat(50000) * 3 + 0.1
    act = invValues[16000 * 3 + 1];
    OIIO_CHECK_EQUAL(act.bits(), 46662);  // halfToFloat(15000) * -0.5 + 0.02
    act = invValues[52000 * 3 + 1];
    OIIO_CHECK_EQUAL(act.bits(), 15885);  // halfToFloat(50000) * -0.4 + 0.05

    bool reversal = false;
    for (unsigned long i = 1; i < 31745; i++)
    {
        if (invValues[i * 3] < invValues[(i - 1) * 3])           // increasing red
            reversal = true;
    }
    OIIO_CHECK_ASSERT(!reversal);
    // Check no overlap at +0 and -0.
    OIIO_CHECK_ASSERT(invValues[0] >= invValues[32768 * 3]);
    reversal = false;
    for (unsigned long i = 1; i < 31745; i++)
    {
        if (invValues[i * 3 + 1] > invValues[(i - 1) * 3 + 1])   // decreasing grn
            reversal = true;
    }
    OIIO_CHECK_ASSERT(!reversal);
    OIIO_CHECK_ASSERT(invValues[0 + 1] <= invValues[32768 * 3 + 1]);
    reversal = false;
    for (unsigned long i = 1; i < 31745; i++)
    {
        if (invValues[i * 3 + 2] < invValues[(i - 1) * 3 + 2])   // increasing blu
            reversal = true;
    }
    OIIO_CHECK_ASSERT(!reversal);
    OIIO_CHECK_ASSERT(invValues[0 + 2] >= invValues[32768 * 3 + 2]);
}


// TODO: Port syncolor test: renderer\test\invLutUtil_test.cpp - GPURendererInvLut1D_LutSize_ExtendedDomain
// TODO: Port syncolor test: renderer\test\invLutUtil_test.cpp - GPURendererInvLut1D_LutSize_GPUOpt
// TODO: Port syncolor test: renderer\test\invLutUtil_test.cpp - GPURendererInvLut1D_LutSize_HalfDomain
// TODO: Port syncolor test: renderer\test\invLutUtil_test.cpp - GPURendererInvLut3D_LutSize

#endif // OCIO_UNIT_TEST

