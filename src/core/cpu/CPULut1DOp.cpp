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


#include "CPULut1DOp.h"
#include "CPUGamutMapUtils.h"

#include "../opdata/OpDataTools.h"

#include "../BitDepthUtils.h"
#include "../MathUtils.h"

#include "SSE2.h"

#include <algorithm>
#include <math.h>
#include <stdint.h>


#define L_ADJUST(val) \
    ((isOutInteger) ? clamp((val)+0.5f, outMin,  outMax) : SanitizeFloat(val))




OCIO_NAMESPACE_ENTER
{
    namespace
    {
        inline uint8_t getLookupValue(const uint8_t& val)
        {
            return val;
        }

        inline uint16_t getLookupValue(const uint16_t& val)
        {
            return val;
        }

        inline unsigned short getLookupValue(const half& val)
        {
            return val.bits();
        }

        // When instantiating all templates this case is needed.
        // But it will never be used as the 32f is not a lookup case.
        inline uint16_t getLookupValue(const float& val)
        {
            return (uint16_t)val;
        }

        template<typename InType, typename OutType>
        struct LookupLut
        {
            static inline OutType compute(const OutType* lutData,
                                          const InType& val)
            {
                return lutData[getLookupValue(val)];
            }
        };
    }

    BaseLut1DRenderer::BaseLut1DRenderer(const OpData::OpDataLut1DRcPtr & lut)
        :   CPUOp()
        ,   m_dim(0)
        ,   m_tmpLutR(0x0)
        ,   m_tmpLutG(0x0)
        ,   m_tmpLutB(0x0)
        ,   m_alphaScaling(0.0f)
        ,   m_outBitDepth(BIT_DEPTH_UNKNOWN)
    {
        m_dim = lut->getArray().getLength();
        m_outBitDepth = lut->getOutputBitDepth();
    }

    void BaseLut1DRenderer::resetData()
    {
        delete [] m_tmpLutR;
        delete [] m_tmpLutG;
        delete [] m_tmpLutB;
    }

    BaseLut1DRenderer::~BaseLut1DRenderer()
    {
        resetData();
    }

    Lut1DRendererHalfCode::Lut1DRendererHalfCode(const OpData::OpDataLut1DRcPtr & lut)
        :  BaseLut1DRenderer(lut)
    {
        updateData(lut);
    }

    void Lut1DRendererHalfCode::updateData(const OpData::OpDataLut1DRcPtr & lut)
    {
        resetData();

        m_dim = lut->getArray().getLength();

        const BitDepth inBD = lut->getInputBitDepth();
        const BitDepth outBD = lut->getOutputBitDepth();

        const float outMax = GetBitDepthMaxValue(outBD);
        const float outMin = 0.0f;

        const bool isLookup = inBD != BIT_DEPTH_F32 && inBD != BIT_DEPTH_UINT32;

        const bool isOutInteger = !IsFloatBitDepth(outBD);

        const bool mustResample = !lut->mayLookup(inBD);

        // If we are able to lookup, need to resample the LUT based on inBitDepth.
        if (isLookup && mustResample)
        {
            OpData::OpDataLut1DRcPtr newDomain
                = OpData::Lut1D::makeLookupDomain(inBD);

            // Note: Compose should render at 32f, to avoid infinite recursion.
            OpData::OpDataLut1DRcPtr newLut
                = OpData::Compose(newDomain,
                                  lut,
                                  // Prevent compose from modifying newDomain
                                  OpData::COMPOSE_RESAMPLE_NO);

            m_dim = newLut->getArray().getLength();

            m_tmpLutR = new float[m_dim];
            m_tmpLutG = new float[m_dim];
            m_tmpLutB = new float[m_dim];

            const OpData::Array::Values & 
                lutValues = newLut->getArray().getValues();

            // TODO: Would be faster if R, G, B were adjacent in memory?
            for(unsigned i=0; i<m_dim; ++i)
            {
                m_tmpLutR[i] = L_ADJUST(lutValues[i*3+0]);
                m_tmpLutG[i] = L_ADJUST(lutValues[i*3+1]);
                m_tmpLutB[i] = L_ADJUST(lutValues[i*3+2]);
            }
        }
        else
        {
            const OpData::Array::Values & 
                lutValues = lut->getArray().getValues();

            m_tmpLutR = new float[m_dim];
            m_tmpLutG = new float[m_dim];
            m_tmpLutB = new float[m_dim];

            for(unsigned i=0; i<m_dim; ++i)
            {
                m_tmpLutR[i] = L_ADJUST(lutValues[i*3+0]);
                m_tmpLutG[i] = L_ADJUST(lutValues[i*3+1]);
                m_tmpLutB[i] = L_ADJUST(lutValues[i*3+2]);
            }
        }

        m_alphaScaling = GetBitDepthMaxValue(outBD)
                            / GetBitDepthMaxValue(inBD);
    }

    Lut1DRendererHalfCode::~Lut1DRendererHalfCode()
    {
    }

    void Lut1DRendererHalfCode::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        // TODO: Add ability for input or output to be an integer rather than always float.

        // TODO: To uncomment when apply bit depth is in
        const bool isLookup = false; // GET_BIT_DEPTH( PF_IN ) != OCIO::BIT_DEPTH_F32;

        const float * lutR = (const float *)m_tmpLutR;
        const float * lutG = (const float *)m_tmpLutG;
        const float * lutB = (const float *)m_tmpLutB;

        if (isLookup)
        {
            float * rgba = rgbaBuffer;

            for(unsigned idx=0; idx<numPixels; ++idx)
            {
                rgba[0] = LookupLut<float, float>::compute(lutR, rgba[0]);
                rgba[1] = LookupLut<float, float>::compute(lutG, rgba[1]);
                rgba[2] = LookupLut<float, float>::compute(lutB, rgba[2]);
                rgba[3] = rgba[3] * m_alphaScaling;

                rgba += 4;
            }
        }
        else  // need to interpolate rather than simply lookup
        {
            float * rgba = rgbaBuffer;

            for(unsigned idx=0; idx<numPixels; ++idx)
            {
                // Red
                IndexPair redInterVals = getEdgeFloatValues(rgba[0]);

                // Green
                IndexPair greenInterVals = getEdgeFloatValues(rgba[1]);

                // Blue
                IndexPair blueInterVals = getEdgeFloatValues(rgba[2]);

                // Since fraction is in the domain [0, 1), interpolate using 1-fraction
                // in order to avoid cases like -/+Inf * 0.
                rgba[0] = lerpf(lutR[redInterVals.valB],
                                lutR[redInterVals.valA],
                                1.0f-redInterVals.fraction);

                rgba[1] = lerpf(lutG[greenInterVals.valB],
                                lutG[greenInterVals.valA],
                                1.0f-greenInterVals.fraction);

                rgba[2] = lerpf(lutB[blueInterVals.valB],
                                lutB[blueInterVals.valA],
                                1.0f-blueInterVals.fraction);

                rgba[3] = rgba[3] * m_alphaScaling;

                rgba += 4;
            }
        }
    }

    Lut1DRendererHalfCode::IndexPair Lut1DRendererHalfCode::getEdgeFloatValues(float fIn) const
    {
        // TODO: Could we speed this up (perhaps alternate nan/inf behavior)?

        half halfVal;
        IndexPair idxPair;

        halfVal = half( fIn );
        if(halfVal.isInfinity())
        {
            halfVal = halfVal.isNegative() ? -HALF_MAX : HALF_MAX;
            fIn = halfVal;
        }

        // Convert back to float to compare to fIn
        // and interpolate both values.
        const float floatTemp = (float) halfVal;

        if (fabs(floatTemp)> fabs(fIn)) // Strict comparison required here,
                                        // otherwise negative fractions will occur.
        {
            idxPair.valB = halfVal.bits();
            idxPair.valA = idxPair.valB;
            --(idxPair.valA);
        }
        else
        {
            idxPair.valA = halfVal.bits();
            idxPair.valB = idxPair.valA;
            ++(idxPair.valB);

            halfVal.setBits(idxPair.valB);
            if(halfVal.isInfinity())
            {
                halfVal =  halfVal.isNegative () ? -HALF_MAX : HALF_MAX;
                idxPair.valB = halfVal.bits();
            }
        }

        halfVal.setBits(idxPair.valA);
        const float fA = (float) halfVal;

        halfVal.setBits(idxPair.valB);
        const float fB = (float) halfVal;

        idxPair.fraction = (fIn - fA) / (fB - fA);

        if (isnan(idxPair.fraction)) idxPair.fraction = 0.0f;

        return idxPair;
    }

    Lut1DRenderer::Lut1DRenderer(const OpData::OpDataLut1DRcPtr & lut)
        :  BaseLut1DRenderer(lut)
        ,   m_step(0.0f)
        ,   m_dimMinusOne(0.0f)
    {
        updateData(lut);
    }

    void Lut1DRenderer::updateData(const OpData::OpDataLut1DRcPtr & lut)
    {
        resetData();

        m_dim = lut->getArray().getLength();

        const BitDepth inBD = lut->getInputBitDepth();
        const BitDepth outBD= lut->getOutputBitDepth();

        const float outMin = 0.0f;
        const float outMax = GetBitDepthMaxValue(outBD);

        const bool isLookup = inBD != BIT_DEPTH_F32;

        const bool isOutInteger = !IsFloatBitDepth(outBD);

        const bool mustResample = !lut->mayLookup(inBD);

        // If we are able to lookup, need to resample the LUT based on inBitDepth.
        if (isLookup && mustResample)
        {
            OpData::OpDataLut1DRcPtr newDomain
               = OpData::Lut1D::makeLookupDomain(lut->getInputBitDepth());

            // Note: Compose should render at 32f, to avoid infinite recursion.
            OpData::OpDataLut1DRcPtr newLut
                = OpData::Compose(newDomain,
                                  lut,
                                  // Prevent compose from modifying newDomain
                                  OpData::COMPOSE_RESAMPLE_NO);

            m_dim = newLut->getArray().getLength();

            const OpData::Array::Values& lutValues = newLut->getArray().getValues();

            m_tmpLutR = new float[m_dim];
            m_tmpLutG = new float[m_dim];
            m_tmpLutB = new float[m_dim];

            // TODO: Would be faster if R, G, B were adjacent in memory?
            for(unsigned i=0; i<m_dim; ++i)
            {
                m_tmpLutR[i] = L_ADJUST(lutValues[i*3+0]);
                m_tmpLutG[i] = L_ADJUST(lutValues[i*3+1]);
                m_tmpLutB[i] = L_ADJUST(lutValues[i*3+2]);
            }
        }
        else  // just copy the array into tmpLut
        {
            m_step = 1.0f / OpData::GetValueStepSize(lut->getInputBitDepth(),
                                                     m_dim);

            const OpData::Array::Values& lutValues = lut->getArray().getValues();

            m_tmpLutR = new float[m_dim];
            m_tmpLutG = new float[m_dim];
            m_tmpLutB = new float[m_dim];

            // TODO: Would be faster if R, G, B were adjacent in memory?
            for(unsigned i=0; i<m_dim; ++i)
            {
                // Urgent TODO: The tmpLut must contain unclamped floats.
                // Clamping/quantizing before interpolation will cause errors.
                // TODO: We need two tmpLut arrays, a float array that can be
                // used when we're interpolating and a templated type that
                // gets used for lookups.
                m_tmpLutR[i] = L_ADJUST(lutValues[i*3+0]);
                m_tmpLutG[i] = L_ADJUST(lutValues[i*3+1]);
                m_tmpLutB[i] = L_ADJUST(lutValues[i*3+2]);
            }
        }

        m_alphaScaling = GetBitDepthMaxValue(outBD)
                            / GetBitDepthMaxValue(inBD);

        m_dimMinusOne = (float)m_dim - 1.0f;

    }

    Lut1DRenderer::~Lut1DRenderer()
    {
    }

    void Lut1DRenderer::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        const float * lutR = m_tmpLutR;
        const float * lutG = m_tmpLutG;
        const float * lutB = m_tmpLutB;

        // TODO: To uncomment when apply bit depth is in
        const bool isLookup = false; //GET_BIT_DEPTH( PF_IN ) != BIT_DEPTH_F32;

        // NB: The if/else is expanded at compile time based on the template args.
        //     (Should be no runtime cost.)
        if (isLookup)
        {
            float * rgba = rgbaBuffer;

            for(unsigned idx=0; idx<numPixels; ++idx)
            {
                rgba[0] = LookupLut<float, float>::compute(lutR, rgba[0]);
                rgba[1] = LookupLut<float, float>::compute(lutG, rgba[1]);
                rgba[2] = LookupLut<float, float>::compute(lutB, rgba[2]);
                rgba[3] = rgba[3] * m_alphaScaling;

                rgba += 4;
            }
        }
        else  // Need to interpolate rather than simply lookup
        {
            float * rgba = rgbaBuffer;

            for(unsigned i=0; i<numPixels; ++i)
            {

                __m128 idx
                    = _mm_mul_ps(_mm_set_ps(rgba[3],
                                            rgba[2],
                                            rgba[1],
                                            rgba[0]),
                                 _mm_set_ps(1.0f,
                                            m_step,
                                            m_step,
                                            m_step));

                // _mm_max_ps => NaNs become 0
                idx = _mm_min_ps(_mm_max_ps(idx, EZERO),
                                 _mm_set1_ps(m_dimMinusOne));

                // zero < std::floor(idx) < maxIdx
                // SSE => zero < truncate(idx) < maxIdx
                //
                __m128 lowIdx = _mm_cvtepi32_ps(_mm_cvttps_epi32(idx));

                // zero < std::ceil(idx) < maxIdx
                // SSE => (lowIdx (already truncated) + 1) < maxIdx
                //   When the idx is exactly equal to an index (e.g. 0,1,2...)
                //   then the computation of highIdx is wrong. However,
                //   the delta is then equal to zero (e.g. lowIdx-idx),
                //   so the highIdx has no impact.
                //
                __m128 highIdx = _mm_min_ps(_mm_add_ps(lowIdx, EONE),
                                            _mm_set1_ps(m_dimMinusOne));

                // Computing delta relative to high rather than lowIdx
                // to save computing (1-delta) below.
                __m128 delta = _mm_sub_ps(highIdx, idx);

                float d[4];    _mm_storeu_ps(d, delta);
                float lIdx[4]; _mm_storeu_ps(lIdx, lowIdx);
                float hIdx[4]; _mm_storeu_ps(hIdx, highIdx);

                // TODO: If the LUT is a lookup one and the
                //       the output bit depth is different from float,
                //       then this computation implicitely introduces
                //       several type conversions.
                //       Could it impact performance ?
                //       To be investigated...

                // Since fraction is in the domain [0, 1), interpolate using 1-fraction
                // in order to avoid cases like -/+Inf * 0. Therefore we never multiply by 0 and
                // thus handle the case where A or B is infinity and return infinity rather than
                // 0*Infinity (which is NaN).
                rgba[0] = lerpf(lutR[(unsigned)hIdx[0]], lutR[(unsigned)lIdx[0]], d[0]);
                rgba[1] = lerpf(lutG[(unsigned)hIdx[1]], lutG[(unsigned)lIdx[1]], d[1]);
                rgba[2] = lerpf(lutB[(unsigned)hIdx[2]], lutB[(unsigned)lIdx[2]], d[2]);
                rgba[3] = rgba[3] * m_alphaScaling;

                rgba += 4;
            }
        }
    }

    // TODO:  Below largely copy/pasted the regular and halfCode renderers in order
    // to implement the hueAdjust versions quickly and without risk of breaking
    // the original renderers.

    Lut1DRendererHalfCodeHueAdjust::Lut1DRendererHalfCodeHueAdjust(const OpData::OpDataLut1DRcPtr & lut)
        :  Lut1DRendererHalfCode(lut)
    {
        // Regardless of the desired out-depth, we need the LUT1D to produce a 32f
        // result to be used in the hue adjust post-process.
        m_outBitDepth = BIT_DEPTH_F32;

        updateData(lut);
    }

    Lut1DRendererHalfCodeHueAdjust::~Lut1DRendererHalfCodeHueAdjust()
    {
    }

    void Lut1DRendererHalfCodeHueAdjust::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        // TODO: To uncomment when apply bit depth will be in
        const bool isLookup = false; //GET_BIT_DEPTH( PF_IN ) != BIT_DEPTH_F32;

        // The LUT entries should always be 32f in this case 
        // since we need to do additional computations.
        const float* lutR = (const float*)m_tmpLutR;
        const float* lutG = (const float*)m_tmpLutG;
        const float* lutB = (const float*)m_tmpLutB;

        // NB: The if/else is expanded at compile time based on the template args.
        // (Should be no runtime cost.)

        if (isLookup)
        {
            float * rgba = rgbaBuffer;

            for(unsigned idx=0; idx<numPixels; ++idx)
            {
                const float RGB[] = {rgba[0], rgba[1], rgba[2]};

                // TODO: Refactor to use SSE2 intrinsic instructions

                int min, mid, max;
                GamutMapUtils::Order3( RGB, min, mid, max);

                const float orig_chroma = RGB[max] - RGB[min];
                const float hue_factor 
                    = orig_chroma == 0.f  ?  0.f 
                                          :  (RGB[mid] - RGB[min]) / orig_chroma;

                float RGB2[] = {
                    LookupLut<float, float>::compute(lutR, rgba[0]),
                    LookupLut<float, float>::compute(lutG, rgba[1]),
                    LookupLut<float, float>::compute(lutB, rgba[2])   };

                const float new_chroma = RGB2[max] - RGB2[min];

                RGB2[mid] = hue_factor * new_chroma + RGB2[min];

                rgba[0] = RGB2[0];
                rgba[1] = RGB2[1];
                rgba[2] = RGB2[2];
                rgba[3] = rgba[3] * m_alphaScaling;

                rgba += 4;
            }
        }
        else  // need to interpolate rather than simply lookup
        {
            float * rgba = rgbaBuffer;

            for(unsigned idx=0; idx<numPixels; ++idx)
            {
                const float RGB[] = {rgba[0], rgba[1], rgba[2]};

                // TODO: Refactor to use SSE2 intrinsic instructions

                int min, mid, max;
                GamutMapUtils::Order3( RGB, min, mid, max);
                const float orig_chroma = RGB[max] - RGB[min];
                const float hue_factor 
                    = orig_chroma == 0.f  ? 0.f
                                          : (RGB[mid] - RGB[min]) / orig_chroma;

                // Red
                IndexPair redInterVals = getEdgeFloatValues(RGB[0]);

                // Green
                IndexPair greenInterVals = getEdgeFloatValues(RGB[1]);

                // Blue
                IndexPair blueInterVals = getEdgeFloatValues(RGB[2]);

                // Since fraction is in the domain [0, 1), interpolate using 1-fraction
                // in order to avoid cases like -/+Inf * 0.
                float RGB2[] = {
                    lerpf(lutR[redInterVals.valB],
                          lutR[redInterVals.valA],
                          1.0f-redInterVals.fraction),
                    lerpf(lutG[greenInterVals.valB],
                          lutG[greenInterVals.valA],
                          1.0f-greenInterVals.fraction),
                    lerpf(lutB[blueInterVals.valB],
                          lutB[blueInterVals.valA],
                          1.0f-blueInterVals.fraction)  };

                const float new_chroma = RGB2[max] - RGB2[min];

                // TODO: Mauro suggests the following to ease SSE
                // implementation (may be applied to all chans):
                // RGB2[channel] = (RGB[channel] - RGB(min)) 
                //                 * new_chroma / orig_chroma + RGB2[min].
                RGB2[mid] = hue_factor * new_chroma + RGB2[min];

                rgba[0] = RGB2[0];
                rgba[1] = RGB2[1];
                rgba[2] = RGB2[2];
                rgba[3] = rgba[3] * m_alphaScaling;

                rgba += 4;
            }
        }
    }

    Lut1DRendererHueAdjust::Lut1DRendererHueAdjust(const OpData::OpDataLut1DRcPtr & lut)
        :  Lut1DRenderer(lut)
    {
        // Regardless of the desired out-depth, we need the LUT1D to produce a 32f
        // result to be used in the hue adjust post-process.
        m_outBitDepth = BIT_DEPTH_F32;

        // Need to recalculate the tmpLUTs at 32f outdepth.
        updateData(lut);
    }

    Lut1DRendererHueAdjust::~Lut1DRendererHueAdjust()
    {
    }

    void Lut1DRendererHueAdjust::apply(float * rgbaBuffer, unsigned numPixels) const
    {
        // TODO: To uncomment when apply bit depth will be in
        const bool isLookup = false; //GET_BIT_DEPTH( PF_IN ) != OCIO::BIT_DEPTH_F32;

        // The LUT entries should always be 32f in this case 
        // since we need to do additional computations.
        const float * lutR = (const float *)m_tmpLutR;
        const float * lutG = (const float *)m_tmpLutG;
        const float * lutB = (const float *)m_tmpLutB;

        // NB: The if/else is expanded at compile time based on the template args.
        // (Should be no runtime cost.)

        if (isLookup)
        {
            float * rgba = rgbaBuffer;

            for(unsigned idx=0; idx<numPixels; ++idx)
            {
                const float RGB[] = {rgba[0], rgba[1], rgba[2]};

                int min, mid, max;
                GamutMapUtils::Order3(RGB, min, mid, max);
                
                const float orig_chroma = RGB[max] - RGB[min];
                const float hue_factor
                    = orig_chroma == 0.f ? 0.f
                                         : (RGB[mid] - RGB[min]) / orig_chroma;

                float RGB2[] = {
                    LookupLut<float, float>::compute(lutR, rgba[0]),
                    LookupLut<float, float>::compute(lutG, rgba[1]),
                    LookupLut<float, float>::compute(lutB, rgba[2])
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
        else  // Need to interpolate rather than simply lookup
        {
            float * rgba = rgbaBuffer;

            for(unsigned i=0; i<numPixels; ++i)
            {
                const float RGB[] = {rgba[0], rgba[1], rgba[2]};

                int min, mid, max;
                GamutMapUtils::Order3( RGB, min, mid, max);

                const float orig_chroma = RGB[max] - RGB[min];
                const float hue_factor 
                    = orig_chroma == 0.f ? 0.f
                                         : (RGB[mid] - RGB[min]) / orig_chroma;

                __m128 idx
                    = _mm_mul_ps(_mm_set_ps(rgba[3],
                                            RGB[2],
                                            RGB[1],
                                            RGB[0]),
                                 _mm_set_ps(1.0f,
                                            m_step,
                                            m_step,
                                            m_step));

                // _mm_max_ps => NaNs become 0
                idx = _mm_min_ps(_mm_max_ps(idx, EZERO),
                                 _mm_set1_ps(m_dimMinusOne));

                // zero < std::floor(idx) < maxIdx
                // SSE => zero < truncate(idx) < maxIdx
                //
                __m128 lowIdx = _mm_cvtepi32_ps(_mm_cvttps_epi32(idx));

                // zero < std::ceil(idx) < maxIdx
                // SSE => (lowIdx (already truncated) + 1) < maxIdx
                //   When the idx is exactly equal to an index (e.g. 0,1,2...)
                //   then the computation of highIdx is wrong. However,
                //   the delta is then equal to zero (e.g. lowIdx-idx),
                //   so the highIdx has no impact.
                //
                __m128 highIdx = _mm_min_ps(_mm_add_ps(lowIdx, EONE),
                                            _mm_set1_ps(m_dimMinusOne));

                // Computing delta relative to high rather than lowIdx
                // to save computing (1-delta) below.
                __m128 delta = _mm_sub_ps(highIdx, idx);

                float d[4];    _mm_storeu_ps(d, delta);
                float lIdx[4]; _mm_storeu_ps(lIdx, lowIdx);
                float hIdx[4]; _mm_storeu_ps(hIdx, highIdx);

                // TODO: If the LUT is a lookup one and the
                //       the output bit depth is different from float,
                //       then this computation implicitely introduces
                //       several type conversions.
                //       Could it impact performance ?
                //       To be investigated...

                // Since fraction is in the domain [0, 1), interpolate using 1-fraction
                // in order to avoid cases like -/+Inf * 0. Therefore we never multiply by 0 and
                // thus handle the case where A or B is infinity and return infinity rather than
                // 0*Infinity (which is NaN).
                float RGB2[] = {
                    lerpf(lutR[(unsigned)hIdx[0]], lutR[(unsigned)lIdx[0]], d[0]),
                    lerpf(lutG[(unsigned)hIdx[1]], lutG[(unsigned)lIdx[1]], d[1]),
                    lerpf(lutB[(unsigned)hIdx[2]], lutB[(unsigned)lIdx[2]], d[2])
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
    }


    CPUOpRcPtr Lut1DRenderer::GetRenderer(const OpData::OpDataLut1DRcPtr & lut)
    {
        CPUOpRcPtr op(new CPUNoOp);

        // NB: Unlike bit-depth, the half domain status of a LUT
        //     may not be changed.
        if (lut->isInputHalfDomain())
        {
            if (lut->getHueAdjust() == OpData::Lut1D::HUE_DW3)
            {
                op.reset(new Lut1DRendererHalfCodeHueAdjust(lut));
            }
            else
            {
                op.reset(new Lut1DRendererHalfCode(lut));
            }
        }
        else
        {
            if (lut->getHueAdjust() == OpData::Lut1D::HUE_DW3)
            {
                op.reset(new Lut1DRendererHueAdjust(lut));
            }
            else
            {
                op.reset(new Lut1DRenderer(lut));
            }
        }

        return op;
    }

}
OCIO_NAMESPACE_EXIT

