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

#include "OpDataRange.h"
#include "OpDataMatrix.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace OpData
    {
        Range::Range()
            :   OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
            ,   m_minInValue(Range::EmptyValue())
            ,   m_maxInValue(Range::EmptyValue())
            ,   m_minOutValue(Range::EmptyValue())
            ,   m_maxOutValue(Range::EmptyValue())

            ,   m_scale(0.)
            ,   m_offset(0.)
            ,   m_lowBound(0.)
            ,   m_highBound(0.)
            ,   m_alphaScale(0.)
        {
        }

        Range::Range(const IndexMapping& pIM, BitDepth inDepth, unsigned len)
            :   OpData(inDepth, BIT_DEPTH_F32)
            ,   m_minInValue(Range::EmptyValue())
            ,   m_maxInValue(Range::EmptyValue())
            ,   m_minOutValue(Range::EmptyValue())
            ,   m_maxOutValue(Range::EmptyValue())
            ,   m_scale(0.)
            ,   m_offset(0.)
            ,   m_lowBound(0.)
            ,   m_highBound(0.)
            ,   m_alphaScale(0.)
        {
            if (pIM.getDimension() != 2)
            {
                throw Exception("CTF parsing error. IndexMap must have two entries.");
            }

            float first, second;
            pIM.getPair(0u, first, second);

            // The first half of the pair is scaled to the LUT's input bit depth.
            m_minInValue = first;
            // The second half is scaled to the number of entries in the LUT.
            m_minOutValue = second / (float)(len - 1u);

            // Note: The CLF spec does not say how to handle out-of-range values.
            // E.g., a user could specify an index longer than the LUT length.
            // For now, we are not preventing this (no harm is done since those values
            // are already clipped safely on input to the LUT renderers).

            pIM.getPair(1u, first, second);
            m_maxInValue = first;
            m_maxOutValue = second / (float)(len - 1u);

            validate();

            // The maxOutValues are scaled for 32f, so call the Range version to
            // set the depth and rescale values if necessary.  Note we are prepping
            // things for the input depth of the LUT (which follows the range).
            setOutputBitDepth(inDepth);
        }

        Range::Range(BitDepth inBitDepth,
                     BitDepth outBitDepth,
                     double minInValue,
                     double maxInValue,
                     double minOutValue,
                     double maxOutValue)
            :   OpData(inBitDepth, outBitDepth)
            ,   m_minInValue(minInValue)
            ,   m_maxInValue(maxInValue)
            ,   m_minOutValue(minOutValue)
            ,   m_maxOutValue(maxOutValue)
            ,   m_scale(0.)
            ,   m_offset(0.)
            ,   m_lowBound(0.)
            ,   m_highBound(0.)
            ,   m_alphaScale(0.)
        {
            validate();
        }

        Range::Range(BitDepth inBitDepth,
                     BitDepth outBitDepth,
                     const std::string & id,
                     const std::string & name,
                     const Descriptions & descriptions,
                     double minInValue,
                     double maxInValue,
                     double minOutValue,
                     double maxOutValue)
            :   OpData(inBitDepth, outBitDepth, id, name, descriptions)
            ,   m_minInValue(minInValue)
            ,   m_maxInValue(maxInValue)
            ,   m_minOutValue(minOutValue)
            ,   m_maxOutValue(maxOutValue)
            ,   m_scale(0.)
            ,   m_offset(0.)
            ,   m_lowBound(0.)
            ,   m_highBound(0.)
            ,   m_alphaScale(0.)
        {
            validate();
        }

        Range::~Range()
        {
        }


        const std::string& Range::getOpTypeName() const
        {
            static const std::string type("Range");
            return type;
        }


        void Range::setMinInValue(double value)
        {
            m_minInValue = value;
        }

        bool Range::hasMinInValue() const 
        {
            return !isnan((float)m_minInValue);
        }

        void Range::unsetMinInValue()
        {
            m_minInValue = EmptyValue(); 
        }


        void Range::setMaxInValue(double value)
        {
            m_maxInValue = value;
        }

        bool Range::hasMaxInValue() const 
        {
            return !isnan((float)m_maxInValue);
        }

        void Range::unsetMaxInValue()
        {
            m_maxInValue = EmptyValue(); 
        }


        void Range::setMinOutValue(double value)
        {
            m_minOutValue = value;
        }

        bool Range::hasMinOutValue() const 
        {
            return !isnan((float)m_minOutValue);
        }

        void Range::unsetMinOutValue()
        {
            m_minOutValue = EmptyValue(); 
        }


        void Range::setMaxOutValue(double value)
        {
            m_maxOutValue = value;
        }

        bool Range::hasMaxOutValue() const 
        {
            return !isnan((float)m_maxOutValue);
        }

        void Range::unsetMaxOutValue()
        {
            m_maxOutValue = EmptyValue(); 
        }


        // Important: The spec allows max/min elements to be missing.  When this 
        // happens, we set the member variables to NaN.  The interpretation of this
        // is that no clamping is requested at that bound.  The use of the NaN
        // technique is not exposed outside this module.
        double Range::EmptyValue()
        {
            return std::numeric_limits<double>::quiet_NaN();
        }

        void Range::validate() const
        {
            OpData::validate();

            // NB: Need to allow vals to exceed normal integer range
            // to allow lossless setting of bit-depth from float-->int-->float.

            // If in_min or out_min is not empty, so must the other half be.
            if (isnan((float)m_minInValue))
            {
                if (!isnan((float)m_minOutValue))
                {
                    throw Exception(
                        "In and out minimum limits must be both set or both missing in Range.");
                }
            }
            else
            {
                if (isnan((float)m_minOutValue))
                {
                    throw Exception(
                        "In and out minimum limits must be both set or both missing in Range.");
                }
            }

            if (isnan((float)m_maxInValue))
            {
                if (!isnan((float)m_maxOutValue))
                {
                    throw Exception(
                        "In and out maximum limits must be both set or both missing in Range.");
                }
            }
            else
            {
                if (isnan((float)m_maxOutValue))
                {
                    throw Exception(
                        "In and out maximum limits must be both set or both missing in Range.");
                }
            }

            // Currently not allowing polarity inversion so enforce max > min.
            if (!isnan((float)m_minInValue) && !isnan((float)m_maxInValue))
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
            fillBounds();
        }

        bool Range::isIdentity() const
        {
            // Note that a range op may scale but not clip or vice versa.
            // E.g. 32f --> 32f with non-empty min or max does not scale.
            // 8i --> 16f with empty min & max does not clip.

            // If clipping was requested then the op is not classified as an identity.  
            // This is potentially confusing since the equivalent 1d-LUT would be.
            // However, although it is acceptable to replace an identity LUT with Range,
            // it is not acceptable to omit the Range since then optimization may cause
            // a color change (due to omitting the clip).

            // Originally used "if (minClips() || maxClips())" here but the problem with
            // that is that isIdentity() then becomes a function of the current bit-depths.
            // Although the new approach will say false for some ranges that are currently 
            // identities, the advantage is that as ops are inserted/deleted and the
            // surrounding bit-depths change, this function will be consistent.
            if ( ! minIsEmpty() || ! maxIsEmpty() )
            {
                return false;
            }

            if (scales(true))
            {
                return false;
            }

            return true;
        }

        bool Range::isClamping() const
        {
            // TODO: isClamping has been added to avoid clamping ops to be optimized.
            //       this is a trivial implementation.
            return (!minIsEmpty() || !maxIsEmpty());
        }

        std::auto_ptr<OpData> Range::getIdentityReplacement() const
        {
            return std::auto_ptr<OpData>(
                new Matrix(getInputBitDepth(), getOutputBitDepth()));
        }

        bool Range::isClampIdentity() const
        {
            // No scale or offset allowed.
            if (scales(true))
            {
                return false;
            }

            // If there is clamping, it does not enter into the standard domain.
            // (Considered using minClips() & maxClips() here, but did not want
            //  the result to be bit-depth dependent.)

            if ( !minIsEmpty() && m_minInValue > 0.0f )
            {
                return false;
            }

            if ( !maxIsEmpty() && m_maxInValue < GetBitDepthMaxValue( getInputBitDepth() ) )
            {
                return false;
            }

            return true;
        }

        bool Range::clampsToLutDomain() const
        {
            if ( minIsEmpty() || m_minInValue < 0.0f )
            {
                return false;
            }

            if ( maxIsEmpty() || m_maxInValue > GetBitDepthMaxValue( getInputBitDepth() ) )
            {
                return false;
            }

            return true;
        }

        bool Range::isClampNegs() const
        {
            return  maxIsEmpty() && ! minIsEmpty() && m_minInValue == 0.0f;
        }

        bool Range::FloatsDiffer(double x1, double x2)
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

        bool Range::scales(bool ignoreBitDepth) const
        {
            // Check if offset is non-zero or scale is not unity.

            // Offset is likely to be zero, so cannot do a relative comparison.
            if ( fabs(m_offset) > 1e-6 )
            {
                return true;
            }

            double aim_scale = 1.0;
            if (ignoreBitDepth)
            {
                aim_scale 
                    = GetBitDepthMaxValue(getOutputBitDepth()) 
                        / GetBitDepthMaxValue(getInputBitDepth());
            }

            // AlphaScale may range from 1/65535 to 65535 and Scale even more, however
            // scale is also allowed to be 0, so neither relative or absolute comparison
            // is approprite for all cases.
            if ( FloatsDiffer(m_scale, aim_scale) || 
                 FloatsDiffer(m_alphaScale, aim_scale) )
            {
                return true;
            }

            return false;
        }

        void Range::setInputBitDepth(BitDepth d)
        {
            const double scaleFactor 
                = GetBitDepthMaxValue(d) 
                    / GetBitDepthMaxValue(getInputBitDepth());

            // Call parent to set the input bitdepth
            OpData::setInputBitDepth(d);

            // NB: This may result in int values that are out of range, however,
            // cannot clip them.  Also, empties always have to remain empties.
            // (Need to keep this operation lossless.)

            if (!minIsEmpty())
            {
                m_minInValue *= scaleFactor;
            }
            if (!maxIsEmpty())
            {
                m_maxInValue *= scaleFactor;
            }

            fillScaleOffset();
            fillBounds();
        }

        void Range::setOutputBitDepth(BitDepth d)
        {
            const double scaleFactor
                = GetBitDepthMaxValue(d) 
                    / GetBitDepthMaxValue(getOutputBitDepth());

            // Call parent to set the outputbit depth
            OpData::setOutputBitDepth(d);

            if (!minIsEmpty())
            {
                m_minOutValue *= scaleFactor;
            }
            if (!maxIsEmpty())
            {
                m_maxOutValue *= scaleFactor;
            }

            fillScaleOffset();
            fillBounds();
        }

        bool Range::minIsEmpty() const
        {
            // NB: Validation ensures out is not empty if in is not.
            return isnan((float)m_minInValue) != 0;
        }

        bool Range::maxIsEmpty() const
        {
            // NB: Validation ensures out is not empty if in is not.
            return isnan((float)m_maxInValue) != 0;
        }

        bool Range::minClips() const
        {
            return !isnan((float)m_lowBound);
        }

        bool Range::maxClips() const
        {
            return !isnan((float)m_highBound);
        }

        void Range::fillScaleOffset() const
        {

            // Convert: out = ( in - minIn) * scale + minOut
            // to the model: out = in * scale + offset

            // Note that scaling is required for bit-depth conversion 
            // in addition to whatever range remapping the min/max imply.

            // The case where only one bound clamps and the other is empty
            // is potentially ambiguous regarding how to calculate scale & offset.
            // We set scale to whatever is needed for the bit-depth conversion
            // and set offset such that the requested bound is mapped as requested.
            m_scale 
                = GetBitDepthMaxValue(getOutputBitDepth()) 
                    / GetBitDepthMaxValue(getInputBitDepth());

            m_alphaScale = m_scale;
            if (minIsEmpty())
            {
                if (maxIsEmpty())     // Op is just a bit-depth conversion
                {
                    m_offset = 0.f;
                }
                else                  // Bottom unlimited but top clamps
                {
                    m_offset = m_maxOutValue - m_scale * m_maxInValue;
                }
            }
            else
            {
                if (maxIsEmpty())     // Top unlimited but bottom clamps
                {
                    m_offset = m_minOutValue - m_scale * m_minInValue;
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

        void Range::fillBounds() const
        {
            m_lowBound  = clipOverride(true);
            m_highBound = clipOverride(false);
        }

        double Range::clipOverride(bool isLower) const
        {
            // Unfortunately, the semantics of the Range op is quite complicated.
            //
            // If the max or min are not empty, then clipping has been requested.
            // However, this method determines whether it is actually required.
            // It is required if there are elements of the input domain that
            // after scaling/offset do not fit in the output range.
            // 
            // Sometimes you need to add a clip even if none was requested (float-->int),
            // and sometimes you want to remove the clip (for efficiency) since even
            // though it was requested, it is not necessary.
            //
            // The clip calculated here is what is applied to the output (after scaling).

            // IMPORTANT: This code assumes that if the input is an integer type
            // that values are limited to that domain.  Given the float processing
            // being done (e.g. on GPU) this may not be a safe assumption.

            double inBnd;
            double outBnd;
            double orig;
            bool emptyOrig;

            if (isLower)    // working on the low bound
            {
                inBnd = 0.;
                outBnd = 0.;
                orig = m_minOutValue;
                emptyOrig = minIsEmpty();
            }
            else            // working on the high bound
            {
                inBnd = GetBitDepthMaxValue(getInputBitDepth());
                outBnd = GetBitDepthMaxValue(getOutputBitDepth());
                orig = m_maxOutValue;
                emptyOrig = maxIsEmpty();
            }

            if (emptyOrig)  // no clipping requested, any needed?
            {
                // For float output depths, if it's not requested it's not needed.
                // (One might ask about 32f-->16f, however half.h takes care of this
                //  anyway, so to do it here is unnecessary.)
                // For integer output depths, we may over-ride ...
                if (!IsFloatBitDepth(getOutputBitDepth()))
                {
                    // Float to int always requires clipping.
                    if (IsFloatBitDepth(getInputBitDepth()))
                    {
                        return outBnd;    // over-ride with boundary of integer range
                    }
                    // The int to int case could require clipping.  This could happen if
                    // the bound other than this one is not empty and induces an offset.
                    else
                    {
                        if (wouldClip(inBnd))
                        {
                            return outBnd;  // over-ride with boundary of integer range
                        }
                    }
                }
            }

            else            // clipping requested, but is it needed?
            {
                // For float input depths, if it's requested, it is required.
                // For integer inputs, we may over-ride ...
                if (!IsFloatBitDepth(getInputBitDepth()))
                {
                    // For any out depth, if the int. domain bnds don't clip, nothing's req'd.
                    if (!wouldClip(inBnd))
                    {
                        return Range::EmptyValue();  // over-ride by removing the clip
                    }
                }

                // Since it is necessary to allow the min/max to exceed the integer bounds,
                // may need to over-ride to respect the current output depth.
                if (!IsFloatBitDepth(getOutputBitDepth()))
                {
                    if (isLower)
                    {
                        if (orig < outBnd)
                        {
                            return outBnd;  // over-ride by tightening to the integer range
                        }
                    }
                    else
                    {
                        if (orig > outBnd)
                        {
                            return outBnd;  // over-ride by tightening to the integer range
                        }
                    }
                }
            }

            return orig;            // an over-ride was not necessary
        }

        bool Range::wouldClip(double val) const
        {
            // It may seem like this could be done by simply comparing val
            // to minIn and maxIn.  However, since these must be allowed
            // to be outside the normal integer domain, it is more complicated.
            // Also note that even if out min/max are less than full range,
            // no clipping may actually be required.

            // Map in domain to out range.
            double out = val * m_scale + m_offset;

            // Apply clipping, if any.
            double out_lim = out;
            if (!minIsEmpty())
            {
                out_lim = (out_lim < m_minOutValue) ? m_minOutValue : out_lim;
            }
            if (!maxIsEmpty())
            {
                out_lim = (out_lim > m_maxOutValue) ? m_maxOutValue : out_lim;
            }

            // Additional clipping implied by integer out depths.
            if (!IsFloatBitDepth(getOutputBitDepth()))
            {
                out_lim = clamp(out_lim, 0., (double)GetBitDepthMaxValue(getOutputBitDepth()));
            }

            // Check if clipping altered the output.
            return FloatsDiffer(out, out_lim);
        }

        Matrix* Range::convertToMatrix() const
        {
            std::auto_ptr<Matrix> mtx(new Matrix(getInputBitDepth(),
                getOutputBitDepth()));

            const float scale = getScale();
            const float coefs[9] = { scale, 0.f, 0.f, 0.f, scale, 0.f, 0.f, 0.f, scale };
            mtx->setRGBValues(coefs);

            const float offset = getOffset();
            const float offs[3] = { offset, offset, offset };
            mtx->setRGBOffsets(offs);

            mtx->validate();

            return mtx.release();
        }

        bool Range::operator==(const OpData& other) const
        {
            if (this == &other) return true;

            if (getOpType() != other.getOpType()) return false;

            const Range* rop = static_cast<const Range*>(&other);

            if (!(OpData::operator==(other))) return false;

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

        OpData * Range::clone(CloneType) const
        {
            return new Range(getInputBitDepth(),
                             getOutputBitDepth(),
                             getId(),
                             getName(),
                             getDescriptions(),
                             m_minInValue,
                             m_maxInValue,
                             m_minOutValue,
                             m_maxOutValue);
        }

        void Range::inverse(OpDataVec & ops) const
        {
            // Inverse swaps min/max values.
            // The min/max "include" the scale factor, but since in/out scale are also
            // swapped, no need to rescale the min/max.

            std::auto_ptr<Range> invOp( new Range(getOutputBitDepth(),
                                                  getInputBitDepth(),
                                                  m_minOutValue,
                                                  m_maxOutValue,
                                                  m_minInValue,
                                                  m_maxInValue) );
            invOp->validate();

            ops.append(invOp.release());
        }

        bool Range::isInverse(const OpDataRangeRcPtr & /*r*/) const
        {
            // TODO: To be implemented...
            return false;
        }

        OpDataRcPtr Range::compose(const OpDataRcPtr & /*r*/) const
        {
            throw Exception("TODO");
            return OpDataRangeRcPtr(new Range(getInputBitDepth(), getOutputBitDepth(),
                getMinInValue(), getMaxInValue(),
                getMinOutValue(), getMaxOutValue()));
        }

    } // exit OpData namespace
}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "../UnitTest.h"


OIIO_ADD_TEST(OpDataRange, TestAccessors)
{
    OCIO::OpData::Range r;
    
    OIIO_CHECK_ASSERT(isnan((float)r.getMinInValue()));
    OIIO_CHECK_ASSERT(isnan((float)r.getMaxInValue()));
    OIIO_CHECK_ASSERT(isnan((float)r.getMinOutValue()));
    OIIO_CHECK_ASSERT(isnan((float)r.getMaxOutValue()));

    double minVal = 1.0;
    double maxVal = 10.0;
    r.setMinInValue(minVal);
    r.setMaxInValue(maxVal);
    r.setMinOutValue(2.0*minVal);
    r.setMaxOutValue(2.0*maxVal);

    OIIO_CHECK_ASSERT(r.getMinInValue() == minVal);
    OIIO_CHECK_ASSERT(r.getMaxInValue() == maxVal);
    OIIO_CHECK_ASSERT(r.getMinOutValue() == 2.0*minVal);
    OIIO_CHECK_ASSERT(r.getMaxOutValue() == 2.0*maxVal);

    OIIO_CHECK_ASSERT(r.getOpTypeName() == "Range");
}

OIIO_ADD_TEST(OpDataRange, Validation)
{
    OCIO::OpData::Range r;

    r.setMinInValue(16.);
    r.setMaxInValue(235.);
    // leave min output empty
    r.setMaxOutValue(2.);

    OIIO_CHECK_THROW_WHAT(r.validate(), 
                          OCIO::Exception, 
                          "must be both set or both missing");
}

OIIO_ADD_TEST(OpDataRange, SetBitDepth)
{
    OCIO::OpData::Range 
        r1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_F16,
                    16., 235., -0.5, 2. );

    OCIO::BitDepth newBitdepth = OCIO::BIT_DEPTH_UINT10;
    r1.setOutputBitDepth(newBitdepth);
    OIIO_CHECK_ASSERT(r1.getOutputBitDepth() == newBitdepth);
    OIIO_CHECK_EQUAL((float)r1.getMinInValue(), 16.f);
    OIIO_CHECK_EQUAL((float)r1.getMaxInValue(), 235.f);
    OIIO_CHECK_EQUAL((float)r1.getMinOutValue(), -0.5f * 1023.f);
    OIIO_CHECK_EQUAL((float)r1.getMaxOutValue(), 2.f * 1023.f);

    newBitdepth = OCIO::BIT_DEPTH_F32;
    r1.setOutputBitDepth(newBitdepth);
    OIIO_CHECK_ASSERT(r1.getOutputBitDepth() == newBitdepth);
    OIIO_CHECK_EQUAL((float)r1.getMinOutValue(), -0.5f);
    OIIO_CHECK_EQUAL((float)r1.getMaxOutValue(), 2.f);

    OCIO::OpData::Range 
        r2(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT10, -0.5, 2.0, 0., 1023. );

    newBitdepth = OCIO::BIT_DEPTH_UINT10;
    r2.setInputBitDepth(newBitdepth);
    OIIO_CHECK_ASSERT(r2.getInputBitDepth() == newBitdepth);
    OIIO_CHECK_EQUAL((float)r2.getMinInValue(), -0.5f * 1023.f);
    OIIO_CHECK_EQUAL((float)r2.getMaxInValue(), 2.f * 1023.f);
    OIIO_CHECK_EQUAL((float)r2.getMinOutValue(), 0.f);
    OIIO_CHECK_EQUAL((float)r2.getMaxOutValue(), 1023.f);

    newBitdepth = OCIO::BIT_DEPTH_F16;
    r2.setInputBitDepth(newBitdepth);
    OIIO_CHECK_ASSERT(r2.getInputBitDepth() == newBitdepth);
    OIIO_CHECK_EQUAL((float)r2.getMinInValue(), -0.5f);
    OIIO_CHECK_EQUAL((float)r2.getMaxInValue(), 2.f);

    OCIO::OpData::Range r3;
    r3.setInputBitDepth(OCIO::BIT_DEPTH_F16);
    r3.setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    // leave min in empty
    r3.setMaxInValue(2.);
    // leave min out empty
    r3.setMaxOutValue(1023.);

    r3.validate();

    OIIO_CHECK_ASSERT(r3.minClips());  // since it's float --> int
    OIIO_CHECK_ASSERT(r3.maxClips());
    OIIO_CHECK_EQUAL(r3.getScale(), 1023.f);
    OIIO_CHECK_EQUAL(r3.getOffset(), -1023.f);
    newBitdepth = OCIO::BIT_DEPTH_UINT10;
    r3.setInputBitDepth(newBitdepth);
    OIIO_CHECK_ASSERT(r3.getInputBitDepth() == newBitdepth);
    OIIO_CHECK_ASSERT(r3.minIsEmpty());
    OIIO_CHECK_EQUAL(r3.getScale(), 1.f);
    OIIO_CHECK_EQUAL(r3.getOffset(), -1023.f);
    OIIO_CHECK_EQUAL((float)r3.getMaxInValue(), 2.f * 1023.f);
    OIIO_CHECK_ASSERT(r3.minClips());  // due to negative offset
    OIIO_CHECK_ASSERT(!r3.maxClips()); // not needed since domain max is in range
}

OIIO_ADD_TEST(OpDataRange, clamp_identity )
{
  OCIO::OpData::Range 
    r1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_F16, 0., 255., 0., 1. );
  OIIO_CHECK_ASSERT( r1.clampsToLutDomain() );
  OIIO_CHECK_ASSERT( r1.isClampIdentity() );
  OIIO_CHECK_ASSERT( ! r1.isClampNegs() );

  OCIO::OpData::Range 
    r2(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_F16, 16., 300., -0.5, 2. );
  OIIO_CHECK_ASSERT( ! r2.clampsToLutDomain() );
  OIIO_CHECK_ASSERT( ! r2.isClampIdentity() );
  OIIO_CHECK_ASSERT( ! r2.isClampNegs() );

  OCIO::OpData::Range 
    r3(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_F16, -16., 300., -0.5, 2. );
  OIIO_CHECK_ASSERT( ! r3.clampsToLutDomain() );
  OIIO_CHECK_ASSERT( ! r3.isClampIdentity() );
  OIIO_CHECK_ASSERT( ! r3.isClampNegs() );

  OCIO::OpData::Range 
    r4(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT8, 0., 1., 8., 255. );
  OIIO_CHECK_ASSERT( r4.clampsToLutDomain() );
  OIIO_CHECK_ASSERT( ! r4.isClampIdentity() );
  OIIO_CHECK_ASSERT( ! r4.isClampNegs() );

  OCIO::OpData::Range 
    r5(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT8, 0.1, 1., -8., 255. );
  OIIO_CHECK_ASSERT( r5.clampsToLutDomain() );
  OIIO_CHECK_ASSERT( ! r5.isClampIdentity() );
  OIIO_CHECK_ASSERT( ! r5.isClampNegs() );

  OCIO::OpData::Range 
    r6(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT8, -0.1, 1.1, -255. * 0.1, 255. * 1.1 );
  OIIO_CHECK_ASSERT( ! r6.clampsToLutDomain() );
  OIIO_CHECK_ASSERT( r6.isClampIdentity() );
  OIIO_CHECK_ASSERT( ! r6.isClampNegs() );

  OCIO::OpData::Range 
    r7( OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT8, 
        0., OCIO::OpData::Range::EmptyValue(), 0., OCIO::OpData::Range::EmptyValue() );
  OIIO_CHECK_ASSERT( ! r7.clampsToLutDomain() );
  OIIO_CHECK_ASSERT( r7.isClampIdentity() );
  OIIO_CHECK_ASSERT( r7.isClampNegs() );

  OCIO::OpData::Range 
    r8( OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32, 
        OCIO::OpData::Range::EmptyValue(), 1., OCIO::OpData::Range::EmptyValue(), 1. );
  OIIO_CHECK_ASSERT( ! r8.clampsToLutDomain() );
  OIIO_CHECK_ASSERT( r8.isClampIdentity() );
  OIIO_CHECK_ASSERT( ! r8.isClampNegs() );
}

OIIO_ADD_TEST(OpDataRange, identity)
{
    OCIO::OpData::Range r4(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F16,
                           OCIO::OpData::Range::EmptyValue(), 
                           OCIO::OpData::Range::EmptyValue(), 
                           OCIO::OpData::Range::EmptyValue(), 
                           OCIO::OpData::Range::EmptyValue() );
    OIIO_CHECK_ASSERT(r4.isIdentity());
    OIIO_CHECK_ASSERT(r4.isNoOp());
    OIIO_CHECK_ASSERT(!r4.hasChannelCrosstalk());
    OIIO_CHECK_ASSERT(!r4.minClips());
    OIIO_CHECK_ASSERT(!r4.maxClips());
    OIIO_CHECK_ASSERT(!r4.scales(false));
    OIIO_CHECK_ASSERT(r4.minIsEmpty());
    OIIO_CHECK_ASSERT(r4.maxIsEmpty());

    OCIO::OpData::Range r5(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10,
                           0., 255., 0., 1023. );
    OIIO_CHECK_ASSERT(!r5.wouldClip(0.));
    OIIO_CHECK_ASSERT(!r5.wouldClip(255.));
    OIIO_CHECK_ASSERT(!r5.scales(true));
    OIIO_CHECK_ASSERT(!r5.isIdentity());
    OIIO_CHECK_ASSERT(!r5.hasChannelCrosstalk());
    OIIO_CHECK_ASSERT(!r5.isNoOp());
    OIIO_CHECK_ASSERT(!r5.minClips());
    OIIO_CHECK_ASSERT(!r5.maxClips());
    OIIO_CHECK_ASSERT(r5.scales(false));
    OIIO_CHECK_ASSERT(!r5.minIsEmpty());
    OIIO_CHECK_ASSERT(!r5.maxIsEmpty());

    OCIO::OpData::Range r6(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT16,
                           0., 255., -1., 65540. );
    OIIO_CHECK_ASSERT(!r6.isIdentity());
    OIIO_CHECK_ASSERT(!r6.isNoOp());
    OIIO_CHECK_ASSERT(!r6.hasChannelCrosstalk());
    OIIO_CHECK_ASSERT(r6.minClips());
    OIIO_CHECK_ASSERT(r6.maxClips());
    OIIO_CHECK_EQUAL(r6.getLowBound(), 0.f);  // check tightening of bound
    OIIO_CHECK_EQUAL(r6.getHighBound(), 65535.f);
    OIIO_CHECK_ASSERT(r6.scales(true));
}

OIIO_ADD_TEST(OpDataRange, equality)
{
  OCIO::OpData::Range r1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT16,
                         0., 255., -1., 65540. );

  OCIO::OpData::Range r2(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT16,
                         0.123, 255., -1., 65540. );

  OIIO_CHECK_ASSERT(!(r1 == r2));

  OCIO::OpData::Range r3(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT16,
                         0., 252., -1., 65540. );

  OIIO_CHECK_ASSERT(!(r1 == r3));

  OCIO::OpData::Range r4(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT16,
                         0., 255., -12., 65540. );

  OIIO_CHECK_ASSERT(!(r1 == r4));

  OCIO::OpData::Range r5(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT16,
                         0., 255., -1., 65540. );

  OIIO_CHECK_ASSERT(r5 == r1);

  OCIO::OpData::Range r6(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F16,
                         OCIO::OpData::Range::EmptyValue(),
                         OCIO::OpData::Range::EmptyValue(),
                         OCIO::OpData::Range::EmptyValue(),
                         OCIO::OpData::Range::EmptyValue() );

  OCIO::OpData::Range r7(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F16,
                         OCIO::OpData::Range::EmptyValue(),
                         OCIO::OpData::Range::EmptyValue(), 
                         OCIO::OpData::Range::EmptyValue(),
                         OCIO::OpData::Range::EmptyValue() );

  OIIO_CHECK_ASSERT(r6 == r7);
}

