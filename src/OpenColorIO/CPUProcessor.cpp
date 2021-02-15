// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "CPUProcessor.h"
#include "ops/lut1d/Lut1DOpCPU.h"
#include "ops/lut3d/Lut3DOpCPU.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/range/RangeOpCPU.h"
#include "ScanlineHelper.h"


namespace OCIO_NAMESPACE
{

template<BitDepth inBD, BitDepth outBD>
class BitDepthCast : public OpCPU
{
    typedef typename BitDepthInfo<inBD>::Type InType;
    typedef typename BitDepthInfo<outBD>::Type OutType;

public:
    BitDepthCast() = default;
    ~BitDepthCast() override {};

    void apply(const void * inImg, void * outImg, long numPixels) const override
    {
        const InType * in = reinterpret_cast<const InType*>(inImg);
        OutType * out = reinterpret_cast<OutType*>(outImg);

        for(long pxl=0; pxl<numPixels; ++pxl)
        {
            out[0] = Converter<outBD>::CastValue(in[0] * m_scale);
            out[1] = Converter<outBD>::CastValue(in[1] * m_scale);
            out[2] = Converter<outBD>::CastValue(in[2] * m_scale);
            out[3] = Converter<outBD>::CastValue(in[3] * m_scale);

            in  += 4;
            out += 4;
        }
    }

protected:
    const float m_scale = float(BitDepthInfo<outBD>::maxValue)
                            / float(BitDepthInfo<inBD>::maxValue);
};

template<>
class BitDepthCast<BIT_DEPTH_F32, BIT_DEPTH_F32> : public OpCPU
{
public:
    BitDepthCast() = default;
    ~BitDepthCast() override {};

    void apply(const void * inImg, void * outImg, long numPixels) const override
    {
        if(inImg!=outImg)
        {
            memcpy(outImg, inImg, 4*numPixels*sizeof(float));
        }
    }
};

ConstOpCPURcPtr CreateGenericBitDepthHelper(BitDepth in, BitDepth out)
{

#define ADD_OUT_BIT_DEPTH(in, out)                    \
case out:                                             \
{                                                     \
    return std::make_shared<BitDepthCast<in, out>>(); \
    break;                                            \
}

#define ADD_IN_BIT_DEPTH(in)                          \
case in:                                              \
{                                                     \
    switch(out)                                       \
    {                                                 \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_UINT8)        \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_UINT10)       \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_UINT12)       \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_UINT16)       \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_F16)          \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_F32)          \
        case BIT_DEPTH_UINT14:                        \
        case BIT_DEPTH_UINT32:                        \
        case BIT_DEPTH_UNKNOWN:                       \
        default:                                      \
            throw Exception("Unsupported bit-depth"); \
            break;                                    \
                                                      \
    }                                                 \
    break;                                            \
}


    switch(in)
    {
        ADD_IN_BIT_DEPTH(BIT_DEPTH_UINT8)
        ADD_IN_BIT_DEPTH(BIT_DEPTH_UINT10)
        ADD_IN_BIT_DEPTH(BIT_DEPTH_UINT12)
        ADD_IN_BIT_DEPTH(BIT_DEPTH_UINT16)
        ADD_IN_BIT_DEPTH(BIT_DEPTH_F16)
        ADD_IN_BIT_DEPTH(BIT_DEPTH_F32)
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
            throw Exception("Unsupported bit-depth");
    }

#undef ADD_OUT_BIT_DEPTH
#undef ADD_IN_BIT_DEPTH

    throw Exception("Unsupported bit-depths");
}

