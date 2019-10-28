// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
    (T)((isOutInteger) ? Clamp((val)+0.5f, outMin,  outMax) : SanitizeFloat(val))


OCIO_NAMESPACE_ENTER
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
            Lut1DOpDataRcPtr newLutTmp = Lut1DOpData::MakeLookupDomain(inBD);

            // Note: Compose should render at 32f, to avoid infinite recursion.
            Lut1DOpData::Compose(newLutTmp,
                                 lut,
                                 // Prevent compose from modifying newLut domain.
                                 Lut1DOpData::COMPOSE_RESAMPLE_NO);
            newLut = newLutTmp;
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
    if (lut->getDirection() == TRANSFORM_DIR_FORWARD)
    {
        return GetForwardLut1DRenderer<inBD, outBD>(lut);
    }
    else
    {
        if (lut->getConcreteInversionQuality() == LUT_INVERSION_FAST)
        {
            ConstLut1DOpDataRcPtr newLut = Lut1DOpData::MakeFastLut1DFromInverse(lut, false);

            // Render with a Lut1D renderer.
            return GetForwardLut1DRenderer<inBD, outBD>(newLut);
        }
        else  // INV_EXACT
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
        }
    }
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


}
OCIO_NAMESPACE_EXIT




#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "UnitTest.h"
#include "UnitTestUtils.h"

