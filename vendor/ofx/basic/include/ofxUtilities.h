#ifndef __ofxUtilities_H_
#define __ofxUtilities_H_

#include "ofxMessage.h"
#include "ofxPixels.h"

////////////////////////////////////////////////////////////////////////////////
// This is a set of utility functions that got placed here as I got tired of
// typing the same thing.
// If anyone uses this for real functionality in real plugins, you are nuts.

extern OfxHost               *gHost;
extern OfxImageEffectSuiteV1 *gEffectHost;
extern OfxPropertySuiteV1    *gPropHost;
extern OfxInteractSuiteV1    *gInteractHost;
extern OfxParameterSuiteV1   *gParamHost;
extern OfxMemorySuiteV1      *gMemoryHost;
extern OfxMultiThreadSuiteV1 *gThreadHost;
extern OfxMessageSuiteV1 *gMessageSuite;

/* fetch our host APIs from the host struct given us
   the plugin's set host function must have been already called
 */
inline OfxStatus
ofxuFetchHostSuites(void)
{
  if(!gHost)
    return kOfxStatErrMissingHostFeature;
    
  gEffectHost   = (OfxImageEffectSuiteV1 *) gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
  gPropHost     = (OfxPropertySuiteV1 *)    gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
  gParamHost    = (OfxParameterSuiteV1 *)   gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);
  gMemoryHost   = (OfxMemorySuiteV1 *)      gHost->fetchSuite(gHost->host, kOfxMemorySuite, 1);
  gThreadHost   = (OfxMultiThreadSuiteV1 *) gHost->fetchSuite(gHost->host, kOfxMultiThreadSuite, 1);
  gMessageSuite   = (OfxMessageSuiteV1 *)   gHost->fetchSuite(gHost->host, kOfxMessageSuite, 1);
  gInteractHost   = (OfxInteractSuiteV1 *)   gHost->fetchSuite(gHost->host, kOfxInteractSuite, 1);
  if(!gEffectHost || !gPropHost || !gParamHost || !gMemoryHost || !gThreadHost)
    return kOfxStatErrMissingHostFeature;
  return kOfxStatOK;
}

// is the clip or image unpremultiplied or not
inline bool
ofxuIsUnPremultiplied(OfxPropertySetHandle clipOrImageProps)
{
  char *premult = NULL;
  gPropHost->propGetString(clipOrImageProps, kOfxImageEffectPropPreMultiplication, 0, &premult);
  return strcmp(premult, kOfxImageUnPreMultiplied) == 0;
}

// is the clip unpremultiplied or not
inline bool
ofxuIsUnPremultiplied(OfxImageClipHandle clipHandle)
{
  OfxPropertySetHandle props;
  gEffectHost->clipGetPropertySet(clipHandle, &props);
  return ofxuIsUnPremultiplied(props);
}


// Convinience wrapper to check for connection of a clip
inline bool
ofxuIsClipConnected(OfxImageEffectHandle pluginInstance, const char *clipName)
{
  int connected = 0;
  OfxImageClipHandle clipHandle;
  OfxPropertySetHandle props;
  gEffectHost->clipGetHandle(pluginInstance, clipName, &clipHandle, &props);

  gPropHost->propGetInt(props,  kOfxImageClipPropConnected, 0, &connected);
  return connected != 0;
}


// fetches the size and offset for the project
inline void
ofxuGetProjectSetup(OfxImageEffectHandle pluginInstance, OfxPointD &projSize, OfxPointD &projOffset)
{
  OfxPropertySetHandle props;
  gEffectHost->getPropertySet(pluginInstance, &props);
  gPropHost->propGetDoubleN(props, kOfxImageEffectPropProjectSize, 2, &projSize.x); 
  gPropHost->propGetDoubleN(props, kOfxImageEffectPropProjectOffset, 2, &projOffset.x); 
}

// get the pixel scale from an interact instance
inline void
ofxuGetInteractPixelScale(OfxPropertySetHandle interactArgs, double pixelScale[2])
{
  gPropHost->propGetDoubleN(interactArgs, kOfxInteractPropPixelScale, 2, pixelScale);  
}

// gets the time from a property set
inline double
ofxuGetTime(OfxPropertySetHandle propSet)
{
  double time = 0;
  gPropHost->propGetDouble(propSet, kOfxPropTime, 0, &time); 
  return time;
}

// get the time from an instance
inline double
ofxuGetTime(OfxImageEffectHandle pluginInstance)
{
  OfxPropertySetHandle props;
  gEffectHost->getPropertySet(pluginInstance, &props);
  return ofxuGetTime(props);
}

// routines to extract image properties
inline void *
ofxuGetImageData(OfxPropertySetHandle imageHandle)
{
  void *r;
  gPropHost->propGetPointer(imageHandle, kOfxImagePropData, 0, &r);
  return r;
}

inline OfxRectI
ofxuGetImageBounds(OfxPropertySetHandle imageHandle)
{
  OfxRectI r;
  gPropHost->propGetIntN(imageHandle, kOfxImagePropBounds, 4, &r.x1);
  return r;
}

inline int
ofxuGetImageRowBytes(OfxPropertySetHandle imageHandle)
{
  int r;
  gPropHost->propGetInt(imageHandle, kOfxImagePropRowBytes, 0, &r);
  return r;
}

// turn a bit depth string descriptor into a number of bits
inline int
ofxuMapPixelDepth(char *bitString)
{
  if(strcmp(bitString, kOfxBitDepthByte) == 0) {
    return 8;
  }
  else if(strcmp(bitString, kOfxBitDepthShort) == 0) {
    return 16;
  }
  else if(strcmp(bitString, kOfxBitDepthFloat) == 0) {
    return 32;
  }
  return 0;  
}

