// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ImagePacking.h"


namespace OCIO_NAMESPACE
{


template<typename Type>
void Generic<Type>::PackRGBAFromImageDesc(const GenericImageDesc & srcImg,
                                          Type * inBitDepthBuffer,
                                          float * outputBuffer,
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
        throw Exception("Invalid output image position.");
    }

    const ptrdiff_t xStrideBytes = srcImg.m_xStrideBytes;
    const ptrdiff_t yStrideBytes = srcImg.m_yStrideBytes;

    const long yIndex = imagePixelStartIndex / imgWidth;
    long xIndex = imagePixelStartIndex % imgWidth;

    // Figure out our initial ptr positions
    char * rRow = srcImg.m_rData + yStrideBytes * yIndex;
    char * gRow = srcImg.m_gData + yStrideBytes * yIndex;
    char * bRow = srcImg.m_bData + yStrideBytes * yIndex;
    char * aRow = nullptr;

    Type * rPtr = reinterpret_cast<Type*>(rRow + xStrideBytes * xIndex);
    Type * gPtr = reinterpret_cast<Type*>(gRow + xStrideBytes * xIndex);
    Type * bPtr = reinterpret_cast<Type*>(bRow + xStrideBytes * xIndex);
    Type * aPtr = nullptr;

    if(srcImg.m_aData)
    {
        aRow = srcImg.m_aData + yStrideBytes * yIndex;
        aPtr = reinterpret_cast<Type*>(aRow + xStrideBytes*xIndex);
    }

    // Process one single, complete scanline.
    int pixelsCopied = 0;
    while(pixelsCopied < outputBufferSize)
    {
        // Reorder channels from arbitrary channel ordering to RGBA 32-bit float.
        inBitDepthBuffer[4*pixelsCopied+0] = *rPtr;
        inBitDepthBuffer[4*pixelsCopied+1] = *gPtr;
        inBitDepthBuffer[4*pixelsCopied+2] = *bPtr;
        inBitDepthBuffer[4*pixelsCopied+3] = aPtr ? *aPtr : (Type)0.0f;

        pixelsCopied++;
        xIndex++;

        rPtr = reinterpret_cast<Type*>(reinterpret_cast<char*>(rPtr) + xStrideBytes);
        gPtr = reinterpret_cast<Type*>(reinterpret_cast<char*>(gPtr) + xStrideBytes);
        bPtr = reinterpret_cast<Type*>(reinterpret_cast<char*>(bPtr) + xStrideBytes);
        if(aPtr)
        {
            aPtr = reinterpret_cast<Type*>(reinterpret_cast<char*>(aPtr) + xStrideBytes);
        }
    }

    // Convert from the input bit-depth to F32 (i.e always in RGBA).
    srcImg.m_bitDepthOp->apply(&inBitDepthBuffer[0], outputBuffer, pixelsCopied);
}

