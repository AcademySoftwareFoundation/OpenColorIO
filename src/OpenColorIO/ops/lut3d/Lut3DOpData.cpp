// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "md5/md5.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ops/lut3d/Lut3DOpData.h"
#include "ops/OpTools.h"
#include "ops/range/RangeOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{
// Note: The Lut3DOpData Array stores the values in blue-fastest order.

// There are two inversion algorithms provided for 3D LUT, an exact
// method (that assumes use of tetrahedral in the forward direction)
// and a fast method that bakes the inverse out as another forward
// 3D LUT. The exact method is currently unavailable on the GPU.
// Both methods assume that the input and output to the 3D LUT are
// roughly perceptually uniform. Values outside the range of the
// forward 3D LUT are clamped to someplace on the exterior surface
// of the 3D LUT.

Lut3DOpDataRcPtr MakeFastLut3DFromInverse(ConstLut3DOpDataRcPtr & lut)
{
    if (lut->getDirection() != TRANSFORM_DIR_INVERSE)
    {
        throw Exception("MakeFastLut3DFromInverse expects an inverse LUT");
    }

    // TODO: The FastLut will limit inputs to [0,1].  If the forward LUT has an extended range
    // output, perhaps add a Range op before the FastLut to bring values into [0,1].

    // Make a domain for the composed Lut3D.
    // TODO: Using a large number like 48 here is better for accuracy,
    // but it causes a delay when creating the renderer.
    const long GridSize = 48u;
    Lut3DOpDataRcPtr newDomain = std::make_shared<Lut3DOpData>(GridSize);

    newDomain->setFileOutputBitDepth(lut->getFileOutputBitDepth());

    ConstLut3DOpDataRcPtr constNewDomain = newDomain;

    // Compose the LUT newDomain with our inverse LUT (using INV_EXACT style).
    Lut3DOpDataRcPtr result = Lut3DOpData::Compose(constNewDomain, lut);

    // The INV_EXACT inversion style computes an inverse to the tetrahedral
    // style of forward evaluation.
    // TODO: Although this seems like the "correct" thing to do, it does
    // not seem to help accuracy (and is slower).  To investigate ...
    //result->setInterpolation(INTERP_TETRAHEDRAL);
    return result;
}

