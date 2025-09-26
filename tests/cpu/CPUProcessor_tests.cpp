// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "CPUProcessor.cpp"

#include "ops/lut1d/Lut1DOp.h"
#include "ops/lut1d/Lut1DOpData.h"
#include "ScanlineHelper.h"
#include "testutils/UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;


OCIO_ADD_TEST(CPUProcessor, dynamic_properties)
{
    OCIO::ExposureContrastTransformRcPtr ec = OCIO::ExposureContrastTransform::Create();

    ec->setExposure(1.2);
    ec->setPivot(0.5);
    ec->makeContrastDynamic();

    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    auto cpuProc = config->getProcessor(ec)->getDefaultCPUProcessor();
    OCIO_CHECK_ASSERT(cpuProc->isDynamic());
    OCIO_CHECK_ASSERT(cpuProc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OCIO_CHECK_ASSERT(!cpuProc->hasDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE));
    OCIO::DynamicPropertyRcPtr dpc;
    OCIO_CHECK_NO_THROW(dpc = cpuProc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_CONTRAST));
    OCIO_CHECK_ASSERT(dpc);
    OCIO_CHECK_THROW_WHAT(cpuProc->getDynamicProperty(OCIO::DYNAMIC_PROPERTY_EXPOSURE),
                          OCIO::Exception,
                          "Cannot find dynamic property; not used by CPU processor.");
}

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


