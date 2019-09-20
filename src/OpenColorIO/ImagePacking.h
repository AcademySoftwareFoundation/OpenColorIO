// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_IMAGEPACKING_H
#define INCLUDED_OCIO_IMAGEPACKING_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


OCIO_NAMESPACE_ENTER
{

struct GenericImageDesc
{
    long m_width = 0;
    long m_height = 0;

    ptrdiff_t m_chanStrideBytes = 0;
    ptrdiff_t m_xStrideBytes = 0;
    ptrdiff_t m_yStrideBytes  = 0;
    
    void * m_rData = nullptr;
    void * m_gData = nullptr;
    void * m_bData = nullptr;
    void * m_aData = nullptr;

    bool m_packedFloatRGBA = false;

    BitDepth m_bitDepth;
    // Conversion op to/from F32 to enforce float internal processing.
    ConstOpCPURcPtr m_bitDepthOp;

    
    // Resolves all AutoStride.
    void init(const ImageDesc & img, BitDepth bitDepth, const ConstOpCPURcPtr & bitDepthOp);
    
    bool isPackedFloatRGBA() const;
};

template<typename Type>
struct Generic
{
    static void PackRGBAFromImageDesc(const GenericImageDesc & srcImg,
                                      Type * inBitDepthBuffer,
                                      float * outputBuffer,
                                      int & numPixelsCopied,
                                      int outputBufferSize,
                                      long imagePixelStartIndex);

    static void UnpackRGBAToImageDesc(GenericImageDesc & dstImg,
                                      float * inputBuffer,
                                      Type * outBitDepthBuffer,
                                      int numPixelsToUnpack,
                                      long imagePixelStartIndex);
};

}
OCIO_NAMESPACE_EXIT

#endif
