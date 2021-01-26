// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "fileformats/ctf/IndexMapping.h"
#include "ops/range/RangeOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
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

    ,   m_scale(1.)
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

RangeOpData::RangeOpData(double minInValue,
                         double maxInValue,
                         double minOutValue,
                         double maxOutValue,
                         TransformDirection dir)
    :   OpData()
    ,   m_minInValue(minInValue)
    ,   m_maxInValue(maxInValue)
    ,   m_minOutValue(minOutValue)
    ,   m_maxOutValue(maxOutValue)
    ,   m_scale(0.)
    ,   m_offset(0.)
{
    setDirection(dir);
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
    // NB: Need to allow vals to exceed normal integer range
    // to allow lossless setting of bit-depth from float-->int-->float.

    // If in_min or out_min is not empty, so must the other half be.
    if (IsNan((float)m_minInValue))
    {
        if (!IsNan((float)m_minOutValue))
        {
            throw Exception("In and out minimum limits must be both set "
                            "or both missing in Range.");
        }
    }
    else
    {
        if (IsNan((float)m_minOutValue))
        {
            throw Exception("In and out minimum limits must be both set "
                            "or both missing in Range.");
        }
    }

    if (IsNan((float)m_maxInValue))
    {
        if (!IsNan((float)m_maxOutValue))
        {
            throw Exception("In and out maximum limits must be both set "
                            "or both missing in Range.");
        }
        if (IsNan((float)m_minInValue))
        {
            throw Exception("At least minimum or maximum limits must be set in Range.");
        }
    }
    else
    {
        if (IsNan((float)m_maxOutValue))
        {
            throw Exception("In and out maximum limits must be both set "
                            "or both missing in Range.");
        }
    }

    // Currently not allowing polarity inversion so enforce max > min.
    if (!IsNan((float)m_minInValue) && !IsNan((float)m_maxInValue))
    {
        if (m_minInValue > m_maxInValue)
        {
            throw Exception("Range maximum input value is less than minimum input value");
        }
        if (m_minOutValue > m_maxOutValue)
        {
            throw Exception("Range maximum output value is less than minimum output value");
        }
    }

    // A one-sided clamp must have matching in & out values.

    if (IsNan((float)m_maxInValue) && !IsNan((float)m_minInValue) &&
        FloatsDiffer(m_minOutValue, m_minInValue))
    {
        throw Exception("In and out minimum limits must be equal "
                        "if maximum values are missing in Range.");
    }

    if (IsNan((float)m_minInValue) && !IsNan((float)m_maxInValue) &&
        FloatsDiffer(m_maxOutValue, m_maxInValue))
    {
        throw Exception("In and out maximum limits must be equal "
                        "if minimum values are missing in Range.");
    }

    // Complete the initialization of the object.
    fillScaleOffset();  // This also validates that maxIn - minIn != 0.
}

// A RangeOp always clamps (the noClamp style is converted to a Matrix).
bool RangeOpData::isNoOp() const
{
    return false;
}

// An identity Range does not modify pixel values between [0,1] but may
// clamp values outside that domain.
bool RangeOpData::isIdentity() const
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
    // relative or absolute comparison is appropriate for all cases.
    if ( FloatsDiffer(m_scale, 1.0) )
    {
        return true;
    }

    return false;
}

