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

// Meta data about the plugin
#define OCIOPluginIdentifier "org.OpenColorIO.FileTransformPlugin"
#define OCIOPluginName "File Transform"
#define OCIOPluginGroup "Color/OpenColorIO"
#define OCIOPluginDescription "A plugin for file transform through OCIO"
#define OCIOToggleR "ToggleR"
#define OCIOToggleRHint "Toggle red component"
#define OCIOToggleG "ToggleG"
#define OCIOToggleGHint "Toggle green component"
#define OCIOToggleB "ToggleB"
#define OCIOToggleBHint "Toggle blue component"
#define OCIOFileParam "file"
#define OCIODirectionParam "direction"
#define OCIODirectionParamHint "Define the direction for file transform"
#define OCIOInterpolationParam "interpolation"
#define OCIOInterpolationParamHint "Define the interpolation for file transform"

#if defined __APPLE__ || defined linux || defined __FreeBSD__
#  define EXPORT __attribute__((visibility("default")))
#elif defined _WIN32
#  define EXPORT OfxExport
#else
#  error Not building on your operating system quite yet
#endif

namespace OCIO = OCIO_NAMESPACE;

// This is a custom class made for storing plugin specific data
class FileContainer
{
public:
    // The source clip
    OfxImageClipHandle m_srcClip;
    // The destination clip
    OfxImageClipHandle m_dstClip;

    // Parameter for toggling red component trigger
    OfxParamHandle m_toggleR;
    // Parameter for toggling green component trigger
    OfxParamHandle m_toggleG;
    // Parameter for toggling blue component trigger
    OfxParamHandle m_toggleB;
    // Parameter for recieving LUT file
    OfxParamHandle m_file;
    // Parameter for setting transformation direction
    OfxParamHandle m_direction;
    // Parameter to select interpolation options
    OfxParamHandle m_interpolation;
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

// Instanciating OCIO FileTransform API
OCIO::FileTransformRcPtr g_FileTransform = OCIO::FileTransform::Create();

// Getting the current instance configuration
OCIO::ConstConfigRcPtr g_Config = OCIO::GetCurrentConfig();

// Setting up pointers for the plugin's host and its suites
OfxHost               *g_Host;
OfxImageEffectSuiteV1 *g_EffectHost   = 0;
OfxPropertySuiteV1    *g_PropHost     = 0;
OfxParameterSuiteV1   *g_ParamHost    = 0;
OfxMemorySuiteV1      *g_MemoryHost   = 0;
OfxMultiThreadSuiteV1 *g_ThreadHost   = 0;
OfxMessageSuiteV1     *g_MessageSuite = 0;
OfxInteractSuiteV1    *g_InteractHost = 0;

// Function
OfxStatus OnLoad(void)
{
    return kOfxStatOK;
}

// Get FileContainer from a property set handle
static FileContainer* GetContainer(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle effectProps;
    g_EffectHost->getPropertySet(effect, &effectProps);


    FileContainer *Container = 0;
    g_PropHost->propGetPointer(effectProps, kOfxPropInstanceData, 0, (void**) &Container);

    return Container;
}

static OfxStatus FetchSuites(OfxImageEffectHandle effect)
{
    g_EffectHost   = reinterpret_cast<OfxImageEffectSuiteV1 *>(const_cast<void *>(g_Host->fetchSuite(g_Host->host, kOfxImageEffectSuite, 1)));
    g_PropHost     = reinterpret_cast<OfxPropertySuiteV1 *>   (const_cast<void *>(g_Host->fetchSuite(g_Host->host, kOfxPropertySuite, 1)));
    g_ParamHost    = reinterpret_cast<OfxParameterSuiteV1 *>  (const_cast<void *>(g_Host->fetchSuite(g_Host->host, kOfxParameterSuite, 1)));
    g_MemoryHost   = reinterpret_cast<OfxMemorySuiteV1 *>     (const_cast<void *>(g_Host->fetchSuite(g_Host->host, kOfxMemorySuite, 1)));
    g_ThreadHost   = reinterpret_cast<OfxMultiThreadSuiteV1 *>(const_cast<void *>(g_Host->fetchSuite(g_Host->host, kOfxMultiThreadSuite, 1)));
    g_MessageSuite = reinterpret_cast<OfxMessageSuiteV1 *>    (const_cast<void *>(g_Host->fetchSuite(g_Host->host, kOfxMessageSuite, 1)));
    g_InteractHost = reinterpret_cast<OfxInteractSuiteV1 *>   (const_cast<void *>(g_Host->fetchSuite(g_Host->host, kOfxInteractSuite, 1)));

    return kOfxStatOK;
}

static OfxStatus CreateInstance(OfxImageEffectHandle effect)
{
    // get a pointer to the effect properties
    OfxPropertySetHandle effectProps;
    g_EffectHost->getPropertySet(effect, &effectProps);

    // get a pointer to the effect's parameter set
    OfxParamSetHandle paramSet;
    g_EffectHost->getParamSet(effect, &paramSet);

    // Instanciating FileContainer
    FileContainer *Container = new FileContainer;

    // Cache away out clip handles
    g_EffectHost->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &Container->m_srcClip, 0);
    g_EffectHost->clipGetHandle(effect, kOfxImageEffectOutputClipName, &Container->m_dstClip, 0);

