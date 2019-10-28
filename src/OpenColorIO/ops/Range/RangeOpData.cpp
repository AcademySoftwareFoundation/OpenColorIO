// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.



#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/IndexMapping.h"
#include "ops/Range/RangeOpData.h"
#include "Platform.h"


OCIO_NAMESPACE_ENTER
{

namespace DefaultValues
{
    const int FLOAT_DECIMALS = 7;
}


RangeOpData::RangeOpData()
    :   OpData()
    ,   m_minInValue(RangeOpData::EmptyValue())
    ,   m_maxInValue(RangeOpData::EmptyValue())
    ,   m_minOutValue(RangeOpData::EmptyValue())
    ,   m_maxOutValue(RangeOpData::EmptyValue())

    ,   m_scale(0.)
    ,   m_offset(0.)
{
}

RangeOpData::RangeOpData(double minInValue,
                         double maxInValue,
                         double minOutValue,
                         double maxOutValue)
    :   OpData()
    ,   m_minInValue(minInValue)
    ,   m_maxInValue(maxInValue)
    ,   m_minOutValue(minOutValue)
    ,   m_maxOutValue(maxOutValue)
    ,   m_scale(0.)
    ,   m_offset(0.)
{
    validate();
}

RangeOpData::RangeOpData(const IndexMapping & pIM, unsigned int len, BitDepth bitdepth)
    : OpData()
    ,   m_minInValue(RangeOpData::EmptyValue())
    ,   m_maxInValue(RangeOpData::EmptyValue())
    ,   m_minOutValue(RangeOpData::EmptyValue())
    ,   m_maxOutValue(RangeOpData::EmptyValue())
    ,   m_scale(0.)
    ,   m_offset(0.)
    ,   m_fileInBitDepth(bitdepth)
    ,   m_fileOutBitDepth(bitdepth)
{
    if (pIM.getDimension() != 2)
    {
        throw Exception("CTF/CLF parsing error. Only two entry IndexMaps are supported.");
    }

    const double scaleIn = 1.0 / GetBitDepthMaxValue(bitdepth);

    float first, second;
    pIM.getPair(0u, first, second);

    // The first half of the pair is scaled to the LUT's input bit depth.
    m_minInValue = first * scaleIn;
    // The second half is scaled to the number of entries in the LUT.
    m_minOutValue = second / (double)(len - 1u);

    // Note: The CLF spec does not say how to handle out-of-range values.
    // E.g., a user could specify an index longer than the LUT length.
    // For now, we are not preventing this (no harm is done since those values
    // are already clipped safely on input to the LUT renderers).

    pIM.getPair(1u, first, second);
    m_maxInValue = first * scaleIn;
    m_maxOutValue = second / (double)(len - 1u);

    validate();
}

RangeOpData::~RangeOpData()
{
}

RangeOpDataRcPtr RangeOpData::clone() const
{
    return std::make_shared<RangeOpData>(*this);
}

void RangeOpData::setMinInValue(double value)
{
    m_minInValue = value;
}

bool RangeOpData::hasMinInValue() const 
{
    return !IsNan((float)m_minInValue);
}

void RangeOpData::unsetMinInValue()
{
    m_minInValue = EmptyValue(); 
}


void RangeOpData::setMaxInValue(double value)
{
    m_maxInValue = value;
}

bool RangeOpData::hasMaxInValue() const 
{
    return !IsNan((float)m_maxInValue);
}

void RangeOpData::unsetMaxInValue()
{
    m_maxInValue = EmptyValue(); 
}


void RangeOpData::setMinOutValue(double value)
{
    m_minOutValue = value;
}

bool RangeOpData::hasMinOutValue() const 
{
    return !IsNan((float)m_minOutValue);
}

void RangeOpData::unsetMinOutValue()
{
    m_minOutValue = EmptyValue(); 
}


void RangeOpData::setMaxOutValue(double value)
{
    m_maxOutValue = value;
}

bool RangeOpData::hasMaxOutValue() const 
{
    return !IsNan((float)m_maxOutValue);
}

void RangeOpData::unsetMaxOutValue()
{
    m_maxOutValue = EmptyValue(); 
}


// Important: The spec allows max/min elements to be missing.  When this 
// happens, we set the member variables to NaN.  The interpretation of this
// is that no clamping is requested at that bound.  The use of the NaN
// technique is not exposed outside this module.
double RangeOpData::EmptyValue()
{
    return std::numeric_limits<double>::quiet_NaN();
}

void RangeOpData::validate() const
{
    OpData::validate();

    // NB: Need to allow vals to exceed normal integer range
    // to allow lossless setting of bit-depth from float-->int-->float.

    // If in_min or out_min is not empty, so must the other half be.
    if (IsNan((float)m_minInValue))
    {
        if (!IsNan((float)m_minOutValue))
        {
            throw Exception(
                "In and out minimum limits must be both set or both missing in Range.");
        }
    }
    else
    {
        if (IsNan((float)m_minOutValue))
        {
            throw Exception(
                "In and out minimum limits must be both set or both missing in Range.");
        }
    }

    if (IsNan((float)m_maxInValue))
    {
        if (!IsNan((float)m_maxOutValue))
        {
            throw Exception(
                "In and out maximum limits must be both set or both missing in Range.");
        }
    }
    else
    {
        if (IsNan((float)m_maxOutValue))
        {
            throw Exception(
                "In and out maximum limits must be both set or both missing in Range.");
        }
    }

    // Currently not allowing polarity inversion so enforce max > min.
    if (!IsNan((float)m_minInValue) && !IsNan((float)m_maxInValue))
    {
        if (m_minInValue > m_maxInValue)
        {
            throw Exception(
                "Range maximum input value is less than minimum input value");
        }
        if (m_minOutValue > m_maxOutValue)
        {
            throw Exception(
                "Range maximum output value is less than minimum output value");
        }
    }
            
    // Complete the initialization of the object.
    fillScaleOffset();  // This also validates that maxIn - minIn != 0.
}

bool RangeOpData::isNoOp() const
{
    return isIdentity();
}

bool RangeOpData::isIdentity() const
{
    if ( ! minIsEmpty() || ! maxIsEmpty() )
    {
        return false;
    }

    if (scales())
    {
        return false;
    }

    // TODO: May want to disallow both to be empty or convert to a matrix upon import.
    return true;
}

OpDataRcPtr RangeOpData::getIdentityReplacement() const
{
    auto mat = std::make_shared<MatrixOpData>();
    mat->setFileInputBitDepth(m_fileInBitDepth);
    mat->setFileOutputBitDepth(m_fileOutBitDepth);
    return std::static_pointer_cast<OpData>(mat);
}

bool RangeOpData::isClampIdentity() const
{
    // No scale or offset allowed.
    if (scales())
    {
        return false;
    }

    if ( !minIsEmpty() && m_minInValue > 0.0 )
    {
        return false;
    }

    if ( !maxIsEmpty() && m_maxInValue < 1.0 )
    {
        return false;
    }

    // TODO: We could use this in the optimizer code to avoid the performance penalty associated
    // with removing the clipOverride stuff below. If the inputBD is integer, we could remove a
    // clampIdentity at the start of a chain.If the outputBD is integer, we could remove it at
    // the end of a chain.
    return true;
}

bool RangeOpData::clampsToLutDomain() const
{
    if ( minIsEmpty() || m_minInValue < 0.0 )
    {
        return false;
    }

    if ( maxIsEmpty() || m_maxInValue > 1.0 )
    {
        return false;
    }

    return true;
}

bool RangeOpData::isClampNegs() const
{
    return  maxIsEmpty() && ! minIsEmpty() && m_minInValue == 0.0;
}

bool RangeOpData::FloatsDiffer(double x1, double x2)
{
    // Hybrid absolute/relative comparison.  Tolerances are chosen based on the
    // expected use-cases for the Range op.

    bool different = false;

    if ( fabs(x1) < 1e-3 )
    {
        different = fabs( x1 - x2 ) > 1e-6;  // absolute error near zero
    }
    else
    {
        different = fabs( 1.0 - (x2 / x1) ) > 1e-6; // relative error otherwise
    }

    return different;
}

bool RangeOpData::scales() const
{
    // Check if offset is non-zero or scale is not unity.

    // Offset is likely to be zero, so cannot do a relative comparison.
    if ( fabs(m_offset) > 1e-6 )
    {
        return true;
    }

    // Scale may vary from very small to vary large, however it's also allowed to be 0, so neither
    // relative or absolute comparison is approprite for all cases.
    if ( FloatsDiffer(m_scale, 1.0) )
    {
        return true;
    }

    return false;
}

bool RangeOpData::minIsEmpty() const
{
    // NB: Validation ensures out is not empty if in is not.
    return IsNan((float)m_minInValue) != 0;
}

bool RangeOpData::maxIsEmpty() const
{
    // NB: Validation ensures out is not empty if in is not.
    return IsNan((float)m_maxInValue) != 0;
}

void RangeOpData::fillScaleOffset() const
{

    // Convert: out = ( in - minIn) * scale + minOut
    // to the model: out = in * scale + offset

    // The case where only one bound clamps and the other is empty
    // is potentially ambiguous regarding how to calculate scale & offset.
    // We set scale to whatever is needed for the bit-depth conversion
    // and set offset such that the requested bound is mapped as requested.
    m_scale = 1.0;

    if (minIsEmpty())
    {
        if (maxIsEmpty())     // Op is identity
        {
            m_offset = 0.f;
        }
        else                  // Bottom unlimited but top clamps
        {
            m_offset = m_maxOutValue - m_maxInValue;
        }
    }
    else
    {
        if (maxIsEmpty())     // Top unlimited but bottom clamps
        {
            m_offset = m_minOutValue - m_minInValue;
        }
        else                  // Both ends clamp
        {
            double denom = m_maxInValue - m_minInValue;
            if (fabs(denom) < 1e-6)
            {
                throw Exception("Range maxInValue is too close to minInValue");
            }
            // NB: Allowing out min == max as it could be useful to create a constant.
            m_scale = (m_maxOutValue - m_minOutValue) / denom;
            m_offset = m_minOutValue - m_scale * m_minInValue;
        }
    }
}

MatrixOpDataRcPtr RangeOpData::convertToMatrix() const
{
    // Create an identity matrix.
    MatrixOpDataRcPtr mtx = std::make_shared<MatrixOpData>();
    mtx->getFormatMetadata() = getFormatMetadata();
    mtx->setFileInputBitDepth(m_fileInBitDepth);
    mtx->setFileOutputBitDepth(m_fileOutBitDepth);

    const double scale = getScale();
    mtx->setArrayValue(0, scale);
    mtx->setArrayValue(5, scale);
    mtx->setArrayValue(10, scale);

    const double offset = getOffset();
    mtx->setOffsetValue(0, offset);
    mtx->setOffsetValue(1, offset);
    mtx->setOffsetValue(2, offset);
    mtx->setOffsetValue(3, 0.);

    mtx->validate();

    return mtx;
}

bool RangeOpData::operator==(const OpData & other) const
{
    // NB: FormatMetadata and fileIn/OutDepths are ignored.

    if (this == &other) return true;

    if (!OpData::operator==(other)) return false;

    const RangeOpData* rop = static_cast<const RangeOpData*>(&other);

    if ( (minIsEmpty() != rop->minIsEmpty()) || 
            (maxIsEmpty() != rop->maxIsEmpty()) )
    {
        return false;
    }

    if (!minIsEmpty() && !rop->minIsEmpty() &&
        ( FloatsDiffer(m_minInValue, rop->m_minInValue) ||
            FloatsDiffer(m_minOutValue, rop->m_minOutValue) ) )
    {
        return false;
    }

    if (!maxIsEmpty() && !rop->maxIsEmpty() &&
        ( FloatsDiffer(m_maxInValue, rop->m_maxInValue) ||
            FloatsDiffer(m_maxOutValue, rop->m_maxOutValue) ) )
    {
        return false;
    }

    return true;
}

bool RangeOpData::isInverse(ConstRangeOpDataRcPtr & r) const
{
    return *r == *inverse();
}

RangeOpDataRcPtr RangeOpData::inverse() const
{
    // Inverse swaps min/max values.
    // The min/max "include" the scale factor, but since in/out scale are also
    // swapped, no need to rescale the min/max.

    RangeOpDataRcPtr invOp = std::make_shared<RangeOpData>(getMinOutValue(),
                                                           getMaxOutValue(),
                                                           getMinInValue(),
                                                           getMaxInValue());
    invOp->getFormatMetadata() = getFormatMetadata();
    invOp->setFileInputBitDepth(getFileOutputBitDepth());
    invOp->setFileOutputBitDepth(getFileInputBitDepth());

    invOp->validate();

    // Note that any existing metadata could become stale at this point but
    // trying to update it is also challenging since inverse() is sometimes
    // called even during the creation of new ops.
    return invOp;
}

void RangeOpData::finalize()
{
    AutoMutex lock(m_mutex);

    validate();

    std::ostringstream cacheIDStream;
    cacheIDStream << getID();

    cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);

    cacheIDStream << m_minInValue;
    cacheIDStream << m_maxInValue;
    cacheIDStream << m_minOutValue;
    cacheIDStream << m_maxOutValue;

    m_cacheID = cacheIDStream.str();
}

