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

#include <algorithm>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ScanlineHelper.h"


OCIO_NAMESPACE_ENTER
{

namespace
{
    const long PIXELS_PER_LINE = 4096;
}


template<typename InType, typename OutType>
GenericScanlineHelper<InType, OutType>::GenericScanlineHelper(BitDepth inputBitDepth,
                                                              ConstOpCPURcPtr & inBitDepthOp,
                                                              BitDepth outputBitDepth,
                                                              ConstOpCPURcPtr & outBitDepthOp)
    :   ScanlineHelper()
    ,   m_inputBitDepth(inputBitDepth)
    ,   m_outputBitDepth(outputBitDepth)
    ,   m_inBitDepthOp(inBitDepthOp)
    ,   m_outBitDepthOp(outBitDepthOp)
    ,   m_imagePixelIndex(0)
    ,   m_numPixelsCopied(0)
    ,   m_defaultWidth(0)
    ,   m_yIndex(0)
    ,   m_inPlaceMode(false)
{
}

template<typename InType, typename OutType>
void GenericScanlineHelper<InType, OutType>::init(const ImageDesc & srcImg, ImageDesc & dstImg)
{
    m_imagePixelIndex   = 0;
    m_numPixelsCopied   = 0;
    m_yIndex            = 0;

    m_srcImg.init(srcImg, m_inputBitDepth, m_inBitDepthOp);
    m_dstImg.init(dstImg, m_outputBitDepth, m_outBitDepthOp);

    const long numPixels = m_srcImg.m_width * m_srcImg.m_height;
    if( numPixels!=long(m_dstImg.m_width*m_dstImg.m_height) )
    {
        throw Exception("Number of pixel is inconsistent between source and destination image buffers.");
    }

    // TODO: Even in 'in-place' mode, a potential optimization would be to process more than one 
    //       scanline for images that are both small and have contiguous scanlines.

    // It would be great to perform inplace processing.
    m_inPlaceMode = m_srcImg.isPackedFloatRGBA() && m_dstImg.isPackedFloatRGBA() 
                        && m_srcImg.m_rData==m_dstImg.m_rData;

    if(!m_inPlaceMode)
    {
        // It would be great to process several lines in one shot 
        // (or the complete image if the number of pixel is lower than PIXELS_PER_LINE).
        m_defaultWidth = std::max(m_dstImg.m_width, PIXELS_PER_LINE);
        m_defaultWidth = std::min(numPixels, m_defaultWidth);

        // TODO: Re-use memory from thread-safe memory pool, rather
        // than doing a new allocation each time.

        const long bufferSize = 4 * m_defaultWidth;

        m_buffer.resize(bufferSize);
        m_inBitDepthBuffer.resize(bufferSize);
        m_outBitDepthBuffer.resize(bufferSize);
    }
}

template<typename InType, typename OutType>
void GenericScanlineHelper<InType, OutType>::init(ImageDesc & img)
{
    m_imagePixelIndex   = 0;
    m_numPixelsCopied   = 0;
    m_yIndex            = 0;

    m_srcImg.init(img, m_inputBitDepth, m_inBitDepthOp);
    m_dstImg.init(img, m_outputBitDepth, m_outBitDepthOp);

    // It would be great to perform inplace processing.
    m_inPlaceMode = m_srcImg.isPackedFloatRGBA();

    if(!m_inPlaceMode)
    {
        const long numPixels = m_srcImg.m_width * m_srcImg.m_height;

        // It would be great to process several lines in one shot 
        // (or the complete image if the number of pixel is lower than PIXELS_PER_LINE).
        m_defaultWidth = std::max(m_dstImg.m_width, PIXELS_PER_LINE);
        m_defaultWidth = std::min(numPixels, m_defaultWidth);

        // TODO: Re-use memory from thread-safe memory pool, rather
        // than doing a new allocation each time.

        m_buffer.resize(4 * m_defaultWidth);

        m_inBitDepthBuffer.resize(4 * m_defaultWidth);
        m_outBitDepthBuffer.resize(4 * m_defaultWidth);
    }
}

template<typename InType, typename OutType>
GenericScanlineHelper<InType, OutType>::~GenericScanlineHelper()
{
}

// Copy from the src image to our scanline, in our preferred
// pixel layout.

template<typename InType, typename OutType>
void GenericScanlineHelper<InType, OutType>::prepRGBAScanline(float** buffer, long & numPixels)
{
    if(m_inPlaceMode)
    {
        if(m_yIndex >= m_dstImg.m_height)
        {
            numPixels = 0;
            return;
        }

        // In-place mode is always RGBA F32, so we may apply the first op 
        // rather than calling PackRGBAFromImageDesc().

        *buffer = (float*)(((char*)m_dstImg.m_rData) + m_dstImg.m_yStrideBytes * m_yIndex);

        m_srcImg.m_bitDepthOp->apply(*buffer, *buffer, m_dstImg.m_width);

        numPixels = m_dstImg.m_width;
    }
    else
    {
        // Pack from any channel ordering & bit-depth to RGBA F32.
        Generic<InType>::PackRGBAFromImageDesc(m_srcImg,
                                               &m_inBitDepthBuffer[0],
                                               &m_buffer[0],
                                               m_numPixelsCopied,
                                               m_defaultWidth,
                                               m_imagePixelIndex);

        *buffer   = &m_buffer[0];
        numPixels = m_numPixelsCopied;
    }
}

// Write back the result of our work, from the scanline to our
// destination image.
template<typename InType, typename OutType>
void GenericScanlineHelper<InType, OutType>::finishRGBAScanline()
{
    if(m_inPlaceMode)
    {
        // When in RGBA F32, apply the last op.

        float * out = (float*)(((char*)m_dstImg.m_rData) + m_dstImg.m_yStrideBytes * m_yIndex);

        m_dstImg.m_bitDepthOp->apply(out, out, m_dstImg.m_width);

        ++m_yIndex;
    }
    else
    {
        // Unpack from RGBA F32 to any channel ordering & bit-depth.
        Generic<OutType>::UnpackRGBAToImageDesc(m_dstImg,
                                                &m_buffer[0],
                                                &m_outBitDepthBuffer[0],
                                                m_numPixelsCopied,
                                                m_imagePixelIndex);

        m_imagePixelIndex += m_numPixelsCopied;
    }
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


}
OCIO_NAMESPACE_EXIT
