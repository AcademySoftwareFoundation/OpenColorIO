#include <cstring>
#include <new>
#include <stdexcept>
#include <iostream>

#include "ofxImageEffect.h"
#include "ofxMemory.h"
#include "ofxMultiThread.h"

#include <OpenColorIO/OpenColorIO.h>

// Meta data about the plugin
#define OCIOPluginIdentifier "org.OpenColorIO.DisplayViewTransformPlugin"
#define OCIOPluginName "DisplayView Transform"
#define OCIOPluginGroup "Color/OpenColorIO"
#define OCIOPluginDescription "A plugin for display view transform through OCIO"
#define OCIOSourceColorSpace "srcColorSpace"
#define OCIOSourceColorSpaceHint "Select a input colorspace for the displayview transform"
#define OCIOConfigParam "config"
#define OCIOConfigParamHint "Locate the config file for OCIO"
#define OCIODisplayParam "display"
#define OCIODisplayParamHint "Choose a display device for the displayview transform"
#define OCIOViewParam "view"
#define OCIOViewParamHint "Choose a view for the displayview transform"


#if defined __APPLE__ || defined linux || defined __FreeBSD__
#  define EXPORT __attribute__((visibility("default")))
#elif defined _WIN32
#  define EXPORT OfxExport
#else
#  error Not building on your operating system quite yet
#endif

namespace OCIO = OCIO_NAMESPACE;

// This is  a custom class made for storing plugin specific data
class DisplayViewContainer
{
public:

    // Source Clip
    OfxImageClipHandle m_srcClip;
    // Destination Clip
    OfxImageClipHandle m_dstClip;

    // Source Colorspace handle
    OfxParamHandle m_srcColorSpace;
    // Display handle
    OfxParamHandle m_display;
    // View handle
    OfxParamHandle m_view;
    // Config file handle
    OfxParamHandle m_configFile;
};

// Exception thrown when images are missing
class NoImageException{};

// Exception thrown with an status to return
class StatusException
{
public :
    explicit StatusException(OfxStatus stat)
        : status_(stat)
    {}

    OfxStatus status() const {return status_;}

protected :
    OfxStatus status_;
};

// Instanciating OCIO DisplayViewTransform API
OCIO::DisplayViewTransformRcPtr g_DisplayViewTransform = OCIO::DisplayViewTransform::Create();

// Getting the current instance configuration
extern OCIO::ConstConfigRcPtr g_Config;

// Setting up pointers for OpenFX plugin
extern OfxHost               *g_Host;
extern OfxImageEffectSuiteV1 *g_EffectHost;
extern OfxPropertySuiteV1    *g_PropHost;
extern OfxParameterSuiteV1   *g_ParamHost;
extern OfxMemorySuiteV1      *g_MemoryHost;
extern OfxMultiThreadSuiteV1 *g_ThreadHost;
extern OfxMessageSuiteV1     *g_MessageSuite;
extern OfxInteractSuiteV1    *g_InteractHost;

static OfxStatus OnLoad(void)
{
    return kOfxStatOK;
}

// Get DisplayViewContainer from a property set handle
static DisplayViewContainer* GetContainer(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle effectProps;
    g_EffectHost->getPropertySet(effect, &effectProps);


    DisplayViewContainer *Container = 0;
    g_PropHost->propGetPointer(effectProps, kOfxPropInstanceData, 0, (void**) &Container);

    return Container;
}

OfxStatus FetchSuites(OfxImageEffectHandle);

