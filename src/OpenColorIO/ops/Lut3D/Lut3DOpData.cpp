// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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

    // TODO: The FastLut will limit inputs to [0,1].  If the forward LUT has an extended range
    // output, perhaps add a Range op before the FastLut to bring values into [0,1].

    // The composition needs to use the EXACT renderer.
    // (Also avoids infinite loop.)
    // So temporarily set the style to EXACT.
    LutStyleGuard<Lut3DOpData> guard(*lut);

    // Make a domain for the composed Lut3D.
    // TODO: Using a large number like 48 here is better for accuracy, 
    // but it causes a delay when creating the renderer. 
    const long GridSize = 48u;
    Lut3DOpDataRcPtr newDomain = std::make_shared<Lut3DOpData>(GridSize);

    newDomain->setFileOutputBitDepth(lut->getFileOutputBitDepth());

    // Compose the LUT newDomain with our inverse LUT (using INV_EXACT style).
    Lut3DOpData::Compose(newDomain, lut);

    // The INV_EXACT inversion style computes an inverse to the tetrahedral
    // style of forward evalutation.
    // TODO: Although this seems like the "correct" thing to do, it does
    // not seem to help accuracy (and is slower).  To investigate ...
    //newLut->setInterpolation(INTERP_TETRAHEDRAL);

    return newDomain;
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
void Lut3DOpData::Compose(Lut3DOpDataRcPtr & A,
                          ConstLut3DOpDataRcPtr & B)
{
    // TODO: Composition of LUTs is a potentially lossy operation.
    // We try to be safe by making the result at least as big as either A or B
    // but we may want to even increase the resolution further.  However,
    // currently composition is done pairs at a time and we would want to
    // determine the increase size once at the start rather than bumping it up
    // as each pair is done.

    // We assume the caller has validated that A and B are forward LUTs.
    // TODO: Support inverse Lut3D here and refactor MakeFastLUT to use it.

    const long min_sz = B->getArray().getLength();
    const long n = A->getArray().getLength();
    OpRcPtrVec ops;

    Lut3DOpDataRcPtr domain;

    if (n >= min_sz)
    {
        // The range of the first LUT becomes the domain to interp in the second.
        // Use the original domain.
        domain = A;
    }
    else
    {
        // Since the 2nd LUT is more finely sampled, use its grid size.

        // Create identity with finer domain.

        // TODO: Should not need to create a new LUT object for this.
        //       Perhaps add a utility function to be shared with the constructor.

        auto metadata = A->getFormatMetadata();
        domain = std::make_shared<Lut3DOpData>(A->getInterpolation(), min_sz);

        domain->getFormatMetadata() = metadata;
        // Interpolate through both LUTs in this case (resample).
        CreateLut3DOp(ops, A, TRANSFORM_DIR_FORWARD);
    }

    // TODO: Would like to not require a clone simply to prevent the delete
    //       from being called on the op when the opList goes out of scope.
    Lut3DOpDataRcPtr clonedB = B->clone();
    CreateLut3DOp(ops, clonedB, TRANSFORM_DIR_FORWARD);


    const BitDepth fileOutBD = A->getFileOutputBitDepth();
    auto metadata = A->getFormatMetadata();

    // Min size, we replace it anyway.
    A = std::make_shared<Lut3DOpData>(A->getInterpolation(), 2);

    // TODO: May want to revisit metadata propagation.
    A->getFormatMetadata() = metadata;
    A->getFormatMetadata().combine(B->getFormatMetadata());

    A->setFileOutputBitDepth(fileOutBD);

    const Array::Values& inValues = domain->getArray().getValues();
    const long gridSize = domain->getArray().getLength();
    const long numPixels = gridSize * gridSize * gridSize;

    A->getArray().resize(gridSize, 3);
    Array::Values& outValues = A->getArray().getValues();

    EvalTransform((const float*)(&inValues[0]),
                  (float*)(&outValues[0]),
                  numPixels,
                  ops);
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
    , m_invQuality(LUT_INVERSION_FAST)
{
}

Lut3DOpData::Lut3DOpData(long gridSize, TransformDirection dir)
    : OpData()
    , m_interpolation(INTERP_DEFAULT)
    , m_array(gridSize)
    , m_direction(dir)
    , m_invQuality(LUT_INVERSION_FAST)
{
}

Lut3DOpData::Lut3DOpData(Interpolation interpolation,
                         unsigned long gridSize)
    : OpData()
    , m_interpolation(interpolation)
    , m_array(gridSize)
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

void Lut3DOpData::setInversionQuality(LutInversionQuality style)
{
    m_invQuality = style;
}

void Lut3DOpData::setArrayFromRedFastestOrder(const std::vector<float> & lut)
{
    Array & lutArray = getArray();
    const auto lutSize = lutArray.getLength();

    if (lutSize * lutSize * lutSize * 3 != lut.size())
    {
        throw Exception("Lut3DOpData length does not match the vector size.");
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

bool Lut3DOpData::haveEqualBasics(const Lut3DOpData & B) const
{
    // TODO: Should interpolation style be considered?
    return (m_array == B.m_array);
}

bool Lut3DOpData::operator==(const OpData & other) const
{
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

bool Lut3DOpData::isInverse(ConstLut3DOpDataRcPtr & B) const
{
    if ((m_direction == TRANSFORM_DIR_FORWARD
         && B->m_direction == TRANSFORM_DIR_INVERSE) ||
        (m_direction == TRANSFORM_DIR_INVERSE
         && B->m_direction == TRANSFORM_DIR_FORWARD))
    {
        return haveEqualBasics(*B);
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

void Lut3DOpData::finalize()
{
    AutoMutex lock(m_mutex);

    validate();

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
    // NB: The m_invQuality is not currently included.

    m_cacheID = cacheIDStream.str();
}

void Lut3DOpData::scale(float scale)
{
    getArray().scale(scale);
}

}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include "UnitTestUtils.h"

OCIO_ADD_TEST(Lut3DOpData, empty)
{
    OCIO::Lut3DOpData l(2);
    OCIO_CHECK_NO_THROW(l.validate());
    OCIO_CHECK_ASSERT(!l.isIdentity());
    OCIO_CHECK_ASSERT(!l.isNoOp());
    OCIO_CHECK_EQUAL(l.getType(), OCIO::OpData::Lut3DType);
    OCIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_FAST);
    OCIO_CHECK_EQUAL(l.getDirection(), OCIO::TRANSFORM_DIR_FORWARD);
    OCIO_CHECK_ASSERT(l.hasChannelCrosstalk());
}

OCIO_ADD_TEST(Lut3DOpData, accessors)
{
    OCIO::Interpolation interpol = OCIO::INTERP_LINEAR;

    OCIO::Lut3DOpData l(interpol, 33);
    l.getFormatMetadata().addAttribute(OCIO::METADATA_ID, "uid");

    OCIO_CHECK_EQUAL(l.getInterpolation(), interpol);

    l.getArray()[0] = 1.0f;

    OCIO_CHECK_ASSERT(!l.isIdentity());
    OCIO_CHECK_NO_THROW(l.validate());

    interpol = OCIO::INTERP_TETRAHEDRAL;
    l.setInterpolation(interpol);
    OCIO_CHECK_EQUAL(l.getInterpolation(), interpol);

    OCIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_FAST);
    l.setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    OCIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_EXACT);
    l.setInversionQuality(OCIO::LUT_INVERSION_FAST);

    OCIO_CHECK_EQUAL(l.getArray().getLength(), (long)33);
    OCIO_CHECK_EQUAL(l.getArray().getNumValues(), (long)33 * 33 * 33 * 3);
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), (long)3);

    l.getArray().resize(17, 3);

    OCIO_CHECK_EQUAL(l.getArray().getLength(), (long)17);
    OCIO_CHECK_EQUAL(l.getArray().getNumValues(), (long)17 * 17 * 17 * 3);
    OCIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), (long)3);
    OCIO_CHECK_NO_THROW(l.validate());
}

