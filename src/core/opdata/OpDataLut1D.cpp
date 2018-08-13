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

#include "OpDataLut1D.h"
#include "OpDataInvLut1D.h"
#include "OpDataRange.h"
#include "OpDataMatrix.h"
#include "OpDataTools.h"

#include <sstream>

OCIO_NAMESPACE_ENTER
{
    // Number of possible values for the Half domain
    static const unsigned HALF_DOMAIN_REQUIRED_ENTRIES = 65536;

    namespace OpData
    {
        Lut1D::Lut3by1DArray::Lut3by1DArray(BitDepth inBitDepth,
                                            BitDepth outBitDepth,
                                            HalfFlags halfFlags)
        {
            resize(GetLutIdealSize(inBitDepth, halfFlags), getMaxColorComponents());
            fill(halfFlags, outBitDepth);
        }

        Lut1D::Lut3by1DArray::Lut3by1DArray(BitDepth outBitDepth,
                                            HalfFlags halfFlags,
                                            unsigned length)
        {
            resize(length, getMaxColorComponents());
            fill(halfFlags, outBitDepth);
        }

        Lut1D::Lut3by1DArray::~Lut3by1DArray()
        {
        }

        void Lut1D::Lut3by1DArray::fill(HalfFlags halfFlags,
                                        BitDepth outBitDepth)
        {
            const unsigned dim = getLength();
            const unsigned maxChannels = getMaxColorComponents();

            Array::Values& values = getValues();
            if ((halfFlags & LUT_INPUT_HALF_CODE) == LUT_INPUT_HALF_CODE)
            {
                const float scaleFactor = GetBitDepthMaxValue(outBitDepth);

                for (unsigned idx = 0; idx<dim; ++idx)
                {
                    half htemp; htemp.setBits((unsigned short)idx);
                    const float ftemp = htemp * scaleFactor;

                    const unsigned row = maxChannels * idx;
                    for (unsigned channel = 0; channel<maxChannels; ++channel)
                    {
                        values[channel + row] = ftemp;
                    }
                }

            }
            else
            {
                const float stepValue 
                    = GetBitDepthMaxValue(outBitDepth) / ((float)dim - 1.0f);

                for(unsigned idx = 0; idx<dim; ++idx)
                {
                    const float ftemp = (float)idx * stepValue;

                    const unsigned row = maxChannels * idx;
                    for (unsigned channel = 0; channel<maxChannels; ++channel)
                    {
                        values[channel + row] = ftemp;
                    }
                }
            }
        }

        bool Lut1D::Lut3by1DArray::isIdentity(HalfFlags halfFlags,
                                              BitDepth outBitDepth) const
        {
            // An identity LUT does nothing except possibly bit-depth conversion.

            const unsigned dim = getLength();
            const Array::Values& values = getValues();
            const unsigned maxChannels = getMaxColorComponents();

            if((halfFlags & Lut1D::LUT_INPUT_HALF_CODE) 
                == Lut1D::LUT_INPUT_HALF_CODE)
            {

                const float scaleFactor = GetBitDepthMaxValue(outBitDepth);

                for(unsigned idx=0; idx<dim; ++idx)
                {
                    half htemp; htemp.setBits((unsigned short)idx);
                    const float ftemp = htemp * scaleFactor;

                    const half aimHalf = ftemp;

                    const unsigned row = maxChannels * idx;
                    for(unsigned channel=0; channel<maxChannels; ++channel)
                    {
                        const half valHalf = values[channel + row];
                        if(HalfsDiffer(aimHalf, valHalf, 1))
                        {
                            return false;
                        }
                    }
                }
            }
            else
            {
                const float stepValue = GetValueStepSize(outBitDepth, dim);

                const float rel_tol = 1e-6f;
                const float abs_tol = GetBitDepthMaxValue(outBitDepth) * rel_tol;

                for(unsigned idx=0; idx<dim; ++idx)
                {
                    const float aim = (float)idx * stepValue;

                    const unsigned row = maxChannels * idx;
                    for(unsigned channel=0; channel<maxChannels; ++channel)
                    {
                        const float err = values[ channel + row ] - aim;

                        if ( fabs(err) > abs_tol )
                        {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        unsigned Lut1D::Lut3by1DArray::getNumValues() const
        {
            return getLength() * getMaxColorComponents();
        }

        Lut1D::Lut1D(unsigned dimension)
            :   OpData(BIT_DEPTH_F32, BIT_DEPTH_F32)
            ,   m_interpolation(INTERP_LINEAR)
            ,   m_array(getOutputBitDepth(), LUT_STANDARD, dimension)
            ,   m_halfFlags(LUT_STANDARD)
            ,   m_hueAdjust(HUE_NONE)
        {
        }

        Lut1D::Lut1D(BitDepth inBitDepth, BitDepth outBitDepth, HalfFlags halfFlags)
            :   OpData(inBitDepth, outBitDepth)
            ,   m_interpolation(INTERP_LINEAR)
            ,   m_array(getInputBitDepth(), getOutputBitDepth(), halfFlags)
            ,   m_halfFlags(halfFlags)
            ,   m_hueAdjust(HUE_NONE)
        {
        }

        Lut1D::Lut1D(BitDepth inBitDepth, BitDepth outBitDepth,
                     const std::string& id, const std::string & name,
                     const Descriptions& descriptions,
                     Interpolation interpolation,
                     HalfFlags halfFlags)
            :   OpData(inBitDepth, outBitDepth, id, name, descriptions)
            ,   m_interpolation(interpolation)
            ,   m_array(getInputBitDepth(), getOutputBitDepth(), halfFlags)
            ,   m_halfFlags(halfFlags)
            ,   m_hueAdjust(HUE_NONE)
        {
        }

        Lut1D::Lut1D(BitDepth inBitDepth, BitDepth outBitDepth,
                     const std::string& id, const std::string& name,
                     const Descriptions& descriptions,
                     Interpolation interpolation,
                     HalfFlags halfFlags,
                     unsigned dimension)
            :   OpData(inBitDepth, outBitDepth, id, name, descriptions)
            ,   m_interpolation(interpolation)
            ,   m_array(getInputBitDepth(), halfFlags, dimension)
            ,   m_halfFlags(halfFlags)
            ,   m_hueAdjust(HUE_NONE)
        {
        }

        Lut1D::~Lut1D()
        {
        }

        const std::string& Lut1D::getOpTypeName() const
        {
            static const std::string type("LUT 1D");
            return type;
        }

        OpData::OpType Lut1D::getOpType() const
        {
            return OpData::Lut1DType;
        }

        Interpolation Lut1D::getInterpolation(const char * str)
        {
            if (str && *str)
            {
                if (0 == strcmp(str, "linear"))
                {
                    return INTERP_LINEAR;
                }
                else if (0 == strcmp(str, "cubic"))
                {
                    return INTERP_CUBIC;
                }
                else if (0 == strcmp(str, "default"))
                {
                    return INTERP_DEFAULT;
                }
                
                std::ostringstream oss;
                oss << "1D LUT interpolation not recongnized: ";
                oss << str;

                throw Exception(oss.str().c_str());
            }

            throw Exception("1D LUT missing interpolation value.");

        }

        Interpolation Lut1D::getConcreteInterpolation() const
        {
            // TODO: BEST should probably be CUBIC, changing the OCIO v1 behavior
            if (m_interpolation == INTERP_DEFAULT
                || m_interpolation == INTERP_BEST)
            {
                return INTERP_LINEAR;
            }

            // TODO: currently INTERP_NEAREST is not implemented.
            // This is a regression for OCIO v1.
            if (m_interpolation == INTERP_NEAREST)
            {
                return INTERP_LINEAR;
            }

            // TODO: currently INTERP_CUBIC is not implemented.
            if (m_interpolation == INTERP_CUBIC)
            {
                return INTERP_LINEAR;
            }

            return m_interpolation;
        }

        void Lut1D::setInterpolation(Interpolation algo)
        {
            m_interpolation = algo;
        }

        bool Lut1D::isIdentity() const
        {
            return m_array.isIdentity(m_halfFlags, getOutputBitDepth());
        }

        bool Lut1D::hasChannelCrosstalk() const
        {
            if (getHueAdjust() != Lut1D::HUE_NONE)
            {
               return !isIdentity();
            }
            else
            {
                return false;
            }
        }

        bool Lut1D::mayCompose(const Lut1D & lut) const
        {
            // NB: This does not check bypass or dynamic.
            return     getHueAdjust() == Lut1D::HUE_NONE  &&
                   lut.getHueAdjust() == Lut1D::HUE_NONE;
        }

        std::auto_ptr<OpData> Lut1D::getIdentityReplacement() const
        {
            const BitDepth inBD = getInputBitDepth();
            const BitDepth outBD = getOutputBitDepth();
            std::auto_ptr<OpData> op;

            if (isInputHalfDomain())
            {
                op.reset(new Matrix(inBD, outBD));
            }
            else
            {
                op.reset(new Range(inBD,
                                   outBD,
                                   0.,
                                   GetBitDepthMaxValue(inBD),
                                   0.,
                                   GetBitDepthMaxValue(outBD)));
            }
            return op;
        }

        void Lut1D::setOutputBitDepth(BitDepth out)
        {
            // Scale factor is max_new_depth/max_old_depth
            const float scaleFactor
                = GetBitDepthMaxValue(out)
                    / GetBitDepthMaxValue(getOutputBitDepth());

            // Call parent to set the outputbitdepth
            OpData::setOutputBitDepth(out);

            // Scale array by scaleFactor, 
            // don't scale if scaleFactor = 1.0f
            if (scaleFactor != 1.0f)
            {
                Array::Values& arrayVals = m_array.getValues();
                const size_t size = arrayVals.size();

                for (unsigned i = 0; i < size; i++)
                {
                    arrayVals[i] *= scaleFactor;
                }
            }
        }

        void Lut1D::setInputHalfDomain(bool isHalfDomain)
        {
            m_halfFlags = (isHalfDomain) ?
                ((HalfFlags) (m_halfFlags | LUT_INPUT_HALF_CODE)) :
                ((HalfFlags) (m_halfFlags & ~LUT_INPUT_HALF_CODE));
        }

        void Lut1D::setOutputRawHalfs(bool isRawHalfs)
        {
            m_halfFlags = (isRawHalfs) ?
                ((HalfFlags) (m_halfFlags | LUT_OUTPUT_HALF_CODE)) :
                ((HalfFlags) (m_halfFlags & ~LUT_OUTPUT_HALF_CODE));
        }

        void Lut1D::validate() const
        {
            if(getInputBitDepth()==BIT_DEPTH_UNKNOWN)
            {
                throw Exception("Unknown bit depth");
            }

            if(getOutputBitDepth()==BIT_DEPTH_UNKNOWN)
            {
                throw Exception("Unknown bit depth");
            }

            Interpolation interp = getConcreteInterpolation();

            if(interp!=INTERP_LINEAR)
            {
                const std::string i = InterpolationToString(getInterpolation());
                std::string err = "Unsupported interpolation algorithm: " + i;
                throw Exception(err.c_str());
            }

            OpData::validate();

            try
            {
                getArray().validate();
            }
            catch (Exception& e)
            {
                std::ostringstream oss;
                oss << "Lut1D content array issue: ";
                oss << e.what();

                throw Exception(oss.str().c_str());
            }

            // If isHalfDomain is set, we need to make sure we have 65536 entries
            // array.
            if (isInputHalfDomain() && getArray().getLength()
                    != HALF_DOMAIN_REQUIRED_ENTRIES)
            {
                std::ostringstream oss;
                oss << "Lut1D: ";
                oss << getArray().getLength();
                oss << " entries found, ";
                oss << HALF_DOMAIN_REQUIRED_ENTRIES;
                oss << " required for halfDomain LUT1D.";

                throw Exception(oss.str().c_str());
            }
        }


        unsigned Lut1D::GetLutIdealSize(BitDepth inputBitDepth,
                                        HalfFlags halfFlags)
        {
            // Returns the number of entries that _fill() expects in order to make
            // an identity LUT.

            // For half domain always return 65536, since that is what _fill() expects.
            // However note that if the inputBitDepth is, e.g. 10i, this might not be
            // the number of entries required for a look-up.

            // For 32f, a look-up is impractical so in that case return 64k.

            unsigned size = HALF_DOMAIN_REQUIRED_ENTRIES;

            if ((halfFlags & LUT_INPUT_HALF_CODE) == LUT_INPUT_HALF_CODE)
            {
                return size;
            }

            switch (inputBitDepth)
            {
            case BIT_DEPTH_UINT8:
            case BIT_DEPTH_UINT10:
            case BIT_DEPTH_UINT12:
            case BIT_DEPTH_UINT14:
            case BIT_DEPTH_UINT16:
                size = (unsigned)(GetBitDepthMaxValue(inputBitDepth) + 1);
                break;
            case BIT_DEPTH_UINT32:
            case BIT_DEPTH_F16:
            case BIT_DEPTH_F32:
            case BIT_DEPTH_UNKNOWN:
                break;
            }

            return size;
        }

        bool Lut1D::mayLookup(BitDepth incomingDepth) const
        {
            if(isInputHalfDomain())
            {
                return incomingDepth == BIT_DEPTH_F16;
            }
            else  // not a half-domain LUT
            {
                if(!IsFloatBitDepth(incomingDepth))
                {
                    return m_array.getLength()
                        == (GetBitDepthMaxValue(incomingDepth) + 1);
                }
            }
            return false;
        }

        OpDataLut1DRcPtr Lut1D::makeLookupDomain(BitDepth incomingDepth)
        {
            // For integer in-depths, we need a standard domain.
            Lut1D::HalfFlags domainType = Lut1D::LUT_STANDARD;

            // For 16f in-depth, we need a half domain.
            // (Return same for 32f, even though a pure lookup wouldn't be appropriate.)
            if(IsFloatBitDepth(incomingDepth))
            {
                domainType = Lut1D::LUT_INPUT_HALF_CODE;
            }

            // Note that in this case the domainType is always appropriate for
            // the incomingDepth, so it should be safe to rely on the constructor
            // and _fill() to always return the correct length.
            // (E.g., we don't need to worry about 10i with a half domain.)
            OpDataLut1DRcPtr
                newDomain(new Lut1D(incomingDepth,
                                    incomingDepth,
                                    "", "", Descriptions(), 
                                    INTERP_LINEAR,
                                    domainType));
            return newDomain;
        }

        bool Lut1D::isInverse(const Lut1D & B) const
        {
            // Note: The invLut1D constructor modifies the array to make it monotonic,
            // hence, this could return false in unexpected cases.  However, one could argue
            // that those LUTs should not be optimized out as an identity anyway.

            if (getOpType() == OpData::Lut1DType
                && B.getOpType() == OpData::InvLut1DType)
            {
                // Need to check bit-depth because the array scaling is relative to it.
                // (For LUT it is the out-depth, for INVLUT it is the in-depth.)
                // Note that we use MaxValue so that 16f and 32f are considered the same.

                // In the LUT-->INVLUT case this will typically be true anyway.
                if ( GetBitDepthMaxValue(getOutputBitDepth()) != 
                        GetBitDepthMaxValue(B.getInputBitDepth()) )
                    return false;

                // Test the core parts such as array, half domain, and hue adjust while
                // ignoring superficial differences such as in/out bit-depth.
                return haveEqualBasics(B);
            }
            else if (getOpType() == OpData::InvLut1DType 
                && B.getOpType() == OpData::Lut1DType)
            {
                // TODO: Harmonize array bit-depths to allow a proper array comparison.

                // In the INVLUT-->LUT case this could easily not be true even if the
                // pair are actually inverses.
                if ( GetBitDepthMaxValue(getInputBitDepth()) != 
                        GetBitDepthMaxValue(B.getOutputBitDepth()) )
                    return false;

                return haveEqualBasics(B);
            }
            return false;
        }

        bool Lut1D::haveEqualBasics(const Lut1D & B) const
        {
            // Question:  Should interpolation style be considered?
            return         m_array == B.m_array &&
                       m_halfFlags == B.m_halfFlags &&
                       m_hueAdjust == B.m_hueAdjust;
        }

        bool Lut1D::operator==(const OpData & other) const
        {
            if (this == &other) return true;
            if (getOpType() != other.getOpType()) return false;

            const Lut1D* lop = static_cast<const Lut1D*>(&other);

            if ( ! haveEqualBasics(*lop) ) return false;

            return (OpData::operator==(other) &&
                    m_interpolation == lop->m_interpolation);
        }

        const CTF::Version & Lut1D::getMinimumVersion() const
        {
            if (getHueAdjust() != Lut1D::HUE_NONE)
            {
                return CTF::CTF_PROCESS_LIST_VERSION_1_4;
            }

            // TODO: Could lower this further?
            return CTF::CTF_PROCESS_LIST_VERSION_1_3;
        }

        void Lut1D::adjustColorComponentNumber()
        {
            m_array.adjustColorComponentNumber();
        }

        void Lut1D::setHueAdjust(Lut1D::HueAdjust algo)
        {
            m_hueAdjust = algo;
        }

        OpData * Lut1D::clone(CloneType /*type*/) const
        {
            return new Lut1D(*this);
        }

        void Lut1D::inverse(OpDataVec & ops) const
        {
            std::auto_ptr<InvLut1D> invOp(new InvLut1D(*this));

            // TODO: No bit depth swap !!!

            ops.append(invOp.release());
        }

        OpDataLut1DRcPtr Lut1D::compose(const Lut1D& /*B*/) const
        {
            // TODO: To be implemented
			Lut1D * composed(0);
            return OpDataLut1DRcPtr(composed);
        }

    } // exit OpData namespace
}
OCIO_NAMESPACE_EXIT



#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "../UnitTest.h"

OIIO_ADD_TEST(OpDataLut1D, Constructor)
{
    OCIO::OpData::Lut1D lut(2);

    OIIO_CHECK_ASSERT(lut.getOpType() == OCIO::OpData::OpData::Lut1DType);
    OIIO_CHECK_ASSERT(false == lut.isNoOp());
    OIIO_CHECK_ASSERT(lut.isIdentity());
    OIIO_CHECK_EQUAL(lut.getOpTypeName(), "LUT 1D");
    OIIO_CHECK_EQUAL(lut.getArray().getLength(), 2);
    OIIO_CHECK_EQUAL(lut.getInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(lut.validate());
}

OIIO_ADD_TEST(OpDataLut1D, Accessors)
{
    OCIO::OpData::Lut1D l(17);
    l.setInputBitDepth(OCIO::BIT_DEPTH_UINT10);
    l.setOutputBitDepth(OCIO::BIT_DEPTH_UINT10);
    l.setInterpolation(OCIO::INTERP_LINEAR);

    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_ASSERT(false == l.isNoOp());
    OIIO_CHECK_ASSERT(l.isIdentity());
    OIIO_CHECK_NO_THROW(l.validate());

    OIIO_CHECK_EQUAL(l.getHueAdjust(), OCIO::OpData::Lut1D::HUE_NONE);
    l.setHueAdjust(OCIO::OpData::Lut1D::HUE_DW3);
    OIIO_CHECK_EQUAL(l.getHueAdjust(), OCIO::OpData::Lut1D::HUE_DW3);

    // Note: Hue and Sat adjust do not affect identity status.
    OIIO_CHECK_ASSERT(l.isIdentity());

    const float back1 = l.getArray()[1];
    l.getArray()[1] = 1.0f;
    OIIO_CHECK_ASSERT(!l.isNoOp());
    OIIO_CHECK_ASSERT(!l.isIdentity());
    OIIO_CHECK_NO_THROW(l.validate());

    l.setInterpolation(OCIO::INTERP_BEST);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_BEST);

    OIIO_CHECK_EQUAL(l.getArray().getLength(), (unsigned)17);
    OIIO_CHECK_EQUAL(l.getArray().getNumValues(), (unsigned)17 * 3);
    OIIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), (unsigned)3);

    l.getArray().resize(65, 3);

    OIIO_CHECK_EQUAL(l.getArray().getLength(), (unsigned)65);
    OIIO_CHECK_EQUAL(l.getArray().getNumValues(), (unsigned)65 * 3);
    OIIO_CHECK_EQUAL(l.getArray().getNumColorComponents(), (unsigned)3);
    OIIO_CHECK_NO_THROW(l.validate());
}

OIIO_ADD_TEST(OpDataLut1D, Identity)
{
    OCIO::OpData::Lut1D l1(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT8, 
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_LINEAR, 
                           OCIO::OpData::Lut1D::LUT_STANDARD);
    OCIO::OpData::Lut1D l2(OCIO::BIT_DEPTH_UINT8, OCIO::BIT_DEPTH_UINT10, 
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_LINEAR, 
                           OCIO::OpData::Lut1D::LUT_STANDARD);

    OIIO_CHECK_ASSERT(false == l1.isNoOp());
    OIIO_CHECK_ASSERT(l1.isIdentity());
    OIIO_CHECK_NO_THROW(l1.validate());

    OIIO_CHECK_ASSERT(!l2.isNoOp());
    OIIO_CHECK_ASSERT(l2.isIdentity());
    OIIO_CHECK_NO_THROW(l2.validate());

    const float coeff 
        = OCIO::GetBitDepthMaxValue(l2.getOutputBitDepth())
             / OCIO::GetBitDepthMaxValue(l2.getInputBitDepth());

    // To validate the result, compute the expected results
    //  not using the Lut1DOp algorithm
    const float error = 1e-6f;
    for(unsigned idx=0; idx<l2.getArray().getLength(); ++idx)
    {
        OIIO_CHECK_CLOSE(l1.getArray().getValues()[idx*3+0]*coeff,
                         l2.getArray().getValues()[idx*3+0],
                         error);

        OIIO_CHECK_CLOSE(l1.getArray().getValues()[idx*3+1]*coeff,
                         l2.getArray().getValues()[idx*3+1],
                         error);
        
        OIIO_CHECK_CLOSE(l1.getArray().getValues()[idx*3+2]*coeff,
                         l2.getArray().getValues()[idx*3+2],
                         error);
    }

    OCIO::OpData::Lut1D l3(OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT8, 
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_LINEAR, 
                           OCIO::OpData::Lut1D::LUT_INPUT_HALF_CODE);

    OIIO_CHECK_ASSERT(l3.isIdentity());
}

OIIO_ADD_TEST(OpDataLut1D, Clone)
{
    OCIO::OpData::Lut1D ref(20);
    ref.getArray()[1] = 0.5f;
    ref.setHueAdjust(OCIO::OpData::Lut1D::HUE_DW3);

    std::auto_ptr<OCIO::OpData::Lut1D> pClone(
        dynamic_cast<OCIO::OpData::Lut1D*>(
            ref.clone(OCIO::OpData::OpData::DO_SHALLOW_COPY)));

    OIIO_CHECK_ASSERT(!pClone->isNoOp());
    OIIO_CHECK_ASSERT(!pClone->isIdentity());
    OIIO_CHECK_NO_THROW(pClone->validate());
    OIIO_CHECK_EQUAL(pClone->getOpTypeName(), "LUT 1D");
    OIIO_CHECK_ASSERT(pClone->getArray()==ref.getArray());
    OIIO_CHECK_EQUAL(pClone->getHueAdjust(), OCIO::OpData::Lut1D::HUE_DW3);
}

OIIO_ADD_TEST(OpDataLut1D, OutputDepthScaling)
{
    OCIO::OpData::Lut1D ref(OCIO::BIT_DEPTH_UINT10,
                            OCIO::BIT_DEPTH_UINT10,
                            OCIO::OpData::Lut1D::LUT_STANDARD);

    //get the array values and keep them to compare later
    const OCIO::OpData::Array::Values initialArrValues = ref.getArray().getValues();
    const OCIO::BitDepth initialBitdepth = ref.getOutputBitDepth();

    const OCIO::BitDepth newBitdepth = OCIO::BIT_DEPTH_UINT16;

    const float factor = OCIO::GetBitDepthMaxValue(newBitdepth)
                            / OCIO::GetBitDepthMaxValue(initialBitdepth);

    ref.setOutputBitDepth(newBitdepth);
    // now we need to make sure that the bitdepth was changed from the overriden
    // method.
    OIIO_CHECK_EQUAL(ref.getOutputBitDepth(), newBitdepth);

    // now we need to check the scaling between the new array and initial array
    // matches the factor computed above.
    const OCIO::OpData::Array::Values& newArrValues = ref.getArray().getValues();

    // sanity check first
    OIIO_CHECK_EQUAL(initialArrValues.size(), newArrValues.size());

    float expectedValue = 0.0f;
    const float error = 1e-6f;
    for (unsigned i = 0; i < newArrValues.size(); i++)
    {
        expectedValue = initialArrValues[i] * factor;
        OIIO_CHECK_CLOSE(expectedValue, newArrValues[i], error);
    }

}

OIIO_ADD_TEST(OpDataLut1D, Equality_test)
{
    OCIO::OpData::Lut1D l1(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10, 
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_LINEAR, 
                           OCIO::OpData::Lut1D::LUT_STANDARD);

    OCIO::OpData::Lut1D l2(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10, 
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_CUBIC, 
                           OCIO::OpData::Lut1D::LUT_STANDARD);  

    OIIO_CHECK_ASSERT(!(l1 == l2));

    OCIO::OpData::Lut1D l3(OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_UINT10, 
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_LINEAR, 
                           OCIO::OpData::Lut1D::LUT_STANDARD);

    OIIO_CHECK_ASSERT(!(l1 == l3) && !(l3 == l2));

    OCIO::OpData::Lut1D l4(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10, 
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_LINEAR, 
                           OCIO::OpData::Lut1D::LUT_STANDARD);

    OIIO_CHECK_ASSERT(l1 == l4);

    l1.setHueAdjust(OCIO::OpData::Lut1D::HUE_DW3);

    OIIO_CHECK_ASSERT(!(l1 == l4));
}

OIIO_ADD_TEST(OpDataLut1D, Channel)
{
    OCIO::OpData::Lut1D L1(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10, 
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_LINEAR,
                           OCIO::OpData::Lut1D::LUT_STANDARD,
                           17);

    OCIO::OpData::Lut1D L2(OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10, 
                           "", "", OCIO::OpData::Descriptions(), 
                           OCIO::INTERP_LINEAR, 
                           OCIO::OpData::Lut1D::LUT_STANDARD,
                           20);

    // false: identity
    OIIO_CHECK_ASSERT(!L1.hasChannelCrosstalk());
    OIIO_CHECK_ASSERT(L1.mayCompose(L2));

    L1.setHueAdjust(OCIO::OpData::Lut1D::HUE_DW3);
    // false: hue restore is on but it's an identity LUT
    OIIO_CHECK_ASSERT(!L1.hasChannelCrosstalk());
    OIIO_CHECK_ASSERT(!L1.mayCompose(L2));
    OIIO_CHECK_ASSERT(!L2.mayCompose(L1));

    L1.setHueAdjust(OCIO::OpData::Lut1D::HUE_NONE);
    L1.getArray()[1] = 3.0f;
    // false: non-identity
    OIIO_CHECK_ASSERT(!L1.hasChannelCrosstalk());

    L1.setHueAdjust(OCIO::OpData::Lut1D::HUE_DW3);
    // true: non-identity w/hue restore
    OIIO_CHECK_ASSERT(L1.hasChannelCrosstalk());
}

OIIO_ADD_TEST(OpDataLut1D, Interpolation)
{
    OCIO::OpData::Lut1D l(17);
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

    l.setInterpolation(OCIO::INTERP_DEFAULT);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_DEFAULT);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    // TODO: INTERP_CUBIC is currently implemented as INTERP_LINEAR
    l.setInterpolation(OCIO::INTERP_CUBIC);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_CUBIC);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    // TODO: INTERP_NEAREST is currently implemented as INTERP_LINEAR
    l.setInterpolation(OCIO::INTERP_NEAREST);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_NEAREST);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_LINEAR);
    OIIO_CHECK_NO_THROW(l.validate());

    // Invalid interpolation type duo not get translated
    // by getConcreteInterpolation
    l.setInterpolation(OCIO::INTERP_UNKNOWN);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_UNKNOWN);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_UNKNOWN);
    OIIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "Unsupported interpolation algorithm");

    l.setInterpolation(OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_EQUAL(l.getInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_EQUAL(l.getConcreteInterpolation(), OCIO::INTERP_TETRAHEDRAL);
    OIIO_CHECK_THROW_WHAT(l.validate(), OCIO::Exception, "Unsupported interpolation algorithm");
}

#endif