RangeOpDataRcPtr RangeOpData::compose(ConstRangeOpDataRcPtr & r) const
{
    double minInNew = m_minInValue;
    double maxInNew = m_maxInValue;
    double minOutNew = r->m_minOutValue;
    double maxOutNew = r->m_maxOutValue;
    if (!minIsEmpty())
    {
        if (!r->maxIsEmpty() && m_minOutValue >= r->m_maxInValue)
        {
            minOutNew = r->m_maxOutValue;
            maxOutNew = r->m_maxOutValue;
            // Range outputting a constant value.
            return std::make_shared<RangeOpData>(m_minInValue, m_maxInValue, minOutNew, maxOutNew);
        }
        else
        {
            if (!r->minIsEmpty())
            {
                if (m_minOutValue >= r->m_minInValue)
                {
                    // Transform m_minOutValue with r.
                    minOutNew = m_minOutValue * r->m_scale + r->m_offset;
                }
                else
                {
                    // Transform r->m_minInValue with inverse of this.
                    minInNew = (r->m_minInValue - m_offset) / m_scale;
                }
            }
            else
            {
                minOutNew = m_minOutValue;
            }
        }
    }
    else if (!r->minIsEmpty())
    {
        // minIsEmpty() is true.
        minInNew = r->m_minInValue;
    }

    if (!maxIsEmpty())
    {
        if (!r->minIsEmpty() && m_maxOutValue <= r->m_minInValue)
        {
            minOutNew = r->m_minOutValue;
            maxOutNew = r->m_minOutValue;
            // Range outputting a constant value.
            return std::make_shared<RangeOpData>(m_minInValue, m_maxInValue, minOutNew, maxOutNew);
        }
        else
        {
            if (!r->maxIsEmpty())
            {
                if (m_maxOutValue <= r->m_maxInValue)
                {
                    // Transform m_maxOutValue with r.
                    maxOutNew = m_maxOutValue * r->m_scale + r->m_offset;
                }
                else
                {
                    // Transform r->m_maxOutValue with inverse of this.
                    maxInNew = (r->m_maxInValue - m_offset) / m_scale;
                }
            }
            else
            {
                maxOutNew = m_maxOutValue;
            }
        }
    }
    else if (!r->maxIsEmpty())
    {
        // maxIsEmpty() is true.
        maxInNew = r->m_maxInValue;
    }

    return std::make_shared<RangeOpData>(minInNew, maxInNew, minOutNew, maxOutNew);
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

    // The case where one bound clamps and the other is empty was potentially ambiguous in
    // versions 1 and 2 of CLF but v3 requires that the in & out bounds must match in
    // this case.  Hence offset must be zero and scale must be 1.
    m_scale = 1.0;

    if (minIsEmpty())
    {
        m_offset = 0.;        // Bottom unlimited but top clamps
    }
    else
    {
        if (maxIsEmpty())     // Top unlimited but bottom clamps
        {
            m_offset = 0.;
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
    if (minIsEmpty() || maxIsEmpty())
    {
        throw Exception("Non-clamping Range min & max values have to be set.");
    }
    const RangeOpData * fwdThis = this;
    ConstRangeOpDataRcPtr tempFwd;
    if (getDirection() == TRANSFORM_DIR_INVERSE)
    {
        tempFwd = getAsForward();
        fwdThis = tempFwd.get();
    }
    // Create an identity matrix.
    MatrixOpDataRcPtr mtx = std::make_shared<MatrixOpData>();
    mtx->getFormatMetadata() = fwdThis->getFormatMetadata();
    mtx->setFileInputBitDepth(fwdThis->getFileInputBitDepth());
    mtx->setFileOutputBitDepth(fwdThis->getFileOutputBitDepth());

    const double scale = fwdThis->getScale();
    mtx->setArrayValue(0, scale);
    mtx->setArrayValue(5, scale);
    mtx->setArrayValue(10, scale);

    const double offset = fwdThis->getOffset();
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
    if (!OpData::operator==(other)) return false;

    const RangeOpData* rop = static_cast<const RangeOpData*>(&other);

    if (m_direction != rop->m_direction)
    {
        return false;
    }

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

void RangeOpData::setDirection(TransformDirection dir) noexcept
{
    m_direction = dir;
}

RangeOpDataRcPtr RangeOpData::getAsForward() const
{
    if (m_direction == TRANSFORM_DIR_FORWARD)
    {
        return clone();
    }
    RangeOpDataRcPtr invOp = std::make_shared<RangeOpData>(getMinOutValue(),
                                                           getMaxOutValue(),
                                                           getMinInValue(),
                                                           getMaxInValue());

    // Note any existing metadata may be stale at this point, but trying to update it is
    // challenging.
    invOp->getFormatMetadata() = getFormatMetadata();
    invOp->m_fileInBitDepth = m_fileOutBitDepth;
    invOp->m_fileOutBitDepth = m_fileInBitDepth;

    invOp->validate();

    return invOp;
}

std::string RangeOpData::getCacheID() const
{
    AutoMutex lock(m_mutex);

    std::ostringstream cacheIDStream;
    if (!getID().empty())
    {
        cacheIDStream << getID() << " ";
    }

    cacheIDStream << TransformDirectionToString(m_direction) << " ";

    cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);

    cacheIDStream << "[" << m_minInValue
                  << ", " << m_maxInValue
                  << ", " << m_minOutValue
                  << ", " << m_maxOutValue
                  << "]";

    return cacheIDStream.str();
}

void RangeOpData::normalize()
{
    const double inScale = 1.0 / GetBitDepthMaxValue(getFileInputBitDepth());
    const double outScale = 1.0 / GetBitDepthMaxValue(getFileOutputBitDepth());
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

} // namespace OCIO_NAMESPACE

