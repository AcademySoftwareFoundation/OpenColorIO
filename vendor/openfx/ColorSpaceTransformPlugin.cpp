/**
 * OpenColorIO ColorSpace Iop.
*/

#include <cstring>
#include <new>
#include <stdexcept>
#include <iostream>

#include "ofxImageEffect.h"
#include "ofxMemory.h"
#include "ofxMultiThread.h"

#include <OpenColorIO/OpenColorIO.h>

#if defined __APPLE__ || defined linux || defined __FreeBSD__
#  define EXPORT __attribute__((visibility("default")))
#elif defined _WIN32
#  define EXPORT OfxExport
#else
#  error Not building on your operating system quite yet
#endif

namespace OCIO = OCIO_NAMESPACE;

// This is  a custom class made for storing plugin specific data
class ColorSpaceContainer
{
public:
    OfxImageClipHandle m_srcClip;
    OfxImageClipHandle m_dstClip;

    OfxParamHandle m_ColorSpace;
    OfxParamHandle m_srcColorSpace;
    OfxParamHandle m_dstColorSpace;
    OfxParamHandle m_ConfigFile;
};

// Instanciating OCIO ColorSpace Transform API
OCIO::ColorSpaceTransformRcPtr g_ColorSpaceTransform = OCIO::ColorSpaceTransform::Create();

// Getting the current instance configuration
OCIO::ConstConfigRcPtr g_Config = OCIO::GetCurrentConfig();

// Setting up pointers for OpenFX plugin
OfxHost               *g_Host;
OfxImageEffectSuiteV1 *g_EffectHost   = 0;
OfxPropertySuiteV1    *g_PropHost     = 0;
OfxParameterSuiteV1   *g_ParamHost    = 0;
OfxMemorySuiteV1      *g_MemoryHost   = 0;
OfxMultiThreadSuiteV1 *g_ThreadHost   = 0;
OfxMessageSuiteV1     *g_MessageSuite = 0;
OfxInteractSuiteV1    *g_InteractHost = 0;

OfxStatus OnLoad(void)
{
    g_EffectHost   = (OfxImageEffectSuiteV1 *) g_Host->fetchSuite(g_Host->host, kOfxImageEffectSuite, 1);
    g_PropHost     = (OfxPropertySuiteV1 *)    g_Host->fetchSuite(g_Host->host, kOfxPropertySuite, 1);
    g_ParamHost    = (OfxParameterSuiteV1 *)   g_Host->fetchSuite(g_Host->host, kOfxParameterSuite, 1);
    g_MemoryHost   = (OfxMemorySuiteV1 *)      g_Host->fetchSuite(g_Host->host, kOfxMemorySuite, 1);
    g_ThreadHost   = (OfxMultiThreadSuiteV1 *) g_Host->fetchSuite(g_Host->host, kOfxMultiThreadSuite, 1);
    g_MessageSuite = (OfxMessageSuiteV1 *)     g_Host->fetchSuite(g_Host->host, kOfxMessageSuite, 1);
    g_InteractHost = (OfxInteractSuiteV1 *)    g_Host->fetchSuite(g_Host->host, kOfxInteractSuite, 1);

    return kOfxStatOK;
}

// Get ColorSpaceContainer from a property set handle
ColorSpaceContainer* GetContainer(OfxPropertySetHandle effectProps)
{
    ColorSpaceContainer *Container = 0;
    g_PropHost->propGetPointer(effectProps, kOfxPropInstanceData, 0, (void**) &Container);

    return Container;
}

OfxStatus CreateInstance(OfxImageEffectHandle effect)
{
    // get a pointer to the effect properties
    OfxPropertySetHandle effectProps;
    g_EffectHost->getPropertySet(effect, &effectProps);

    // get a pointer to the effect's parameter set
    OfxParamSetHandle paramSet;
    g_EffectHost->getParamSet(effect, &paramSet);

    // Instanciating ColorSpaceContainer
    ColorSpaceContainer *Container = new ColorSpaceContainer;

    // Cache away out clip handles
    g_EffectHost->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &Container->m_srcClip, 0);
    g_EffectHost->clipGetHandle(effect, kOfxImageEffectOutputClipName, &Container->m_dstClip, 0);

    // Caching all the parameters in Container
    g_ParamHost->paramGetHandle(paramSet, "sourceColorspace", &(Container->m_srcColorSpace), 0);
    g_ParamHost->paramGetHandle(paramSet, "destinationColorspace", &(Container->m_dstColorSpace), 0);
    g_ParamHost->paramGetHandle(paramSet, "Config", &(Container->m_ConfigFile), 0);

    g_PropHost->propSetPointer(effectProps, kOfxPropInstanceData, 0, (void *) Container);

    return kOfxStatOK;
}

// Utility function for defining a ColorSpace choice param
static void DefineColorSpaceParam(OfxParamSetHandle effectParams, const char *name, const char *label, const char *scriptName, const char *hint, const char *parent)
{
    OfxPropertySetHandle props;
    OfxStatus status;

    // Setting the param to a Choice type
    status = g_ParamHost->paramDefine(effectParams, kOfxParamTypeChoice, name, &props);
    if(status != kOfxStatOK)
    {
        throw status;
        return;
    }

    // Filling the choices with ColorSpaces available in the current Config
    for (int i = 0; i < g_Config->getNumColorSpaces(); i++)
        g_PropHost->propSetString(props, kOfxParamPropChoiceOption, i, g_Config->getColorSpaceNameByIndex(i));

    // Default value of the param is the colorspace with index 1
    g_PropHost->propSetString(props, kOfxParamPropDefault, 0, g_Config->getColorSpaceNameByIndex(0));
    g_PropHost->propSetString(props, kOfxParamPropHint, 0, hint);
    g_PropHost->propSetString(props, kOfxParamPropScriptName, 0, scriptName);
    g_PropHost->propSetString(props, kOfxPropLabel, 0, label);

    if(parent)
        g_PropHost->propSetString(props, kOfxParamPropParent, 0, parent);
}