void CreateCPUEngine(const OpRcPtrVec & ops, 
                     BitDepth in, 
                     BitDepth out,
                     OptimizationFlags oFlags,
                     // The bit-depth 'cast' or the first CPU Op.
                     ConstOpCPURcPtr & inBitDepthOp,
                     // The remaining CPU Ops.
                     ConstOpCPURcPtrVec & cpuOps,
                     // The bit-depth 'cast' or the last CPU Op.
                     ConstOpCPURcPtr & outBitDepthOp)
{
    const size_t maxOps = ops.size();
    const bool fastLogExpPow = HasFlag(oFlags, OPTIMIZATION_FAST_LOG_EXP_POW);
    for(size_t idx=0; idx<maxOps; ++idx)
    {
        ConstOpRcPtr op = ops[idx];
        ConstOpDataRcPtr opData = op->data();

        if(idx==0)
        {
            if(opData->getType()==OpData::Lut1DType)
            {
                ConstLut1DOpDataRcPtr lut = DynamicPtrCast<const Lut1DOpData>(opData);
                inBitDepthOp = GetLut1DRenderer(lut, in, BIT_DEPTH_F32);
            }
            else if(in==BIT_DEPTH_F32)
            {
                inBitDepthOp = op->getCPUOp(fastLogExpPow);
            }
            else
            {
                inBitDepthOp = CreateGenericBitDepthHelper(in, BIT_DEPTH_F32);
                cpuOps.push_back(op->getCPUOp(fastLogExpPow));
            }

            if(maxOps==1)
            {
                outBitDepthOp = CreateGenericBitDepthHelper(BIT_DEPTH_F32, out);
            }
        }
        else if(idx==(maxOps-1))
        {
            if(opData->getType()==OpData::Lut1DType)
            {
                ConstLut1DOpDataRcPtr lut = DynamicPtrCast<const Lut1DOpData>(opData);
                outBitDepthOp = GetLut1DRenderer(lut, BIT_DEPTH_F32, out);
            }
            else if(out==BIT_DEPTH_F32)
            {
                outBitDepthOp = op->getCPUOp(fastLogExpPow);
            }
            else
            {
                outBitDepthOp = CreateGenericBitDepthHelper(BIT_DEPTH_F32, out);
                cpuOps.push_back(op->getCPUOp(fastLogExpPow));
            }
        }
        else
        {
            cpuOps.push_back(op->getCPUOp(fastLogExpPow));
        }
    }
}


ScanlineHelper * CreateScanlineHelper(BitDepth in, const ConstOpCPURcPtr & inBitDepthOp,
                                      BitDepth out, const ConstOpCPURcPtr & outBitDepthOp)
{

#define ADD_OUT_BIT_DEPTH(in, out)                    \
case out:                                             \
{                                                     \
    return new GenericScanlineHelper<BitDepthInfo<in>::Type,                      \
                                     BitDepthInfo<out>::Type>(in, inBitDepthOp,   \
                                                              out, outBitDepthOp);\
    break;                                            \
}

#define ADD_IN_BIT_DEPTH(in)                          \
case in:                                              \
{                                                     \
    switch(out)                                       \
    {                                                 \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_UINT8)        \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_UINT10)       \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_UINT12)       \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_UINT16)       \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_F16)          \
        ADD_OUT_BIT_DEPTH(in, BIT_DEPTH_F32)          \
        case BIT_DEPTH_UINT14:                        \
        case BIT_DEPTH_UINT32:                        \
        case BIT_DEPTH_UNKNOWN:                       \
        default:                                      \
            throw Exception("Unsupported bit-depth"); \
                                                      \
    }                                                 \
    break;                                            \
}

    switch(in)
    {
        ADD_IN_BIT_DEPTH(BIT_DEPTH_UINT8)
        ADD_IN_BIT_DEPTH(BIT_DEPTH_UINT10)
        ADD_IN_BIT_DEPTH(BIT_DEPTH_UINT12)
        ADD_IN_BIT_DEPTH(BIT_DEPTH_UINT16)
        ADD_IN_BIT_DEPTH(BIT_DEPTH_F16)
        ADD_IN_BIT_DEPTH(BIT_DEPTH_F32)
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
        default:
            throw Exception("Unsupported bit-depth");
    }

#undef ADD_OUT_BIT_DEPTH
#undef ADD_IN_BIT_DEPTH

    throw Exception("Unsupported bit-depths");
}

