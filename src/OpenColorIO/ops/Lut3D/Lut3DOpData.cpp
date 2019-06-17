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

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "md5/md5.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ops/Lut3D/Lut3DOpData.h"
#include "ops/Range/RangeOpData.h"
#include "OpTools.h"
#include "Platform.h"

OCIO_NAMESPACE_ENTER
{

Lut3DOpDataRcPtr MakeFastLut3DFromInverse(ConstLut3DOpDataRcPtr & lut)
{
    if (lut->getDirection() != TRANSFORM_DIR_INVERSE)
    {
        throw Exception("MakeFastLut3DFromInverse expects an inverse LUT");
    }

    // The composition needs to use the EXACT renderer.
    // (Also avoids infinite loop.)
    // So temporarily set the style to EXACT.
    LutStyleGuard<Lut3DOpData> guard(*lut);

    // Make a domain for the composed Lut3D.
    // TODO: Using a large number like 48 here is better for accuracy, 
    // but it causes a delay when creating the renderer. 
    const long GridSize = 48u;
    Lut3DOpDataRcPtr newDomain = std::make_shared<Lut3DOpData>(GridSize);

    // Regardless of what depth is used to build the domain, set the in & out to the
    // actual depth so that scaling is done correctly.
    newDomain->setInputBitDepth(lut->getInputBitDepth());
    newDomain->setOutputBitDepth(lut->getInputBitDepth());

    // Compose the LUT newDomain with our inverse LUT (using INV_EXACT style).
    Lut3DOpData::Compose(newDomain, lut);

    // The INV_EXACT inversion style computes an inverse to the tetrahedral
    // style of forward evalutation.
    // TODO: Although this seems like the "correct" thing to do, it does
    // not seem to help accuracy (and is slower).  To investigate ...
    //newLut->setInterpolation(INTERP_TETRAHEDRAL);

    return newDomain;
}

const unsigned long Lut3DOpData::maxSupportedLength = 129;

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
void Lut3DOpData::Compose(Lut3DOpDataRcPtr & A,
                          ConstLut3DOpDataRcPtr & B)
{
    // TODO: Composition of LUTs is a potentially lossy operation.
    // We try to be safe by making the result at least as big as either A or B
    // but we may want to even increase the resolution further.  However,
    // currently composition is done pairs at a time and we would want to
    // determine the increase size once at the start rather than bumping it up
    // as each pair is done.

    if (A->getOutputBitDepth() != B->getInputBitDepth())
    {
        throw Exception("A bit depth mismatch forbids the composition of LUTs");
    }

    const long min_sz = B->getArray().getLength();
    const long n = A->getArray().getLength();
    OpRcPtrVec ops;

    ConstLut3DOpDataRcPtr domain;

    if (n >= min_sz)
    {
        // The range of the first LUT becomes the domain to interp in the second.

        const float iScale = 1.f / GetBitDepthMaxValue(A->getOutputBitDepth());
        const float iScale4[4] = { iScale, iScale, iScale, 1.0f };
        CreateScaleOp(ops, iScale4, TRANSFORM_DIR_FORWARD);

        // Use the original domain.
        domain = A;
    }
    else
    {
        // Since the 2nd LUT is more finely sampled, use its grid size.

        // Create identity with finer domain.

        // TODO: Should not need to create a new LUT object for this.
        //       Perhaps add a utility function to be shared with the constructor.
        domain = std::make_shared<Lut3DOpData>(A->getInputBitDepth(),
                                               BIT_DEPTH_F32,
                                               A->getID(),
                                               A->getDescriptions(),
                                               A->getInterpolation(),
                                               min_sz);

        // Interpolate through both LUTs in this case (resample).
        CreateLut3DOp(ops, A, TRANSFORM_DIR_FORWARD);
    }

    // TODO: Would like to not require a clone simply to prevent the delete
    //       from being called on the op when the opList goes out of scope.
    Lut3DOpDataRcPtr clonedB = B->clone();
    CreateLut3DOp(ops, clonedB, TRANSFORM_DIR_FORWARD);

    const float iScale = GetBitDepthMaxValue(B->getOutputBitDepth());

    const float iScale4[4] = { iScale, iScale, iScale, 1.0f };
    CreateScaleOp(ops, iScale4, TRANSFORM_DIR_FORWARD);

    OpData::Descriptions newDesc = A->getDescriptions();
    newDesc += B->getDescriptions();
    // TODO: May want to revisit metadata propagation.
    A = std::make_shared<Lut3DOpData>(A->getInputBitDepth(),
                                      B->getOutputBitDepth(),
                                      A->getID() + B->getID(),
                                      newDesc,
                                      A->getInterpolation(),
                                      2);  // we replace it anyway

    const Array::Values& inValues = domain->getArray().getValues();
    const long gridSize = domain->getArray().getLength();
    const long numPixels = gridSize * gridSize * gridSize;

    A->getArray().resize(gridSize, 3);
    Array::Values& outValues = A->getArray().getValues();

    EvalTransform((const float*)(&inValues[0]),
                  (float*)(&outValues[0]),
                  numPixels,
                  ops);

    // TODO: Code to handle dynamic properties should go here.
}

Lut3DOpData::Lut3DArray::Lut3DArray(unsigned long length,
                                    BitDepth outBitDepth)
{
    resize(length, getMaxColorComponents());
    fill(outBitDepth);
}

Lut3DOpData::Lut3DArray::~Lut3DArray()
{
}

Lut3DOpData::Lut3DArray& Lut3DOpData::Lut3DArray::operator=(const Array& a)
{
    if (this != &a)
    {
        *(Array*)this = a;
    }
    return *this;
}

void Lut3DOpData::Lut3DArray::fill(BitDepth outBitDepth)
{
    // Make an identity LUT.
    const long length = getLength();
    const long maxChannels = getMaxColorComponents();

    Array::Values& values = getValues();

    const float stepValue 
        = GetBitDepthMaxValue(outBitDepth) / ((float)length - 1.0f);

    const long maxEntries = length*length*length;

    for (long idx = 0; idx<maxEntries; idx++)
    {
        values[maxChannels*idx + 0] = (float)((idx / length / length) % length) * stepValue;
        values[maxChannels*idx + 1] = (float)((idx / length) % length) * stepValue;
        values[maxChannels*idx + 2] = (float)(idx%length) * stepValue;
    }
}

bool Lut3DOpData::Lut3DArray::isIdentity(BitDepth outBitDepth) const
{
    const long length = getLength();
    const Array::Values & values = getValues();

    // As an identity LUT shall not change color component values,
    // aside from possibly a scaling for bit-depth conversion.

    const float stepSize =
        GetBitDepthMaxValue(outBitDepth) / ((float)length - 1.0f);

    const long maxEntries = length*length*length;

    for(long i=0; i<maxEntries; i++)
    {
        // TODO: Use EqualWithSafeRelError to account for outBitDepth.
        if(!EqualWithAbsError(values[3*i+0],
                              (float)((i/ length / length)% length) * stepSize,
                              0.0001f))
        {
            return false;
        }

        if(!EqualWithAbsError(values[3*i+1],
                              (float)((i/ length)% length) * stepSize,
                              0.0001f))
        {
            return false;
        }

        if(!EqualWithAbsError(values[3*i+2],
                              (float)(i%length) * stepSize,
                              0.0001f))
        {
            return false;
        }
    }
    return true;
}

unsigned long Lut3DOpData::Lut3DArray::getNumValues() const
{
    const unsigned long numEntries = getLength()*getLength()*getLength();
    return numEntries*getMaxColorComponents();
}

void Lut3DOpData::Lut3DArray::getRGB(long i, long j, long k, float* RGB) const
{
    const long length = getLength();
    const long maxChannels = getMaxColorComponents();
    const Array::Values& values = getValues();
    // Array order matches ctf order: channels vary most rapidly, then B, G, R.
    long offset = (i*length*length + j*length + k) * maxChannels;
    RGB[0] = values[offset];
    RGB[1] = values[++offset];
    RGB[2] = values[++offset];
}

void Lut3DOpData::Lut3DArray::setRGB(long i, long j, long k, float* RGB)
{
    const long length = getLength();
    const long maxChannels = getMaxColorComponents();
    Array::Values& values = getValues();
    // Array order matches ctf order: channels vary most rapidly, then B, G, R.
    long offset = (i*length*length + j*length + k) * maxChannels;
    values[offset] = RGB[0];
    values[++offset] = RGB[1];
    values[++offset] = RGB[2];
}

void Lut3DOpData::Lut3DArray::scale(float scaleFactor)
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

Lut3DOpData::Lut3DOpData(unsigned long gridSize)
    : OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    , m_interpolation(INTERP_DEFAULT)
    , m_array(gridSize, getOutputBitDepth())
    , m_direction(TRANSFORM_DIR_FORWARD)
    , m_invQuality(LUT_INVERSION_FAST)
{
}

Lut3DOpData::Lut3DOpData(long gridSize, TransformDirection dir)
    : OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
    , m_interpolation(INTERP_DEFAULT)
    , m_array(gridSize, getOutputBitDepth())
    , m_direction(dir)
    , m_invQuality(LUT_INVERSION_FAST)
{
}

Lut3DOpData::Lut3DOpData(BitDepth inBitDepth,
                         BitDepth outBitDepth,
                         const std::string& id,
                         const Descriptions& descriptions,
                         Interpolation interpolation,
                         unsigned long gridSize)
    : OpData(inBitDepth, outBitDepth, id, descriptions)
    , m_interpolation(interpolation)
    , m_array(gridSize, getOutputBitDepth())
    , m_direction(TRANSFORM_DIR_FORWARD)
    , m_invQuality(LUT_INVERSION_FAST)
{
}

Lut3DOpData::~Lut3DOpData()
{
}

void Lut3DOpData::setInterpolation(Interpolation algo)
{
    m_interpolation = algo;
}

Interpolation Lut3DOpData::getConcreteInterpolation() const
{
    switch (m_interpolation)
    {
    case INTERP_BEST:
    case INTERP_TETRAHEDRAL:
        return INTERP_TETRAHEDRAL;

    case INTERP_DEFAULT:
    case INTERP_LINEAR:
    case INTERP_CUBIC:
    case INTERP_NEAREST:
        // NB: In OCIO v2, INTERP_NEAREST is implemented as trilinear,
        // this is a change from OCIO v1.
    case INTERP_UNKNOWN:
        // NB: INTERP_UNKNOWN is not valid and will make validate() throw.
    default:
        return INTERP_LINEAR;
    }
}

LutInversionQuality Lut3DOpData::getConcreteInversionQuality() const
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

void Lut3DOpData::setInversionQuality(LutInversionQuality style)
{
    m_invQuality = style;
}

namespace
{
bool IsValid(const Interpolation & interpolation)
{
    switch (interpolation)
    {
    case INTERP_BEST:
    case INTERP_TETRAHEDRAL:
    case INTERP_DEFAULT:
    case INTERP_LINEAR:
    case INTERP_NEAREST:
        return true;
    case INTERP_CUBIC:
    case INTERP_UNKNOWN:
    default:
        return false;
    }
}
}

void Lut3DOpData::validate() const
{
    OpData::validate();

    if (!IsValid(m_interpolation))
    {
        throw Exception("Lut3D has an invalid interpolation type. ");
    }

    try
    {
        getArray().validate();
    }
    catch (Exception& e)
    {
        std::ostringstream oss;
        oss << "Lut3D content array issue: ";
        oss << e.what();

        throw Exception(oss.str().c_str());
    }

    if (getArray().getNumColorComponents() != 3)
    {
        throw Exception("Lut3D has an incorrect number of color components. ");
    }

    if (getArray().getLength()>maxSupportedLength)
    {
        std::ostringstream oss;
        oss << "Lut3D length: ";
        oss << getArray().getLength();
        oss << " is not supported. ";

        throw Exception(oss.str().c_str());
    }
}

bool Lut3DOpData::isNoOp() const
{
    // 3D LUT is clamping to its domain
    return false;
}

bool Lut3DOpData::isIdentity() const
{
    return m_array.isIdentity(getOutputBitDepth());
}

void Lut3DOpData::setOutputBitDepth(BitDepth out)
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

void Lut3DOpData::setInputBitDepth(BitDepth in)
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

OpDataRcPtr Lut3DOpData::getIdentityReplacement() const
{
    const BitDepth inBD = getInputBitDepth();
    const BitDepth outBD = getOutputBitDepth();

    return 
        OpDataRcPtr(new RangeOpData(inBD,
                                    outBD,
                                    0.,
                                    GetBitDepthMaxValue(inBD),
                                    0.,
                                    GetBitDepthMaxValue(outBD)));
}

bool Lut3DOpData::haveEqualBasics(const Lut3DOpData & B) const
{
    // TODO: Should interpolation style be considered?
    // NB: The bit-depths must be harmonized by the caller before comparing
    //     the array contents.
    return (m_array == B.m_array);
}

bool Lut3DOpData::operator==(const OpData & other) const
{
    if (this == &other) return true;

    if (!OpData::operator==(other)) return false;

    const Lut3DOpData* lop = static_cast<const Lut3DOpData*>(&other);

    // NB: The m_invQuality is not currently included.
    if (m_direction != lop->m_direction
        || m_interpolation != lop->m_interpolation)
    {
        return false;
    }

    return haveEqualBasics(*lop);
}

Lut3DOpDataRcPtr Lut3DOpData::clone() const
{
    return std::make_shared<Lut3DOpData>(*this);
}

bool Lut3DOpData::isInverse(const Lut3DOpData * lutfwd, const Lut3DOpData * lutinv)
{
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
        Lut3DOpDataRcPtr scaledLut = lutfwd->clone();
        scaledLut->setOutputBitDepth(lutinv->getInputBitDepth());
        return scaledLut->haveEqualBasics(*lutinv);
    }
    else
    {
        // Test the array itself.
        return lutfwd->haveEqualBasics(*lutinv);
    }

}

