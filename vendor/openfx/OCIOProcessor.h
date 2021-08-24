// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OFX_OCIOPROCESSOR_H
#define INCLUDED_OFX_OCIOPROCESSOR_H

#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsProcessing.H"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

class OCIOProcessor : public OFX::ImageProcessor 
{
protected:
    OFX::Image * _srcImg = nullptr;

    OCIO::ConstCPUProcessorRcPtr _cpuProc;

public:
    OCIOProcessor(OFX::ImageEffect & instance)
        : OFX::ImageProcessor(instance)
    {}

    /* Set the src image */
    void setSrcImg(OFX::Image * img);

    /* Set the processor's transform */
    void setTransform(OCIO::ContextRcPtr context,
                      OCIO::ConstTransformRcPtr transform,
                      OCIO::TransformDirection direction);

    /* Process image on multiple threads */
    void multiThreadProcessImages(OfxRectI procWindow) override;

};

#endif // INCLUDED_OFX_OCIOPROCESSOR_H