OCIO_ADD_TEST(Lut3DOpData, clone)
{
    OCIO::Lut3DOpData ref(33);
    ref.getArray()[1] = 0.1f;

    OCIO::Lut3DOpDataRcPtr pClone = ref.clone();

    OCIO_CHECK_ASSERT(pClone.get());
    OCIO_CHECK_ASSERT(!pClone->isNoOp());
    OCIO_CHECK_ASSERT(!pClone->isIdentity());
    OCIO_CHECK_NO_THROW(pClone->validate());
    OCIO_CHECK_ASSERT(pClone->getArray()==ref.getArray());
}

OCIO_ADD_TEST(Lut3DOpData, not_supported_length)
{
    OCIO_CHECK_NO_THROW(OCIO::Lut3DOpData{ OCIO::Lut3DOpData::maxSupportedLength });
    OCIO_CHECK_THROW_WHAT(OCIO::Lut3DOpData{ OCIO::Lut3DOpData::maxSupportedLength + 1 },
                          OCIO::Exception, "must not be greater");
}

OCIO_ADD_TEST(Lut3DOpData, equality)
{
    OCIO::Lut3DOpData l1(OCIO::INTERP_LINEAR, 33);

    OCIO::Lut3DOpData l2(OCIO::INTERP_BEST, 33);  

    OCIO_CHECK_ASSERT(!(l1 == l2));

    OCIO::Lut3DOpData l3(OCIO::INTERP_LINEAR, 33);  

    OCIO_CHECK_ASSERT(l1 == l3);

    // Inversion quality does not affect forward ops equality.
    l1.setInversionQuality(OCIO::LUT_INVERSION_EXACT);

    OCIO_CHECK_ASSERT(l1 == l3);

    // Inversion quality does not affect inverse ops equality.
    // Even so applying the ops could lead to small differences.
    auto l4 = l1.inverse();
    auto l5 = l3.inverse();

    OCIO_CHECK_ASSERT(*l4 == *l5);
}