template<OCIO::BitDepth inBD, OCIO::BitDepth outBD>
OCIO::ConstCPUProcessorRcPtr ComputeValues(unsigned line,
                                           OCIO::ConstProcessorRcPtr processor,
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

    OCIO_CHECK_NO_THROW_FROM(cpuProcessor
        = processor->getOptimizedCPUProcessor(inBD, outBD,
                                              OCIO::OPTIMIZATION_DEFAULT), line);

    size_t numChannels = 4;
    if(outChans==OCIO::CHANNEL_ORDERING_RGB || outChans==OCIO::CHANNEL_ORDERING_BGR)
    {
        numChannels = 3;
    }
    const size_t numValues = size_t(numPixels * numChannels);

    const OCIO::PackedImageDesc srcImgDesc((void *)inImg, numPixels, 1,
                                           inChans,
                                           inBD,
                                           sizeof(inType),
                                           OCIO::AutoStride,
                                           OCIO::AutoStride);

    std::vector<outType> out(numValues);
    OCIO::PackedImageDesc dstImgDesc(&out[0], numPixels, 1,
                                     outChans,
                                     outBD,
                                     sizeof(outType),
                                     OCIO::AutoStride,
                                     OCIO::AutoStride);

    OCIO_CHECK_NO_THROW_FROM(cpuProcessor->apply(srcImgDesc, dstImgDesc), line);

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
    constexpr double offset4[4] = { 1.4002, 0.4005, 0.0807, 0.5 };
    transform->setOffset( offset4 );

    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = config->getProcessor(transform));

    constexpr unsigned NB_PIXELS = 3;

    const std::vector<float> f_inImg =
        {  -1.0000f, -0.8000f, -0.1000f,  0.0f,
            0.1023f,  0.5045f,  1.5089f,  1.0f,
            1.0000f,  1.2500f,  1.9900f,  0.0f  };

    {
        const std::vector<float> resImg
            = { 0.4002f, -0.3995f, -0.0193f,  0.5000f,
                1.5025f,  0.9050f,  1.5896f,  1.5000f,
                2.4002f,  1.6505f,  2.0707f,  0.5000f };

        ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>(
            __LINE__, processor,
            &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS,
            1e-5f);
    }

    {
        const std::vector<float> resImg
            = { -0.9193f,   -0.3995f,  1.3002f,  0.5000f,
                 0.182999f,  0.9050f,  2.9091f,  1.5000f,
                 1.0807f,    1.6505f,  3.3902f,  0.5000f };

        ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>(
            __LINE__, processor,
            &f_inImg[0], OCIO::CHANNEL_ORDERING_BGRA,
            &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA,
            NB_PIXELS,
            1e-5f);
    }

    {
        const std::vector<float> resImg
            = {  -0.500000f, -0.719300f, 0.300500f, 1.400200f,
                  0.602300f,  0.585199f, 1.909399f, 2.400200f,
                  1.500000f,  1.330700f, 2.390500f, 1.400200f  };

        ComputeValues<OCIO::BIT_DEPTH_F32,OCIO::BIT_DEPTH_F32>(
            __LINE__, processor,
            &f_inImg[0], OCIO::CHANNEL_ORDERING_ABGR,
            &resImg[0],  OCIO::CHANNEL_ORDERING_ABGR,
            NB_PIXELS,
            1e-5f);
    }

    {
        const std::vector<float> resImg
            = { -0.0193f, -0.3995f,  0.4002f,  0.5000f,
                 1.5896f,  0.9050f,  1.5025f,  1.5000f,
                 2.0707f,  1.6505f,  2.4002f,  0.5000f };

        ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>( 
            __LINE__, processor,
            &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],  OCIO::CHANNEL_ORDERING_BGRA,
            NB_PIXELS,
            1e-5f);
    }

    {
        const std::vector<float> resImg
            = { 0.5000f, -0.0193f, -0.3995f, 0.4002f,
                1.5000f,  1.5896f,  0.9050f, 1.5025f,
                0.5000f,  2.0707f,  1.6505f, 2.4002f  };

        ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>(
            __LINE__, processor,
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
            = { 0.4002f, -0.3995f, -0.0193f,
                1.5025f,  0.9050f,  1.5896f,
                2.4002f,  1.6505f,  2.0707f };

        ComputeValues<OCIO::BIT_DEPTH_F32,OCIO::BIT_DEPTH_F32>( 
            __LINE__, processor,
            &inImg[0],  OCIO::CHANNEL_ORDERING_RGB,
            &resImg[0], OCIO::CHANNEL_ORDERING_RGB,
            NB_PIXELS,
            1e-5f);
    }

    {
        const std::vector<float> inImg =
            {    -1.0000f,    -0.8000f, -0.1000f,
                   0.1023f,    0.5045f,  1.5089f,
                   1.0000f,    1.2500f,  1.9900f  };

        const std::vector<float> resImg
            = { -0.919300f, -0.399500f,  1.300199f,
                 0.182999f,  0.905000f,  2.909100f,
                 1.080700f,  1.650500f,  3.390200f };

        ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>( 
            __LINE__, processor,
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
            = { -0.01929f,  -0.3995f,  0.4002f,
                 1.58960f,   0.9050f,  1.5025f,
                 2.070699f,  1.6505f,  2.4002f };

        ComputeValues<OCIO::BIT_DEPTH_F32,OCIO::BIT_DEPTH_F32>(
            __LINE__, processor,
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
            = { -0.01929f,  -0.3995f,  0.4002f, 0.5f,
                 1.58960f,   0.9050f,  1.5025f, 0.5f,
                 2.070699f,  1.6505f,  2.4002f, 0.5f   };

        ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>( 
            __LINE__, processor,
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
            = { -0.01929f,  -0.3995f,  0.4002f,
                 1.58960f,   0.9050f,  1.5025f,
                 2.070699f,  1.6505f,  2.4002f   };

        ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>( 
            __LINE__, processor,
            &inImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0], OCIO::CHANNEL_ORDERING_BGR,
            NB_PIXELS,
            1e-5f);
    }

    const std::vector<uint16_t> ui16_inImg =
        {    0,      8,    32,  0,
            64,    128,   256,  0,
          5120,  20140, 65535,  0  };

    {
        const std::vector<float> resImg
            = { 1.40020000f,  0.40062206f,  0.08118829f,  0.5f,
                1.40117657f,  0.40245315f,  0.08460631f,  0.5f,
                1.47832620f,  0.70781672f,  1.08070004f,  0.5f };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_F32>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS,
            1e-5f);
    }

    {
        const std::vector<uint16_t> resImg
            = { 65535, 26255,  5321, 32768,
                65535, 26375,  5545, 32768,
                65535, 46387, 65535, 32768 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = {  5321, 26255, 65535, 32768,
                 5545, 26375, 65535, 32768,
                65535, 46387, 65535, 32768 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_BGRA,
            NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = {  5321, 26255, 65535,
                 5545, 26375, 65535,
                65535, 46387, 65535 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_BGR,
            NB_PIXELS);
    }

    {
        const std::vector<uint8_t> resImg
            = { 255, 102,  21, 128,
                255, 103,  22, 128,
                255, 180, 255, 128 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT8>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
    }

    {
        const std::vector<uint8_t> resImg
            = {  21, 102, 255,
                 22, 103, 255,
                255, 180, 255 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT8>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_BGR,
            NB_PIXELS);
    }

    {
        const std::vector<uint8_t> resImg
            = { 128,  21, 102, 255,
                128,  22, 103, 255,
                128, 255, 180, 255 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT8>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_ABGR,
            NB_PIXELS);
    }

    // Test OCIO::BIT_DEPTH_UINT10.

    {
        const std::vector<uint16_t> ui10_resImg
            = { 1023,  410,   83,  512,
                1023,  412,   87,  512,
                1023,  724, 1023,  512 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT10>(
            __LINE__, processor,
            &ui16_inImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
            &ui10_resImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);

        const std::vector<uint16_t> ui10_inImg
            = {    0,    8,   12,  256,
                 128,   16,   64,  512,
                1023,   32,   96,  512 };

        const std::vector<uint16_t> ui16_resImg
            = { 65535, 26759,  6057, 49167,
                65535, 27272,  9389, 65535,
                65535, 28297, 11439, 65535 };

        ComputeValues<OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui10_inImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
            &ui16_resImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
    }

    // Test OCIO::BIT_DEPTH_UINT12.

    {
        const std::vector<uint16_t> ui12_resImg
            = { 4095, 1641,  332, 2048,
                4095, 1648,  346, 2048,
                4095, 2899, 4095, 2048 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT12>(
            __LINE__, processor,
            &ui16_inImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
            &ui12_resImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);

        const std::vector<uint16_t> ui12_inImg
            = {     0,    8,    12,   1024,
                 2048,   16,    64,   2048,
                 4095,   32,    96,   4095 };

        const std::vector<uint16_t> ui16_resImg
            = { 65535, 26375,  5481, 49155,
                65535, 26503,  6313, 65535,
                65535, 26759,  6825, 65535 };

        ComputeValues<OCIO::BIT_DEPTH_UINT12, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui12_inImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
            &ui16_resImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
    }
}

OCIO_ADD_TEST(CPUProcessor, with_one_1d_lut)
{
    // The unit test validates that pixel formats are correctly
    // processed when the op list only contains one 1D LUT because it
    // has a dedicated optimization when the input bit-depth is an integer type.

    const std::string filePath = OCIO::GetTestFilesDir() + "/lut1d_5.spi1d";

    OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
    transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
    transform->setSrc(filePath.c_str());
    transform->setInterpolation(OCIO::INTERP_LINEAR);

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = config->getProcessor(transform));

    constexpr unsigned NB_PIXELS = 4;

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

        ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>( 
            __LINE__, processor,
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

        ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &f_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
    }

    const std::vector<uint16_t> ui16_inImg =
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

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_F32>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS, 1e-7f);
    }

    {
        const std::vector<uint16_t> resImg
            = {     0,    24,    95,     0,
                  123,   178,   268,    32,
                  394,   598,   955,    64,
                 1986,  8589, 65535,   512 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = {    95,    24,     0,     0,
                  268,   178,   123,    32,
                  955,   598,   394,    64,
                65535,  8589,  1986,   512 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_BGRA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = {    95,    24,     0,     0,
                  268,   178,   123,    32,
                  955,   598,   394,    64,
                65535,  8589,  1986,   512 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_BGRA,
            NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = {    95,    24,     0,
                  268,   178,   123,
                  955,   598,   394,
                65535,  8589,  1986 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_BGR,
            NB_PIXELS);
    }

    {
        const std::vector<uint16_t> resImg
            = {     0,    24,    95,     0,
                  123,   178,   268,    32,
                  394,   598,   955,    64,
                 1986,  8589, 65535,    512 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui16_inImg[0], OCIO::CHANNEL_ORDERING_BGRA,
            &resImg[0],     OCIO::CHANNEL_ORDERING_BGRA,
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

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &my_i_inImg[0], OCIO::CHANNEL_ORDERING_RGB,
            &resImg[0],     OCIO::CHANNEL_ORDERING_BGRA,
            NB_PIXELS);
    }

    // Test OCIO::BIT_DEPTH_UINT10.

    {
        const std::vector<uint16_t> ui10_resImg
            = {     0,     0,     1,     0,
                    2,     3,     4,     0,
                    6,     9,    15,     1,
                   31,   134,  1023,     8 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT10>( 
            __LINE__, processor,
            &ui16_inImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
            &ui10_resImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
    }

    {
        const std::vector<uint16_t> ui10_inImg
            = {     0,     8,    32,
                   64,   128,   256,
                   96,   256,   512,
                  128,  1023,   640  };

        const std::vector<uint16_t> ui10_resImg
            = {     0,     6,    15,   0,
                   26,    48,   106,   0,
                   36,   106,   252,   0,
                   48,  1023,   384,   0 };

        ComputeValues<OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT10>( 
            __LINE__, processor,
            &ui10_inImg[0],  OCIO::CHANNEL_ORDERING_RGB,
            &ui10_resImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);

        const std::vector<uint16_t> ui16_resImg
            = {     0,   394,   955,   0,
                 1656,  3092,  6794,   0,
                 2301,  6794, 16162,   0,
                 3092, 65535, 24593,   0 };

        ComputeValues<OCIO::BIT_DEPTH_UINT10, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui10_inImg[0],  OCIO::CHANNEL_ORDERING_RGB,
            &ui16_resImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
    }

    // Test OCIO::BIT_DEPTH_UINT12.

    {
        const std::vector<uint16_t> ui12_resImg
            = {     0,     1,     6,     0,
                    8,    11,    17,     2,
                   25,    37,    60,     4,
                  124,   537,  4095,    32 };

        ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT12>( 
            __LINE__, processor,
            &ui16_inImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
            &ui12_resImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
    }

    {
        const std::vector<uint16_t> ui12_inImg
            = {     0,     8,    32,
                   64,   128,   256,
                   96,   256,   512,
                 1024,  2048,  4095  };

        const std::vector<uint16_t> ui12_resImg
            = {     0,    11,    25,   0,
                   37,    60,   103,   0,
                   49,   103,   193,   0,
                  424,  1009,  4095,   0 };

        ComputeValues<OCIO::BIT_DEPTH_UINT12, OCIO::BIT_DEPTH_UINT12>( 
            __LINE__, processor,
            &ui12_inImg[0],  OCIO::CHANNEL_ORDERING_RGB,
            &ui12_resImg[0], OCIO::CHANNEL_ORDERING_RGBA,
            NB_PIXELS);
        const std::vector<uint16_t> ui16_resImg
            = {     0,   178,   394,   0,
                  598,   955,  1655,   0,
                  779,  1655,  3089,   0,
                 6789, 16143, 65535,   0 };

        ComputeValues<OCIO::BIT_DEPTH_UINT12, OCIO::BIT_DEPTH_UINT16>( 
            __LINE__, processor,
            &ui12_inImg[0],  OCIO::CHANNEL_ORDERING_RGB,
            &ui16_resImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
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
        "search_path: " + OCIO::GetTestFilesDir() + "\n"
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
        OCIO_CHECK_NO_THROW(config->validate());

        OCIO::ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW(processor = config->getProcessor("cs1", "cs2"));

        constexpr unsigned NB_PIXELS = 4;

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

            ComputeValues<OCIO::BIT_DEPTH_F32,OCIO::BIT_DEPTH_F32>( 
                __LINE__, processor,
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

            ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT16>( 
                __LINE__, processor,
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

            auto cpuProcessor = ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_F32>(
                __LINE__, processor,
                &i_inImg[0], OCIO::CHANNEL_ORDERING_RGBA,
                &resImg[0],  OCIO::CHANNEL_ORDERING_RGBA,
                NB_PIXELS, 1e-7f);

            // SSE2/AVX/AVX2 generate a slightly different LUT1D
            // floating error below the absErrorThreshold, but cacheID hash will be different

            const std::string cacheID{ cpuProcessor->getCacheID() };

            const std::string expectedID("CPU Processor: from 16ui to 32f oFlags 263995331 ops"
                ":  <Lut1D d2f58fb9dbbf324478d9bdad54443ac7 forward default standard domain none>");

            // Test integer optimization. The ops should be optimized into a single LUT
            // when finalizing with an integer input bit-depth.
            OCIO_CHECK_EQUAL(cacheID.length(), expectedID.length());

            // check everything but the cacheID hash
            const std::vector<std::string> toCheck = {
                "CPU Processor: from 16ui to 32f oFlags 263995331 ops:",
                "<Lut1D",
                "forward default standard domain none>" };

            for (unsigned int i = 0; i < toCheck.size(); ++i)
            {
                size_t count = 0;
                size_t nPos = cacheID.find(toCheck[i], 0);
                while (nPos != std::string::npos)
                {
                    count++;
                    nPos = cacheID.find(toCheck[i], nPos + 1);
                }

                OCIO_CHECK_EQUAL(count, 1);
            }
        }

        {
            const std::vector<uint16_t> resImg
                = {     0,  5105,    58,     0,
                        0,  5159,   260,     65535,
                        0,  5553,   950,     0,
                        0, 16270, 65535,     65535 };

            ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
                __LINE__, processor,
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

            ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
                __LINE__, processor,
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

            ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
                __LINE__, processor,
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

            ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
                __LINE__, processor,
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
        OCIO_CHECK_NO_THROW(config->validate());

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

            ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>( 
                __LINE__, processor,
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

            ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT16>( 
                __LINE__, processor,
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

            ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_F32>( 
                __LINE__, processor,
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

            ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
                __LINE__, processor,
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
        OCIO_CHECK_NO_THROW(config->validate());

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

            ComputeValues<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>( 
                __LINE__, processor,
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

            ComputeValues<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>( 
                __LINE__, processor,
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
        "search_path: " + OCIO::GetTestFilesDir() + "\n"
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
    OCIO_CHECK_NO_THROW(config->validate());

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

constexpr unsigned NB_PIXELS = 6;

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


OCIO::ConstProcessorRcPtr BuildProcessor(OCIO::TransformDirection dir)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::MatrixTransformRcPtr m1 = OCIO::MatrixTransform::Create();
    constexpr double offset1[4] = { 1.0, 0.2, 0.4007, 0.3007 };
    m1->setOffset(offset1);
    m1->setDirection(dir);

    OCIO::MatrixTransformRcPtr m2 = OCIO::MatrixTransform::Create();
    constexpr double offset2[4] = { 0.2002, 0.2, 0.2, 0.2 };
    m2->setOffset(offset2);
    m2->setDirection(dir);

    OCIO::MatrixTransformRcPtr m3 = OCIO::MatrixTransform::Create();
    constexpr double offset3[4] = { 0.2, 0.0005, 0.2, 0.0 };
    m3->setOffset(offset3);
    m3->setDirection(dir);

    auto transform = OCIO::GroupTransform::Create();
    transform->appendTransform(m1);
    transform->appendTransform(m2);
    transform->appendTransform(m3);

    return config->getProcessor(transform);
}

OCIO::ConstCPUProcessorRcPtr BuildCPUProcessor(OCIO::TransformDirection dir)
{
    OCIO::ConstProcessorRcPtr processor = BuildProcessor(dir);
    return processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_NONE);
}

void Validate(const OCIO::PackedImageDesc & imgDesc, unsigned lineNo)
{
    const float * outImg = reinterpret_cast<float*>(imgDesc.getData());
    for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
    {
        OCIO_CHECK_CLOSE_FROM(outImg[4*pxl+0], resImg[4*pxl+0], 1e-6f, lineNo);
        OCIO_CHECK_CLOSE_FROM(outImg[4*pxl+1], resImg[4*pxl+1], 1e-6f, lineNo);
        OCIO_CHECK_CLOSE_FROM(outImg[4*pxl+2], resImg[4*pxl+2], 1e-6f, lineNo);
        OCIO_CHECK_CLOSE_FROM(outImg[4*pxl+3], resImg[4*pxl+3], 1e-6f, lineNo);
    }
}

void Process(const OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
             const OCIO::PackedImageDesc & srcImgDesc,
             OCIO::PackedImageDesc & dstImgDesc,
             unsigned lineNo)
{
    OCIO_CHECK_NO_THROW_FROM(cpuProcessor->apply(srcImgDesc, dstImgDesc), lineNo);
    Validate(dstImgDesc, lineNo);
}

void Process(const OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
             OCIO::PackedImageDesc & imgDesc,
             unsigned lineNo)
{
    OCIO_CHECK_NO_THROW_FROM(cpuProcessor->apply(imgDesc), lineNo);
    Validate(imgDesc, lineNo);
}

void Process(const OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
             const OCIO::PlanarImageDesc & srcImgDesc,
             OCIO::PlanarImageDesc & dstImgDesc,
             unsigned lineNo)
{
    OCIO_CHECK_NO_THROW_FROM(cpuProcessor->apply(srcImgDesc, dstImgDesc), lineNo);

    const float * outImgR = reinterpret_cast<float*>(dstImgDesc.getRData());
    const float * outImgG = reinterpret_cast<float*>(dstImgDesc.getGData());
    const float * outImgB = reinterpret_cast<float*>(dstImgDesc.getBData());
    const float * outImgA = reinterpret_cast<float*>(dstImgDesc.getAData());

    for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
    {
        OCIO_CHECK_CLOSE_FROM(outImgR[pxl], resImg[4*pxl+0], 1e-6f, lineNo);
        OCIO_CHECK_CLOSE_FROM(outImgG[pxl], resImg[4*pxl+1], 1e-6f, lineNo);
        OCIO_CHECK_CLOSE_FROM(outImgB[pxl], resImg[4*pxl+2], 1e-6f, lineNo);
        if(outImgA)
        {
            OCIO_CHECK_CLOSE_FROM(outImgA[pxl], resImg[4*pxl+3], 1e-6f, lineNo);
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
        OCIO_CHECK_CLOSE(outR[idx], resImg[4*idx+0], 1e-6f);
        OCIO_CHECK_CLOSE(outG[idx], resImg[4*idx+1], 1e-6f);
        OCIO_CHECK_CLOSE(outB[idx], resImg[4*idx+2], 1e-6f);
        OCIO_CHECK_CLOSE(outA[idx], resImg[4*idx+3], 1e-6f);
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

OCIO_ADD_TEST(CPUProcessor, scanline_packed)
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
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 2, 3,
                                         OCIO::CHANNEL_ORDERING_RGBA,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         OCIO::AutoStride,
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 2, 3,
                                         OCIO::CHANNEL_ORDERING_RGBA,
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         OCIO::AutoStride,
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 2, 3,
                                         OCIO::CHANNEL_ORDERING_RGBA,
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         4*sizeof(float),
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 2, 3,
                                         4, // Number of channels
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         4*sizeof(float),
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 2, 3,
                                         4, // Number of channels
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         4*sizeof(float),
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 2, 3,
                                         4, // Number of channels
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         4*sizeof(float),
                                         2*4*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], 2, 3,
                                         4, // Number of channels
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         OCIO::AutoStride,
                                         2*4*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }
}

OCIO_ADD_TEST(CPUProcessor, scanline_packed_planar)
{
    // Test to validate the conversion from packed to planar images with a bit-depth different
    // from the default F32.
 
    std::vector<uint8_t> routImg(NB_PIXELS);
    std::vector<uint8_t> goutImg(NB_PIXELS);
    std::vector<uint8_t> boutImg(NB_PIXELS);
    std::vector<uint8_t> aoutImg(NB_PIXELS);

    OCIO::PackedImageDesc srcImgDesc(&inImg[0], 2, 3, 4);
    OCIO::PlanarImageDesc dstImgDesc(&routImg[0], &goutImg[0], &boutImg[0], &aoutImg[0], 2, 3,
                                     OCIO::BIT_DEPTH_UINT8,
                                     OCIO::AutoStride,
                                     OCIO::AutoStride);

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::MatrixTransformRcPtr m = OCIO::MatrixTransform::Create();

    static constexpr double offset[4] = { 0.1, 0.2, 0.4007, 0.3007 };
    m->setOffset(offset);

    OCIO::ConstProcessorRcPtr processor = config->getProcessor(m);
    auto cpuProc = processor->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                       OCIO::BIT_DEPTH_UINT8,
                                                       OCIO::OPTIMIZATION_NONE);

    OCIO_CHECK_NO_THROW(cpuProc->apply(srcImgDesc, dstImgDesc));

    static const std::vector<uint8_t> rresImg = {  0,   0,  51, 179, 255, 255 };
    static const std::vector<uint8_t> gresImg = {  0,   0, 115, 255, 255, 255 };
    static const std::vector<uint8_t> bresImg = {  0,  77, 217, 255, 255, 255 };
    static const std::vector<uint8_t> aresImg = { 78, 180, 255,  78, 255, 101 };

    OCIO_CHECK_ASSERT(routImg == rresImg);
    OCIO_CHECK_ASSERT(goutImg == gresImg);
    OCIO_CHECK_ASSERT(boutImg == bresImg);
    OCIO_CHECK_ASSERT(aoutImg == aresImg);
}

OCIO_ADD_TEST(CPUProcessor, scanline_packed_one_buffer)
{
    // Now that the previous unit test covers all cases with different buffers,
    // let's test some cases using the same in and out buffer.

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_FORWARD));

    std::vector<float> processingImg(NB_PIXELS*4);

    {
        processingImg = inImg;

        OCIO::PackedImageDesc imgDesc(&processingImg[0], NB_PIXELS, 1, 4);

        Process(cpuProcessor, imgDesc, __LINE__);
    }

    {
        processingImg = inImg;

        OCIO::PackedImageDesc imgDesc(&processingImg[0], 3, 2, 4);

        Process(cpuProcessor, imgDesc, __LINE__);
    }

    {
        processingImg = inImg;

        OCIO::PackedImageDesc imgDesc(&processingImg[0], 1, NB_PIXELS, 4);

        Process(cpuProcessor, imgDesc, __LINE__);
    }
}

OCIO_ADD_TEST(CPUProcessor, scanline_planar)
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
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], NB_PIXELS, 1);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], NB_PIXELS, 1);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], NB_PIXELS, 1);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 3, 2);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0], 3, 2);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 2, 3);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0],
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         OCIO::AutoStride);

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 2, 3);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0],
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         2*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0], 2, 3);
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0],
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         2*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0],
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         2*sizeof(float));

        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0],
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         2*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0],
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         2*sizeof(float));

        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0],
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         2*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0],
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         2*sizeof(float));

        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], nullptr,
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         2*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }

    {
        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], nullptr,
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         2*sizeof(float));

        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], nullptr,
                                         2, 3,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         2*sizeof(float));

        Process(cpuProcessor, srcImgDesc, dstImgDesc, __LINE__);
    }
}

