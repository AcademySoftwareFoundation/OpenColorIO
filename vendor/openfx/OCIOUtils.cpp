// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OCIOUtils.h"

namespace OCIO = OCIO_NAMESPACE;

#include <sstream>
#include <vector>

#include "ofxsLog.h"
#include "pystring/pystring.h"

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

ContextMap deserializeContextStore(const std::string & contextStoreRaw)
{
    ContextMap contextMap;

    // Format: key0:value0;key1:value1;...
    std::vector<std::string> contextPairsRaw;
    pystring::split(contextStoreRaw, contextPairsRaw, ";");

    for (size_t i = 0; i < contextPairsRaw.size(); i++)
    {
        std::vector<std::string> contextPair;
        pystring::split(contextPairsRaw[i], contextPair, ":");
        
        if (contextPair.size() == 2)
        {
            contextMap[contextPair[0]] = contextPair[1];
        }
    }

    return contextMap;
}

std::string serializeContextStore(const ContextMap & contextMap)
{
    std::string contextStoreRaw;

    std::ostringstream os;
    ContextMap::const_iterator it = contextMap.begin();

    for (; it != contextMap.end(); it++)
    {
        os << it->first << ":" << it->second << ";";
    }

    contextStoreRaw = os.str();

    return pystring::rstrip(contextStoreRaw, ";");
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
    bool defaultValue)
{
    OFX::BooleanParamDescriptor * param = desc.defineBooleanParam(name);
    initParam(param, name, label, hint, parent);

    param->setDefault(defaultValue);

    return param;
}

OFX::StringParamDescriptor * defineStringParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent,
    std::string defaultValue,
    OFX::StringTypeEnum stringType)
{
    OFX::StringParamDescriptor * param = desc.defineStringParam(name);
    initParam(param, name, label, hint, parent);

    param->setDefault(defaultValue);
    param->setStringType(stringType);

    return param;
}

OFX::PushButtonParamDescriptor * definePushButtonParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent)
{
    OFX::PushButtonParamDescriptor * param = desc.definePushButtonParam(name);
    initParam(param, name, label, hint, parent);

    return param;
}

void defineContextParams(OFX::ImageEffectDescriptor & desc,
                         OFX::PageParamDescriptor * page)
{
    OFX::GroupParamDescriptor * group = desc.defineGroupParam("Context");
    group->setOpen(false);
    group->setHint("Set or override context variables declared in OCIO config "
                   "'environment' section");

    // Define StringParam per config-declared environment variable
    OCIO::ConstConfigRcPtr config = getOCIOConfig();

    for (int i = 0; i < config->getNumEnvironmentVars(); i++)
    {
        std::string envVarName(
            config->getEnvironmentVarNameByIndex(i));
        std::string envVarDefault(
            config->getEnvironmentVarDefault(envVarName.c_str()));

        OFX::StringParamDescriptor * contextParam = defineStringParam(
            desc, 
            "context_" + envVarName,              // name
            envVarName,                           // label
            ("Set or override context variable: " // hint
             + envVarName + " (default: '" 
             + envVarDefault + "')"),
            group);

        page->addChild(*contextParam);
    }

    // Preserve all context_* param values through OCIO config/context changes
    OFX::StringParamDescriptor * contextStoreParam = defineStringParam(
        desc,
        "context_store",
        "Context store",
        "Persistent context parameter value storage",
        group
    );
    contextStoreParam->setIsSecret(true);

    page->addChild(*contextStoreParam);
}

void fetchContextParams(OFX::ImageEffect & instance, ParamMap & params)
{
    OCIO::ConstConfigRcPtr config = getOCIOConfig();

    OFX::StringParam * contextStoreParam = 
        instance.fetchStringParam("context_store");
    
    // Deserialize raw context store string into a context map
    std::string contextStoreRaw;
    contextStoreParam->getValue(contextStoreRaw);
    ContextMap contextMap = deserializeContextStore(contextStoreRaw);

    // Fetch current context params and set their values from store if empty
    for (int i = 0; i < config->getNumEnvironmentVars(); i++)
    {
        std::string envVarName(config->getEnvironmentVarNameByIndex(i));

        OFX::StringParam * contextParam = 
            instance.fetchStringParam("context_" + envVarName);

        if (contextParam != 0)
        {
            params[envVarName] = contextParam;

            std::string envVarValue;
            contextParam->getValue(envVarValue);

            // Only load from store if param is currently empty
            if (envVarValue.empty())
            {
                ContextMap::const_iterator it = contextMap.find(envVarName);

                if (it != contextMap.end())
                {
                    contextParam->setValue(it->second);
                }
            }
        }
    }
}

void contextParamChanged(OFX::ImageEffect & instance, 
                         const std::string & paramName)
{
    // Is changed param a context variable?
    if (!pystring::startswith(paramName, "context_") 
            || paramName == "context_store")
    {
        return;
    }
    
    OFX::StringParam * contextStoreParam = 
        instance.fetchStringParam("context_store");

    // Deserialize raw context store string into a context map
    std::string contextStoreRaw;
    contextStoreParam->getValue(contextStoreRaw);
    ContextMap contextMap = deserializeContextStore(contextStoreRaw);
    
    // Update context map with new value
    OFX::StringParam * contextParam = instance.fetchStringParam(paramName);

    if (contextParam != 0)
    {
        // "context_name" -> "name"
        std::string envVarName = paramName;
        envVarName.erase(0, 8);

        std::string envVarValue;
        contextParam->getValue(envVarValue);

        // NOTE: This could be storing an empty value
        contextMap[envVarName] = envVarValue;
    }

    // Serialize context map into raw context store string
    contextStoreRaw = serializeContextStore(contextMap);
    contextStoreParam->setValue(contextStoreRaw);
}

OCIO::ContextRcPtr createOCIOContext(ParamMap & params)
{
    OCIO::ConstConfigRcPtr config = getOCIOConfig();
    OCIO::ConstContextRcPtr srcContext = config->getCurrentContext();
    OCIO::ContextRcPtr context = srcContext->createEditableCopy();

    ParamMap::const_iterator it = params.begin();

    for (; it != params.end(); it++)
    {
        std::string value;
        it->second->getValue(value);
        
        if (!value.empty())
        {
            context->setStringVar(it->first.c_str(), value.c_str());
        }
    }

    return context;
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
