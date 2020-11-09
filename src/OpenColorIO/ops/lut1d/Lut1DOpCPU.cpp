// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <math.h>
#include <memory>
#include <stdint.h>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "MathUtils.h"
#include "ops/lut1d/Lut1DOpCPU.h"
#include "ops/OpTools.h"
#include "Platform.h"
#include "SSE.h"


#define L_ADJUST(val) \
    (T)((isOutInteger) ? Clamp((val)+0.5f, outMin,  outMax) : SanitizeFloat(val))


namespace OCIO_NAMESPACE
{

namespace
{

inline uint8_t GetLookupValue(const uint8_t & val)
{
    return val;
}

inline uint16_t GetLookupValue(const uint16_t & val)
{
    return val;
}

inline unsigned short GetLookupValue(const half & val)
{
    return val.bits();
}

// When instantiating all templates this case is needed.
// But it will never be used as the 32f is not a lookup case.
inline uint16_t GetLookupValue(const float & val)
{
    return (uint16_t)val;
}

template<typename InType, typename OutType>
struct LookupLut
{
    static inline OutType compute(const OutType * lutData,
                                  const InType & val)
    {
        return lutData[GetLookupValue(val)];
    }
};


template<BitDepth inBD, BitDepth outBD>
class BaseLut1DRenderer : public OpCPU
{
public:
    explicit BaseLut1DRenderer(ConstLut1DOpDataRcPtr & lut);
    BaseLut1DRenderer(ConstLut1DOpDataRcPtr & lut, BitDepth outBitDepth);
    virtual ~BaseLut1DRenderer();

    // NB: 1D LUT as Lookup table is so important for the performance
    //     that having a way to test it is critical.
    constexpr bool isLookup() const noexcept { return inBD != BIT_DEPTH_F32; }

protected:

    virtual void update(ConstLut1DOpDataRcPtr & lut);

    template<typename T>
    void updateData(ConstLut1DOpDataRcPtr & lut);

    void reset();

    template<typename T>
    void resetData();

protected:
    unsigned long m_dim = 0;

    void * m_tmpLutR = nullptr;
    void * m_tmpLutG = nullptr;
    void * m_tmpLutB = nullptr;

    float m_alphaScaling = 0.0f;

    BitDepth m_outBitDepth = BIT_DEPTH_UNKNOWN;

    float m_step = 1.0f;
    float m_dimMinusOne = 0.0f;

private:
    BaseLut1DRenderer() = delete;
    BaseLut1DRenderer(const BaseLut1DRenderer &) = delete;
    BaseLut1DRenderer & operator=(const BaseLut1DRenderer &) = delete;
};

// Structure used to keep track of the interpolation data for
// special case 16f/64k 1D LUT.
struct IndexPair
{
    unsigned short valA;
    unsigned short valB;
    float fraction;

    IndexPair()
    {
        valA = 0;
        valB = 0;
        fraction = 0.0f;
    }

    static IndexPair GetEdgeFloatValues(float fIn);
};

template<BitDepth inBD, BitDepth outBD>
class Lut1DRendererHalfCode : public BaseLut1DRenderer<inBD, outBD>
{
public:
    Lut1DRendererHalfCode() = delete;

    explicit Lut1DRendererHalfCode(ConstLut1DOpDataRcPtr & lut)
        : BaseLut1DRenderer<inBD, outBD>(lut) {}

    Lut1DRendererHalfCode(ConstLut1DOpDataRcPtr & lut, BitDepth outBitDepth)
        : BaseLut1DRenderer<inBD, outBD>(lut, outBitDepth) {}

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

template<BitDepth inBD, BitDepth outBD>
class Lut1DRenderer : public BaseLut1DRenderer<inBD, outBD>
{
public:
    Lut1DRenderer() = delete;

    explicit Lut1DRenderer(ConstLut1DOpDataRcPtr & lut) 
        : BaseLut1DRenderer<inBD, outBD>(lut) {}

    Lut1DRenderer(ConstLut1DOpDataRcPtr & lut, BitDepth outBitDepth)
        : BaseLut1DRenderer<inBD, outBD>(lut, outBitDepth) {}

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

template<BitDepth inBD, BitDepth outBD>
class Lut1DRendererHueAdjust : public Lut1DRenderer<inBD, outBD>
{
public:
    Lut1DRendererHueAdjust() = delete;

    explicit Lut1DRendererHueAdjust(ConstLut1DOpDataRcPtr & lut)
        :  Lut1DRenderer<inBD, outBD>(lut, BIT_DEPTH_F32) {} // HueAdjust needs float processing.

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

template<BitDepth inBD, BitDepth outBD>
class Lut1DRendererHalfCodeHueAdjust : public Lut1DRendererHalfCode<inBD, outBD>
{
public:
    Lut1DRendererHalfCodeHueAdjust() = delete;

    explicit Lut1DRendererHalfCodeHueAdjust(ConstLut1DOpDataRcPtr & lut)
        : Lut1DRendererHalfCode<inBD, outBD>(lut, BIT_DEPTH_F32) {} // HueAdjust needs float processing.

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

// Holds the parameters of a color component.
// Note: The structure does not own any of the pointers.
struct ComponentParams
{
    ComponentParams()
        :   lutStart(nullptr)
        ,   startOffset(0.f)
        ,   lutEnd(nullptr)
        ,   negLutStart(nullptr)
        ,   negStartOffset(0.f)
        ,   negLutEnd(nullptr)
        ,   flipSign(1.f)
        ,   bisectPoint(0.f)
    {}

    const float * lutStart;   // Copy of the pointer to start of effective lutData.
    float startOffset;        // Difference between real and effective start of lut.
    const float * lutEnd;     // Copy of the pointer to end of effective lutData.
    const float * negLutStart;// lutStart for negative part of half domain LUT.
    float negStartOffset;     // startOffset for negative part of half domain LUT.
    const float * negLutEnd;  // lutEnd for negative part of half domain LUT.
    float flipSign;           // Flip the sign of value to handle decreasing luts.
    float bisectPoint;        // Point of switching from pos to neg of half domain.

    static void setComponentParams(ComponentParams & params,
                                   const Lut1DOpData::ComponentProperties & properties,
                                   const float * lutPtr,
                                   float lutZeroEntry);
};

template<BitDepth inBD, BitDepth outBD>
class InvLut1DRenderer : public OpCPU
{
public:
    InvLut1DRenderer() = delete;
    explicit InvLut1DRenderer(ConstLut1DOpDataRcPtr & lut);
    InvLut1DRenderer(const InvLut1DRenderer&) = delete;
    InvLut1DRenderer& operator=(const InvLut1DRenderer&) = delete;
    virtual ~InvLut1DRenderer();