OCIO_ADD_TEST(CPUProcessor, scanline_packed_tile)
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
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         4*sizeof(float),
                                         3*4*sizeof(float));

        OCIO::PackedImageDesc dstImgDesc(&outImg[4],
                                         2, 2, 4,   // width=2, height=2, and nchannels=4
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         4*sizeof(float),
                                         3*4*sizeof(float));

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
        {
            OCIO_CHECK_CLOSE(outImg[4*pxl+0], resImg[4*pxl+0], 1e-6f);
            OCIO_CHECK_CLOSE(outImg[4*pxl+1], resImg[4*pxl+1], 1e-6f);
            OCIO_CHECK_CLOSE(outImg[4*pxl+2], resImg[4*pxl+2], 1e-6f);
            OCIO_CHECK_CLOSE(outImg[4*pxl+3], resImg[4*pxl+3], 1e-6f);
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
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         4*sizeof(float),
                                         3*4*sizeof(float));

        OCIO::PackedImageDesc dstImgDesc(&outImg[0],
                                         2, 2, 4,   // width=2, height=2, and nchannels=4
                                         OCIO::BIT_DEPTH_F32,
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
                                         OCIO::BIT_DEPTH_F32,
                                         sizeof(float),
                                         4*sizeof(float),
                                         3*4*sizeof(float));

        Process(cpuProcessor, dstImgDesc, dstImgDesc, __LINE__);
    }

}