DynamicPropertyRcPtr CPUProcessor::Impl::getDynamicProperty(DynamicPropertyType type) const
{
    if (m_inBitDepthOp->hasDynamicProperty(type))
    {
        return m_inBitDepthOp->getDynamicProperty(type);
    }

    for (const auto & op : m_cpuOps)
    {
        if (op->hasDynamicProperty(type))
        {
            return op->getDynamicProperty(type);
        }
    }

    if (m_outBitDepthOp->hasDynamicProperty(type))
    {
        return m_outBitDepthOp->getDynamicProperty(type);
    }

    throw Exception("Cannot find dynamic property; not used by CPU processor.");
}

void FinalizeOpsForCPU(OpRcPtrVec & ops, const OpRcPtrVec & rawOps,
                       BitDepth in, BitDepth out,
                       OptimizationFlags oFlags)
{
    ops = rawOps;

    if(!ops.empty())
    {
        // Optimize the ops.
        ops.finalize(oFlags);
        ops.optimizeForBitdepth(in, out, oFlags);
    }

    if(ops.empty())
    {
        // Support an empty list.

        const double scale = GetBitDepthMaxValue(out) / GetBitDepthMaxValue(in);

        if(scale==1.0f)
        {
            // Needs at least one op (even an identity one) as the input
            // and output buffers could be different.
            CreateIdentityMatrixOp(ops);
        }
        else
        {
            // Note: CreateScaleOp will not add an op if scale == 1.
            const double scale4[4] = {scale, scale, scale, scale};
            CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);
        }
    }

    if (!((oFlags & OPTIMIZATION_NO_DYNAMIC_PROPERTIES) == OPTIMIZATION_NO_DYNAMIC_PROPERTIES))
    {
        ops.validateDynamicProperties();
    }
}

void CPUProcessor::Impl::finalize(const OpRcPtrVec & rawOps,
                                  BitDepth in, BitDepth out,
                                  OptimizationFlags oFlags)
{
    AutoMutex lock(m_mutex);

    OpRcPtrVec ops;
    FinalizeOpsForCPU(ops, rawOps, in, out, oFlags);

    m_inBitDepth  = in;
    m_outBitDepth = out;

    m_isIdentity = ops.isNoOp();
    m_isNoOp     = m_isIdentity && m_inBitDepth==m_outBitDepth;

    // Does the color processing introduce crosstalk between the pixel channels?
    m_hasChannelCrosstalk = ops.hasChannelCrosstalk();

    // Get the CPU Ops while taking care of the input and output bit-depths.

    m_cpuOps.clear();
    m_inBitDepthOp = nullptr;
    m_outBitDepthOp = nullptr;
    CreateCPUEngine(ops, in, out, oFlags, m_inBitDepthOp, m_cpuOps, m_outBitDepthOp);

    // Compute the cache id.

    std::stringstream ss;
    ss << "CPU Processor: from " << BitDepthToString(in)
       << " to "  << BitDepthToString(out)
       << " oFlags " << oFlags
       << " ops:";
    for(const auto & op : ops)
    {
        ss << " " << op->getCacheID();
    }

    m_cacheID = ss.str();
}

void CPUProcessor::Impl::apply(ImageDesc & imgDesc) const
{   
    // Get the ScanlineHelper for this thread (no significant performance impact).
    std::unique_ptr<ScanlineHelper> 
        scanlineBuilder(CreateScanlineHelper(m_inBitDepth, m_inBitDepthOp,
                                             m_outBitDepth, m_outBitDepthOp));

    // Prepare the processing.
    scanlineBuilder->init(imgDesc);

    float * rgbaBuffer = nullptr;
    long numPixels = 0;

    while(true)
    {
        scanlineBuilder->prepRGBAScanline(&rgbaBuffer, numPixels);
        if(numPixels == 0) break;

        const size_t numOps = m_cpuOps.size();
        for(size_t i = 0; i<numOps; ++i)
        {
            m_cpuOps[i]->apply(rgbaBuffer, rgbaBuffer, numPixels);
        }

        scanlineBuilder->finishRGBAScanline();
    }
}