    void apply(const void * inImg, void * outImg, long numPixels) const override;

    void resetData();

    virtual void updateData(ConstLut1DOpDataRcPtr & lut);

protected:
    float m_scale; // Output scaling for the r, g and b components.

    ComponentParams m_paramsR;
    ComponentParams m_paramsG;
    ComponentParams m_paramsB;

    unsigned long      m_dim;
    std::vector<float> m_tmpLutR;
    std::vector<float> m_tmpLutG;
    std::vector<float> m_tmpLutB;
    float              m_alphaScaling;  // Bit-depth scale factor for alpha channel.
};

template<BitDepth inBD, BitDepth outBD>
class InvLut1DRendererHalfCode : public InvLut1DRenderer<inBD, outBD>
{
public:
    InvLut1DRendererHalfCode() = delete;
    explicit InvLut1DRendererHalfCode(ConstLut1DOpDataRcPtr & lut);
    InvLut1DRendererHalfCode(const InvLut1DRendererHalfCode &) = delete;
    InvLut1DRendererHalfCode& operator=(const InvLut1DRendererHalfCode &) = delete;
    virtual ~InvLut1DRendererHalfCode();

    void updateData(ConstLut1DOpDataRcPtr & lut) override;

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

template<BitDepth inBD, BitDepth outBD>
class InvLut1DRendererHueAdjust : public InvLut1DRenderer<inBD, outBD>
{
public:
    explicit InvLut1DRendererHueAdjust(ConstLut1DOpDataRcPtr & lut);
 
    void apply(const void * inImg, void * outImg, long numPixels) const override;
};

template<BitDepth inBD, BitDepth outBD>
class InvLut1DRendererHalfCodeHueAdjust : public InvLut1DRendererHalfCode<inBD, outBD>
{
public:
    explicit InvLut1DRendererHalfCodeHueAdjust(ConstLut1DOpDataRcPtr & lut);

    void apply(const void * inImg, void * outImg, long numPixels) const override;
};


template<BitDepth inBD, BitDepth outBD>
BaseLut1DRenderer<inBD, outBD>::BaseLut1DRenderer(ConstLut1DOpDataRcPtr & lut)
    :   OpCPU()
    ,   m_dim(lut->getArray().getLength())
    ,   m_outBitDepth(outBD)
{
    static_assert(inBD!=BIT_DEPTH_UINT32 && inBD!=BIT_DEPTH_UINT14, "Unsupported bit depth.");
    update(lut);
}

template<BitDepth inBD, BitDepth outBD>
BaseLut1DRenderer<inBD, outBD>::BaseLut1DRenderer(ConstLut1DOpDataRcPtr & lut, BitDepth outBitDepth)
    :   OpCPU()
    ,   m_dim(lut->getArray().getLength())
    ,   m_outBitDepth(outBitDepth)
{
    static_assert(inBD!=BIT_DEPTH_UINT32 && inBD!=BIT_DEPTH_UINT14, "Unsupported bit depth.");
    update(lut);
}

template<BitDepth inBD, BitDepth outBD>
void BaseLut1DRenderer<inBD, outBD>::update(ConstLut1DOpDataRcPtr & lut)
{
    switch(m_outBitDepth)
    {
        case BIT_DEPTH_UINT8:
            updateData<BitDepthInfo<BIT_DEPTH_UINT8>::Type>(lut);
            break;
        case BIT_DEPTH_UINT10:
            updateData<BitDepthInfo<BIT_DEPTH_UINT10>::Type>(lut);
            break;
        case BIT_DEPTH_UINT12:
            updateData<BitDepthInfo<BIT_DEPTH_UINT12>::Type>(lut);
            break;
        case BIT_DEPTH_UINT16:
            updateData<BitDepthInfo<BIT_DEPTH_UINT16>::Type>(lut);
            break;
        case BIT_DEPTH_F16:
            updateData<BitDepthInfo<BIT_DEPTH_F16>::Type>(lut);
            break;
        case BIT_DEPTH_F32:
            updateData<BitDepthInfo<BIT_DEPTH_F32>::Type>(lut);
            break;

        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
            break;
    }
}

template<BitDepth inBD, BitDepth outBD>
template<typename T>
void BaseLut1DRenderer<inBD, outBD>::updateData(ConstLut1DOpDataRcPtr & lut)
{
    resetData<T>();

    m_dim = lut->getArray().getLength();

    const float outMax = (float)GetBitDepthMaxValue(outBD);
    const float outMin = 0.0f;

    // (Used by L_ADJUST macro.)
    const bool isOutInteger = !IsFloatBitDepth(outBD);

    const bool mustResample = !lut->mayLookup(inBD);

    if (isLookup())
    {
        ConstLut1DOpDataRcPtr newLut = lut;

        // If we are able to lookup, need to resample the LUT based on inBitDepth.
        if(mustResample)
        {
            ConstLut1DOpDataRcPtr newLutTmp = Lut1DOpData::MakeLookupDomain(inBD);

            // Note: Compose should render at 32f, to avoid infinite recursion.
            newLut = Lut1DOpData::Compose(newLutTmp, lut,
                                          // Prevent compose from modifying newLut domain.
                                          Lut1DOpData::COMPOSE_RESAMPLE_NO);
        }

        m_dim = newLut->getArray().getLength();

        m_tmpLutR = new T[m_dim];
        m_tmpLutG = new T[m_dim];
        m_tmpLutB = new T[m_dim];

        const Array::Values & lutValues = newLut->getArray().getValues();

        // TODO: Would be faster if R, G, B were adjacent in memory?
        for(unsigned long i=0; i<m_dim; ++i)
        {
            ((T*)m_tmpLutR)[i] = L_ADJUST(lutValues[i*3+0] * outMax);
            ((T*)m_tmpLutG)[i] = L_ADJUST(lutValues[i*3+1] * outMax);
            ((T*)m_tmpLutB)[i] = L_ADJUST(lutValues[i*3+2] * outMax);
        }
    }
    else
    {
        const Array::Values & lutValues = lut->getArray().getValues();

        m_tmpLutR = new float[m_dim];
        m_tmpLutG = new float[m_dim];
        m_tmpLutB = new float[m_dim];

        for(unsigned long i=0; i<m_dim; ++i)
        {
            ((float*)m_tmpLutR)[i] = SanitizeFloat(lutValues[i*3+0] * outMax);
            ((float*)m_tmpLutG)[i] = SanitizeFloat(lutValues[i*3+1] * outMax);
            ((float*)m_tmpLutB)[i] = SanitizeFloat(lutValues[i*3+2] * outMax);
        }
    }

    m_alphaScaling = (float)GetBitDepthMaxValue(outBD)
                     / (float)GetBitDepthMaxValue(inBD);

    m_step = ((float)m_dim - 1.0f) / (float)GetBitDepthMaxValue(inBD);

    m_dimMinusOne = m_dim - 1.0f;
}

template<BitDepth inBD, BitDepth outBD>
void BaseLut1DRenderer<inBD, outBD>::reset()
{
    if(!m_tmpLutR && !m_tmpLutG && !m_tmpLutB) return;

    if (isLookup())
    {
        switch(m_outBitDepth)
        {
            case BIT_DEPTH_UINT8:
                resetData<BitDepthInfo<BIT_DEPTH_UINT8>::Type>();
                break;
            case BIT_DEPTH_UINT10:
                resetData<BitDepthInfo<BIT_DEPTH_UINT10>::Type>();
                break;
            case BIT_DEPTH_UINT12:
                resetData<BitDepthInfo<BIT_DEPTH_UINT12>::Type>();
                break;
            case BIT_DEPTH_UINT16:
                resetData<BitDepthInfo<BIT_DEPTH_UINT16>::Type>();
                break;
            case BIT_DEPTH_F16:
                resetData<BitDepthInfo<BIT_DEPTH_F16>::Type>();
                break;
            case BIT_DEPTH_F32:
                resetData<BitDepthInfo<BIT_DEPTH_F32>::Type>();
                break;

            case BIT_DEPTH_UINT14:
            case BIT_DEPTH_UINT32:
            case BIT_DEPTH_UNKNOWN:
            default:
                break;
        }
    }
    else
    {
        resetData<float>();
    }
}

template<BitDepth inBD, BitDepth outBD>
template<typename T>
void BaseLut1DRenderer<inBD, outBD>::resetData()
{
    delete [](T*)m_tmpLutR; m_tmpLutR = nullptr;
    delete [](T*)m_tmpLutG; m_tmpLutG = nullptr;
    delete [](T*)m_tmpLutB; m_tmpLutB = nullptr;
}

template<BitDepth inBD, BitDepth outBD>
BaseLut1DRenderer<inBD, outBD>::~BaseLut1DRenderer()
{
    reset();
}

template<BitDepth inBD, BitDepth outBD>
void Lut1DRendererHalfCode<inBD, outBD>::apply(const void * inImg, void * outImg, long numPixels) const
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType * in = (InType *)inImg;
    OutType * out = (OutType *)outImg;