inline int
ofxuGetImagePixelDepth(OfxPropertySetHandle imageHandle, bool isUnMapped = false)
{
  char *bitString;
  if(isUnMapped)
    gPropHost->propGetString(imageHandle, kOfxImageClipPropUnmappedPixelDepth, 0, &bitString); // get unmapped component types of a clip
  else
    gPropHost->propGetString(imageHandle, kOfxImageEffectPropPixelDepth, 0, &bitString);
  return ofxuMapPixelDepth(bitString);
}

inline bool
ofxuGetImagePixelsAreRGBA(OfxPropertySetHandle imageHandle, bool unmapped = false)
{
  char *v;
  if(unmapped)
    gPropHost->propGetString(imageHandle, kOfxImageClipPropUnmappedComponents, 0, &v); // get unmapped pixel depths
  else
    gPropHost->propGetString(imageHandle, kOfxImageEffectPropComponents, 0, &v);
  return strcmp(v, kOfxImageComponentAlpha) != 0;
}

inline int
ofxuGetClipPixelDepth(OfxImageClipHandle clipHandle, bool unmapped = false)
{
  OfxPropertySetHandle props;
  gEffectHost->clipGetPropertySet(clipHandle, &props);
  return ofxuGetImagePixelDepth(props, unmapped); // same property
}

inline bool
ofxuGetClipPixelsAreRGBA(OfxImageClipHandle clipHandle, bool unmapped = false)
{
  OfxPropertySetHandle props;
  gEffectHost->clipGetPropertySet(clipHandle, &props);
  return ofxuGetImagePixelsAreRGBA(props, unmapped); // same property
}

inline void
ofxuClipGetFormat(OfxImageClipHandle clipHandle, int &bitDepth, bool &isRGBA, bool unmapped = false)
{
  bitDepth = ofxuGetClipPixelDepth(clipHandle, unmapped);
  isRGBA = ofxuGetClipPixelsAreRGBA(clipHandle, unmapped);
}

// set the data pointer on an interact instance
inline void
ofxuSetInteractInstanceData(OfxInteractHandle interactInstance, void *data)
{
  OfxPropertySetHandle props;
  gInteractHost->interactGetPropertySet(interactInstance, &props);
  gPropHost->propSetPointer(props, kOfxPropInstanceData, 0, data); 
}

// get the data pointer from an interact instance
inline void *
ofxuGetInteractInstanceData(OfxInteractHandle interactInstance)
{
  OfxPropertySetHandle props;
  gInteractHost->interactGetPropertySet(interactInstance, &props);
  void *data;
  gPropHost->propGetPointer(props, kOfxPropInstanceData, 0, &data);   
  return data;
}

// set the data pointer on an interact instance
inline void
ofxuSetEffectInstanceData(OfxImageEffectHandle effect, void *data)
{
  OfxPropertySetHandle props;
  gEffectHost->getPropertySet(effect, &props);
  gPropHost->propSetPointer(props, kOfxPropInstanceData, 0, data); 
}

// get the data pointer from an interact instance
inline void *
ofxuGetEffectInstanceData(OfxImageEffectHandle effect)
{
  OfxPropertySetHandle props;
  gEffectHost->getPropertySet(effect, &props);
  void *data;
  gPropHost->propGetPointer(props, kOfxPropInstanceData, 0, &data);   
  return data;
}

// is a rect infinite in X
template <class RECT> bool
ofxuInfiniteRectInX(const RECT &rect)
{
  return rect.x1 == kOfxFlagInfiniteMin && rect.x2 == kOfxFlagInfiniteMax;
}

// is a rect infinite in Y
template <class RECT> bool
ofxuInfiniteRectInY(const RECT &rect)
{
  return rect.y1 == kOfxFlagInfiniteMin && rect.y2 == kOfxFlagInfiniteMax;
}

// is a rect infinite in both dimensions
template <class RECT> bool
ofxuInfiniteRect(const RECT &rect)
{
  return ofxuInfiniteRectInX(rect) && ofxuInfiniteRectInY(rect);
}


// fetch an image an associated bits from a clip
//
// In real code, all the params should be bundled up into a class, as
// does the OFX C++ support code
inline OfxPropertySetHandle ofxuGetImage(OfxImageClipHandle &clip,
                                         OfxTime time,
                                         int &rowBytes,
                                         int &bitDepth,
                                         bool &isAlpha,
                                         OfxRectI &rect,
                                         void * &data)
{
  OfxPropertySetHandle imageProps = NULL;
  if(gEffectHost->clipGetImage(clip, time, NULL, &imageProps) == kOfxStatOK) {
    rowBytes  =  ofxuGetImageRowBytes(imageProps);
    bitDepth  =  ofxuGetImagePixelDepth(imageProps);
    isAlpha   = !ofxuGetImagePixelsAreRGBA(imageProps);
    rect      =  ofxuGetImageBounds(imageProps);
    data      =  ofxuGetImageData(imageProps);
    if(data == NULL) {
      gEffectHost->clipReleaseImage(imageProps);
      imageProps = NULL;
    }
  } else {
    rowBytes  = 0;
    bitDepth  = 0;
    isAlpha   = false;
    rect.x1 = rect.x2 = rect.y1 = rect.y2 = 0;
    data      =  NULL;
  }
  return imageProps;
}

/// exception thrown when images are missing
class OfxuNoImageException{};                     

/// exception thrown with an status to return
class OfxuStatusException {
public :
  explicit OfxuStatusException(OfxStatus stat)
    : status_(stat)
  {}

  OfxStatus status() const {return status_;}

protected :
  OfxStatus status_;
};

#endif