bool Lut3DOpData::isInverse(ConstLut3DOpDataRcPtr & B) const
{
    if (m_direction == TRANSFORM_DIR_FORWARD
        && B->m_direction == TRANSFORM_DIR_INVERSE)
    {
        return isInverse(this, B.get());
    }
    else if (m_direction == TRANSFORM_DIR_INVERSE
             && B->m_direction == TRANSFORM_DIR_FORWARD)
    {
        return isInverse(B.get(), this);
    }
    return false;
}

Lut3DOpDataRcPtr Lut3DOpData::inverse() const
{
    Lut3DOpDataRcPtr invLut = clone();

    invLut->m_direction = (m_direction == TRANSFORM_DIR_FORWARD) ?
                          TRANSFORM_DIR_INVERSE : TRANSFORM_DIR_FORWARD;

    // Swap input/output bitdepths.
    // Note: Call for OpData::set*BitDepth() in order to only swap the bitdepths
    //       without any rescaling.
    const BitDepth in = getInputBitDepth();
    invLut->OpData::setInputBitDepth(getOutputBitDepth());
    invLut->OpData::setOutputBitDepth(in);

    return invLut;
}

void Lut3DOpData::finalize()
{
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
    cacheIDStream << InterpolationToString(m_interpolation) << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";
    cacheIDStream << BitDepthToString(getInputBitDepth()) << " ";
    cacheIDStream << BitDepthToString(getOutputBitDepth());
    // NB: The m_invQuality is not currently included.

    m_cacheID = cacheIDStream.str();
}

}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"
#include "UnitTestUtils.h"

