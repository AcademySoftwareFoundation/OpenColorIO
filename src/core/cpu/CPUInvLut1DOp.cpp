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


#include "CPUInvLut1DOp.h"

#include "CPUGamutMapUtils.h"
#include "CPULutUtils.h"
#include "CPULut1DOp.h"

#include "../BitDepthUtils.h"

#include "SSE2.h"
#include "half.h"

#include <algorithm>


OCIO_NAMESPACE_ENTER
{
    namespace
    {
        // TODO: Use SSE intrinsics to speed this up.

        // Calculate the inverse of a value resulting from linear interpolation
        //        in a 1d LUT.
        // - start pointer to the first effective LUT entry (end of flat spot)
        // - startOffset distance between first LUT entry and start
        // - end pointer to the last effective LUT entry (start of flat spot)
        // - flipSign flips val if we're working with the negative of the orig LUT
        // - scale from LUT index units to outDepth units
        // - val the value to invert
        // Return the result that would produce val if used 
        // in a forward linear interpolation in the LUT
        float FindLutInv(const float* start,
                         const float  startOffset,
                         const float* end,
                         const float  flipSign,
                         const float  scale,
                         const float  val)
        {
            // Note that the LUT data pointed to by start/end must be in increasing order,
            // regardless of whether the original LUT was increasing or decreasing because
            // this function uses std::lower_bound().

            // Clamp the value to the range of the LUT.
            const float cv = std::min( std::max( val * flipSign, *start ), *end );

            // std::lower_bound()
            // "Returns an iterator pointing to the first element in the range [first,last) 
            // which does not compare less than val (but could be equal)."
            // (NB: This is correct using either end or end+1 since lower_bound will return a
            //  value one greater than the second argument if no values in the array are >= cv.)
            // http://www.sgi.com/tech/stl/lower_bound.html
            const float* lowbound = std::lower_bound(start, end, cv);

            // lower_bound() returns first entry >= val so decrement it unless val == *start.
            if (lowbound > start) {
                --lowbound;
            }

            const float* highbound = lowbound;
            if (highbound < end) {
                ++highbound;
            }

            // Delta is the fractional distance of val between the adjacent LUT entries.
            float delta = 0.f;
            if (*highbound > *lowbound) {   // (handle flat spots by leaving delta = 0)
                delta = (cv - *lowbound) / (*highbound - *lowbound);
            }

            // Inds is the index difference from the effective start to lowbound.
            const float inds = (float)( lowbound - start );

            // Correct for the fact that start is not the beginning of the LUT if it
            // starts with a flat spot.
            // (NB: It may seem like lower_bound would automatically find the end of the
            //  flat spot, so start could always simply be the start of the LUT, however
            //  this fails when val equals the flat spot value.)
            const float totalInds = inds + startOffset;

            // Scale converts from units of [0,dim] to [0,outDepth].
            return (totalInds + delta) * scale;
        }

        // Calculate the inverse of a value resulting from linear interpolation
        // in a half domain 1d LUT.
        // - start pointer to the first effective LUT entry (end of flat spot)
        // - startOffset distance between first LUT entry and start
        // - end pointer to the last effective LUT entry (start of flat spot)
        // - flipSign flips val if we're working with the negative of the orig LUT
        // - scale from LUT index units to outDepth units
        // - val the value to invert
        // Return the result that would produce val if used in a forward linear
        //        interpolation in the LUT
        float FindLutInvHalf(const float* start,
                             const float  startOffset,
                             const float* end,
                             const float  flipSign,
                             const float  scale,
                             const float  val)
        {
            // Note that the LUT data pointed to by start/end must be in increasing order,
            // regardless of whether the original LUT was increasing or decreasing because
            // this function uses std::lower_bound().

            // Clamp the value to the range of the LUT.
            const float cv = std::min( std::max( val * flipSign, *start ), *end );

            const float* lowbound = std::lower_bound(start, end, cv);

            // lower_bound() returns first entry >= val so decrement it unless val == *start.
            if (lowbound > start) {
                --lowbound;
            }

            const float* highbound = lowbound;
            if (highbound < end) {
                ++highbound;
            }

            // Delta is the fractional distance of val between the adjacent LUT entries.
            float delta = 0.f;
            if (*highbound > *lowbound) {   // (handle flat spots by leaving delta = 0)
                delta = (cv - *lowbound) / (*highbound - *lowbound);
            }

            // Inds is the index difference from the effective start to lowbound.
            const float inds = (float)( lowbound - start );

            // Correct for the fact that start is not the beginning of the LUT if it
            // starts with a flat spot.
            // (NB: It may seem like lower_bound would automatically find the end of the
            //  flat spot, so start could always simply be the start of the LUT, however
            //  this fails when val equals the flat spot value.)
            const float totalInds = inds + startOffset;

            // For a half domain LUT, the entries are not a constant distance apart,
            // so convert the indices (which are half floats) into real floats in order
            // to calculate what distance the delta factor is working over.
            half h;
            h.setBits((unsigned short)totalInds);
            float base = h;
            h.setBits((unsigned short)(totalInds + 1));
            float basePlus1 = h;
            float domain = base + delta * (basePlus1 - base);

            // Scale converts from units of [0,dim] to [0,outDepth].
            return domain * scale;
        }
    }

    InvLut1DRenderer::InvLut1DRenderer(const OpData::OpDataLut1DRcPtr & lut) 
        :   CPUOp()
        ,   m_dim(0)
        ,   m_tmpLutR(0x0)
        ,   m_tmpLutG(0x0)
        ,   m_tmpLutB(0x0)
        ,   m_alphaScaling(0.0f)
    {
        if(lut->getOpType() != OpData::OpData::InvLut1DType)
        {
            throw Exception("Cannot apply InvLut1DOp op, "
                            "Not an inverse LUT 1D data");
        }

        updateData(lut);
    }

    InvLut1DRenderer::~InvLut1DRenderer()
    {
        resetData();
    }

    void InvLut1DRenderer::setComponentParams(
        InvLut1DRenderer::ComponentParams & params,
        const OpData::InvLut1D::ComponentProperties & properties,
        const float * lutPtr,
        const float lutZeroEntry)
    {
        params.flipSign = properties.isIncreasing ? 1.f: -1.f;
        params.bisectPoint = lutZeroEntry;
        params.startOffset = (float) properties.startDomain;
        params.lutStart = lutPtr + properties.startDomain;
        params.lutEnd   = lutPtr + properties.endDomain;
        params.negStartOffset = (float) properties.negStartDomain;
        params.negLutStart = lutPtr + properties.negStartDomain;
        params.negLutEnd   = lutPtr + properties.negEndDomain;
    }

    void InvLut1DRenderer::resetData()
    {
        delete [] m_tmpLutR; m_tmpLutR = 0x0;
        delete [] m_tmpLutG; m_tmpLutG = 0x0;
        delete [] m_tmpLutB; m_tmpLutB = 0x0;
    }

    void InvLut1DRenderer::updateData(const OpData::OpDataLut1DRcPtr & lut)
    {
        resetData();

        const bool hasSingleLut = lut->hasSingleLut();

        m_dim = lut->getArray().getLength();

        // Allocated temporary LUT(s)

        m_tmpLutR = new float[m_dim];
        m_tmpLutG = 0x0;
        m_tmpLutB = 0x0;

        if( !hasSingleLut )
        {
            m_tmpLutG = new float[m_dim];
            m_tmpLutB = new float[m_dim];
        }

        // Get component properties and initialize component parameters structure.

        const OpData::InvLut1D::ComponentProperties & 
            redProperties = dynamic_cast<OpData::InvLut1D*>(lut.get())->getRedProperties();

        const OpData::InvLut1D::ComponentProperties & 
            greenProperties = dynamic_cast<OpData::InvLut1D*>(lut.get())->getGreenProperties();

        const OpData::InvLut1D::ComponentProperties & 
            blueProperties = dynamic_cast<OpData::InvLut1D*>(lut.get())->getBlueProperties();

        setComponentParams(m_paramsR, redProperties, (float*)m_tmpLutR, 0.f);

        if( hasSingleLut )
        {
            // NB: All pointers refer to _tmpLutR.
            m_paramsB = m_paramsG = m_paramsR;
        }
        else
        {
            setComponentParams(m_paramsG, greenProperties, (float*)m_tmpLutG, 0.f);
            setComponentParams(m_paramsB, blueProperties, (float*)m_tmpLutB, 0.f);
        }

        // Fill temporary LUT.
        //   Note: Since FindLutInv requires increasing arrays, if the LUT is decreasing
        //   we negate the values to obtain the required sort order of smallest to largest.

        const OpData::Array::Values& lutValues = lut->getArray().getValues();
        for(unsigned i = 0; i<m_dim; ++i)
        {
            m_tmpLutR[i] = redProperties.isIncreasing ? lutValues[i*3+0]: -lutValues[i*3+0];

            if(!hasSingleLut)
            {
                m_tmpLutG[i] = greenProperties.isIncreasing ? lutValues[i*3+1]: -lutValues[i*3+1];
                m_tmpLutB[i] = blueProperties.isIncreasing ? lutValues[i*3+2]: -lutValues[i*3+2];
            }
        }

        const float outMax = GetBitDepthMaxValue(lut->getOutputBitDepth());

        m_alphaScaling = outMax / GetBitDepthMaxValue(lut->getInputBitDepth());

        // Converts from index units to inDepth units of the original LUT.
        // (Note that inDepth of the original LUT is outDepth of the inverse LUT.)
        m_scale = outMax / (float) (m_dim - 1);
    }

    void InvLut1DRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            // red
            rgba[0] = FindLutInv(m_paramsR.lutStart,
                                 m_paramsR.startOffset,
                                 m_paramsR.lutEnd,
                                 m_paramsR.flipSign,
                                 m_scale,
                                 rgba[0]);

            // green
            rgba[1] = FindLutInv(m_paramsG.lutStart,
                                 m_paramsG.startOffset,
                                 m_paramsG.lutEnd,
                                 m_paramsG.flipSign,
                                 m_scale,
                                 rgba[1]);

            // blue
            rgba[2] = FindLutInv(m_paramsB.lutStart,
                                 m_paramsB.startOffset,
                                 m_paramsB.lutEnd,
                                 m_paramsB.flipSign,
                                 m_scale,
                                 rgba[2]);

            // alpha
            rgba[3] = rgba[3] * m_alphaScaling;

            rgba += 4;
        }
    }

    InvLut1DRendererHueAdjust::InvLut1DRendererHueAdjust(const OpData::OpDataLut1DRcPtr & lut) 
        :  InvLut1DRenderer(lut)
    {
        updateData(lut);
    }

    InvLut1DRendererHueAdjust::~InvLut1DRendererHueAdjust()
    {
    }

    void InvLut1DRendererHueAdjust::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            const float RGB[] = {rgba[0], rgba[1], rgba[2]};

            int min, mid, max;
            GamutMapUtils::Order3(RGB, min, mid, max);

            const float orig_chroma = RGB[max] - RGB[min];
            const float hue_factor
                = (orig_chroma == 0.f) ? 0.f
                                       : (RGB[mid] - RGB[min]) / orig_chroma;

            float RGB2[] = {
                // red
                FindLutInv(m_paramsR.lutStart,
                           m_paramsR.startOffset,
                           m_paramsR.lutEnd,
                           m_paramsR.flipSign,
                           m_scale,
                           RGB[0]),
                // green
                FindLutInv(m_paramsG.lutStart,
                           m_paramsG.startOffset,
                           m_paramsG.lutEnd,
                           m_paramsG.flipSign,
                           m_scale,
                           RGB[1]),
                // blue
                FindLutInv(m_paramsB.lutStart,
                           m_paramsB.startOffset,
                           m_paramsB.lutEnd,
                           m_paramsB.flipSign,
                           m_scale,
                           RGB[2])
            };

            const float new_chroma = RGB2[max] - RGB2[min];

            RGB2[mid] = hue_factor * new_chroma + RGB2[min];

            rgba[0] = RGB2[0];
            rgba[1] = RGB2[1];
            rgba[2] = RGB2[2];
            rgba[3] = rgba[3] * m_alphaScaling;

            rgba += 4;
        }
    }

    InvLut1DRendererHalfCode::InvLut1DRendererHalfCode(const OpData::OpDataLut1DRcPtr & lut) 
        :  InvLut1DRenderer(lut)
    {
        updateData(lut);
    }

    InvLut1DRendererHalfCode::~InvLut1DRendererHalfCode()
    {
        resetData();
    }

    void InvLut1DRendererHalfCode::updateData(const OpData::OpDataLut1DRcPtr & lut)
    {
        resetData();

        const bool hasSingleLut = lut->hasSingleLut();

        m_dim = lut->getArray().getLength();

        // Allocated temporary LUT(s)

        m_tmpLutR = new float[m_dim];
        m_tmpLutG = 0x0;
        m_tmpLutB = 0x0;

        if( !hasSingleLut )
        {
            m_tmpLutG = new float[m_dim];
            m_tmpLutB = new float[m_dim];
        }

        // Get component properties and initialize component parameters structure.

        const OpData::InvLut1D::ComponentProperties & 
            redProperties = dynamic_cast<OpData::InvLut1D*>(lut.get())->getRedProperties();

        const OpData::InvLut1D::ComponentProperties & 
            greenProperties = dynamic_cast<OpData::InvLut1D*>(lut.get())->getGreenProperties();

        const OpData::InvLut1D::ComponentProperties & 
            blueProperties = dynamic_cast<OpData::InvLut1D*>(lut.get())->getBlueProperties();

        const OpData::Array::Values & lutValues = lut->getArray().getValues();

        setComponentParams(m_paramsR, redProperties, m_tmpLutR, lutValues[0]);

        if( hasSingleLut )
        {
            // NB: All pointers refer to m_tmpLutR.
            m_paramsB = m_paramsG = m_paramsR;
        }
        else
        {
            setComponentParams(m_paramsG, greenProperties, m_tmpLutG, lutValues[1]);
            setComponentParams(m_paramsB, blueProperties, m_tmpLutB, lutValues[2]);
        }

        // Fill temporary LUT.
        //   Note: Since FindLutInv requires increasing arrays, if the LUT is decreasing
        //   we negate the values to obtain the required sort order of smallest to largest.
        for(unsigned i = 0; i < 32768; ++i)     // positive half domain
        {
            m_tmpLutR[i] = redProperties.isIncreasing ? lutValues[i*3+0]: -lutValues[i*3+0];

            if( !hasSingleLut )
            {
                m_tmpLutG[i] = greenProperties.isIncreasing ? lutValues[i*3+1]: -lutValues[i*3+1];
                m_tmpLutB[i] = blueProperties.isIncreasing ? lutValues[i*3+2]: -lutValues[i*3+2];
            }
        }

        for(unsigned i = 32768; i < 65536; ++i) // negative half domain
        {
            // (Per above, the LUT must be increasing, so negative half domain is sign reversed.)
            m_tmpLutR[i] = redProperties.isIncreasing ? -lutValues[i*3+0]: lutValues[i*3+0];

            if( !hasSingleLut )
            {
                m_tmpLutG[i] = greenProperties.isIncreasing ? -lutValues[i*3+1]: lutValues[i*3+1];
                m_tmpLutB[i] = blueProperties.isIncreasing ? -lutValues[i*3+2]: lutValues[i*3+2];
            }
        }

        const float outMax = GetBitDepthMaxValue(lut->getOutputBitDepth());

        m_alphaScaling = outMax / GetBitDepthMaxValue(lut->getInputBitDepth());

        // Note the difference for half domain LUTs, since the distance between
        // between adjacent entries is not constant, we cannot roll it into the scale.
        m_scale = outMax;
    }

    void InvLut1DRendererHalfCode::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        const bool redIsIncreasing = m_paramsR.flipSign > 0.f;
        const bool grnIsIncreasing = m_paramsG.flipSign > 0.f;
        const bool bluIsIncreasing = m_paramsB.flipSign > 0.f;

        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            // Test the values against the bisectPoint to determine which half of the
            // float domain to do the inverse eval in.

            // Note that since the clamp of values outside the effective domain happens
            // in FindLutInvHalf, that input values < the bisectPoint but > the neg effective
            // domain will get clamped to -0 or wherever the neg effective domain starts.
            // If this proves to be a problem, could move the clamp here instead.

            const float redIn = rgba[0];
            const float redOut 
                = (redIsIncreasing == (redIn >= m_paramsR.bisectPoint)) 
                    ? FindLutInvHalf(m_paramsR.lutStart,
                                     m_paramsR.startOffset,
                                     m_paramsR.lutEnd,
                                     m_paramsR.flipSign,
                                     m_scale,
                                     redIn) 
                    : FindLutInvHalf(m_paramsR.negLutStart,
                                     m_paramsR.negStartOffset,
                                     m_paramsR.negLutEnd,
                                    -m_paramsR.flipSign,
                                     m_scale,
                                     redIn);

            const float grnIn = rgba[1];
            const float grnOut 
                = (grnIsIncreasing == (grnIn >= m_paramsG.bisectPoint)) 
                    ? FindLutInvHalf(m_paramsG.lutStart,
                                     m_paramsG.startOffset,
                                     m_paramsG.lutEnd,
                                     m_paramsG.flipSign,
                                     m_scale,
                                     grnIn) 
                    : FindLutInvHalf(m_paramsG.negLutStart,
                                     m_paramsG.negStartOffset,
                                     m_paramsG.negLutEnd,
                                    -m_paramsG.flipSign,
                                     m_scale,
                                     grnIn);

            const float bluIn = rgba[2];
            const float bluOut 
                = (bluIsIncreasing == (bluIn >= m_paramsB.bisectPoint)) 
                    ? FindLutInvHalf(m_paramsB.lutStart,
                                     m_paramsB.startOffset,
                                     m_paramsB.lutEnd,
                                     m_paramsB.flipSign,
                                     m_scale,
                                     bluIn)
                    : FindLutInvHalf(m_paramsB.negLutStart,
                                     m_paramsB.negStartOffset,
                                     m_paramsB.negLutEnd,
                                    -m_paramsR.flipSign,
                                     m_scale,
                                     bluIn);

            rgba[0] = redOut;
            rgba[1] = grnOut;
            rgba[2] = bluOut;
            rgba[3] = rgba[3] * m_alphaScaling;

            rgba += 4;
        }
    }

    InvLut1DRendererHalfCodeHueAdjust::InvLut1DRendererHalfCodeHueAdjust(const OpData::OpDataLut1DRcPtr & lut) 
        :  InvLut1DRendererHalfCode(lut)
    {
        updateData(lut);
    }

    InvLut1DRendererHalfCodeHueAdjust::~InvLut1DRendererHalfCodeHueAdjust()
    {
    }

    void InvLut1DRendererHalfCodeHueAdjust::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        const bool redIsIncreasing = m_paramsR.flipSign > 0.f;
        const bool grnIsIncreasing = m_paramsG.flipSign > 0.f;
        const bool bluIsIncreasing = m_paramsB.flipSign > 0.f;

        float * rgba = rgbaBuffer;

        for(unsigned idx=0; idx<numPixels; ++idx)
        {
            const float RGB[] = {rgba[0], rgba[1], rgba[2]};

            int min, mid, max;
            GamutMapUtils::Order3( RGB, min, mid, max);

            const float orig_chroma = RGB[max] - RGB[min];
            const float hue_factor 
                = orig_chroma == 0.f ? 0.f
                                     : (RGB[mid] - RGB[min]) / orig_chroma;

            const float redOut 
                = (redIsIncreasing == (RGB[0] >= m_paramsR.bisectPoint)) 
                    ? FindLutInvHalf(m_paramsR.lutStart,
                                     m_paramsR.startOffset,
                                     m_paramsR.lutEnd,
                                     m_paramsR.flipSign,
                                     m_scale,
                                     RGB[0])
                    : FindLutInvHalf(m_paramsR.negLutStart,
                                     m_paramsR.negStartOffset,
                                     m_paramsR.negLutEnd,
                                    -m_paramsR.flipSign,
                                     m_scale,
                                     RGB[0]);

            const float grnOut 
                = (grnIsIncreasing == (RGB[1] >= m_paramsG.bisectPoint)) 
                    ? FindLutInvHalf(m_paramsG.lutStart,
                                     m_paramsG.startOffset,
                                     m_paramsG.lutEnd,
                                     m_paramsG.flipSign,
                                     m_scale,
                                     RGB[1]) 
                    : FindLutInvHalf(m_paramsG.negLutStart,
                                     m_paramsG.negStartOffset,
                                     m_paramsG.negLutEnd,
                                    -m_paramsG.flipSign,
                                     m_scale,
                                     RGB[1]);

            const float bluOut 
                = (bluIsIncreasing == (RGB[2] >= m_paramsB.bisectPoint)) 
                    ? FindLutInvHalf(m_paramsB.lutStart,
                                     m_paramsB.startOffset,
                                     m_paramsB.lutEnd,
                                     m_paramsB.flipSign,
                                     m_scale,
                                     RGB[2]) 
                    : FindLutInvHalf(m_paramsB.negLutStart,
                                     m_paramsB.negStartOffset,
                                     m_paramsB.negLutEnd,
                                    -m_paramsR.flipSign,
                                     m_scale,
                                     RGB[2]);

            float RGB2[] = { redOut, grnOut, bluOut };

            const float new_chroma = RGB2[max] - RGB2[min];

            RGB2[mid] = hue_factor * new_chroma + RGB2[min];

            rgba[0] = RGB2[0];
            rgba[1] = RGB2[1];
            rgba[2] = RGB2[2];
            rgba[3] = rgba[3] * m_alphaScaling;

            rgba += 4;
        }
    }

    CPUOpRcPtr InvLut1DRenderer::GetRenderer(const OpData::OpDataLut1DRcPtr & lut)
    {
        CPUOpRcPtr op(new CPUNoOp);

        // TODO: Should also take the FAST path if input bit depth is integer.

        if(dynamic_cast<OpData::InvLut1D*>(lut.get())->getInvStyle() == OpData::InvLut1D::FAST)
        {
            //
            // OK to have a scoped ptr here because getRenderer will end up copying
            // any data from newLut that is needed for the renderer.  
            //
            OpData::OpDataLut1DRcPtr 
                newLut = InvLutUtil::makeFastLut1D(lut, false);

            // Render with a Lut1D renderer.
            op = Lut1DRenderer::GetRenderer(newLut);
        }
        else  // EXACT
        {
            if (lut->isInputHalfDomain())
            {
                if (lut->getHueAdjust() != OpData::Lut1D::HUE_NONE)
                {
                    op.reset(new InvLut1DRendererHalfCodeHueAdjust(lut));
                }
                else
                {
                    op.reset(new InvLut1DRendererHalfCode(lut));
                }
            }
            else
            {
                if (lut->getHueAdjust() != OpData::Lut1D::HUE_NONE)
                {
                    op.reset(new InvLut1DRendererHueAdjust(lut));
                }
                else
                {
                    op.reset(new InvLut1DRenderer(lut));
                }
            }
        }

        return op;
    }
}
OCIO_NAMESPACE_EXIT