OCIO_ADD_TEST(GamutMapUtil, order3_test)
{
    const float posinf = std::numeric_limits<float>::infinity();
    const float qnan = std::numeric_limits<float>::quiet_NaN();

    // { A, NaN, B } with A > B test (used to be a crash).
    {
        const float RGB[] = { 65504.f, -qnan, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 2);
        OCIO_CHECK_EQUAL(mid, 1);
        OCIO_CHECK_EQUAL(min, 0);
    }
    // Triple NaN test.
    {
    const float RGB[] = { qnan, qnan, -qnan };
    int min, mid, max;
    OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
    OCIO_CHECK_EQUAL(max, 2);
    OCIO_CHECK_EQUAL(mid, 1);
    OCIO_CHECK_EQUAL(min, 0);
    }
    // -Inf test.
    {
        const float RGB[] = { 65504.f, -posinf, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 0);
        OCIO_CHECK_EQUAL(mid, 2);
        OCIO_CHECK_EQUAL(min, 1);
    }
    // Inf test.
    {
        const float RGB[] = { 0.f, posinf, -65504.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 1);
        OCIO_CHECK_EQUAL(mid, 0);
        OCIO_CHECK_EQUAL(min, 2);
    }
    // Double Inf test.
    {
        const float RGB[] = { posinf, posinf, -65504.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 1);
        OCIO_CHECK_EQUAL(mid, 0);
        OCIO_CHECK_EQUAL(min, 2);
    }

    // Equal values.
    {
        const float RGB[] = { 0.f, 0.f, 0.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        // In this case we only really care that they are distinct and in [0,2]
        // so this test could be changed (it is ok, but overly restrictive).
        OCIO_CHECK_EQUAL(max, 2);
        OCIO_CHECK_EQUAL(mid, 1);
        OCIO_CHECK_EQUAL(min, 0);
    }

    // Now test the six typical possibilities.
    {
        const float RGB[] = { 3.f, 2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 0);
        OCIO_CHECK_EQUAL(mid, 1);
        OCIO_CHECK_EQUAL(min, 2);
    }
    {
        const float RGB[] = { -3.f, -2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 2);
        OCIO_CHECK_EQUAL(mid, 1);
        OCIO_CHECK_EQUAL(min, 0);
    }
    {
        const float RGB[] = { -3.f, 2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 1);
        OCIO_CHECK_EQUAL(mid, 2);
        OCIO_CHECK_EQUAL(min, 0);
    }
    {
        const float RGB[] = { -0.3f, 2.f, -1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 1);
        OCIO_CHECK_EQUAL(mid, 0);
        OCIO_CHECK_EQUAL(min, 2);
    }
    {
        const float RGB[] = { 3.f, -2.f, 1.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 0);
        OCIO_CHECK_EQUAL(mid, 2);
        OCIO_CHECK_EQUAL(min, 1);
    }
    {
        const float RGB[] = { 3.f, -2.f, 10.f };
        int min, mid, max;
        OCIO::GamutMapUtils::Order3(RGB, min, mid, max);
        OCIO_CHECK_EQUAL(max, 2);
        OCIO_CHECK_EQUAL(mid, 0);
        OCIO_CHECK_EQUAL(min, 1);
    }

}

OCIO_ADD_TEST(Lut1DRenderer, nan_test)
{
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(8);

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
    OCIO::ConstOpCPURcPtr renderer;
    OCIO_CHECK_NO_THROW(renderer = OCIO::GetLut1DRenderer(lutConst,
                                                          OCIO::BIT_DEPTH_F32,
                                                          OCIO::BIT_DEPTH_F32));

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();

    float pixels[24] = { qnan, 0.5f, 0.3f, -0.2f,
                         0.5f, qnan, 0.3f, 0.2f, 
                         0.5f, 0.3f, qnan, 1.2f,
                         0.5f, 0.3f, 0.2f, qnan,
                         inf,  inf,  inf,  inf,
                         -inf, -inf, -inf, -inf };

    renderer->apply(pixels, pixels, 6);

    OCIO_CHECK_CLOSE(pixels[0], values[0], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[5], values[1], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[10], values[2], 1e-7f);
    OCIO_CHECK_ASSERT(OCIO::IsNan(pixels[15]));
    OCIO_CHECK_CLOSE(pixels[16], values[21], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[17], values[22], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[18], values[23], 1e-7f);
    OCIO_CHECK_EQUAL(pixels[19], inf);
    OCIO_CHECK_CLOSE(pixels[20], values[0], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[21], values[1], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[22], values[2], 1e-7f);
    OCIO_CHECK_EQUAL(pixels[23], -inf);
}

OCIO_ADD_TEST(Lut1DRenderer, nan_half_test)
{
    OCIO::Lut1DOpDataRcPtr lut = std::make_shared<OCIO::Lut1DOpData>(
        OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE, 65536);

    float * values = &lut->getArray().getValues()[0];

    // Changed values for nan input.
    constexpr int nanIdRed = 32256 * 3;
    values[nanIdRed] = -1.0f;
    values[nanIdRed + 1] = -2.0f;
    values[nanIdRed + 2] = -3.0f;

    OCIO::ConstLut1DOpDataRcPtr lutConst = lut;
    OCIO::ConstOpCPURcPtr renderer;
    OCIO_CHECK_NO_THROW(renderer = OCIO::GetLut1DRenderer(lutConst,
                                                          OCIO::BIT_DEPTH_F32,
                                                          OCIO::BIT_DEPTH_F32));

    const float qnan = std::numeric_limits<float>::quiet_NaN();
    float pixels[16] = { qnan, 0.5f, 0.3f, -0.2f,
                         0.5f, qnan, 0.3f, 0.2f,
                         0.5f, 0.3f, qnan, 1.2f,
                         0.5f, 0.3f, 0.2f, qnan };

    renderer->apply(pixels, pixels, 4);

    // This verifies that a half-domain Lut1D can map NaNs to whatever the LUT author wants.
    // In this test, a different value for R, G, and B.

    OCIO_CHECK_CLOSE(pixels[0], values[nanIdRed], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[5], values[nanIdRed + 1], 1e-7f);
    OCIO_CHECK_CLOSE(pixels[10], values[nanIdRed + 2], 1e-7f);
    OCIO_CHECK_ASSERT(OCIO::IsNan(pixels[15]));
}

OCIO_ADD_TEST(Lut1DRenderer, bit_depth_support)
{
    // Unit test to validate the pixel bit depth processing with the 1D LUT.

    // Note: Copy & paste of logtolin_8to8.lut

    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(256);

    OCIO::Array::Values & vals = lutData->getArray().getValues();

    static const std::vector<float> lutValues = { 
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         0.0f,          0.0f,          0.0f,
         1.0f,          1.0f,          1.0f,
         1.0f,          1.0f,          1.0f,
         2.0f,          2.0f,          2.0f,
         2.0f,          2.0f,          2.0f,
         3.0f,          3.0f,          3.0f,
         3.0f,          3.0f,          3.0f,
         4.0f,          4.0f,          4.0f,
         5.0f,          5.0f,          5.0f,
         5.0f,          5.0f,          5.0f,
         6.0f,          6.0f,          6.0f,
         6.0f,          6.0f,          6.0f,
         7.0f,          7.0f,          7.0f,
         8.0f,          8.0f,          8.0f,
         8.0f,          8.0f,          8.0f,
         9.0f,          9.0f,          9.0f,
        10.0f,         10.0f,         10.0f,
        10.0f,         10.0f,         10.0f,
        11.0f,         11.0f,         11.0f,
        12.0f,         12.0f,         12.0f,
        12.0f,         12.0f,         12.0f,
        13.0f,         13.0f,         13.0f,
        14.0f,         14.0f,         14.0f,
        15.0f,         15.0f,         15.0f,
        15.0f,         15.0f,         15.0f,
        16.0f,         16.0f,         16.0f,
        17.0f,         17.0f,         17.0f,
        18.0f,         18.0f,         18.0f,
        18.0f,         18.0f,         18.0f,
        19.0f,         19.0f,         19.0f,
        20.0f,         20.0f,         20.0f,
        21.0f,         21.0f,         21.0f,
        22.0f,         22.0f,         22.0f,
        22.0f,         22.0f,         22.0f,
        23.0f,         23.0f,         23.0f,
        24.0f,         24.0f,         24.0f,
        25.0f,         25.0f,         25.0f,
        26.0f,         26.0f,         26.0f,
        27.0f,         27.0f,         27.0f,
        28.0f,         28.0f,         28.0f,
        29.0f,         29.0f,         29.0f,
        30.0f,         30.0f,         30.0f,
        30.0f,         30.0f,         30.0f,
        31.0f,         31.0f,         31.0f,
        32.0f,         32.0f,         32.0f,
        33.0f,         33.0f,         33.0f,
        34.0f,         34.0f,         34.0f,
        35.0f,         35.0f,         35.0f,
        36.0f,         36.0f,         36.0f,
        37.0f,         37.0f,         37.0f,
        39.0f,         39.0f,         39.0f,
        40.0f,         40.0f,         40.0f,
        41.0f,         41.0f,         41.0f,
        42.0f,         42.0f,         42.0f,
        43.0f,         43.0f,         43.0f,
        44.0f,         44.0f,         44.0f,
        45.0f,         45.0f,         45.0f,
        46.0f,         46.0f,         46.0f,
        48.0f,         48.0f,         48.0f,
        49.0f,         49.0f,         49.0f,
        50.0f,         50.0f,         50.0f,
        51.0f,         51.0f,         51.0f,
        52.0f,         52.0f,         52.0f,
        54.0f,         54.0f,         54.0f,
        55.0f,         55.0f,         55.0f,
        56.0f,         56.0f,         56.0f,
        58.0f,         58.0f,         58.0f,
        59.0f,         59.0f,         59.0f,
        60.0f,         60.0f,         60.0f,
        62.0f,         62.0f,         62.0f,
        63.0f,         63.0f,         63.0f,
        64.0f,         64.0f,         64.0f,
        66.0f,         66.0f,         66.0f,
        67.0f,         67.0f,         67.0f,
        69.0f,         69.0f,         69.0f,
        70.0f,         70.0f,         70.0f,
        72.0f,         72.0f,         72.0f,
        73.0f,         73.0f,         73.0f,
        75.0f,         75.0f,         75.0f,
        76.0f,         76.0f,         76.0f,
        78.0f,         78.0f,         78.0f,
        80.0f,         80.0f,         80.0f,
        81.0f,         81.0f,         81.0f,
        83.0f,         83.0f,         83.0f,
        85.0f,         85.0f,         85.0f,
        86.0f,         86.0f,         86.0f,
        88.0f,         88.0f,         88.0f,
        90.0f,         90.0f,         90.0f,
        92.0f,         92.0f,         92.0f,
        94.0f,         94.0f,         94.0f,
        95.0f,         95.0f,         95.0f,
        97.0f,         97.0f,         97.0f,
        99.0f,         99.0f,         99.0f,
       101.0f,        101.0f,        101.0f,
       103.0f,        103.0f,        103.0f,
       105.0f,        105.0f,        105.0f,
       107.0f,        107.0f,        107.0f,
       109.0f,        109.0f,        109.0f,
       111.0f,        111.0f,        111.0f,
       113.0f,        113.0f,        113.0f,
       115.0f,        115.0f,        115.0f,
       117.0f,        117.0f,        117.0f,
       120.0f,        120.0f,        120.0f,
       122.0f,        122.0f,        122.0f,
       124.0f,        124.0f,        124.0f,
       126.0f,        126.0f,        126.0f,
       129.0f,        129.0f,        129.0f,
       131.0f,        131.0f,        131.0f,
       133.0f,        133.0f,        133.0f,
       136.0f,        136.0f,        136.0f,
       138.0f,        138.0f,        138.0f,
       140.0f,        140.0f,        140.0f,
       143.0f,        143.0f,        143.0f,
       145.0f,        145.0f,        145.0f,
       148.0f,        148.0f,        148.0f,
       151.0f,        151.0f,        151.0f,
       153.0f,        153.0f,        153.0f,
       156.0f,        156.0f,        156.0f,
       159.0f,        159.0f,        159.0f,
       161.0f,        161.0f,        161.0f,
       164.0f,        164.0f,        164.0f,
       167.0f,        167.0f,        167.0f,
       170.0f,        170.0f,        170.0f,
       173.0f,        173.0f,        173.0f,
       176.0f,        176.0f,        176.0f,
       179.0f,        179.0f,        179.0f,
       182.0f,        182.0f,        182.0f,
       185.0f,        185.0f,        185.0f,
       188.0f,        188.0f,        188.0f,
       191.0f,        191.0f,        191.0f,
       194.0f,        194.0f,        194.0f,
       198.0f,        198.0f,        198.0f,
       201.0f,        201.0f,        201.0f,
       204.0f,        204.0f,        204.0f,
       208.0f,        208.0f,        208.0f,
       211.0f,        211.0f,        211.0f,
       214.0f,        214.0f,        214.0f,
       218.0f,        218.0f,        218.0f,
       222.0f,        222.0f,        222.0f,
       225.0f,        225.0f,        225.0f,
       229.0f,        229.0f,        229.0f,
       233.0f,        233.0f,        233.0f,
       236.0f,        236.0f,        236.0f,
       240.0f,        240.0f,        240.0f,
       244.0f,        244.0f,        244.0f,
       248.0f,        248.0f,        248.0f,
       252.0f,        252.0f,        252.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f,
       255.0f,        255.0f,        255.0f   };

    vals = lutValues;
    lutData->getArray().scale(1.0f / 255.0f);
    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;

    constexpr unsigned NB_PIXELS = 4;

    const std::vector<uint8_t> uint8_inImg = { 
          0,   1,   2,   0,
         50,  51,  52, 255,
        150, 151, 152,   0,
        230, 240, 250, 255  };

    const std::vector<uint16_t> uint16_outImg = {
            0,     0,     0,     0,
         4369,  4626,  4626, 65535,
        46774, 47545, 48316,     0,
        65535, 65535, 65535, 65535 };

    // Processing from UINT8 to UINT8.
    {
        OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_UINT8,
                                                           OCIO::BIT_DEPTH_UINT8));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_UINT8,
                                                                     OCIO::BIT_DEPTH_UINT8>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(isLookup);

        std::vector<uint8_t> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&uint8_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0],   0);
        OCIO_CHECK_EQUAL(outImg[ 1],   0);
        OCIO_CHECK_EQUAL(outImg[ 2],   0);
        OCIO_CHECK_EQUAL(outImg[ 3],   0);

        OCIO_CHECK_EQUAL(outImg[ 4],  17);
        OCIO_CHECK_EQUAL(outImg[ 5],  18);
        OCIO_CHECK_EQUAL(outImg[ 6],  18);
        OCIO_CHECK_EQUAL(outImg[ 7], 255);

        OCIO_CHECK_EQUAL(outImg[ 8], 182);
        OCIO_CHECK_EQUAL(outImg[ 9], 185);
        OCIO_CHECK_EQUAL(outImg[10], 188);
        OCIO_CHECK_EQUAL(outImg[11],   0);

        OCIO_CHECK_EQUAL(outImg[12], 255);
        OCIO_CHECK_EQUAL(outImg[13], 255);
        OCIO_CHECK_EQUAL(outImg[14], 255);
        OCIO_CHECK_EQUAL(outImg[15], 255);
    }

    // Processing from UINT8 to UINT8, using the inverse LUT.
    {
        OCIO::Lut1DOpDataRcPtr lutInvData = lutData->inverse();
        OCIO::ConstLut1DOpDataRcPtr constInvLut = lutInvData;

        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_UINT8,
                                                           OCIO::BIT_DEPTH_UINT8));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_UINT8,
                                                                     OCIO::BIT_DEPTH_UINT8>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(isLookup);

        std::vector<uint8_t> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&uint8_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0],  24);
        OCIO_CHECK_EQUAL(outImg[ 1],  25);
        OCIO_CHECK_EQUAL(outImg[ 2],  27);
        OCIO_CHECK_EQUAL(outImg[ 3],   0);

        OCIO_CHECK_EQUAL(outImg[ 4],  84);
        OCIO_CHECK_EQUAL(outImg[ 5],  85);
        OCIO_CHECK_EQUAL(outImg[ 6],  86);
        OCIO_CHECK_EQUAL(outImg[ 7], 255);

        OCIO_CHECK_EQUAL(outImg[ 8], 139);
        OCIO_CHECK_EQUAL(outImg[ 9], 139);
        OCIO_CHECK_EQUAL(outImg[10], 140);
        OCIO_CHECK_EQUAL(outImg[11],   0);

        OCIO_CHECK_EQUAL(outImg[12], 164);
        OCIO_CHECK_EQUAL(outImg[13], 167);
        OCIO_CHECK_EQUAL(outImg[14], 170);
        OCIO_CHECK_EQUAL(outImg[15], 255);
    }

    // Processing from UINT8 to UINT16.
    {
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_UINT8,
                                                           OCIO::BIT_DEPTH_UINT16));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_UINT8,
                                                                     OCIO::BIT_DEPTH_UINT16>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(isLookup);

        std::vector<uint16_t> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&uint8_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0], uint16_outImg[ 0]);
        OCIO_CHECK_EQUAL(outImg[ 1], uint16_outImg[ 1]);
        OCIO_CHECK_EQUAL(outImg[ 2], uint16_outImg[ 2]);
        OCIO_CHECK_EQUAL(outImg[ 3], uint16_outImg[ 3]);

        OCIO_CHECK_EQUAL(outImg[ 4], uint16_outImg[ 4]);
        OCIO_CHECK_EQUAL(outImg[ 5], uint16_outImg[ 5]);
        OCIO_CHECK_EQUAL(outImg[ 6], uint16_outImg[ 6]);
        OCIO_CHECK_EQUAL(outImg[ 7], uint16_outImg[ 7]);

        OCIO_CHECK_EQUAL(outImg[ 8], uint16_outImg[ 8]);
        OCIO_CHECK_EQUAL(outImg[ 9], uint16_outImg[ 9]);
        OCIO_CHECK_EQUAL(outImg[10], uint16_outImg[10]);
        OCIO_CHECK_EQUAL(outImg[11], uint16_outImg[11]);

        OCIO_CHECK_EQUAL(outImg[12], uint16_outImg[12]);
        OCIO_CHECK_EQUAL(outImg[13], uint16_outImg[13]);
        OCIO_CHECK_EQUAL(outImg[14], uint16_outImg[14]);
        OCIO_CHECK_EQUAL(outImg[15], uint16_outImg[15]);
    }

    // Processing from UINT8 to F16.
    {
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_UINT8,
                                                           OCIO::BIT_DEPTH_F16));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_UINT8,
                                                                     OCIO::BIT_DEPTH_F16>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(isLookup);

        std::vector<half> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&uint8_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 1], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 2], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 3], 0.0f);

        OCIO_CHECK_CLOSE(outImg[ 4], 0.066650390625f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 5], 0.070617675781f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 6], 0.070617675781f, 1e-6f);
        OCIO_CHECK_EQUAL(outImg[ 7], 1.0f);

        OCIO_CHECK_CLOSE(outImg[ 8], 0.7138671875f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 9], 0.7255859375f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[10], 0.7373046875f, 1e-6f);
        OCIO_CHECK_EQUAL(outImg[11], 0.0f);

        OCIO_CHECK_EQUAL(outImg[12], 1.0f);
        OCIO_CHECK_EQUAL(outImg[13], 1.0f);
        OCIO_CHECK_EQUAL(outImg[14], 1.0f);
        OCIO_CHECK_EQUAL(outImg[15], 1.0f);
    }

    // Processing from UINT8 to F32.
    {
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_UINT8,
                                                           OCIO::BIT_DEPTH_F32));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_UINT8,
                                                                     OCIO::BIT_DEPTH_F32>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(isLookup);

        std::vector<float> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&uint8_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 1], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 2], 0.0f);
        OCIO_CHECK_EQUAL(outImg[ 3], 0.0f);

        OCIO_CHECK_CLOSE(outImg[ 4], 0.06666666666666667f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 5], 0.07058823529411765f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 6], 0.07058823529411765f, 1e-6f);
        OCIO_CHECK_EQUAL(outImg[ 7], 1.0f);

        OCIO_CHECK_CLOSE(outImg[ 8], 0.7137254901960784f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[ 9], 0.7254901960784313f, 1e-6f);
        OCIO_CHECK_CLOSE(outImg[10], 0.7372549019607844f, 1e-6f);
        OCIO_CHECK_EQUAL(outImg[11], 0.0f);

        OCIO_CHECK_EQUAL(outImg[12], 1.0f);
        OCIO_CHECK_EQUAL(outImg[13], 1.0f);
        OCIO_CHECK_EQUAL(outImg[14], 1.0f);
        OCIO_CHECK_EQUAL(outImg[15], 1.0f);
    }

    // Use scaled previous input values so previous output values could be 
    // reused (i.e. uint16_outImg) to validate the pixel bit depth processing.

    const std::vector<float> float_inImg = { 
        uint8_inImg[ 0]/255.0f, uint8_inImg[ 1]/255.0f, uint8_inImg[ 2]/255.0f, uint8_inImg[ 3]/255.0f,
        uint8_inImg[ 4]/255.0f, uint8_inImg[ 5]/255.0f, uint8_inImg[ 6]/255.0f, uint8_inImg[ 7]/255.0f,
        uint8_inImg[ 8]/255.0f, uint8_inImg[ 9]/255.0f, uint8_inImg[10]/255.0f, uint8_inImg[11]/255.0f,
        uint8_inImg[12]/255.0f, uint8_inImg[13]/255.0f, uint8_inImg[14]/255.0f, uint8_inImg[15]/255.0f };

    // LUT will be used for interpolation, not look-up.
    {
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_UINT8));

        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_F32,
                                                                     OCIO::BIT_DEPTH_UINT8>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(!isLookup);

        std::vector<uint8_t> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&float_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0],   0);
        OCIO_CHECK_EQUAL(outImg[ 1],   0);
        OCIO_CHECK_EQUAL(outImg[ 2],   0);
        OCIO_CHECK_EQUAL(outImg[ 3],   0);

        OCIO_CHECK_EQUAL(outImg[ 4],  17);
        OCIO_CHECK_EQUAL(outImg[ 5],  18);
        OCIO_CHECK_EQUAL(outImg[ 6],  18);
        OCIO_CHECK_EQUAL(outImg[ 7], 255);

        OCIO_CHECK_EQUAL(outImg[ 8], 182);
        OCIO_CHECK_EQUAL(outImg[ 9], 185);
        OCIO_CHECK_EQUAL(outImg[10], 188);
        OCIO_CHECK_EQUAL(outImg[11],   0);

        OCIO_CHECK_EQUAL(outImg[12], 255);
        OCIO_CHECK_EQUAL(outImg[13], 255);
        OCIO_CHECK_EQUAL(outImg[14], 255);
        OCIO_CHECK_EQUAL(outImg[15], 255);
    }

    // LUT will be used for interpolation, not look-up.
    {
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_UINT16));
        auto op = OCIO::DynamicPtrCast<const OCIO::BaseLut1DRenderer<OCIO::BIT_DEPTH_F32,
                                                                     OCIO::BIT_DEPTH_UINT16>>(cpuOp);
        OCIO_REQUIRE_ASSERT(op);
        const bool isLookup = op->isLookup();
        OCIO_CHECK_ASSERT(!isLookup);

        std::vector<uint16_t> outImg(NB_PIXELS * 4, 0);

        cpuOp->apply(&float_inImg[0], &outImg[0], NB_PIXELS);

        OCIO_CHECK_EQUAL(outImg[ 0], uint16_outImg[ 0]);
        OCIO_CHECK_EQUAL(outImg[ 1], uint16_outImg[ 1]);
        OCIO_CHECK_EQUAL(outImg[ 2], uint16_outImg[ 2]);
        OCIO_CHECK_EQUAL(outImg[ 3], uint16_outImg[ 3]);

        OCIO_CHECK_EQUAL(outImg[ 4], uint16_outImg[ 4]);
        OCIO_CHECK_EQUAL(outImg[ 5], uint16_outImg[ 5]);
        OCIO_CHECK_EQUAL(outImg[ 6], uint16_outImg[ 6]);
        OCIO_CHECK_EQUAL(outImg[ 7], uint16_outImg[ 7]);

        OCIO_CHECK_EQUAL(outImg[ 8], uint16_outImg[ 8]);
        OCIO_CHECK_EQUAL(outImg[ 9], uint16_outImg[ 9]);
        OCIO_CHECK_EQUAL(outImg[10], uint16_outImg[10]);
        OCIO_CHECK_EQUAL(outImg[11], uint16_outImg[11]);

        OCIO_CHECK_EQUAL(outImg[12], uint16_outImg[12]);
        OCIO_CHECK_EQUAL(outImg[13], uint16_outImg[13]);
        OCIO_CHECK_EQUAL(outImg[14], uint16_outImg[14]);
        OCIO_CHECK_EQUAL(outImg[15], uint16_outImg[15]);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, basic)
{
    // By default, this constructor creates an 'identity LUT'.
    OCIO::Lut1DOpDataRcPtr lutData =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 65536);

    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);

    OCIO_CHECK_NO_THROW(lutData->finalize());

    const float step = 1.f / ((float)lutData->getArray().getLength() - 1.0f);

    const float inImg[8] = {
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, step, 1.0f };

    const float error = 1e-6f;
    {
        OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32));

        std::vector<float> outImg(2 * 4, 1);
        cpuOp->apply(&inImg[0], &outImg[0], 2);

        OCIO_CHECK_CLOSE(outImg[0], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[1], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[2], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[3], 1.0f, error);

        OCIO_CHECK_CLOSE(outImg[4], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[5], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[6], step, error);
        OCIO_CHECK_CLOSE(outImg[7], 1.0f, error);
    }

    // No more an 'identity LUT 1D'.
    const float arbitraryVal = 0.123456f;

    lutData->getArray()[5] = arbitraryVal;

    OCIO_CHECK_NO_THROW(lutData->finalize());
    OCIO_CHECK_ASSERT(!lutData->isIdentity());
    {
        OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32));

        std::vector<float> outImg(2 * 4, 1);
        cpuOp->apply(&inImg[0], &outImg[0], 2);

        OCIO_CHECK_CLOSE(outImg[0], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[1], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[2], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[3], 1.0f, error);

        OCIO_CHECK_CLOSE(outImg[4], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[5], 0.0f, error);
        OCIO_CHECK_CLOSE(outImg[6], arbitraryVal, error);
        OCIO_CHECK_CLOSE(outImg[7], 1.0f, error);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, half)
{
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 65536);

    const float step = 1.f / ((float)lutData->getArray().getLength() - 1.0f);

    // No more an 'identity LUT 1D'.
    constexpr float arbitraryVal = 0.123456f;
    lutData->getArray()[5] = arbitraryVal;
    OCIO_CHECK_ASSERT(!lutData->isIdentity());

    const half inImg[8] = {
        0.1f, 0.3f, 0.4f, 1.0f,
        0.0f, 0.9f, step, 0.0f };

    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut, OCIO::BIT_DEPTH_F16, OCIO::BIT_DEPTH_F32));

    std::vector<float> outImg(2 * 4, -1.f);
    cpuOp->apply(&inImg[0], &outImg[0], 2);

    OCIO_CHECK_EQUAL(outImg[0], inImg[0]);
    OCIO_CHECK_EQUAL(outImg[1], inImg[1]);
    OCIO_CHECK_EQUAL(outImg[2], inImg[2]);
    OCIO_CHECK_EQUAL(outImg[3], inImg[3]);

    OCIO_CHECK_EQUAL(outImg[4], inImg[4]);
    OCIO_CHECK_EQUAL(outImg[5], inImg[5]);
    OCIO_CHECK_CLOSE(outImg[6], arbitraryVal, 1e-5f);
    OCIO_CHECK_EQUAL(outImg[7], inImg[7]);
}