OIIO_ADD_TEST(Lut3DOpData, empty)
{
    OCIO::Lut3DOpData l(2);
    OIIO_CHECK_NO_THROW(l.validate());
    OIIO_CHECK_ASSERT(l.isIdentity());
    OIIO_CHECK_ASSERT(!l.isNoOp());
    OIIO_CHECK_EQUAL(l.getType(), OCIO::OpData::Lut3DType);
    OIIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_FAST);
    OIIO_CHECK_EQUAL(l.getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
}

OIIO_ADD_TEST(Lut3DOpData, accessors)
{
    OCIO::Interpolation interpol = OCIO::INTERP_LINEAR;

    OCIO::Lut3DOpData l(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                        "uid", OCIO::OpData::Descriptions(),
                        interpol, 33);

    OIIO_CHECK_EQUAL(l.getInterpolation(), interpol);
    OIIO_CHECK_ASSERT(l.isIdentity());

    OIIO_CHECK_NO_THROW(l.validate());

    l.getArray()[0] = 1.0f;

    OIIO_CHECK_ASSERT(!l.isIdentity());
    OIIO_CHECK_NO_THROW(l.validate());

    interpol = OCIO::INTERP_TETRAHEDRAL;
    l.setInterpolation(interpol);
    OIIO_CHECK_EQUAL(l.getInterpolation(), interpol);

    OIIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_FAST);
    l.setInversionQuality(OCIO::LUT_INVERSION_BEST);
    OIIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_BEST);
    OIIO_CHECK_EQUAL(l.getConcreteInversionQuality(), OCIO::LUT_INVERSION_EXACT);
    l.setInversionQuality(OCIO::LUT_INVERSION_FAST);

    OIIO_CHECK_EQUAL(l.getArray().getLength(), (long)33);
    OIIO_CHECK_EQUAL(l.getArray().getNumValues(), (long)33 * 33 * 33 * 3);
    OIIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), (long)3);

    l.getArray().resize(17, 3);

    OIIO_CHECK_EQUAL(l.getArray().getLength(), (long)17);
    OIIO_CHECK_EQUAL(l.getArray().getNumValues(), (long)17 * 17 * 17 * 3);
    OIIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), (long)3);
    OIIO_CHECK_NO_THROW(l.validate());
}

