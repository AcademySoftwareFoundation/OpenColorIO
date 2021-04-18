// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OCIOColorSpace.h"
#include "OCIOProcessor.h"
#include "OCIOUtils.h"

namespace OCIO = OCIO_NAMESPACE;

OCIOColorSpace::OCIOColorSpace(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , srcClip_(0)
    , srcCsNameParam_(0)
    , dstCsNameParam_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);

    srcCsNameParam_ = fetchChoiceParam("src");
    dstCsNameParam_ = fetchChoiceParam("dst");
}

void OCIOColorSpace::render(const OFX::RenderArguments & args)
{
    // Get images
    std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));

    // Get color spaces
    std::string srcCsName = getChoiceParamOption(srcCsNameParam_);
    std::string dstCsName = getChoiceParamOption(dstCsNameParam_);

    // Build transform
    OCIO::ColorSpaceTransformRcPtr tr = OCIO::ColorSpaceTransform::Create();
    tr->setSrc(srcCsName.c_str());
    tr->setDst(dstCsName.c_str());

    // Setup and apply processor
    OCIOProcessor proc(*this);

    proc.setDstImg(dst.get());
    proc.setSrcImg(src.get());
    proc.setRenderWindow(args.renderWindow);
    proc.setTransform(tr);

    proc.process();
}

bool OCIOColorSpace::isIdentity(const OFX::IsIdentityArguments & args, 
                                OFX::Clip *& identityClip, 
                                double & identityTime)
{
    std::string srcCsName = getChoiceParamOption(srcCsNameParam_);
    std::string dstCsName = getChoiceParamOption(dstCsNameParam_);

    // Is processing needed?
    if (srcCsName.empty() || dstCsName.empty() || srcCsName == dstCsName)
    {
        identityClip = srcClip_;
        identityTime = args.time;
        return true;
    }

    return false;
}

void OCIOColorSpaceFactory::describe(OFX::ImageEffectDescriptor& desc)
{
    // Labels
    desc.setLabels("OCIOColorSpace", "OCIOColorSpace", "OCIOColorSpace");
    desc.setPluginGrouping("OpenColorIO");

    // Supported contexts
    desc.addSupportedContext(OFX::eContextFilter);
    desc.addSupportedContext(OFX::eContextGeneral);

    // Supported pixel depths
    desc.addSupportedBitDepth(OFX::eBitDepthUByte);
    desc.addSupportedBitDepth(OFX::eBitDepthUShort);
    desc.addSupportedBitDepth(OFX::eBitDepthHalf);
    desc.addSupportedBitDepth(OFX::eBitDepthFloat);

    // Flags
    desc.setRenderTwiceAlways(false);
}

void OCIOColorSpaceFactory::describeInContext(OFX::ImageEffectDescriptor& desc, 
                                              OFX::ContextEnum context)
{
    // Create the mandated source clip
    OFX::ClipDescriptor * srcClip = desc.defineClip(
        kOfxImageEffectSimpleSourceClipName);

    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGB);

    // Create the mandated output clip
    OFX::ClipDescriptor * dstClip = desc.defineClip(kOfxImageEffectOutputClipName);

    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->addSupportedComponent(OFX::ePixelComponentRGB);

    OFX::PageParamDescriptor * page = desc.definePageParam("Controls");

    OFX::ChoiceParamDescriptor * srcCsNameParam = defineCsNameParam(
        desc, 
        "src", "src", 
        "source color space name", 
        0);
    page->addChild(*srcCsNameParam);

    OFX::ChoiceParamDescriptor * dstCsNameParam = defineCsNameParam(
        desc, 
        "dst", "dst", 
        "destination color space name", 
        0);
    page->addChild(*dstCsNameParam);
}

OFX::ImageEffect * OCIOColorSpaceFactory::createInstance(
    OfxImageEffectHandle handle, 
    OFX::ContextEnum /*context*/)
{
    return new OCIOColorSpace(handle);
}
