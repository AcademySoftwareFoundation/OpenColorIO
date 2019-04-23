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

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "CPUProcessor.h"
#include "ops/Lut1D/Lut1DOpCPU.h"


OCIO_NAMESPACE_ENTER
{

// Create the CPUOp instance for any bit depths.
ConstOpCPURcPtr CreateCPUOp(const OpRcPtr & op, BitDepth inBD, BitDepth outBD)
{
    ConstOpRcPtr o = op;

    if(inBD==BIT_DEPTH_F32 && outBD==BIT_DEPTH_F32)
    {
        // The ops were already finalized in RGBA F32 so the existing
        // CPUOp instances can be reused as-is.
        return o->getCPUOp();
    }
    else if (o->data()->getType()==OpData::Lut1DType)
    {
        // Note: Only case where a clone is mandatory.
        Lut1DOpDataRcPtr lut = DynamicPtrCast<const Lut1DOpData>(o->data())->clone();
        lut->setInputBitDepth(inBD);
        lut->setOutputBitDepth(outBD);
        lut->finalize();

        ConstLut1DOpDataRcPtr l = lut;
        return GetLut1DRenderer(l, inBD, outBD);
    }

    throw Exception("Only the 1D LUT Op supports other bit depths than F32");
}

std::string PixelFormatToString(PixelFormat pxlFormat)
{
    std::string val;

    switch(ExtractChannelOrder(pxlFormat))
    {
        case CHANNEL_ORDERING_RGBA:
            val += "rgba";
            break;
        case CHANNEL_ORDERING_BGRA:
            val += "bgra";
            break;
        default:
            throw Exception("Unsupported channel ordering");
            break;
    }

    val += "_";

    switch(ExtractBitDepth(pxlFormat))
    {
        case BIT_DEPTH_UINT8:
            val += "uint8";
            break;
        case BIT_DEPTH_UINT10:
            val += "uint10";
            break;
        case BIT_DEPTH_UINT12:
            val += "uint12";
            break;
        case BIT_DEPTH_UINT14:
            val += "uint14";
            break;
        case BIT_DEPTH_F16:
            val += "half";
            break;
        case BIT_DEPTH_F32:
            val += "float";
            break;
        default:
            throw Exception("Unsupported bit depth");
            break;
    }

    return val;    
}

OpCPURcPtr CreateChannelOrderOp(ChannelOrdering inChannelOrder,
                                ChannelOrdering outChannelOrder, 
                                BitDepth bitdepth)
{
    OpCPURcPtr cpu;

    // TODO: Create the CPUOp instance dedicated to channel order conversions.

    return cpu;
}

// Create the Op to handle pixel format conversions.
OpCPURcPtr CreatePixelFormatOp(PixelFormat in, PixelFormat out)
{
    OpCPURcPtr cpu;

    // TODO: Create the CPUOp instance dedicated to pixel format 
    //       and/or bit depth conversions.

    return cpu;
}

const char * CPUProcessor::Impl::getCacheID() const
{
    return m_cacheID.c_str();
}

void CPUProcessor::Impl::finalize(const OpRcPtrVec & ops, PixelFormat in, PixelFormat out)
{
    // Whatever are the requested input and output pixel formats, the internal 
    // computations are only performed in RGBA F32 i.e. only the first and the last Ops
    // handle different pixel formats. To simplify the Op implementations 
    // a custom internal Op handles any possible pixel formats. Only the 1D LUT Op 
    // natively supports bit depths i.e. to benefit from its lookup implementation 
    // for interger bit depths.
    // 
    // The finalization of the CPUProcessor instance starting from a finalized Processor 
    // instance, it could then reuse as-is the existing CPUOp instances without 
    // any performance or memory costs.
    // 
    // The sequence is:
    // 1. ConstProcessorRcPtr Config::getProcessor() (refer to Config.cpp)
    // 1.1. Create an empty processor
    // 1.2. Add the transformation
    // 1.3. processor finalization (refer to Processor.cpp)
    // 1.3.1. FinalizeOpVec
    // 1.3.1.1 OptimizeOpVec
    // 1.3.1.2 Op finalize
    // 2. From the processor instance, access to a CPUProcessor instance
    // 2.1. Create an empty CPU processor
    // 2.2. CPU processor finalization

    // Note:
    // Avoid changing/cloning the op and op data instances as the list of ops was 
    // already finalized in RGBA F32 pixel format by the processor instance, 
    // and all ops (except 1D LUT) are only used in that context.

    // Note: 
    // Only the 1D LUT CPUOp can natively support the bit depth conversions
    // (and benefit from it) so the algorithm tries to optimize the 1D LUT CPUOp usage 
    // from the list of ops. However, the op does only support the RGBA channel ordering.

    m_ops.clear();

    m_inPixelFormat  = in;
    m_outPixelFormat = out;

    // Does the color processing introduce crosstalk between the pixel channels?

    m_hasChannelCrosstalk = false;
    for(auto & op : ops)
    {
        if(op->hasChannelCrosstalk())
        {
            m_hasChannelCrosstalk = true;
            break;
        }
    }

    const size_t numOps = ops.size();

    // Adjust the op list to only process RGBA F32 pixel formats except for 1D LUT ops
    // which benefit from integer bit depths.

    if(numOps==1)
    {
        if(DynamicPtrCast<const Op>(ops[0])->data()->getType()==OpData::Lut1DType)
        {
            if(ExtractChannelOrder(in)!=CHANNEL_ORDERING_RGBA)
            {
                m_ops.push_back( CreateChannelOrderOp(ExtractChannelOrder(in),
                                                      CHANNEL_ORDERING_RGBA, 
                                                      ExtractBitDepth(in)) );
            }

            // Benefit from the 1D LUT look-up for integer bit depths.
            m_ops.push_back( CreateCPUOp(ops[0], ExtractBitDepth(in), ExtractBitDepth(out)) );

            if(ExtractChannelOrder(out)!=CHANNEL_ORDERING_RGBA)
            {
                m_ops.push_back( CreateChannelOrderOp(CHANNEL_ORDERING_RGBA,
                                                      ExtractChannelOrder(out), 
                                                      ExtractBitDepth(out)) );
            }
        }
        else
        {
            if(in!=PIXEL_FORMAT_RGBA_F32)
            {
                m_ops.push_back( CreatePixelFormatOp(in, PIXEL_FORMAT_RGBA_F32) );
            }

            m_ops.push_back( CreateCPUOp(ops[0], BIT_DEPTH_F32, BIT_DEPTH_F32) );

            if(out!=PIXEL_FORMAT_RGBA_F32)
            {
                m_ops.push_back( CreatePixelFormatOp(PIXEL_FORMAT_RGBA_F32, out) );
            }
        }
    }
    else if(numOps>1)
    {
        // Step 1: Process the head of the list.

        if(DynamicPtrCast<const Op>(ops[0])->data()->getType()==OpData::Lut1DType)
        {
            // Benefit from the 1D LUT look-up for integer bit depths.

            if(ExtractChannelOrder(in)!=CHANNEL_ORDERING_RGBA)
            {
                m_ops.push_back( CreateChannelOrderOp(ExtractChannelOrder(in),
                                                      CHANNEL_ORDERING_RGBA, 
                                                      ExtractBitDepth(in)) );
            }

            m_ops.push_back( CreateCPUOp(ops[0], ExtractBitDepth(in), BIT_DEPTH_F32) );
        }
        else
        {
            if(in!=PIXEL_FORMAT_RGBA_F32)
            {
                m_ops.push_back( CreatePixelFormatOp(in, PIXEL_FORMAT_RGBA_F32) );
            }

            m_ops.push_back( CreateCPUOp(ops[0], BIT_DEPTH_F32, BIT_DEPTH_F32) );
        }

        // Step 2: Process the body of the list (i.e. all ops except the first and the last ones).

        for(size_t idx=1; idx<(numOps-1); ++idx)
        {
            m_ops.push_back(ops[idx]->getCPUOp());
        }

        // Step 3: Process the tail of the list.

        if(DynamicPtrCast<const Op>(ops[numOps-1])->data()->getType()==OpData::Lut1DType)
        {
            // Benefit from the 1D LUT native bit depth support.

            m_ops.push_back( CreateCPUOp(ops[numOps-1], BIT_DEPTH_F32, ExtractBitDepth(out)) );

            if(ExtractChannelOrder(out)!=CHANNEL_ORDERING_RGBA)
            {
                m_ops.push_back( CreateChannelOrderOp(CHANNEL_ORDERING_RGBA,
                                                      ExtractChannelOrder(out), 
                                                      ExtractBitDepth(out)) );
            }
        }
        else
        {
            m_ops.push_back( CreateCPUOp(ops[numOps-1], BIT_DEPTH_F32, BIT_DEPTH_F32) );

            if(out!=PIXEL_FORMAT_RGBA_F32)
            {
                // Convert from RGBA F32 to the output pixel format.
                m_ops.push_back( CreatePixelFormatOp(PIXEL_FORMAT_RGBA_F32, out) );
            }
        }
    }

    // The optimization may result in an empty op list (e.g. a Processor is created 
    // with the same src & dst colour space).

    else if (in!=PIXEL_FORMAT_RGBA_F32 || out!=PIXEL_FORMAT_RGBA_F32)
    {
        // There is some conversion needed between the in and out pixel formats.

        if(in!=PIXEL_FORMAT_RGBA_F32)
        {
            m_ops.push_back( CreatePixelFormatOp(in, PIXEL_FORMAT_RGBA_F32) );
        }

        if(out!=PIXEL_FORMAT_RGBA_F32)
        {
            m_ops.push_back( CreatePixelFormatOp(PIXEL_FORMAT_RGBA_F32, out) );
        }
    }
    else
    {
        // There is some minimal processing needed to support different buffer images.

        m_ops.push_back( CreatePixelFormatOp(PIXEL_FORMAT_RGBA_F32, PIXEL_FORMAT_RGBA_F32) );
    }

    // Compute the cache id.

    std::stringstream ss;
    ss << "from " << PixelFormatToString(in)
       << " to "  << PixelFormatToString(out)
       << " ops :";
    for(auto & op : ops)
    {
        ss << " " << op->getCacheID();
    }

    m_cacheID = ss.str();

    m_numOps         = m_ops.size();

    // A float output buffer could be used by intermediate Op processings
    // which are in BIT_DEPTH_F32 by default.
    // TODO: Validate that the pixel format has 4 color channels; 
    //       otherwise, an intermediate buffer is still needed.
    m_reuseOutBuffer = ExtractBitDepth(getOutputPixelFormat())==BIT_DEPTH_F32;
}

void CPUProcessor::Impl::apply(const void * inImg, void * outImg, long numPixels) const
{
    // TODO: Investigate a template method to remove the if's which could impact
    //       the one pixel processing.

    if(m_numOps==1)
    {
        m_ops[0]->apply(inImg, outImg, numPixels);
    }
    else if(m_reuseOutBuffer)
    {
        // Use the output buffer as intermediate buffer.
        float * out = (float *)outImg;

        m_ops[0]->apply(inImg, out, numPixels);

        for(size_t idx=1; idx<=m_numOps-2; ++idx)
        {
            m_ops[idx]->apply(out, out, numPixels);
        }

        m_ops[m_numOps-1]->apply(out, out, numPixels);
    }
    else
    {
        std::vector<float> buffer(numPixels*4);

        m_ops[0]->apply(inImg, &buffer[0], numPixels);

        for(size_t idx=1; idx<=m_numOps-2; ++idx)
        {
            m_ops[idx]->apply(&buffer[0], &buffer[0], numPixels);
        }

        m_ops[m_numOps-1]->apply(&buffer[0], outImg, numPixels);
    }
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

bool CPUProcessor::hasChannelCrosstalk() const
{
    return getImpl()->hasChannelCrosstalk();
}

const char * CPUProcessor::getCacheID() const
{
    return getImpl()->getCacheID();
}

PixelFormat CPUProcessor::getInputPixelFormat() const
{
    return getImpl()->getInputPixelFormat();
}

PixelFormat CPUProcessor::getOutputPixelFormat() const
{
    return getImpl()->getOutputPixelFormat();
}

void CPUProcessor::apply(const void * inImg, void * outImg, long numPixels) const
{
    getImpl()->apply(inImg, outImg, numPixels);
}


}
OCIO_NAMESPACE_EXIT



///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;

#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut1D/Lut1DOpData.h"
#include "unittest.h"
#include "UnitTestUtils.h"


// TODO: CPUProcessor being part of the OCIO public API limits the ability 
//       to inspect the CPUProcessor instance content i.e. the list of CPUOps. 
//       Even a successful apply could hide a major performance hit because of
//       an 'useless' CPUOp in the list. 


template<OCIO::PixelFormat pf>
struct ExtractBitDepthInfo
{
    static const OCIO::BitDepth bd = OCIO::BitDepth(0x00FF&pf);
};

template<OCIO::PixelFormat inPF, OCIO::PixelFormat outPF, unsigned line>
OCIO::ConstCPUProcessorRcPtr ComputeValues(OCIO::ConstProcessorRcPtr processor, 
                                           const void * inImg, 
                                           const void * resImg, 
                                           long numPixels,
                                           // Default value to nan to break any float comparisons
                                           // as a valid error threshold is mandatory in that case.
                                           float absErrorThreshold = std::numeric_limits<float>::quiet_NaN())
{
    typedef typename OCIO::BitDepthInfo< ExtractBitDepthInfo<outPF>::bd >::Type outType;

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OIIO_CHECK_NO_THROW(cpuProcessor = processor->getCPUProcessor(inPF, outPF));

    const size_t numValues = size_t(numPixels * 4);

    std::vector<outType> out(numValues);
    OIIO_CHECK_NO_THROW(cpuProcessor->apply(inImg, &out[0], numPixels));

    const outType * res = (const outType *)resImg;

    for(size_t idx=0; idx<numValues; ++idx)
    {
        if(OCIO::BitDepthInfo< ExtractBitDepthInfo<outPF>::bd >::isFloat)
        {
            OIIO_CHECK_CLOSE_FROM(out[idx], res[idx], absErrorThreshold, line);
        }
        else
        {
            OIIO_CHECK_EQUAL_FROM(out[idx], res[idx], line);
        }
    }

    return cpuProcessor;
}

OIIO_ADD_TEST(CPUProcessor, with_one_matrix)
{
    // The unit test validates that pixel formats are correctly 
    // processed when the op list contains only one arbitrary Op
    // (except the 1D LUT one).

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::MatrixTransformRcPtr transform = OCIO::MatrixTransform::Create();
    const float offset4[4] = { 1.4002f, 0.4005f, 0.8007f, 0.0f };
    transform->setOffset( offset4 );

    OCIO::ConstProcessorRcPtr processor; 
    OIIO_CHECK_NO_THROW(processor = config->getProcessor(transform));

    const unsigned NB_PIXELS = 3;

    const std::vector<float> f_inImg =
        {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
            0.1023f,  0.5045f,  1.5089f,  1.0f,
            1.0000f,  1.2500f,  1.9900f,  0.0f  };

    OCIO::ConstCPUProcessorRcPtr cpuProcessor; 

    {
        const std::vector<float> resImg
            = { 0.4002f, -0.3995f,  0.7007f,  0.0000f,
                1.5025f,  0.9050f,  2.3096f,  1.0000f,
                2.4002f,  1.6505f,  2.7907f,  0.0000f };

        cpuProcessor 
            = ComputeValues<OCIO::PIXEL_FORMAT_RGBA_F32,
                            OCIO::PIXEL_FORMAT_RGBA_F32,
                            __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS, 1e-7f);

        OIIO_CHECK_EQUAL(cpuProcessor->getInputPixelFormat(), OCIO::PIXEL_FORMAT_RGBA_F32);
        OIIO_CHECK_EQUAL(cpuProcessor->getOutputPixelFormat(), OCIO::PIXEL_FORMAT_RGBA_F32);

        // Validate that the two apply paths produce identical results.

        std::vector<float> f_outImg2(NB_PIXELS * 4);
        f_outImg2 = f_inImg;

        OCIO::PackedImageDesc desc(&f_outImg2[0], NB_PIXELS, 1, 4);
        OIIO_CHECK_NO_THROW(processor->apply(desc));

        for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
        {
            OIIO_CHECK_CLOSE(f_outImg2[idx], resImg[idx],  1e-7f);
        }
    }

    // TODO: Will uncomment after implementation is complete.
/*
    {
        const std::vector<float> resImg
            = { 0.7007f, -0.3995f,  0.4002f,  0.0000f,
                2.3096f,  0.9050f,  1.5025f,  1.0000f,
                2.7907f,  1.6505f,  2.4002f,  0.0000f };

        ComputeValues<OCIO::PIXEL_FORMAT_RGBA_F32,
                      OCIO::PIXEL_FORMAT_BGRA_F32,
                      __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS, 1e-7f);
    }

    {
        const std::vector<float> resImg 
            = { 1.3002f, -0.3995f, -0.1993f, 0.0000f,
                2.9091f,  0.9050f,  0.9030f, 1.0000f,
                3.3902f,  1.6505f,  1.8007f, 0.0000f };

        ComputeValues<OCIO::PIXEL_FORMAT_BGRA_F32,
                      OCIO::PIXEL_FORMAT_RGBA_F32,
                      __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS, 1e-6f);
    }

    {
        const std::vector<uint16_t> resImg
            = { 26227,     0, 45920,     0,
                65535, 59309, 65535, 65535,
                65535, 65535, 65535,     0 };

        ComputeValues<OCIO::PIXEL_FORMAT_RGBA_F32,
                      OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS);
    }

    const std::vector<uint16_t> i_inImg =
        {    0,      8,    32,  0,
            64,    128,   256,  0,
          5120,  20140, 65535,  0  };

    {
        const std::vector<float> resImg 
            = { 1.40020000f,  0.40062206f,  0.80118829f,  0.0f,
                1.40117657f,  0.40245315f,  0.80460631f,  0.0f,
                1.47832620f,  0.70781672f,  1.80070000f,  0.0f };

        ComputeValues<OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      OCIO::PIXEL_FORMAT_RGBA_F32,
                      __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS, 1e-7f);
    }

    {
        const std::vector<uint16_t> resImg
            = { 65535, 26255, 52506, 0,
                65535, 26375, 52730, 0,
                65535, 46387, 65535, 0 };

        ComputeValues<OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = { 52506, 26255, 65535, 0,
                52730, 26375, 65535, 0,
                65535, 46387, 65535, 0 };

        ComputeValues<OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      OCIO::PIXEL_FORMAT_BGRA_UINT16,
                      __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);
    }
*/
}