OIIO_ADD_TEST(Lut3DOpData, diff_bitdepth)
{
    OCIO::Interpolation interpol = OCIO::INTERP_LINEAR;
    OCIO::Lut3DOpData l1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8,
                         "uid", OCIO::OpData::Descriptions(),
                         interpol, 33);
    OCIO::Lut3DOpData l2(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10,
                         "uid", OCIO::OpData::Descriptions(),
                         interpol, 33);

    OIIO_CHECK_ASSERT(l1.isIdentity());
    OIIO_CHECK_NO_THROW(l1.validate());
    OIIO_CHECK_ASSERT(l2.isIdentity());
    OIIO_CHECK_NO_THROW(l2.validate());

    const float coeff
        = OCIO::GetBitDepthMaxValue(l2.getOutputBitDepth())
            / OCIO::GetBitDepthMaxValue(l2.getInputBitDepth());

    // To validate the result, compute the expected results
    // not using the Lut3DOp algorithm.
    const float error = 1e-4f;
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
}

OIIO_ADD_TEST(Lut3DOpData, clone)
{
    OCIO::Lut3DOpData ref(33);
    ref.getArray()[1] = 0.1f;

    OCIO::Lut3DOpDataRcPtr pClone = ref.clone();

    OIIO_CHECK_ASSERT(pClone.get());
    OIIO_CHECK_ASSERT(!pClone->isNoOp());
    OIIO_CHECK_ASSERT(!pClone->isIdentity());
    OIIO_CHECK_NO_THROW(pClone->validate());
    OIIO_CHECK_ASSERT(pClone->getArray()==ref.getArray());
}