OCIO_ADD_TEST(Lut3DOpData, interpolation)
{
    OCIO::Lut3DOpData l(2);
    
    l.setInterpolation(OCIO::INTERP_LINEAR);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_CUBIC);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_CUBIC);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "invalid interpolation");

    l.setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_DEFAULT);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_DEFAULT);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_BEST);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_BEST);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OCIO_CHECK_NO_THROW(l.validate());

    // NB: INTERP_NEAREST is currently implemented as INTERP_LINEAR.
    l.setInterpolation(OCIO::INTERP_NEAREST);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_NEAREST);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_NO_THROW(l.validate());

    // Invalid interpolation type are implemented as INTERP_LINEAR
    // but can not be used because validation throws.
    l.setInterpolation(OCIO::INTERP_UNKNOWN);
    OCIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_UNKNOWN);
    OCIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OCIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "invalid interpolation");
}

OCIO_ADD_TEST(Lut3DOpData, inversion_quality)
{
    OCIO::Lut3DOpData l(2);

    l.setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    OCIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_EXACT);
    OCIO_CHECK_NO_THROW(l.validate());

    l.setInversionQuality(OCIO::LUT_INVERSION_FAST);
    OCIO_CHECK_EQUAL(l.getInversionQuality(), OCIO::LUT_INVERSION_FAST);
    OCIO_CHECK_NO_THROW(l.validate());
}

OCIO_ADD_TEST(Lut3DOpData, is_inverse)
{
    // Create forward LUT.
    OCIO::Lut3DOpDataRcPtr L1NC = std::make_shared<OCIO::Lut3DOpData>(OCIO::INTERP_LINEAR, 5);
    // Set some metadata.
    L1NC->setName("Forward");
    // Make it not an identity.
    OCIO::Array & array = L1NC->getArray();
    OCIO::Array::Values & values = array.getValues();
    values[0] = 20.f;
    OCIO_CHECK_ASSERT(!L1NC->isIdentity());

    // Create an inverse LUT with same basics.
    OCIO::ConstLut3DOpDataRcPtr L1 = L1NC;
    OCIO::Lut3DOpDataRcPtr L2NC = L1->inverse();
    // Change metadata.
    L2NC->setName("Inverse");
    OCIO::ConstLut3DOpDataRcPtr L2 = L2NC;
    OCIO::ConstLut3DOpDataRcPtr L3 = L2->inverse();
    OCIO_CHECK_ASSERT(*L3 == *L1);
    OCIO_CHECK_ASSERT(!(*L1 == *L2));

    // Check isInverse.
    OCIO_CHECK_ASSERT(L1->isInverse(L2));
    OCIO_CHECK_ASSERT(L2->isInverse(L1));
}