void RangeOpData::scale(double inScale, double outScale)
{
    if (!minIsEmpty())
    {
        m_minInValue *= inScale;
    }
    if (!maxIsEmpty())
    {
        m_maxInValue *= inScale;
    }

    if (!minIsEmpty())
    {
        m_minOutValue *= outScale;
    }
    if (!maxIsEmpty())
    {
        m_maxOutValue *= outScale;
    }

}

}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "UnitTest.h"


OCIO_ADD_TEST(RangeOpData, accessors)
{
    {
    OCIO::RangeOpData r;
    
    OCIO_CHECK_ASSERT(OCIO::IsNan((float)r.getMinInValue()));
    OCIO_CHECK_ASSERT(OCIO::IsNan((float)r.getMaxInValue()));
    OCIO_CHECK_ASSERT(OCIO::IsNan((float)r.getMinOutValue()));
    OCIO_CHECK_ASSERT(OCIO::IsNan((float)r.getMaxOutValue()));

    double minVal = 1.0;
    double maxVal = 10.0;
    r.setMinInValue(minVal);
    r.setMaxInValue(maxVal);
    r.setMinOutValue(2.0*minVal);
    r.setMaxOutValue(2.0*maxVal);

    OCIO_CHECK_EQUAL(r.getMinInValue(), minVal);
    OCIO_CHECK_EQUAL(r.getMaxInValue(), maxVal);
    OCIO_CHECK_EQUAL(r.getMinOutValue(), 2.0*minVal);
    OCIO_CHECK_EQUAL(r.getMaxOutValue(), 2.0*maxVal);

    OCIO_CHECK_EQUAL(r.getType(), OCIO::OpData::RangeType);
    }

    {
    const float g_error = 1e-7f;

    OCIO::RangeOpData range(0.0f, 1.0f, 0.5f, 1.5f);

    OCIO_CHECK_EQUAL(range.getMinInValue(), 0.);
    OCIO_CHECK_EQUAL(range.getMaxInValue(), 1.);
    OCIO_CHECK_EQUAL(range.getMinOutValue(), 0.5);
    OCIO_CHECK_EQUAL(range.getMaxOutValue(), 1.5);

    OCIO_CHECK_NO_THROW(range.setMinInValue(-0.05432));
    OCIO_CHECK_NO_THROW(range.validate());
    OCIO_CHECK_EQUAL(range.getMinInValue(), -0.05432);

    OCIO_CHECK_NO_THROW(range.setMaxInValue(1.05432));
    OCIO_CHECK_NO_THROW(range.validate());
    OCIO_CHECK_EQUAL(range.getMaxInValue(), 1.05432);

    OCIO_CHECK_NO_THROW(range.setMinOutValue(0.05432));
    OCIO_CHECK_NO_THROW(range.validate());
    OCIO_CHECK_EQUAL(range.getMinOutValue(), 0.05432);

    OCIO_CHECK_NO_THROW(range.setMaxOutValue(2.05432));
    OCIO_CHECK_NO_THROW(range.validate());
    OCIO_CHECK_EQUAL(range.getMaxOutValue(), 2.05432);

    OCIO_CHECK_CLOSE(range.getScale(), 1.804012123, g_error);
    OCIO_CHECK_CLOSE(range.getOffset(), 0.1523139385, g_error);
    }
}