OIIO_ADD_TEST(Lut3DOpData, not_supported_length)
{
    const OCIO::Lut3DOpData ref1(OCIO::Lut3DOpData::maxSupportedLength);

    OIIO_CHECK_NO_THROW(ref1.validate());

    const OCIO::Lut3DOpData ref2(OCIO::Lut3DOpData::maxSupportedLength + 1);

    OIIO_CHECK_THROW_WHAT(ref2.validate(), OCIO::Exception, "is not supported");

}

OIIO_ADD_TEST(Lut3DOpData, ouput_depth_scaling)
{
    OCIO::Interpolation interpol = OCIO::INTERP_LINEAR;
    OCIO::Lut3DOpData ref(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10,
                          "uid", OCIO::OpData::Descriptions(),
                          interpol, 33);

    // Get the array values and keep them to compare later.
    const OCIO::Array::Values 
        initialArrValues = ref.getArray().getValues();
    const OCIO::BitDepth initialBitdepth = ref.getOutputBitDepth();

    const OCIO::BitDepth newBitdepth = OCIO::BIT_DEPTH_UINT16;

    const float factor
        = OCIO::GetBitDepthMaxValue(newBitdepth)
            / OCIO::GetBitDepthMaxValue(initialBitdepth);

    ref.setOutputBitDepth(newBitdepth);
    // Now we need to make sure that the bitdepth was changed from the overriden
    // method.
    OIIO_CHECK_EQUAL(ref.getOutputBitDepth(), newBitdepth);

    // Now we need to check if the scaling between the new array and initial array
    // matches the factor computed above.
    const OCIO::Array::Values& newArrValues = ref.getArray().getValues();

    // Sanity check first.
    OIIO_CHECK_EQUAL(initialArrValues.size(), newArrValues.size());

    float expectedValue = 0.0f;

    for(unsigned long i = 0; i < (unsigned long)newArrValues.size(); i++)
    {
        expectedValue = initialArrValues[i] * factor;
        OIIO_CHECK_ASSERT(OCIO::EqualWithAbsError(expectedValue,
                                                  newArrValues[i],
                                                  0.0001f));
    }
}