template<>
void Generic<float>::PackRGBAFromImageDesc(const GenericImageDesc & srcImg,
                                           float * /*inBitDepthBuffer*/,
                                           float * outputBuffer,
                                           int outputBufferSize,
                                           long imagePixelStartIndex)
{
    if(outputBuffer==nullptr)
    {
        throw Exception("Invalid output image buffer.");
    }

    const long imgWidth  = srcImg.m_width;
    const long imgHeight = srcImg.m_height;
    const long imgPixels = imgWidth * imgHeight;

    if(imagePixelStartIndex<0 || imagePixelStartIndex>=imgPixels)
    {
        throw Exception("Invalid output image position.");
    }

    const ptrdiff_t xStrideBytes = srcImg.m_xStrideBytes;
    const ptrdiff_t yStrideBytes = srcImg.m_yStrideBytes;

    const long yIndex = imagePixelStartIndex / imgWidth;
    long xIndex = imagePixelStartIndex % imgWidth;

    // Figure out our initial ptr positions
    char * rRow = srcImg.m_rData + yStrideBytes * yIndex;
    char * gRow = srcImg.m_gData + yStrideBytes * yIndex;
    char * bRow = srcImg.m_bData + yStrideBytes * yIndex;
    char * aRow = nullptr;

    float * rPtr = reinterpret_cast<float*>(rRow + xStrideBytes * xIndex);
    float * gPtr = reinterpret_cast<float*>(gRow + xStrideBytes * xIndex);
    float * bPtr = reinterpret_cast<float*>(bRow + xStrideBytes * xIndex);
    float * aPtr = nullptr;

    if(srcImg.m_aData)
    {
        aRow = srcImg.m_aData + yStrideBytes * yIndex;
        aPtr = reinterpret_cast<float*>(aRow + xStrideBytes*xIndex);
    }

    // Process one single, complete scanline.
    int pixelsCopied = 0;
    while(pixelsCopied < outputBufferSize)
    {
        // Reorder channels from arbitrary channel ordering to RGBA 32-bit float.
        outputBuffer[4*pixelsCopied+0] = *rPtr;
        outputBuffer[4*pixelsCopied+1] = *gPtr;
        outputBuffer[4*pixelsCopied+2] = *bPtr;
        outputBuffer[4*pixelsCopied+3] = aPtr ? *aPtr : 0.0f;

        pixelsCopied++;
        xIndex++;

        rPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(rPtr) + xStrideBytes);
        gPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(gPtr) + xStrideBytes);
        bPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(bPtr) + xStrideBytes);
        if(aPtr)
        {
            aPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(aPtr) + xStrideBytes);
        }
    }

    // In the float specialization, the BitDepthOp is the first Op of the color processing.
    srcImg.m_bitDepthOp->apply(&outputBuffer[0], &outputBuffer[0], pixelsCopied);
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

    const long yIndex = imagePixelStartIndex / imgWidth;
    long xIndex = imagePixelStartIndex % imgWidth;

    // Figure out our initial ptr positions
    char * rRow = dstImg.m_rData + yStrideBytes * yIndex;
    char * gRow = dstImg.m_gData + yStrideBytes * yIndex;
    char * bRow = dstImg.m_bData + yStrideBytes * yIndex;
    char * aRow = nullptr;

    Type * rPtr = reinterpret_cast<Type*>(rRow + xStrideBytes * xIndex);
    Type * gPtr = reinterpret_cast<Type*>(gRow + xStrideBytes * xIndex);
    Type * bPtr = reinterpret_cast<Type*>(bRow + xStrideBytes * xIndex);
    Type * aPtr = nullptr;

    if(dstImg.m_aData)
    {
        aRow = dstImg.m_aData + yStrideBytes * yIndex;
        aPtr = reinterpret_cast<Type*>(aRow + xStrideBytes * xIndex);
    }

    // Convert from F32 to the output bit-depth (i.e always RGBA).
    dstImg.m_bitDepthOp->apply(&inputBuffer[0], &outBitDepthBuffer[0], numPixelsToUnpack);

    // Process one single, complete scanline.
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

        rPtr = reinterpret_cast<Type*>(reinterpret_cast<char*>(rPtr) + xStrideBytes);
        gPtr = reinterpret_cast<Type*>(reinterpret_cast<char*>(gPtr) + xStrideBytes);
        bPtr = reinterpret_cast<Type*>(reinterpret_cast<char*>(bPtr) + xStrideBytes);
        if(aPtr)
        {
            aPtr = reinterpret_cast<Type*>(reinterpret_cast<char*>(aPtr) + xStrideBytes);
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

    const long yIndex = imagePixelStartIndex / imgWidth;
    long xIndex = imagePixelStartIndex % imgWidth;

    // Figure out our initial ptr positions
    char * rRow = dstImg.m_rData + yStrideBytes * yIndex;
    char * gRow = dstImg.m_gData + yStrideBytes * yIndex;
    char * bRow = dstImg.m_bData + yStrideBytes * yIndex;
    char * aRow = nullptr;

    float * rPtr = reinterpret_cast<float*>(rRow + xStrideBytes * xIndex);
    float * gPtr = reinterpret_cast<float*>(gRow + xStrideBytes * xIndex);
    float * bPtr = reinterpret_cast<float*>(bRow + xStrideBytes * xIndex);
    float * aPtr = nullptr;

    if(dstImg.m_aData)
    {
        aRow = dstImg.m_aData + yStrideBytes * yIndex;
        aPtr = reinterpret_cast<float*>(aRow + xStrideBytes * xIndex);
    }

    // In the float specialization, the BitDepthOp is the last Op of the color processing.
    dstImg.m_bitDepthOp->apply(&inputBuffer[0], &inputBuffer[0], numPixelsToUnpack);

    // Process one single, complete scanline.
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

        rPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(rPtr) + xStrideBytes);
        gPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(gPtr) + xStrideBytes);
        bPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(bPtr) + xStrideBytes);
        if(aPtr)
        {
            aPtr = reinterpret_cast<float*>(reinterpret_cast<char*>(aPtr) + xStrideBytes);
        }
    }
}



////////////////////////////////////////////////////////////////////////////


template struct Generic<uint8_t>;
template struct Generic<uint16_t>;
template struct Generic<half>;


} // namespace OCIO_NAMESPACE
