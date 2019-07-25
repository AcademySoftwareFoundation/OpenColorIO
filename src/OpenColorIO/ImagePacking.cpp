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

#include <sstream>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ImagePacking.h"


OCIO_NAMESPACE_ENTER
{


// TODO: GENERIC CASE, SLOW BUT ALWAYS WORKS

    
template<typename Type>
void Generic<Type>::PackRGBAFromImageDesc(const GenericImageDesc & srcImg,
                                          Type * inBitDepthBuffer,
                                          float * outputBuffer,
                                          int & numPixelsCopied,
                                          int outputBufferSize,
                                          long imagePixelStartIndex)
{
    if(outputBuffer==nullptr)
    {
        throw Exception("Invalid output image buffer");
    }

    const long imgWidth  = srcImg.m_width;
    const long imgHeight = srcImg.m_height;
    const long imgPixels = imgWidth * imgHeight;
    
    if(imagePixelStartIndex<0 || imagePixelStartIndex>=imgPixels)
    {
        numPixelsCopied = 0;
        return;
    }
    
    const ptrdiff_t xStrideBytes = srcImg.m_xStrideBytes;
    const ptrdiff_t yStrideBytes = srcImg.m_yStrideBytes;

    long yIndex = imagePixelStartIndex / imgWidth;
    long xIndex = imagePixelStartIndex % imgWidth;
    
    // Figure out our initial ptr positions
    char * rRow = reinterpret_cast<char*>(srcImg.m_rData) + yStrideBytes * yIndex;
    char * gRow = reinterpret_cast<char*>(srcImg.m_gData) + yStrideBytes * yIndex;
    char * bRow = reinterpret_cast<char*>(srcImg.m_bData) + yStrideBytes * yIndex;
    char * aRow = nullptr;
    
    Type * rPtr = reinterpret_cast<Type*>(rRow + xStrideBytes * xIndex);
    Type * gPtr = reinterpret_cast<Type*>(gRow + xStrideBytes * xIndex);
    Type * bPtr = reinterpret_cast<Type*>(bRow + xStrideBytes * xIndex);
    Type * aPtr = nullptr;
    
    if(srcImg.m_aData)
    {
        aRow = reinterpret_cast<char*>(srcImg.m_aData) + yStrideBytes * yIndex;
        aPtr = reinterpret_cast<Type*>(aRow + xStrideBytes*xIndex);
    }

    int pixelsCopied = 0;
    while(pixelsCopied < outputBufferSize)
    {
        // Reorder channels from arbitrary channel ordering to RGBA.
        inBitDepthBuffer[4*pixelsCopied+0] = *rPtr;
        inBitDepthBuffer[4*pixelsCopied+1] = *gPtr;
        inBitDepthBuffer[4*pixelsCopied+2] = *bPtr;
        inBitDepthBuffer[4*pixelsCopied+3] = aPtr ? *aPtr : (Type)0.0f;

        pixelsCopied++;
        xIndex++;

        // Jump to the next scanline
        if(xIndex == imgWidth)
        {
            yIndex += 1;
            if(yIndex == imgHeight)
            {
                // Convert from the input bit-depth to F32 (i.e always in RGBA).
                srcImg.m_bitDepthOp->apply(&inBitDepthBuffer[0], outputBuffer, pixelsCopied);

                numPixelsCopied = pixelsCopied;
                return;
            }
            
            xIndex = 0;
            rRow += yStrideBytes;
            gRow += yStrideBytes;
            bRow += yStrideBytes;
            if(aPtr) aRow += yStrideBytes;
            
            rPtr = reinterpret_cast<Type*>(rRow);
            gPtr = reinterpret_cast<Type*>(gRow);
            bPtr = reinterpret_cast<Type*>(bRow);
            if(aPtr) aPtr = reinterpret_cast<Type*>(aRow);
        }
        // Jump to the next pixel
        else
        {
            rPtr = reinterpret_cast<Type*>(
                reinterpret_cast<char*>(rPtr) + xStrideBytes);
            gPtr = reinterpret_cast<Type*>(
                reinterpret_cast<char*>(gPtr) + xStrideBytes);
            bPtr = reinterpret_cast<Type*>(
                reinterpret_cast<char*>(bPtr) + xStrideBytes);
            if(aPtr)
            {
                aPtr = reinterpret_cast<Type*>(
                    reinterpret_cast<char*>(aPtr) + xStrideBytes);
            }
        }
    }
    
    // Convert from the input bit-depth to F32 (i.e always in RGBA).
    srcImg.m_bitDepthOp->apply(&inBitDepthBuffer[0], outputBuffer, pixelsCopied);

    numPixelsCopied = pixelsCopied;
}

template<>
void Generic<float>::PackRGBAFromImageDesc(const GenericImageDesc & srcImg,
                                           float * /*inBitDepthBuffer*/,
                                           float * outputBuffer,
                                           int & numPixelsCopied,
                                           int outputBufferSize,
                                           long imagePixelStartIndex)
{
    if(outputBuffer==nullptr)
    {
        throw Exception("Invalid output image buffer");
    }

    const long imgWidth  = srcImg.m_width;
    const long imgHeight = srcImg.m_height;
    const long imgPixels = imgWidth * imgHeight;
    
    if(imagePixelStartIndex<0 || imagePixelStartIndex>=imgPixels)
    {
        numPixelsCopied = 0;
        return;
    }
    
    const ptrdiff_t xStrideBytes = srcImg.m_xStrideBytes;
    const ptrdiff_t yStrideBytes = srcImg.m_yStrideBytes;

    long yIndex = imagePixelStartIndex / imgWidth;
    long xIndex = imagePixelStartIndex % imgWidth;
    
    // Figure out our initial ptr positions
    char * rRow = reinterpret_cast<char*>(srcImg.m_rData) + yStrideBytes * yIndex;
    char * gRow = reinterpret_cast<char*>(srcImg.m_gData) + yStrideBytes * yIndex;
    char * bRow = reinterpret_cast<char*>(srcImg.m_bData) + yStrideBytes * yIndex;
    char * aRow = nullptr;
    
    float * rPtr = reinterpret_cast<float*>(rRow + xStrideBytes * xIndex);
    float * gPtr = reinterpret_cast<float*>(gRow + xStrideBytes * xIndex);
    float * bPtr = reinterpret_cast<float*>(bRow + xStrideBytes * xIndex);
    float * aPtr = nullptr;
    
    if(srcImg.m_aData)
    {
        aRow = reinterpret_cast<char*>(srcImg.m_aData) + yStrideBytes * yIndex;
        aPtr = reinterpret_cast<float*>(aRow + xStrideBytes*xIndex);
    }

    int pixelsCopied = 0;
    while(pixelsCopied < outputBufferSize)
    {
        // Reorder channels from arbitrary channel ordering to RGBA.
        outputBuffer[4*pixelsCopied+0] = *rPtr;
        outputBuffer[4*pixelsCopied+1] = *gPtr;
        outputBuffer[4*pixelsCopied+2] = *bPtr;
        outputBuffer[4*pixelsCopied+3] = aPtr ? *aPtr : 0.0f;

        pixelsCopied++;
        xIndex++;

        // Jump to the next scanline
        if(xIndex == imgWidth)
        {
            yIndex += 1;
            if(yIndex == imgHeight)
            {
                // In the float specialization, the BitDepthOp is the first Op
                // of the color processing.
                srcImg.m_bitDepthOp->apply(&outputBuffer[0], &outputBuffer[0], pixelsCopied);

                numPixelsCopied = pixelsCopied;
                return;
            }
            
            xIndex = 0;
            rRow += yStrideBytes;
            gRow += yStrideBytes;
            bRow += yStrideBytes;
            if(aRow) aRow += yStrideBytes;
            
            rPtr = reinterpret_cast<float*>(rRow);
            gPtr = reinterpret_cast<float*>(gRow);
            bPtr = reinterpret_cast<float*>(bRow);
            if(aPtr) aPtr = reinterpret_cast<float*>(aRow);
        }
        // Jump to the next pixel
        else
        {
            rPtr = reinterpret_cast<float*>(
                reinterpret_cast<char*>(rPtr) + xStrideBytes);
            gPtr = reinterpret_cast<float*>(
                reinterpret_cast<char*>(gPtr) + xStrideBytes);
            bPtr = reinterpret_cast<float*>(
                reinterpret_cast<char*>(bPtr) + xStrideBytes);
            if(aPtr)
            {
                aPtr = reinterpret_cast<float*>(
                    reinterpret_cast<char*>(aPtr) + xStrideBytes);
            }
        }
    }
    
    // In the float specialization, the BitDepthOp is the first Op
    // of the color processing.
    srcImg.m_bitDepthOp->apply(&outputBuffer[0], &outputBuffer[0], pixelsCopied);

    numPixelsCopied = pixelsCopied;
}

template<typename Type>
void Generic<Type>::UnpackRGBAToImageDesc(GenericImageDesc & dstImg,
                                          float * inputBuffer,
                                          Type * outBitDepthBuffer,
                                          int numPixelsToUnpack,
                                          long imagePixelStartIndex)
{
    if(inputBuffer==nullptr)
    {
        throw Exception("Invalid input image buffer");
    }

    const long imgWidth  = dstImg.m_width;
    const long imgHeight = dstImg.m_height;
    const long imgPixels = imgWidth * imgHeight;
    
    if(imagePixelStartIndex<0 || imagePixelStartIndex>=imgPixels)
    {
        return;
    }
    
    const ptrdiff_t xStrideBytes = dstImg.m_xStrideBytes;
    const ptrdiff_t yStrideBytes = dstImg.m_yStrideBytes;

    long yIndex = imagePixelStartIndex / imgWidth;
    long xIndex = imagePixelStartIndex % imgWidth;
    
    // Figure out our initial ptr positions
    char * rRow = reinterpret_cast<char*>(dstImg.m_rData) + yStrideBytes * yIndex;
    char * gRow = reinterpret_cast<char*>(dstImg.m_gData) + yStrideBytes * yIndex;
    char * bRow = reinterpret_cast<char*>(dstImg.m_bData) + yStrideBytes * yIndex;
    char * aRow = nullptr;
    
    Type * rPtr = reinterpret_cast<Type*>(rRow + xStrideBytes * xIndex);
    Type * gPtr = reinterpret_cast<Type*>(gRow + xStrideBytes * xIndex);
    Type * bPtr = reinterpret_cast<Type*>(bRow + xStrideBytes * xIndex);
    Type * aPtr = nullptr;
    
    if(dstImg.m_aData)
    {
        aRow = reinterpret_cast<char*>(dstImg.m_aData) + yStrideBytes * yIndex;
        aPtr = reinterpret_cast<Type*>(aRow + xStrideBytes * xIndex);
    }
    
    // Convert from F32 to the output bit-depth (i.e always RGBA).
    dstImg.m_bitDepthOp->apply(&inputBuffer[0], &outBitDepthBuffer[0], numPixelsToUnpack);

    int pixelsCopied = 0;
    while(pixelsCopied < numPixelsToUnpack)
    {
        // Copy from RGBA buffer to arbitrary channel ordering.
        *rPtr = outBitDepthBuffer[4*pixelsCopied];
        *gPtr = outBitDepthBuffer[4*pixelsCopied+1];
        *bPtr = outBitDepthBuffer[4*pixelsCopied+2];
        if(aPtr) *aPtr = outBitDepthBuffer[4*pixelsCopied+3];

        pixelsCopied++;
        xIndex++;
        
        // Jump to the next scanline
        if(xIndex == imgWidth)
        {
            yIndex += 1;
            if(yIndex == imgHeight)
            {
                return;
            }
            
            xIndex = 0;
            rRow += yStrideBytes;
            gRow += yStrideBytes;
            bRow += yStrideBytes;
            if(aPtr) aRow += yStrideBytes;
            
            rPtr = reinterpret_cast<Type*>(rRow);
            gPtr = reinterpret_cast<Type*>(gRow);
            bPtr = reinterpret_cast<Type*>(bRow);
            if(aPtr) aPtr = reinterpret_cast<Type*>(aRow);
        }
        // Jump to the next pixel
        else
        {
            rPtr = reinterpret_cast<Type*>(
                reinterpret_cast<char*>(rPtr) + xStrideBytes);
            gPtr = reinterpret_cast<Type*>(
                reinterpret_cast<char*>(gPtr) + xStrideBytes);
            bPtr = reinterpret_cast<Type*>(
                reinterpret_cast<char*>(bPtr) + xStrideBytes);
            if(aPtr)
            {
                aPtr = reinterpret_cast<Type*>(
                    reinterpret_cast<char*>(aPtr) + xStrideBytes);
            }
        }
    }
}

template<>
void Generic<float>::UnpackRGBAToImageDesc(GenericImageDesc & dstImg,
                                           float * inputBuffer,
                                           float * /*outBitDepthBuffer*/,
                                           int numPixelsToUnpack,
                                           long imagePixelStartIndex)
{
    if(inputBuffer==nullptr)
    {
        throw Exception("Invalid input image buffer");
    }

    const long imgWidth  = dstImg.m_width;
    const long imgHeight = dstImg.m_height;
    const long imgPixels = imgWidth * imgHeight;
    
    if(imagePixelStartIndex<0 || imagePixelStartIndex>=imgPixels)
    {
        return;
    }
    
    const ptrdiff_t xStrideBytes = dstImg.m_xStrideBytes;
    const ptrdiff_t yStrideBytes = dstImg.m_yStrideBytes;

    long yIndex = imagePixelStartIndex / imgWidth;
    long xIndex = imagePixelStartIndex % imgWidth;
    
    // Figure out our initial ptr positions
    char * rRow = reinterpret_cast<char*>(dstImg.m_rData) + yStrideBytes * yIndex;
    char * gRow = reinterpret_cast<char*>(dstImg.m_gData) + yStrideBytes * yIndex;
    char * bRow = reinterpret_cast<char*>(dstImg.m_bData) + yStrideBytes * yIndex;
    char * aRow = nullptr;
    
    float * rPtr = reinterpret_cast<float*>(rRow + xStrideBytes * xIndex);
    float * gPtr = reinterpret_cast<float*>(gRow + xStrideBytes * xIndex);
    float * bPtr = reinterpret_cast<float*>(bRow + xStrideBytes * xIndex);
    float * aPtr = nullptr;
    
    if(dstImg.m_aData)
    {
        aRow = reinterpret_cast<char*>(dstImg.m_aData) + yStrideBytes * yIndex;
        aPtr = reinterpret_cast<float*>(aRow + xStrideBytes * xIndex);
    }
    
    // In the float specialization, the BitDepthOp is the last Op of the color processing.
    dstImg.m_bitDepthOp->apply(&inputBuffer[0], &inputBuffer[0], numPixelsToUnpack);

    int pixelsCopied = 0;
    while(pixelsCopied < numPixelsToUnpack)
    {
        // Copy from RGBA buffer to arbitrary channel ordering.
        *rPtr = inputBuffer[4*pixelsCopied];
        *gPtr = inputBuffer[4*pixelsCopied+1];
        *bPtr = inputBuffer[4*pixelsCopied+2];
        if(aPtr) *aPtr = inputBuffer[4*pixelsCopied+3];

        pixelsCopied++;
        xIndex++;
        
        // Jump to the next scanline
        if(xIndex == imgWidth)
        {
            yIndex += 1;
            if(yIndex == imgHeight)
            {
                return;
            }
            
            xIndex = 0;
            rRow += yStrideBytes;
            gRow += yStrideBytes;
            bRow += yStrideBytes;
            if(aPtr) aRow += yStrideBytes;
            
            rPtr = reinterpret_cast<float*>(rRow);
            gPtr = reinterpret_cast<float*>(gRow);
            bPtr = reinterpret_cast<float*>(bRow);
            if(aPtr) aPtr = reinterpret_cast<float*>(aRow);
        }
        // Jump to the next pixel
        else
        {
            rPtr = reinterpret_cast<float*>(
                reinterpret_cast<char*>(rPtr) + xStrideBytes);
            gPtr = reinterpret_cast<float*>(
                reinterpret_cast<char*>(gPtr) + xStrideBytes);
            bPtr = reinterpret_cast<float*>(
                reinterpret_cast<char*>(bPtr) + xStrideBytes);
            if(aPtr)
            {
                aPtr = reinterpret_cast<float*>(
                    reinterpret_cast<char*>(aPtr) + xStrideBytes);
            }
        }
    }
}
    
    
    /*
    namespace
    {
    
        void PackRGBAFromImageDesc_RGBAMemcpy(const GenericImageDesc& srcImg,
                                              float* outputBuffer,
                                              int* numPixelsCopied,
                                              int outputBufferSize,
                                              int imagePixelStartIndex)
        {
            assert(outputBuffer);
            assert(numPixelsCopied);
            
            long imgWidth = srcImg.getWidth();
            long imgHeight = srcImg.getHeight();
            long imgPixels = srcImg.getNumPixels();
            
            if(imagePixelStartIndex<0 || imagePixelStartIndex>=imgPixels)
            {
                *numPixelsCopied = 0;
                return;
            }
            
            ptrdiff_t xStrideBytes  = srcImg.getXStrideBytes();
            ptrdiff_t yStrideBytes  = srcImg.getYStrideBytes();
            int yIndex = imagePixelStartIndex / imgWidth;
            int xIndex = imagePixelStartIndex % imgWidth;
            
            // Figure out our initial ptr positions
            char* imgRow = reinterpret_cast<char*>(srcImg.getRData()) +
                yStrideBytes * yIndex;
            
            char* imgPtr = imgRow + xStrideBytes*xIndex;
            
            int totalPixelsCopied = 0;
            int pixelsRemainingToCopy = outputBufferSize;
            
            while(pixelsRemainingToCopy>0 && yIndex < imgHeight)
            {
                int imgScanlinePixels = imgWidth - xIndex;
                int numPixels = std::min(imgScanlinePixels,
                                         pixelsRemainingToCopy);
                memcpy(outputBuffer+totalPixelsCopied,
                       imgPtr, numPixels);
                
                yIndex += 1;
                xIndex = 0;
                
                imgRow += yStrideBytes;
                imgPtr = imgRow;
                totalPixelsCopied += numPixels;
                pixelsRemainingToCopy -= numPixels;
            }
            
            if(numPixelsCopied) *numPixelsCopied = totalPixelsCopied;
        }
    }
    */





////////////////////////////////////////////////////////////////////////////


template struct Generic<uint8_t>;
template struct Generic<uint16_t>;
template struct Generic<half>;


}
OCIO_NAMESPACE_EXIT
