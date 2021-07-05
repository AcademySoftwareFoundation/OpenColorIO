// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OCIOColorSpace.h"
#include "OCIOProcessor.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{

const std::string PLUGIN_TYPE = "OCIOColorSpace";

} // namespace

OCIOColorSpace::OCIOColorSpace(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , srcClip_(0)
    , srcCsNameParam_(0)
    , dstCsNameParam_(0)
    , inverseParam_(0)
    , swapSrcDstParam_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);

    srcCsNameParam_  = fetchChoiceParam("src_cs");
    dstCsNameParam_  = fetchChoiceParam("dst_cs");
    inverseParam_    = fetchBooleanParam("inverse");
    swapSrcDstParam_ = fetchPushButtonParam("swap_src_dst");

    // Handle missing config values
    restoreChoiceParamOption(*this, "src_cs", PLUGIN_TYPE);
    restoreChoiceParamOption(*this, "dst_cs", PLUGIN_TYPE);

    fetchContextParams(*this, contextParams_);
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

    // Create context with overrides
    OCIO::ContextRcPtr context = createOCIOContext(contextParams_);

    // Build transform
    OCIO::ColorSpaceTransformRcPtr tr = OCIO::ColorSpaceTransform::Create();
    tr->setSrc(srcCsName.c_str());
    tr->setDst(dstCsName.c_str());

    // Setup and apply processor
    OCIOProcessor proc(*this);

    proc.setDstImg(dst.get());
    proc.setSrcImg(src.get());
    proc.setRenderWindow(args.renderWindow);
    proc.setTransform(context, tr, (inverse ? OCIO::TRANSFORM_DIR_INVERSE 
                                            : OCIO::TRANSFORM_DIR_FORWARD));

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

        std::string srcCsName = getChoiceParamOption(srcCsNameParam_);
        OCIO::ConstColorSpaceRcPtr srcCs = 
            config->getColorSpace(srcCsName.c_str());
        OCIO::ReferenceSpaceType srcRef = srcCs->getReferenceSpaceType();

        std::string dstCsName = getChoiceParamOption(dstCsNameParam_);
        OCIO::ConstColorSpaceRcPtr dstCs = 
            config->getColorSpace(dstCsName.c_str());
        OCIO::ReferenceSpaceType dstRef = dstCs->getReferenceSpaceType();

        // Suggest using OCIODisplayView instead
        int numViewTransforms = config->getNumViewTransforms();

        if (numViewTransforms > 1 && srcRef != dstRef)
        {
            std::string configName(config->getName());
            if (!configName.empty())
            {
                configName = " '" + configName + "'";
            }

            OCIO::ConstViewTransformRcPtr defaultViewTr = 
                config->getDefaultSceneToDisplayViewTransform();

            std::ostringstream os;
            os << PLUGIN_TYPE << " WARNING: Color space '";
            os << srcCsName << "' is ";
            os << (srcRef == OCIO::REFERENCE_SPACE_SCENE ? "scene" : "display");
            os << "-referred and '" << dstCsName << "' is ";
            os << (dstRef == OCIO::REFERENCE_SPACE_SCENE ? "scene" : "display");
            os << "-referred. The OCIO config" << configName << " contains ";
            os << numViewTransforms << " view transforms and the default '";
            os << defaultViewTr->getName() << "' will be used for this ";
            os << "conversion. If this is not what you want, please ";
            os << "use 'OCIODisplayView' to select your desired view";
            os << "transform.";

            sendMessage(OFX::Message::eMessageWarning,
                        "view_transform_warning",
                        os.str());
        }

        // Store config values
        choiceParamChanged(*this, paramName);
    }
    else if (paramName == "swap_src_dst")
    {
        int srcCsIdx, dstCsIdx;
        srcCsNameParam_->getValue(srcCsIdx);
        dstCsNameParam_->getValue(dstCsIdx);

        // Swap src and dst color space indices
        srcCsNameParam_->setValue(dstCsIdx);
        dstCsNameParam_->setValue(srcCsIdx);
    }
    else
    {
        // Store context overrides
        contextParamChanged(*this, paramName);
    }
}

void OCIOColorSpaceFactory::describe(OFX::ImageEffectDescriptor& desc)
{
    baseDescribe(PLUGIN_TYPE, desc);
}

void OCIOColorSpaceFactory::describeInContext(OFX::ImageEffectDescriptor& desc, 
                                              OFX::ContextEnum /*context*/)
{
    baseDescribeInContext(desc);

    // Define parameters
    OFX::PageParamDescriptor * page = desc.definePageParam(PARAM_NAME_PAGE_0);

    // Src color space
    defineCsNameParam(desc, page, 
                      "src_cs", 
                      "Source Color Space", 
                      "Source color space name", 
                      0);

    // Dst color space
    defineCsNameParam(desc, page, 
                      "dst_cs", 
                      "Destination Color Space", 
                      "Destination color space name", 
                      0);

    // Inverse
    defineBooleanParam(desc, page,
                       "inverse", 
                       "Inverse", 
                       "Invert the transform",
                       0);

    // Swap color spaces
    definePushButtonParam(desc, page,
                          "swap_src_dst", 
                          "Swap color spaces", 
                          "Swap src and dst color spaces",
                          0);

    // Context overrides
    defineContextParams(desc, page);
}

OFX::ImageEffect * OCIOColorSpaceFactory::createInstance(
    OfxImageEffectHandle handle, 
    OFX::ContextEnum /*context*/)
{
    return new OCIOColorSpace(handle);
}