// 129 allows for a MESH dimension of 7 in the 3dl file format.
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
Lut3DOpDataRcPtr Lut3DOpData::Compose(ConstLut3DOpDataRcPtr & lutc1,
                                      ConstLut3DOpDataRcPtr & lutc2)
{
    // TODO: Composition of LUTs is a potentially lossy operation.
    // We try to be safe by making the result at least as big as either lut1 or
    // lut2 but we may want to even increase the resolution further.  However,
    // currently composition is done pairs at a time and we would want to
    // determine the increase size once at the start rather than bumping it up
    // as each pair is done.

    Lut3DOpDataRcPtr lut1 = std::const_pointer_cast<Lut3DOpData>(lutc1);
    Lut3DOpDataRcPtr lut2 = std::const_pointer_cast<Lut3DOpData>(lutc2);
    bool restoreInverse = false;
    if (lut1->getDirection() == TRANSFORM_DIR_INVERSE && lut2->getDirection() == TRANSFORM_DIR_INVERSE)
    {
        // Using the fact that: iInv(l2 x l1) = inv(l1) x inv(l2).
        // Compute l2 x l1 and inverse the result.
        lut1.swap(lut2);

        lut1->setDirection(TRANSFORM_DIR_FORWARD);
        lut2->setDirection(TRANSFORM_DIR_FORWARD);
        restoreInverse = true;
    }

    const long min_sz = lut2->getArray().getLength();
    const long n = lut1->getArray().getLength();
    const long domain_size = std::max(min_sz, n);
    OpRcPtrVec ops;

    Lut3DOpDataRcPtr result;

    if (n >= min_sz && !(lut1->getDirection() == TRANSFORM_DIR_INVERSE))
    {
        // The range of the first LUT becomes the domain to interp in the second.
        // Use the original domain.
        result = lut1->clone();
    }
    else
    {
        // Since the 2nd LUT is more finely sampled, use its grid size.

        // Create identity with finer domain.

        result = std::make_shared<Lut3DOpData>(lut1->getInterpolation(), domain_size);

        auto metadata = lut1->getFormatMetadata();
        result->getFormatMetadata() = metadata;

        // Interpolate through both LUTs in this case (resample).
        Lut3DOpDataRcPtr nonConstLut1 = std::const_pointer_cast<Lut3DOpData>(lut1);
        CreateLut3DOp(ops, nonConstLut1, TRANSFORM_DIR_FORWARD);
    }

    // We need a non-const version of lut2 to create the op.
    // Op will not be modified (except by finalize, but that should have been done already).
    Lut3DOpDataRcPtr nonConstLut2 = std::const_pointer_cast<Lut3DOpData>(lut2);
    CreateLut3DOp(ops, nonConstLut2, TRANSFORM_DIR_FORWARD);

    const BitDepth fileOutBD = lut1->getFileOutputBitDepth();

    // TODO: May want to revisit metadata propagation.
    result->getFormatMetadata().combine(lut2->getFormatMetadata());

    result->setFileOutputBitDepth(fileOutBD);

    const Array::Values & domain = result->getArray().getValues();
    const long gridSize = result->getArray().getLength();
    const long numPixels = gridSize * gridSize * gridSize;

    EvalTransform((const float*)(&domain[0]),
                  (float*)(&domain[0]),
                  numPixels,
                  ops);

    if (restoreInverse)
    {
        lut1->setDirection(TRANSFORM_DIR_INVERSE);
        lut2->setDirection(TRANSFORM_DIR_INVERSE);
        result->setDirection(TRANSFORM_DIR_INVERSE);
    }

    return result;
}