OIIO_ADD_TEST(Lut3DOpData, equality)
{
    OCIO::Lut3DOpData l1(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                         "", OCIO::OpData::Descriptions(), 
                         OCIO::INTERP_LINEAR, 33);

    OCIO::Lut3DOpData l2(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                         "", OCIO::OpData::Descriptions(), 
                         OCIO::INTERP_BEST, 33);  

    OIIO_CHECK_ASSERT(!(l1 == l2));

    OCIO::Lut3DOpData l3(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32,
                         "", OCIO::OpData::Descriptions(), 
                         OCIO::INTERP_LINEAR, 33);

    OIIO_CHECK_ASSERT(!(l1 == l3) && !(l2 == l3));

    OCIO::Lut3DOpData l4(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32,
                         "", OCIO::OpData::Descriptions(),
                         OCIO::INTERP_LINEAR, 33);  

    OIIO_CHECK_ASSERT(l1 == l4);

    // Inversion quality does not affect forward ops equality.
    l1.setInversionQuality(OCIO::LUT_INVERSION_BEST);

    OIIO_CHECK_ASSERT(l1 == l4);

    // Inversion quality does not affect inverse ops equality.
    // Even so applying the ops could lead to small differences.
    auto l5 = l1.inverse();
    auto l6 = l4.inverse();

    OIIO_CHECK_ASSERT(*l5 == *l6);
}

OIIO_ADD_TEST(Lut3DOpData, interpolation)
{
    OCIO::Lut3DOpData l(2);
    l.setInputBitDepth(OCIO::BIT_DEPTH_F32);
    l.setOutputBitDepth(OCIO::BIT_DEPTH_F32);
    
    l.setInterpolation(OCIO::INTERP_LINEAR);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_CUBIC);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_CUBIC);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "invalid interpolation");

    l.setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_DEFAULT);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_DEFAULT);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_BEST);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_BEST);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_NO_THROW(l.validate());

    // NB: INTERP_NEAREST is currently implemented as INTERP_LINEAR.
    l.setInterpolation(OCIO::INTERP_NEAREST);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_NEAREST);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    // Invalid interpolation type are implemented as INTERP_LINEAR
    // but can not be used because validation throws.
    l.setInterpolation(OCIO::INTERP_UNKNOWN);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_UNKNOWN);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "invalid interpolation");
}

