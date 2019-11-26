// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ScanlineHelper.h"


namespace OCIO_NAMESPACE
{

Optimizations GetOptimizationMode(const GenericImageDesc & imgDesc)
{
    Optimizations optim = NO_OPTIMIZATION;

    if(imgDesc.isRGBAPacked())
    {
        optim = PACKED_OPTIMIZATION;

        if(imgDesc.isFloat())
        {
            optim = PACKED_FLOAT_OPTIMIZATION;
        }
    }

    return optim;
}


template<typename InType, typename OutType>
GenericScanlineHelper<InType, OutType>::GenericScanlineHelper(BitDepth inputBitDepth,
                                                              const ConstOpCPURcPtr & inBitDepthOp,
                                                              BitDepth outputBitDepth,
                                                              const ConstOpCPURcPtr & outBitDepthOp)
    :   ScanlineHelper()
    ,   m_inputBitDepth(inputBitDepth)
    ,   m_outputBitDepth(outputBitDepth)
    ,   m_inBitDepthOp(inBitDepthOp)
    ,   m_outBitDepthOp(outBitDepthOp)
    ,   m_inOptimizedMode(NO_OPTIMIZATION)
    ,   m_outOptimizedMode(NO_OPTIMIZATION)
    ,   m_yIndex(0)
    ,   m_useDstBuffer(false)
{
}

template<typename InType, typename OutType>
void GenericScanlineHelper<InType, OutType>::init(const ImageDesc & srcImg, const ImageDesc & dstImg)
{
    m_yIndex = 0;

    m_srcImg.init(srcImg, m_inputBitDepth, m_inBitDepthOp);
    m_dstImg.init(dstImg, m_outputBitDepth, m_outBitDepthOp);

    if(m_srcImg.m_width!=m_dstImg.m_width || m_srcImg.m_height!=m_dstImg.m_height)
    {
        throw Exception("Dimension inconsistency between source and destination image buffers.");
    }

    m_inOptimizedMode  = GetOptimizationMode(m_srcImg);
    m_outOptimizedMode = GetOptimizationMode(m_dstImg);

    // Can the output buffer be used as the internal RGBA F32 buffer?
    m_useDstBuffer
        = (m_outOptimizedMode & PACKED_FLOAT_OPTIMIZATION) == PACKED_FLOAT_OPTIMIZATION;

    if( (m_inOptimizedMode & PACKED_OPTIMIZATION) != PACKED_OPTIMIZATION)
    {
        const long bufferSize = 4 * m_dstImg.m_width;
        m_inBitDepthBuffer.resize(bufferSize);
    }

    if(!m_useDstBuffer)
    {
        const long bufferSize = 4 * m_dstImg.m_width;
        m_rgbaFloatBuffer.resize(bufferSize);
        m_outBitDepthBuffer.resize(bufferSize);
    }
}

template<typename InType, typename OutType>
void GenericScanlineHelper<InType, OutType>::init(const ImageDesc & img)
{
    m_yIndex = 0;

    m_srcImg.init(img, m_inputBitDepth, m_inBitDepthOp);
    m_dstImg.init(img, m_outputBitDepth, m_outBitDepthOp);

    m_inOptimizedMode  = GetOptimizationMode(m_srcImg);
    m_outOptimizedMode = m_inOptimizedMode;

    // Can the output buffer be used as the internal RGBA F32 buffer?
    m_useDstBuffer
        = (m_outOptimizedMode & PACKED_FLOAT_OPTIMIZATION) == PACKED_FLOAT_OPTIMIZATION;

    if(!m_useDstBuffer)
    {
        // TODO: Re-use memory from thread-safe memory pool, rather
        // than doing a new allocation each time.

        const long bufferSize = 4 * m_dstImg.m_width;

        m_rgbaFloatBuffer.resize(bufferSize);
        m_inBitDepthBuffer.resize(bufferSize);
        m_outBitDepthBuffer.resize(bufferSize);
    }
}

template<typename InType, typename OutType>
GenericScanlineHelper<InType, OutType>::~GenericScanlineHelper()
{
}

// Copy from the src image to our scanline, in our preferred pixel layout.
template<typename InType, typename OutType>
void GenericScanlineHelper<InType, OutType>::prepRGBAScanline(float** buffer, long & numPixels)
{
    // Note that only a line-by-line processing is done on the image buffer.

    if(m_yIndex >= m_dstImg.m_height)
    {
        numPixels = 0;
        return;
    }

    *buffer = m_useDstBuffer ? (float*)(m_dstImg.m_rData + m_dstImg.m_yStrideBytes * m_yIndex)
                             : &m_rgbaFloatBuffer[0];

    if((m_inOptimizedMode&PACKED_OPTIMIZATION)==PACKED_OPTIMIZATION)
    {
        const void * inBuffer = (void*)(m_srcImg.m_rData + m_srcImg.m_yStrideBytes * m_yIndex);

        m_srcImg.m_bitDepthOp->apply(inBuffer, *buffer, m_dstImg.m_width);
    }
    else
    {
        // Pack from any channel ordering & bit-depth to a packed RGBA F32 buffer.

        Generic<InType>::PackRGBAFromImageDesc(m_srcImg,
                                               &m_inBitDepthBuffer[0],
                                               *buffer,
                                               m_dstImg.m_width,
                                               m_yIndex * m_dstImg.m_width);
    }

    numPixels = m_dstImg.m_width;
}

// Write back the result of our work, from the scanline to our destination image.
template<typename InType, typename OutType>
void GenericScanlineHelper<InType, OutType>::finishRGBAScanline()
{
    // Note that only a line-by-line processing is done on the image buffer.

    if((m_outOptimizedMode&PACKED_OPTIMIZATION)==PACKED_OPTIMIZATION)
    {
        void * out = (void*)(m_dstImg.m_rData + m_dstImg.m_yStrideBytes * m_yIndex);

        const void * in  = m_useDstBuffer ? out : (void*)&m_rgbaFloatBuffer[0];

        m_dstImg.m_bitDepthOp->apply(in, out, m_dstImg.m_width);
    }
    else
    {
        // Unpack from packed RGBA F32 to any channel ordering & bit-depth.
        Generic<OutType>::UnpackRGBAToImageDesc(m_dstImg,
                                                &m_rgbaFloatBuffer[0],
                                                &m_outBitDepthBuffer[0],
                                                m_dstImg.m_width,
                                                m_yIndex * m_dstImg.m_width);
    }

    ++m_yIndex;
}



////////////////////////////////////////////////////////////////////////////




template class GenericScanlineHelper<uint8_t, uint8_t>;
template class GenericScanlineHelper<uint8_t, uint16_t>;
template class GenericScanlineHelper<uint8_t, half>;
template class GenericScanlineHelper<uint8_t, float>;

template class GenericScanlineHelper<uint16_t, uint8_t>;
template class GenericScanlineHelper<uint16_t, uint16_t>;
template class GenericScanlineHelper<uint16_t, half>;
template class GenericScanlineHelper<uint16_t, float>;

template class GenericScanlineHelper<half, uint8_t>;
template class GenericScanlineHelper<half, uint16_t>;
template class GenericScanlineHelper<half, half>;
template class GenericScanlineHelper<half, float>;

template class GenericScanlineHelper<float, uint8_t>;
template class GenericScanlineHelper<float, uint16_t>;
template class GenericScanlineHelper<float, half>;
template class GenericScanlineHelper<float, float>;


} // namespace OCIO_NAMESPACE