OIIO_ADD_TEST(CPUProcessor, with_one_1d_lut)
{
    // The unit test validates that pixel formats are correctly 
    // processed when the op list contains only one 1D LUT.

    const std::string filePath
        = std::string(OCIO::getTestFilesDir()) + "/lut1d_5.spi1d";

    OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
    transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    transform->setSrc(filePath.c_str());
    transform->setInterpolation(OCIO::INTERP_LINEAR);

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ConstProcessorRcPtr processor; 
    OIIO_CHECK_NO_THROW(processor = config->getProcessor(transform));

    const unsigned NB_PIXELS = 4;

    const std::vector<float> f_inImg =
        {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
            0.1002f,  0.2509f,  0.5009f,  1.0f,
            0.5505f,  0.7090f,  0.9099f,  1.0f,
            1.0000f,  1.2500f,  1.9900f,  0.0f  };

    OCIO::ConstCPUProcessorRcPtr cpuProcessor; 

    {
        const std::vector<float> resImg
            = {  0,            0,            0,            0,
                 0.03728949f,  0.10394855f,  0.24695572f,  1,
                 0.29089212f,  0.50935059f,  1.91091322f,  1,
                64,           64,           64,            0 };

        cpuProcessor
            = ComputeValues<OCIO::PIXEL_FORMAT_RGBA_F32,
                            OCIO::PIXEL_FORMAT_RGBA_F32,
                            __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS, 1e-7f);

        OIIO_CHECK_EQUAL(cpuProcessor->getInputPixelFormat(), OCIO::PIXEL_FORMAT_RGBA_F32);
        OIIO_CHECK_EQUAL(cpuProcessor->getOutputPixelFormat(), OCIO::PIXEL_FORMAT_RGBA_F32);

        // Validate that the two apply paths produce identical results.

        std::vector<float> f_outImg2(NB_PIXELS * 4);
        f_outImg2 = f_inImg;

        OCIO::PackedImageDesc desc(&f_outImg2[0], NB_PIXELS, 1, 4);
        OIIO_CHECK_NO_THROW(processor->apply(desc));

        for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
        {
            OIIO_CHECK_CLOSE(f_outImg2[idx], resImg[idx],  1e-7f);
        }
    }

    // TODO: Will uncomment after implementation is complete.