OCIO_ADD_TEST(CPUProcessor, scanline_packed_custom)
{
    // Cases testing custom xStrideInBytes and yStrideInBytes values.

    static constexpr float magicNumber = 12345.6789f;
    static constexpr long width  = 3;
    static constexpr long height = 2;

    static_assert(width * height == NB_PIXELS, "Validation of the image dimensions");


    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_FORWARD));

    {
        // Pixels are { RGBA, RGBA, RGBA,
        //              RGBA, RGBA, RGBA  }.

        std::vector<float> img
            = { inImg[ 0], inImg[ 1], inImg[ 2], inImg[ 3],
                inImg[ 4], inImg[ 5], inImg[ 6], inImg[ 7],
                inImg[ 8], inImg[ 9], inImg[10], inImg[11],
                inImg[12], inImg[13], inImg[14], inImg[15],
                inImg[16], inImg[17], inImg[18], inImg[19],
                inImg[20], inImg[21], inImg[22], inImg[23]
              };

        // NB: Do not use OCIO::AutoStride for the y stride to test a custom value.
        static constexpr ptrdiff_t yStrideInBytes = width * 4 * sizeof(float);

        {
            // Test a positive y stride.

            // It means to start the processing from the first pixel of the first line.
            OCIO::PackedImageDesc srcImgDesc(&img[0], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::AutoStride,
                                             OCIO::AutoStride,
                                             // Bytes to the next line.
                                             yStrideInBytes);

            std::vector<float> outImg(NB_PIXELS*4);
            OCIO::PackedImageDesc dstImgDesc(&outImg[0], width, height, 4);

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

            for (size_t pxl = 0; pxl < NB_PIXELS; ++pxl)
            {
                const size_t dep = 4 * pxl;
                OCIO_CHECK_CLOSE(outImg[dep + 0], resImg[dep + 0], 1e-6f);
                OCIO_CHECK_CLOSE(outImg[dep + 1], resImg[dep + 1], 1e-6f);
                OCIO_CHECK_CLOSE(outImg[dep + 2], resImg[dep + 2], 1e-6f);
                OCIO_CHECK_CLOSE(outImg[dep + 3], resImg[dep + 3], 1e-6f);
            }
        }
        {
            // Test a negative y stride.
            //
            // Note: It 'inverts' the processed image i.e. the last line is then moved to become
            // the first line and so on.

            // It means to start the processing from the first pixel of the last line.
            OCIO::PackedImageDesc srcImgDesc(&img[yStrideInBytes / sizeof(float)], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::AutoStride,
                                             OCIO::AutoStride,
                                             // Bytes to the next line.
                                             -yStrideInBytes);

            // Output to 32-bits float.

            std::vector<float> floatOutImg(NB_PIXELS*4);
            OCIO::PackedImageDesc floatDstImgDesc(&floatOutImg[0], width, height, 4);

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, floatDstImgDesc));

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const size_t outDep = (height - y - 1) * width + x;
                    const size_t resDep = y * width + x;

                    OCIO_CHECK_CLOSE(floatOutImg[4 * outDep + 0], resImg[4 * resDep + 0], 1e-6f);
                    OCIO_CHECK_CLOSE(floatOutImg[4 * outDep + 1], resImg[4 * resDep + 1], 1e-6f);
                    OCIO_CHECK_CLOSE(floatOutImg[4 * outDep + 2], resImg[4 * resDep + 2], 1e-6f);
                    OCIO_CHECK_CLOSE(floatOutImg[4 * outDep + 3], resImg[4 * resDep + 3], 1e-6f);
                }
            }

            // Output to 8-bits integer.

            std::vector<uint8_t> charOutImg(NB_PIXELS*4);
            OCIO::PackedImageDesc charDstImgDesc(&charOutImg[0], width, height, 4, 
                                                 OCIO::BIT_DEPTH_UINT8,
                                                 OCIO::AutoStride,
                                                 OCIO::AutoStride,
                                                 OCIO::AutoStride);

            auto new_proc = BuildProcessor(OCIO::TRANSFORM_DIR_FORWARD);
            auto new_cpu = new_proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                              OCIO::BIT_DEPTH_UINT8,
                                                              OCIO::OPTIMIZATION_NONE);

            OCIO_CHECK_NO_THROW(new_cpu->apply(srcImgDesc, charDstImgDesc));

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const size_t outDep = (height - y - 1) * width + x;
                    const size_t resDep = y * width + x;

                    const uint8_t red
                        = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImg[4 * resDep + 0]);
                    OCIO_CHECK_EQUAL(charOutImg[4 * outDep + 0], red);

                    const uint8_t green
                        = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImg[4 * resDep + 1]);
                    OCIO_CHECK_EQUAL(charOutImg[4 * outDep + 1], green);

                    const uint8_t blue
                        = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImg[4 * resDep + 2]);
                    OCIO_CHECK_EQUAL(charOutImg[4 * outDep + 2], blue);

                    const uint8_t alpha
                        = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImg[4 * resDep + 3]);
                    OCIO_CHECK_EQUAL(charOutImg[4 * outDep + 3], alpha);
                }
            }

            // Output to 8-bits integer with a negative y stride.

            static constexpr ptrdiff_t out_yStrideInBytes = width * 4 * sizeof(uint8_t);

            charOutImg.assign(charOutImg.size(), 0);
            OCIO::PackedImageDesc new_charDstImgDesc(&charOutImg[out_yStrideInBytes / sizeof(uint8_t)],
                                                     width, height, 4, 
                                                     OCIO::BIT_DEPTH_UINT8,
                                                     OCIO::AutoStride,
                                                     OCIO::AutoStride,
                                                     -out_yStrideInBytes);

            new_cpu = new_proc->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                         OCIO::BIT_DEPTH_UINT8,
                                                         OCIO::OPTIMIZATION_NONE);

            OCIO_CHECK_NO_THROW(new_cpu->apply(srcImgDesc, new_charDstImgDesc));

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const size_t dep = y * width + x;

                    const uint8_t red
                        = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImg[4 * dep + 0]);
                    OCIO_CHECK_EQUAL(charOutImg[4 * dep + 0], red);

                    const uint8_t green
                        = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImg[4 * dep + 1]);
                    OCIO_CHECK_EQUAL(charOutImg[4 * dep + 1], green);

                    const uint8_t blue
                        = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImg[4 * dep + 2]);
                    OCIO_CHECK_EQUAL(charOutImg[4 * dep + 2], blue);

                    const uint8_t alpha
                        = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImg[4 * dep + 3]);
                    OCIO_CHECK_EQUAL(charOutImg[4 * dep + 3], alpha);
                }
            }
        }
        {
            // Test a negative y stride for the in and out images.
            //
            // Note: For the two images, the processing starts from the last line which means
            // to process from the first pixel of the last line for the two image buffers.

            OCIO::PackedImageDesc srcImgDesc(&img[yStrideInBytes / sizeof(float)], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::AutoStride,
                                             OCIO::AutoStride,
                                             // Bytes to the next line.
                                             -yStrideInBytes);

            std::vector<float> outImg(NB_PIXELS*4);
            OCIO::PackedImageDesc dstImgDesc(&outImg[width * 4 * (height - 1)], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::AutoStride,
                                             OCIO::AutoStride,
                                             // Bytes to the next line.
                                             -1 * (width * 4 * sizeof(float)));

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const size_t dep = y * width + x;

                    OCIO_CHECK_CLOSE(outImg[4 * dep + 0], resImg[4 * dep + 0], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * dep + 1], resImg[4 * dep + 1], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * dep + 2], resImg[4 * dep + 2], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * dep + 3], resImg[4 * dep + 3], 1e-6f);
                }
            }
        }
        {
            // Test a positive y stride with a negative x stride.
            //
            // Note: It 'inverts' the lines of the processed image i.e. the last pixel of a line is
            // then moved to become the first pxel of the same line and so on.

            // It means to start the processing from the last pixel of the first line.
            OCIO::PackedImageDesc srcImgDesc(&img[(width-1) * 4], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::AutoStride,
                                             -1 * (4 * sizeof(float)),
                                             // Bytes to the next line.
                                             yStrideInBytes);

            std::vector<float> outImg(NB_PIXELS*4);
            OCIO::PackedImageDesc dstImgDesc(&outImg[0], width, height, 4);

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const size_t outDep = y * width + (width - x - 1);
                    const size_t resDep = y * width + x;

                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 0], resImg[4 * resDep + 0], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 1], resImg[4 * resDep + 1], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 2], resImg[4 * resDep + 2], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 3], resImg[4 * resDep + 3], 1e-6f);
                }
            }
        }
    }
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

        static constexpr ptrdiff_t yStrideInBytes = width * 4 * sizeof(float) + sizeof(float);

        {
            // Test a positive y stride.

            // It means to start the processing from the first pixel of the first line.
            OCIO::PackedImageDesc srcImgDesc(&img[0], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::AutoStride,
                                             OCIO::AutoStride,
                                             // Bytes to the next line.
                                             yStrideInBytes);

            std::vector<float> outImg(NB_PIXELS*4);
            OCIO::PackedImageDesc dstImgDesc(&outImg[0], width, height, 4);

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

            for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
            {
                OCIO_CHECK_CLOSE(outImg[4*pxl+0], resImg[4*pxl+0], 1e-6f);
                OCIO_CHECK_CLOSE(outImg[4*pxl+1], resImg[4*pxl+1], 1e-6f);
                OCIO_CHECK_CLOSE(outImg[4*pxl+2], resImg[4*pxl+2], 1e-6f);
                OCIO_CHECK_CLOSE(outImg[4*pxl+3], resImg[4*pxl+3], 1e-6f);
            }
        }
        {
            // Test a negative y stride.
            //
            // Note: It 'inverts' the processed image i.e. the last line is then moved to become
            // the first line and so on.

            // It means to start the processing from the first pixel of the last line.
            OCIO::PackedImageDesc srcImgDesc(&img[yStrideInBytes / sizeof(float)], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::AutoStride,
                                             OCIO::AutoStride,
                                             // Bytes to the next line.
                                             -yStrideInBytes);

            std::vector<float> outImg(NB_PIXELS*4);
            OCIO::PackedImageDesc dstImgDesc(&outImg[0], width, height, 4);

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const size_t outDep = (height - y - 1) * width + x;
                    const size_t resDep = y * width + x;

                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 0], resImg[4 * resDep + 0], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 1], resImg[4 * resDep + 1], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 2], resImg[4 * resDep + 2], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 3], resImg[4 * resDep + 3], 1e-6f);
                }
            }
        }
        {
            // Test a negative y stride for the in and out images.
            //
            // Note: For the two images, the processing starts from the last line which means
            // to process from the first pixel of the last line for the two image buffers.

            OCIO::PackedImageDesc srcImgDesc(&img[yStrideInBytes / sizeof(float)], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::AutoStride,
                                             OCIO::AutoStride,
                                             // Bytes to the next line.
                                             -yStrideInBytes);

            std::vector<float> outImg(NB_PIXELS*4);
            OCIO::PackedImageDesc dstImgDesc(&outImg[width * 4 * (height - 1)], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::AutoStride,
                                             OCIO::AutoStride,
                                             // Bytes to the next line.
                                             -1 * (width * 4 * sizeof(float)));

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const size_t dep = y * width + x;

                    OCIO_CHECK_CLOSE(outImg[4 * dep + 0], resImg[4 * dep + 0], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * dep + 1], resImg[4 * dep + 1], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * dep + 2], resImg[4 * dep + 2], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * dep + 3], resImg[4 * dep + 3], 1e-6f);
                }
            }
        }
        {
            // Test a positive y stride with a negative x stride.
            //
            // Note: It 'inverts' the lines of the processed image i.e. the last pixel of a line is
            // then moved to become the first pxel of the same line and so on.

            // It means to start the processing from the last pixel of the first line.
            OCIO::PackedImageDesc srcImgDesc(&img[(width-1) * 4], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             OCIO::AutoStride,
                                             -1 * (4 * sizeof(float)),
                                             // Bytes to the next line.
                                             yStrideInBytes);

            std::vector<float> outImg(NB_PIXELS*4);
            OCIO::PackedImageDesc dstImgDesc(&outImg[0], width, height, 4);

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const size_t outDep = y * width + (width - x - 1);
                    const size_t resDep = y * width + x;

                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 0], resImg[4 * resDep + 0], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 1], resImg[4 * resDep + 1], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 2], resImg[4 * resDep + 2], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[4 * outDep + 3], resImg[4 * resDep + 3], 1e-6f);
                }
            }
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

        static constexpr ptrdiff_t chanInBytes    = sizeof(float) + sizeof(float);
        static constexpr ptrdiff_t xStrideInBytes = chanInBytes * 4;
        static constexpr ptrdiff_t yStrideInBytes = xStrideInBytes * width;

        {
            OCIO::PackedImageDesc srcImgDesc(&img[0], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             // Bytes to the next color channel.
                                             chanInBytes,
                                             OCIO::AutoStride,
                                             OCIO::AutoStride);

            std::vector<float> outImg(NB_PIXELS*3);
            OCIO::PackedImageDesc dstImgDesc(&outImg[0], width, height, 3);

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

            for (size_t pxl = 0; pxl < NB_PIXELS; ++pxl)
            {
                OCIO_CHECK_CLOSE(outImg[3 * pxl + 0], resImg[4 * pxl + 0], 1e-6f);
                OCIO_CHECK_CLOSE(outImg[3 * pxl + 1], resImg[4 * pxl + 1], 1e-6f);
                OCIO_CHECK_CLOSE(outImg[3 * pxl + 2], resImg[4 * pxl + 2], 1e-6f);
            }
        }
        {
            // Test with a negative y stride.

            OCIO::PackedImageDesc srcImgDesc(&img[yStrideInBytes/sizeof(float)], width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             // Bytes to the next color channel.
                                             chanInBytes,
                                             OCIO::AutoStride,
                                             // Bytes to the next line.
                                             -yStrideInBytes);

            std::vector<float> outImg(NB_PIXELS*3);
            OCIO::PackedImageDesc dstImgDesc(&outImg[0], width, height, 3);

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const size_t outDep = (height - y - 1) * width + x;
                    const size_t resDep = y * width + x;

                    OCIO_CHECK_CLOSE(outImg[3 * outDep + 0], resImg[4 * resDep + 0], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[3 * outDep + 1], resImg[4 * resDep + 1], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[3 * outDep + 2], resImg[4 * resDep + 2], 1e-6f);
                }
            }
        }
        {
            // Test with a negative x stride.

            OCIO::PackedImageDesc srcImgDesc(&img[(xStrideInBytes / sizeof(float)) * (width - 1)], 
                                             width, height, 4,
                                             OCIO::BIT_DEPTH_F32,
                                             // Bytes to the next color channel.
                                             chanInBytes,
                                             // Bytes to the next pixel.
                                             -xStrideInBytes,
                                             // Bytes to the next line.
                                             yStrideInBytes);

            std::vector<float> outImg(NB_PIXELS*3);
            OCIO::PackedImageDesc dstImgDesc(&outImg[0], width, height, 3);

            OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

            for (size_t y = 0; y < height; ++y)
            {
                for (size_t x = 0; x < width; ++x)
                {
                    const size_t outDep = y * width + (width - x -1);
                    const size_t resDep = y * width + x;

                    OCIO_CHECK_CLOSE(outImg[3 * outDep + 0], resImg[4 * resDep + 0], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[3 * outDep + 1], resImg[4 * resDep + 1], 1e-6f);
                    OCIO_CHECK_CLOSE(outImg[3 * outDep + 2], resImg[4 * resDep + 2], 1e-6f);
                }
            }
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

        OCIO::PackedImageDesc srcImgDesc(&img[0], width, height, 4,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         // Bytes to the next pixel.
                                         4*sizeof(float)+sizeof(float),
                                         OCIO::AutoStride);

        std::vector<float> outImg(NB_PIXELS*3);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], width, height, 3);

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
        {
            OCIO_CHECK_CLOSE(outImg[3*pxl+0], resImg[4*pxl+0], 1e-6f);
            OCIO_CHECK_CLOSE(outImg[3*pxl+1], resImg[4*pxl+1], 1e-6f);
            OCIO_CHECK_CLOSE(outImg[3*pxl+2], resImg[4*pxl+2], 1e-6f);
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
                                         width, height, 4,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         // Bytes to the next pixel.
                                         4*sizeof(float)+sizeof(float),
                                         // Bytes to the next line.
                                         width*(4*sizeof(float)+sizeof(float))+sizeof(float));

        std::vector<float> outImg(NB_PIXELS*3);
        OCIO::PackedImageDesc dstImgDesc(&outImg[0], width, height, 3);

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for(size_t pxl=0; pxl<NB_PIXELS; ++pxl)
        {
            OCIO_CHECK_CLOSE(outImg[3*pxl+0], resImg[4*pxl+0], 1e-6f);
            OCIO_CHECK_CLOSE(outImg[3*pxl+1], resImg[4*pxl+1], 1e-6f);
            OCIO_CHECK_CLOSE(outImg[3*pxl+2], resImg[4*pxl+2], 1e-6f);
        }
    }

}