    // Input pixel format allows lookup rather than interpolation.
    //
    // NB: The if/else is expanded at compile time based on the template args.
    //     (Should be no runtime cost.)
    if (inBD != BIT_DEPTH_F32)
    {
        const OutType * lutR = (const OutType *)this->m_tmpLutR;
        const OutType * lutG = (const OutType *)this->m_tmpLutG;
        const OutType * lutB = (const OutType *)this->m_tmpLutB;

        for (long idx=0; idx<numPixels; ++idx)
        {
            out[0] = LookupLut<InType, OutType>::compute(lutR, in[0]);
            out[1] = LookupLut<InType, OutType>::compute(lutG, in[1]);
            out[2] = LookupLut<InType, OutType>::compute(lutB, in[2]);
            out[3] = OutType(in[3] * this->m_alphaScaling);

            in  += 4;
            out += 4;
        }
    }
    else  // Need to interpolate rather than simply lookup.
    {
        const float * lutR = (const float *)this->m_tmpLutR;
        const float * lutG = (const float *)this->m_tmpLutG;
        const float * lutB = (const float *)this->m_tmpLutB;

        for(long idx=0; idx<numPixels; ++idx)
        {
            const IndexPair redInterVals   = IndexPair::GetEdgeFloatValues(in[0]);
            const IndexPair greenInterVals = IndexPair::GetEdgeFloatValues(in[1]);
            const IndexPair blueInterVals  = IndexPair::GetEdgeFloatValues(in[2]);

            // Since fraction is in the domain [0, 1), interpolate using
            // 1-fraction in order to avoid cases like -/+Inf * 0.
            out[0] = Converter<outBD>::CastValue(
                        lerpf(lutR[redInterVals.valB],
                              lutR[redInterVals.valA],
                              1.0f-redInterVals.fraction));

            out[1] = Converter<outBD>::CastValue(
                        lerpf(lutG[greenInterVals.valB],
                              lutG[greenInterVals.valA],
                              1.0f-greenInterVals.fraction));

            out[2] = Converter<outBD>::CastValue(
                        lerpf(lutB[blueInterVals.valB],
                              lutB[blueInterVals.valA],
                              1.0f-blueInterVals.fraction));

            out[3] = Converter<outBD>::CastValue(in[3] * this->m_alphaScaling);

            in  += 4;
            out += 4;
        }
    }
}

IndexPair IndexPair::GetEdgeFloatValues(float fIn)
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

    if (IsNan(idxPair.fraction)) idxPair.fraction = 0.0f;

    return idxPair;
}

template<BitDepth inBD, BitDepth outBD>
void Lut1DRenderer<inBD, outBD>::apply(const void * inImg, void * outImg, long numPixels) const
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType * in = (InType *)inImg;
    OutType * out = (OutType *)outImg;