    // Caching all the parameters in Container
    g_ParamHost->paramGetHandle(paramSet, "file", &(Container->m_file), 0);
    g_ParamHost->paramGetHandle(paramSet, "interpolation", &(Container->m_interpolation), 0);
    g_ParamHost->paramGetHandle(paramSet, "direction", &(Container->m_direction), 0);
    g_ParamHost->paramGetHandle(paramSet, "ToggleR", &(Container->m_toggleR), 0);
    g_ParamHost->paramGetHandle(paramSet, "ToggleG", &(Container->m_toggleG), 0);
    g_ParamHost->paramGetHandle(paramSet, "ToggleB", &(Container->m_toggleB), 0);
    

    g_PropHost->propSetPointer(effectProps, kOfxPropInstanceData, 0, (void *) Container);

    return kOfxStatOK;
}

// A function to describe context specific properties and parameters
static OfxStatus DescribeInContext(OfxImageEffectHandle effect)
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

    // Defining parameters for the plugin
    // Fetching parameter set from the effect handle
    OfxParamSetHandle paramSet;
    g_EffectHost->getParamSet(effect, &paramSet);

    // TODO: Revise the structure of params and labels
    g_ParamHost->paramDefine(paramSet, kOfxParamTypeString, OCIOFileParam, &props);
    g_ParamHost->paramDefine(paramSet, kOfxParamTypeChoice, OCIOInterpolationParam, &props);
    g_ParamHost->paramDefine(paramSet, kOfxParamTypeChoice, OCIODirectionParam, &props);
    g_ParamHost->paramDefine(paramSet, kOfxParamTypeBoolean, OCIOToggleR, &props);
    g_ParamHost->paramDefine(paramSet, kOfxParamTypeBoolean, OCIOToggleG, &props);
    g_ParamHost->paramDefine(paramSet, kOfxParamTypeBoolean, OCIOToggleB, &props);


    // Making a page of controls and add the parameters to it
    g_ParamHost->paramDefine(paramSet, kOfxParamTypePage, "Main", &props);

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
    g_PropHost->propSetInt(effectProps, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 0);

    // Set the bit depths the plugin can handle
    g_PropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthByte);
    g_PropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthShort);
    g_PropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 2, kOfxBitDepthFloat);

    // Set some labels and the group it belongs to
    g_PropHost->propSetString(effectProps, kOfxPropLabel, 0, OCIOPluginName);
    g_PropHost->propSetString(effectProps, kOfxImageEffectPluginPropGrouping, 0, OCIOPluginGroup);

    // Define the contexts we can be used in
    g_PropHost->propSetString(effectProps, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);

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