void CPUProcessor::Impl::apply(const ImageDesc & srcImgDesc, ImageDesc & dstImgDesc) const
{
    // Get the ScanlineHelper for this thread (no significant performance impact).
    std::unique_ptr<ScanlineHelper> 
        scanlineBuilder(CreateScanlineHelper(m_inBitDepth, m_inBitDepthOp,
                                             m_outBitDepth, m_outBitDepthOp));

    // Prepare the processing.
    scanlineBuilder->init(srcImgDesc, dstImgDesc);

    float * rgbaBuffer = nullptr;
    long numPixels = 0;

    while(true)
    {
        scanlineBuilder->prepRGBAScanline(&rgbaBuffer, numPixels);
        if(numPixels == 0) break;

        const size_t numOps = m_cpuOps.size();
        for(size_t i = 0; i<numOps; ++i)
        {
            m_cpuOps[i]->apply(rgbaBuffer, rgbaBuffer, numPixels);
        }

        scanlineBuilder->finishRGBAScanline();
    }
}

void CPUProcessor::Impl::applyRGB(float * pixel) const
{
    float v[4]{pixel[0], pixel[1], pixel[2], 0.0f};

    m_inBitDepthOp->apply(v, v, 1);

    const size_t numOps = m_cpuOps.size();
    for(size_t i = 0; i<numOps; ++i)
    {
        m_cpuOps[i]->apply(v, v, 1);
    }

    m_outBitDepthOp->apply(v, v, 1);

    pixel[0] = v[0];
    pixel[1] = v[1];
    pixel[2] = v[2];
}

void CPUProcessor::Impl::applyRGBA(float * pixel) const
{
    m_inBitDepthOp->apply(pixel, pixel, 1);

    const size_t numOps = m_cpuOps.size();
    for(size_t i = 0; i<numOps; ++i)
    {
        m_cpuOps[i]->apply(pixel, pixel, 1);
    }

    m_outBitDepthOp->apply(pixel, pixel, 1);
}




//////////////////////////////////////////////////////////////////////////




void CPUProcessor::deleter(CPUProcessor * c)
{
    delete c;
}

CPUProcessor::CPUProcessor()
    :   m_impl(new Impl)
{
}

CPUProcessor::~CPUProcessor()
{
    delete m_impl;
    m_impl = nullptr;
}

bool CPUProcessor::isNoOp() const
{
    return getImpl()->isNoOp();
}

bool CPUProcessor::isIdentity() const
{
    return getImpl()->isIdentity();
}

bool CPUProcessor::hasChannelCrosstalk() const
{
    return getImpl()->hasChannelCrosstalk();
}

const char * CPUProcessor::getCacheID() const
{
    return getImpl()->getCacheID();
}

BitDepth CPUProcessor::getInputBitDepth() const
{
    return getImpl()->getInputBitDepth();
}

BitDepth CPUProcessor::getOutputBitDepth() const
{
    return getImpl()->getOutputBitDepth();
}

DynamicPropertyRcPtr CPUProcessor::getDynamicProperty(DynamicPropertyType type) const
{
    return getImpl()->getDynamicProperty(type);
}

void CPUProcessor::apply(ImageDesc & imgDesc) const
{
    getImpl()->apply(imgDesc);
}

void CPUProcessor::apply(const ImageDesc & srcImgDesc, ImageDesc & dstImgDesc) const
{
    getImpl()->apply(srcImgDesc, dstImgDesc);
}

void CPUProcessor::applyRGB(float * pixel) const
{
    getImpl()->applyRGB(pixel);
}

void CPUProcessor::applyRGBA(float * pixel) const
{
    getImpl()->applyRGBA(pixel);
}

} // namespace OCIO_NAMESPACE