OCIO_ADD_TEST(Lut1DRenderer, nan)
{
    // By default, this constructor creates an 'identity LUT'.
    OCIO::Lut1DOpDataRcPtr lutData =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 65536);

    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32));

    const float step = 1.f / ((float)lutData->getArray().getLength() - 1.0f);

    float myImage[8] = {
        std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f, 1.0f,
                                           0.0f, 0.0f, step, 1.0f };

    std::vector<float> outImg(2 * 4);
    cpuOp->apply(&myImage[0], &outImg[0], 2);

    OCIO_CHECK_EQUAL(outImg[0], 0.0f);
    OCIO_CHECK_EQUAL(outImg[1], 0.0f);
    OCIO_CHECK_EQUAL(outImg[2], 0.0f);
    OCIO_CHECK_EQUAL(outImg[3], 1.0f);

    OCIO_CHECK_EQUAL(outImg[4], 0.0f);
    OCIO_CHECK_EQUAL(outImg[5], 0.0f);
    OCIO_CHECK_EQUAL(outImg[6], step);
    OCIO_CHECK_EQUAL(outImg[7], 1.0f);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_red)
{
    OCIO::Lut1DOpDataRcPtr lutData =
        std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 32);

    OCIO::Array::Values & vals = lutData->getArray().getValues();
    const std::vector<float> lutValues = {
            0.f / 1023.f, 0.f, 0.f,
           33.f / 1023.f, 0.f, 0.f,
           66.f / 1023.f, 0.f, 0.f,
           99.f / 1023.f, 0.f, 0.f,
          132.f / 1023.f, 0.f, 0.f,
          165.f / 1023.f, 0.f, 0.f,
          198.f / 1023.f, 0.f, 0.f,
          231.f / 1023.f, 0.f, 0.f,
          264.f / 1023.f, 0.f, 0.f,
          297.f / 1023.f, 0.f, 0.f,
          330.f / 1023.f, 0.f, 0.f,
          363.f / 1023.f, 0.f, 0.f,
          396.f / 1023.f, 0.f, 0.f,
          429.f / 1023.f, 0.f, 0.f,
          462.f / 1023.f, 0.f, 0.f,
          495.f / 1023.f, 0.f, 0.f,
          528.f / 1023.f, 0.f, 0.f,
          561.f / 1023.f, 0.f, 0.f,
          594.f / 1023.f, 0.f, 0.f,
          627.f / 1023.f, 0.f, 0.f,
          660.f / 1023.f, 0.f, 0.f,
          693.f / 1023.f, 0.f, 0.f,
          726.f / 1023.f, 0.f, 0.f,
          759.f / 1023.f, 0.f, 0.f,
          792.f / 1023.f, 0.f, 0.f,
          825.f / 1023.f, 0.f, 0.f,
          858.f / 1023.f, 0.f, 0.f,
          891.f / 1023.f, 0.f, 0.f,
          924.f / 1023.f, 0.f, 0.f,
          957.f / 1023.f, 0.f, 0.f,
          990.f / 1023.f, 0.f, 0.f,
         1023.f / 1023.f, 0.f, 0.f
    };
    vals = lutValues;

    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut, OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT16));

    constexpr float step = 1.f / 31.f;
    const std::vector<float> inImg = {
        0.f,   0.f,  0.f,  0.f,
        step,  0.f,  0.f,  0.f,
        0.f,  step,  0.f,  0.f,
        0.f,   0.f, step,  0.f,
        step, step, step,  0.f };

    std::vector<uint16_t> outImg(5 * 4, 1);
    cpuOp->apply(&inImg[0], &outImg[0], 5);

    const uint16_t scaledStep = (uint16_t)roundf(step * 65535.0f);

    OCIO_CHECK_EQUAL(outImg[0], 0);
    OCIO_CHECK_EQUAL(outImg[1], 0);
    OCIO_CHECK_EQUAL(outImg[2], 0);
    OCIO_CHECK_EQUAL(outImg[3], 0);

    OCIO_CHECK_EQUAL(outImg[4], scaledStep);
    OCIO_CHECK_EQUAL(outImg[5], 0);
    OCIO_CHECK_EQUAL(outImg[6], 0);
    OCIO_CHECK_EQUAL(outImg[7], 0);

    OCIO_CHECK_EQUAL(outImg[8], 0);
    OCIO_CHECK_EQUAL(outImg[9], 0);
    OCIO_CHECK_EQUAL(outImg[10], 0);
    OCIO_CHECK_EQUAL(outImg[11], 0);

    OCIO_CHECK_EQUAL(outImg[12], 0);
    OCIO_CHECK_EQUAL(outImg[13], 0);
    OCIO_CHECK_EQUAL(outImg[14], 0);
    OCIO_CHECK_EQUAL(outImg[15], 0);

    OCIO_CHECK_EQUAL(outImg[16], scaledStep);
    OCIO_CHECK_EQUAL(outImg[17], 0);
    OCIO_CHECK_EQUAL(outImg[18], 0);
    OCIO_CHECK_EQUAL(outImg[19], 0);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_green)
{
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 32);

    OCIO::Array::Values & vals = lutData->getArray().getValues();
    const std::vector<float> lutValues = {
        0.f,    0.f / 1023.f, 0.f,
        0.f,   33.f / 1023.f, 0.f,
        0.f,   66.f / 1023.f, 0.f,
        0.f,   99.f / 1023.f, 0.f,
        0.f,  132.f / 1023.f, 0.f,
        0.f,  165.f / 1023.f, 0.f,
        0.f,  198.f / 1023.f, 0.f,
        0.f,  231.f / 1023.f, 0.f,
        0.f,  264.f / 1023.f, 0.f,
        0.f,  297.f / 1023.f, 0.f,
        0.f,  330.f / 1023.f, 0.f,
        0.f,  363.f / 1023.f, 0.f,
        0.f,  396.f / 1023.f, 0.f,
        0.f,  429.f / 1023.f, 0.f,
        0.f,  462.f / 1023.f, 0.f,
        0.f,  495.f / 1023.f, 0.f,
        0.f,  528.f / 1023.f, 0.f,
        0.f,  561.f / 1023.f, 0.f,
        0.f,  594.f / 1023.f, 0.f,
        0.f,  627.f / 1023.f, 0.f,
        0.f,  660.f / 1023.f, 0.f,
        0.f,  693.f / 1023.f, 0.f,
        0.f,  726.f / 1023.f, 0.f,
        0.f,  759.f / 1023.f, 0.f,
        0.f,  792.f / 1023.f, 0.f,
        0.f,  825.f / 1023.f, 0.f,
        0.f,  858.f / 1023.f, 0.f,
        0.f,  891.f / 1023.f, 0.f,
        0.f,  924.f / 1023.f, 0.f,
        0.f,  957.f / 1023.f, 0.f,
        0.f,  990.f / 1023.f, 0.f,
        0.f, 1023.f / 1023.f, 0.f
    };
    vals = lutValues;

    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_UINT16,
                                                       OCIO::BIT_DEPTH_F32));

    constexpr uint16_t step = (uint16_t)(65535 / 31);
    const std::vector<uint16_t> uint16_inImg = {
        0,    0,    0,    0,
        step, 0,    0,    0,
        0,    step, 0,    0,
        0,    0,    step, 0,
        step, step, step, 0 };

    std::vector<float> outImg(5 * 4, -1.f);
    cpuOp->apply(&uint16_inImg[0], &outImg[0], 5);

    constexpr float scaledStep = (float)step / 65535.0f;

    OCIO_CHECK_EQUAL(outImg[0], 0.f);
    OCIO_CHECK_EQUAL(outImg[1], 0.f);
    OCIO_CHECK_EQUAL(outImg[2], 0.f);
    OCIO_CHECK_EQUAL(outImg[3], 0.f);

    OCIO_CHECK_EQUAL(outImg[4], 0.f);
    OCIO_CHECK_EQUAL(outImg[5], 0.f);
    OCIO_CHECK_EQUAL(outImg[6], 0.f);
    OCIO_CHECK_EQUAL(outImg[7], 0.f);

    OCIO_CHECK_EQUAL(outImg[8], 0.f);
    OCIO_CHECK_EQUAL(outImg[9], scaledStep);
    OCIO_CHECK_EQUAL(outImg[10], 0.f);
    OCIO_CHECK_EQUAL(outImg[11], 0.f);

    OCIO_CHECK_EQUAL(outImg[12], 0.f);
    OCIO_CHECK_EQUAL(outImg[13], 0.f);
    OCIO_CHECK_EQUAL(outImg[14], 0.f);
    OCIO_CHECK_EQUAL(outImg[15], 0.f);

    OCIO_CHECK_EQUAL(outImg[16], 0.f);
    OCIO_CHECK_EQUAL(outImg[17], scaledStep);
    OCIO_CHECK_EQUAL(outImg[18], 0.f);
    OCIO_CHECK_EQUAL(outImg[19], 0.f);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_blue)
{
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, 32);

    OCIO::Array::Values & vals = lutData->getArray().getValues();
    const std::vector<float> lutValues = {
        0.f, 0.f,    0.f / 1023.f,
        0.f, 0.f,   33.f / 1023.f,
        0.f, 0.f,   66.f / 1023.f,
        0.f, 0.f,   99.f / 1023.f,
        0.f, 0.f,  132.f / 1023.f,
        0.f, 0.f,  165.f / 1023.f,
        0.f, 0.f,  198.f / 1023.f,
        0.f, 0.f,  231.f / 1023.f,
        0.f, 0.f,  264.f / 1023.f,
        0.f, 0.f,  297.f / 1023.f,
        0.f, 0.f,  330.f / 1023.f,
        0.f, 0.f,  363.f / 1023.f,
        0.f, 0.f,  396.f / 1023.f,
        0.f, 0.f,  429.f / 1023.f,
        0.f, 0.f,  462.f / 1023.f,
        0.f, 0.f,  495.f / 1023.f,
        0.f, 0.f,  528.f / 1023.f,
        0.f, 0.f,  561.f / 1023.f,
        0.f, 0.f,  594.f / 1023.f,
        0.f, 0.f,  627.f / 1023.f,
        0.f, 0.f,  660.f / 1023.f,
        0.f, 0.f,  693.f / 1023.f,
        0.f, 0.f,  726.f / 1023.f,
        0.f, 0.f,  759.f / 1023.f,
        0.f, 0.f,  792.f / 1023.f,
        0.f, 0.f,  825.f / 1023.f,
        0.f, 0.f,  858.f / 1023.f,
        0.f, 0.f,  891.f / 1023.f,
        0.f, 0.f,  924.f / 1023.f,
        0.f, 0.f,  957.f / 1023.f,
        0.f, 0.f,  990.f / 1023.f,
        0.f, 0.f, 1023.f / 1023.f
    };
    vals = lutValues;

    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_UINT16,
                                                       OCIO::BIT_DEPTH_UINT16));

    constexpr uint16_t step = (uint16_t)(65535 / 31);
    const std::vector<uint16_t> uint16_inImg = {
        0,    0,    0,    0,
        step, 0,    0,    0,
        0,    step, 0,    0,
        0,    0,    step, 0,
        step, step, step, 0 };

    std::vector<uint16_t> outImg(5 * 4, 2000);
    cpuOp->apply(&uint16_inImg[0], &outImg[0], 5);

    OCIO_CHECK_EQUAL(outImg[0], 0);
    OCIO_CHECK_EQUAL(outImg[1], 0);
    OCIO_CHECK_EQUAL(outImg[2], 0);
    OCIO_CHECK_EQUAL(outImg[3], 0);

    OCIO_CHECK_EQUAL(outImg[4], 0);
    OCIO_CHECK_EQUAL(outImg[5], 0);
    OCIO_CHECK_EQUAL(outImg[6], 0);
    OCIO_CHECK_EQUAL(outImg[7], 0);

    OCIO_CHECK_EQUAL(outImg[8], 0);
    OCIO_CHECK_EQUAL(outImg[9], 0);
    OCIO_CHECK_EQUAL(outImg[10], 0);
    OCIO_CHECK_EQUAL(outImg[11], 0);

    OCIO_CHECK_EQUAL(outImg[12], 0);
    OCIO_CHECK_EQUAL(outImg[13], 0);
    OCIO_CHECK_EQUAL(outImg[14], step);
    OCIO_CHECK_EQUAL(outImg[15], 0);

    OCIO_CHECK_EQUAL(outImg[16], 0);
    OCIO_CHECK_EQUAL(outImg[17], 0);
    OCIO_CHECK_EQUAL(outImg[18], step);
    OCIO_CHECK_EQUAL(outImg[19], 0);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_special_values)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    const std::string ctfLUT("lut1d_infinity.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));

    // This test should use the "interpolation" renderer path.
    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getDefaultCPUProcessor());

    constexpr float step = 1.f / 65535.f;

    float inImage[] = {
                  0.0f,     0.5f * step,            step, 1.0f,
        3000.0f * step, 32000.0f * step, 65535.0f * step, 1.0f
    };

    std::vector<float> outImage(2 * 4, -12.345f);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], 2, 1, 4);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], 2, 1, 4);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    // -Inf is mapped to -MAX_FLOAT
    const float negmax = -std::numeric_limits<float>::max();

    const float lutElem_1 = -3216.000488281f;
    const float lutElem_3000 = -539.565734863f;

    const float rtol = powf(2.f, -14.f);

    // LUT output bit-depth is 12i so normalize to F32.
    const float outRange = 4095.f;

    OCIO_CHECK_CLOSE(outImage[0], negmax, rtol);
    OCIO_CHECK_CLOSE(outImage[1], (lutElem_1 / outRange + negmax) * 0.5f, rtol);
    OCIO_CHECK_CLOSE(outImage[2], lutElem_1 / outRange, rtol);
    OCIO_CHECK_EQUAL(outImage[3], 1.0f);

    OCIO_CHECK_CLOSE(outImage[4], lutElem_3000 / outRange, rtol);
    OCIO_CHECK_CLOSE(outImage[5], negmax, rtol);
    OCIO_CHECK_CLOSE(outImage[6], negmax, rtol);
    OCIO_CHECK_EQUAL(outImage[7], 1.0f);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_hue_adjust)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    const std::string ctfLUT("lut1d_1024_hue_adjust_test.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));

    // This test should use the "lookup" renderer path.
    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_UINT16,
                                                                OCIO::BIT_DEPTH_UINT16,
                                                                OCIO::OPTIMIZATION_DEFAULT,
                                                                OCIO::FINALIZATION_EXACT));

    constexpr long NB_PIXELS = 2;
    // TODO: use UINT10 when implemented by ImageDesc.
    const uint16_t inImage[NB_PIXELS * 4] = {
        100 * 65535 / 1023, 500 * 65535 / 1023, 800 * 65535 / 1023,  200 * 65535 / 1023,
        400 * 65535 / 1023, 700 * 65535 / 1023, 300 * 65535 / 1023, 1023 * 65535 / 1023 };

    std::vector<uint16_t> outImage(NB_PIXELS * 4, 2000);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], NB_PIXELS, 1, 4,
                                     OCIO::BIT_DEPTH_UINT16,
                                     sizeof(uint16_t),
                                     OCIO::AutoStride,
                                     OCIO::AutoStride);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], NB_PIXELS, 1, 4,
                                     OCIO::BIT_DEPTH_UINT16,
                                     sizeof(uint16_t),
                                     OCIO::AutoStride,
                                     OCIO::AutoStride);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    OCIO_CHECK_EQUAL(outImage[0], 1523);
    OCIO_CHECK_EQUAL(outImage[1], 33883);  // Would be 31583 w/out hue adjust.
    OCIO_CHECK_EQUAL(outImage[2], 58154);
    OCIO_CHECK_EQUAL(outImage[3], 12812);

    OCIO_CHECK_EQUAL(outImage[4], 22319);  // Would be 21710 w/out hue adjust.
    OCIO_CHECK_EQUAL(outImage[5], 50648);
    OCIO_CHECK_EQUAL(outImage[6], 12877);
    OCIO_CHECK_EQUAL(outImage[7], 65535);

    // This test should use the "interpolation" renderer path.
    float inFloatImage[8];
    for (int i = 0; i < 8; ++i)
    {
        inFloatImage[i] = (float)inImage[i] / 65535.f;
    }
    std::fill(outImage.begin(), outImage.end(), 200);
    OCIO::PackedImageDesc srcImgFDesc((void*)&inFloatImage[0], NB_PIXELS, 1, 4,
                                      OCIO::BIT_DEPTH_F32,
                                      sizeof(float),
                                      OCIO::AutoStride,
                                      OCIO::AutoStride);

    OCIO::ConstCPUProcessorRcPtr cpuFwdFast;
    OCIO_CHECK_NO_THROW(cpuFwdFast = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                    OCIO::BIT_DEPTH_UINT16,
                                                                    OCIO::OPTIMIZATION_DEFAULT,
                                                                    OCIO::FINALIZATION_DEFAULT));
    
    cpuFwdFast->apply(srcImgFDesc, dstImgDesc);
    OCIO_CHECK_EQUAL(outImage[0], 1523);
    OCIO_CHECK_EQUAL(outImage[1], 33883);  // Would be 31583 w/out hue adjust.
    OCIO_CHECK_EQUAL(outImage[2], 58154);
    OCIO_CHECK_EQUAL(outImage[3], 12812);

    OCIO_CHECK_EQUAL(outImage[4], 22319);  // Would be 21710 w/out hue adjust.
    OCIO_CHECK_EQUAL(outImage[5], 50648);
    OCIO_CHECK_EQUAL(outImage[6], 12877);
    OCIO_CHECK_EQUAL(outImage[7], 65535);
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_half_domain_hue_adjust)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    const std::string ctfLUT("lut1d_hd_hue_adjust.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));

    // This test should use the "interpolation" renderer path.
    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getDefaultCPUProcessor());

    const float inImage[] = {
        0.05f, 0.18f, 1.1f, 0.5f,
        2.3f, 0.01f, 0.3f, 1.0f };

    std::vector<float> outImage(2 * 4, -1.f);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], 2, 1, 4);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], 2, 1, 4);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    constexpr float rtol = 1e-6f;
    constexpr float minExpected = 1e-3f;

    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[0], 0.54780269f, rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[1],
                                    9.57448578f, // Would be 5.0 w/out hue adjust.
                                    rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[2], 73.45562744f, rtol, minExpected));
    OCIO_CHECK_EQUAL(outImage[3], 0.5f);

    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[4], 188.087067f, rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[5], 0.0324990489f, rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[6],
                                    23.8472710f, // Would be 11.3372078 w/out hue adjust.
                                    rtol, minExpected));
    OCIO_CHECK_EQUAL(outImage[7], 1.0f);

    // This test should use the "lookup" renderer path.
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_UINT16,
                                                                OCIO::BIT_DEPTH_F32,
                                                                OCIO::OPTIMIZATION_DEFAULT,
                                                                OCIO::FINALIZATION_EXACT));

    // TODO: Use 10i when ImageDesc handles 10i.
    const uint16_t inImageInt[] = {
        200 * 65535 / 1023, 800 * 65535 / 1023, 500 * 65535 / 1023, 0,
        400 * 65535 / 1023, 100 * 65535 / 1023, 700 * 65535 / 1023, 1023 * 65535 / 1023
    };

    std::fill(outImage.begin(), outImage.end(), -1.f);
    OCIO::PackedImageDesc srcImgIntDesc((void*)&inImageInt[0], 2, 1, 4,
                                        OCIO::BIT_DEPTH_UINT16,
                                        sizeof(uint16_t),
                                        OCIO::AutoStride,
                                        OCIO::AutoStride);
    cpuFwd->apply(srcImgIntDesc, dstImgDesc);

    OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(outImage[0],
                                                  5.72640753f, rtol, minExpected));
    OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(outImage[1],
                                                  46.2259789f, rtol, minExpected));
    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[2],
                                    25.9756680f, // Would be 23.6895752 w/out hue adjust.
                                    rtol, minExpected));
    OCIO_CHECK_EQUAL(outImage[3], 0.0f);

    OCIO_CHECK_ASSERT(
        OCIO::EqualWithSafeRelError(outImage[4],
                                    20.0804043f, // Would be 17.0063152 w/out hue adjust.
                                    rtol, minExpected));
    OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(outImage[5],
                                                  1.78572845f, rtol, minExpected));
    OCIO_CHECK_ASSERT(OCIO::EqualWithSafeRelError(outImage[6],
                                                  38.3760300f, rtol, minExpected));
    OCIO_CHECK_EQUAL(outImage[7], 1.0f);
}