    // Input pixel format allows lookup rather than interpolation.
    //
    // NB: The if/else is expanded at compile time based on the template args.
    //     (Should be no runtime cost.)
    if (inBD != BIT_DEPTH_F32)
    {
        const OutType * lutR = (const OutType *)this->m_tmpLutR;
        const OutType * lutG = (const OutType *)this->m_tmpLutG;
        const OutType * lutB = (const OutType *)this->m_tmpLutB;

        for(long idx=0; idx<numPixels; ++idx)
        {
            out[0] = LookupLut<InType, OutType>::compute(lutR, in[0]);
            out[1] = LookupLut<InType, OutType>::compute(lutG, in[1]);
            out[2] = LookupLut<InType, OutType>::compute(lutB, in[2]);
            out[3] = OutType(in[3] * this->m_alphaScaling);

            in  += 4;
            out += 4;
        }
    }
    else  // Need to interpolate rather than simply lookup.
    {
        const float * lutR = (const float *)this->m_tmpLutR;
        const float * lutG = (const float *)this->m_tmpLutG;
        const float * lutB = (const float *)this->m_tmpLutB;

#ifdef USE_SSE
        __m128 step = _mm_set_ps(1.0f, this->m_step, this->m_step, this->m_step);
        __m128 dimMinusOne = _mm_set1_ps(this->m_dimMinusOne);
#endif

        for(long i=0; i<numPixels; ++i)
        {
#ifdef USE_SSE
            __m128 idx
                = _mm_mul_ps(_mm_set_ps(in[3], in[2], in[1], in[0]), step);

            // _mm_max_ps => NaNs become 0
            idx = _mm_min_ps(_mm_max_ps(idx, EZERO), dimMinusOne);

            // zero < std::floor(idx) < maxIdx
            // SSE => zero < truncate(idx) < maxIdx
            //
            __m128 lIdx = _mm_cvtepi32_ps(_mm_cvttps_epi32(idx));

            // zero < std::ceil(idx) < maxIdx
            // SSE => (lowIdx (already truncated) + 1) < maxIdx
            // then clamp to prevent hIdx from falling off the end
            // of the LUT
            __m128 hIdx = _mm_min_ps(_mm_add_ps(lIdx, EONE), dimMinusOne);

            // Computing delta relative to high rather than lowIdx
            // to save computing (1-delta) below.
            __m128 d = _mm_sub_ps(hIdx, idx);

            OCIO_ALIGN(float delta[4]);   _mm_store_ps(delta, d);
            OCIO_ALIGN(float lowIdx[4]);  _mm_store_ps(lowIdx, lIdx);
            OCIO_ALIGN(float highIdx[4]); _mm_store_ps(highIdx, hIdx);
#else
            float idx[3];
            idx[0] = this->m_step * in[0];
            idx[1] = this->m_step * in[1];
            idx[2] = this->m_step * in[2];

            // NaNs become 0
            idx[0] = std::min(std::max(0.f, idx[0]), this->m_dimMinusOne);
            idx[1] = std::min(std::max(0.f, idx[1]), this->m_dimMinusOne);
            idx[2] = std::min(std::max(0.f, idx[2]), this->m_dimMinusOne);

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
            // Since fraction is in the domain [0, 1), interpolate using 1-fraction
            // in order to avoid cases like -/+Inf * 0. Therefore we never multiply by 0 and
            // thus handle the case where A or B is infinity and return infinity rather than
            // 0*Infinity (which is NaN).

            out[0] = Converter<outBD>::CastValue(
                        lerpf(lutR[(unsigned int)highIdx[0]], 
                              lutR[(unsigned int)lowIdx[0]], 
                              delta[0]));
            out[1] = Converter<outBD>::CastValue(
                        lerpf(lutG[(unsigned int)highIdx[1]],
                              lutG[(unsigned int)lowIdx[1]],
                              delta[1]));
            out[2] = Converter<outBD>::CastValue(
                        lerpf(lutB[(unsigned int)highIdx[2]], 
                              lutB[(unsigned int)lowIdx[2]],
                              delta[2]));
            out[3] = Converter<outBD>::CastValue(in[3] * this->m_alphaScaling);

            in  += 4;
            out += 4;
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

template<BitDepth inBD, BitDepth outBD>
void Lut1DRendererHalfCodeHueAdjust<inBD, outBD>::apply(const void * inImg, void * outImg, long numPixels) const
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const float * lutR = (const float *)this->m_tmpLutR;
    const float * lutG = (const float *)this->m_tmpLutG;
    const float * lutB = (const float *)this->m_tmpLutB;

    const InType * in = (InType *)inImg;
    OutType * out = (OutType *)outImg;

    // NB: The if/else is expanded at compile time based on the template args.
    // (Should be no runtime cost.)
    if (inBD != BIT_DEPTH_F32)
    {
        for(long idx=0; idx<numPixels; ++idx)
        {
            const float RGB[] = {(float)in[0], (float)in[1], (float)in[2]};

            int min, mid, max;
            GamutMapUtils::Order3( RGB, min, mid, max);

            const float orig_chroma = RGB[max] - RGB[min];
            const float hue_factor 
                = orig_chroma == 0.f  ?  0.f 
                                      :  (RGB[mid] - RGB[min]) / orig_chroma;

            float RGB2[] = {
                LookupLut<InType, float>::compute(lutR, in[0]),
                LookupLut<InType, float>::compute(lutG, in[1]),
                LookupLut<InType, float>::compute(lutB, in[2])   };

            const float new_chroma = RGB2[max] - RGB2[min];

            RGB2[mid] = hue_factor * new_chroma + RGB2[min];

            out[0] = OutType(RGB2[0]);
            out[1] = OutType(RGB2[1]);
            out[2] = OutType(RGB2[2]);
            out[3] = OutType(in[3] * this->m_alphaScaling);

            in  += 4;
            out += 4;
        }
    }
    else  // Need to interpolate rather than simply lookup.
    {
        for(long idx=0; idx<numPixels; ++idx)
        {
            const float RGB[] = {(float)in[0], (float)in[1], (float)in[2]};

            int min, mid, max;
            GamutMapUtils::Order3(RGB, min, mid, max);

            const IndexPair redInterVals   = IndexPair::GetEdgeFloatValues(RGB[0]);
            const IndexPair greenInterVals = IndexPair::GetEdgeFloatValues(RGB[1]);
            const IndexPair blueInterVals  = IndexPair::GetEdgeFloatValues(RGB[2]);

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

            out[0] = Converter<outBD>::CastValue(RGB2[0]);
            out[1] = Converter<outBD>::CastValue(RGB2[1]);
            out[2] = Converter<outBD>::CastValue(RGB2[2]);
            out[3] = Converter<outBD>::CastValue(in[3] * this->m_alphaScaling);

            in  += 4;
            out += 4;
        }
    }
}

template<BitDepth inBD, BitDepth outBD>
void Lut1DRendererHueAdjust<inBD, outBD>::apply(const void * inImg, void * outImg, long numPixels) const
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const float * lutR = (const float *)this->m_tmpLutR;
    const float * lutG = (const float *)this->m_tmpLutG;
    const float * lutB = (const float *)this->m_tmpLutB;

    const InType * in = (InType *)inImg;
    OutType * out = (OutType *)outImg;

    // NB: The if/else is expanded at compile time based on the template args.
    // (Should be no runtime cost.)
    if (inBD != BIT_DEPTH_F32)
    {
        for(long idx=0; idx<numPixels; ++idx)
        {
            const float RGB[] = {(float)in[0], (float)in[1], (float)in[2]};

            int min, mid, max;
            GamutMapUtils::Order3(RGB, min, mid, max);

            const float orig_chroma = RGB[max] - RGB[min];
            const float hue_factor
                = orig_chroma == 0.f ? 0.f
                                     : (RGB[mid] - RGB[min]) / orig_chroma;

            float RGB2[] = {
                LookupLut<InType, float>::compute(lutR, in[0]),
                LookupLut<InType, float>::compute(lutG, in[1]),
                LookupLut<InType, float>::compute(lutB, in[2])
            };

            const float new_chroma = RGB2[max] - RGB2[min];

            RGB2[mid] = hue_factor * new_chroma + RGB2[min];

            out[0] = OutType(RGB2[0]);
            out[1] = OutType(RGB2[1]);
            out[2] = OutType(RGB2[2]);
            out[3] = OutType(in[3] * this->m_alphaScaling);

            in  += 4;
            out += 4;
        }
    }
    else  // Need to interpolate rather than simply lookup.
    {
        for(long i=0; i<numPixels; ++i)
        {
            const float RGB[] = {(float)in[0], (float)in[1], (float)in[2]};

            int min, mid, max;
            GamutMapUtils::Order3( RGB, min, mid, max);

            const float orig_chroma = RGB[max] - RGB[min];
            const float hue_factor 
                = orig_chroma == 0.f ? 0.f
                                     : (RGB[mid] - RGB[min]) / orig_chroma;

#ifdef USE_SSE
            __m128 idx
                = _mm_mul_ps(_mm_set_ps(in[3],
                                        RGB[2],
                                        RGB[1],
                                        RGB[0]),
                             _mm_set_ps(1.0f,
                                        this->m_step,
                                        this->m_step,
                                        this->m_step));

            // _mm_max_ps => NaNs become 0
            idx = _mm_min_ps(_mm_max_ps(idx, EZERO),
                             _mm_set1_ps(this->m_dimMinusOne));

            // zero < std::floor(idx) < maxIdx
            // SSE => zero < truncate(idx) < maxIdx
            // then clamp to prevent hIdx from falling off the end
            // of the LUT
            __m128 lIdx = _mm_cvtepi32_ps(_mm_cvttps_epi32(idx));

            // zero < std::ceil(idx) < maxIdx
            // SSE => (lowIdx (already truncated) + 1) < maxIdx
            __m128 hIdx = _mm_min_ps(_mm_add_ps(lIdx, EONE),
                                     _mm_set1_ps(this->m_dimMinusOne));

            // Computing delta relative to high rather than lowIdx
            // to save computing (1-delta) below.
            __m128 d = _mm_sub_ps(hIdx, idx);

            OCIO_ALIGN(float delta[4]);   _mm_store_ps(delta, d);
            OCIO_ALIGN(float lowIdx[4]);  _mm_store_ps(lowIdx, lIdx);
            OCIO_ALIGN(float highIdx[4]); _mm_store_ps(highIdx, hIdx);
#else
            float idx[3];
            idx[0] = this->m_step * RGB[0];
            idx[1] = this->m_step * RGB[1];
            idx[2] = this->m_step * RGB[2];

            // NaNs become 0
            idx[0] = std::min(std::max(0.f, idx[0]), this->m_dimMinusOne);
            idx[1] = std::min(std::max(0.f, idx[1]), this->m_dimMinusOne);
            idx[2] = std::min(std::max(0.f, idx[2]), this->m_dimMinusOne);

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

            out[0] = Converter<outBD>::CastValue(RGB2[0]);
            out[1] = Converter<outBD>::CastValue(RGB2[1]);
            out[2] = Converter<outBD>::CastValue(RGB2[2]);
            out[3] = Converter<outBD>::CastValue(in[3] * this->m_alphaScaling);

            in  += 4;
            out += 4;
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

template<BitDepth inBD, BitDepth outBD>
InvLut1DRenderer<inBD, outBD>::InvLut1DRenderer(ConstLut1DOpDataRcPtr & lut) 
    :   OpCPU()
    ,   m_dim(0)
    ,   m_alphaScaling(0.0f)
{
    updateData(lut);
}

template<BitDepth inBD, BitDepth outBD>
InvLut1DRenderer<inBD, outBD>::~InvLut1DRenderer()
{
    resetData();
}

void ComponentParams::setComponentParams(ComponentParams & params,
                                         const Lut1DOpData::ComponentProperties & properties,
                                         const float * lutPtr,
                                         float lutZeroEntry)
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

template<BitDepth inBD, BitDepth outBD>
void InvLut1DRenderer<inBD, outBD>::resetData()
{
    m_tmpLutR.resize(0);
    m_tmpLutG.resize(0);
    m_tmpLutB.resize(0);
}

template<BitDepth inBD, BitDepth outBD>
void InvLut1DRenderer<inBD, outBD>::updateData(ConstLut1DOpDataRcPtr & lut)
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
    const Lut1DOpData::ComponentProperties & redProperties   = lut->getRedProperties();
    const Lut1DOpData::ComponentProperties & greenProperties = lut->getGreenProperties();
    const Lut1DOpData::ComponentProperties & blueProperties  = lut->getBlueProperties();

    ComponentParams::setComponentParams(this->m_paramsR, redProperties, m_tmpLutR.data(), 0.f);

    if( hasSingleLut )
    {
        // NB: All pointers refer to _tmpLutR.
        this->m_paramsB = this->m_paramsG = this->m_paramsR;
    }
    else
    {
        ComponentParams::setComponentParams(this->m_paramsG, greenProperties, m_tmpLutG.data(), 0.f);
        ComponentParams::setComponentParams(this->m_paramsB, blueProperties, m_tmpLutB.data(), 0.f);
    }

    // Fill temporary LUT.
    // Note: Since FindLutInv requires increasing arrays, if the LUT is
    // decreasing we negate the values to obtain the required sort order
    // of smallest to largest.

    const float lutScale = (float)GetBitDepthMaxValue(inBD);

    const Array::Values& lutValues = lut->getArray().getValues();
    for(unsigned long i = 0; i<m_dim; ++i)
    {
        m_tmpLutR[i] = redProperties.isIncreasing ? lutValues[i*3+0] * lutScale :
                                                    -lutValues[i*3+0] * lutScale;

        if(!hasSingleLut)
        {
            m_tmpLutG[i] = greenProperties.isIncreasing ? lutValues[i*3+1] * lutScale :
                                                          -lutValues[i*3+1] * lutScale;
            m_tmpLutB[i] = blueProperties.isIncreasing ? lutValues[i*3+2] * lutScale :
                                                         -lutValues[i*3+2] * lutScale;
        }
    }

    const float outMax = (float)GetBitDepthMaxValue(outBD);

    m_alphaScaling = outMax / (float)GetBitDepthMaxValue(inBD);

    // Converts from index units to inDepth units of the original LUT.
    // (Note that inDepth of the original LUT is outDepth of the inverse LUT.)
    m_scale = outMax / (float) (m_dim - 1);
}

template<BitDepth inBD, BitDepth outBD>
void InvLut1DRenderer<inBD, outBD>::apply(const void * inImg, void * outImg, long numPixels) const
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType * in = (InType *)inImg;
    OutType * out = (OutType *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // red
        out[0] = Converter<outBD>::CastValue(
                    FindLutInv(this->m_paramsR.lutStart,
                               this->m_paramsR.startOffset,
                               this->m_paramsR.lutEnd,
                               this->m_paramsR.flipSign,
                               m_scale,
                               (float)in[0]));

        // green
        out[1] = Converter<outBD>::CastValue(
                    FindLutInv(this->m_paramsG.lutStart,
                               this->m_paramsG.startOffset,
                               this->m_paramsG.lutEnd,
                               this->m_paramsG.flipSign,
                               m_scale,
                               (float)in[1]));

        // blue
        out[2] = Converter<outBD>::CastValue(
                    FindLutInv(this->m_paramsB.lutStart,
                               this->m_paramsB.startOffset,
                               this->m_paramsB.lutEnd,
                               this->m_paramsB.flipSign,
                               m_scale,
                               (float)in[2]));

        // alpha
        out[3] = Converter<outBD>::CastValue(in[3] * m_alphaScaling);

        in  += 4;
        out += 4;
    }
}

template<BitDepth inBD, BitDepth outBD>
InvLut1DRendererHueAdjust<inBD, outBD>::InvLut1DRendererHueAdjust(ConstLut1DOpDataRcPtr & lut) 
    :  InvLut1DRenderer<inBD, outBD>(lut)
{
    this->updateData(lut);
}

template<BitDepth inBD, BitDepth outBD>
void InvLut1DRendererHueAdjust<inBD, outBD>::apply(const void * inImg, void * outImg, long numPixels) const
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType * in = (InType *)inImg;
    OutType * out = (OutType *)outImg;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float RGB[] = {(float)in[0], (float)in[1], (float)in[2]};

        int min, mid, max;
        GamutMapUtils::Order3(RGB, min, mid, max);

        const float orig_chroma = RGB[max] - RGB[min];
        const float hue_factor
            = (orig_chroma == 0.f) ? 0.f
                                   : (RGB[mid] - RGB[min]) / orig_chroma;

        float RGB2[] = {
            // red
            FindLutInv(this->m_paramsR.lutStart,
                       this->m_paramsR.startOffset,
                       this->m_paramsR.lutEnd,
                       this->m_paramsR.flipSign,
                       this->m_scale,
                       RGB[0]),
            // green
            FindLutInv(this->m_paramsG.lutStart,
                       this->m_paramsG.startOffset,
                       this->m_paramsG.lutEnd,
                       this->m_paramsG.flipSign,
                       this->m_scale,
                       RGB[1]),
            // blue
            FindLutInv(this->m_paramsB.lutStart,
                       this->m_paramsB.startOffset,
                       this->m_paramsB.lutEnd,
                       this->m_paramsB.flipSign,
                       this->m_scale,
                       RGB[2])
        };

        const float new_chroma = RGB2[max] - RGB2[min];

        RGB2[mid] = hue_factor * new_chroma + RGB2[min];

        out[0] = Converter<outBD>::CastValue(RGB2[0]);
        out[1] = Converter<outBD>::CastValue(RGB2[1]);
        out[2] = Converter<outBD>::CastValue(RGB2[2]);
        out[3] = Converter<outBD>::CastValue(in[3] * this->m_alphaScaling);

        in  += 4;
        out += 4;
    }
}

template<BitDepth inBD, BitDepth outBD>
InvLut1DRendererHalfCode<inBD, outBD>::InvLut1DRendererHalfCode(ConstLut1DOpDataRcPtr & lut) 
    :  InvLut1DRenderer<inBD, outBD>(lut)
{
    this->updateData(lut);
}

template<BitDepth inBD, BitDepth outBD>
InvLut1DRendererHalfCode<inBD, outBD>::~InvLut1DRendererHalfCode()
{
    this->resetData();
}

template<BitDepth inBD, BitDepth outBD>
void InvLut1DRendererHalfCode<inBD, outBD>::updateData(ConstLut1DOpDataRcPtr & lut)
{
    this->resetData();

    const bool hasSingleLut = lut->hasSingleLut();

    this->m_dim = lut->getArray().getLength();

    // Allocated temporary LUT(s)

    this->m_tmpLutR.resize(this->m_dim);
    this->m_tmpLutG.resize(0);
    this->m_tmpLutB.resize(0);

    if( !hasSingleLut )
    {
        this->m_tmpLutG.resize(this->m_dim);
        this->m_tmpLutB.resize(this->m_dim);
    }

    // Get component properties and initialize component parameters structure.
    const Lut1DOpData::ComponentProperties & redProperties   = lut->getRedProperties();
    const Lut1DOpData::ComponentProperties & greenProperties = lut->getGreenProperties();
    const Lut1DOpData::ComponentProperties & blueProperties  = lut->getBlueProperties();

    const Array::Values & lutValues = lut->getArray().getValues();

    ComponentParams::setComponentParams(this->m_paramsR, redProperties, this->m_tmpLutR.data(), lutValues[0]);

    if( hasSingleLut )
    {
        // NB: All pointers refer to m_tmpLutR.
        this->m_paramsB = this->m_paramsG = this->m_paramsR;
    }
    else
    {
        ComponentParams::setComponentParams(this->m_paramsG, greenProperties, this->m_tmpLutG.data(), lutValues[1]);
        ComponentParams::setComponentParams(this->m_paramsB, blueProperties,  this->m_tmpLutB.data(), lutValues[2]);
    }

    const float lutScale = (float)GetBitDepthMaxValue(inBD);

    // Fill temporary LUT.
    // Note: Since FindLutInv requires increasing arrays, if the LUT is
    // decreasing we negate the values to obtain the required sort order
    // of smallest to largest.
    for(unsigned long i = 0; i < 32768; ++i)     // positive half domain
    {
        this->m_tmpLutR[i] = redProperties.isIncreasing ? lutValues[i*3+0] * lutScale :
                                                          -lutValues[i*3+0] * lutScale;

        if( !hasSingleLut )
        {
            this->m_tmpLutG[i] = greenProperties.isIncreasing ? lutValues[i*3+1] * lutScale :
                                                                -lutValues[i*3+1] * lutScale;
            this->m_tmpLutB[i] = blueProperties.isIncreasing ? lutValues[i*3+2] * lutScale :
                                                               -lutValues[i*3+2] * lutScale;
        }
    }

    for(unsigned long i = 32768; i < 65536; ++i) // negative half domain
    {
        // (Per above, the LUT must be increasing, so negative half domain is sign reversed.)
        this->m_tmpLutR[i] = redProperties.isIncreasing ? -lutValues[i*3+0] * lutScale :
                                                          lutValues[i*3+0] * lutScale;

        if( !hasSingleLut )
        {
            this->m_tmpLutG[i] = greenProperties.isIncreasing ? -lutValues[i*3+1] * lutScale :
                                                                lutValues[i*3+1] * lutScale;
            this->m_tmpLutB[i] = blueProperties.isIncreasing ? -lutValues[i*3+2] * lutScale :
                                                               lutValues[i*3+2] * lutScale;
        }
    }

    const float outMax = (float)GetBitDepthMaxValue(outBD);

    this->m_alphaScaling = outMax / (float)GetBitDepthMaxValue(inBD);

    // Note the difference for half domain LUTs, since the distance between
    // between adjacent entries is not constant, we cannot roll it into the
    // scale.
    this->m_scale = outMax;
}

template<BitDepth inBD, BitDepth outBD>
void InvLut1DRendererHalfCode<inBD, outBD>::apply(const void * inImg, void * outImg, long numPixels) const
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType * in = (InType *)inImg;
    OutType * out = (OutType *)outImg;