OCIO_ADD_TEST(Lut3DOpData, compose)
{
    const std::string spi3dFile("spi_ocio_srgb_test.spi3d");
    OCIO::OpRcPtrVec ops;

    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, spi3dFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    // First op is a FileNoOp.
    OCIO_REQUIRE_EQUAL(2, ops.size());

    const std::string spi3dFile1("comp2.spi3d");
    OCIO::OpRcPtrVec ops1;
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops1, spi3dFile1, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(2, ops1.size());

    OCIO_SHARED_PTR<const OCIO::Op> op0 = ops[1];
    OCIO_SHARED_PTR<const OCIO::Op> op1 = ops1[1];

    OCIO::ConstOpDataRcPtr opData0 = op0->data();
    OCIO::ConstOpDataRcPtr opData1 = op1->data();

    OCIO::ConstLut3DOpDataRcPtr lutData0 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData0);
    OCIO::ConstLut3DOpDataRcPtr lutData1 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData1);

    OCIO_REQUIRE_ASSERT(lutData0);
    OCIO_REQUIRE_ASSERT(lutData1);

    OCIO::Lut3DOpDataRcPtr composed = lutData0->clone();
    composed->setName("lut1");
    composed->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "description of lut1");
    OCIO::Lut3DOpDataRcPtr lutData1Cloned = lutData1->clone();
    lutData1Cloned->setName("lut2");
    lutData1Cloned->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, "description of lut2");
    OCIO::ConstLut3DOpDataRcPtr lutData1ClonedConst = lutData1Cloned;
    OCIO::Lut3DOpData::Compose(composed, lutData1ClonedConst);

    // FormatMetadata composition.
    OCIO_CHECK_EQUAL(composed->getName(), "lut1 + lut2");
    OCIO_REQUIRE_EQUAL(composed->getFormatMetadata().getNumChildrenElements(), 2);
    const auto & desc1 = composed->getFormatMetadata().getChildElement(0);
    OCIO_CHECK_EQUAL(std::string(desc1.getName()), OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(desc1.getValue()), "description of lut1");
    const auto & desc2 = composed->getFormatMetadata().getChildElement(1);
    OCIO_CHECK_EQUAL(std::string(desc2.getName()), OCIO::METADATA_DESCRIPTION);
    OCIO_CHECK_EQUAL(std::string(desc2.getValue()), "description of lut2");

    OCIO_CHECK_EQUAL(composed->getArray().getLength(), (unsigned long)32);
    OCIO_CHECK_EQUAL(composed->getArray().getNumColorComponents(),
        (unsigned long)3);
    OCIO_CHECK_EQUAL(composed->getArray().getNumValues(),
        (unsigned long)32 * 32 * 32 * 3);

    OCIO_CHECK_CLOSE(composed->getArray().getValues()[0], 0.0288210791f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[1], 0.0280428901f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[2], 0.0262413863f, 1e-7f);

    OCIO_CHECK_CLOSE(composed->getArray().getValues()[666], 0.0f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[667], 0.274289876f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[668], 0.854598403f, 1e-7f);

    OCIO_CHECK_CLOSE(composed->getArray().getValues()[1800], 0.0f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[1801], 0.411249638f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[1802], 0.881694913f, 1e-7f);

    OCIO_CHECK_CLOSE(composed->getArray().getValues()[96903], 1.0f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[96904], 0.588273168f, 1e-7f);
    OCIO_CHECK_CLOSE(composed->getArray().getValues()[96905], 0.0f, 1e-7f);

}