// Creating an instance of DisplayViewContainer and setting it up
static OfxStatus CreateInstance(OfxImageEffectHandle effect)
{
    // get a pointer to the effect properties
    OfxPropertySetHandle effectProps;
    g_EffectHost->getPropertySet(effect, &effectProps);

    // get a pointer to the effect's parameter set
    OfxParamSetHandle paramSet;
    g_EffectHost->getParamSet(effect, &paramSet);

    // Instanciating DisplayViewContainer
    DisplayViewContainer *Container = new DisplayViewContainer;

    // Cache away out clip handles
    g_EffectHost->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &Container->m_srcClip, 0);
    g_EffectHost->clipGetHandle(effect, kOfxImageEffectOutputClipName, &Container->m_dstClip, 0);

    // Caching all the parameters in Container
    g_ParamHost->paramGetHandle(paramSet, OCIOSourceColorSpace, &(Container->m_srcColorSpace), 0);
    g_ParamHost->paramGetHandle(paramSet, OCIOConfigParam, &(Container->m_configFile), 0);
    g_ParamHost->paramGetHandle(paramSet, OCIODisplayParam, &(Container->m_display), 0);
    g_ParamHost->paramGetHandle(paramSet, OCIOViewParam, &(Container->m_view), 0);

    g_PropHost->propSetPointer(effectProps, kOfxPropInstanceData, 0, reinterpret_cast<void *>(Container));

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

static void DefineDisplayParam(OfxParamSetHandle paramSet, const char* name, const char* hint)
{
    OfxPropertySetHandle props;
    OfxStatus stat;

    stat = g_ParamHost->paramDefine(paramSet, kOfxParamTypeChoice, name, &props);

    if(stat != kOfxStatOK)
    {
        throw stat;
        return;
    }

    for (int i = 0; i < g_Config->getNumDisplays(); i++)
        g_PropHost->propSetString(props, kOfxParamPropChoiceOption, i, g_Config->getActiveDisplays());
}

// A function to describe context specific properties and parameters
static OfxStatus DescribeInContext(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs)
{
    OfxPropertySetHandle props;
    // Defining the output clip for the plugin
    g_EffectHost->clipDefine(effect, kOfxImageEffectOutputClipName, &props);

    // Set the component types we can handle on the output
    g_PropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
    g_PropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 1, kOfxImageComponentAlpha);

    // Defining the source clip for the plugin
    g_EffectHost->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &props);

    // Set the component types we can handle on the input
    g_PropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
    g_PropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 1, kOfxImageComponentAlpha);

    // Fetching parameter set from the effect handle
    OfxParamSetHandle paramSet;
    g_EffectHost->getParamSet(effect, &paramSet);

    // Input Colorspace
    DefineColorSpaceParam(paramSet, OCIOSourceColorSpace, OCIOSourceColorSpace, OCIOSourceColorSpace, OCIOSourceColorSpaceHint, 0);
    // Config
    g_ParamHost->paramDefine(paramSet, kOfxParamTypeString, OCIOConfigParam, &props);
    // Display
    DefineDisplayParam(paramSet, OCIODisplayParam, OCIODisplayParamHint);

    // Making a page of controls
    g_ParamHost->paramDefine(paramSet, kOfxParamTypePage, "Main", &props);

    // Adding the parameters to "Main"
    g_PropHost->propSetString(props, kOfxParamPropPageChild, 0, OCIOSourceColorSpace);
    g_PropHost->propSetString(props, kOfxParamPropPageChild, 1, OCIOConfigParam);
    g_PropHost->propSetString(props, kOfxParamPropPageChild, 2, OCIODisplayParam);
    g_PropHost->propSetString(props, kOfxParamPropPageChild, 3, OCIOViewParam);

    return kOfxStatOK;
}

// A function to describe the plugin and define parameters common to all contexts in the Image Effect
static OfxStatus Describe(OfxImageEffectHandle effect)
{
    // Fetching host suites
    OfxStatus stat;
    if((stat = FetchSuites(effect)) != kOfxStatOK)
        return stat;

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
    g_PropHost->propSetString(effectProps, kOfxPropLabel, 0, OCIOPluginName);
    g_PropHost->propSetString(effectProps, kOfxImageEffectPluginPropGrouping, 0, OCIOPluginGroup);
    g_PropHost->propSetString(effectProps, kOfxPropPluginDescription, 0, OCIOPluginDescription);

    return kOfxStatOK;
}

// TODO: Move Utilities functions/exceptions to a separate file

