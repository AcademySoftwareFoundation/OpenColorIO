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

namespace OCIO = OCIO_NAMESPACE;

#if defined __APPLE__ || defined linux || defined __FreeBSD__
#  define EXPORT __attribute__((visibility("default")))
#elif defined _WIN32
#  define EXPORT OfxExport
#else
#  error Not building on your operating system quite yet
#endif

extern OfxPlugin ColorSpaceTransformPlugin;
extern OfxPlugin DisplayViewTransformPlugin;
extern OfxPlugin FileTransformPlugin;

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

OfxStatus FetchSuites(OfxImageEffectHandle effect)
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

// Function for setting the Host
void SetHost(OfxHost* host)
{
    g_Host = host;
}

// The two mandated functions
EXPORT OfxPlugin* OfxGetPlugin(int nth)
{
    if(nth == 0)
        return &ColorSpaceTransformPlugin;
    if(nth == 1)
        return &FileTransformPlugin;
    if(nth == 2)
        return &DisplayViewTransformPlugin;
    return 0;
}

EXPORT int OfxGetNumberOfPlugins(void)
{
    return 3;
}
