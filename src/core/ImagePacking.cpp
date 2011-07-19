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

#include <OpenColorIO/OpenColorIO.h>
#include "ImagePacking.h"

#include <sstream>
#include <iostream>
#include <cassert>
#include <cstring>

OCIO_NAMESPACE_ENTER
{
    float * ImageDesc_GetAData(const ImageDesc& srcImg)
    {
        if(const PackedImageDesc * packedImg = dynamic_cast<const PackedImageDesc*>(&srcImg))
        {
            return packedImg->getAData();
        }
        else if(const PlanarImageDesc * planarImg = dynamic_cast<const PlanarImageDesc*>(&srcImg))
        {
            return planarImg->getAData();
        }
        
        throw Exception("Cannot query alpha ptr on unknown ImageDesc type.");
    }
    
    namespace
    {
        // GENERIC CASE, SLOW BUT ALWAYS WORKS
        
        void PackRGBAFromImageDesc_Generic(const ImageDesc& srcImg,
                                           float* outputBuffer,
                                           int* numPixelsCopied,
                                           int outputBufferSize,
                                           long imagePixelStartIndex)
        {
            assert(outputBuffer);
            assert(numPixelsCopied);
            
            long imgWidth = srcImg.getWidth();
            long imgHeight = srcImg.getHeight();
            long imgPixels = imgWidth * imgHeight;
            
            if(imagePixelStartIndex<0 || imagePixelStartIndex>=imgPixels)
            {
                *numPixelsCopied = 0;
                return;
            }
            
            ptrdiff_t xStrideBytes  = srcImg.getXStrideBytes();
            ptrdiff_t yStrideBytes  = srcImg.getYStrideBytes();
            long yIndex = imagePixelStartIndex / imgWidth;
            long xIndex = imagePixelStartIndex % imgWidth;
            
            // Figure out our initial ptr positions
            char* rRow = reinterpret_cast<char*>(srcImg.getRData()) +
                yStrideBytes * yIndex;
            char* gRow = reinterpret_cast<char*>(srcImg.getGData()) +
                yStrideBytes * yIndex;
            char* bRow = reinterpret_cast<char*>(srcImg.getBData()) +
                yStrideBytes * yIndex;
            char* aRow = NULL;
            
            float* rPtr = reinterpret_cast<float*>(rRow + xStrideBytes*xIndex);
            float* gPtr = reinterpret_cast<float*>(gRow + xStrideBytes*xIndex);
            float* bPtr = reinterpret_cast<float*>(bRow + xStrideBytes*xIndex);
            float* aPtr = NULL;
            
            if(ImageDesc_GetAData(srcImg))
            {
                aRow = reinterpret_cast<char*>(ImageDesc_GetAData(srcImg)) +
                    yStrideBytes * yIndex;
                aPtr = reinterpret_cast<float*>(aRow + xStrideBytes*xIndex);
            }
            
            if(aPtr)
            {
                int pixelsCopied = 0;
                while(pixelsCopied < outputBufferSize)
                {
                    outputBuffer[4*pixelsCopied] = *rPtr;
                    outputBuffer[4*pixelsCopied+1] = *gPtr;
                    outputBuffer[4*pixelsCopied+2] = *bPtr;
                    outputBuffer[4*pixelsCopied+3] = *aPtr;
                    pixelsCopied++;
                    xIndex++;
                    
                    // Jump to the next scanline
                    if(xIndex == imgWidth)
                    {
                        yIndex += 1;
                        if(yIndex == imgHeight)
                        {
                            *numPixelsCopied = pixelsCopied;
                            return;
                        }
                        
                        xIndex = 0;
                        rRow += yStrideBytes;
                        gRow += yStrideBytes;
                        bRow += yStrideBytes;
                        aRow += yStrideBytes;
                        
                        rPtr = reinterpret_cast<float*>(rRow);
                        gPtr = reinterpret_cast<float*>(gRow);
                        bPtr = reinterpret_cast<float*>(bRow);
                        aPtr = reinterpret_cast<float*>(aRow);
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
                        aPtr = reinterpret_cast<float*>(
                            reinterpret_cast<char*>(aPtr) + xStrideBytes);
                    }
                }
                
                *numPixelsCopied = pixelsCopied;
            }
            else
            {
                int pixelsCopied = 0;
                while(pixelsCopied < outputBufferSize)
                {
                    outputBuffer[4*pixelsCopied] = *rPtr;
                    outputBuffer[4*pixelsCopied+1] = *gPtr;
                    outputBuffer[4*pixelsCopied+2] = *bPtr;
                    outputBuffer[4*pixelsCopied+3] = 0.0;
                    pixelsCopied++;
                    xIndex++;
                    
                    // Jump to the next scanline
                    if(xIndex == imgWidth)
                    {
                        yIndex += 1;
                        if(yIndex == imgHeight)
                        {
                            *numPixelsCopied = pixelsCopied;
                            return;
                        }
                        
                        xIndex = 0;
                        rRow += yStrideBytes;
                        gRow += yStrideBytes;
                        bRow += yStrideBytes;
                        
                        rPtr = reinterpret_cast<float*>(rRow);
                        gPtr = reinterpret_cast<float*>(gRow);
                        bPtr = reinterpret_cast<float*>(bRow);
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
                    }
                }
                
                *numPixelsCopied = pixelsCopied;
            }
        }
        
        void UnpackRGBAToImageDesc_Generic(ImageDesc& dstImg,
                                           float* inputBuffer,
                                           int numPixelsToUnpack,
                                           long imagePixelStartIndex)
        {
            assert(inputBuffer);
            
            long imgWidth = dstImg.getWidth();
            long imgHeight = dstImg.getHeight();
            long imgPixels = imgWidth * imgHeight;
            
            if(imagePixelStartIndex<0 || imagePixelStartIndex>=imgPixels)
            {
                return;
            }
            
            ptrdiff_t xStrideBytes  = dstImg.getXStrideBytes();
            ptrdiff_t yStrideBytes  = dstImg.getYStrideBytes();
            long yIndex = imagePixelStartIndex / imgWidth;
            long xIndex = imagePixelStartIndex % imgWidth;
            
            // Figure out our initial ptr positions
            char* rRow = reinterpret_cast<char*>(dstImg.getRData()) +
                yStrideBytes * yIndex;
            char* gRow = reinterpret_cast<char*>(dstImg.getGData()) +
                yStrideBytes * yIndex;
            char* bRow = reinterpret_cast<char*>(dstImg.getBData()) +
                yStrideBytes * yIndex;
            char* aRow = NULL;
            
            float* rPtr = reinterpret_cast<float*>(rRow + xStrideBytes*xIndex);
            float* gPtr = reinterpret_cast<float*>(gRow + xStrideBytes*xIndex);
            float* bPtr = reinterpret_cast<float*>(bRow + xStrideBytes*xIndex);
            float* aPtr = NULL;
            
            if(ImageDesc_GetAData(dstImg))
            {
                aRow = reinterpret_cast<char*>(ImageDesc_GetAData(dstImg)) +
                    yStrideBytes * yIndex;
                aPtr = reinterpret_cast<float*>(aRow + xStrideBytes*xIndex);
            }
            
            if(aPtr)
            {
                int pixelsCopied = 0;
                while(pixelsCopied < numPixelsToUnpack)
                {
                    *rPtr = inputBuffer[4*pixelsCopied];
                    *gPtr = inputBuffer[4*pixelsCopied+1];
                    *bPtr = inputBuffer[4*pixelsCopied+2];
                    *aPtr = inputBuffer[4*pixelsCopied+3];
                    
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
                        aRow += yStrideBytes;
                        
                        rPtr = reinterpret_cast<float*>(rRow);
                        gPtr = reinterpret_cast<float*>(gRow);
                        bPtr = reinterpret_cast<float*>(bRow);
                        aPtr = reinterpret_cast<float*>(aRow);
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
                        aPtr = reinterpret_cast<float*>(
                            reinterpret_cast<char*>(aPtr) + xStrideBytes);
                    }
                }
            }
            else
            {
                int pixelsCopied = 0;
                while(pixelsCopied < numPixelsToUnpack)
                {
                    *rPtr = inputBuffer[4*pixelsCopied];
                    *gPtr = inputBuffer[4*pixelsCopied+1];
                    *bPtr = inputBuffer[4*pixelsCopied+2];
                    
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
                        
                        rPtr = reinterpret_cast<float*>(rRow);
                        gPtr = reinterpret_cast<float*>(gRow);
                        bPtr = reinterpret_cast<float*>(bRow);
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
                    }
                }
            }
        }
        
    }
    
    
    /*
    namespace
    {
    
        void PackRGBAFromImageDesc_RGBAMemcpy(const ImageDesc& srcImg,
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
    
    // TODO: Add optimized codepaths to image packing / unpacking
    
    void PackRGBAFromImageDesc(const ImageDesc& srcImg,
                               float* outputBuffer,
                               int* numPixelsCopied,
                               int outputBufferSize,
                               long imagePixelStartIndex)
    {
        PackRGBAFromImageDesc_Generic(srcImg, outputBuffer,
                                      numPixelsCopied,
                                      outputBufferSize,
                                      imagePixelStartIndex);
    }
    
    
    void UnpackRGBAToImageDesc(ImageDesc& dstImg,
                               float* inputBuffer,
                               int numPixelsToUnpack,
                               long imagePixelStartIndex)
    {
        UnpackRGBAToImageDesc_Generic(dstImg, inputBuffer,
                                      numPixelsToUnpack,
                                      imagePixelStartIndex);
    }
}
OCIO_NAMESPACE_EXIT
