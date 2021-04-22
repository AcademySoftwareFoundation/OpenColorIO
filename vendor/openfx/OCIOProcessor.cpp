// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OCIOProcessor.h"
#include "OCIOUtils.h"

namespace OCIO = OCIO_NAMESPACE;

#include <sstream>

#include "ofxsLog.h"

void OCIOProcessor::setSrcImg(OFX::Image * v)
{ 
    _srcImg = v;

    // Make sure input and output bit-depth match
    OFX::BitDepthEnum srcBitDepth = _srcImg->getPixelDepth();
    OFX::BitDepthEnum dstBitDepth = _dstImg->getPixelDepth();

    if (srcBitDepth != dstBitDepth)
    {
        std::ostringstream os;
        os << "Input bit-depth mismatch: ";
        os << OFX::mapBitDepthEnumToStr(srcBitDepth);
        os << " != ";
        os << OFX::mapBitDepthEnumToStr(dstBitDepth);
        OFX::Log::error(true, os.str().c_str());
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }

    // Make sure input and output channels match
    OFX::PixelComponentEnum srcComponents = _srcImg->getPixelComponents();
    OFX::PixelComponentEnum dstComponents = _dstImg->getPixelComponents();

    if (srcComponents != dstComponents)
    {
        std::ostringstream os;
        os << "Input component mismatch: ";
        os << OFX::mapPixelComponentEnumToStr(srcComponents);
        os << " != ";
        os << OFX::mapPixelComponentEnumToStr(dstComponents);
        OFX::Log::error(true, os.str().c_str());
        OFX::throwSuiteStatusException(kOfxStatErrFormat);
    }
}

void OCIOProcessor::setTransform(OCIO::ConstTransformRcPtr tr)
{
    OCIO::BitDepth bitDepth = getOCIOBitDepth(_dstImg->getPixelDepth());

    try
    {
        // Throw if the transform is invalid
        tr->validate();

        OCIO::ConstConfigRcPtr config = getOCIOConfig();
        OCIO::ConstProcessorRcPtr proc = config->getProcessor(tr);

        // Build processor which optimizes for input and output bit-depth
        _cpuProc = proc->getOptimizedCPUProcessor(
            bitDepth, bitDepth, 
            OCIO::OPTIMIZATION_DEFAULT);
    }
    catch (const OCIO::Exception & e)
    {
        OFX::Log::error(true, e.what());
        OFX::throwSuiteStatusException(kOfxStatErrFatal);
    }
}

void OCIOProcessor::multiThreadProcessImages(OfxRectI procWindow)
{
    // Inspect image buffer
    OCIO::BitDepth bitDepth = getOCIOBitDepth(_dstImg->getPixelDepth());

    int numChannels = _dstImg->getPixelComponentCount();
    int chanStrideBytes = getChanStrideBytes(bitDepth);
    int xStrideBytes = chanStrideBytes * numChannels;
    int yStrideBytes = _dstImg->getRowBytes();

    // Offset image address to processing window start
    int begin = procWindow.y1 * yStrideBytes 
              + procWindow.x1 * xStrideBytes;
    int w = procWindow.x2 - procWindow.x1;
    int h = procWindow.y2 - procWindow.y1;

    char * srcData = static_cast<char *>(_srcImg->getPixelData());
    srcData += begin;
    char * dstData = static_cast<char *>(_dstImg->getPixelData());
    dstData += begin;

    // Wrap in OCIO image description, which doesn't take ownership of data
    OCIO::PackedImageDesc srcImgDesc(srcData, 
                                        w, h, 
                                        numChannels, 
                                        bitDepth,
                                        chanStrideBytes,
                                        xStrideBytes,
                                        yStrideBytes);

    OCIO::PackedImageDesc dstImgDesc(dstData, 
                                        w, h, 
                                        numChannels, 
                                        bitDepth,
                                        chanStrideBytes,
                                        xStrideBytes,
                                        yStrideBytes);

    // Apply processor on CPU
    _cpuProc->apply(srcImgDesc, dstImgDesc);
}
