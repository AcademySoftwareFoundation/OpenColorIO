// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OCIOUtils.h"

namespace OCIO = OCIO_NAMESPACE;

#include <sstream>

#include "ofxsLog.h"

namespace
{

void initParam(OFX::ParamDescriptor * param,
               const std::string & name, 
               const std::string & label, 
               const std::string & hint,
               OFX::GroupParamDescriptor * parent)
{
    param->setLabels(label, label, label);
    param->setScriptName(name);
    param->setHint(hint);

    if (parent) 
    {
        param->setParent(*parent);
    }
}

} // namespace

OCIO::ConstConfigRcPtr getOCIOConfig()
{
    OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
    if (!config)
    {
        config = OCIO::Config::CreateFromEnv();
        OCIO::SetCurrentConfig(config);
    }

    return config;
}

OCIO::BitDepth getOCIOBitDepth(OFX::BitDepthEnum ofxBitDepth)
{
    OCIO::BitDepth ocioBitDepth = OCIO::BIT_DEPTH_UNKNOWN;

    switch (ofxBitDepth)
    {
        case OFX::eBitDepthUByte:
            ocioBitDepth = OCIO::BIT_DEPTH_UINT8;
            break;
        case OFX::eBitDepthUShort:
            ocioBitDepth = OCIO::BIT_DEPTH_UINT16;               
            break;
        case OFX::eBitDepthHalf:
            ocioBitDepth = OCIO::BIT_DEPTH_F16;               
            break;
        case OFX::eBitDepthFloat:
            ocioBitDepth = OCIO::BIT_DEPTH_F32;
            break;
        case OFX::eBitDepthNone:
        case OFX::eBitDepthCustom:
        default:
            std::ostringstream os;
            os << "Unsupported bit-depth: ";
            os << OFX::mapBitDepthEnumToStr(ofxBitDepth);
            OFX::Log::error(true, os.str().c_str());
            OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }

    return ocioBitDepth;
}

int getChanStrideBytes(OCIO::BitDepth ocioBitDepth)
{
    int chanStrideBytes = 0;

    switch (ocioBitDepth)
    {
        case OCIO::BIT_DEPTH_UINT8:
            chanStrideBytes = 1;
            break;
        case OCIO::BIT_DEPTH_UINT16:
        case OCIO::BIT_DEPTH_F16:
            chanStrideBytes = 2;               
            break;
        case OCIO::BIT_DEPTH_F32:
            chanStrideBytes = 4;
            break;
        case OCIO::BIT_DEPTH_UNKNOWN:
        case OCIO::BIT_DEPTH_UINT10:
        case OCIO::BIT_DEPTH_UINT12:
        case OCIO::BIT_DEPTH_UINT14:
        case OCIO::BIT_DEPTH_UINT32:
        default:
            std::ostringstream os;
            os << "Unsupported bit-depth: ";
            os << OCIO::BitDepthToString(ocioBitDepth);
            OFX::Log::error(true, os.str().c_str());
            OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }

    return chanStrideBytes;
}

OFX::ChoiceParamDescriptor * defineCsNameParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent)
{
    OFX::ChoiceParamDescriptor * param = desc.defineChoiceParam(name);
    initParam(param, name, label, hint, parent);

    OCIO::ConstConfigRcPtr config = getOCIOConfig();

    // Populate color space names
    for (int i = 0; i < config->getNumColorSpaces(); i++)
    {
        std::string csName(config->getColorSpaceNameByIndex(i));
        param->appendOption(csName, csName);
    }

    // Set a reasonable default color space
    int defaultCsIdx = config->getIndexForColorSpace(OCIO::ROLE_SCENE_LINEAR);
    if (defaultCsIdx == -1)
    {
        defaultCsIdx = config->getIndexForColorSpace(OCIO::ROLE_DEFAULT);
        if (defaultCsIdx == -1)
        {
            defaultCsIdx = 0;
        }
    }
    param->setDefault(defaultCsIdx);

    return param;
}

OFX::ChoiceParamDescriptor * defineDisplayParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent)
{
    OFX::ChoiceParamDescriptor * param = desc.defineChoiceParam(name);
    initParam(param, name, label, hint, parent);

    OCIO::ConstConfigRcPtr config = getOCIOConfig();

    // Populate displays and set default
    std::string defaultDisplay(config->getDefaultDisplay());
    int defaultDisplayIdx = 0;

    for (int i = 0; i < config->getNumDisplays(); i++)
    {
        std::string display(config->getDisplay(i));
        param->appendOption(display, display);

        if (display == defaultDisplay)
        {
            defaultDisplayIdx = i;
        }
    }
    param->setDefault(defaultDisplayIdx);

    return param;
}

OFX::ChoiceParamDescriptor * defineViewParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent)
{
    OFX::ChoiceParamDescriptor * param = desc.defineChoiceParam(name);
    initParam(param, name, label, hint, parent);

    OCIO::ConstConfigRcPtr config = getOCIOConfig();

    // Populate views and set default
    const char * defaultDisplay = config->getDefaultDisplay();

    std::string defaultView(config->getDefaultView(defaultDisplay));
    int defaultViewIdx = 0;

    for (int i = 0; i < config->getNumViews(defaultDisplay); i++)
    {
        std::string view(config->getView(defaultDisplay, i));
        param->appendOption(view, view);

        if (view == defaultView)
        {
            defaultViewIdx = i;
        }
    }
    param->setDefault(defaultViewIdx);

    return param;
}

OFX::BooleanParamDescriptor * defineBooleanParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent,
    bool default_value)
{
    OFX::BooleanParamDescriptor * param = desc.defineBooleanParam(name);
    initParam(param, name, label, hint, parent);

    param->setDefault(default_value);

    return param;
}

std::string getChoiceParamOption(OFX::ChoiceParam * param)
{
    int idx;
    std::string name;
    param->getValue(idx);
    param->getOption(idx, name);

    return name;
}

void updateViewParamOptions(OFX::ChoiceParam * displayParam, 
                            OFX::ChoiceParam * viewParam)
{
    OCIO::ConstConfigRcPtr config = getOCIOConfig();

    // Current display and view
    std::string currentView = getChoiceParamOption(viewParam);
    std::string display     = getChoiceParamOption(displayParam);

    // Clear views
    viewParam->resetOptions();

    // Get new default view
    std::string defaultView(config->getDefaultView(display.c_str()));
    int defaultViewIdx = 0;
    int currentViewIdx = -1;

    // Re=populate views and find current and default index
    for (int i = 0; i < config->getNumViews(display.c_str()); i++)
    {
        std::string view(config->getView(display.c_str(), i));
        viewParam->appendOption(view, view);

        if (view == currentView)
        {
            currentViewIdx = i;
        }
        if (view == defaultView)
        {
            defaultViewIdx = i;
        }
    }

    // Set new current and default option
    if (currentViewIdx == -1)
    {
        currentViewIdx = defaultViewIdx;
    }
    viewParam->setValue(currentViewIdx);
    viewParam->setDefault(defaultViewIdx);
}