OCIO_ADD_TEST(CPUProcessor, scanline_planar_custom)
{
    // Cases testing custom stride values for planar.

    static constexpr long width  = 3;
    static constexpr long height = 2;

    static_assert(width * height == NB_PIXELS, "Validation of the image dimensions");

     {
        // Test with default strides.

        OCIO::ConstCPUProcessorRcPtr cpuProcessor;
        OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_FORWARD));

        std::vector<float> outImgR(NB_PIXELS);
        std::vector<float> outImgG(NB_PIXELS);
        std::vector<float> outImgB(NB_PIXELS);
        std::vector<float> outImgA(NB_PIXELS);

        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0],
                                         width, height,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         width * sizeof(float));
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0],
                                         width, height);

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for (size_t y = 0; y < height; ++y)
        {
            for (size_t x = 0; x < width; ++x)
            {
                const size_t dep = y * width + x;
                OCIO_CHECK_CLOSE(outImgR[dep], resImgR[dep], 1e-6f);
                OCIO_CHECK_CLOSE(outImgG[dep], resImgG[dep], 1e-6f);
                OCIO_CHECK_CLOSE(outImgB[dep], resImgB[dep], 1e-6f);
                OCIO_CHECK_CLOSE(outImgA[dep], resImgA[dep], 1e-6f);
            }
        }
    }

    {
        // Test with default strides, and output in 8-bits integer.

        OCIO::ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW(processor = BuildProcessor(OCIO::TRANSFORM_DIR_FORWARD));

        OCIO::ConstCPUProcessorRcPtr cpuProcessor;
        OCIO_CHECK_NO_THROW(cpuProcessor = processor->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                               OCIO::BIT_DEPTH_UINT8,
                                                                               OCIO::OPTIMIZATION_NONE));

        std::vector<uint8_t> outImgR(NB_PIXELS);
        std::vector<uint8_t> outImgG(NB_PIXELS);
        std::vector<uint8_t> outImgB(NB_PIXELS);
        std::vector<uint8_t> outImgA(NB_PIXELS);

        OCIO::PlanarImageDesc srcImgDesc(&inImgR[0], &inImgG[0], &inImgB[0], &inImgA[0],
                                         width, height,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         width * sizeof(float));
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0],
                                         width, height,
                                         OCIO::BIT_DEPTH_UINT8,
                                         OCIO::AutoStride,
                                         OCIO::AutoStride);

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for (size_t y = 0; y < height; ++y)
        {
            for (size_t x = 0; x < width; ++x)
            {
                const size_t dep = y * width + x;

                const uint8_t red
                    = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImgR[dep]);
                OCIO_CHECK_EQUAL(outImgR[dep], red);

                const uint8_t green
                    = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImgG[dep]);
                OCIO_CHECK_EQUAL(outImgG[dep], green);

                const uint8_t blue
                    = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImgB[dep]);
                OCIO_CHECK_EQUAL(outImgB[dep], blue);

                const uint8_t alpha
                    = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImgA[dep]);
                OCIO_CHECK_EQUAL(outImgA[dep], alpha);
            }
        }
    }

    {
        // Test with a negative y stride.

        OCIO::ConstCPUProcessorRcPtr cpuProcessor;
        OCIO_CHECK_NO_THROW(cpuProcessor = BuildCPUProcessor(OCIO::TRANSFORM_DIR_FORWARD));

        std::vector<float> outImgR(NB_PIXELS);
        std::vector<float> outImgG(NB_PIXELS);
        std::vector<float> outImgB(NB_PIXELS);
        std::vector<float> outImgA(NB_PIXELS);

        OCIO::PlanarImageDesc srcImgDesc(&inImgR[width], &inImgG[width], &inImgB[width], &inImgA[width],
                                         width, height,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         -1 * (width * sizeof(float)));
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[0], &outImgG[0], &outImgB[0], &outImgA[0],
                                         width, height);

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for (size_t y = 0; y < height; ++y)
        {
            for (size_t x = 0; x < width; ++x)
            {
                const size_t outDep = (height - y - 1) * width + x;
                const size_t resDep = y * width + x;
                OCIO_CHECK_CLOSE(outImgR[outDep], resImgR[resDep], 1e-6f);
                OCIO_CHECK_CLOSE(outImgG[outDep], resImgG[resDep], 1e-6f);
                OCIO_CHECK_CLOSE(outImgB[outDep], resImgB[resDep], 1e-6f);
                OCIO_CHECK_CLOSE(outImgA[outDep], resImgA[resDep], 1e-6f);
            }
        }
    }

    {
        // Test with negative y strides on in and out buffers, and output in 8-bits integer.

        OCIO::ConstProcessorRcPtr processor;
        OCIO_CHECK_NO_THROW(processor = BuildProcessor(OCIO::TRANSFORM_DIR_FORWARD));

        OCIO::ConstCPUProcessorRcPtr cpuProcessor;
        OCIO_CHECK_NO_THROW(cpuProcessor = processor->getOptimizedCPUProcessor(OCIO::BIT_DEPTH_F32,
                                                                               OCIO::BIT_DEPTH_UINT8,
                                                                               OCIO::OPTIMIZATION_NONE));

        std::vector<uint8_t> outImgR(NB_PIXELS);
        std::vector<uint8_t> outImgG(NB_PIXELS);
        std::vector<uint8_t> outImgB(NB_PIXELS);
        std::vector<uint8_t> outImgA(NB_PIXELS);

        OCIO::PlanarImageDesc srcImgDesc(&inImgR[width], &inImgG[width], &inImgB[width], &inImgA[width],
                                         width, height,
                                         OCIO::BIT_DEPTH_F32,
                                         OCIO::AutoStride,
                                         -1 * (width * sizeof(float)));
        OCIO::PlanarImageDesc dstImgDesc(&outImgR[width], &outImgG[width], &outImgB[width], &outImgA[width],
                                         width, height,
                                         OCIO::BIT_DEPTH_UINT8,
                                         OCIO::AutoStride,
                                         -1 * (width * sizeof(uint8_t)));

        OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));

        for (size_t y = 0; y < height; ++y)
        {
            for (size_t x = 0; x < width; ++x)
            {
                const size_t dep = y * width + x;

                const uint8_t red
                    = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImgR[dep]);
                OCIO_CHECK_EQUAL(outImgR[dep], red);

                const uint8_t green
                    = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImgG[dep]);
                OCIO_CHECK_EQUAL(outImgG[dep], green);

                const uint8_t blue
                    = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImgB[dep]);
                OCIO_CHECK_EQUAL(outImgB[dep], blue);

                const uint8_t alpha
                    = OCIO::Converter<OCIO::BIT_DEPTH_UINT8>::CastValue(255.0f * resImgA[dep]);
                OCIO_CHECK_EQUAL(outImgA[dep], alpha);
            }
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