OCIO_ADD_TEST(Lut3DOpData, compose_2)
{
    const std::string clfFile("lut3d_bizarre.clf");
    OCIO::OpRcPtrVec ops;

    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, clfFile, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(2, ops.size());

    const std::string clfFile1("lut3d_17x17x17_10i_12i.clf");
    OCIO::OpRcPtrVec ops1;
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops1, clfFile1, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(2, ops1.size());

    OCIO_SHARED_PTR<const OCIO::Op> op0 = ops[1];
    OCIO_SHARED_PTR<const OCIO::Op> op1 = ops1[1];

    OCIO::ConstOpDataRcPtr opData0 = op0->data();
    OCIO::ConstOpDataRcPtr opData1 = op1->data();

    OCIO::ConstLut3DOpDataRcPtr lutData0 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData0);
    OCIO::ConstLut3DOpDataRcPtr lutData1 =
        OCIO::DynamicPtrCast<const OCIO::Lut3DOpData>(opData1);

    OCIO_REQUIRE_ASSERT(lutData0);
    OCIO_REQUIRE_ASSERT(lutData1);

    OCIO::Lut3DOpDataRcPtr composed = lutData0->clone();
    OCIO_CHECK_NO_THROW(OCIO::Lut3DOpData::Compose(composed, lutData1));

    OCIO_CHECK_EQUAL(composed->getArray().getLength(), (unsigned long)17);
    const std::vector<float> & a = composed->getArray().getValues();
    OCIO_CHECK_CLOSE(a[6]    ,    2.5942142f  / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[7]    ,   29.60961342f / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[8]    ,  154.82646179f / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[8289] , 1184.69213867f / 4095.0f, 1e-6f);
    OCIO_CHECK_CLOSE(a[8290] , 1854.97229004f / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[8291] , 1996.75830078f / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[14736], 4094.07617188f / 4095.0f, 1e-7f);
    OCIO_CHECK_CLOSE(a[14737], 4067.37231445f / 4095.0f, 1e-6f);
    OCIO_CHECK_CLOSE(a[14738], 4088.30493164f / 4095.0f, 1e-6f);
}

OCIO_ADD_TEST(Lut3DOpData, inv_lut3d_lut_size)
{
    const std::string fileName("lut3d_17x17x17_10i_12i.clf");
    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(BuildOpsTest(ops, fileName, context,
                                     OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(2, ops.size());

    auto op1 = std::dynamic_pointer_cast<const OCIO::Op>(ops[1]);
    OCIO_REQUIRE_ASSERT(op1);
    auto fwdLutData = std::dynamic_pointer_cast<const OCIO::Lut3DOpData>(op1->data());
    OCIO_REQUIRE_ASSERT(fwdLutData);
    OCIO::ConstLut3DOpDataRcPtr invLutData = fwdLutData->inverse();

    OCIO::Lut3DOpDataRcPtr invFastLutData = MakeFastLut3DFromInverse(invLutData);

    OCIO_CHECK_EQUAL(invFastLutData->getFileOutputBitDepth(), OCIO::BIT_DEPTH_UINT12);

    OCIO_CHECK_EQUAL(invFastLutData->getArray().getLength(), 48);
}

OCIO_ADD_TEST(Lut3DOpData, compose_only_forward)
{
    OCIO::Lut3DOpDataRcPtr lut = std::make_shared<OCIO::Lut3DOpData>(OCIO::INTERP_LINEAR, 5);

    OCIO::OpRcPtrVec ops;
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::TRANSFORM_DIR_INVERSE));
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::TRANSFORM_DIR_FORWARD));
    OCIO_CHECK_NO_THROW(CreateLut3DOp(ops, lut, OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 4);
    OCIO::ConstOpRcPtr op1 = ops[1]; // Inverse.
    OCIO::ConstOpRcPtr op3 = ops[3]; // Forward.
    OCIO_CHECK_ASSERT(!ops[0]->canCombineWith(op1));
    OCIO_CHECK_ASSERT(ops[2]->canCombineWith(op3));
    OCIO_CHECK_ASSERT(!ops[0]->canCombineWith(op3));
    OCIO_CHECK_ASSERT(!ops[2]->canCombineWith(op1));

    // All identity LUT 3d are optimized as a range and combined.
    OCIO::OptimizeFinalizeOpVec(ops);
    OCIO_REQUIRE_EQUAL(ops.size(), 1);
    OCIO_CHECK_EQUAL(ops[0]->getInfo(), "<RangeOp>");
}

#endif