/*
    {
        const std::vector<uint16_t> resImg
            = {     0,     0,     0,     0,
                 2444,  6812, 16184, 65535,
                19064, 33381, 65535, 65535,
                65535, 65535, 65535,     0 };

        ComputeValues<OCIO::PIXEL_FORMAT_RGBA_F32,
                      OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS);    
    }

    const std::vector<uint16_t> i_inImg =
        {    0,      8,    32,  0,
            64,    128,   256,  0,
           512,   1024,  2048,  0,
          5120,  20480, 65535,  0  };

    {
        const std::vector<float> resImg
            = {  0,           0.00036166f, 0.00144666f, 0,
                 0.00187417f, 0.00271759f, 0.00408672f, 0,
                 0.00601041f, 0.00912247f, 0.01456576f, 0,
                 0.03030112f, 0.13105739f, 64, 0 };

        ComputeValues<OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      OCIO::PIXEL_FORMAT_RGBA_F32,
                      __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS, 1e-7f);    
    }

    {
        const std::vector<uint16_t> resImg
            = {     0,    24,    95,     0,
                  123,   178,   268,     0,
                  394,   598,   955,     0,
                 1986,  8589, 65535,     0 };

        ComputeValues<OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);    
    }

    {
        const std::vector<uint16_t> resImg
            = {    95,    24,     0,     0,
                  268,   178,   123,     0,
                  955,   598,   394,     0,
                65535,  8588,  1986,     0 };

        ComputeValues<OCIO::PIXEL_FORMAT_BGRA_UINT16,
                      OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);    
    }

    {
        const std::vector<uint16_t> resImg
            = {    95,    24,     0,     0,
                  268,   178,   123,     0,
                  955,   598,   394,     0,
                65535,  8589,  1986,     0 };

        ComputeValues<OCIO::PIXEL_FORMAT_RGBA_UINT16,
                      OCIO::PIXEL_FORMAT_BGRA_UINT16,
                      __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);    
    }

    {
        const std::vector<uint16_t> resImg
            = {     0,    24,    95,     0,
                  123,   178,   268,     0,
                  394,   598,   955,     0,
                 1986,  8589, 65535,     0 };

        ComputeValues<OCIO::PIXEL_FORMAT_BGRA_UINT16,
                      OCIO::PIXEL_FORMAT_BGRA_UINT16,
                      __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);    
    }
*/
}

