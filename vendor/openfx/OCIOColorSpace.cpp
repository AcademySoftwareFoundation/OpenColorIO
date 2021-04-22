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
    , inverseParam_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);

    srcCsNameParam_ = fetchChoiceParam("src_cs");
    dstCsNameParam_ = fetchChoiceParam("dst_cs");
    inverseParam_   = fetchBooleanParam("inverse");
}

void OCIOColorSpace::render(const OFX::RenderArguments & args)
{
    // Get images
    std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));

    // Get transform parameters
    std::string srcCsName = getChoiceParamOption(srcCsNameParam_);
    std::string dstCsName = getChoiceParamOption(dstCsNameParam_);
    bool inverse = inverseParam_->getValue();

    // Build transform
    OCIO::ColorSpaceTransformRcPtr tr = OCIO::ColorSpaceTransform::Create();
    tr->setSrc(srcCsName.c_str());
    tr->setDst(dstCsName.c_str());
    tr->setDirection((inverse ? OCIO::TRANSFORM_DIR_INVERSE 
                              : OCIO::TRANSFORM_DIR_FORWARD));

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

void OCIOColorSpace::changedParam(const OFX::InstanceChangedArgs & /*args*/, 
                                  const std::string & paramName)
{
    if (paramName == "src_cs" || paramName == "dst_cs")
    {
        OCIO::ConstConfigRcPtr config = getOCIOConfig();

        std::string configName(config->getName());
        if (!configName.empty())
        {
            configName = " '" + configName + "'";
        }

        std::string defaultViewTrName(config->getDefaultViewTransformName());
        int numViewTransforms = config->getNumViewTransforms();

        std::string srcCsName = getChoiceParamOption(srcCsNameParam_);
        OCIO::ConstColorSpaceRcPtr srcCs = 
            config->getColorSpace(srcCsName.c_str());
        OCIO::ReferenceSpaceType srcRef = srcCs->getReferenceSpaceType();

        std::string dstCsName = getChoiceParamOption(dstCsNameParam_);
        OCIO::ConstColorSpaceRcPtr dstCs = 
            config->getColorSpace(dstCsName.c_str());
        OCIO::ReferenceSpaceType dstRef = dstCs->getReferenceSpaceType();

        // Suggest using OCIODisplay instead
        if (numViewTransforms > 1 && srcRef != dstRef)
        {
            std::ostringstream os;
            os << "Color space '" << srcCsName << "' is ";
            os << (srcRef == OCIO::REFERENCE_SPACE_SCENE ? "scene" : "display");
            os << " referred and '" << dstCsName << "' is ";
            os << (dstRef == OCIO::REFERENCE_SPACE_SCENE ? "scene" : "display");
            os << " referred. The OCIO config" << configName << " contains ";
            os << numViewTransforms << " view transforms and the default '";
            os << defaultViewTrName << "' will be used for this conversion, ";
            os << "which may not be the intended behvior. Please use ";
            os << "'OCIODisplay' to ensure a correct viewing pipeline result.";

            sendMessage(OFX::Message::eMessageWarning,
                        "view_transform_warning",
                        os.str());
        }
    }
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

    // src color space
    OFX::ChoiceParamDescriptor * srcCsNameParam = defineCsNameParam(
        desc, 
        "src_cs", 
        "src color space", 
        "source color space name", 
        0);
    page->addChild(*srcCsNameParam);

    // dst color space
    OFX::ChoiceParamDescriptor * dstCsNameParam = defineCsNameParam(
        desc, 
        "dst_cs", 
        "dst color space", 
        "destination color space name", 
        0);
    page->addChild(*dstCsNameParam);

    // inverse
    OFX::BooleanParamDescriptor * inverseParam = defineBooleanParam(
        desc, 
        "inverse", 
        "inverse", 
        "invert the transform", 
        false,
        0);
    page->addChild(*inverseParam);
}

OFX::ImageEffect * OCIOColorSpaceFactory::createInstance(
    OfxImageEffectHandle handle, 
    OFX::ContextEnum /*context*/)
{
    return new OCIOColorSpace(handle);
}