    const bool redIsIncreasing = this->m_paramsR.flipSign > 0.f;
    const bool grnIsIncreasing = this->m_paramsG.flipSign > 0.f;
    const bool bluIsIncreasing = this->m_paramsB.flipSign > 0.f;

    for(long idx=0; idx<numPixels; ++idx)
    {
        // Test the values against the bisectPoint to determine which half of
        // the float domain to do the inverse eval in.

        // Note that since the clamp of values outside the effective domain
        // happens in FindLutInvHalf, that input values < the bisectPoint
        // but > the neg effective domain will get clamped to -0 or wherever
        // the neg effective domain starts.
        // If this proves to be a problem, could move the clamp here instead.

        const float redIn = in[0];
        const float redOut 
            = (redIsIncreasing == (redIn >= this->m_paramsR.bisectPoint)) 
                ? FindLutInvHalf(this->m_paramsR.lutStart,
                                 this->m_paramsR.startOffset,
                                 this->m_paramsR.lutEnd,
                                 this->m_paramsR.flipSign,
                                 this->m_scale,
                                 redIn) 
                : FindLutInvHalf(this->m_paramsR.negLutStart,
                                 this->m_paramsR.negStartOffset,
                                 this->m_paramsR.negLutEnd,
                                 -this->m_paramsR.flipSign,
                                 this->m_scale,
                                 redIn);

        const float grnIn = in[1];
        const float grnOut 
            = (grnIsIncreasing == (grnIn >= this->m_paramsG.bisectPoint)) 
                ? FindLutInvHalf(this->m_paramsG.lutStart,
                                 this->m_paramsG.startOffset,
                                 this->m_paramsG.lutEnd,
                                 this->m_paramsG.flipSign,
                                 this->m_scale,
                                 grnIn) 
                : FindLutInvHalf(this->m_paramsG.negLutStart,
                                 this->m_paramsG.negStartOffset,
                                 this->m_paramsG.negLutEnd,
                                 -this->m_paramsG.flipSign,
                                 this->m_scale,
                                 grnIn);

        const float bluIn = in[2];
        const float bluOut 
            = (bluIsIncreasing == (bluIn >= this->m_paramsB.bisectPoint)) 
                ? FindLutInvHalf(this->m_paramsB.lutStart,
                                 this->m_paramsB.startOffset,
                                 this->m_paramsB.lutEnd,
                                 this->m_paramsB.flipSign,
                                 this->m_scale,
                                 bluIn)
                : FindLutInvHalf(this->m_paramsB.negLutStart,
                                 this->m_paramsB.negStartOffset,
                                 this->m_paramsB.negLutEnd,
                                 -this->m_paramsR.flipSign,
                                 this->m_scale,
                                 bluIn);

        out[0] = Converter<outBD>::CastValue(redOut);
        out[1] = Converter<outBD>::CastValue(grnOut);
        out[2] = Converter<outBD>::CastValue(bluOut);
        out[3] = Converter<outBD>::CastValue(in[3] * this->m_alphaScaling);

        in  += 4;
        out += 4;
    }
}

template<BitDepth inBD, BitDepth outBD>
InvLut1DRendererHalfCodeHueAdjust<inBD, outBD>::InvLut1DRendererHalfCodeHueAdjust(ConstLut1DOpDataRcPtr & lut) 
    :  InvLut1DRendererHalfCode<inBD, outBD>(lut)
{
    this->updateData(lut);
}

template<BitDepth inBD, BitDepth outBD>
void InvLut1DRendererHalfCodeHueAdjust<inBD, outBD>::apply(const void * inImg, void * outImg, long numPixels) const
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