OIIO_ADD_TEST(Lut3DOpData, inversion_quality)
{
    OCIO::Lut3DOpData l(2);
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

namespace
{
const char uid[] = "uid";
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
    OCIO::Lut3DOpData refLut3d(
        refInputBitDepth, refOutputBitDepth,
        uid, desc,
        refInterpolationAlgo, 5);

    // Get inverse of reference 3D LUT operation 
    OCIO::Lut3DOpDataRcPtr invLut3d = refLut3d.inverse();

    OIIO_CHECK_ASSERT(invLut3d);

    OIIO_CHECK_EQUAL(invLut3d->getInputBitDepth(),
                     expectedInvInputBitDepth);

    OIIO_CHECK_EQUAL(invLut3d->getOutputBitDepth(),
                     expectedInvOutputBitDepth);

    OIIO_CHECK_EQUAL(invLut3d->getInterpolation(),
                     expectedInvInterpolationAlgo);
}

OIIO_ADD_TEST(Lut3DOpData, inverse_bitDepth_domain)
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

OIIO_ADD_TEST(Lut3DOpData, is_inverse)
{
    // Create forward LUT.
    OCIO::Lut3DOpDataRcPtr L1NC =
        std::make_shared<OCIO::Lut3DOpData>(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10,
                                            uid, desc, OCIO::INTERP_LINEAR, 5);
    // Make it not an identity.
    OCIO::Array& array = L1NC->getArray();
    OCIO::Array::Values& values = array.getValues();
    values[0] = 20.f;
    OIIO_CHECK_ASSERT(!L1NC->isIdentity());

    // Create an inverse LUT with same basics.
    OCIO::ConstLut3DOpDataRcPtr L1 = L1NC;
    OCIO::ConstLut3DOpDataRcPtr L2 = L1->inverse();
    OCIO::ConstLut3DOpDataRcPtr L3 = L2->inverse();
    OIIO_CHECK_ASSERT(*L3 == *L1);
    OIIO_CHECK_ASSERT(!(*L1 == *L2));

    // Check isInverse.
    OIIO_CHECK_ASSERT(L1->isInverse(L2));
    OIIO_CHECK_ASSERT(L2->isInverse(L1));

    // This should pass, since the arrays are actually the same if you
    // normalize for the scaling difference.
    L1NC->setOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_ASSERT(L1->isInverse(L2));
    OIIO_CHECK_ASSERT(L2->isInverse(L1));

    // Catch the situation where the arrays are the same even though the
    // bit-depths don't match and hence the arrays effectively aren't the same.
    L1NC->setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);  // restore original
    OIIO_CHECK_ASSERT(L1->isInverse(L2));
    L1NC->OpData::setOutputBitDepth(OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_ASSERT(!L1->isInverse(L2));
    OIIO_CHECK_ASSERT(!L2->isInverse(L1));
}

OIIO_ADD_TEST(Lut3DOpData, compose)
{
    const std::string spi3dFile("spi_ocio_srgb_test.spi3d");
    OCIO::OpRcPtrVec ops;

    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops, spi3dFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    // First op is a FileNoOp.
    OIIO_REQUIRE_EQUAL(2, ops.size());

    const std::string spi3dFile1("comp2.spi3d");
    OCIO::OpRcPtrVec ops1;
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops1, spi3dFile1, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OIIO_REQUIRE_EQUAL(2, ops1.size());

    OCIO_SHARED_PTR<const OCIO::Op> op0 = ops[1];
    OCIO_SHARED_PTR<const OCIO::Op> op1 = ops1[1];

    OCIO::ConstOpDataRcPtr opData0 = op0->data();
    OCIO::ConstOpDataRcPtr opData1 = op1->data();

    OCIO::ConstLut3DOpDataRcPtr lutData0 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData0);
    OCIO::ConstLut3DOpDataRcPtr lutData1 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData1);

    OIIO_REQUIRE_ASSERT(lutData0);
    OIIO_REQUIRE_ASSERT(lutData1);

    OCIO::Lut3DOpDataRcPtr composed = lutData0->clone();
    OCIO::Lut3DOpData::Compose(composed, lutData1);

    OIIO_CHECK_EQUAL(composed->getArray().getLength(), (unsigned long)32);
    OIIO_CHECK_EQUAL(composed->getArray().getNumColorComponents(),
        (unsigned long)3);
    OIIO_CHECK_EQUAL(composed->getArray().getNumValues(),
        (unsigned long)32 * 32 * 32 * 3);

    OIIO_CHECK_CLOSE(composed->getArray().getValues()[0], 0.0288210791f, 1e-7f);
    OIIO_CHECK_CLOSE(composed->getArray().getValues()[1], 0.0280428901f, 1e-7f);
    OIIO_CHECK_CLOSE(composed->getArray().getValues()[2], 0.0262413863f, 1e-7f);

    OIIO_CHECK_CLOSE(composed->getArray().getValues()[666], 0.0f, 1e-7f);
    OIIO_CHECK_CLOSE(composed->getArray().getValues()[667], 0.274289876f, 1e-7f);
    OIIO_CHECK_CLOSE(composed->getArray().getValues()[668], 0.854598403f, 1e-7f);

    OIIO_CHECK_CLOSE(composed->getArray().getValues()[1800], 0.0f, 1e-7f);
    OIIO_CHECK_CLOSE(composed->getArray().getValues()[1801], 0.411249638f, 1e-7f);
    OIIO_CHECK_CLOSE(composed->getArray().getValues()[1802], 0.881694913f, 1e-7f);

    OIIO_CHECK_CLOSE(composed->getArray().getValues()[96903], 1.0f, 1e-7f);
    OIIO_CHECK_CLOSE(composed->getArray().getValues()[96904], 0.588273168f, 1e-7f);
    OIIO_CHECK_CLOSE(composed->getArray().getValues()[96905], 0.0f, 1e-7f);

}