namespace
{
std::string GetErrorMessage(float expected, float actual)
{
    std::ostringstream oss;
    oss << "expected: " << expected << " != " << "actual: " << actual;
    return oss.str();
}
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_hue_adjust)
{
    const std::string ctfLUT("lut1d_1024_hue_adjust_test.ctf");

    OCIO::OpRcPtrVec ops;
    OCIO::ContextRcPtr context = OCIO::Context::Create();
    OCIO_CHECK_NO_THROW(OCIO::BuildOpsTest(ops, ctfLUT, context,
                        OCIO::TRANSFORM_DIR_FORWARD));

    OCIO_REQUIRE_EQUAL(ops.size(), 2);
    auto op = std::const_pointer_cast<const OCIO::Op>(ops[1]);
    auto opData = op->data();
    OCIO_CHECK_EQUAL(opData->getType(), OCIO::OpData::Lut1DType);
    auto lut = std::dynamic_pointer_cast<const OCIO::Lut1DOpData>(opData);

    auto lutData = lut->clone();

    // Currently need to set this to 16i in order for style == FAST to pass.
    // See comment in Lut1DOpData::MakeFastLut1DFromInverse.
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT16);

    OCIO_CHECK_NO_THROW(lutData->finalize());
    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_F32,
                                                       OCIO::BIT_DEPTH_F32));

    const float inImage[] = {
        0.1f,  0.25f, 0.7f,  0.f,
        0.66f, 0.25f, 0.81f, 0.5f,
        // 0.18f, 0.80f, 0.45f, 1.f }; // This one is easier to get correct.
        0.18f, 0.99f, 0.45f, 1.f };    // Setting G way up on the s-curve is harder.
    constexpr long NB_PIXELS = 3;

    std::vector<float> outImage(NB_PIXELS * 4);
    cpuOp->apply(inImage, &outImage[0], NB_PIXELS);

    // Inverse using LUT_INVERSION_FAST.
    lutData->setInversionQuality(OCIO::LUT_INVERSION_FAST);
    auto lutDataInv = lutData->inverse();

    OCIO::ConstLut1DOpDataRcPtr constLutInv = lutDataInv;
    OCIO_CHECK_NO_THROW(lutDataInv->finalize());
    OCIO::ConstOpCPURcPtr cpuOpInv;
    OCIO_CHECK_NO_THROW(cpuOpInv = OCIO::GetLut1DRenderer(constLutInv,
                                                          OCIO::BIT_DEPTH_F32,
                                                          OCIO::BIT_DEPTH_F32));

    std::vector<float> backImage(NB_PIXELS * 4, -1.f);
    cpuOpInv->apply(&outImage[0], &backImage[0], NB_PIXELS);

    for (long i = 0; i < NB_PIXELS * 4; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 130, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }

    // Repeat with LUT_INVERSION_EXACT.
    lutData->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    auto lutDataInv2 = lutData->inverse();

    OCIO::ConstLut1DOpDataRcPtr constLutInv2 = lutDataInv2;
    OCIO_CHECK_NO_THROW(lutDataInv2->finalize());
    OCIO::ConstOpCPURcPtr cpuOpInv2;
    OCIO_CHECK_NO_THROW(cpuOpInv2 = OCIO::GetLut1DRenderer(constLutInv2,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_F32));

    std::fill(backImage.begin(), backImage.end(), -1.f),
    cpuOpInv2->apply(&outImage[0], &backImage[0], NB_PIXELS);

    for (long i = 0; i < NB_PIXELS * 4; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 50, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_identity_half)
{
    // Create the 64k 16f Identity 1D LUT and the test Image.

    // By default, this constructor creates an 'identity lut'.
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE,
                                              65536);

    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_F16,
                                                       OCIO::BIT_DEPTH_F16));

    constexpr unsigned nbPixels = 65536;
    std::vector<half> myImage(nbPixels * 4);

    unsigned imgCntr = 0;

    for (unsigned i = 0; i < nbPixels; i++)
    {
        half hVal;
        hVal.setBits((unsigned short)i);

        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = 1.0f;
    }

    std::vector<half> outImg(nbPixels * 4);
    cpuOp->apply(&myImage[0], &outImg[0], nbPixels);

    imgCntr = 0;
    for (unsigned i = 0; i < nbPixels; i++, imgCntr += 4)
    {
        half hVal;
        hVal.setBits((unsigned short)i);

        if (hVal.isNan())
        {
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 0], 0.f);
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 1], 0.f);
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 2], 0.f);
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 3], 1.f);
        }
        else if (hVal.isInfinity())
        {
            OCIO_CHECK_ASSERT(outImg[imgCntr + 0].isInfinity());
            OCIO_CHECK_ASSERT(outImg[imgCntr + 1].isInfinity());
            OCIO_CHECK_ASSERT(outImg[imgCntr + 2].isInfinity());
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 3], 1.f);
        }
        else
        {
            OCIO_CHECK_EQUAL(outImg[imgCntr + 0], hVal);
            OCIO_CHECK_EQUAL(outImg[imgCntr + 1], hVal);
            OCIO_CHECK_EQUAL(outImg[imgCntr + 2], hVal);
            OCIO_CHECK_EQUAL((float)outImg[imgCntr + 3], 1.f);
        }
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_identity_half_to_int)
{
    // Create the 64k 16f Identity 1D LUT and the test Image.

    // By default, this constructor creates an 'identity lut'.
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE, 65536);

    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_F16,
                                                       OCIO::BIT_DEPTH_UINT16));

    constexpr unsigned nbPixels = 65536;
    std::vector<half> myImage(nbPixels * 4);

    unsigned imgCntr = 0;

    for (unsigned i = 0; i < nbPixels; i++)
    {
        half hVal;
        hVal.setBits((unsigned short)i);

        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = hVal;
        myImage[imgCntr++] = 1.0f;
    }

    std::vector<uint16_t> outImg(nbPixels * 4);
    cpuOp->apply(&myImage[0], &outImg[0], nbPixels);

    const float scaleFactor = (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT16);

    imgCntr = 0;
    for (unsigned i = 0; i < nbPixels; i++, imgCntr += 4)
    {
        half hVal;
        hVal.setBits((unsigned short)i);
        const float fVal = scaleFactor * (float)hVal;

        const uint16_t val = (uint16_t)OCIO::Clamp(fVal + 0.5f, 0.0f, scaleFactor);

        OCIO_CHECK_EQUAL(val, outImg[imgCntr + 0]);
        OCIO_CHECK_EQUAL(val, outImg[imgCntr + 1]);
        OCIO_CHECK_EQUAL(val, outImg[imgCntr + 2]);
        OCIO_CHECK_EQUAL(1.0f*scaleFactor, outImg[imgCntr + 3]);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_identity_int_to_half)
{
    // Create the 64k 16f Identity 1D LUT and the test Image.

    // By default, this constructor creates an 'identity lut'.
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE, 65536);

    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_UINT16,
                                                       OCIO::BIT_DEPTH_F16));

    constexpr unsigned nbPixels = 65536;
    std::vector<uint16_t> myImage(nbPixels * 4);

    unsigned imgCntr = 0;

    for (unsigned i = 0; i < nbPixels; i++)
    {
        myImage[imgCntr++] = (uint16_t)i;
        myImage[imgCntr++] = (uint16_t)i;
        myImage[imgCntr++] = (uint16_t)i;
        myImage[imgCntr++] = 1;
    }

    std::vector<half> outImg(nbPixels * 4);
    cpuOp->apply(&myImage[0], &outImg[0], nbPixels);

    const float scaleFactor = 1.f / (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT16);
    const half hScaleFactor = scaleFactor;
    imgCntr = 0;
    constexpr int tol = 1;
    for (unsigned i = 0; i < nbPixels; i++, imgCntr += 4)
    {
        const half hVal = scaleFactor * i;

        OCIO_CHECK_ASSERT(!OCIO::HalfsDiffer(outImg[imgCntr + 0], hVal, tol));
        OCIO_CHECK_ASSERT(!OCIO::HalfsDiffer(outImg[imgCntr + 1], hVal, tol));
        OCIO_CHECK_ASSERT(!OCIO::HalfsDiffer(outImg[imgCntr + 2], hVal, tol));
        OCIO_CHECK_EQUAL(outImg[imgCntr + 3].bits(), hScaleFactor.bits());
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_identity_half_code)
{
    // Create the 64k 16f Identity 1D LUT and the test Image.

    // By default, this constructor creates an 'identity lut'.
    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_OUTPUT_HALF_CODE, 65536);

    OCIO_CHECK_NO_THROW(lutData->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = lutData;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_F16,
                                                       OCIO::BIT_DEPTH_F16));

    constexpr unsigned nbPixels = 5;
    std::vector<half> myImage(nbPixels * 4);

    myImage[0] = 0.0f;
    myImage[1] = 0.0f;
    myImage[2] = 0.0f;
    myImage[3] = 1.0f;

    // Use values between points to test interpolation code.
    for (unsigned i = 4; i < 4 * nbPixels; i += 4)
    {
        half hVal1, hVal2;
        hVal1.setBits((unsigned short)i);
        hVal2.setBits((unsigned short)(i + 1));
        const float delta = fabs(hVal2 - hVal1);
        const float min = hVal1 < hVal2 ? hVal1 : hVal2;

        myImage[i + 0] = min + (delta / i);
        myImage[i + 1] = min + (delta / i);
        myImage[i + 2] = min + (delta / i);
        myImage[i + 3] = 1.0f;
    }

    std::vector<half> outImg(nbPixels * 4);
    cpuOp->apply(&myImage[0], &outImg[0], nbPixels);

    for (unsigned i = 0; i < 4 * nbPixels; i += 4)
    {
        OCIO_CHECK_EQUAL(outImg[i + 0].bits(), myImage[i + 0].bits());
        OCIO_CHECK_EQUAL(outImg[i + 1].bits(), myImage[i + 1].bits());
        OCIO_CHECK_EQUAL(outImg[i + 2].bits(), myImage[i + 2].bits());
        OCIO_CHECK_EQUAL((float)outImg[i + 3], 1.f);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_identity)
{
    // By default, this constructor creates an 'identity lut'.
    const auto dim = OCIO::Lut1DOpData::GetLutIdealSize(OCIO::BIT_DEPTH_UINT10);

    OCIO::Lut1DOpDataRcPtr lutData
        = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_STANDARD, dim);

    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    OCIO_CHECK_EQUAL(lutData->getInversionQuality(), OCIO::LUT_INVERSION_FAST);
    auto invLut = lutData->inverse();
    OCIO_CHECK_NO_THROW(invLut->finalize());

    OCIO::ConstLut1DOpDataRcPtr constLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constLut,
                                                       OCIO::BIT_DEPTH_UINT10,
                                                       OCIO::BIT_DEPTH_F32));

    constexpr uint16_t stepui = 700;  // relative to 10i.
    constexpr float step = stepui / 1023.f;

    const uint16_t inImage[] = {
        0,      0,      0,      0,
        stepui, 0,      0,      0,
        0,      stepui, 0,      0,
        0,      0,      stepui, 0,
        stepui, stepui, stepui, 0 };

    float outImage[] = {
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f };

    // Inverse of identity should still be identity.
    const float expected[] = {
        0.f,   0.f,  0.f, 0.f,
        step,  0.f,  0.f, 0.f,
        0.f,  step,  0.f, 0.f,
        0.f,   0.f, step, 0.f,
        step, step, step, 0.f };

    cpuOp->apply(&inImage[0], &outImage[0], 5);

    for (unsigned i = 0; i < 20; ++i)
    {
        OCIO_CHECK_CLOSE(outImage[i], expected[i], 1e-6f);
    }

    // Repeat with LUT_INVERSION_EXACT.
    invLut->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    OCIO::ConstOpCPURcPtr cpuOpExact;
    OCIO_CHECK_NO_THROW(cpuOpExact = OCIO::GetLut1DRenderer(constLut,
                                                            OCIO::BIT_DEPTH_UINT10,
                                                            OCIO::BIT_DEPTH_F32));

    cpuOpExact->apply(&inImage[0], &outImage[0], 5);

    for (unsigned i = 0; i < 20; ++i)
    {
        OCIO_CHECK_CLOSE(outImage[i], expected[i], 1e-6f);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_increasing)
{
    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(32);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    // This is a typical "easy" lut with a simple power function.
    // Linear to 1/2.2 gamma corrected code values.
    OCIO::Array::Values & vals = lutData->getArray().getValues();
    int i = 0;
    vals[i] = vals[i+1] = vals[i+2] =    0.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  215.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  294.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  354.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  403.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  446.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  485.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  520.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  553.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  583.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  612.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  639.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  665.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  689.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  713.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  735.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  757.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  779.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  799.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  819.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  838.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  857.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  875.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  893.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  911.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  928.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  944.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  961.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  977.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  992.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 1008.f / 1023.f;
    i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 1023.f / 1023.f;

    auto invLut = lutData->inverse();
    OCIO_CHECK_NO_THROW(invLut->finalize());

    invLut->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                       OCIO::BIT_DEPTH_UINT10,
                                                       OCIO::BIT_DEPTH_UINT16));

    // The first 2 rows are actual LUT entries, the others are intermediate values.
    const uint16_t inImage[] = {  // scaled to 10i
        (uint16_t)   0, (uint16_t) 215, (uint16_t) 446, (uint16_t)   0,
        (uint16_t) 639, (uint16_t) 944, (uint16_t)1023, (uint16_t) 445, // also test alpha
        (uint16_t)  40, (uint16_t) 190, (uint16_t) 260, (uint16_t) 685,
        (uint16_t) 380, (uint16_t) 540, (uint16_t) 767, (uint16_t)1023,
        (uint16_t) 888, (uint16_t)1000, (uint16_t)1018, (uint16_t)   0 };

    uint16_t outImage[] = {
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1 };

    const uint16_t expected[] = { // scaled to 16i
        (uint16_t)    0, (uint16_t)  2114, (uint16_t)10570, (uint16_t)    0,
        (uint16_t)23254, (uint16_t) 54965, (uint16_t)65535, (uint16_t)28507,
        (uint16_t)  393, (uint16_t)  1868, (uint16_t) 3318, (uint16_t)43882,
        (uint16_t) 7464, (uint16_t) 16079, (uint16_t)34785, (uint16_t)65535,
        (uint16_t)48036, (uint16_t) 62364, (uint16_t)64830, (uint16_t)    0 };

    cpuOp->apply(&inImage[0], &outImage[0], 5);

    for (unsigned i = 0; i < 20; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }

    // Repeat with LUT_INVERSION_FAST.
    invLut->setInversionQuality(OCIO::LUT_INVERSION_FAST);
    OCIO::ConstOpCPURcPtr cpuOpFast;
    OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_UINT10,
                                                           OCIO::BIT_DEPTH_UINT16));

    cpuOpFast->apply(&inImage[0], &outImage[0], 5);

    for (unsigned i = 0; i < 20; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_decreasing_reversals)
{
    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(12);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    // This is a more "difficult" LUT that is decreasing and has reversals
    // and values outside the typical range.
    OCIO::Array::Values & vals = lutData->getArray().getValues();
    int i = 0;
    vals[i] = vals[i+1] = vals[i+2] =  90.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  90.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 100.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  80.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  70.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  50.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  60.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  70.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  40.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  20.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] = -10.f / 255.f; // note: LUT vals may exceed [0,255]
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] = -10.f / 255.f;

    auto invLut = lutData->inverse();

    // Render as 32f in depth so we can test negative input vals.
    OCIO_CHECK_NO_THROW(invLut->finalize());

    // Default InvStyle should be 'FAST' but we test the 'EXACT' InvStyle first.
    invLut->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                       OCIO::BIT_DEPTH_F32,
                                                       OCIO::BIT_DEPTH_UINT16));

    // Render as 32f in depth so we can test negative input vals.
    const float inScaleFactor = 1.0f / (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT8);

    const float inImage[] = {  // scaled to 32f
        100.f * inScaleFactor, 90.f * inScaleFactor,  85.f * inScaleFactor, 0.f,
         75.f * inScaleFactor, 60.f * inScaleFactor,  50.f * inScaleFactor, 0.f,
         45.f * inScaleFactor, 30.f * inScaleFactor, -10.f * inScaleFactor, 0.f,
        -20.f * inScaleFactor, 75.f * inScaleFactor,  30.f * inScaleFactor, 0.f };

    uint16_t outImage[] = {
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1 };

    uint16_t expected[] = { // scaled to 16i
        (uint16_t)11915, (uint16_t)11915, (uint16_t)14894, (uint16_t)0,
        (uint16_t)20852, (uint16_t)26810, (uint16_t)29789, (uint16_t)0,
        (uint16_t)44683, (uint16_t)50641, (uint16_t)59577, (uint16_t)0,
        (uint16_t)59577, (uint16_t)20852, (uint16_t)50641, (uint16_t)0 };

    cpuOp->apply(&inImage[0], &outImage[0], 4);

    for (unsigned i = 0; i < 16; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }

    // Repeat with LUT_INVERSION_FAST.
    invLut->setInversionQuality(OCIO::LUT_INVERSION_FAST);
    OCIO::ConstOpCPURcPtr cpuOpFast;
    OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_UINT16));

    cpuOpFast->apply(&inImage[0], &outImage[0], 4);

    // Note: When there are flat spots in the original LUT, the approximate 
    // inverse LUT used in FAST mode has vertical jumps and so one would expect
    // significant differences from EXACT mode (which returns the left edge).
    // Since any value that is within the flat spot would result in the original
    // value on forward interpolation, we may loosen the tolerance for the inverse
    // to the domain of the flat spot.  Also, note that this is only an issue for
    // 32f inDepths since in all other cases EXACT mode is used to compute a LUT
    // that is used for look-up rather than interpolation.
    expected[1] = 11924;
    expected[6] = 38433;

    for (unsigned i = 0; i < 16; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_clamp_to_range)
{
    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(12);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    // Note that the start and end values do not span the full [0,255] range
    // so we test that input values are clamped correctly to this range when
    // the LUT has no flat spots at start or end.
    OCIO::Array::Values & vals = lutData->getArray().getValues();
    int i = 0;
    vals[i] = vals[i+1] = vals[i+2] =  30.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  40.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  60.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  65.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  70.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  50.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  60.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] =  70.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 100.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 190.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 200.f / 255.f;
	i += 3;
    vals[i] = vals[i+1] = vals[i+2] = 210.f / 255.f;

    auto invLut = lutData->inverse();
    // Render as 32f in depth so we can test negative input vals.
    OCIO_CHECK_NO_THROW(invLut->finalize());

    invLut->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                       OCIO::BIT_DEPTH_F32,
                                                       OCIO::BIT_DEPTH_UINT16));

    const float inScaleFactor = 1.0f / (float)OCIO::GetBitDepthMaxValue(OCIO::BIT_DEPTH_UINT8);

    const float inImage[] = {  // scaled to 32f
          0.f * inScaleFactor,  10.f * inScaleFactor,  30.f * inScaleFactor, 0.f,
         35.f * inScaleFactor, 202.f * inScaleFactor, 210.f * inScaleFactor, 0.f,
        -10.f * inScaleFactor, 255.f * inScaleFactor, 355.f * inScaleFactor, 0.f, };

    uint16_t outImage[] = {
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1 };

    uint16_t expected[] = { // scaled to 16i
        (uint16_t)    0, (uint16_t)    0, (uint16_t)    0, (uint16_t)0,
        (uint16_t) 2979, (uint16_t)60769, (uint16_t)65535, (uint16_t)0,
        (uint16_t)    0, (uint16_t)65535, (uint16_t)65535, (uint16_t)0, };

    cpuOp->apply(&inImage[0], &outImage[0], 3);

    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }

    // Repeat with LUT_INVERSION_FAST.
    invLut->setInversionQuality(OCIO::LUT_INVERSION_FAST);
    OCIO::ConstOpCPURcPtr cpuOpFast;
    OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_UINT16));

    cpuOpFast->apply(&inImage[0], &outImage[0], 3);

    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_flat_start_or_end)
{
    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(9);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

    // This LUT tests that flat spots at beginning and end of various lengths
    // are handled for increasing and decreasing LUTs (it also verifies that
    // LUTs with different R, G, B values are handled correctly).
    OCIO::Array::Values & vals = lutData->getArray().getValues();
    const std::vector<float> lutValues = { // scaled to 32f
        900.f / 1023.f,  70.f / 1023.f,  70.f / 1023.f,
        900.f / 1023.f,  70.f / 1023.f, 120.f / 1023.f,
        900.f / 1023.f, 120.f / 1023.f, 300.f / 1023.f,
        900.f / 1023.f, 300.f / 1023.f, 450.f / 1023.f,
        450.f / 1023.f, 450.f / 1023.f, 900.f / 1023.f,
        300.f / 1023.f, 900.f / 1023.f, 900.f / 1023.f,
        120.f / 1023.f, 900.f / 1023.f, 900.f / 1023.f,
         70.f / 1023.f, 900.f / 1023.f, 900.f / 1023.f,
         70.f / 1023.f, 900.f / 1023.f, 900.f / 1023.f };

    vals = lutValues;

    auto invLut = lutData->inverse();
    OCIO_CHECK_NO_THROW(invLut->finalize());

    invLut->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                       OCIO::BIT_DEPTH_UINT10,
                                                       OCIO::BIT_DEPTH_UINT16));

    const uint16_t inImage[] = {  // scaled to 10i
        (uint16_t)1023, (uint16_t)1023, (uint16_t)1023, (uint16_t)0,
        (uint16_t) 900, (uint16_t) 900, (uint16_t) 900, (uint16_t)0,
        (uint16_t) 800, (uint16_t) 800, (uint16_t) 800, (uint16_t)0,
        (uint16_t) 500, (uint16_t) 500, (uint16_t) 500, (uint16_t)0,
        (uint16_t) 450, (uint16_t) 450, (uint16_t) 450, (uint16_t)0,
        (uint16_t) 330, (uint16_t) 330, (uint16_t) 330, (uint16_t)0,
        (uint16_t) 150, (uint16_t) 150, (uint16_t) 150, (uint16_t)0,
        (uint16_t) 120, (uint16_t) 120, (uint16_t) 120, (uint16_t)0,
        (uint16_t)  80, (uint16_t)  80, (uint16_t)  80, (uint16_t)0,
        (uint16_t)  70, (uint16_t)  70, (uint16_t)  70, (uint16_t)0,
        (uint16_t)  60, (uint16_t)  60, (uint16_t)  60, (uint16_t)0,
        (uint16_t)   0, (uint16_t)   0, (uint16_t)   0, (uint16_t)0 };

    uint16_t outImage[] = {
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1,
        (uint16_t)-1, (uint16_t)-1, (uint16_t)-1, (uint16_t)-1 };

    const uint16_t expected[] = { // scaled to 16i
        (uint16_t)24576, (uint16_t)40959, (uint16_t)32768, (uint16_t)0,
        (uint16_t)24576, (uint16_t)40959, (uint16_t)32768, (uint16_t)0,
        (uint16_t)26396, (uint16_t)39139, (uint16_t)30947, (uint16_t)0,
        (uint16_t)31857, (uint16_t)33678, (uint16_t)25486, (uint16_t)0,
        (uint16_t)32768, (uint16_t)32768, (uint16_t)24576, (uint16_t)0,
        (uint16_t)39321, (uint16_t)26214, (uint16_t)18022, (uint16_t)0,
        (uint16_t)47786, (uint16_t)17749, (uint16_t) 9557, (uint16_t)0,
        (uint16_t)49151, (uint16_t)16384, (uint16_t) 8192, (uint16_t)0,
        (uint16_t)55705, (uint16_t) 9830, (uint16_t) 1638, (uint16_t)0,
        (uint16_t)57343, (uint16_t) 8192, (uint16_t)    0, (uint16_t)0,
        (uint16_t)57343, (uint16_t) 8192, (uint16_t)    0, (uint16_t)0,
        (uint16_t)57343, (uint16_t) 8192, (uint16_t)    0, (uint16_t)0 };

    cpuOp->apply(&inImage[0], &outImage[0], 12);

    for (unsigned i = 0; i < 12 * 4; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }

    // Repeat with LUT_INVERSION_FAST.
    invLut->setInversionQuality(OCIO::LUT_INVERSION_FAST);
    OCIO::ConstOpCPURcPtr cpuOpFast;
    OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_UINT10,
                                                           OCIO::BIT_DEPTH_UINT16));

    cpuOpFast->apply(&inImage[0], &outImage[0], 12);

    for (unsigned i = 0; i < 12 * 4; ++i)
    {
        OCIO_CHECK_EQUAL(outImage[i], expected[i]);
    }

}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_half_input)
{
    constexpr unsigned dim = 15;
    OCIO::Lut1DOpDataRcPtr lutData = std::make_shared<OCIO::Lut1DOpData>(dim);
    lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT8);

    // LUT entries.
    const float lutEntries[] = {
        0.00f, 0.05f, 0.10f, 0.15f, 0.20f,
        0.30f, 0.40f, 0.50f, 0.60f, 0.70f,
        0.80f, 0.85f, 0.90f, 0.95f, 1.00f };

    lutData->getArray().resize(dim, 1);
    for (unsigned i = 0; i < dim; ++i)
    {
        lutData->getArray()[i * 3 + 0] = lutEntries[i];
        lutData->getArray()[i * 3 + 1] = lutEntries[i];
        lutData->getArray()[i * 3 + 2] = lutEntries[i];
    }

    auto invLut = lutData->inverse();
    OCIO_CHECK_NO_THROW(invLut->finalize());

    invLut->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
    OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
    OCIO::ConstOpCPURcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                       OCIO::BIT_DEPTH_F16,
                                                       OCIO::BIT_DEPTH_F16));

    const half inImage[] = {
        1.00f, 0.91f, 0.85f, 0.f,
        0.75f, 0.02f, 0.53f, 0.f,
        0.47f, 0.30f, 0.21f, 0.f,
        0.50f, 0.11f, 0.00f, 0.f };

    half outImage[] = {
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f,
        -1.f, -1.f, -1.f, -1.f };

    // (dist + (val-low)/(high-low)) / (dim-1)
    const half expected[] = {
        1.0000000000f, 0.8714285714f, 0.7857142857f, 0.f,
        0.6785714285f, 0.0285714285f, 0.5214285714f, 0.f,
        0.4785714285f, 0.3571428571f, 0.2928571428f, 0.f,
        0.5000000000f, 0.1571428571f, 0.0000000000f, 0.f };

    cpuOp->apply(&inImage[0], &outImage[0], 4);

    for (unsigned i = 0; i < 4 * 4; ++i)
    {
        OCIO_CHECK_CLOSE(outImage[i].bits(), expected[i].bits(), 1.1f);
    }

    // Repeat with LUT_INVERSION_FAST.
    invLut->setInversionQuality(OCIO::LUT_INVERSION_FAST);
    OCIO::ConstOpCPURcPtr cpuOpFast;
    OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_F16,
                                                           OCIO::BIT_DEPTH_F16));

    cpuOpFast->apply(&inImage[0], &outImage[0], 4);

    for (unsigned i = 0; i < 4 * 4; ++i)
    {
        OCIO_CHECK_CLOSE(outImage[i].bits(), expected[i].bits(), 1.1f);
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_half_identity)
{
    // Need to do 10i-->32f and vice versa to check that
    // both the in scaling and out scaling are working correctly.

    constexpr uint16_t stepui = 700;  // relative to 10i
    constexpr float step = stepui / 1023.f;

    // Process from 10i to 32f bit-depths.
    {
        // By default, this constructor creates an 'identity LUT'.
        OCIO::Lut1DOpDataRcPtr lutData
            = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE, 65536);
        lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_UINT10);

        auto invLut = lutData->inverse();
        OCIO_CHECK_NO_THROW(invLut->finalize());

        invLut->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
        OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_UINT10,
                                                           OCIO::BIT_DEPTH_F32));

        const uint16_t inImage[] = {
            0,      0,      0,      0,
            stepui, 0,      0,      0,
            0,      stepui, 0,      0,
            0,      0,      stepui, 0,
            stepui, stepui, stepui, 0 };

        float outImage[] = {
            -1.f, -1.f, -1.f, -1.f,
            -1.f, -1.f, -1.f, -1.f,
            -1.f, -1.f, -1.f, -1.f,
            -1.f, -1.f, -1.f, -1.f,
            -1.f, -1.f, -1.f, -1.f };

        // Inverse of identity should still be identity.
        const float expected[] = {
            0.f,   0.f,  0.f, 0.f,
            step,  0.f,  0.f, 0.f,
            0.f,  step,  0.f, 0.f,
            0.f,   0.f, step, 0.f,
            step, step, step, 0.f };

        cpuOp->apply(&inImage[0], &outImage[0], 5);

        for (unsigned i = 0; i < 5 * 4; ++i)
        {
            OCIO_CHECK_CLOSE(outImage[i], expected[i], 1e-6f);
        }

        // Repeat with LUT_INVERSION_FAST.
        invLut->setInversionQuality(OCIO::LUT_INVERSION_FAST);
        OCIO::ConstOpCPURcPtr cpuOpFast;
        OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                               OCIO::BIT_DEPTH_UINT10,
                                                               OCIO::BIT_DEPTH_F32));

        cpuOpFast->apply(&inImage[0], &outImage[0], 5);

        for (unsigned i = 0; i < 5 * 4; ++i)
        {
            OCIO_CHECK_CLOSE(outImage[i], expected[i], 1e-6f);
        }
    }
    // Process from 32f to 10i bit-depths.
    {
        // By default, this constructor creates an 'identity LUT'.
        OCIO::Lut1DOpDataRcPtr lutData
            = std::make_shared<OCIO::Lut1DOpData>(OCIO::Lut1DOpData::LUT_INPUT_HALF_CODE,
                                                  65536);
        lutData->setFileOutputBitDepth(OCIO::BIT_DEPTH_F32);

        auto invLut = lutData->inverse();
        OCIO_CHECK_NO_THROW(invLut->finalize());

        invLut->setInversionQuality(OCIO::LUT_INVERSION_EXACT);
        OCIO::ConstLut1DOpDataRcPtr constInvLut = invLut;
        OCIO::ConstOpCPURcPtr cpuOp;
        OCIO_CHECK_NO_THROW(cpuOp = OCIO::GetLut1DRenderer(constInvLut,
                                                           OCIO::BIT_DEPTH_F32,
                                                           OCIO::BIT_DEPTH_UINT10));

        const float inImage[] = {
            0.f,   0.f,  0.f, 0.f,
            step,  0.f,  0.f, 0.f,
            0.f,  step,  0.f, 0.f,
            0.f,   0.f, step, 0.f,
            step, step, step, 0.f };

        uint16_t outImage[] = {
            10000, 10000, 10000, 10000,
            10000, 10000, 10000, 10000,
            10000, 10000, 10000, 10000,
            10000, 10000, 10000, 10000,
            10000, 10000, 10000, 10000 };

        // Inverse of identity should still be identity.
        const uint16_t expected[] = {
            0,      0,      0,      0,
            stepui, 0,      0,      0,
            0,      stepui, 0,      0,
            0,      0,      stepui, 0,
            stepui, stepui, stepui, 0 };
        cpuOp->apply(&inImage[0], &outImage[0], 5);

        for (unsigned i = 0; i < 5 * 4; ++i)
        {
            OCIO_CHECK_EQUAL(outImage[i], expected[i]);
        }

        // Repeat with LUT_INVERSION_FAST.
        invLut->setInversionQuality(OCIO::LUT_INVERSION_FAST);
        OCIO::ConstOpCPURcPtr cpuOpFast;
        OCIO_CHECK_NO_THROW(cpuOpFast = OCIO::GetLut1DRenderer(constInvLut,
                                                               OCIO::BIT_DEPTH_F32,
                                                               OCIO::BIT_DEPTH_UINT10));

        cpuOpFast->apply(&inImage[0], &outImage[0], 5);

        for (unsigned i = 0; i < 5 * 4; ++i)
        {
            OCIO_CHECK_EQUAL(outImage[i], expected[i]);
        }
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_half_ctf)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    // This ctf has increasing R & B channels and decreasing G channel.
    // It also has flat spots.
    const std::string ctfLUT("lut1d_halfdom.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));

    // This test should use the "interpolation" renderer path.
    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getDefaultCPUProcessor());

    const float inImage[12] = {
         1.f,    1.f,   0.5f, 0.f,
         0.001f, 0.1f,  4.f,  0.5f,  // test positive half domain of R, G, B channels
        -0.08f, -1.f, -10.f,  1.f }; // test negative half domain of R, G, B channels

    // Apply forward LUT.
    std::vector<float> outImage(12, -1.f);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], 3, 1, 4);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], 3, 1, 4);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    // Apply inverse LUT.
    // Inverse using LUT_INVERSION_FAST.
    fileTransform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    OCIO::ConstCPUProcessorRcPtr cpuOpInv;
    OCIO_CHECK_NO_THROW(cpuOpInv = proc->getDefaultCPUProcessor());

    std::vector<float> backImage(12, -1.f);
    OCIO::PackedImageDesc backImgDesc((void*)&backImage[0], 3, 1, 4);
    cpuOpInv->apply(dstImgDesc, backImgDesc);

    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 50, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }

    // Repeat with LUT_INVERSION_EXACT.
    OCIO::ConstCPUProcessorRcPtr cpuInvFast;
    std::fill(backImage.begin(), backImage.end(), -1.f);
    OCIO_CHECK_NO_THROW(cpuInvFast = proc->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT,
                                                                    OCIO::FINALIZATION_EXACT));
    cpuInvFast->apply(dstImgDesc, backImgDesc);

    for (unsigned i = 0; i < 12; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 50, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }
}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_half_fclut)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    // TODO: Review the test to add LUT & inverse LUT together when optimization is reworked.

    // Lustre log_default.fclut.  All positive halfs map to unique 16-bit ints
    // so it's a good test to see that the inverse can restore the halfs losslessly.
    const std::string ctfLUT("lut1d_hd_16f_16i_1chan.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    OCIO::ConstCPUProcessorRcPtr cpuOp;
    OCIO_CHECK_NO_THROW(cpuOp = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F16,
                                                               OCIO::BIT_DEPTH_F32,
                                                               OCIO::OPTIMIZATION_DEFAULT,
                                                               OCIO::FINALIZATION_EXACT));

    // Test all positive halfs (less than inf) round-trip losslessly.
    const int nbPixels = 31744;
    std::vector<half> inImage(nbPixels * 4);
    std::vector<float> outImage(nbPixels * 4, -1.f);
    for (unsigned short i = 0; i < nbPixels; ++i)
    {
        inImage[i * 4 + 0].setBits(i);
        inImage[i * 4 + 1].setBits(i);
        inImage[i * 4 + 2].setBits(i);
        inImage[i * 4 + 3].setBits(i);
    }

    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], nbPixels, 1, 4,
                                     OCIO::BIT_DEPTH_F16,
                                     sizeof(half),
                                     OCIO::AutoStride,
                                     OCIO::AutoStride);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], nbPixels, 1, 4);
    cpuOp->apply(srcImgDesc, dstImgDesc);

    fileTransform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    OCIO::ConstCPUProcessorRcPtr cpuOpInv;
    OCIO_CHECK_NO_THROW(cpuOpInv = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                  OCIO::BIT_DEPTH_F16,
                                                                  OCIO::OPTIMIZATION_DEFAULT,
                                                                  OCIO::FINALIZATION_EXACT));

    std::vector<half> backImage(nbPixels * 4, -1.f);
    OCIO::PackedImageDesc backImgDesc((void*)&backImage[0], nbPixels, 1, 4,
                                      OCIO::BIT_DEPTH_F16,
                                      sizeof(half),
                                      OCIO::AutoStride,
                                      OCIO::AutoStride);

    cpuOpInv->apply(dstImgDesc, backImgDesc);

    for (unsigned short i = 0; i < nbPixels; ++i)
    {
        OCIO_CHECK_EQUAL(inImage[i * 4 + 0].bits(), backImage[i * 4 + 0].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 1].bits(), backImage[i * 4 + 1].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 2].bits(), backImage[i * 4 + 2].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 3].bits(), backImage[i * 4 + 3].bits());
    }

    // Repeat with LUT_INVERSION_FAST.
    OCIO_CHECK_NO_THROW(cpuOpInv = proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                  OCIO::BIT_DEPTH_F16,
                                                                  OCIO::OPTIMIZATION_DEFAULT,
                                                                  OCIO::FINALIZATION_FAST));

    std::fill(backImage.begin(), backImage.end(), -1.f);
    cpuOpInv->apply(dstImgDesc, backImgDesc);

    for (unsigned short i = 0; i < nbPixels; ++i)
    {
        OCIO_CHECK_EQUAL(inImage[i * 4 + 0].bits(), backImage[i * 4 + 0].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 1].bits(), backImage[i * 4 + 1].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 2].bits(), backImage[i * 4 + 2].bits());
        OCIO_CHECK_EQUAL(inImage[i * 4 + 3].bits(), backImage[i * 4 + 3].bits());
    }

}

