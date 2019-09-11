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


#ifndef INCLUDED_OCIO_SCANLINEHELPER_H
#define INCLUDED_OCIO_SCANLINEHELPER_H

#include <OpenColorIO/OpenColorIO.h>

#include "ImagePacking.h"

OCIO_NAMESPACE_ENTER
{

// All potential processing optimizations.
enum Optimizations
{
    NO_OPTIMIZATION     = 0x00,
    PACKED_OPTIMIZATION = 0x01,  // The image is a packed RGBA buffer.
    FLOAT_OPTIMIZATION  = 0x02,  // The image is a F32 i.e. 32-bit float.

    PACKED_FLOAT_OPTIMIZATION = (PACKED_OPTIMIZATION|FLOAT_OPTIMIZATION)
};

Optimizations GetOptimizationMode(const GenericImageDesc & imgDesc);


class ScanlineHelper
{
public:
    ScanlineHelper() = default;
    ScanlineHelper(const ScanlineHelper &) = delete;
    ScanlineHelper& operator=(const ScanlineHelper &) = delete;

    virtual ~ScanlineHelper() = default;

    virtual void init(const ImageDesc & srcImg, const ImageDesc & dstImg) = 0;
    virtual void init(const ImageDesc & img) = 0;

    virtual void prepRGBAScanline(float** buffer, long & numPixels) = 0;
    
    virtual void finishRGBAScanline() = 0;
};

template<typename InType, typename OutType>
class GenericScanlineHelper : public ScanlineHelper
{
public:
    GenericScanlineHelper() = delete;
    GenericScanlineHelper(const GenericScanlineHelper&) = delete;
    GenericScanlineHelper& operator=(const GenericScanlineHelper&) = delete;

    GenericScanlineHelper(BitDepth inputBitDepth, const ConstOpCPURcPtr & inBitDepthOp,
                          BitDepth outputBitDepth, const ConstOpCPURcPtr & outBitDepthOp);

    void init(const ImageDesc & srcImg, const ImageDesc & dstImg) override;
    void init(const ImageDesc & img) override;

    ~GenericScanlineHelper() override;

    // Copy from the src image to our scanline, in our preferred
    // pixel layout. Return the number of pixels to process.

    void prepRGBAScanline(float** buffer, long & numPixels) override;

    // Write back the result of our work, from the scanline to our
    // destination image.

    void finishRGBAScanline() override;

private:
    BitDepth m_inputBitDepth;
    BitDepth m_outputBitDepth;
    ConstOpCPURcPtr m_inBitDepthOp;
    ConstOpCPURcPtr m_outBitDepthOp;

    GenericImageDesc m_srcImg; // Description of the source image.
    GenericImageDesc m_dstImg; // Description of the destination image.

    Optimizations m_inOptimizedMode;  // Optimization applicable to the input buffer.
    Optimizations m_outOptimizedMode; // Optimization applicable to the output buffer.

    // Processing needs an intermediate buffer as CPU Ops only process packed RGBA F32.
    std::vector<float> m_rgbaFloatBuffer;

    // Processing needs additional buffers of the same pixel type as the input/output
    // in order to convert arbitrary channel order from/to RGBA.
    std::vector<InType> m_inBitDepthBuffer;
    std::vector<OutType> m_outBitDepthBuffer;

    // The index of the current line to process.
    int m_yIndex;

    // If the destination buffer is packed RGBA F32 it could then be used
    // as the internal processing buffer (i.e. instead of m_rgbaFloatBuffer
    // and m_outBitDepthBuffer).
    bool m_useDstBuffer;
};


}
OCIO_NAMESPACE_EXIT

#endif