// Utility Functions to get Image
inline int GetImageRowBytes(OfxPropertySetHandle handle)
{
    int r;
    g_PropHost->propGetInt(handle, kOfxImagePropRowBytes, 0, &r);
    return r;
}

// Utility Functions to get Image
inline int GetImagePixelDepth(OfxPropertySetHandle handle, bool isUnMapped = false)
{
    char *bitString;
    if(isUnMapped)
        g_PropHost->propGetString(handle, kOfxImageClipPropUnmappedPixelDepth, 0, &bitString);
    else
        g_PropHost->propGetString(handle, kOfxImageEffectPropPixelDepth, 0, &bitString);

    if(strcmp(bitString, kOfxBitDepthByte) == 0)
        return 8;

    else if(strcmp(bitString, kOfxBitDepthShort) == 0)
        return 16;

    else if(strcmp(bitString, kOfxBitDepthFloat) == 0)
        return 32;

    return 0;
}

// Utility Functions to get Image
inline bool GetImagePixelsAreRGBA(OfxPropertySetHandle handle, bool unmapped = false)
{
    char *v;
    if(unmapped)
        g_PropHost->propGetString(handle, kOfxImageClipPropUnmappedComponents, 0, &v);
    else
        g_PropHost->propGetString(handle, kOfxImageEffectPropComponents, 0, &v);
    return strcmp(v, kOfxImageComponentAlpha) != 0;
}

// Utility Functions to get Image
inline OfxRectI GetImageBounds(OfxPropertySetHandle handle)
{
    OfxRectI r;
    g_PropHost->propGetIntN(handle, kOfxImagePropBounds, 4, &r.x1);
    return r;
}

// Utility Functions to get Image
inline void* GetImageData(OfxPropertySetHandle handle)
{
    void *r;
    g_PropHost->propGetPointer(handle, kOfxImagePropData, 0, &r);
    return r;
}

// Utility Functions to get Image
inline OfxPropertySetHandle GetImage(OfxImageClipHandle &clip, OfxTime time, int& rowBytes, int& bitDepth, bool& isAlpha, OfxRectI &rect, void* &data)
{
    OfxPropertySetHandle imageProps = NULL;
    if (g_EffectHost->clipGetImage(clip, time, NULL, &imageProps) == kOfxStatOK)
    {
        rowBytes  =  GetImageRowBytes(imageProps);
        bitDepth  =  GetImagePixelDepth(imageProps);
        isAlpha   = !GetImagePixelsAreRGBA(imageProps);
        rect      =  GetImageBounds(imageProps);
        data      =  GetImageData(imageProps);
        if(data == NULL) 
        {
            g_EffectHost->clipReleaseImage(imageProps);
            imageProps = NULL;
        }
    }
    else
    {
        rowBytes  = 0;
        bitDepth  = 0;
        isAlpha   = false;
        rect.x1 = rect.x2 = rect.y1 = rect.y2 = 0;
        data = NULL;
    }
    return imageProps;
}