Lut3DOpData::Lut3DArray::Lut3DArray(unsigned long length)
{
    resize(length, getMaxColorComponents());
    fill();
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

void Lut3DOpData::Lut3DArray::fill()
{
    // Make an identity LUT.
    const long length = getLength();
    const long maxChannels = getMaxColorComponents();

    Array::Values& values = getValues();

    const float stepValue = 1.0f / ((float)length - 1.0f);

    const long maxEntries = length*length*length;

    for (long idx = 0; idx<maxEntries; idx++)
    {
        values[maxChannels*idx + 0] = (float)((idx / length / length) % length) * stepValue;
        values[maxChannels*idx + 1] = (float)((idx / length) % length) * stepValue;
        values[maxChannels*idx + 2] = (float)(idx%length) * stepValue;
    }
}

void Lut3DOpData::Lut3DArray::resize(unsigned long length, unsigned long numColorComponents)
{
    if (length > maxSupportedLength)
    {
        std::ostringstream oss;
        oss << "LUT 3D: Grid size '" << length
            << "' must not be greater than '" << maxSupportedLength << "'.";
        throw Exception(oss.str().c_str());
    }
    Array::resize(length, numColorComponents);
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
    : OpData()
    , m_interpolation(INTERP_DEFAULT)
    , m_array(gridSize)
    , m_direction(TRANSFORM_DIR_FORWARD)
{
}

Lut3DOpData::Lut3DOpData(long gridSize, TransformDirection dir)
    : OpData()
    , m_interpolation(INTERP_DEFAULT)
    , m_array(gridSize)
    , m_direction(dir)
{
}

Lut3DOpData::Lut3DOpData(Interpolation interpolation, unsigned long gridSize)
    : OpData()
    , m_interpolation(interpolation)
    , m_array(gridSize)
    , m_direction(TRANSFORM_DIR_FORWARD)
{
}

Lut3DOpData::~Lut3DOpData()
{
}

void Lut3DOpData::setInterpolation(Interpolation interpolation)
{
    m_interpolation = interpolation;
}

Interpolation Lut3DOpData::getConcreteInterpolation() const
{
    return GetConcreteInterpolation(m_interpolation);
}

Interpolation Lut3DOpData::GetConcreteInterpolation(Interpolation interp)
{
    switch (interp)
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

void Lut3DOpData::setArrayFromRedFastestOrder(const std::vector<float> & lut)
{
    Array & lutArray = getArray();
    const auto lutSize = lutArray.getLength();

    if (lutSize * lutSize * lutSize * 3 != lut.size())
    {
        std::ostringstream oss;
        oss << "Lut3D length '" << lutSize << " * " << lutSize << " * " << lutSize << " * 3";
        oss << "' does not match the vector size '"<< lut.size()  <<"'.";
        throw Exception(oss.str().c_str());
    }

    for (unsigned long b = 0; b < lutSize; ++b)
    {
        for (unsigned long g = 0; g < lutSize; ++g)
        {
            for (unsigned long r = 0; r < lutSize; ++r)
            {
                // Lut3DOpData Array index. Blue changes fastest.
                const unsigned long blueFastIdx = 3 * ((r*lutSize + g)*lutSize + b);

                // Float array index. Red changes fastest.
                const unsigned long redFastIdx = 3 * ((b*lutSize + g)*lutSize + r);

                lutArray[blueFastIdx + 0] = lut[redFastIdx + 0];
                lutArray[blueFastIdx + 1] = lut[redFastIdx + 1];
                lutArray[blueFastIdx + 2] = lut[redFastIdx + 2];
            }
        }
    }
}

bool Lut3DOpData::IsValidInterpolation(Interpolation interpolation)
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

void Lut3DOpData::validate() const
{
    if (!IsValidInterpolation(m_interpolation))
    {
        std::ostringstream oss;
        oss << "Lut3D does not support interpolation algorithm: ";
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
        // This should never happen. Enforced by resize.
        std::ostringstream oss;
        oss << "Lut3D length: " << getArray().getLength();
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
    return false;
}

OpDataRcPtr Lut3DOpData::getIdentityReplacement() const
{
    return std::make_shared<RangeOpData>(0., 1., 0., 1.);
}

bool Lut3DOpData::haveEqualBasics(const Lut3DOpData & other) const
{
    // TODO: Should interpolation style be considered?
    return (m_array == other.m_array);
}

bool Lut3DOpData::operator==(const OpData & other) const
{
    if (!OpData::operator==(other)) return false;

    const Lut3DOpData* lop = static_cast<const Lut3DOpData*>(&other);

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

bool Lut3DOpData::isInverse(ConstLut3DOpDataRcPtr & other) const
{
    if ((m_direction == TRANSFORM_DIR_FORWARD
         && other->m_direction == TRANSFORM_DIR_INVERSE) ||
        (m_direction == TRANSFORM_DIR_INVERSE
         && other->m_direction == TRANSFORM_DIR_FORWARD))
    {
        return haveEqualBasics(*other);
    }
    return false;
}

Lut3DOpDataRcPtr Lut3DOpData::inverse() const
{
    Lut3DOpDataRcPtr invLut = clone();

    invLut->m_direction = (m_direction == TRANSFORM_DIR_FORWARD) ?
                          TRANSFORM_DIR_INVERSE : TRANSFORM_DIR_FORWARD;

    // Note that any existing metadata could become stale at this point but
    // trying to update it is also challenging since inverse() is sometimes
    // called even during the creation of new ops.
    return invLut;
}

std::string Lut3DOpData::getCacheID() const
{
    AutoMutex lock(m_mutex);

    const Lut3DArray::Values & values = getArray().getValues();

    std::ostringstream cacheIDStream;
    if (!getID().empty())
    {
        cacheIDStream << getID() << " ";
    }

    cacheIDStream << CacheIDHash(reinterpret_cast<const char*>(&values[0]),
                                 int(values.size() * sizeof(values[0])))
                  << " ";

    cacheIDStream << InterpolationToString(m_interpolation)  << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";

    return cacheIDStream.str();
}

void Lut3DOpData::scale(float scale)
{
    getArray().scale(scale);
}

} // namespace OCIO_NAMESPACE