OIIO_ADD_TEST(Lut3DOpData, compose_2)
{
    const std::string clfFile("lut3d_bizarre.clf");
    OCIO::OpRcPtrVec ops;

    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops, clfFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OIIO_REQUIRE_EQUAL(2, ops.size());

    const std::string clfFile1("lut3d_17x17x17_10i_12i.clf");
    OCIO::OpRcPtrVec ops1;
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops1, clfFile1, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OIIO_REQUIRE_EQUAL(2, ops1.size());

    OCIO_SHARED_PTR<const OCIO::Op> op0 = ops[1];
    OCIO_SHARED_PTR<const OCIO::Op> op1 = ops1[1];

    OCIO::ConstOpDataRcPtr opData0 = op0->data();
    OCIO::ConstOpDataRcPtr opData1 = op1->data();

    OCIO::ConstLut3DOpDataRcPtr lutData0 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData0);
    OCIO::ConstLut3DOpDataRcPtr lutData1 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData1);

    OIIO_REQUIRE_ASSERT(lutData0);
    OIIO_REQUIRE_ASSERT(lutData1);

    OCIO::Lut3DOpDataRcPtr composed = lutData0->clone();
    OIIO_CHECK_NO_THROW(OCIO::Lut3DOpData::Compose(composed, lutData1));

    OIIO_CHECK_EQUAL(composed->getArray().getLength(), (unsigned long)17);
    const std::vector<float> & a = composed->getArray().getValues();
    OIIO_CHECK_CLOSE(a[6]     / 4095.0f,    2.5942142f  / 4095.0f, 1e-7f);
    OIIO_CHECK_CLOSE(a[7]     / 4095.0f,   29.60961342f / 4095.0f, 1e-7f);
    OIIO_CHECK_CLOSE(a[8]     / 4095.0f,  154.82646179f / 4095.0f, 1e-7f);
    OIIO_CHECK_CLOSE(a[8289]  / 4095.0f, 1184.69213867f / 4095.0f, 1e-6f);
    OIIO_CHECK_CLOSE(a[8290]  / 4095.0f, 1854.97229004f / 4095.0f, 1e-7f);
    OIIO_CHECK_CLOSE(a[8291]  / 4095.0f, 1996.75830078f / 4095.0f, 1e-7f);
    OIIO_CHECK_CLOSE(a[14736] / 4095.0f, 4094.07617188f / 4095.0f, 1e-7f);
    OIIO_CHECK_CLOSE(a[14737] / 4095.0f, 4067.37231445f / 4095.0f, 1e-6f);
    OIIO_CHECK_CLOSE(a[14738] / 4095.0f, 4088.30493164f / 4095.0f, 1e-6f);

    // Check that if the connecting bit-depths don't match, it's an exception.
    OCIO::Lut3DOpDataRcPtr composed1 = lutData1->clone();
    OIIO_CHECK_THROW_WHAT(OCIO::Lut3DOpData::Compose(composed1, lutData0),
                          OCIO::Exception, "bit depth mismatch");

}

OIIO_ADD_TEST(Lut3DOpData, inv_lut3d_lut_size)
{
    const std::string fileName("lut3d_17x17x17_10i_12i.clf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OIIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OIIO_REQUIRE_EQUAL(2, ops.size());

    auto op1 = std::dynamic_pointer_cast<const OCIO::Op>(ops[1]);
    OIIO_REQUIRE_ASSERT(op1);
    auto fwdLutData = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(op1->data());
    OIIO_REQUIRE_ASSERT(fwdLutData);
    OCIO::ConstLut3DOpDataRcPtr invLutData = fwdLutData->inverse();

    OCIO::Lut3DOpDataRcPtr invFastLutData = MakeFastLut3DFromInverse(invLutData);

    // NB: Avoid finalizing the transform since this will reset the bit-depths
    // and prevent checking that the inversion swapped them correctly.
    OIIO_CHECK_EQUAL(invFastLutData->getInputBitDepth(), OCIO::BIT_DEPTH_UINT12);
    OIIO_CHECK_EQUAL(invFastLutData->getOutputBitDepth(), OCIO::BIT_DEPTH_UINT10);

    OIIO_CHECK_EQUAL(invFastLutData->getArray().getLength(), 48);
}

#endif

