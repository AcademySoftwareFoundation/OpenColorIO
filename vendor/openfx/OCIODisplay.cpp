// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OCIODisplay.h"
#include "OCIOProcessor.h"
#include "OCIOUtils.h"

namespace OCIO = OCIO_NAMESPACE;

OCIODisplay::OCIODisplay(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , srcClip_(0)
    , srcCsNameParam_(0)
    , displayParam_(0)
    , viewParam_(0)
    , inverseParam_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);

    srcCsNameParam_ = fetchChoiceParam("src_cs");
    displayParam_   = fetchChoiceParam("display");
    viewParam_      = fetchChoiceParam("view");
    inverseParam_   = fetchBooleanParam("inverse");

    fetchContextParams(*this, contextParams_);
}

void OCIODisplay::render(const OFX::RenderArguments & args)
{
    // Get images
    std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));

    // Get transform parameters
    std::string srcCsName = getChoiceParamOption(srcCsNameParam_);
    std::string display   = getChoiceParamOption(displayParam_);
    std::string view      = getChoiceParamOption(viewParam_);
    bool inverse = inverseParam_->getValue();

    // Create context with overrides
    OCIO::ContextRcPtr context = createOCIOContext(contextParams_);

    // Build transform
    OCIO::DisplayViewTransformRcPtr tr = OCIO::DisplayViewTransform::Create();
    tr->setSrc(srcCsName.c_str());
    tr->setDisplay(display.c_str());
    tr->setView(view.c_str());

    // Setup and apply processor
    OCIOProcessor proc(*this);

    proc.setDstImg(dst.get());
    proc.setSrcImg(src.get());
    proc.setRenderWindow(args.renderWindow);
    proc.setTransform(context, tr, (inverse ? OCIO::TRANSFORM_DIR_INVERSE 
                                            : OCIO::TRANSFORM_DIR_FORWARD));

    proc.process();
}

bool OCIODisplay::isIdentity(const OFX::IsIdentityArguments & args, 
                                OFX::Clip *& identityClip, 
                                double & identityTime)
{
    std::string srcCsName = getChoiceParamOption(srcCsNameParam_);
    OCIO::ConstColorSpaceRcPtr srcCs;

    if (!srcCsName.empty())
    {
        OCIO::ConstConfigRcPtr config = getOCIOConfig();
        srcCs = config->getColorSpace(srcCsName.c_str());
    }

    // Is processing needed?
    if (!srcCs || srcCs->isData())
    {
        identityClip = srcClip_;
        identityTime = args.time;
        return true;
    }

    return false;
}

void OCIODisplay::changedParam(const OFX::InstanceChangedArgs & /*args*/, 
                               const std::string & paramName)
{
    if (paramName == "display")
    {
        updateViewParamOptions(displayParam_, viewParam_);
    }
}

void OCIODisplayFactory::describe(OFX::ImageEffectDescriptor& desc)
{
    // Labels
    desc.setLabels("OCIODisplay", "OCIODisplay", "OCIODisplay");
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

void OCIODisplayFactory::describeInContext(OFX::ImageEffectDescriptor& desc, 
                                           OFX::ContextEnum /*context*/)
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

    // Define parameters
    OFX::PageParamDescriptor * page = desc.definePageParam("Controls");

    // Src color space
    OFX::ChoiceParamDescriptor * srcCsNameParam = defineCsNameParam(
        desc, 
        "src_cs", 
        "Src Color Space", 
        "Source color space name", 
        0);
    page->addChild(*srcCsNameParam);

    // Display
    OFX::ChoiceParamDescriptor * displayParam = defineDisplayParam(
        desc, 
        "display", 
        "Display", 
        "Display device name", 
        0);
    page->addChild(*displayParam);

    // View
    OFX::ChoiceParamDescriptor * viewParam = defineViewParam(
        desc, 
        "view", 
        "View", 
        "View name", 
        0);
    page->addChild(*viewParam);

    // Inverse
    OFX::BooleanParamDescriptor * inverseParam = defineBooleanParam(
        desc, 
        "inverse", 
        "Inverse", 
        "Invert the transform",
        0);
    page->addChild(*inverseParam);

    // Context overrides
    defineContextParams(desc, page);
}

OFX::ImageEffect * OCIODisplayFactory::createInstance(
    OfxImageEffectHandle handle, 
    OFX::ContextEnum /*context*/)
{
    return new OCIODisplay(handle);
}
