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


class ScanlineHelper
{
public:
    ScanlineHelper() {};
    ScanlineHelper(const ScanlineHelper &) = delete;
    ScanlineHelper& operator= (const ScanlineHelper &) = delete;

    virtual ~ScanlineHelper() {}

    virtual void init(const ImageDesc & srcImg, ImageDesc & dstImg) = 0;
    virtual void init(ImageDesc & img) = 0;

    virtual void prepRGBAScanline(float** buffer, long & numPixels) = 0;
    
    virtual void finishRGBAScanline() = 0;
};

template<typename InType, typename OutType>
class GenericScanlineHelper : public ScanlineHelper
{
public:
    GenericScanlineHelper() = delete;
    GenericScanlineHelper(BitDepth inputBitDepth, ConstOpCPURcPtr & inBitDepthOp,
                          BitDepth outputBitDepth, ConstOpCPURcPtr & outBitDepthOp);

    void init(const ImageDesc & srcImg, ImageDesc & dstImg) override;
    void init(ImageDesc & img) override;
    
    ~GenericScanlineHelper();
    
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
    
    // Copy mode
    // The mode needs an intermediate RGBA F32 buffer.
    std::vector<float> m_buffer;
    long m_imagePixelIndex;
    int m_numPixelsCopied;

    // Need additional buffers of the same pixel type as the input/output
    // in order to convert arbitrary channel order from/to RGBA.
    std::vector<InType> m_inBitDepthBuffer;
    std::vector<OutType> m_outBitDepthBuffer;
    
    // Number of pixels to process when inplace processing is not possible.
    long m_defaultWidth;

    // In place mode i.e. RGBA F32 only support.
    int m_yIndex;
    bool m_inPlaceMode;
};

    
}
OCIO_NAMESPACE_EXIT

#endif