// A function to describe context specific properties and parameters
OfxStatus DescribeInContext(OfxImageEffectHandle effect)
{

    OfxPropertySetHandle props;
    // Defining the output clip for the plugin
    g_EffectHost->clipDefine(effect, kOfxImageEffectOutputClipName, &props);

    // Set the component types we can handle on the output
    g_PropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
    g_PropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 1, kOfxImageComponentAlpha);

    // Defining the source clip for the plugin
    g_EffectHost->clipDefine(effect, kOfxImageEffectOutputClipName, &props);

    // Set the component types we can handle on the input
    g_PropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
    g_PropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 1, kOfxImageComponentAlpha);

    // Defining parameters for the plugin
    // Fetching parameter set from the effect handle
    OfxParamSetHandle paramSet;
    g_EffectHost->getParamSet(effect, &paramSet);

    g_ParamHost->paramDefine(paramSet, kOfxParamTypeGroup, "ColorSpaces", &props);
    // TODO: Revise the structure of params and labels
    g_PropHost->propSetString(props, kOfxParamPropHint, 0, "Choose Input and output colorspaces for the transform");
    g_PropHost->propSetString(props, kOfxPropLabel, 0, "ColorSpaces");
    // Input ColorSpace
    DefineColorSpaceParam(paramSet, "srcColorSpace", "Input ColorSpace", "Input ColorSpace", "Choose the input ColorSpace for the Transform", "ColorSpaces");
    // Output ColorSpace
    DefineColorSpaceParam(paramSet, "dstColorSpace", "Output ColorSpace", "Output ColorSpace", "Choose the output ColorSpace for the Transform", "ColorSpaces");

    // Making a page of controls and add the parameters to it
    g_ParamHost->paramDefine(paramSet, kOfxParamTypePage, "Main", &props);
    g_PropHost->propSetString(props, kOfxParamPropPageChild, 0, "srcColorSpace");
    g_PropHost->propSetString(props, kOfxParamPropPageChild, 1, "dstColorSpace");

    return kOfxStatOK;
}

// A function to describe the plugin and define parameters common to all contexts in the Image Effect
OfxStatus Describe(OfxImageEffectHandle effect)
{
    // Getting the property handle out of the suite
    OfxPropertySetHandle effectProps;
    g_EffectHost->getPropertySet(effect, &effectProps);

    // We can support multiple pixel depths and let the clip preferences action deal with it all.
    g_PropHost->propSetInt(effectProps, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 1);

    // Set the bit depths the plugin can handle
    g_PropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthByte);
    g_PropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthShort);
    g_PropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 2, kOfxBitDepthFloat);

    // Set some labels and the group it belongs to
    g_PropHost->propSetString(effectProps, kOfxPropLabel, 0, "OpenColorIO ColorSpace Transform");
    g_PropHost->propSetString(effectProps, kOfxImageEffectPluginPropGrouping, 0, "OpenColorIO");

    // Define the contexts we can be used in
    g_PropHost->propSetString(effectProps, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);

    return kOfxStatOK;
}

// -----------------------------------------------------------------------------------------------
// ---------------------------------- Plugin's Main Entry point ----------------------------------
// -----------------------------------------------------------------------------------------------
static OfxStatus EntryPoint(const char* action, const void* handle, OfxPropertySetHandle inArgs,  OfxPropertySetHandle outArgs)
{
    try
    {
        OfxImageEffectHandle effect = (OfxImageEffectHandle) handle;
        if(strcmp(action, kOfxActionDescribe)==0)
        {
            return Describe(effect);
        }
        if(strcmp(action, kOfxImageEffectActionDescribeInContext) == 0)
        {
            return DescribeInContext(effect);
        }
        if(strcmp(action, kOfxActionLoad) == 0)
        {
            return OnLoad();
        }
        if(strcmp(action, kOfxActionCreateInstance) == 0)
        {
            return CreateInstance(effect);
        }
    }
    catch(std::bad_alloc)
    {
        //catch memory
        std::cerr << "Error: OpenColorIO ColorSpace Transform Plugin    bad_alloc" << std::endl;
        return kOfxStatErrMemory;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: OpenColorIO ColorSpace Transform Plugin    " << e.what();
        return kOfxStatErrUnknown;
    }
    catch(...)
    {
        std::cerr << "Unknown Error: OpenColorIO ColorSpaceTransform Plugin" << std::endl;
    }
}

// ----------------------------------------------------------------------------
// ------------------------ Mandatory OpenFX Functions ------------------------
// ----------------------------------------------------------------------------

// Function for setting the Host
static void SetHost(OfxHost* host)
{
    g_Host = host;
}

// Creating the plugin struct
static OfxPlugin ColorSpaceTransformPlugin = 
{
    kOfxImageEffectPluginApi,
    1,
    "com.OpenColorIO.ColorSpaceTransformPlugin",
    1,
    0,
    SetHost,
    EntryPoint
};

// The two mandated functions
EXPORT OfxPlugin* OfxGetPlugin(int nth)
{
    if(nth == 0)
        return &ColorSpaceTransformPlugin;
    return 0;
}

EXPORT int OfxGetNumberOfPlugins(void)
{
    return 1;
}