namespace
{

template<OCIO::BitDepth inBD, OCIO::BitDepth outBD>
void ComputeImage(unsigned width, unsigned height, unsigned nChannels,
                   const void * inBuf, void * outBuf,
                   unsigned line)
{
    typedef typename OCIO::BitDepthInfo<inBD>::Type InType;
    typedef typename OCIO::BitDepthInfo<outBD>::Type OutType;

    OCIO::ConfigRcPtr config = OCIO::Config::Create();

    OCIO::MatrixTransformRcPtr transform = OCIO::MatrixTransform::Create();
    constexpr double offset4[4] = { 1.2002, 0.4005, 0.8007, 0.5 };
    transform->setOffset( offset4 );

    OCIO::ConstProcessorRcPtr processor;
    OCIO_CHECK_NO_THROW(processor = config->getProcessor(transform));

    OCIO::ConstCPUProcessorRcPtr cpuProcessor;
    OCIO_CHECK_NO_THROW(cpuProcessor
        = processor->getOptimizedCPUProcessor(inBD, outBD,
                                              OCIO::OPTIMIZATION_DEFAULT));

    const OCIO::PackedImageDesc srcImgDesc((void *)inBuf,
                                           width, height, nChannels,
                                           inBD,
                                           OCIO::AutoStride,
                                           OCIO::AutoStride,
                                           OCIO::AutoStride);

    OCIO::PackedImageDesc dstImgDesc(outBuf,
                                     width, height, nChannels,
                                     outBD,
                                     sizeof(OutType),
                                     OCIO::AutoStride,
                                     OCIO::AutoStride);

    OCIO_CHECK_NO_THROW(cpuProcessor->apply(srcImgDesc, dstImgDesc));


    const InType * inValues  = (const InType *)inBuf;
    OutType * outValues = (OutType *)outBuf;

    const float inScale = float(GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32)
                                    / GetBitDepthMaxValue(inBD));

