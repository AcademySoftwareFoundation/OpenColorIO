/*
Copyright (c) 2019 Autodesk Inc., et al.
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

#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "CPUProcessor.h"
#include "ops/Lut1D/Lut1DOpCPU.h"
#include "ops/Lut3D/Lut3DOpCPU.h"
#include "ops/Matrix/MatrixOps.h"
#include "ops/Range/RangeOpCPU.h"
#include "ScanlineHelper.h"


OCIO_NAMESPACE_ENTER
{

template<BitDepth inBD, BitDepth outBD>
class BitDepthCast : public OpCPU
{
public:
    BitDepthCast() : OpCPU(), m_scale(1.0f) 
    {
        m_scale = float(BitDepthInfo<outBD>::maxValue) 
                    / float(BitDepthInfo<inBD>::maxValue);
    }

    void apply(const void * inImg, void * outImg, long numPixels) const override
    {
        typedef typename BitDepthInfo<inBD>::Type InType;
        typedef typename BitDepthInfo<outBD>::Type OutType;

        const InType * in = (const InType *)inImg;
        OutType * out = (OutType *)outImg;

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
    float m_scale;
};

template<>
class BitDepthCast<BIT_DEPTH_F32, BIT_DEPTH_F32> : public OpCPU
{
public:
    BitDepthCast() : OpCPU(), m_scale(1.0f) 
    {
    }

    void apply(const void * inImg, void * outImg, long numPixels) const override
    {
        if(inImg!=outImg)
        {
            memcpy(outImg, inImg, 4*numPixels*sizeof(float));
        }
    }

protected:
    float m_scale;
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
            break;
    }

#undef ADD_OUT_BIT_DEPTH
#undef ADD_IN_BIT_DEPTH

    throw Exception("Unsupported bit-depths");
}

// 1D LUT is the only op natively supporting bit-depths.
ConstOpCPURcPtr CreateLut1DHelper(ConstLut1DOpDataRcPtr & lut, BitDepth in, BitDepth out)
{
    if(in==out && in==BIT_DEPTH_F32)
    {
        if(lut->getInputBitDepth()!=BIT_DEPTH_F32
            || lut->getOutputBitDepth()!=BIT_DEPTH_F32)
        {
            throw Exception("Unsupported 1D LUT bit-depths.");
        }

        return GetLut1DRenderer(lut, in, out);
    }

    Lut1DOpDataRcPtr l = lut->clone();
    l->setInputBitDepth(in);
    l->setOutputBitDepth(out);
    ConstLut1DOpDataRcPtr tmp = l;
    return GetLut1DRenderer(tmp, in, out);
}

void CreateCPUEngine(const OpRcPtrVec & ops, 
                     BitDepth in, 
                     BitDepth out,
                     // The bit-depth 'cast' or the first CPU Op.
                     ConstOpCPURcPtr & inBitDepthOp, 
                     // The remaining CPU Ops.
                     ConstOpCPURcPtrVec & cpuOps, 
                     // The bit-depth 'cast' or the last CPU Op.
                     ConstOpCPURcPtr & outBitDepthOp)
{
    const size_t maxOps = ops.size();
    for(size_t idx=0; idx<maxOps; ++idx)
    {
        ConstOpRcPtr op = ops[idx];
        ConstOpDataRcPtr opData = op->data();

        if(idx==0)
        {
            if(opData->getType()==OpData::Lut1DType)
            {
                ConstLut1DOpDataRcPtr lut = DynamicPtrCast<const Lut1DOpData>(opData);
                inBitDepthOp = CreateLut1DHelper(lut, in, BIT_DEPTH_F32);
            }
            else if(in==BIT_DEPTH_F32)
            {
                inBitDepthOp = op->getCPUOp();
            }
            else
            {
                inBitDepthOp = CreateGenericBitDepthHelper(in, BIT_DEPTH_F32);
                cpuOps.push_back(op->getCPUOp());
            }

            if(idx==(maxOps-1))
            {
                outBitDepthOp = CreateGenericBitDepthHelper(BIT_DEPTH_F32, out);
            }
        }
        else if(idx==(maxOps-1))
        {
            if(opData->getType()==OpData::Lut1DType)
            {
                ConstLut1DOpDataRcPtr lut = DynamicPtrCast<const Lut1DOpData>(opData);
                outBitDepthOp = CreateLut1DHelper(lut, BIT_DEPTH_F32, out);
            }
            else if(out==BIT_DEPTH_F32)
            {
                outBitDepthOp = op->getCPUOp();
            }
            else
            {
                outBitDepthOp = CreateGenericBitDepthHelper(BIT_DEPTH_F32, out);
                cpuOps.push_back(op->getCPUOp());
            }
        }
        else
        {
            cpuOps.push_back(op->getCPUOp());
        }
    }
}


ScanlineHelper * CreateScanlineHelper(BitDepth in, ConstOpCPURcPtr & inBitDepthOp,
                                      BitDepth out, ConstOpCPURcPtr & outBitDepthOp)
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
    if(m_inBitDepthOp->hasDynamicProperty(type))
    {
        return m_inBitDepthOp->getDynamicProperty(type);
    }

    for(const auto & op : m_cpuOps)
    {
        if(op->hasDynamicProperty(type))
        {
            return op->getDynamicProperty(type);
        }
    }

    if(m_outBitDepthOp->hasDynamicProperty(type))
    {
        return m_outBitDepthOp->getDynamicProperty(type);
    }

    throw Exception("Cannot find dynamic property; not used by CPU processor.");
}

void CPUProcessor::Impl::finalize(const OpRcPtrVec & rawOps,
                                  BitDepth in, BitDepth out,
                                  OptimizationFlags oFlags, FinalizationFlags fFlags)
{
    AutoMutex lock(m_mutex);

    OpRcPtrVec ops = rawOps.clone();

    if(!ops.empty())
    {
        // Adjust the op list to the input and output bit-depths
        // to enable the separable optimization.

        ops.front()->setInputBitDepth(in);
        ops.back()->setOutputBitDepth(out);

        // Optimize the ops.

        OptimizeOpVec(ops, oFlags);
    }

    if(ops.empty())
    {
        // Support an empty list.

        const float scale = GetBitDepthMaxValue(out) / GetBitDepthMaxValue(in);

        if(scale==1.0f)
        {
            // Needs at least one op (even an identity one) as the input 
            // and output buffers could be different. 
            CreateIdentityMatrixOp(ops);
        }
        else
        {
            // Note: CreateScaleOp will not add an op if scale == 1.
            const float scale4[4] = {scale, scale, scale, scale};
            CreateScaleOp(ops, scale4, TRANSFORM_DIR_FORWARD);
        }
    }

    // Finalize the ops.

    FinalizeOpVec(ops, fFlags);
    UnifyDynamicProperties(ops);

    m_inBitDepth  = in;
    m_outBitDepth = out;

    // Does the color processing introduce crosstalk between the pixel channels?

    m_hasChannelCrosstalk = false;
    for(const auto & op : ops)
    {
        if(op->hasChannelCrosstalk())
        {
            m_hasChannelCrosstalk = true;
            break;
        }
    }

    // Get the CPU Ops while taking care of the input and output bit-depths.

    m_cpuOps.clear();
    m_inBitDepthOp = nullptr;
    m_outBitDepthOp = nullptr;
    CreateCPUEngine(ops, in, out, m_inBitDepthOp, m_cpuOps, m_outBitDepthOp);

    // Get the right ScanlineHelper.
    m_scanlineBuilder.reset(CreateScanlineHelper(m_inBitDepth, m_inBitDepthOp,
                                                 m_outBitDepth, m_outBitDepthOp));

    // Compute the cache id.

    std::stringstream ss;
    ss << "CPU Processor: from " << BitDepthToString(in)
       << " to "  << BitDepthToString(out)
       << " oFlags " << oFlags
       << " fFlags " << fFlags
       << " ops :";
    for(const auto & op : ops)
    {
        ss << " " << op->getCacheID();
    }

    m_cacheID = ss.str();
}

void CPUProcessor::Impl::apply(ImageDesc & imgDesc) const
{
    m_scanlineBuilder->init(imgDesc);

    float * rgbaBuffer = nullptr;
    long numPixels = 0;

    while(true)
    {
        m_scanlineBuilder->prepRGBAScanline(&rgbaBuffer, numPixels);
        if(numPixels == 0) break;
        if(!rgbaBuffer)
            throw Exception("Cannot apply transform; null image.");

        for(const auto & op : m_cpuOps)
        {
            op->apply(rgbaBuffer, rgbaBuffer, numPixels);
        }
        
        m_scanlineBuilder->finishRGBAScanline();
    }
}

void CPUProcessor::Impl::apply(const ImageDesc & srcImgDesc, ImageDesc & dstImgDesc) const
{
    m_scanlineBuilder->init(srcImgDesc, dstImgDesc);

    float * rgbaBuffer = nullptr;
    long numPixels = 0;

    while(true)
    {
        m_scanlineBuilder->prepRGBAScanline(&rgbaBuffer, numPixels);
        if(numPixels == 0) break;
        if(!rgbaBuffer)
            throw Exception("Cannot apply transform; null image.");

        for(const auto & op : m_cpuOps)
        {
            op->apply(rgbaBuffer, rgbaBuffer, numPixels);
        }
        
        m_scanlineBuilder->finishRGBAScanline();
    }
}

void CPUProcessor::Impl::applyRGB(void * pixel) const
{
    switch(m_inBitDepth)
    {
        case BIT_DEPTH_UINT8:
        {
            uint8_t * p = reinterpret_cast<uint8_t *>(pixel);
            uint8_t v[8]{p[0], p[1], p[2], 0};
            applyRGBA(v);
            p[0] = v[0];
            p[1] = v[1];
            p[2] = v[2];
            break;
        }
        case BIT_DEPTH_UINT16:
        {
            uint16_t * p = reinterpret_cast<uint16_t *>(pixel);
            uint16_t v[8]{p[0], p[1], p[2], 0};
            applyRGBA(v);
            p[0] = v[0];
            p[1] = v[1];
            p[2] = v[2];
            break;
        }
        case BIT_DEPTH_F16:
        {
            half * p = reinterpret_cast<half *>(pixel);
            half v[8]{p[0], p[1], p[2], 0};
            applyRGBA(v);
            p[0] = v[0];
            p[1] = v[1];
            p[2] = v[2];
            break;
        }
        case BIT_DEPTH_F32:
        {
            float * p = reinterpret_cast<float *>(pixel);
            float v[8]{p[0], p[1], p[2], 0.0f};
            applyRGBA(v);
            p[0] = v[0];
            p[1] = v[1];
            p[2] = v[2];
            break;
        }
        case BIT_DEPTH_UINT10:
        case BIT_DEPTH_UINT12:
        case BIT_DEPTH_UINT14:
        case BIT_DEPTH_UINT32:
        case BIT_DEPTH_UNKNOWN:
            throw Exception("Cannot apply transform; Unsupported bit-depths.");
            break;
    }
}

void CPUProcessor::Impl::applyRGBA(void * pixel) const
{
    if(m_inBitDepth!=m_outBitDepth)
    {
        throw Exception("Cannot apply transform; bit-depths are different.");
    }

    float v[4];
    m_inBitDepthOp->apply(pixel, v, 1);

    for(const auto & op : m_cpuOps)
    {
        op->apply(v, v, 1);
    }

    m_outBitDepthOp->apply(v, pixel, 1);
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

void CPUProcessor::applyRGB(void * pixel) const
{
    getImpl()->applyRGB(pixel);
}

void CPUProcessor::applyRGBA(void * pixel) const
{
    getImpl()->applyRGBA(pixel);
}

}
OCIO_NAMESPACE_EXIT



///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut1D/Lut1DOpData.h"
#include "UnitTest.h"
#include "UnitTestUtils.h"


OCIO_ADD_TEST(CPUProcessor, flag_composition)
{
    // The test validates the build of a custom optimization flag.

    OCIO::OptimizationFlags customFlags = OCIO::OPTIMIZATION_LOSSLESS;

    OCIO_CHECK_EQUAL((customFlags & OCIO::OPTIMIZATION_COMP_LUT1D), 
                     OCIO::OPTIMIZATION_NONE);

    customFlags
        = OCIO::OptimizationFlags(customFlags | OCIO::OPTIMIZATION_COMP_LUT1D);

    OCIO_CHECK_EQUAL((customFlags & OCIO::OPTIMIZATION_COMP_LUT1D), 
                     OCIO::OPTIMIZATION_COMP_LUT1D);
}


// TODO: CPUProcessor being part of the OCIO public API limits the ability 
//       to inspect the CPUProcessor instance content i.e. the list of CPUOps. 
//       Even a successful apply could hide a major performance hit because of
//       a missing/partial optimization. 


template<OCIO::BitDepth inBD, OCIO::BitDepth outBD, unsigned line>
OCIO::ConstCPUProcessorRcPtr ComputeValues(OCIO::ConstProcessorRcPtr processor, 
                                           const void * inImg,
                                           OCIO::ChannelOrdering inChans, 
                                           const void * resImg, 
                                           OCIO::ChannelOrdering outChans, 
                                           long numPixels,
                                           // Default value to nan to break any float comparisons
                                           // as a valid error threshold is mandatory in that case.
                                           float absErrorThreshold
                                                = std::numeric_limits<float>::quiet_NaN())
{
    typedef typename OCIO::BitDepthInfo<inBD>::Type inType;
    typedef typename OCIO::BitDepthInfo<outBD>::Type outType;

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;

    OCIO_CHECK_NO_THROW(cpuProcessor 
        = processor->getOptimizedCPUProcessor(inBD, outBD, 
                                              OCIO::OPTIMIZATION_DEFAULT,
                                              OCIO::FINALIZATION_DEFAULT));

    size_t numChannels = 4;
    if(outChans==OCIO::CHANNEL_ORDERING_RGB || outChans==OCIO::CHANNEL_ORDERING_BGR)
    {
        numChannels = 3;
    }
    const size_t numValues = size_t(numPixels * numChannels);

    const OCIO::PackedImageDesc srcImgDesc((void *)inImg, numPixels, 1,
                                           inChans,
                                           sizeof(inType),
                                           OCIO::AutoStride,
                                           OCIO::AutoStride);

    std::vector<outType> out(numValues);
    OCIO::PackedImageDesc dstImgDesc(&out[0], numPixels, 1,
                                     outChans,
                                     sizeof(outType),
                                     OCIO::AutoStride,
                                     OCIO::AutoStride);

    OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

    const outType * res = reinterpret_cast<const outType*>(resImg);

    for(size_t idx=0; idx<numValues; ++idx)
    {
        if(OCIO::BitDepthInfo<outBD>::isFloat)
        {
            OCIO_CHECK_CLOSE_FROM(out[idx], res[idx], absErrorThreshold, line);
        }
        else
        {
            OCIO_CHECK_EQUAL_FROM(out[idx], res[idx], line);
        }
    }

    return cpuProcessor;
}

OCIO_ADD_TEST(CPUProcessor, with_one_matrix)
{
    // The unit test validates that pixel formats are correctly 
    // processed when the op list contains only one arbitrary Op
    // (except a 1D LUT one which has dedicated optimizations).

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::MatrixTransformRcPtr transform = OCIO::MatrixTransform::Create();
    const float offset4[4] = { 1.4002f, 0.4005f, 0.8007f, 0.5f };
    transform->setOffset( offset4 );

    OCIO::ConstProcessorRcPtr processor; 
    OCIO_CHECK_NO_THROW(processor = config->getProcessor(transform));

    const unsigned NB_PIXELS = 3;

    const std::vector<float> f_inImg =
        {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
            0.1023f,  0.5045f,  1.5089f,  1.0f,
            1.0000f,  1.2500f,  1.9900f,  0.0f  };

    {
        const std::vector<float> resImg
            = { 0.4002f, -0.3995f,  0.7007f,  0.5000f,
                1.5025f,  0.9050f,  2.3096f,  1.5000f,
                2.4002f,  1.6505f,  2.7907f,  0.5000f };

        ComputeValues<OCIO::BIT_DEPTH_F32, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                     &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    {
        const std::vector<float> resImg
            = { -0.1993f, -0.3995f,  1.3002f,  0.5000f,
                 0.9030f,  0.9050f,  2.9091f,  1.5000f,
                 1.8007f,  1.6505f,  3.3902f,  0.5000f };

        ComputeValues<OCIO::BIT_DEPTH_F32, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &f_inImg[0], OCIO::CHANNEL_ORDERING_BGRA, 
                                                     &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    {
        const std::vector<float> resImg
            = {  -0.500000f,  0.000700f, 0.300500f, 1.400200f,
                  0.602300f,  1.305199f, 1.909399f, 2.400200f,
                  1.500000f,  2.050699f, 2.390500f, 1.400200f  };

        ComputeValues<OCIO::BIT_DEPTH_F32, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &f_inImg[0], OCIO::CHANNEL_ORDERING_ABGR, 
                                                     &resImg[0],  OCIO::CHANNEL_ORDERING_ABGR, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    {
        const std::vector<float> resImg
            = { 0.7007f, -0.3995f,  0.4002f,  0.5000f,
                2.3096f,  0.9050f,  1.5025f,  1.5000f,
                2.7907f,  1.6505f,  2.4002f,  0.5000f };

        ComputeValues<OCIO::BIT_DEPTH_F32, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                     &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    {
        const std::vector<float> resImg
            = { 0.5000f, 0.7007f, -0.3995f, 0.4002f,
                1.5000f, 2.3096f,  0.9050f, 1.5025f,
                0.5000f, 2.7907f,  1.6505f, 2.4002f  };

        ComputeValues<OCIO::BIT_DEPTH_F32, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                     &resImg[0],  OCIO::CHANNEL_ORDERING_ABGR, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    {
        const std::vector<float> inImg =
            {  -1.0000f, -0.8000f, -0.1000f,
                0.1023f,  0.5045f,  1.5089f,
                1.0000f,  1.2500f,  1.9900f  };

        const std::vector<float> resImg
            = { 0.4002f, -0.3995f,  0.7007f,
                1.5025f,  0.9050f,  2.3096f,
                2.4002f,  1.6505f,  2.7907f };

        ComputeValues<OCIO::BIT_DEPTH_F32, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &inImg[0],  OCIO::CHANNEL_ORDERING_RGB, 
                                                     &resImg[0], OCIO::CHANNEL_ORDERING_RGB, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    {
        const std::vector<float> inImg =
            {  -1.0000f, -0.8000f, -0.1000f,
                0.1023f,  0.5045f,  1.5089f,
                1.0000f,  1.2500f,  1.9900f  };

        const std::vector<float> resImg
            = { -0.199299f, -0.399500f, 1.300199f,
                 0.902999f,  0.905000f, 2.909100f,
                 1.800699f,  1.650500f, 3.390200f };

        ComputeValues<OCIO::BIT_DEPTH_F32, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &inImg[0],  OCIO::CHANNEL_ORDERING_BGR, 
                                                     &resImg[0], OCIO::CHANNEL_ORDERING_BGR, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    {
        const std::vector<float> inImg =
            {  -1.0000f, -0.8000f, -0.1000f,
                0.1023f,  0.5045f,  1.5089f,
                1.0000f,  1.2500f,  1.9900f  };

        const std::vector<float> resImg
            = { 0.7007f, -0.3995f,  0.4002f,
                2.3096f,  0.9050f,  1.5025f,
                2.7907f,  1.6505f,  2.4002f };

        ComputeValues<OCIO::BIT_DEPTH_F32, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &inImg[0],  OCIO::CHANNEL_ORDERING_RGB, 
                                                     &resImg[0], OCIO::CHANNEL_ORDERING_BGR, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    {
        const std::vector<float> inImg =
            {  -1.0000f, -0.8000f, -0.1000f,
                0.1023f,  0.5045f,  1.5089f,
                1.0000f,  1.2500f,  1.9900f  };

        const std::vector<float> resImg
            = { 0.7007f, -0.3995f,  0.4002f, 0.5f,
                2.3096f,  0.9050f,  1.5025f, 0.5f,
                2.7907f,  1.6505f,  2.4002f, 0.5f   };

        ComputeValues<OCIO::BIT_DEPTH_F32, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &inImg[0],  OCIO::CHANNEL_ORDERING_RGB, 
                                                     &resImg[0], OCIO::CHANNEL_ORDERING_BGRA, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    {
        const std::vector<float> inImg =
            {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
                0.1023f,  0.5045f,  1.5089f,  1.0f,
                1.0000f,  1.2500f,  1.9900f,  0.0f  };

        const std::vector<float> resImg
            = { 0.7007f, -0.3995f,  0.4002f,
                2.3096f,  0.9050f,  1.5025f,
                2.7907f,  1.6505f,  2.4002f   };

        ComputeValues<OCIO::BIT_DEPTH_F32, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &inImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                     &resImg[0], OCIO::CHANNEL_ORDERING_BGR, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    const std::vector<uint16_t> i_inImg =
        {    0,      8,    32,  0,
            64,    128,   256,  0,
          5120,  20140, 65535,  0  };

    {
        const std::vector<float> resImg 
            = { 1.40020000f,  0.40062206f,  0.80118829f,  0.5f,
                1.40117657f,  0.40245315f,  0.80460631f,  0.5f,
                1.47832620f,  0.70781672f,  1.80070000f,  0.5f };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, 
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                     &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                     NB_PIXELS, 
                                                     1e-5f);
    }

    {
        const std::vector<uint16_t> resImg
            = { 65535, 26255, 52506, 32768,
                65535, 26375, 52730, 32768,
                65535, 46387, 65535, 32768 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, 
                      OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                        &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                        &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                        NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = { 52506, 26255, 65535, 32768,
                52730, 26375, 65535, 32768,
                65535, 46387, 65535, 32768 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                        &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                        &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA, 
                                                        NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = { 52506, 26255, 65535,
                52730, 26375, 65535,
                65535, 46387, 65535 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                        &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                        &resImg[0],  OCIO::CHANNEL_ORDERING_BGR, 
                                                        NB_PIXELS);
    }

    {
        const std::vector<uint8_t> resImg
            = { 255, 102, 204, 128,
                255, 103, 205, 128,
                255, 180, 255, 128 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT8, __LINE__>(processor, 
                                                       &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                       &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                       NB_PIXELS);
    }

    {
        const std::vector<uint8_t> resImg
            = { 204, 102, 255,
                205, 103, 255,
                255, 180, 255 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT8, __LINE__>(processor, 
                                                       &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                       &resImg[0],  OCIO::CHANNEL_ORDERING_BGR, 
                                                       NB_PIXELS);
    }

    {
        const std::vector<uint8_t> resImg
            = { 128, 204, 102, 255,
                128, 205, 103, 255,
                128, 255, 180, 255 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT8, __LINE__>(processor, 
                                                       &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                       &resImg[0],  OCIO::CHANNEL_ORDERING_ABGR, 
                                                       NB_PIXELS);
    }
}

OCIO_ADD_TEST(CPUProcessor, with_one_1d_lut)
{
    // The unit test validates that pixel formats are correctly 
    // processed when the op list only contains one 1D LUT because it 
    // has a dedicated optimization when the input bit-depth is an integer type.

    const std::string filePath
        = std::string(OCIO::getTestFilesDir()) + "/lut1d_5.spi1d";

    OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
    transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    transform->setSrc(filePath.c_str());
    transform->setInterpolation(OCIO::INTERP_LINEAR);

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ConstProcessorRcPtr processor; 
    OCIO_CHECK_NO_THROW(processor = config->getProcessor(transform));

    const unsigned NB_PIXELS = 4;

    const std::vector<float> f_inImg =
        {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
            0.1002f,  0.2509f,  0.5009f,  1.0f,
            0.5505f,  0.7090f,  0.9099f,  1.0f,
            1.0000f,  1.2500f,  1.9900f,  0.0f  };

    {
        const std::vector<float> resImg
            = {  0,            0,            0,            0,
                 0.03728949f,  0.10394855f,  0.24695572f,  1,
                 0.29089212f,  0.50935059f,  1.91091322f,  1,
                64,           64,           64,            0 };

        ComputeValues<OCIO::BIT_DEPTH_F32,
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                     &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                     NB_PIXELS, 
                                                     1e-7f);
    }

    {
        const std::vector<uint16_t> resImg
            = {     0,     0,     0,     0,
                 2444,  6812, 16184, 65535,
                19064, 33380, 65535, 65535,
                65535, 65535, 65535,     0 };

        ComputeValues<OCIO::BIT_DEPTH_F32,
                      OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                        &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                        &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                        NB_PIXELS);
    }

    const std::vector<uint16_t> i_inImg =
        {    0,      8,    32,     0,
            64,    128,   256,    32,
           512,   1024,  2048,    64,
          5120,  20480, 65535,   512 };

    {
        const std::vector<float> resImg
            = {  0,           0.00036166f, 0.00144666f, 0,
                 0.00187417f, 0.00271759f, 0.00408672f, 0.00048828f,
                 0.00601041f, 0.00912247f, 0.01456576f, 0.00097657f,
                 0.03030112f, 0.13105739f, 64,          0.00781261f };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                     &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                     &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                     NB_PIXELS, 1e-7f);
    }

    {
        const std::vector<uint16_t> resImg
            = {     0,    24,    95,     0,
                  123,   178,   268,    32,
                  394,   598,   955,    64,
                 1986,  8589, 65535,   512 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                        &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                        &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                        NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = {    95,    24,     0,     0,
                  268,   178,   123,    32,
                  955,   598,   394,    64,
                65535,  8589,  1986,   512 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                        &i_inImg[0], OCIO::CHANNEL_ORDERING_BGRA, 
                                                        &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                        NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = {    95,    24,     0,     0,
                  268,   178,   123,    32,
                  955,   598,   394,    64,
                65535,  8589,  1986,   512 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                        &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                        &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA, 
                                                        NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = {    95,    24,     0,
                  268,   178,   123,
                  955,   598,   394,
                65535,  8589,  1986 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                        &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                        &resImg[0],  OCIO::CHANNEL_ORDERING_BGR, 
                                                        NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = {     0,    24,    95,     0,
                  123,   178,   268,    32,
                  394,   598,   955,    64,
                 1986,  8589, 65535,    512 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                        &i_inImg[0], OCIO::CHANNEL_ORDERING_BGRA, 
                                                        &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA, 
                                                        NB_PIXELS);
    }

    {
        const std::vector<uint16_t> my_i_inImg =
            {    0,      8,    32,
                64,    128,   256,
               512,   1024,  2048,
              5120,  20480, 65535  };

        const std::vector<uint16_t> resImg
            = {    95,    24,     0,     0,
                  268,   178,   123,     0,
                  955,   598,   394,     0,
                65535,  8589,  1986,     0 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16,
                      OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                        &my_i_inImg[0], OCIO::CHANNEL_ORDERING_RGB, 
                                                        &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA, 
                                                        NB_PIXELS);
    }
}

OCIO_ADD_TEST(CPUProcessor, with_several_ops)
{
    // The unit test validates that pixel formats are correctly 
    // processed when the op list starts or ends with a 1D LUT because it 
    // has a dedicated optimization when the input bit-depth is an integer type.

    const std::string SIMPLE_PROFILE =
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: " + std::string(OCIO::getTestFilesDir()) + "\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "  scene_linear: cs2\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: cs1}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    allocation: uniform\n";

    // Step 1: The 1D LUT is the last Op.

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<MatrixTransform> { offset: [-0.19, 0.19, -0.00019, 0] }\n"
            "        - !<FileTransform>   { src: lut1d_5.spi1d, interpolation: linear }\n";

        const std::string str = SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        OCIO::ConstProcessorRcPtr processor; 
        OCIO_CHECK_NO_THROW(processor = config->getProcessor("cs1", "cs2"));

        const unsigned NB_PIXELS = 4;

        const std::vector<float> f_inImg =
            {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
                0.1002f,  0.2509f,  0.5009f,  1.0f,
                0.5505f,  0.7090f,  0.9099f,  1.0f,
                1.0000f,  1.2500f,  1.9900f,  0.0f  };

        {
            const std::vector<float> resImg
                = {  0.0f,         0.0f,         0.0f,         0.0f,
                     0.0f,         0.20273837f,  0.24680146f,  1.0f,
                     0.15488569f,  1.69210147f,  1.90666747f,  1.0f,
                     0.81575858f, 64.0f,        64.0f,         0.0f };

            ComputeValues<OCIO::BIT_DEPTH_F32, 
                          OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                         &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                         &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                         NB_PIXELS, 1e-7f);
        }

        {
            const std::vector<uint16_t> resImg
                = {     0,     0,     0,     0,
                        0, 13286, 16174, 65535,
                    10150, 65535, 65535, 65535,
                    53461, 65535, 65535,     0 };

            ComputeValues<OCIO::BIT_DEPTH_F32, 
                          OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                            &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                            &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                            NB_PIXELS);
        }

        const std::vector<uint16_t> i_inImg =
            {    0,      8,    32,  0,
                64,    128,   256,  65535,
               512,   1024,  2048,  0,
              5120,  20480, 65535,  65535  };

        {
            const std::vector<float> resImg
                = {  0.0f,  0.07789713f,  0.00088374f,  0.0f,
                     0.0f,  0.07871927f,  0.00396248f,  1.0f,
                     0.0f,  0.08474064f,  0.01450117f,  0.0f,
                     0.0f,  0.24826171f, 56.39490891f,  1.0f };

            ComputeValues<OCIO::BIT_DEPTH_UINT16, 
                          OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                         &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                         &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                         NB_PIXELS, 1e-7f);
        }

        {
            const std::vector<uint16_t> resImg
                = {     0,  5105,    58,     0,
                        0,  5159,   260,     65535,
                        0,  5553,   950,     0,
                        0, 16270, 65535,     65535 };

            ComputeValues<OCIO::BIT_DEPTH_UINT16, 
                          OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                            &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                            &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                            NB_PIXELS);
        }

        {
            const std::vector<uint16_t> resImg
                = {     0,  5105,     0,     0,
                        0,  5159,   112,     65535,
                        0,  5553,   388,     0,
                    53461, 16270,  1982,     65535 };

            ComputeValues<OCIO::BIT_DEPTH_UINT16, 
                          OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                            &i_inImg[0], OCIO::CHANNEL_ORDERING_BGRA, 
                                                            &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                            NB_PIXELS);
        }

        {
            const std::vector<uint16_t> resImg
                = {    58,  5105,     0,     0,
                      260,  5159,     0,     65535,
                      950,  5553,     0,     0,
                    65535, 16270,     0,     65535 };

            ComputeValues<OCIO::BIT_DEPTH_UINT16, 
                          OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                            &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                            &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA, 
                                                            NB_PIXELS);
        }

        {
            const std::vector<uint16_t> resImg
                = {     0,  5105,     0,     0,
                      112,  5159,     0,     65535,
                      388,  5553,     0,     0,
                     1982, 16270, 53461,     65535 };

            ComputeValues<OCIO::BIT_DEPTH_UINT16, 
                          OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                            &i_inImg[0], OCIO::CHANNEL_ORDERING_BGRA, 
                                                            &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA, 
                                                            NB_PIXELS);
        }
    }

    // Step 2: The 1D LUT is the first Op.

    {
        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FileTransform>   { src: lut1d_5.spi1d, interpolation: linear }\n"
            "        - !<MatrixTransform> { offset: [-0.19, 0.19, -0.00019, 0] }\n";

        const std::string str = SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        OCIO::ConstProcessorRcPtr processor; 
        OCIO_CHECK_NO_THROW(processor = config->getProcessor("cs1", "cs2"));

        const unsigned NB_PIXELS = 4;

        const std::vector<float> f_inImg =
            {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
                0.1002f,  0.2509f,  0.5009f,  1.0f,
                0.5505f,  0.7090f,  0.9099f,  1.0f,
                1.0000f,  1.2500f,  1.9900f,  0.0f  };

        {
            const std::vector<float> resImg
                = { -0.18999999f,  0.18999999f, -0.00019000f,  0.0f,
                    -0.15271049f,  0.29394856f,  0.24676571f,  1.0f,
                     0.10089212f,  0.69935059f,  1.91072320f,  1.0f,
                    63.81000137f, 64.19000244f, 63.99980927f,  0.0f };

            ComputeValues<OCIO::BIT_DEPTH_F32, 
                          OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                         &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                         &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                         NB_PIXELS, 1e-7f);
        }

        {
            const std::vector<uint16_t> resImg
                = {     0, 12452,     0,     0,
                        0, 19264, 16172, 65535,
                     6612, 45832, 65535, 65535,
                    65535, 65535, 65535,     0 };

            ComputeValues<OCIO::BIT_DEPTH_F32, 
                          OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                            &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                            &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                            NB_PIXELS);
        }

        const std::vector<uint16_t> i_inImg =
            {    0,      8,    32,  0,
                64,    128,   256,  0,
               512,   1024,  2048,  0,
              5120,  20480, 65535,  0  };
        {
            const std::vector<float> resImg
                = { -0.18999999f, 0.19036166f,  0.00125666f,  0.0f,
                    -0.18812581f, 0.19271758f,  0.00389672f,  0.0f,
                    -0.18398958f, 0.19912247f,  0.01437576f,  0.0f,
                    -0.15969887f, 0.32105737f, 63.99980927f,  0.0f };

            ComputeValues<OCIO::BIT_DEPTH_UINT16, 
                          OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                         &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                         &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                         NB_PIXELS, 1e-7f);
        }

        {
            const std::vector<uint16_t> resImg
                = {     0, 12475,     0,     0,
                      110, 12630,     0,     0,
                      381, 13049,     0,     0,
                     1973, 21040, 65535,     0 };

            ComputeValues<OCIO::BIT_DEPTH_UINT16, 
                          OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                            &i_inImg[0], OCIO::CHANNEL_ORDERING_BGRA, 
                                                            &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA, 
                                                            NB_PIXELS);
        }
    }

    // Step 3: The 1D LUT is the first and the last Op.

    {

        const std::string strEnd =
            "    from_reference: !<GroupTransform>\n"
            "      children:\n"
            "        - !<FileTransform>   { src: lut1d_5.spi1d, interpolation: linear }\n"
            "        - !<MatrixTransform> { offset: [-0.19, 0.19, -0.00019, 0] }\n"
            "        - !<FileTransform>   { src: lut1d_4.spi1d, interpolation: linear }\n";

        const std::string str = SIMPLE_PROFILE + strEnd;

        std::istringstream is;
        is.str(str);

        OCIO::ConstConfigRcPtr config;
        OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OCIO_CHECK_NO_THROW(config->sanityCheck());

        OCIO::ConstProcessorRcPtr processor; 
        OCIO_CHECK_NO_THROW(processor = config->getProcessor("cs1", "cs2"));

        const unsigned NB_PIXELS = 4;

        const std::vector<float> f_inImg =
            {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
                0.1002f,  0.2509f,  0.5009f,  1.0f,
                0.5505f,  0.7090f,  0.9099f,  1.0f,
                1.0000f,  1.2500f,  1.9900f,  0.0f  };

        {
            const std::vector<float> resImg
                = { -0.79690927f, -0.06224250f, -0.42994320f,  0.0f,
                    -0.72481626f,  0.13872468f,  0.04750441f,  1.0f,
                    -0.23451784f,  0.92250210f,  3.26448941f,  1.0f,
                     3.43709063f,  3.43709063f,  3.43709063f,  0.0f };

            ComputeValues<OCIO::BIT_DEPTH_F32, 
                          OCIO::BIT_DEPTH_F32, __LINE__>(processor, 
                                                         &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA, 
                                                         &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA, 
                                                         NB_PIXELS, 1e-7f);
        }

        const std::vector<uint16_t> i_inImg =
            {    0,      8,    32,  0,
                64,    128,   256,  0,
               512,   1024,  2048,  0,
              5120,  20480, 65535,  0  };

        {
            const std::vector<uint16_t> resImg
                = {     0,     0,     0,     0,
                        0,     0,     0,     0,
                        0,     0,     0,     0,
                        0, 12526, 65535,     0 };

            ComputeValues<OCIO::BIT_DEPTH_UINT16, 
                          OCIO::BIT_DEPTH_UINT16, __LINE__>(processor, 
                                                            &i_inImg[0], OCIO::CHANNEL_ORDERING_BGRA, 
                                                            &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA, 
                                                            NB_PIXELS);
        }
    }
}

OCIO_ADD_TEST(CPUProcessor, image_desc)
{
    // The tests validate the image description types when using the same buffer image.

    const std::string SIMPLE_PROFILE =
        "ocio_profile_version: 2\n"
        "\n"
        "search_path: " + std::string(OCIO::getTestFilesDir()) + "\n"
        "strictparsing: true\n"
        "luma: [0.2126, 0.7152, 0.0722]\n"
        "\n"
        "roles:\n"
        "  default: cs1\n"
        "  scene_linear: cs2\n"
        "\n"
        "displays:\n"
        "  sRGB:\n"
        "    - !<View> {name: Raw, colorspace: cs1}\n"
        "\n"
        "colorspaces:\n"
        "  - !<ColorSpace>\n"
        "    name: cs1\n"
        "    allocation: uniform\n"
        "\n"
        "  - !<ColorSpace>\n"
        "    name: cs2\n"
        "    allocation: uniform\n";

    const std::string strEnd =
        "    from_reference: !<GroupTransform>\n"
        "      children:\n"
        "        - !<MatrixTransform> { offset: [-0.19, 0.19, -0.00019, 0.5] }\n"
        "        - !<FileTransform>   { src: lut1d_5.spi1d, interpolation: linear }\n";

    const std::string str = SIMPLE_PROFILE + strEnd;

    std::istringstream is;
    is.str(str);

    OCIO::ConstConfigRcPtr config;
    OCIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
    OCIO_CHECK_NO_THROW(config->sanityCheck());

    OCIO::ConstProcessorRcPtr processor; 
    OCIO_CHECK_NO_THROW(processor = config->getProcessor("cs1", "cs2"));

    const std::vector<float> f_rInImg =
        {  -1.0000f,
            0.1002f,
            0.5505f,
            1.0000f,  };

    const std::vector<float> f_gInImg =
        {            -0.8000f,
                      0.2509f,
                      0.7090f,
                      1.2500f  };

    const std::vector<float> f_bInImg =
        {                       -0.1000f,
                                 0.5009f,
                                 0.9099f,
                                 1.9900f  };

    const std::vector<float> f_aInImg =
        {                                   0.0f,
                                            1.0f,
                                            0.5f,
                                            0.0f  };

    const std::vector<float> f_rOutImg
        = {  0.0f,
             0.0f,
             0.15488569f,
             0.81575858f };

    const std::vector<float> f_gOutImg
        = {                0.0f,
                           0.20273837f,
                           1.69210147f,
                          64.0f };

    const std::vector<float> f_bOutImg
        = {                              0.0f,
                                         0.24680146f,
                                         1.90666747f,
                                        64.0f };

    const std::vector<float> f_aOutImg
        = {                                            0.5f,
                                                       1.5f,
                                                       1.0f,
                                                       0.5f };

    {
        // Packed Image Description with RGBA image.

        std::vector<float> img =
            {  f_rInImg[0], f_gInImg[0], f_bInImg[0], f_aInImg[0],
               f_rInImg[1], f_gInImg[1], f_bInImg[1], f_aInImg[1],
               f_rInImg[2], f_gInImg[2], f_bInImg[2], f_aInImg[2],
               f_rInImg[3], f_gInImg[3], f_bInImg[3], f_aInImg[3] };

        const std::vector<float> res
            {  f_rOutImg[0], f_gOutImg[0], f_bOutImg[0], f_aOutImg[0],
               f_rOutImg[1], f_gOutImg[1], f_bOutImg[1], f_aOutImg[1],
               f_rOutImg[2], f_gOutImg[2], f_bOutImg[2], f_aOutImg[2],
               f_rOutImg[3], f_gOutImg[3], f_bOutImg[3], f_aOutImg[3] };

        OCIO::ConstCPUProcessorRcPtr cpu; 
        OCIO_CHECK_NO_THROW(cpu = processor->getDefaultCPUProcessor());

        OCIO::PackedImageDesc desc(&img[0], 2, 2, 4);
        OCIO_CHECK_NO_THROW(cpu->apply(desc));

        for(size_t idx=0; idx<img.size(); ++idx)
        {
            OCIO_CHECK_CLOSE(img[idx], res[idx], 1e-7f);
        }
    }

    {
        // Packed Image Description with RGB image.

        std::vector<float> img =
            {  f_rInImg[0], f_gInImg[0], f_bInImg[0],
               f_rInImg[1], f_gInImg[1], f_bInImg[1],
               f_rInImg[2], f_gInImg[2], f_bInImg[2],
               f_rInImg[3], f_gInImg[3], f_bInImg[3] };

        const std::vector<float> res
            {  f_rOutImg[0], f_gOutImg[0], f_bOutImg[0],
               f_rOutImg[1], f_gOutImg[1], f_bOutImg[1],
               f_rOutImg[2], f_gOutImg[2], f_bOutImg[2],
               f_rOutImg[3], f_gOutImg[3], f_bOutImg[3] };

        OCIO::ConstCPUProcessorRcPtr cpu; 
        OCIO_CHECK_NO_THROW(cpu = processor->getDefaultCPUProcessor());

        OCIO::PackedImageDesc desc(&img[0], 4, 1, 3);
        OCIO_CHECK_NO_THROW(cpu->apply(desc));

        for(size_t idx=0; idx<img.size(); ++idx)
        {
            OCIO_CHECK_CLOSE(img[idx], res[idx], 1e-7f);
        }
    }

    {
        // Planar Image Description with R/G/B/A.

        std::vector<float> imgRed   = f_rInImg;
        std::vector<float> imgGreen = f_gInImg;
        std::vector<float> imgBlue  = f_bInImg;
        std::vector<float> imgAlpha = f_aInImg;

        OCIO::ConstCPUProcessorRcPtr cpu; 
        OCIO_CHECK_NO_THROW(cpu = processor->getDefaultCPUProcessor());

        OCIO::PlanarImageDesc desc(&imgRed[0], &imgGreen[0], &imgBlue[0], &imgAlpha[0], 2, 2);
        OCIO_CHECK_NO_THROW(cpu->apply(desc));

        for(size_t idx=0; idx<imgRed.size(); ++idx)
        {
            OCIO_CHECK_CLOSE(imgRed[idx],   f_rOutImg[idx], 1e-7f);
            OCIO_CHECK_CLOSE(imgGreen[idx], f_gOutImg[idx], 1e-7f);
            OCIO_CHECK_CLOSE(imgBlue[idx],  f_bOutImg[idx], 1e-7f);
            OCIO_CHECK_CLOSE(imgAlpha[idx], f_aOutImg[idx], 1e-7f);
        }
    }

    {
        // Planar Image Description with R/G/B.

        std::vector<float> imgRed   = f_rInImg;
        std::vector<float> imgGreen = f_gInImg;
        std::vector<float> imgBlue  = f_bInImg;

        OCIO::ConstCPUProcessorRcPtr cpu; 
        OCIO_CHECK_NO_THROW(cpu = processor->getDefaultCPUProcessor());

        OCIO::PlanarImageDesc desc(&imgRed[0], &imgGreen[0], &imgBlue[0], nullptr, 1, 4);
        OCIO_CHECK_NO_THROW(cpu->apply(desc));

        for(size_t idx=0; idx<imgRed.size(); ++idx)
        {
            OCIO_CHECK_CLOSE(imgRed[idx],   f_rOutImg[idx], 1e-7f);
            OCIO_CHECK_CLOSE(imgGreen[idx], f_gOutImg[idx], 1e-7f);
            OCIO_CHECK_CLOSE(imgBlue[idx],  f_bOutImg[idx], 1e-7f);
        }
    }
}

namespace
{

const unsigned NB_PIXELS = 6;

std::vector<float> inImgR =
    {  -1.000012f,
       -0.500012f,
        0.100012f,
        0.600012f,
        1.102312f,
        1.700012f  };

std::vector<float> inImgG =
    {              -0.800012f,
                   -0.300012f,
                    0.250012f,
                    0.800012f,
                    1.204512f,
                    1.800012f };

std::vector<float> inImgB =
    {                          -0.600012f,
                               -0.100012f,
                                0.450012f,
                                0.900012f,
                                1.508912f,
                                1.990012f };

std::vector<float> inImgA =
    {                                       0.005005f,
                                            0.405005f,
                                            0.905005f,
                                            0.005005f,
                                            1.005005f,
                                            0.095005f  };

std::vector<float> inImg =
    { inImgR[0], inImgG[0], inImgB[0], inImgA[0],
      inImgR[1], inImgG[1], inImgB[1], inImgA[1],
      inImgR[2], inImgG[2], inImgB[2], inImgA[2],
      inImgR[3], inImgG[3], inImgB[3], inImgA[3],
      inImgR[4], inImgG[4], inImgB[4], inImgA[4],
      inImgR[5], inImgG[5], inImgB[5], inImgA[5] };

const std::vector<float> resImgR = 
    {  0.4001879692f,
       0.9001880288f,
       1.500211954f,
       2.000211954f,
       2.502511978f,
       3.100212097f };

const std::vector<float> resImgG = 
    {                 -0.3995119929f,
                       0.1004880071f,
                       0.6505119801f,
                       1.200511932f,
                       1.60501194f,
                       2.200511932f };

const std::vector<float> resImgB = 
    {                                 0.2006880045f,
                                      0.7006880045f,
                                      1.250712037f,
                                      1.700711966f,
                                      2.309612036f,
                                      2.790712118f };

const std::vector<float> resImgA = 
    {                                                0.5057050f,
                                                     0.9057050f,
                                                     1.4057050f,
                                                     0.5057050f,
                                                     1.5057050f,
                                                     0.5957050f  };

const std::vector<float> resImg =
    { resImgR[0], resImgG[0], resImgB[0], resImgA[0],
      resImgR[1], resImgG[1], resImgB[1], resImgA[1],
      resImgR[2], resImgG[2], resImgB[2], resImgA[2],
      resImgR[3], resImgG[3], resImgB[3], resImgA[3],
      resImgR[4], resImgG[4], resImgB[4], resImgA[4],
      resImgR[5], resImgG[5], resImgB[5], resImgA[5] };


OCIO::ConstCPUProcessorRcPtr BuildCPUProcessor(OCIO::TransformDirection dir)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::MatrixTransformRcPtr transform = OCIO::MatrixTransform::Create();
    const float offset4[4] = { 1.4002f, 0.4005f, 0.8007f, 0.5007f };
    transform->setOffset(offset4);
    transform->setDirection(dir);

    OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);
    return processor->getDefaultCPUProcessor();
}

void Process(const OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
             const OCIO::PackedImageDesc & srcImgDesc,
             OCIO::PackedImageDesc & dstImgDesc,
             unsigned lineNo)
{
    OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

    const float * outImg = reinterpret_cast<float*>(dstImgDesc.getData());
    for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
    {
        OCIO_CHECK_CLOSE_FROM(outImg[4*pxl+0], resImg[4*pxl+0], 1e-7f, lineNo);
        OCIO_CHECK_CLOSE_FROM(outImg[4*pxl+1], resImg[4*pxl+1], 1e-7f, lineNo);
        OCIO_CHECK_CLOSE_FROM(outImg[4*pxl+2], resImg[4*pxl+2], 1e-7f, lineNo);
        OCIO_CHECK_CLOSE_FROM(outImg[4*pxl+3], resImg[4*pxl+3], 1e-7f, lineNo);
    }
}

void Process(const OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
             const OCIO::PlanarImageDesc & srcImgDesc,
             OCIO::PlanarImageDesc & dstImgDesc,
             unsigned lineNo)
{
    OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

    const float * outImgR = reinterpret_cast<float*>(dstImgDesc.getRData());
    const float * outImgG = reinterpret_cast<float*>(dstImgDesc.getGData());
    const float * outImgB = reinterpret_cast<float*>(dstImgDesc.getBData());
    const float * outImgA = reinterpret_cast<float*>(dstImgDesc.getAData());

    for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
    {
        OCIO_CHECK_CLOSE_FROM(outImgR[pxl], resImg[4*pxl+0], 1e-7f, lineNo);
        OCIO_CHECK_CLOSE_FROM(outImgG[pxl], resImg[4*pxl+1], 1e-7f, lineNo);
        OCIO_CHECK_CLOSE_FROM(outImgB[pxl], resImg[4*pxl+2], 1e-7f, lineNo);
        if(outImgA)
        {
            OCIO_CHECK_CLOSE_FROM(outImgA[pxl], resImg[4*pxl+3], 1e-7f, lineNo);
        }
    }
}

} // anon


OCIO_ADD_TEST(CPUProcessor, planar_vs_packed)
{
    // The unit test validates different types for input and output imageDesc.

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_FORWARD));

    // 1. Process from Packed to Planar Image Desc using the forward transform.

    OCIO::PackedImageDesc srcImgDesc((void*)&inImg[0], NB_PIXELS, 1, 4);

    std::vector<float> outR(NB_PIXELS), outG(NB_PIXELS), outB(NB_PIXELS), outA(NB_PIXELS);
    OCIO::PlanarImageDesc dstImgDesc((void*)&outR[0], (void*)&outG[0], 
                                     (void*)&outB[0], (void*)&outA[0], 
                                     NB_PIXELS, 1);

    OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

    for(size_t idx=0; idx<NB_PIXELS; ++idx)
    {
        OCIO_CHECK_CLOSE(outR[idx], resImg[4*idx+0], 1e-7f);
        OCIO_CHECK_CLOSE(outG[idx], resImg[4*idx+1], 1e-7f);
        OCIO_CHECK_CLOSE(outB[idx], resImg[4*idx+2], 1e-7f);
        OCIO_CHECK_CLOSE(outA[idx], resImg[4*idx+3], 1e-7f);
    }


    // 2. Process from Planar to Packed Image Desc using the inverse transform.

    OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_INVERSE));

    std::vector<float> outImg(NB_PIXELS*4, -1.0f);
    OCIO::PackedImageDesc dstImgDesc2((void*)&outImg[0], NB_PIXELS, 1, 4);

    OCIO_CHECK_NO_THROW(cpuProcessor->apply(dstImgDesc, dstImgDesc2));

    for(size_t idx=0; idx<(NB_PIXELS*4); ++idx)
    {
        OCIO_CHECK_CLOSE(outImg[idx], inImg[idx], 1e-6f);
    }
}

OCIO_ADD_TEST(CPUProcessor, scanline_helper_packed)
{
    // Test the packed image description.

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_FORWARD));

    std::vector<float> outImg(NB_PIXELS*4);

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], NB_PIXELS, 1, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], NB_PIXELS, 1, 4);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 1, NB_PIXELS, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 1, NB_PIXELS, 4);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 2, 3, 4);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 3, 2, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 3, 2, 4);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 1, NB_PIXELS, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 3, 2, 4);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], NB_PIXELS, 1, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 3, 2, 4);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], NB_PIXELS, 1, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 1, NB_PIXELS, 4);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 1, NB_PIXELS, 4);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);

        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 1, NB_PIXELS, 
                                         OCIO::CHANNEL_ORDERING_RGBA);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 1, NB_PIXELS,
                                         OCIO::CHANNEL_ORDERING_RGBA,
                                         sizeof(float),
                                         OCIO::AutoStride,
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 1, NB_PIXELS,
                                         OCIO::CHANNEL_ORDERING_RGBA,
                                         sizeof(float),
                                         4*sizeof(float),
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 1, NB_PIXELS,
                                         4, // Number of channels
                                         sizeof(float),
                                         4*sizeof(float),
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 1, NB_PIXELS,
                                         4, // Number of channels
                                         OCIO::AutoStride,
                                         4*sizeof(float),
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 1, NB_PIXELS,
                                         4, // Number of channels
                                         OCIO::AutoStride,
                                         4*sizeof(float),
                                         4*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 3, 2,
                                         4, // Number of channels
                                         OCIO::AutoStride,
                                         OCIO::AutoStride,
                                         3*4*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }
}

OCIO_ADD_TEST(CPUProcessor, scanline_helper_planar)
{
    // Test the planar image description.

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_FORWARD));

    std::vector<float> outImgR(NB_PIXELS);
    std::vector<float> outImgG(NB_PIXELS);
    std::vector<float> outImgB(NB_PIXELS);
    std::vector<float> outImgA(NB_PIXELS);

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], NB_PIXELS, 1);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], NB_PIXELS, 1);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], NB_PIXELS, 1);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], 1, NB_PIXELS);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 2, 3);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], NB_PIXELS, 1);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 2, 3);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], 3, 2);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 2, 3);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], 
                                         3, 2,
                                         sizeof(float),
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 2, 3);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], 
                                         3, 2,
                                         sizeof(float),
                                         3*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 2, 3);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], 
                                         3, 2,
                                         OCIO::AutoStride,
                                         3*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 
                                         2, 3,
                                         OCIO::AutoStride,
                                         2*sizeof(float));

        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], 
                                         3, 2,
                                         OCIO::AutoStride,
                                         3*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 
                                         2, 3,
                                         sizeof(float),
                                         2*sizeof(float));

        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], 
                                         3, 2,
                                         OCIO::AutoStride,
                                         3*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 
                                         2, 3,
                                         sizeof(float),
                                         2*sizeof(float));

        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], nullptr, 
                                         3, 2,
                                         OCIO::AutoStride,
                                         3*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], nullptr, 
                                         2, 3,
                                         sizeof(float),
                                         2*sizeof(float));

        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], nullptr, 
                                         3, 2,
                                         OCIO::AutoStride,
                                         3*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }
}

OCIO_ADD_TEST(CPUProcessor, scanline_helper_tile)
{
    // Process tiles.

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_FORWARD));

    std::vector<float> outImg(NB_PIXELS*4);

    {
        // Pixels are { 1, 2, 3, 
        //              4, 5, 6  }

        // Copy the 1st pixel which should be untouched.
        outImg[(0 * 4) + 0] = resImg[(0 * 4) + 0];
        outImg[(0 * 4) + 1] = resImg[(0 * 4) + 1];
        outImg[(0 * 4) + 2] = resImg[(0 * 4) + 2];
        outImg[(0 * 4) + 3] = resImg[(0 * 4) + 3];
        // Copy the 4th pixel which should be untouched.
        outImg[(3 * 4) + 0] = resImg[(3 * 4) + 0];
        outImg[(3 * 4) + 1] = resImg[(3 * 4) + 1];
        outImg[(3 * 4) + 2] = resImg[(3 * 4) + 2];
        outImg[(3 * 4) + 3] = resImg[(3 * 4) + 3];

        // Only process the pixels = { 2, 3, 
        //                             5, 6  }

        OCIO::PackedImageDesc srcImgDesc(&inImg[4], 
                                         2, 2, 4,   // width=2, height=2, and nchannels=4
                                         sizeof(float), 
                                         4*sizeof(float),
                                         3*4*sizeof(float));

        OCIO::PackedImageDesc dstImgDesc(&outImg[4], 
                                         2, 2, 4,   // width=2, height=2, and nchannels=4
                                         sizeof(float), 
                                         4*sizeof(float),
                                         3*4*sizeof(float));

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
        {
            OCIO_CHECK_CLOSE(outImg[4*pxl+0], resImg[4*pxl+0], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[4*pxl+1], resImg[4*pxl+1], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[4*pxl+2], resImg[4*pxl+2], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[4*pxl+3], resImg[4*pxl+3], 1e-7f);
        }
    }

    {
        // Pixels are { 1, 2, 3, 
        //              4, 5, 6  }

        // Copy the 3rd pixel which should be untouched.
        outImg[(2 * 4) + 0] = resImg[(2 * 4) + 0];
        outImg[(2 * 4) + 1] = resImg[(2 * 4) + 1];
        outImg[(2 * 4) + 2] = resImg[(2 * 4) + 2];
        outImg[(2 * 4) + 3] = resImg[(2 * 4) + 3];
        // Copy the 6th pixel which should be untouched.
        outImg[(5 * 4) + 0] = resImg[(5 * 4) + 0];
        outImg[(5 * 4) + 1] = resImg[(5 * 4) + 1];
        outImg[(5 * 4) + 2] = resImg[(5 * 4) + 2];
        outImg[(5 * 4) + 3] = resImg[(5 * 4) + 3];

        // Only process the pixels = { 1, 2, 
        //                             4, 5 }

        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 
                                         2, 2, 4,   // width=2, height=2, and nchannels=4
                                         sizeof(float), 
                                         4*sizeof(float),
                                         3*4*sizeof(float));

        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 
                                         2, 2, 4,   // width=2, height=2, and nchannels=4
                                         sizeof(float), 
                                         4*sizeof(float),
                                         3*4*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        // Pixels are { 1, 2, 3, 
        //              4, 5, 6  }

        outImg = inImg; // Use an in-place image buffer.

        // Copy the 3rd pixel which should be untouched.
        outImg[(2 * 4) + 0] = resImg[(2 * 4) + 0];
        outImg[(2 * 4) + 1] = resImg[(2 * 4) + 1];
        outImg[(2 * 4) + 2] = resImg[(2 * 4) + 2];
        outImg[(2 * 4) + 3] = resImg[(2 * 4) + 3];
        // Copy the 6th pixel which should be untouched.
        outImg[(5 * 4) + 0] = resImg[(5 * 4) + 0];
        outImg[(5 * 4) + 1] = resImg[(5 * 4) + 1];
        outImg[(5 * 4) + 2] = resImg[(5 * 4) + 2];
        outImg[(5 * 4) + 3] = resImg[(5 * 4) + 3];

        // Only process the pixels = { 1, 2, 
        //                             4, 5 }

        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 
                                         2, 2, 4,   // width=2, height=2, and nchannels=4
                                         sizeof(float), 
                                         4*sizeof(float),
                                         3*4*sizeof(float));

        Process(cpuProcessor, dstImgDesc, dstImgDesc, __LINE__);
    }

}


OCIO_ADD_TEST(CPUProcessor, custom_scanlines)
{
    // Cases testing custom xStrideBytes and yStrideBytes values.

    const float magicNumber = 12345.6789f;

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_FORWARD));

    {
        // Pixels are { RGBA, RGBA, RGBA, x,
        //              RGBA, RGBA, RGBA, x  } where x is not a color channel.

        std::vector<float> img
            = { inImg[ 0], inImg[ 1], inImg[ 2], inImg[ 3],
                inImg[ 4], inImg[ 5], inImg[ 6], inImg[ 7],
                inImg[ 8], inImg[ 9], inImg[10], inImg[11],
                magicNumber,
                inImg[12], inImg[13], inImg[14], inImg[15],
                inImg[16], inImg[17], inImg[18], inImg[19],
                inImg[20], inImg[21], inImg[22], inImg[23],
                magicNumber };

        OCIO::PackedImageDesc srcImgDesc(&img[0], 
                                         3, 2, 4,
                                         OCIO::AutoStride,
                                         OCIO::AutoStride,
                                         // Bytes to the next line.
                                         3*4*sizeof(float)+sizeof(float));

        std::vector<float> outImg(NB_PIXELS*4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 
                                         NB_PIXELS, 1, 4);

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
        {
            OCIO_CHECK_CLOSE(outImg[4*pxl+0], resImg[4*pxl+0], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[4*pxl+1], resImg[4*pxl+1], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[4*pxl+2], resImg[4*pxl+2], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[4*pxl+3], resImg[4*pxl+3], 1e-7f);
        }
    }

    {
        // Pixels are { RxGxBxAx, RxGxBxAx, RxGxBxAx,
        //              RxGxBxAx, RxGxBxAx, RxGxBxAx  } where x is not a color channel.

        std::vector<float> img
            = { inImg[ 0], magicNumber, inImg[ 1], magicNumber, inImg[ 2], magicNumber, inImg[ 3], magicNumber,
                inImg[ 4], magicNumber, inImg[ 5], magicNumber, inImg[ 6], magicNumber, inImg[ 7], magicNumber,
                inImg[ 8], magicNumber, inImg[ 9], magicNumber, inImg[10], magicNumber, inImg[11], magicNumber,
                inImg[12], magicNumber, inImg[13], magicNumber, inImg[14], magicNumber, inImg[15], magicNumber,
                inImg[16], magicNumber, inImg[17], magicNumber, inImg[18], magicNumber, inImg[19], magicNumber,
                inImg[20], magicNumber, inImg[21], magicNumber, inImg[22], magicNumber, inImg[23], magicNumber };

        OCIO::PackedImageDesc srcImgDesc(&img[0], 
                                         3, 2, 4,
                                         // Bytes to the next channel.
                                         sizeof(float)+sizeof(float), 
                                         OCIO::AutoStride, 
                                         OCIO::AutoStride);

        std::vector<float> outImg(NB_PIXELS*3);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 
                                         1, NB_PIXELS, 3);

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
        {
            OCIO_CHECK_CLOSE(outImg[3*pxl+0], resImg[4*pxl+0], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[3*pxl+1], resImg[4*pxl+1], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[3*pxl+2], resImg[4*pxl+2], 1e-7f);
        }
    }

    {
        // Pixels are { RGBAx, RGBAx, RGBAx,
        //              RGBAx, RGBAx, RGBAx  } where x is not a color channel.

        std::vector<float> img
            = { inImg[ 0], inImg[ 1], inImg[ 2], inImg[ 3], magicNumber,
                inImg[ 4], inImg[ 5], inImg[ 6], inImg[ 7], magicNumber,
                inImg[ 8], inImg[ 9], inImg[10], inImg[11], magicNumber,
                inImg[12], inImg[13], inImg[14], inImg[15], magicNumber,
                inImg[16], inImg[17], inImg[18], inImg[19], magicNumber,
                inImg[20], inImg[21], inImg[22], inImg[23], magicNumber };

        OCIO::PackedImageDesc srcImgDesc(&img[0], 
                                         3, 2, 4,
                                         OCIO::AutoStride, 
                                         // Bytes to the next pixel.
                                         4*sizeof(float)+sizeof(float), 
                                         OCIO::AutoStride);

        std::vector<float> outImg(NB_PIXELS*3);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 
                                         1, NB_PIXELS, 3);

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
        {
            OCIO_CHECK_CLOSE(outImg[3*pxl+0], resImg[4*pxl+0], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[3*pxl+1], resImg[4*pxl+1], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[3*pxl+2], resImg[4*pxl+2], 1e-7f);
        }
    }

    {
        // Pixels are { RGBAx, RGBAx, RGBAx, x
        //              RGBAx, RGBAx, RGBAx, x  } where x is not a color channel.

        std::vector<float> img
            = { inImg[ 0], inImg[ 1], inImg[ 2], inImg[ 3], magicNumber,
                inImg[ 4], inImg[ 5], inImg[ 6], inImg[ 7], magicNumber,
                inImg[ 8], inImg[ 9], inImg[10], inImg[11], magicNumber,
                magicNumber,
                inImg[12], inImg[13], inImg[14], inImg[15], magicNumber,
                inImg[16], inImg[17], inImg[18], inImg[19], magicNumber,
                inImg[20], inImg[21], inImg[22], inImg[23], magicNumber,
                magicNumber };

        OCIO::PackedImageDesc srcImgDesc(&img[0], 
                                         3, 2, 4,
                                         OCIO::AutoStride, 
                                         // Bytes to the next pixel.
                                         4*sizeof(float)+sizeof(float), 
                                         // Bytes to the next line.
                                         3*(4*sizeof(float)+sizeof(float))+sizeof(float));

        std::vector<float> outImg(NB_PIXELS*3);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 
                                         1, NB_PIXELS, 3);

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
        {
            OCIO_CHECK_CLOSE(outImg[3*pxl+0], resImg[4*pxl+0], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[3*pxl+1], resImg[4*pxl+1], 1e-7f);
            OCIO_CHECK_CLOSE(outImg[3*pxl+2], resImg[4*pxl+2], 1e-7f);
        }
    }

}

OCIO_ADD_TEST(CPUProcessor, one_pixel)
{
    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_FORWARD));

    // The CPU Processor only includes a Matrix with offset:
    //   const float offset4[4] = { 1.4002f, 0.4005f, 0.8007f, 0.5007f };

    {
        float pixel[4]{ 0.1f, 0.3f, 0.9f, 1.0f };

        OCIO_CHECK_NO_THROW(cpuProcessor->applyRGBA(pixel));

        OCIO_CHECK_EQUAL(pixel[0], 0.1f + 1.4002f);
        OCIO_CHECK_EQUAL(pixel[1], 0.3f + 0.4005f);
        OCIO_CHECK_EQUAL(pixel[2], 0.9f + 0.8007f);
        OCIO_CHECK_EQUAL(pixel[3], 1.0f + 0.5007f);
    }

    {
        float pixel[3]{ 0.1f, 0.3f, 0.9f };

        OCIO_CHECK_NO_THROW(cpuProcessor->applyRGB(pixel));

        OCIO_CHECK_EQUAL(pixel[0], 0.1f + 1.4002f);
        OCIO_CHECK_EQUAL(pixel[1], 0.3f + 0.4005f);
        OCIO_CHECK_EQUAL(pixel[2], 0.9f + 0.8007f);
    }
}


#endif // OCIO_UNIT_TEST