OIIO_ADD_TEST(CPUProcessor, with_several_ops)
{
    // The unit test validates that pixel formats are correctly 
    // processed when the op list starts or ends with a 1D LUT.

    static const std::string SIMPLE_PROFILE =
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
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        OCIO::ConstProcessorRcPtr processor; 
        OIIO_CHECK_NO_THROW(processor = config->getProcessor("cs1", "cs2"));

        const unsigned NB_PIXELS = 4;

        const std::vector<float> f_inImg =
            {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
                0.1002f,  0.2509f,  0.5009f,  1.0f,
                0.5505f,  0.7090f,  0.9099f,  1.0f,
                1.0000f,  1.2500f,  1.9900f,  0.0f  };

        OCIO::ConstCPUProcessorRcPtr cpuProcessor; 

        {
            const std::vector<float> resImg
                = {  0.0f,         0.0f,         0.0f,         0.0f,
                     0.0f,         0.20273837f,  0.24680146f,  1.0f,
                     0.15488569f,  1.69210147f,  1.90666747f,  1.0f,
                     0.81575858f, 64.0f,        64.0f,         0.0f };

            cpuProcessor 
                = ComputeValues<OCIO::PIXEL_FORMAT_RGBA_F32,
                                OCIO::PIXEL_FORMAT_RGBA_F32,
                                __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS, 1e-7f);    

            OIIO_CHECK_EQUAL(cpuProcessor->getInputPixelFormat(), OCIO::PIXEL_FORMAT_RGBA_F32);
            OIIO_CHECK_EQUAL(cpuProcessor->getOutputPixelFormat(), OCIO::PIXEL_FORMAT_RGBA_F32);

            // Validate that the two apply paths produce identical results.

            std::vector<float> f_outImg2(NB_PIXELS * 4);
            f_outImg2 = f_inImg;

            OCIO::PackedImageDesc desc(&f_outImg2[0], NB_PIXELS, 1, 4);
            OIIO_CHECK_NO_THROW(processor->apply(desc));

            for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
            {
                OIIO_CHECK_CLOSE(f_outImg2[idx], resImg[idx],  1e-7f);
            }
        }

    // TODO: Will uncomment after implementation is complete.