    const float outScale = float( GetBitDepthMaxValue(outBD)
                                    / GetBitDepthMaxValue(OCIO::BIT_DEPTH_F32));

    for(size_t idx=0; idx<(width*height);)
    {
        // Manual computation of the results.
        // Break operations into steps similar to cpu processor
        // to avoid potential fma compiler optimizations
        const float in_scale[4]{ float(inValues[idx+0]) * inScale,
                                 float(inValues[idx+1]) * inScale,
                                 float(inValues[idx+2]) * inScale,
                                 nChannels==4
                                     ? float(inValues[idx+3]) * inScale
                                     : 0.0f
                                };

        const float operation[4]{ in_scale[0] + (float)offset4[0],
                                  in_scale[1] + (float)offset4[1],
                                  in_scale[2] + (float)offset4[2],
                                  in_scale[3] + (float)offset4[3],
                                };

        const float pxl[4]{ operation[0] * outScale,
                            operation[1] * outScale,
                            operation[2] * outScale,
                            operation[3] * outScale,
                      };

        // Validate all the results.

        if(OCIO::BitDepthInfo<outBD>::isFloat)
        {
            OCIO_CHECK_CLOSE_FROM(outValues[idx+0], pxl[0], 1e-6f, line);
            OCIO_CHECK_CLOSE_FROM(outValues[idx+1], pxl[1], 1e-6f, line);
            OCIO_CHECK_CLOSE_FROM(outValues[idx+2], pxl[2], 1e-6f, line);
            if(nChannels==4)
            {
                OCIO_CHECK_CLOSE_FROM(outValues[idx+3], pxl[3], 1e-6f, line);
            }
        }
        else
        {
            OCIO_CHECK_EQUAL_FROM(outValues[idx+0], OCIO::Converter<outBD>::CastValue(pxl[0]), line);
            OCIO_CHECK_EQUAL_FROM(outValues[idx+1], OCIO::Converter<outBD>::CastValue(pxl[1]), line);
            OCIO_CHECK_EQUAL_FROM(outValues[idx+2], OCIO::Converter<outBD>::CastValue(pxl[2]), line);
            if(nChannels==4)
            {
                OCIO_CHECK_EQUAL_FROM(outValues[idx+3], OCIO::Converter<outBD>::CastValue(pxl[3]), line);
            }
        }

        idx += nChannels;
    }
}

}; //anon