// Renders image on demand
static OfxStatus Render(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs)
{
    // Get the render window and the time from the inArgs
    OfxTime time;
    OfxRectI renderWindow;
    OfxStatus status = kOfxStatOK;

    g_PropHost->propGetDouble(inArgs, kOfxPropTime, 0, &time);
    g_PropHost->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, &renderWindow.x1);

    DisplayViewContainer *Container = GetContainer(effect);

    OfxPropertySetHandle srcImg = nullptr, dstImg = nullptr;
    int srcRowBytes, srcBitDepth, dstRowBytes, dstBitDepth;
    bool srcIsAlpha, dstIsAplha;
    OfxRectI dstRect, srcRect;
    void *src, *dst;

    try
    {
        srcImg = GetImage(Container->m_srcClip, time, srcRowBytes, srcBitDepth, srcIsAlpha, srcRect, src);
        if(srcImg == NULL)
            throw NoImageException();
        dstImg = GetImage(Container->m_dstClip, time, dstRowBytes, dstBitDepth, dstIsAplha, dstRect, dst);
        if(dstImg == NULL)
            throw NoImageException();

        if (srcBitDepth != dstBitDepth || srcIsAlpha != dstIsAplha)
            throw StatusException(kOfxStatErrImageFormat);

        // Gettin source and destination colorspaces from suite
        char *srcCS;
        char *dstCS;
        g_ParamHost->paramGetValueAtTime(Container->m_srcColorSpace, time, &srcCS);

        // Setting up OCIO::DisplayViewTransform API
        g_DisplayViewTransform->setSrc(srcCS);

        // Get Processor call
        OCIO::ConstProcessorRcPtr processor = g_Config->getProcessor(g_DisplayViewTransform);

        OCIO::ConstCPUProcessorRcPtr cpu = processor->getDefaultCPUProcessor();

        OCIO::PackedImageDesc img(reinterpret_cast<void *>(dst), static_cast<long>(dstRect.x2 - dstRect.x1), static_cast<long>(dstRect.y2 - dstRect.y1), static_cast<long>(4));

        // Applying the transfor to img
        cpu->apply(img);

    }
    catch(NoImageException &ex)
    {
        if(!g_EffectHost->abort(effect))
            status = kOfxStatFailed;
    }
    catch(StatusException &ex)
    {
        status = ex.status();
    }
    catch(OCIO::Exception & exception)
    {
        std::cerr << "OpenColorIO Error: " << exception.what() << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: OpenColorIO DisplayViewTransform Plugin    " << e.what() << '\n';
    }

    if(srcImg)
        g_EffectHost->clipReleaseImage(srcImg);
    if(dstImg)
        g_EffectHost->clipReleaseImage(dstImg);

    return status;
}

static OfxStatus DestroyInstance(OfxImageEffectHandle effect)
{
    DisplayViewContainer *Container = GetContainer(effect);

    if(Container)
        delete Container;
    Container = nullptr;

    return kOfxStatOK;
}

static OfxStatus UnLoad()
{
    return kOfxStatOK;
}

// -----------------------------------------------------------------------------------------------
// ---------------------------------- Plugin's Main Entry point ----------------------------------
// -----------------------------------------------------------------------------------------------
static OfxStatus EntryPointDV(const char* action, const void* handle, OfxPropertySetHandle inArgs,  OfxPropertySetHandle outArgs)
{
    try
    {
        OfxImageEffectHandle effect = (OfxImageEffectHandle) handle;
        if(strcmp(action, kOfxActionLoad) == 0)
        {
            return OnLoad();
        }
        if(strcmp(action, kOfxActionCreateInstance) == 0)
        {
            return CreateInstance(effect);
        }
        if(strcmp(action, kOfxActionDescribe)==0)
        {
            return Describe(effect);
        }
        if(strcmp(action, kOfxImageEffectActionDescribeInContext) == 0)
        {
            return DescribeInContext(effect, inArgs);
        }
        if(strcmp(action, kOfxImageEffectActionRender) == 0)
        {
            return Render(effect, inArgs, outArgs);
        }
        if(strcmp(action, kOfxActionDestroyInstance) == 0)
        {
            return DestroyInstance(effect);
        }
        if(strcmp(action, kOfxActionUnload) == 0)
        {
            return UnLoad();
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
        std::cerr << "Unknown Error: OpenColorIO DisplayViewTransform Plugin" << std::endl;
        return kOfxStatErrUnknown;
    }
    return kOfxStatReplyDefault;
}

// ----------------------------------------------------------------------------
// ------------------------ Mandatory OpenFX Functions ------------------------
// ----------------------------------------------------------------------------

// Function for setting the Host
void SetHost(OfxHost*);

// Creating the plugin struct
OfxPlugin DisplayViewTransformPlugin = 
{
    kOfxImageEffectPluginApi,
    1,
    OCIOPluginIdentifier,
    1,
    0,
    SetHost,
    EntryPointDV
};
