// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_IMAGEPACKING_H
#define INCLUDED_OCIO_IMAGEPACKING_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


namespace OCIO_NAMESPACE
{

struct GenericImageDesc
{
    long m_width  = 0;
    long m_height = 0;

    ptrdiff_t m_xStrideBytes = 0;
    ptrdiff_t m_yStrideBytes = 0;

    char * m_rData = nullptr;
    char * m_gData = nullptr;
    char * m_bData = nullptr;
    char * m_aData = nullptr;

    // Conversion op to/from 32-bit float to enforce float internal processing.
    ConstOpCPURcPtr m_bitDepthOp;

    // Is the image buffer a RGBA packed buffer?
    bool m_isRGBAPacked = false;
    // Is the image buffer a 32-bit float image buffer?
    bool m_isFloat      = false;


    // Resolves all AutoStride.
    void init(const ImageDesc & img, BitDepth bitDepth, const ConstOpCPURcPtr & bitDepthOp);

    // Is the image buffer a packed RGBA 32-bit float buffer?
    bool isPackedFloatRGBA() const;
    // Is the image buffer a RGBA packed buffer?
    bool isRGBAPacked() const;
    // Is the image buffer a 32-bit float image buffer?
    bool isFloat() const;
};

template<typename Type>
struct Generic
{
    static void PackRGBAFromImageDesc(const GenericImageDesc & srcImg,
                                      Type * inBitDepthBuffer,
                                      float * outputBuffer,
                                      int outputBufferSize,
                                      long imagePixelStartIndex);

    static void UnpackRGBAToImageDesc(GenericImageDesc & dstImg,
                                      float * inputBuffer,
                                      Type * outBitDepthBuffer,
                                      int numPixelsToUnpack,
                                      long imagePixelStartIndex);
};

} // namespace OCIO_NAMESPACE

#endif