OCIO_ADD_TEST(CPUProcessor, optimizations)
{
    // The unit test validates some 'optimization' paths now implemented
    // by the ScanlineHelper class. To fully validate these paths a 'normal' image
    // must be used (i.e. 'few pixels' image is not enough).

    constexpr unsigned width     = 640;
    constexpr unsigned height    = 480;
    constexpr unsigned nChannels = 4;

    // Input and Output are not packed RGBA i.e no optimizations.
    {
        std::vector<uint16_t> inBuf(width*height*3);
        for(size_t idx=0; idx<inBuf.size(); ++idx)
        {
            inBuf[idx] = uint16_t(idx % OCIO::BitDepthInfo<OCIO::BIT_DEPTH_UINT16>::maxValue);
        }

        std::vector<uint16_t> outBuf(width*height*3);

        ComputeImage<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>(width, height, 3,
                                                                     &inBuf[0], &outBuf[0],
                                                                     __LINE__);
    }

    // Input and Output are packed RGBA but not F32.
    {
        std::vector<uint16_t> inBuf(width*height*nChannels);
        for(size_t idx=0; idx<inBuf.size(); ++idx)
        {
            inBuf[idx] = uint16_t(idx % OCIO::BitDepthInfo<OCIO::BIT_DEPTH_UINT16>::maxValue);
        }

        std::vector<uint16_t> outBuf(width*height*nChannels);

        ComputeImage<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_UINT16>(width, height, nChannels,
                                                                     &inBuf[0], &outBuf[0],
                                                                     __LINE__);
    }

    // Input is packed RGBA but not F32, and output is packed RGBA F32.
    {
        std::vector<uint16_t> inBuf(width*height*nChannels);
        for(size_t idx=0; idx<inBuf.size(); ++idx)
        {
            inBuf[idx] = uint16_t(idx % OCIO::BitDepthInfo<OCIO::BIT_DEPTH_UINT16>::maxValue);
        }

        std::vector<float> outBuf(width*height*nChannels);

        ComputeImage<OCIO::BIT_DEPTH_UINT16, OCIO::BIT_DEPTH_F32>(width, height, nChannels,
                                                                  &inBuf[0], &outBuf[0],
                                                                  __LINE__);
    }

    // Input is packed RGBA F32, and output is packed RGBA but not F32.
    {
        std::vector<float> inBuf(width*height*nChannels);
        for(size_t idx=0; idx<inBuf.size(); ++idx)
        {
            inBuf[idx] = float(idx) / float(inBuf.size());
        }

        std::vector<uint16_t> outBuf(width*height*nChannels);

        ComputeImage<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_UINT16>(width, height, nChannels,
                                                                  &inBuf[0], &outBuf[0],
                                                                  __LINE__);
    }

    // Input and output are both packed RGBA F32.
    {
        std::vector<float> inBuf(width*height*nChannels);
        for(size_t idx=0; idx<inBuf.size(); ++idx)
        {
            inBuf[idx] = float(idx) / float(inBuf.size());
        }

        std::vector<float> outBuf(width*height*nChannels);

        ComputeImage<OCIO::BIT_DEPTH_F32, OCIO::BIT_DEPTH_F32>(width, height, nChannels,
                                                               &inBuf[0], &outBuf[0],
                                                               __LINE__);
    }
}
