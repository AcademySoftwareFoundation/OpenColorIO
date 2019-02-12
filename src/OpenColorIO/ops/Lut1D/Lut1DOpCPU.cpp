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

#include <algorithm>
#include <math.h>
#include <memory>
#include <stdint.h>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/Lut1D/Lut1DOpCPU.h"
#include "OpTools.h"
#include "Platform.h"
#include "SSE.h"

#define L_ADJUST(val) \
    ((isOutInteger) ? clamp((val)+0.5f, outMin,  outMax) : SanitizeFloat(val))

OCIO_NAMESPACE_ENTER
{
namespace
{
inline uint8_t GetLookupValue(const uint8_t& val)
{
    return val;
}

inline uint16_t GetLookupValue(const uint16_t& val)
{
    return val;
}

inline unsigned short GetLookupValue(const half& val)
{
    return val.bits();
}

// When instantiating all templates this case is needed.
// But it will never be used as the 32f is not a lookup case.
inline uint16_t GetLookupValue(const float& val)
{
    return (uint16_t)val;
}

template<typename InType, typename OutType>
struct LookupLut
{
    static inline OutType compute(const OutType* lutData,
                                  const InType& val)
    {
        return lutData[GetLookupValue(val)];
    }
};

class BaseLut1DRenderer : public OpCPU
{
public:
    BaseLut1DRenderer(ConstLut1DOpDataRcPtr & lut);
    virtual ~BaseLut1DRenderer();

    virtual void updateData(ConstLut1DOpDataRcPtr & lut) = 0;

    void resetData();

protected:
    unsigned long m_dim;

    // TODO: Anticipates tmpLut eventually allowing integer data types.
    std::vector<float> m_tmpLutR;
    std::vector<float> m_tmpLutG;
    std::vector<float> m_tmpLutB;

    float m_alphaScaling;

    BitDepth m_outBitDepth;

private:
    BaseLut1DRenderer() = delete;
    BaseLut1DRenderer(const BaseLut1DRenderer &) = delete;
    BaseLut1DRenderer & operator=(const BaseLut1DRenderer &) = delete;
};

class Lut1DRendererHalfCode : public BaseLut1DRenderer
{
public:
    explicit Lut1DRendererHalfCode(ConstLut1DOpDataRcPtr & lut);
    virtual ~Lut1DRendererHalfCode();

    // Structure used to keep track of the interpolation data for
    // special case 16f/64k 1D LUT.
    struct IndexPair
    {
        unsigned short valA;
        unsigned short valB;
        float fraction;

    public:
        IndexPair()
        {
            valA = 0;
            valB = 0;
            fraction = 0.0f;
        }
    };

    void updateData(ConstLut1DOpDataRcPtr & lut) override;

    void apply(float * rgbaBuffer, long numPixels) const override;

protected:
    // Interpolation helper to gather data.
    IndexPair getEdgeFloatValues(float fIn) const;
};

class Lut1DRenderer : public BaseLut1DRenderer
{
public:

    Lut1DRenderer(ConstLut1DOpDataRcPtr & lut);
    virtual ~Lut1DRenderer();

    void updateData(ConstLut1DOpDataRcPtr & lut) override;

    void apply(float * rgbaBuffer, long numPixels) const override;

protected:
    float m_step;
    float m_dimMinusOne;
};

class Lut1DRendererHueAdjust : public Lut1DRenderer
{
public:
    Lut1DRendererHueAdjust(ConstLut1DOpDataRcPtr & lut);
    virtual ~Lut1DRendererHueAdjust();

    void apply(float * rgbaBuffer, long numPixels) const override;
};

class Lut1DRendererHalfCodeHueAdjust : public Lut1DRendererHalfCode
{
public:
    Lut1DRendererHalfCodeHueAdjust(ConstLut1DOpDataRcPtr & lut);
    virtual ~Lut1DRendererHalfCodeHueAdjust();

    void apply(float * rgbaBuffer, long numPixels) const override;
};

class InvLut1DRenderer : public OpCPU
{
public:
    InvLut1DRenderer(ConstLut1DOpDataRcPtr & lut);
    virtual ~InvLut1DRenderer();

    void apply(float * rgbaBuffer, long numPixels) const override;

    void resetData();

    virtual void updateData(ConstLut1DOpDataRcPtr & lut);

public:
    // Holds the parameters of a color component.
    // Note: The structure does not own any of the pointers.
    struct ComponentParams
    {
        ComponentParams()
            : lutStart(nullptr), startOffset(0.f), lutEnd(nullptr)
            , negLutStart(nullptr), negStartOffset(0.f), negLutEnd(nullptr)
            , flipSign(1.f), bisectPoint(0.f)
        {}

        const float * lutStart;   // copy of the pointer to start of effective lutData
        float startOffset;        // difference between real and effective start of lut
        const float * lutEnd;     // copy of the pointer to end of effective lutData
        const float * negLutStart;// lutStart for negative part of half domain LUT
        float negStartOffset;     // startOffset for negative part of half domain LUT
        const float * negLutEnd;  // lutEnd for negative part of half domain LUT
        float flipSign;           // flip the sign of value to handle decreasing luts
        float bisectPoint;        // point of switching from pos to neg of half domain
    };

    void setComponentParams(ComponentParams & params,
                            const Lut1DOpData::ComponentProperties & properties,
                            const float * lutPtr,
                            const float lutZeroEntry);

protected:
    float m_scale; // output scaling for the r, g and b components

    ComponentParams m_paramsR;
    ComponentParams m_paramsG;
    ComponentParams m_paramsB;

    unsigned long      m_dim;
    std::vector<float> m_tmpLutR;
    std::vector<float> m_tmpLutG;
    std::vector<float> m_tmpLutB;
    float              m_alphaScaling;  // bit-depth scale factor for alpha channel

private:
    InvLut1DRenderer() = delete;
    InvLut1DRenderer(const InvLut1DRenderer&) = delete;
    InvLut1DRenderer& operator=(const InvLut1DRenderer&) = delete;
};

class InvLut1DRendererHalfCode : public InvLut1DRenderer
{
public:
    InvLut1DRendererHalfCode(ConstLut1DOpDataRcPtr & lut);
    virtual ~InvLut1DRendererHalfCode();

    void updateData(ConstLut1DOpDataRcPtr & lut) override;

    void apply(float * rgbaBuffer, long numPixels) const override;
};

class InvLut1DRendererHueAdjust : public InvLut1DRenderer
{
public:
    InvLut1DRendererHueAdjust(ConstLut1DOpDataRcPtr & lut);
    virtual ~InvLut1DRendererHueAdjust();

    void apply(float * rgbaBuffer, long numPixels) const override;
};

class InvLut1DRendererHalfCodeHueAdjust : public InvLut1DRendererHalfCode
{
public:
    InvLut1DRendererHalfCodeHueAdjust(ConstLut1DOpDataRcPtr & lut);
    virtual ~InvLut1DRendererHalfCodeHueAdjust();

    void apply(float * rgbaBuffer, long numPixels) const override;
};

BaseLut1DRenderer::BaseLut1DRenderer(ConstLut1DOpDataRcPtr & lut)
    :   OpCPU()
    ,   m_dim(lut->getArray().getLength())
    ,   m_alphaScaling(0.0f)
    ,   m_outBitDepth(lut->getOutputBitDepth())
{
}

void BaseLut1DRenderer::resetData()
{
    m_tmpLutR.resize(0);
    m_tmpLutG.resize(0);
    m_tmpLutB.resize(0);
}

BaseLut1DRenderer::~BaseLut1DRenderer()
{
    resetData();
}

Lut1DRendererHalfCode::Lut1DRendererHalfCode(ConstLut1DOpDataRcPtr & lut)
    :  BaseLut1DRenderer(lut)
{
    updateData(lut);
}

void Lut1DRendererHalfCode::updateData(ConstLut1DOpDataRcPtr & lut)
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
        Lut1DOpDataRcPtr newLut
            = Lut1DOpData::MakeLookupDomain(inBD);

        // Note: Compose should render at 32f, to avoid infinite recursion.
        Lut1DOpData::Compose(newLut,
                             lut,
                             // Prevent compose from modifying newLut domain.
                             Lut1DOpData::COMPOSE_RESAMPLE_NO);

        m_dim = newLut->getArray().getLength();

        m_tmpLutR.resize(m_dim);
        m_tmpLutG.resize(m_dim);
        m_tmpLutB.resize(m_dim);

        const Array::Values & 
            lutValues = newLut->getArray().getValues();

        // TODO: Would be faster if R, G, B were adjacent in memory?
        for(unsigned long i=0; i<m_dim; ++i)
        {
            m_tmpLutR[i] = L_ADJUST(lutValues[i*3+0]);
            m_tmpLutG[i] = L_ADJUST(lutValues[i*3+1]);
            m_tmpLutB[i] = L_ADJUST(lutValues[i*3+2]);
        }
    }
    else
    {
        const Array::Values & 
            lutValues = lut->getArray().getValues();

        m_tmpLutR.resize(m_dim);
        m_tmpLutG.resize(m_dim);
        m_tmpLutB.resize(m_dim);

        for(unsigned long i=0; i<m_dim; ++i)
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

void Lut1DRendererHalfCode::apply(float * rgbaBuffer, long numPixels) const
{
    // TODO: Add ability for input or output to be an integer rather than
    // always float.

    // TODO: To uncomment when apply bit-depth is in.
    // const bool isLookup = GET_BIT_DEPTH( PF_IN ) != OCIO::BIT_DEPTH_F32;
    const bool isLookup = false; 

    const float * lutR = m_tmpLutR.data();
    const float * lutG = m_tmpLutG.data();
    const float * lutB = m_tmpLutB.data();

    if (isLookup)
    {
        float * rgba = rgbaBuffer;

        for(long idx=0; idx<numPixels; ++idx)
        {
            rgba[0] = LookupLut<float, float>::compute(lutR, rgba[0]);
            rgba[1] = LookupLut<float, float>::compute(lutG, rgba[1]);
            rgba[2] = LookupLut<float, float>::compute(lutB, rgba[2]);
            rgba[3] = rgba[3] * m_alphaScaling;

            rgba += 4;
        }
    }
    else  // Need to interpolate rather than simply lookup.
    {
        float * rgba = rgbaBuffer;

        for(long idx=0; idx<numPixels; ++idx)
        {
            IndexPair redInterVals = getEdgeFloatValues(rgba[0]);
            IndexPair greenInterVals = getEdgeFloatValues(rgba[1]);
            IndexPair blueInterVals = getEdgeFloatValues(rgba[2]);

            // Since fraction is in the domain [0, 1), interpolate using
            // 1-fraction in order to avoid cases like -/+Inf * 0.
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

    // Strict comparison required otherwise negative fractions will occur.
    if (fabs(floatTemp)> fabs(fIn)) 
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

Lut1DRenderer::Lut1DRenderer(ConstLut1DOpDataRcPtr & lut)
    :  BaseLut1DRenderer(lut)
    ,   m_step(0.0f)
    ,   m_dimMinusOne(0.0f)
{
    updateData(lut);
}

void Lut1DRenderer::updateData(ConstLut1DOpDataRcPtr & lut)
{
    resetData();

    m_dim = lut->getArray().getLength();

    const BitDepth inBD = lut->getInputBitDepth();
    const BitDepth outBD= lut->getOutputBitDepth();

    const float outMin = 0.0f;
    const float outMax = GetBitDepthMaxValue(outBD);

    // TODO: To uncomment when apply bit-depth will be in
    const bool isLookup = false; //GET_BIT_DEPTH( PF_IN ) != BIT_DEPTH_F32;

    const bool isOutInteger = !IsFloatBitDepth(outBD);

    const bool mustResample = !lut->mayLookup(inBD);

    // If we are able to lookup, need to resample the LUT based on inBitDepth.
    if (isLookup && mustResample)
    {
        Lut1DOpDataRcPtr newLut
            = Lut1DOpData::MakeLookupDomain(lut->getInputBitDepth());

        // Note: Compose should render at 32f, to avoid infinite recursion.
        Lut1DOpData::Compose(newLut,
                             lut,
                             // Prevent compose from modifying newLut domain.
                             Lut1DOpData::COMPOSE_RESAMPLE_NO);

        m_dim = newLut->getArray().getLength();

        const Array::Values& lutValues = newLut->getArray().getValues();

        m_tmpLutR.resize(m_dim);
        m_tmpLutG.resize(m_dim);
        m_tmpLutB.resize(m_dim);

        // TODO: Would be faster if R, G, B were adjacent in memory?
        for(unsigned long i=0; i<m_dim; ++i)
        {
            m_tmpLutR[i] = L_ADJUST(lutValues[i*3+0]);
            m_tmpLutG[i] = L_ADJUST(lutValues[i*3+1]);
            m_tmpLutB[i] = L_ADJUST(lutValues[i*3+2]);
        }
    }
    else  // Just copy the array into tmpLut.
    {
        m_step = ((float)m_dim - 1.0f)
                 / GetBitDepthMaxValue(lut->getInputBitDepth());

        const Array::Values & lutValues = lut->getArray().getValues();

        m_tmpLutR.resize(m_dim);
        m_tmpLutG.resize(m_dim);
        m_tmpLutB.resize(m_dim);

        // TODO: Would be faster if R, G, B were adjacent in memory?
        for(unsigned long i=0; i<m_dim; ++i)
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

void Lut1DRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    const float * lutR = m_tmpLutR.data();
    const float * lutG = m_tmpLutG.data();
    const float * lutB = m_tmpLutB.data();

    // TODO: To uncomment when apply bit depth is in.
    // const bool isLookup = GET_BIT_DEPTH( PF_IN ) != BIT_DEPTH_F32;
    const bool isLookup = false; 

    // NB: The if/else is expanded at compile time based on the template args.
    //     (Should be no runtime cost.)
    if (isLookup)
    {
        float * rgba = rgbaBuffer;

        for(long idx=0; idx<numPixels; ++idx)
        {
            rgba[0] = LookupLut<float, float>::compute(lutR, rgba[0]);
            rgba[1] = LookupLut<float, float>::compute(lutG, rgba[1]);
            rgba[2] = LookupLut<float, float>::compute(lutB, rgba[2]);
            rgba[3] = rgba[3] * m_alphaScaling;

            rgba += 4;
        }
    }
    else  // Need to interpolate rather than simply lookup.
    {
        float * rgba = rgbaBuffer;

        for(long i=0; i<numPixels; ++i)
        {
#ifdef USE_SSE
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
            __m128 lIdx = _mm_cvtepi32_ps(_mm_cvttps_epi32(idx));

            // zero < std::ceil(idx) < maxIdx
            // SSE => (lowIdx (already truncated) + 1) < maxIdx
            // then clamp to prevent hIdx from falling off the end
            // of the LUT
            __m128 hIdx = _mm_min_ps(_mm_add_ps(lIdx, EONE),
                                     _mm_set1_ps(m_dimMinusOne));

            // Computing delta relative to high rather than lowIdx
            // to save computing (1-delta) below.
            __m128 d = _mm_sub_ps(hIdx, idx);

            float delta[4];   _mm_storeu_ps(delta, d);
            float lowIdx[4];  _mm_storeu_ps(lowIdx, lIdx);
            float highIdx[4]; _mm_storeu_ps(highIdx, hIdx);
#else
            float idx[3];
            idx[0] = m_step * rgba[0];
            idx[1] = m_step * rgba[1];
            idx[2] = m_step * rgba[2];

            // NaNs become 0
            idx[0] = std::min(std::max(0.f, idx[0]), m_dimMinusOne);
            idx[1] = std::min(std::max(0.f, idx[1]), m_dimMinusOne);
            idx[2] = std::min(std::max(0.f, idx[2]), m_dimMinusOne);

            unsigned int lowIdx[3];
            lowIdx[0] = static_cast<unsigned int>(std::floor(idx[0]));
            lowIdx[1] = static_cast<unsigned int>(std::floor(idx[1]));
            lowIdx[2] = static_cast<unsigned int>(std::floor(idx[2]));

            // When the idx is exactly equal to an index (e.g. 0,1,2...)
            // then the computation of highIdx is wrong. However,
            // the delta is then equal to zero (e.g. lowIdx-idx),
            // so the highIdx has no impact.
            unsigned int highIdx[3];
            highIdx[0] = static_cast<unsigned int>(std::ceil(idx[0]));
            highIdx[1] = static_cast<unsigned int>(std::ceil(idx[1]));
            highIdx[2] = static_cast<unsigned int>(std::ceil(idx[2]));

            // Computing delta relative to high rather than lowIdx
            // to save computing (1-delta) below.
            float delta[3];
            delta[0] = (float)highIdx[0] - idx[0];
            delta[1] = (float)highIdx[1] - idx[1];
            delta[2] = (float)highIdx[2] - idx[2];

#endif
            // TODO: If the LUT is a lookup one and the
            //       the output bit-depth is different from float,
            //       then this computation implicitely introduces
            //       several type conversions.
            //       Could it impact performance ?
            //       To be investigated...

            // Since fraction is in the domain [0, 1), interpolate using 1-fraction
            // in order to avoid cases like -/+Inf * 0. Therefore we never multiply by 0 and
            // thus handle the case where A or B is infinity and return infinity rather than
            // 0*Infinity (which is NaN).
            rgba[0] = lerpf(lutR[(unsigned int)highIdx[0]], lutR[(unsigned int)lowIdx[0]], delta[0]);
            rgba[1] = lerpf(lutG[(unsigned int)highIdx[1]], lutG[(unsigned int)lowIdx[1]], delta[1]);
            rgba[2] = lerpf(lutB[(unsigned int)highIdx[2]], lutB[(unsigned int)lowIdx[2]], delta[2]);
            rgba[3] = rgba[3] * m_alphaScaling;

            rgba += 4;
        }
    }
}

namespace GamutMapUtils
{
    // Compute the indices for the smallest, middle, and largest elements of
    // RGB[3]. (Trying to be clever and do this without branching.)
    inline void Order3(const float* RGB, int& min, int& mid, int& max)
    {
        //                                    0  1  2  3  4  5  6  7  8  (typical val - 3)
        static const int table[] = { 2, 1, 0, 2, 1, 0, 2, 1, 2, 0, 1, 2 };

        int val = (int(RGB[0] > RGB[1]) * 5 + int(RGB[1] > RGB[2]) * 4)
                  - int(RGB[0] > RGB[2]) * 3 + 3;

        // A NaN in a logical comparison always results in False.
        // So the case to be careful of here is { A, NaN, B } with A > B.
        // In that case, the first two compares are false but the third is true
        // (something that would never happen with regular numbers).
        // So that is the reason for the +3, to make val 0 in that case.

        max = table[val];
        mid = table[++val];
        min = table[++val];
    }
};


// TODO:  Below largely copy/pasted the regular and halfCode renderers in order
// to implement the hueAdjust versions quickly and without risk of breaking
// the original renderers.

Lut1DRendererHalfCodeHueAdjust::Lut1DRendererHalfCodeHueAdjust(ConstLut1DOpDataRcPtr & lut)
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

void Lut1DRendererHalfCodeHueAdjust::apply(float * rgbaBuffer, long numPixels) const
{
    // TODO: To uncomment when apply bit-depth will be in
    const bool isLookup = false; //GET_BIT_DEPTH( PF_IN ) != BIT_DEPTH_F32;

    // The LUT entries should always be 32f in this case 
    // since we need to do additional computations.
    const float* lutR = m_tmpLutR.data();
    const float* lutG = m_tmpLutG.data();
    const float* lutB = m_tmpLutB.data();

    // NB: The if/else is expanded at compile time based on the template args.
    // (Should be no runtime cost.)

    if (isLookup)
    {
        float * rgba = rgbaBuffer;

        for(long idx=0; idx<numPixels; ++idx)
        {
            const float RGB[] = {rgba[0], rgba[1], rgba[2]};

            // TODO: Refactor to use SSE2 intrinsic instructions.

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
    else  // Need to interpolate rather than simply lookup.
    {
        float * rgba = rgbaBuffer;

        for(long idx=0; idx<numPixels; ++idx)
        {
            const float RGB[] = {rgba[0], rgba[1], rgba[2]};

            int min, mid, max;
            GamutMapUtils::Order3(RGB, min, mid, max);
            IndexPair redInterVals = getEdgeFloatValues(RGB[0]);
            IndexPair greenInterVals = getEdgeFloatValues(RGB[1]);
            IndexPair blueInterVals = getEdgeFloatValues(RGB[2]);

            // Since fraction is in the domain [0, 1), interpolate using
            // 1-fraction in order to avoid cases like -/+Inf * 0.
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

            // TODO: ease SSE implementation (may be applied to all chans):
            // RGB2[channel] = (RGB[channel] - RGB(min)) 
            //                 * new_chroma / orig_chroma + RGB2[min].
            const float orig_chroma = RGB[max] - RGB[min];
            const float hue_factor 
                = orig_chroma == 0.f  ? 0.f
                                      : (RGB[mid] - RGB[min]) / orig_chroma;

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

Lut1DRendererHueAdjust::Lut1DRendererHueAdjust(ConstLut1DOpDataRcPtr & lut)
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

void Lut1DRendererHueAdjust::apply(float * rgbaBuffer, long numPixels) const
{
    // TODO: To uncomment when apply bit-depth will be in
    //const bool isLookup = GET_BIT_DEPTH( PF_IN ) != OCIO::BIT_DEPTH_F32;
    const bool isLookup = false; 

    // The LUT entries should always be 32f in this case 
    // since we need to do additional computations.
    const float * lutR = m_tmpLutR.data();
    const float * lutG = m_tmpLutG.data();
    const float * lutB = m_tmpLutB.data();

    // NB: The if/else is expanded at compile time based on the template args.
    // (Should be no runtime cost.)

    if (isLookup)
    {
        float * rgba = rgbaBuffer;

        for(long idx=0; idx<numPixels; ++idx)
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
    else  // Need to interpolate rather than simply lookup.
    {
        float * rgba = rgbaBuffer;

        for(long i=0; i<numPixels; ++i)
        {
            const float RGB[] = {rgba[0], rgba[1], rgba[2]};

            int min, mid, max;
            GamutMapUtils::Order3( RGB, min, mid, max);

            const float orig_chroma = RGB[max] - RGB[min];
            const float hue_factor 
                = orig_chroma == 0.f ? 0.f
                                     : (RGB[mid] - RGB[min]) / orig_chroma;

#ifdef USE_SSE
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
            // then clamp to prevent hIdx from falling off the end
            // of the LUT
            __m128 lIdx = _mm_cvtepi32_ps(_mm_cvttps_epi32(idx));

            // zero < std::ceil(idx) < maxIdx
            // SSE => (lowIdx (already truncated) + 1) < maxIdx
            __m128 hIdx = _mm_min_ps(_mm_add_ps(lIdx, EONE),
                                     _mm_set1_ps(m_dimMinusOne));

            // Computing delta relative to high rather than lowIdx
            // to save computing (1-delta) below.
            __m128 d = _mm_sub_ps(hIdx, idx);

            float delta[4];   _mm_storeu_ps(delta, d);
            float lowIdx[4];  _mm_storeu_ps(lowIdx, lIdx);
            float highIdx[4]; _mm_storeu_ps(highIdx, hIdx);
#else
            float idx[3];
            idx[0] = m_step * RGB[0];
            idx[1] = m_step * RGB[1];
            idx[2] = m_step * RGB[2];

            // NaNs become 0
            idx[0] = std::min(std::max(0.f, idx[0]), m_dimMinusOne);
            idx[1] = std::min(std::max(0.f, idx[1]), m_dimMinusOne);
            idx[2] = std::min(std::max(0.f, idx[2]), m_dimMinusOne);

            unsigned int lowIdx[3];
            lowIdx[0] = static_cast<unsigned int>(std::floor(idx[0]));
            lowIdx[1] = static_cast<unsigned int>(std::floor(idx[1]));
            lowIdx[2] = static_cast<unsigned int>(std::floor(idx[2]));

            // When the idx is exactly equal to an index (e.g. 0,1,2...)
            // then the computation of highIdx is wrong. However,
            // the delta is then equal to zero (e.g. lowIdx-idx),
            // so the highIdx has no impact.
            unsigned int highIdx[3];
            highIdx[0] = static_cast<unsigned int>(std::ceil(idx[0]));
            highIdx[1] = static_cast<unsigned int>(std::ceil(idx[1]));
            highIdx[2] = static_cast<unsigned int>(std::ceil(idx[2]));

            // Computing delta relative to high rather than lowIdx
            // to save computing (1-delta) below.
            float delta[3];
            delta[0] = (float)highIdx[0] - idx[0];
            delta[1] = (float)highIdx[1] - idx[1];
            delta[2] = (float)highIdx[2] - idx[2];
#endif
            // TODO: If the LUT is a lookup one and the
            //       the output bit-depth is different from float,
            //       then this computation implicitely introduces
            //       several type conversions.
            //       Could it impact performance ?
            //       To be investigated...

            // Since fraction is in the domain [0, 1), interpolate using 1-fraction
            // in order to avoid cases like -/+Inf * 0. Therefore we never multiply by 0 and
            // thus handle the case where A or B is infinity and return infinity rather than
            // 0*Infinity (which is NaN).
            float RGB2[] = {
                lerpf(lutR[(unsigned int)highIdx[0]], lutR[(unsigned int)lowIdx[0]], delta[0]),
                lerpf(lutG[(unsigned int)highIdx[1]], lutG[(unsigned int)lowIdx[1]], delta[1]),
                lerpf(lutB[(unsigned int)highIdx[2]], lutB[(unsigned int)lowIdx[2]], delta[2])
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

namespace
{
// TODO: Use SSE intrinsics to speed this up.

// Calculate the inverse of a value resulting from linear interpolation
// in a 1d LUT.
// start:       Pointer to the first effective LUT entry (end of flat spot).
// startOffset: Distance between first LUT entry and start.
// end:         Pointer to the last effective LUT entry (start of flat spot).
// flipSign:    Flips val if we're working with the negative of the orig LUT.
// scale:       From LUT index units to outDepth units.
// val:         The value to invert.
// Return the result that would produce val if used 
// in a forward linear interpolation in the LUT.
float FindLutInv(const float * start,
                 const float   startOffset,
                 const float * end,
                 const float   flipSign,
                 const float   scale,
                 const float   val)
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
// start:       Pointer to the first effective LUT entry (end of flat spot).
// startOffset: Distance between first LUT entry and start.
// end:         Pointer to the last effective LUT entry (start of flat spot).
// flipSign:    Flips val if we're working with the negative of the orig LUT.
// scale:       From LUT index units to outDepth units.
// val:         The value to invert.
// Return the result that would produce val if used in a forward linear
// interpolation in the LUT.
float FindLutInvHalf(const float * start,
                     const float   startOffset,
                     const float * end,
                     const float   flipSign,
                     const float   scale,
                     const float   val)
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

InvLut1DRenderer::InvLut1DRenderer(ConstLut1DOpDataRcPtr & lut) 
    :   OpCPU()
    ,   m_dim(0)
    ,   m_alphaScaling(0.0f)
{
    updateData(lut);
}

InvLut1DRenderer::~InvLut1DRenderer()
{
    resetData();
}

void InvLut1DRenderer::setComponentParams(
    InvLut1DRenderer::ComponentParams & params,
    const Lut1DOpData::ComponentProperties & properties,
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
    m_tmpLutR.resize(0);
    m_tmpLutG.resize(0);
    m_tmpLutB.resize(0);
}

void InvLut1DRenderer::updateData(ConstLut1DOpDataRcPtr & lut)
{
    resetData();

    const bool hasSingleLut = lut->hasSingleLut();

    m_dim = lut->getArray().getLength();

    // Allocated temporary LUT(s)

    m_tmpLutR.resize(m_dim);
    m_tmpLutG.resize(0);
    m_tmpLutB.resize(0);

    if( !hasSingleLut )
    {
        m_tmpLutG.resize(m_dim);
        m_tmpLutB.resize(m_dim);
    }

    // Get component properties and initialize component parameters structure.
    const Lut1DOpData::ComponentProperties & redProperties = lut->getRedProperties();
    const Lut1DOpData::ComponentProperties & greenProperties = lut->getGreenProperties();
    const Lut1DOpData::ComponentProperties & blueProperties = lut->getBlueProperties();

    setComponentParams(m_paramsR, redProperties, m_tmpLutR.data(), 0.f);

    if( hasSingleLut )
    {
        // NB: All pointers refer to _tmpLutR.
        m_paramsB = m_paramsG = m_paramsR;
    }
    else
    {
        setComponentParams(m_paramsG, greenProperties, m_tmpLutG.data(), 0.f);
        setComponentParams(m_paramsB, blueProperties, m_tmpLutB.data(), 0.f);
    }

    // Fill temporary LUT.
    // Note: Since FindLutInv requires increasing arrays, if the LUT is
    // decreasing we negate the values to obtain the required sort order
    // of smallest to largest.

    const Array::Values& lutValues = lut->getArray().getValues();
    for(unsigned long i = 0; i<m_dim; ++i)
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

void InvLut1DRenderer::apply(float * rgbaBuffer, long numPixels) const
{
    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
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

InvLut1DRendererHueAdjust::InvLut1DRendererHueAdjust(ConstLut1DOpDataRcPtr & lut) 
    :  InvLut1DRenderer(lut)
{
    updateData(lut);
}

InvLut1DRendererHueAdjust::~InvLut1DRendererHueAdjust()
{
}

void InvLut1DRendererHueAdjust::apply(float * rgbaBuffer, long numPixels) const
{
    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
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

InvLut1DRendererHalfCode::InvLut1DRendererHalfCode(ConstLut1DOpDataRcPtr & lut) 
    :  InvLut1DRenderer(lut)
{
    updateData(lut);
}

InvLut1DRendererHalfCode::~InvLut1DRendererHalfCode()
{
    resetData();
}

void InvLut1DRendererHalfCode::updateData(ConstLut1DOpDataRcPtr & lut)
{
    resetData();

    const bool hasSingleLut = lut->hasSingleLut();

    m_dim = lut->getArray().getLength();

    // Allocated temporary LUT(s)

    m_tmpLutR.resize(m_dim);
    m_tmpLutG.resize(0);
    m_tmpLutB.resize(0);

    if( !hasSingleLut )
    {
        m_tmpLutG.resize(m_dim);
        m_tmpLutB.resize(m_dim);
    }

    // Get component properties and initialize component parameters structure.
    const Lut1DOpData::ComponentProperties & redProperties = lut->getRedProperties();
    const Lut1DOpData::ComponentProperties & greenProperties = lut->getGreenProperties();
    const Lut1DOpData::ComponentProperties & blueProperties = lut->getBlueProperties();

    const Array::Values & lutValues = lut->getArray().getValues();

    setComponentParams(m_paramsR, redProperties, m_tmpLutR.data(), lutValues[0]);

    if( hasSingleLut )
    {
        // NB: All pointers refer to m_tmpLutR.
        m_paramsB = m_paramsG = m_paramsR;
    }
    else
    {
        setComponentParams(m_paramsG, greenProperties, m_tmpLutG.data(), lutValues[1]);
        setComponentParams(m_paramsB, blueProperties, m_tmpLutB.data(), lutValues[2]);
    }

    // Fill temporary LUT.
    // Note: Since FindLutInv requires increasing arrays, if the LUT is
    // decreasing we negate the values to obtain the required sort order
    // of smallest to largest.
    for(unsigned long i = 0; i < 32768; ++i)     // positive half domain
    {
        m_tmpLutR[i] = redProperties.isIncreasing ? lutValues[i*3+0]: -lutValues[i*3+0];

        if( !hasSingleLut )
        {
            m_tmpLutG[i] = greenProperties.isIncreasing ? lutValues[i*3+1]: -lutValues[i*3+1];
            m_tmpLutB[i] = blueProperties.isIncreasing ? lutValues[i*3+2]: -lutValues[i*3+2];
        }
    }

    for(unsigned long i = 32768; i < 65536; ++i) // negative half domain
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
    // between adjacent entries is not constant, we cannot roll it into the
    // scale.
    m_scale = outMax;
}

void InvLut1DRendererHalfCode::apply(float * rgbaBuffer, long numPixels) const
{
    const bool redIsIncreasing = m_paramsR.flipSign > 0.f;
    const bool grnIsIncreasing = m_paramsG.flipSign > 0.f;
    const bool bluIsIncreasing = m_paramsB.flipSign > 0.f;

    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // Test the values against the bisectPoint to determine which half of
        // the float domain to do the inverse eval in.

        // Note that since the clamp of values outside the effective domain
        // happens in FindLutInvHalf, that input values < the bisectPoint
        // but > the neg effective domain will get clamped to -0 or wherever
        // the neg effective domain starts.
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

InvLut1DRendererHalfCodeHueAdjust::InvLut1DRendererHalfCodeHueAdjust(ConstLut1DOpDataRcPtr & lut) 
    :  InvLut1DRendererHalfCode(lut)
{
    updateData(lut);
}

InvLut1DRendererHalfCodeHueAdjust::~InvLut1DRendererHalfCodeHueAdjust()
{
}

void InvLut1DRendererHalfCodeHueAdjust::apply(float * rgbaBuffer, long numPixels) const
{
    const bool redIsIncreasing = m_paramsR.flipSign > 0.f;
    const bool grnIsIncreasing = m_paramsG.flipSign > 0.f;
    const bool bluIsIncreasing = m_paramsB.flipSign > 0.f;

    float * rgba = rgbaBuffer;

    for(long idx=0; idx<numPixels; ++idx)
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

OpCPURcPtr GetForwardLut1DRenderer(ConstLut1DOpDataRcPtr & lut)
{
    // NB: Unlike bit-depth, the half domain status of a LUT
    //     may not be changed.
    if (lut->isInputHalfDomain())
    {
        if (lut->getHueAdjust() == Lut1DOpData::HUE_NONE)
        {
            return std::make_shared<Lut1DRendererHalfCode>(lut);
        }
        else
        {
            return std::make_shared<Lut1DRendererHalfCodeHueAdjust>(lut);
        }
    }
    else
    {
        if (lut->getHueAdjust() == Lut1DOpData::HUE_NONE)
        {
            return std::make_shared<Lut1DRenderer>(lut);
        }
        else
        {
            return std::make_shared<Lut1DRendererHueAdjust>(lut);
        }
    }
}
}

OpCPURcPtr GetLut1DRenderer(ConstLut1DOpDataRcPtr & lut)
{
    if (lut->getDirection() == TRANSFORM_DIR_FORWARD)
    {
        return GetForwardLut1DRenderer(lut);
    }
    else
    {
        // LUT_INVERSION_FAST or LUT_INVERSION_DEFAULT
        if (lut->getInversionQuality() != LUT_INVERSION_BEST)
        {
            ConstLut1DOpDataRcPtr newLut = Lut1DOpData::MakeFastLut1DFromInverse(lut, false);

            // Render with a Lut1D renderer.
            return GetForwardLut1DRenderer(newLut);
        }
        else  // LUT_INVERSION_BEST
        {
            if (lut->isInputHalfDomain())
            {
                if (lut->getHueAdjust() == Lut1DOpData::HUE_NONE)
                {
                    return std::make_shared<InvLut1DRendererHalfCode>(lut);
                }
                else
                {
                    return std::make_shared<InvLut1DRendererHalfCodeHueAdjust>(lut);
                }
            }
            else
            {
                if (lut->getHueAdjust() == Lut1DOpData::HUE_NONE)
                {
                    return std::make_shared<InvLut1DRenderer>(lut);
                }
                else
                {
                    return std::make_shared<InvLut1DRendererHueAdjust>(lut);
                }
            }
        }
    }
}

}
OCIO_NAMESPACE_EXIT


#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "unittest.h"

OIIO_ADD_TEST(GamutMapUtil, order3_test)
{
    const float posinf = std::numeric_limits<float>::infinity();
    const float qnan = std::numeric_limits<float>::quiet_NaN();

    // { A, NaN, B } with A > B test (used to be a crash).
    {
        const float RGB[] = { 65504.f, -qnan, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 2);
        OIIO_CHECK_EQUAL(mid, 1);
        OIIO_CHECK_EQUAL(min, 0);
    }
    // Triple NaN test.
    {
    const float RGB[] = { qnan, qnan, -qnan };
    int min, mid, max;
    OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
    OIIO_CHECK_EQUAL(max, 2);
    OIIO_CHECK_EQUAL(mid, 1);
    OIIO_CHECK_EQUAL(min, 0);
    }
    // -Inf test.
    {
        const float RGB[] = { 65504.f, -posinf, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 0);
        OIIO_CHECK_EQUAL(mid, 2);
        OIIO_CHECK_EQUAL(min, 1);
    }
    // Inf test.
    {
        const float RGB[] = { 0.f, posinf, -65504.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 1);
        OIIO_CHECK_EQUAL(mid, 0);
        OIIO_CHECK_EQUAL(min, 2);
    }
    // Double Inf test.
    {
        const float RGB[] = { posinf, posinf, -65504.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 1);
        OIIO_CHECK_EQUAL(mid, 0);
        OIIO_CHECK_EQUAL(min, 2);
    }

    // Equal values.
    {
        const float RGB[] = { 0.f, 0.f, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        // In this case we only really care that they are distinct and in [0,2]
        // so this test could be changed (it is ok, but overly restrictive).
        OIIO_CHECK_EQUAL(max, 2);
        OIIO_CHECK_EQUAL(mid, 1);
        OIIO_CHECK_EQUAL(min, 0);
    }

    // Now test the six typical possibilities.
    {
        const float RGB[] = { 3.f, 2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 0);
        OIIO_CHECK_EQUAL(mid, 1);
        OIIO_CHECK_EQUAL(min, 2);
    }
    {
        const float RGB[] = { -3.f, -2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 2);
        OIIO_CHECK_EQUAL(mid, 1);
        OIIO_CHECK_EQUAL(min, 0);
    }
    {
        const float RGB[] = { -3.f, 2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 1);
        OIIO_CHECK_EQUAL(mid, 2);
        OIIO_CHECK_EQUAL(min, 0);
    }
    {
        const float RGB[] = { -0.3f, 2.f, -1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 1);
        OIIO_CHECK_EQUAL(mid, 0);
        OIIO_CHECK_EQUAL(min, 2);
    }
    {
        const float RGB[] = { 3.f, -2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 0);
        OIIO_CHECK_EQUAL(mid, 2);
        OIIO_CHECK_EQUAL(min, 1);
    }
    {
        const float RGB[] = { 3.f, -2.f, 10.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OIIO_CHECK_EQUAL(max, 2);
        OIIO_CHECK_EQUAL(mid, 0);
        OIIO_CHECK_EQUAL(min, 1);
    }

}

OIIO_ADD_TEST(Lut1DRenderer, nan_test)
{
    OCIO::Lut1DOpDataRcPtr lut =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::BIT_DEPTH_F32,
                                            OCIO::BIT_DEPTH_F32,
                                            OCIO::Lut1DOpData::LUT_STANDARD);

    lut->getArray().resize(8, 3);
    float * values = &lut->getArray().getValues()[0];

    values[0]  = 0.0f;      values[1]  = 0.0f;      values[2]  = 0.002333f;
    values[3]  = 0.0f;      values[4]  = 0.291341f; values[5]  = 0.015624f;
    values[6]  = 0.106521f; values[7]  = 0.334331f; values[8]  = 0.462431f;
    values[9]  = 0.515851f; values[10] = 0.474151f; values[11] = 0.624611f;
    values[12] = 0.658791f; values[13] = 0.527381f; values[14] = 0.685071f;
    values[15] = 0.908501f; values[16] = 0.707951f; values[17] = 0.886331f;
    values[18] = 0.926671f; values[19] = 0.846431f; values[20] = 1.0f;
    values[21] = 1.0f;      values[22] = 1.0f;      values[23] = 1.0f;

    OCIO::ConstLut1DOpDataRcPtr lutConst = lut;
    OCIO::OpCPURcPtr renderer = OCIO::GetLut1DRenderer(lutConst);

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    float pixels[16] = { qnan, 0.5f, 0.3f, -0.2f, 
                         0.5f, qnan, 0.3f, 0.2f, 
                         0.5f, 0.3f, qnan, 1.2f,
                         0.5f, 0.3f, 0.2f, qnan };

    renderer->apply(pixels, 4);

    OIIO_CHECK_CLOSE(pixels[0], values[0], 1e-7f);
    OIIO_CHECK_CLOSE(pixels[5], values[1], 1e-7f);
    OIIO_CHECK_CLOSE(pixels[10], values[2], 1e-7f);
    OIIO_CHECK_ASSERT(std::isnan(pixels[15]));

}

OIIO_ADD_TEST(Lut1DRenderer, nan_half_test)
{
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(
        OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32,
        "", OCIO::OpData::Descriptions(),
        OCIO::INTERP_LINEAR,
        OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE);

    float * values = &lut->getArray().getValues()[0];

    // Changed values for nan input.
    const int nanIdRed = 32256 * 3;
    values[nanIdRed] = -1.0f;
    values[nanIdRed + 1] = -2.0f;
    values[nanIdRed + 2] = -3.0f;

    OCIO::ConstLut1DOpDataRcPtr lutConst = lut;
    OCIO::OpCPURcPtr renderer = OCIO::GetLut1DRenderer(lutConst);

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    float pixels[16] = { qnan, 0.5f, 0.3f, -0.2f,
                         0.5f, qnan, 0.3f, 0.2f,
                         0.5f, 0.3f, qnan, 1.2f,
                         0.5f, 0.3f, 0.2f, qnan };

    renderer->apply(pixels, 4);

    OIIO_CHECK_CLOSE(pixels[0], values[nanIdRed], 1e-7f);
    OIIO_CHECK_CLOSE(pixels[5], values[nanIdRed + 1], 1e-7f);
    OIIO_CHECK_CLOSE(pixels[10], values[nanIdRed + 2], 1e-7f);
    OIIO_CHECK_ASSERT(std::isnan(pixels[15]));
}
#endif