OCIO_ADD_TEST(RangeOpData, validation)
{
    OCIO::RangeOpData r;

    r.setMinInValue(16.);
    r.setMaxInValue(235.);
    // leave min output empty
    r.setMaxOutValue(2.);

    OCIO_CHECK_THROW_WHAT(r.validate(), 
                          OCIO::Exception, 
                          "must be both set or both missing");
}

OCIO_ADD_TEST(RangeOpData, clamp_identity)
{
    OCIO::RangeOpData r1(0., 1., 0., 1. );
    OCIO_CHECK_ASSERT( r1.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( r1.isClampIdentity() );
    OCIO_CHECK_ASSERT( ! r1.isClampNegs() );

    OCIO::RangeOpData r2(0.1, 1.2, -0.5, 2. );
    OCIO_CHECK_ASSERT( ! r2.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( ! r2.isClampIdentity() );
    OCIO_CHECK_ASSERT( ! r2.isClampNegs() );

    OCIO::RangeOpData r3(-0.1, 1.0, -0.5, 2. );
    OCIO_CHECK_ASSERT( ! r3.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( ! r3.isClampIdentity() );
    OCIO_CHECK_ASSERT( ! r3.isClampNegs() );

    OCIO::RangeOpData r4(0., 1., 0.01, 1. );
    OCIO_CHECK_ASSERT( r4.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( ! r4.isClampIdentity() );
    OCIO_CHECK_ASSERT( ! r4.isClampNegs() );

    OCIO::RangeOpData r5( 0.1, 1., -0.01, 1. );
    OCIO_CHECK_ASSERT( r5.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( ! r5.isClampIdentity() );
    OCIO_CHECK_ASSERT( ! r5.isClampNegs() );

    OCIO::RangeOpData r6(-0.1, 1.1, -0.1, 1.1 );
    OCIO_CHECK_ASSERT( ! r6.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( r6.isClampIdentity() );
    OCIO_CHECK_ASSERT( ! r6.isClampNegs() );

    OCIO::RangeOpData r7(0., OCIO::RangeOpData::EmptyValue(), 
                         0., OCIO::RangeOpData::EmptyValue());
    OCIO_CHECK_ASSERT( ! r7.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( r7.isClampIdentity() );
    OCIO_CHECK_ASSERT( r7.isClampNegs() );

    OCIO::RangeOpData r8(OCIO::RangeOpData::EmptyValue(), 1., 
                         OCIO::RangeOpData::EmptyValue(), 1. );
    OCIO_CHECK_ASSERT( ! r8.clampsToLutDomain() );
    OCIO_CHECK_ASSERT( r8.isClampIdentity() );
    OCIO_CHECK_ASSERT( ! r8.isClampNegs() );
}

OCIO_ADD_TEST(RangeOpData, identity)
{
    OCIO::RangeOpData r4(OCIO::RangeOpData::EmptyValue(), 
                         OCIO::RangeOpData::EmptyValue(), 
                         OCIO::RangeOpData::EmptyValue(), 
                         OCIO::RangeOpData::EmptyValue() );
    OCIO_CHECK_ASSERT(r4.isIdentity());
    OCIO_CHECK_ASSERT(r4.isNoOp());
    OCIO_CHECK_ASSERT(!r4.hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!r4.scales());
    OCIO_CHECK_ASSERT(r4.minIsEmpty());
    OCIO_CHECK_ASSERT(r4.maxIsEmpty());

    OCIO::RangeOpData r5(0., 1., 0., 1. );
    OCIO_CHECK_ASSERT(!r5.scales());
    OCIO_CHECK_ASSERT(!r5.isIdentity());
    OCIO_CHECK_ASSERT(!r5.hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!r5.isNoOp());
    OCIO_CHECK_ASSERT(!r5.minIsEmpty());
    OCIO_CHECK_ASSERT(!r5.maxIsEmpty());

    OCIO::RangeOpData r6(0., 1., -1., 1. );
    OCIO_CHECK_ASSERT(!r6.isIdentity());
    OCIO_CHECK_ASSERT(!r6.isNoOp());
    OCIO_CHECK_ASSERT(!r6.hasChannelCrosstalk());
    OCIO_CHECK_ASSERT(!r6.minIsEmpty());
    OCIO_CHECK_ASSERT(!r6.maxIsEmpty());
    OCIO_CHECK_EQUAL(r6.getMinOutValue(), -1.f);
    OCIO_CHECK_EQUAL(r6.getMaxOutValue(), 1.f);
    OCIO_CHECK_ASSERT(r6.scales());
}

OCIO_ADD_TEST(RangeOpData, equality)
{
    OCIO::RangeOpData r1(0., 1., -1., 1. );

    OCIO::RangeOpData r2(0.123, 1., -1., 1. );

    OCIO_CHECK_ASSERT(!(r1 == r2));

    OCIO::RangeOpData r3(0., 0.99, -1., 1. );

    OCIO_CHECK_ASSERT(!(r1 == r3));

    OCIO::RangeOpData r4(0., 1., -12., 1. );

    OCIO_CHECK_ASSERT(!(r1 == r4));

    OCIO::RangeOpData r5(0., 1., -1., 1. );

    OCIO_CHECK_ASSERT(r5 == r1);

    OCIO::RangeOpData r6(OCIO::RangeOpData::EmptyValue(),
                         OCIO::RangeOpData::EmptyValue(),
                         OCIO::RangeOpData::EmptyValue(),
                         OCIO::RangeOpData::EmptyValue() );

    OCIO::RangeOpData r7(OCIO::RangeOpData::EmptyValue(),
                         OCIO::RangeOpData::EmptyValue(), 
                         OCIO::RangeOpData::EmptyValue(),
                         OCIO::RangeOpData::EmptyValue() );

    OCIO_CHECK_ASSERT(r6 == r7);
}

OCIO_ADD_TEST(RangeOpData, faulty)
{
    {
        OCIO::RangeOpData range(0.0f, 1.0f, 0.5f, 1.5f);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.unsetMinInValue()); 
        OCIO_CHECK_THROW_WHAT(range.validate(), 
                              OCIO::Exception,
                              "In and out minimum limits must be both set or both missing");
    }

    {
        OCIO::RangeOpData range(0.0f, 1.0f, 0.5f, 1.5f);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.unsetMinInValue()); 
        OCIO_CHECK_NO_THROW(range.unsetMinOutValue()); 
        OCIO_CHECK_NO_THROW(range.validate());
    }

    {
        OCIO::RangeOpData range(0.0f, 1.0f, 0.5f, 1.5f);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.unsetMaxInValue()); 
        OCIO_CHECK_THROW_WHAT(range.validate(), 
                              OCIO::Exception,
                              "In and out maximum limits must be both set or both missing");
    }

    {
        OCIO::RangeOpData range(0.0f, 1.0f, 0.5f, 1.5f);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.unsetMaxInValue()); 
        OCIO_CHECK_NO_THROW(range.unsetMaxOutValue()); 
        OCIO_CHECK_NO_THROW(range.validate());
    }

    {
        OCIO::RangeOpData range(0.0f, 1.0f, 0.5f, 1.5f);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.setMaxInValue(-125.)); 
        OCIO_CHECK_THROW_WHAT(range.validate(), 
                              OCIO::Exception,
                              "Range maximum input value is less than minimum input value");
    }

    {
        OCIO::RangeOpData range(0.0f, 1.0f, 0.5f, 1.5f);
        OCIO_CHECK_NO_THROW(range.validate());

        OCIO_CHECK_NO_THROW(range.setMaxOutValue(-125.)); 
        OCIO_CHECK_THROW_WHAT(range.validate(), 
                              OCIO::Exception,
                              "Range maximum output value is less than minimum output value");
    }
}

namespace
{
void checkInverse(double fwdMinIn, double fwdMaxIn,
                    double fwdMinOut, double fwdMaxOut,
                    double revMinIn, double revMaxIn,
                    double revMinOut, double revMaxOut)
{
    OCIO::RangeOpData refOp(fwdMinIn, fwdMaxIn, fwdMinOut, fwdMaxOut);

    OCIO::RangeOpDataRcPtr invOp = refOp.inverse();

    // The min/max values should be swapped.
    if (OCIO::IsNan(revMinIn))
        OCIO_CHECK_ASSERT(OCIO::IsNan(invOp->getMinInValue()));
    else
        OCIO_CHECK_EQUAL(invOp->getMinInValue(), revMinIn);
    if (OCIO::IsNan(revMaxIn))
        OCIO_CHECK_ASSERT(OCIO::IsNan(invOp->getMaxInValue()));
    else
        OCIO_CHECK_EQUAL(invOp->getMaxInValue(), revMaxIn);
    if (OCIO::IsNan(revMinOut))
        OCIO_CHECK_ASSERT(OCIO::IsNan(invOp->getMinOutValue()));
    else
        OCIO_CHECK_EQUAL(invOp->getMinOutValue(), revMinOut);
    if (OCIO::IsNan(revMaxOut))
        OCIO_CHECK_ASSERT(OCIO::IsNan(invOp->getMaxOutValue()));
    else
        OCIO_CHECK_EQUAL(invOp->getMaxOutValue(), revMaxOut);

    // Check that the computation would be correct.
    // (Doing this in lieu of renderer testing.)

    const float fwdScale  = (float)refOp.getScale();
    const float fwdOffset = (float)refOp.getOffset();
    const float revScale  = (float)invOp->getScale();
    const float revOffset = (float)invOp->getOffset();

    // Want in == (in * fwdScale + fwdOffset) * revScale + revOffset
    // in == in * fwdScale * revScale + fwdOffset * revScale + revOffset
    // in == in * 1. + 0.
    OCIO_CHECK_ASSERT(!OCIO::FloatsDiffer( 1.0f, fwdScale * revScale, 10, false));

    //OCIO_CHECK_ASSERT(!OCIO::floatsDiffer(
    //     0.0f, fwdOffset * revScale + revOffset, 100000, false));
    // The above is correct but we lose too much precision in the subtraction
    // so rearrange the compare as follows to allow a tighter tolerance.
    OCIO_CHECK_ASSERT(!OCIO::FloatsDiffer(fwdOffset * revScale, -revOffset, 500, false));
}
}

OCIO_ADD_TEST(RangeOpData, inverse)
{
    checkInverse(OCIO::RangeOpData::EmptyValue(), OCIO::RangeOpData::EmptyValue(), 
                 OCIO::RangeOpData::EmptyValue(), OCIO::RangeOpData::EmptyValue(),
                 OCIO::RangeOpData::EmptyValue(), OCIO::RangeOpData::EmptyValue(),
                 OCIO::RangeOpData::EmptyValue(), OCIO::RangeOpData::EmptyValue());

    // Note: All the following result in scale != 1 and offset != 0.

    checkInverse(OCIO::RangeOpData::EmptyValue(), 0.940, OCIO::RangeOpData::EmptyValue(), 0.235,
                 OCIO::RangeOpData::EmptyValue(), 0.235, OCIO::RangeOpData::EmptyValue(), 0.940);

    checkInverse(0.64, OCIO::RangeOpData::EmptyValue(), 0.16, OCIO::RangeOpData::EmptyValue(),
                 0.16, OCIO::RangeOpData::EmptyValue(), 0.64, OCIO::RangeOpData::EmptyValue());

    checkInverse(0.064, 0.940, 0.032, 0.235,
                 0.032, 0.235, 0.064, 0.940);
}

OCIO_ADD_TEST(RangeOpData, computed_identifier)
{
    std::string id1, id2;

    OCIO::RangeOpData range(0.0, 1.0, 0.5, 1.5);
    OCIO_CHECK_NO_THROW( range.finalize() );
    OCIO_CHECK_NO_THROW( id1 = range.getCacheID() );

    OCIO_CHECK_NO_THROW( range.unsetMaxInValue() ); 
    OCIO_CHECK_NO_THROW( range.unsetMaxOutValue() ); 
    OCIO_CHECK_NO_THROW( range.finalize() );
    OCIO_CHECK_NO_THROW( id2 = range.getCacheID() );

    OCIO_CHECK_ASSERT( id1 != id2 );

    OCIO_CHECK_NO_THROW( range.unsetMinInValue() ); 
    OCIO_CHECK_NO_THROW( range.unsetMinOutValue() ); 
    OCIO_CHECK_NO_THROW( range.finalize() );
    OCIO_CHECK_NO_THROW( id1 = range.getCacheID() );

    OCIO_CHECK_ASSERT( id1 != id2 );

    OCIO_CHECK_NO_THROW( id2 = range.getCacheID() );
    OCIO_CHECK_ASSERT( id1 == id2 );
}

#endif