// Utility function to get interpolation
inline OCIO::Interpolation GetInterpolation(const char* itp)
{
    if(strcmp(itp, "Linear") == 0)
            return OCIO::Interpolation::INTERP_LINEAR;
    else if(strcmp(itp, "Nearest") == 0)
        return OCIO::Interpolation::INTERP_NEAREST;
    else if(strcmp(itp, "Best") == 0)
        return OCIO::Interpolation::INTERP_BEST;
    else if(strcmp(itp, "Tetrahedral") == 0)
        return OCIO::Interpolation::INTERP_TETRAHEDRAL;
    else
        return OCIO::Interpolation::INTERP_UNKNOWN;
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

    FileContainer *Container = GetContainer(effect);

    OfxPropertySetHandle srcImg = NULL, dstImg = NULL;
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

        // Getting interpolation, direction and file from ParamSuite
        char *itp;
        char *dir;
        char *srcFile;
        g_ParamHost->paramGetValueAtTime(Container->m_interpolation, time, &itp);
        g_ParamHost->paramGetValueAtTime(Container->m_direction, time, &dir);
        g_ParamHost->paramGetValueAtTime(Container->m_file, time, &srcFile);

        // Setting up OCIO::FileTransform API
        OCIO::TransformDirection d;
        if(strcmp(dir,"Inverse") == 0)
            d = OCIO::TransformDirection::TRANSFORM_DIR_INVERSE;
        else
            d = OCIO::TransformDirection::TRANSFORM_DIR_FORWARD;

        OCIO::Interpolation iterpolation = GetInterpolation(itp);
        g_FileTransform->setDirection(d);
        g_FileTransform->setInterpolation(iterpolation);

        // Get Processor call
        OCIO::ConstProcessorRcPtr processor = g_Config->getProcessor(g_FileTransform);

        OCIO::ConstCPUProcessorRcPtr cpu = processor->getDefaultCPUProcessor();

        OCIO::PackedImageDesc img(static_cast<void *>(src), static_cast<long>(srcRect.x2 - srcRect.x1), static_cast<long>(srcRect.y2 - srcRect.y1), 4);

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
        std::cerr << "Error: OpenColorIO FileTransform Plugin    " << e.what() << '\n';
    }

    if(srcImg)
        g_EffectHost->clipReleaseImage(srcImg);
    if(dstImg)
        g_EffectHost->clipReleaseImage(dstImg);

    return status;
}

static OfxStatus DestroyInstance(OfxImageEffectHandle effect)
{
    FileContainer *Container = GetContainer(effect);

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
static OfxStatus EntryPoint(const char* action, const void* handle, OfxPropertySetHandle inArgs,  OfxPropertySetHandle outArgs)
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
            return DescribeInContext(effect);
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
        std::cerr << "Error: OpenColorIO FileTransform Plugin    bad_alloc" << std::endl;
        return kOfxStatErrMemory;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: OpenColorIO FileTransform Plugin    " << e.what();
        return kOfxStatErrUnknown;
    }
    catch(...)
    {
        std::cerr << "Unknown Error: OpenColorIO FileTransform Plugin" << std::endl;
        return kOfxStatErrUnknown;
    }
    return kOfxStatReplyDefault;
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
static OfxPlugin FileTransformPlugin = 
{
    kOfxImageEffectPluginApi,
    1,
    OCIOPluginIdentifier,
    1,
    0,
    SetHost,
    EntryPoint
};

// The two mandated functions
EXPORT OfxPlugin* OfxGetPlugin(int nth)
{
    if(nth == 0)
        return &FileTransformPlugin;
    return 0;
}

EXPORT int OfxGetNumberOfPlugins(void)
{
    return 1;
}

