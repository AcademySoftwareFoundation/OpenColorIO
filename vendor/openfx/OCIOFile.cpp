// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OCIOFile.h"
#include "OCIOProcessor.h"
#include "OCIOUtils.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{

    const std::string PLUGIN_TYPE = "OCIOFile";

} // namespace

OCIOFile::OCIOFile(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , dstClip_(0)
    , srcClip_(0)
    , srcPathNameParam_(0)
    , inverseParam_(0)
{
    dstClip_ = fetchClip(kOfxImageEffectOutputClipName);
    srcClip_ = fetchClip(kOfxImageEffectSimpleSourceClipName);
    
    srcPathNameParam_ = fetchStringParam(kOfxParamStringIsFilePath);
   
    inverseParam_ = fetchBooleanParam("inverse");

    fetchContextParams(*this, contextParams_);
}

void OCIOFile::render(const OFX::RenderArguments& args)
{
    // Get images
    std::unique_ptr<OFX::Image> dst(dstClip_->fetchImage(args.time));
    std::unique_ptr<OFX::Image> src(srcClip_->fetchImage(args.time));

    // Get file path
    std::string srcFileName;
    srcPathNameParam_->getValue(srcFileName);
  
    bool inverse = inverseParam_->getValue();

    // Create context with overrides
    OCIO::ContextRcPtr context = createOCIOContext(contextParams_);

    // Build file transform
    OCIO::FileTransformRcPtr tr = OCIO::FileTransform::Create();
    tr->setSrc(srcFileName.c_str());

    // Setup and apply processor
    OCIOProcessor proc(*this);

    proc.setDstImg(dst.get());
    proc.setSrcImg(src.get());
    proc.setRenderWindow(args.renderWindow);
    proc.setTransform(context, tr, (inverse ? OCIO::TRANSFORM_DIR_INVERSE
                                            : OCIO::TRANSFORM_DIR_FORWARD));

    proc.process();
}

bool OCIOFile::isIdentity(const OFX::IsIdentityArguments& args,
    OFX::Clip*& identityClip,
    double& identityTime)
{

    std::string srcFileName;
    srcPathNameParam_->getValue(srcFileName);

    // Is processing needed?
    if (srcFileName.empty())
    {
        identityClip = srcClip_;
        identityTime = args.time;
        return true;
    }

    return false;
}

void OCIOFile::changedParam(const OFX::InstanceChangedArgs& /*args*/,
    const std::string& paramName)
{
   // Store context overrides
        contextParamChanged(*this, paramName);
}

void OCIOFileFactory::describe(OFX::ImageEffectDescriptor& desc)
{
    baseDescribe(PLUGIN_TYPE, desc);
}

void OCIOFileFactory::describeInContext(OFX::ImageEffectDescriptor& desc,
    OFX::ContextEnum /*context*/)
{
    baseDescribeInContext(desc);

    // Define parameters
    OFX::PageParamDescriptor* page = desc.definePageParam(PARAM_NAME_PAGE_0);

    // Src path string
    defineStringParam(desc, page,
        "src_path",
        "Source File Path",
        "Source file path name",
        0);

    // Inverse
    defineBooleanParam(desc, page,
        "inverse",
        "Inverse",
        "Invert the transform",
        0);

    // Context overrides
    defineContextParams(desc, page);
}

OFX::ImageEffect* OCIOFileFactory::createInstance(
    OfxImageEffectHandle handle,
    OFX::ContextEnum /*context*/)
{
    return new OCIOFile(handle);
}