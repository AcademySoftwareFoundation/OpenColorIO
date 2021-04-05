// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OCIOUtils.h"

namespace OCIO = OCIO_NAMESPACE;

#include <sstream>

#include "ofxsLog.h"

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

    param->setLabels(label, label, label);
    param->setScriptName(name);
    param->setHint(hint);
    param->setDefault(0);

    if (parent) 
    {
        param->setParent(*parent);
    }

    OCIO::ConstConfigRcPtr config = getOCIOConfig();

    for (int i = 0; i < config->getNumColorSpaces(); i++)
    {
        std::string csName = std::string(config->getColorSpaceNameByIndex(i));
        param->appendOption(csName, csName);
    }

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