OCIO_ADD_TEST(Lut1DRenderer, lut_1d_inv_half_domain_hue_adjust)
{
    // Create empty Config to use.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setMajorVersion(2);

    const std::string ctfLUT("lut1d_hd_hue_adjust.ctf");
    OCIO::FileTransformRcPtr fileTransform = OCIO::CreateFileTransform(ctfLUT);

    OCIO::ConstProcessorRcPtr proc;
    // Get the processor corresponding to the transform.
    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    OCIO::ConstCPUProcessorRcPtr cpuFwd;
    OCIO_CHECK_NO_THROW(cpuFwd = proc->getDefaultCPUProcessor());

    constexpr long NB_PIXELS = 3;
    const float inImage[NB_PIXELS * 4] = {
        0.1f,  0.25f, 0.7f,  0.f,
        0.66f, 0.25f, 0.81f, 0.5f,
        0.18f, 0.99f, 0.45f, 1.f };

    std::vector<float> outImage(NB_PIXELS * 4, 1.f);
    OCIO::PackedImageDesc srcImgDesc((void*)&inImage[0], NB_PIXELS, 1, 4);
    OCIO::PackedImageDesc dstImgDesc((void*)&outImage[0], NB_PIXELS, 1, 4);
    cpuFwd->apply(srcImgDesc, dstImgDesc);

    // Inverse using LUT_INVERSION_FAST.
    fileTransform->setDirection(OCIO::TRANSFORM_DIR_INVERSE);

    OCIO_CHECK_NO_THROW(proc = config->getProcessor(fileTransform));
    OCIO::ConstCPUProcessorRcPtr cpuInv;
    OCIO_CHECK_NO_THROW(cpuInv = proc->getDefaultCPUProcessor());

    std::vector<float> backImage(NB_PIXELS * 4, -1.f);
    OCIO::PackedImageDesc backImgDesc((void*)&backImage[0], NB_PIXELS, 1, 4);
    cpuInv->apply(dstImgDesc, backImgDesc);

    for (long i = 0; i < NB_PIXELS * 4; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 50, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }

    // Repeat with LUT_INVERSION_EXACT.
    OCIO::ConstCPUProcessorRcPtr cpuInvFast;
    OCIO_CHECK_NO_THROW(cpuInvFast = proc->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_DEFAULT,
                                                                    OCIO::FINALIZATION_EXACT));
    std::fill(backImage.begin(), backImage.end(), -1.f);
    cpuInvFast->apply(dstImgDesc, backImgDesc);

    for (long i = 0; i < NB_PIXELS * 4; ++i)
    {
        OCIO_CHECK_ASSERT_MESSAGE(!OCIO::FloatsDiffer(inImage[i], backImage[i], 50, false),
                                  GetErrorMessage(inImage[i], backImage[i]));
    }
}


#endif