/*
        std::vector<uint16_t> i_outImg(NB_PIXELS * 4, 1);
        {
            const std::vector<uint16_t> resImg
                = {     0,     0,     0,     0,
                        0, 13286, 16174, 65535,
                    10150, 65535, 65535, 65535,
                    53460, 65535, 65535,     0 };

            ComputeValues<OCIO::PIXEL_FORMAT_RGBA_F32,
                          OCIO::PIXEL_FORMAT_RGBA_UINT16,
                          __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS);    
        }

        const std::vector<uint16_t> i_inImg =
            {    0,      8,    32,  0,
                64,    128,   256,  0,
               512,   1024,  2048,  0,
              5120,  20480, 65535,  0  };

        {
            const std::vector<float> resImg
                = {  0.0f,  0.07789713f,  0.00088374f,  0.0f,
                     0.0f,  0.07871927f,  0.00396248f,  0.0f,
                     0.0f,  0.08474064f,  0.01450117f,  0.0f,
                     0.0f,  0.24826171f, 56.39490891f,  0.0f };

            ComputeValues<OCIO::PIXEL_FORMAT_RGBA_UINT16,
                          OCIO::PIXEL_FORMAT_RGBA_F32,
                          __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS, 1e-7f);    
        }

        {
            const std::vector<uint16_t> resImg
                = {     0,  5105,    58,     0,
                        0,  5159,   260,     0,
                        0,  5554,   950,     0,
                        0, 16270, 65535,     0 };

            ComputeValues<OCIO::PIXEL_FORMAT_RGBA_UINT16,
                          OCIO::PIXEL_FORMAT_RGBA_UINT16,
                          __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);    
        }

        {
            const std::vector<uint16_t> resImg
                = {     0,  5105,     0,     0,
                        0,  5159,   112,     0,
                        0,  5554,   388,     0,
                    53460, 16270,  1982,     0 };

            ComputeValues<OCIO::PIXEL_FORMAT_BGRA_UINT16,
                          OCIO::PIXEL_FORMAT_RGBA_UINT16,
                          __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);    
        }

        {
            const std::vector<uint16_t> resImg
                = {    58,  5105,     0,     0,
                      260,  5159,     0,     0,
                      950,  5553,     0,     0,
                    65535, 16270,     0,     0 };

            ComputeValues<OCIO::PIXEL_FORMAT_RGBA_UINT16,
                          OCIO::PIXEL_FORMAT_BGRA_UINT16,
                          __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);    
        }

        {
            const std::vector<uint16_t> resImg
                = {     0,  5105,     0,     0,
                      112,  5159,     0,     0,
                      388,  5553,     0,     0,
                     1982, 16270, 53461,     0 };

            ComputeValues<OCIO::PIXEL_FORMAT_BGRA_UINT16,
                          OCIO::PIXEL_FORMAT_BGRA_UINT16,
                          __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);    
        }
*/
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
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        OCIO::ConstProcessorRcPtr processor; 
        OIIO_CHECK_NO_THROW(processor = config->getProcessor("cs1", "cs2"));

        const unsigned NB_PIXELS = 4;

        const std::vector<float> f_inImg =
            {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
                0.1002f,  0.2509f,  0.5009f,  1.0f,
                0.5505f,  0.7090f,  0.9099f,  1.0f,
                1.0000f,  1.2500f,  1.9900f,  0.0f  };

        OCIO::ConstCPUProcessorRcPtr cpuProcessor; 

        {
            const std::vector<float> resImg
                = { -0.18999999f,  0.18999999f, -0.00019000f,  0.0f,
                    -0.15271049f,  0.29394856f,  0.24676571f,  1.0f,
                     0.10089212f,  0.69935059f,  1.91072320f,  1.0f,
                    63.81000137f, 64.19000244f, 63.99980927f,  0.0f };

            cpuProcessor
                = ComputeValues<OCIO::PIXEL_FORMAT_RGBA_F32,
                                OCIO::PIXEL_FORMAT_RGBA_F32,
                                __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS, 1e-7f);    

            OIIO_CHECK_EQUAL(cpuProcessor->getInputPixelFormat(), OCIO::PIXEL_FORMAT_RGBA_F32);
            OIIO_CHECK_EQUAL(cpuProcessor->getOutputPixelFormat(), OCIO::PIXEL_FORMAT_RGBA_F32);

            // Validate that the two apply paths produce identical results.

            std::vector<float> f_outImg2(NB_PIXELS * 4);
            f_outImg2 = f_inImg;

            OCIO::PackedImageDesc desc(&f_outImg2[0], NB_PIXELS, 1, 4);
            OIIO_CHECK_NO_THROW(processor->apply(desc));

            for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
            {
                OIIO_CHECK_CLOSE(f_outImg2[idx], resImg[idx],  1e-7f);
            }
        }