    const InType * in = (InType *)inImg;
    OutType * out = (OutType *)outImg;

    const bool redIsIncreasing = this->m_paramsR.flipSign > 0.f;
    const bool grnIsIncreasing = this->m_paramsG.flipSign > 0.f;
    const bool bluIsIncreasing = this->m_paramsB.flipSign > 0.f;

    for(long idx=0; idx<numPixels; ++idx)
    {
        const float RGB[] = {(float)in[0], (float)in[1], (float)in[2]};

        int min, mid, max;
        GamutMapUtils::Order3( RGB, min, mid, max);

        const float orig_chroma = RGB[max] - RGB[min];
        const float hue_factor 
            = orig_chroma == 0.f ? 0.f
                                 : (RGB[mid] - RGB[min]) / orig_chroma;

        const float redOut 
            = (redIsIncreasing == (RGB[0] >= this->m_paramsR.bisectPoint)) 
                ? FindLutInvHalf(this->m_paramsR.lutStart,
                                 this->m_paramsR.startOffset,
                                 this->m_paramsR.lutEnd,
                                 this->m_paramsR.flipSign,
                                 this->m_scale,
                                 RGB[0])
                : FindLutInvHalf(this->m_paramsR.negLutStart,
                                 this->m_paramsR.negStartOffset,
                                 this->m_paramsR.negLutEnd,
                                 -this->m_paramsR.flipSign,
                                 this->m_scale,
                                 RGB[0]);

        const float grnOut 
            = (grnIsIncreasing == (RGB[1] >= this->m_paramsG.bisectPoint)) 
                ? FindLutInvHalf(this->m_paramsG.lutStart,
                                 this->m_paramsG.startOffset,
                                 this->m_paramsG.lutEnd,
                                 this->m_paramsG.flipSign,
                                 this->m_scale,
                                 RGB[1]) 
                : FindLutInvHalf(this->m_paramsG.negLutStart,
                                 this->m_paramsG.negStartOffset,
                                 this->m_paramsG.negLutEnd,
                                 -this->m_paramsG.flipSign,
                                 this->m_scale,
                                 RGB[1]);

        const float bluOut 
            = (bluIsIncreasing == (RGB[2] >= this->m_paramsB.bisectPoint)) 
                ? FindLutInvHalf(this->m_paramsB.lutStart,
                                 this->m_paramsB.startOffset,
                                 this->m_paramsB.lutEnd,
                                 this->m_paramsB.flipSign,
                                 this->m_scale,
                                 RGB[2]) 
                : FindLutInvHalf(this->m_paramsB.negLutStart,
                                 this->m_paramsB.negStartOffset,
                                 this->m_paramsB.negLutEnd,
                                 -this->m_paramsR.flipSign,
                                 this->m_scale,
                                 RGB[2]);

        float RGB2[] = { redOut, grnOut, bluOut };

        const float new_chroma = RGB2[max] - RGB2[min];

        RGB2[mid] = hue_factor * new_chroma + RGB2[min];

        out[0] = Converter<outBD>::CastValue(RGB2[0]);
        out[1] = Converter<outBD>::CastValue(RGB2[1]);
        out[2] = Converter<outBD>::CastValue(RGB2[2]);
        out[3] = Converter<outBD>::CastValue(in[3] * this->m_alphaScaling);

        in  += 4;
        out += 4;
    }
}

template<BitDepth inBD, BitDepth outBD>
OpCPURcPtr GetForwardLut1DRenderer(ConstLut1DOpDataRcPtr & lut)
{
    // NB: Unlike bit-depth, the half domain status of a LUT
    //     may not be changed.
    if (lut->isInputHalfDomain())
    {
        if (lut->getHueAdjust() == HUE_NONE)
        {
            return std::make_shared< Lut1DRendererHalfCode<inBD, outBD> >(lut);
        }
        else
        {
            return std::make_shared< Lut1DRendererHalfCodeHueAdjust<inBD, outBD> >(lut);
        }
    }
    else
    {
        if (lut->getHueAdjust() == HUE_NONE)
        {
            return std::make_shared< Lut1DRenderer<inBD, outBD> >(lut);
        }
        else
        {
            return std::make_shared< Lut1DRendererHueAdjust<inBD, outBD> >(lut);
        }
    }
}

}

template<BitDepth inBD, BitDepth outBD>
ConstOpCPURcPtr GetLut1DRenderer_OutBitDepth(ConstLut1DOpDataRcPtr & lut)
{
    switch (lut->getDirection())
    {
    case TRANSFORM_DIR_FORWARD:
    {
        return GetForwardLut1DRenderer<inBD, outBD>(lut);
        break;
    }
    case TRANSFORM_DIR_INVERSE:
    {
        if (lut->isInputHalfDomain())
        {
            if (lut->getHueAdjust() == HUE_NONE)
            {
                return std::make_shared< InvLut1DRendererHalfCode<inBD, outBD> >(lut);
            }
            else
            {
                return std::make_shared< InvLut1DRendererHalfCodeHueAdjust<inBD, outBD> >(lut);
            }
        }
        else
        {
            if (lut->getHueAdjust() == HUE_NONE)
            {
                return std::make_shared< InvLut1DRenderer<inBD, outBD> >(lut);
            }
            else
            {
                return std::make_shared< InvLut1DRendererHueAdjust<inBD, outBD> >(lut);
            }
        }
        break;
    }
    }
    throw Exception("Illegal LUT1D direction.");
}

template<BitDepth inBD>
ConstOpCPURcPtr GetLut1DRenderer_InBitDepth(ConstLut1DOpDataRcPtr & lut, BitDepth outBD)
{
    switch(outBD)
    {
        case BIT_DEPTH_UINT8:
            return GetLut1DRenderer_OutBitDepth<inBD, BIT_DEPTH_UINT8>(lut); break;
        case BIT_DEPTH_UINT10:
            return GetLut1DRenderer_OutBitDepth<inBD, BIT_DEPTH_UINT10>(lut); break;
        case BIT_DEPTH_UINT12:
            return GetLut1DRenderer_OutBitDepth<inBD, BIT_DEPTH_UINT12>(lut); break;
        case BIT_DEPTH_UINT16:
            return GetLut1DRenderer_OutBitDepth<inBD, BIT_DEPTH_UINT16>(lut); break;
        case BIT_DEPTH_F16:
            return GetLut1DRenderer_OutBitDepth<inBD, BIT_DEPTH_F16>(lut); break;
        case BIT_DEPTH_F32:
            return GetLut1DRenderer_OutBitDepth<inBD, BIT_DEPTH_F32>(lut); break;

        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
            break;
    }

    throw Exception("Unsupported output bit depth");
    return ConstOpCPURcPtr();
}

ConstOpCPURcPtr GetLut1DRenderer(ConstLut1DOpDataRcPtr & lut, BitDepth inBD, BitDepth outBD)
{
    switch(inBD)
    {
        case BIT_DEPTH_UINT8:
            return GetLut1DRenderer_InBitDepth<BIT_DEPTH_UINT8>(lut, outBD); break;
        case BIT_DEPTH_UINT10:
            return GetLut1DRenderer_InBitDepth<BIT_DEPTH_UINT10>(lut, outBD); break;
        case BIT_DEPTH_UINT12:
            return GetLut1DRenderer_InBitDepth<BIT_DEPTH_UINT12>(lut, outBD); break;
        case BIT_DEPTH_UINT16:
            return GetLut1DRenderer_InBitDepth<BIT_DEPTH_UINT16>(lut, outBD); break;
        case BIT_DEPTH_F16:
            return GetLut1DRenderer_InBitDepth<BIT_DEPTH_F16>(lut, outBD); break;
        case BIT_DEPTH_F32:
            return GetLut1DRenderer_InBitDepth<BIT_DEPTH_F32>(lut, outBD); break;

        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
            break;
    }

    throw Exception("Unsupported input bit depth");
    return ConstOpCPURcPtr();
}


} // namespace OCIO_NAMESPACE