namespace
{
  void checkInverse(OCIO::BitDepth in, OCIO::BitDepth out,
                    double fwdMinIn, double fwdMaxIn,
                    double fwdMinOut, double fwdMaxOut,
                    double revMinIn, double revMaxIn,
                    double revMinOut, double revMaxOut)
  {
    OCIO::OpData::Range 
        refOp(in, out, fwdMinIn, fwdMaxIn, fwdMinOut, fwdMaxOut);

    OCIO::OpData::OpDataVec invOps;
    refOp.inverse(invOps);
    OIIO_CHECK_EQUAL(invOps.size(), 1u);

    const OCIO::OpData::Range* invOp
      = dynamic_cast<const OCIO::OpData::Range*>(invOps.get(0));
    OIIO_CHECK_ASSERT(invOp);

    // Inverse op should have its input/output bitdepth swapped.
    OIIO_CHECK_EQUAL(invOp->getInputBitDepth(), out);
    OIIO_CHECK_EQUAL(invOp->getOutputBitDepth(), in);

    // The min/max values should be swapped.
    if (OCIO::isnan(revMinIn))
      OIIO_CHECK_ASSERT(OCIO::isnan(invOp->getMinInValue()));
    else
      OIIO_CHECK_EQUAL(invOp->getMinInValue(), revMinIn);
    if (OCIO::isnan(revMaxIn))
      OIIO_CHECK_ASSERT(OCIO::isnan(invOp->getMaxInValue()));
    else
      OIIO_CHECK_EQUAL(invOp->getMaxInValue(), revMaxIn);
    if (OCIO::isnan(revMinOut))
      OIIO_CHECK_ASSERT(OCIO::isnan(invOp->getMinOutValue()));
    else
      OIIO_CHECK_EQUAL(invOp->getMinOutValue(), revMinOut);
    if (OCIO::isnan(revMaxOut))
      OIIO_CHECK_ASSERT(OCIO::isnan(invOp->getMaxOutValue()));
    else
      OIIO_CHECK_EQUAL(invOp->getMaxOutValue(), revMaxOut);

    // Check that the computation would be correct.
    // (Doing this in lieu of renderer testing.)

    float fwdScale = refOp.getScale();
    float fwdOffset = refOp.getOffset();
    float revScale = invOp->getScale();
    float revOffset = invOp->getOffset();

    // Want in == (in * fwdScale + fwdOffset) * revScale + revOffset
    // in == in * fwdScale * revScale + fwdOffset * revScale + revOffset
    // in == in * 1. + 0.
    OIIO_CHECK_ASSERT(
        !OCIO::FloatsDiffer(1.0f, fwdScale * revScale, 10, false));
    //OIIO_CHECK_ASSERT(
    //  !OCIO::floatsDiffer(0.0f, fwdOffset * revScale + revOffset, 100000, false));
    // The above is correct but we lose too much precision in the subtraction
    // so rearrange the compare as follows to allow a tighter tolerance.
    OIIO_CHECK_ASSERT(
        !OCIO::FloatsDiffer(fwdOffset * revScale, -revOffset, 500, false));
  }
}

OIIO_ADD_TEST(OpDataRange, inverse_test)
{
  checkInverse(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT12,
               OCIO::OpData::Range::EmptyValue(), OCIO::OpData::Range::EmptyValue(), 
               OCIO::OpData::Range::EmptyValue(), OCIO::OpData::Range::EmptyValue(),
               OCIO::OpData::Range::EmptyValue(), OCIO::OpData::Range::EmptyValue(), 
               OCIO::OpData::Range::EmptyValue(), OCIO::OpData::Range::EmptyValue());

  // Note: All the following result in scale != 1 and offset != 0.

  checkInverse(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT8,
               OCIO::OpData::Range::EmptyValue(), 940., 
               OCIO::OpData::Range::EmptyValue(), 235.,
               OCIO::OpData::Range::EmptyValue(), 235.,
               OCIO::OpData::Range::EmptyValue(), 940.);

  checkInverse(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT8,
               64., OCIO::OpData::Range::EmptyValue(), 
               16., OCIO::OpData::Range::EmptyValue(),
               16., OCIO::OpData::Range::EmptyValue(), 
               64., OCIO::OpData::Range::EmptyValue());

  checkInverse(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT8,
               64., 940., 32., 235.,
               32., 235., 64., 940.);
}

#endif