/*
        {
            const std::vector<uint16_t> resImg
                = {     0, 12452,     0,     0,
                        0, 19264, 16172, 65535,
                     6612, 45832, 65535, 65535,
                    65535, 65535, 65535,     0 };

            ComputeValues<OCIO::PIXEL_FORMAT_RGBA_F32,
                          OCIO::PIXEL_FORMAT_RGBA_UINT16,
                          __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS);    
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

            ComputeValues<OCIO::PIXEL_FORMAT_RGBA_UINT16,
                          OCIO::PIXEL_FORMAT_RGBA_F32,
                          __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS, 1e-7f);    
        }

        {
            const std::vector<uint16_t> resImg
                = {     0, 12475,     0,     0,
                      110, 12630,     0,     0,
                      381, 13049,     0,     0,
                     1973, 21040, 65535,     0 };

            ComputeValues<OCIO::PIXEL_FORMAT_BGRA_UINT16,
                          OCIO::PIXEL_FORMAT_BGRA_UINT16,
                          __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);    
        }
*/
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
        OIIO_CHECK_NO_THROW(config = OCIO::Config::CreateFromStream(is));
        OIIO_CHECK_NO_THROW(config->sanityCheck());

        OCIO::ConstProcessorRcPtr processor; 
        OIIO_CHECK_NO_THROW(processor = config->getProcessor("cs1", "cs2"));

        const unsigned NB_PIXELS = 4;

        const std::vector<float> f_inImg =
            {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
                0.1002f,  0.2509f,  0.5009f,  1.0f,
                0.5505f,  0.7090f,  0.9099f,  1.0f,
                1.0000f,  1.2500f,  1.9900f,  0.0f  };

        OCIO::ConstCPUProcessorRcPtr cpuProcessor; 

        {
            const std::vector<float> resImg
                = { -0.79690927f, -0.06224250f, -0.42994320f,  0.0f,
                    -0.72481626f,  0.13872468f,  0.04750441f,  1.0f,
                    -0.23451784f,  0.92250210f,  3.26448941f,  1.0f,
                     3.43709063f,  3.43709063f,  3.43709063f,  0.0f };

            cpuProcessor
                = ComputeValues<OCIO::PIXEL_FORMAT_RGBA_F32,
                                OCIO::PIXEL_FORMAT_RGBA_F32,
                                __LINE__>(processor, &f_inImg[0], &resImg[0], NB_PIXELS, 1e-7f);    

            OIIO_CHECK_EQUAL(cpuProcessor->getInputPixelFormat(), OCIO::PIXEL_FORMAT_RGBA_F32);
            OIIO_CHECK_EQUAL(cpuProcessor->getOutputPixelFormat(), OCIO::PIXEL_FORMAT_RGBA_F32);

            // Validate that the two apply paths produce identical results.

            std::vector<float> f_outImg2(NB_PIXELS * 4);
            f_outImg2 = f_inImg;

            OCIO::PackedImageDesc desc(&f_outImg2[0], NB_PIXELS, 1, 4);
            OIIO_CHECK_NO_THROW(processor->apply(desc));

            for(unsigned idx=0; idx<(NB_PIXELS*4); ++idx)
            {
                OIIO_CHECK_CLOSE(f_outImg2[idx], resImg[idx],  1e-7f);
            }
        }
/*
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

            ComputeValues<OCIO::PIXEL_FORMAT_BGRA_UINT16,
                          OCIO::PIXEL_FORMAT_BGRA_UINT16,
                          __LINE__>(processor, &i_inImg[0], &resImg[0], NB_PIXELS);
        }
*/
    }
}

#endif // OCIO_UNIT_TEST
