// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


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
