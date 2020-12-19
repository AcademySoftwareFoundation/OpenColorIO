/*
Software License :

Copyright (c) 2003, The Open Effects Association Ltd. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name The Open Effects Association Ltd, nor the names of its 
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/*
  Ofx Example plugin that show a basic plugin which scales all components in an image
  by a value.

  It is meant to illustrate certain features of the API, as opposed to being a perfectly
  crafted piece of image processing software.

  The main features are
    - implementation of all plugin functions
    - basic property usage
    - basic plugin definition   
       - parameters
       - parameter hierarchy
    - context dependant plugin definition
    - instance creation and private instance data
    - multiple input clips
    - region of interest and region of definition
    - clip preferences
    - multi threaded rendering
    - call back functions for user edited events on parameters
 */
#include <stdexcept>
#include <new>
#include <cstring>
#include "ofxImageEffect.h"
#include "ofxMemory.h"
#include "ofxMultiThread.h"
#include <OpenColorIO/OpenColorIO.h>

#include "include/ofxUtilities.H" // example support utils

// #if defined __APPLE__ || defined linux || defined __FreeBSD__
// #  define EXPORT __attribute__((visibility("default")))
// #elif defined _WIN64 
// #  define EXPORT OfxExport
// #else
// #  error Not building on your operating system quite yet
// #endif

template <class T> inline T Maximum(T a, T b) {return a > b ? a : b;}
template <class T> inline T Minimum(T a, T b) {return a < b ? a : b;}

// pointers64 to various bits of the host
OfxHost                 *gHost;
OfxImageEffectSuiteV1 *gEffectHost = 0;
OfxPropertySuiteV1    *gPropHost = 0;
OfxParameterSuiteV1   *gParamHost = 0;
OfxMemorySuiteV1      *gMemoryHost = 0;
OfxMultiThreadSuiteV1 *gThreadHost = 0;
OfxMessageSuiteV1     *gMessageSuite = 0;
OfxInteractSuiteV1    *gInteractHost = 0;

// some flags about the host's behaviour
int gHostSupportsMultipleBitDepths = false;

// private instance data type
struct MyInstanceData {
  bool isGeneralEffect;

  // handles to the clips we deal with
  OfxImageClipHandle sourceClip;
  OfxImageClipHandle maskClip;
  OfxImageClipHandle outputClip;

  // handles to a our parameters
  OfxParamHandle scaleParam;
  OfxParamHandle perComponentScaleParam;
  OfxParamHandle scaleRParam;
  OfxParamHandle scaleGParam;
  OfxParamHandle scaleBParam;
  OfxParamHandle scaleAParam;
};

/* mandatory function to set up the host structures */


// Convinience wrapper to get private data 
static MyInstanceData *
getMyInstanceData( OfxImageEffectHandle effect)
{
  // get the property handle for the plugin
  OfxPropertySetHandle effectProps;
  gEffectHost->getPropertySet(effect, &effectProps);

  // get my data pointer out of that
  MyInstanceData *myData = 0;
  gPropHost->propGetPointer(effectProps,  kOfxPropInstanceData, 0, 
			    (void **) &myData);
  return myData;
}

// Convinience wrapper to set the enabledness of a parameter
static inline void
setParamEnabledness( OfxImageEffectHandle effect,
                    const char *paramName,
                    int enabledState)
{
  // fetch the parameter set for this effect
  OfxParamSetHandle paramSet;
  gEffectHost->getParamSet(effect, &paramSet);
  
  // fetch the parameter property handle
  OfxParamHandle param; OfxPropertySetHandle paramProps;
  gParamHost->paramGetHandle(paramSet, paramName, &param, &paramProps);

  // and set its enabledness
  gPropHost->propSetInt(paramProps,  kOfxParamPropEnabled, 0, enabledState);
}

// function thats sets the enabledness of the percomponent scale parameters
// depending on the value of the 
// This function is called when the 'scaleComponents' value is changed
// or when the input clip has been changed
static void
setPerComponentScaleEnabledness( OfxImageEffectHandle effect)
{
  // get my instance data
  MyInstanceData *myData = getMyInstanceData(effect);

  // get the value of the percomponent scale param
  int perComponentScale;
  gParamHost->paramGetValue(myData->perComponentScaleParam, &perComponentScale);

  if(ofxuIsClipConnected(effect, kOfxImageEffectSimpleSourceClipName)) {
    OfxPropertySetHandle props; gEffectHost->clipGetPropertySet(myData->sourceClip, &props);

    // get the input clip format
    char *pixelType;
    gPropHost->propGetString(props, kOfxImageEffectPropComponents, 0, &pixelType);

    // only enable the scales if the input is an RGBA input
    perComponentScale = perComponentScale && !(strcmp(pixelType, kOfxImageComponentAlpha) == 0);
  }

  // set the enabled/disabled state of the parameter
  setParamEnabledness(effect, "scaleR", perComponentScale);
  setParamEnabledness(effect, "scaleG", perComponentScale);
  setParamEnabledness(effect, "scaleB", perComponentScale);
  setParamEnabledness(effect, "scaleA", perComponentScale);
}

/** @brief Called at load */
static OfxStatus
onLoad(void)
{
  return kOfxStatOK;
}

/** @brief Called before unload */
static OfxStatus
onUnLoad(void)
{
  return kOfxStatOK;
}

//  instance construction
static OfxStatus
createInstance( OfxImageEffectHandle effect)
{
  // get a pointer to the effect properties
  OfxPropertySetHandle effectProps;
  gEffectHost->getPropertySet(effect, &effectProps);

  // get a pointer to the effect's parameter set
  OfxParamSetHandle paramSet;
  gEffectHost->getParamSet(effect, &paramSet);

  // make my private instance data
  MyInstanceData *myData = new MyInstanceData;
  char *context = 0;

  // is this instance a general effect ?
  gPropHost->propGetString(effectProps, kOfxImageEffectPropContext, 0,  &context);
  myData->isGeneralEffect = context && (strcmp(context, kOfxImageEffectContextGeneral) == 0);

  // cache away out param handles
  gParamHost->paramGetHandle(paramSet, "scaleComponents", &myData->perComponentScaleParam, 0);
  gParamHost->paramGetHandle(paramSet, "scale", &myData->scaleParam, 0);
  gParamHost->paramGetHandle(paramSet, "scaleR", &myData->scaleRParam, 0);
  gParamHost->paramGetHandle(paramSet, "scaleG", &myData->scaleGParam, 0);
  gParamHost->paramGetHandle(paramSet, "scaleB", &myData->scaleBParam, 0);
  gParamHost->paramGetHandle(paramSet, "scaleA", &myData->scaleAParam, 0);

  // cache away out clip handles
  gEffectHost->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &myData->sourceClip, 0);
  gEffectHost->clipGetHandle(effect, kOfxImageEffectOutputClipName, &myData->outputClip, 0);
  
  if(myData->isGeneralEffect) {
    gEffectHost->clipGetHandle(effect, "Mask", &myData->maskClip, 0);
  }
  else
    myData->maskClip = 0;

  // set my private instance data
  gPropHost->propSetPointer(effectProps, kOfxPropInstanceData, 0, (void *) myData);

  // As the parameters values have already been loaded, set 
  // the enabledness of the per component scale values
  setPerComponentScaleEnabledness(effect);

  return kOfxStatOK;
}

// instance destruction
static OfxStatus
destroyInstance( OfxImageEffectHandle  effect)
{
  // get my instance data
  MyInstanceData *myData = getMyInstanceData(effect);

  // and delete it
  if(myData)
    delete myData;
  return kOfxStatOK;
}

// tells the host what region we are capable of filling
OfxStatus 
getSpatialRoD( OfxImageEffectHandle  effect,  OfxPropertySetHandle inArgs,  OfxPropertySetHandle outArgs)
{
  // retrieve any instance data associated with this effect
  MyInstanceData *myData = getMyInstanceData(effect);

  OfxTime time;
  gPropHost->propGetDouble(inArgs, kOfxPropTime, 0, &time);
  
  // my RoD is the same as my input's
  OfxRectD rod;
  gEffectHost->clipGetRegionOfDefinition(myData->sourceClip, time, &rod);

  // note that the RoD is _not_ dependant on the Mask clip

  // set the rod in the out args
  gPropHost->propSetDoubleN(outArgs, kOfxImageEffectPropRegionOfDefinition, 4, &rod.x1);

  return kOfxStatOK;
}

// tells the host how much of the input we need to fill the given window
OfxStatus 
getSpatialRoI( OfxImageEffectHandle  effect,  OfxPropertySetHandle inArgs,  OfxPropertySetHandle outArgs)
{
  // get the RoI the effect is interested in from inArgs
  OfxRectD roi;
  gPropHost->propGetDoubleN(inArgs, kOfxImageEffectPropRegionOfInterest, 4, &roi.x1);

  // the input needed is the same as the output, so set that on the source clip
  gPropHost->propSetDoubleN(outArgs, "OfxImageClipPropRoI_Source", 4, &roi.x1);

  // retrieve any instance data associated with this effect
  MyInstanceData *myData = getMyInstanceData(effect);

  // if a general effect, we need to know the mask as well
  if(myData->isGeneralEffect && ofxuIsClipConnected(effect, "Mask")) {
    gPropHost->propSetDoubleN(outArgs, "OfxImageClipPropRoI_Mask", 4, &roi.x1);
  }
  return kOfxStatOK;
}

// Tells the host how many frames we can fill, only called in the general context.
// This is actually redundant as this is the default behaviour, but for illustrative
// purposes.
OfxStatus 
getTemporalDomain( OfxImageEffectHandle  effect,  OfxPropertySetHandle /*inArgs*/,  OfxPropertySetHandle outArgs)
{
  MyInstanceData *myData = getMyInstanceData(effect);

  double sourceRange[2];
  
  // get the frame range of the source clip
  OfxPropertySetHandle props; gEffectHost->clipGetPropertySet(myData->sourceClip, &props);
  gPropHost->propGetDoubleN(props, kOfxImageEffectPropFrameRange, 2, sourceRange);

  // set it on the out args
  gPropHost->propSetDoubleN(outArgs, kOfxImageEffectPropFrameRange, 2, sourceRange);
  
  return kOfxStatOK;
}


// Set our clip preferences 
static OfxStatus 
getClipPreferences( OfxImageEffectHandle  effect,  OfxPropertySetHandle /*inArgs*/,  OfxPropertySetHandle outArgs)
{
  // retrieve any instance data associated with this effect
  MyInstanceData *myData = getMyInstanceData(effect);
  
  // get the component type and bit depth of our main input
  int  bitDepth;
  bool isRGBA;
  ofxuClipGetFormat(myData->sourceClip, bitDepth, isRGBA, true); // get the unmapped clip component

  // get the strings used to label the various bit depths
  const char *bitDepthStr = bitDepth == 8 ? kOfxBitDepthByte : (bitDepth == 16 ? kOfxBitDepthShort : kOfxBitDepthFloat);
  const char *componentStr = isRGBA ? kOfxImageComponentRGBA : kOfxImageComponentAlpha;

  // set out output to be the same same as the input, component and bitdepth
  gPropHost->propSetString(outArgs, "OfxImageClipPropComponents_Output", 0, componentStr);
  if(gHostSupportsMultipleBitDepths)
    gPropHost->propSetString(outArgs, "OfxImageClipPropDepth_Output", 0, bitDepthStr);

  // if a general effect, we may have a mask input, check that for types
  if(myData->isGeneralEffect) {
    if(ofxuIsClipConnected(effect, "Mask")) {
      // set the mask input to be a single channel image of the same bitdepth as the source
      gPropHost->propSetString(outArgs, "OfxImageClipPropComponents_Mask", 0, kOfxImageComponentAlpha);
      if(gHostSupportsMultipleBitDepths) 
	gPropHost->propSetString(outArgs, "OfxImageClipPropDepth_Mask", 0, bitDepthStr);
    }
  }

  return kOfxStatOK;
}

// are the settings of the effect performing an identity operation
static OfxStatus
isIdentity( OfxImageEffectHandle  effect,
	    OfxPropertySetHandle inArgs,
	    OfxPropertySetHandle outArgs)
{
  // get the render window and the time from the inArgs
  OfxTime time;
  OfxRectI renderWindow;
  
  gPropHost->propGetDouble(inArgs, kOfxPropTime, 0, &time);
  gPropHost->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, &renderWindow.x1);

  // retrieve any instance data associated with this effect
  MyInstanceData *myData = getMyInstanceData(effect);

  double scaleValue, sR = 1, sG = 1, sB = 1, sA = 1;
  gParamHost->paramGetValueAtTime(myData->scaleParam, time, &scaleValue);

  if(ofxuGetClipPixelsAreRGBA(myData->sourceClip)) {
    gParamHost->paramGetValueAtTime(myData->scaleRParam, time, &sR);
    gParamHost->paramGetValueAtTime(myData->scaleGParam, time, &sG);
    gParamHost->paramGetValueAtTime(myData->scaleBParam, time, &sB);
    gParamHost->paramGetValueAtTime(myData->scaleAParam, time, &sA);
  }

  // if the scale values are all 1, then we have an identity xfm on the Source clip
  if(scaleValue == 1.0 && sR==1 && sG == 1 && sB == 1 && sA == 1) {
    // set the property in the out args indicating which is the identity clip
    gPropHost->propSetString(outArgs, kOfxPropName, 0, kOfxImageEffectSimpleSourceClipName);
    return kOfxStatOK;
  }

  // In this case do the default, which in this case is to render 
  return kOfxStatReplyDefault;
}

////////////////////////////////////////////////////////////////////////////////
// function called when the instance has been changed by anything
static OfxStatus
instanceChanged( OfxImageEffectHandle  effect,
		 OfxPropertySetHandle inArgs,
		 OfxPropertySetHandle /*outArgs*/)
{
  // see why it changed
  char *changeReason;
  gPropHost->propGetString(inArgs, kOfxPropChangeReason, 0, &changeReason);

  // we are only interested in user edits
  if(strcmp(changeReason, kOfxChangeUserEdited) != 0) return kOfxStatReplyDefault;

  // fetch the type of the object that changed
  char *typeChanged;
  gPropHost->propGetString(inArgs, kOfxPropType, 0, &typeChanged);

  // was it a clip or a param?
  bool isClip = strcmp(typeChanged, kOfxTypeClip) == 0;
  bool isParam = strcmp(typeChanged, kOfxTypeParameter) == 0;

  // get the name of the thing that changed
  char *objChanged;
  gPropHost->propGetString(inArgs, kOfxPropName, 0, &objChanged);

  // Did the source clip change or the 'scaleComponents' change? In which case enable/disable individual component scale parameters
  if((isClip && strcmp(objChanged, kOfxImageEffectSimpleSourceClipName)  == 0) ||
     (isParam && strcmp(objChanged, "scaleComponents")  == 0)) {
    setPerComponentScaleEnabledness(effect);
    return kOfxStatOK;
  }

  // don't trap any others
  return kOfxStatReplyDefault;
}

////////////////////////////////////////////////////////////////////////////////
// rendering routines
template <class T> inline T 
Clamp(T v, int min, int max)
{
  if(v < T(min)) return T(min);
  if(v > T(max)) return T(max);
  return v;
}

// look up a pixel in the image, does bounds checking to see if it is in the image rectangle
template <class PIX> inline PIX *
pixelAddress(PIX *img, OfxRectI rect, int x, int y, int bytesPerLine)
{  
  if(x < rect.x1 || x >= rect.x2 || y < rect.y1 || y >= rect.y2 || !img)
    return 0;
  PIX *pix = (PIX *) (((char *) img) + (y - rect.y1) * bytesPerLine);
  pix += x - rect.x1;  
  return pix;
}

////////////////////////////////////////////////////////////////////////////////
// base class to process images with
class Processor {
 protected :
  OfxImageEffectHandle  instance;
  float         rScale, gScale, bScale, aScale;
  void *srcV, *dstV, *maskV; 
  OfxRectI srcRect, dstRect, maskRect;
  int srcBytesPerLine, dstBytesPerLine, maskBytesPerLine;
  OfxRectI  window;

 public :
  Processor(OfxImageEffectHandle  inst,
            float rScal, float gScal, float bScal, float aScal,
            void *src, OfxRectI sRect, int sBytesPerLine,
            void *dst, OfxRectI dRect, int dBytesPerLine,
            void *mask, OfxRectI mRect, int mBytesPerLine,
            OfxRectI  win)
    : instance(inst)
    , rScale(rScal)
    , gScale(gScal)
    , bScale(bScal)
    , aScale(aScal)
    , srcV(src)
    , dstV(dst)
    , maskV(mask)
    , srcRect(sRect)
    , dstRect(dRect)
    , maskRect(mRect)
    , srcBytesPerLine(sBytesPerLine)
    , dstBytesPerLine(dBytesPerLine)
    , maskBytesPerLine(mBytesPerLine)
    , window(win)
  {}  

  static void multiThreadProcessing(unsigned int threadId, unsigned int nThreads, void *arg);
  virtual void doProcessing(OfxRectI window) = 0;
  void process(void);
};


// function call once for each thread by the host
void
Processor::multiThreadProcessing(unsigned int threadId, unsigned int nThreads, void *arg)
{
  Processor *proc = (Processor *) arg;

  // slice the y range into the number of threads it has
  unsigned int dy = proc->window.y2 - proc->window.y1;
  
  unsigned int y1 = proc->window.y1 + threadId * dy/nThreads;
  unsigned int y2 = proc->window.y1 + Minimum((threadId + 1) * dy/nThreads, dy);

  OfxRectI win = proc->window;
  win.y1 = y1; win.y2 = y2;

  // and render that thread on each
  proc->doProcessing(win);  
}

// function to kick off rendering across multiple CPUs
void
Processor::process(void)
{
  unsigned int nThreads;
  gThreadHost->multiThreadNumCPUs(&nThreads);
  gThreadHost->multiThread(multiThreadProcessing, nThreads, (void *) this);
}

// template to do the RGBA processing
template <class PIX, class MASK, int max, int isFloat>
class ProcessRGBA : public Processor{
public :
  ProcessRGBA(OfxImageEffectHandle  instance,
	      float rScale, float gScale, float bScale, float aScale,
	      void *srcV, OfxRectI srcRect, int srcBytesPerLine,
	      void *dstV, OfxRectI dstRect, int dstBytesPerLine,
	      void *maskV, OfxRectI maskRect, int maskBytesPerLine,
	      OfxRectI  window)
    : Processor(instance,
                rScale, gScale, bScale, aScale,
                srcV,  srcRect,  srcBytesPerLine,
                dstV,  dstRect,  dstBytesPerLine,
                maskV,  maskRect, maskBytesPerLine,
                window)
  {
  }

  void doProcessing(OfxRectI procWindow)
  {
    PIX *src = (PIX *) srcV;
    PIX *dst = (PIX *) dstV;
    MASK *mask = (MASK *) maskV;

    for(int y = procWindow.y1; y < procWindow.y2; y++) {
      if(gEffectHost->abort(instance)) break;

      PIX *dstPix = pixelAddress(dst, dstRect, procWindow.x1, y, dstBytesPerLine);

      for(int x = procWindow.x1; x < procWindow.x2; x++) {
        
        PIX *srcPix = pixelAddress(src, srcRect, x, y, srcBytesPerLine);
        
        
        // do any pixel masking?
        float maskV = 1.0f;
        if(mask) {
          MASK *maskPix = pixelAddress(mask, maskRect, x, y, maskBytesPerLine);
          if(maskPix) {
            maskV = float(*maskPix)/float(max);
          }
          else
            maskV = 0.0f;
          maskPix++;
        }

        // figure the scale values per component
        float sR = 1.0 + (rScale - 1.0) * maskV;
        float sG = 1.0 + (gScale - 1.0) * maskV;
        float sB = 1.0 + (bScale - 1.0) * maskV;
        float sA = 1.0 + (aScale - 1.0) * maskV;

        if(srcPix) {
          // switch will be compiled out
          if(isFloat) {
            dstPix->r = srcPix->r * sR;
            dstPix->g = srcPix->g * sG;
            dstPix->b = srcPix->b * sB;
            dstPix->a = srcPix->a * sA;
          }
          else {
            dstPix->r = Clamp(int(srcPix->r * sR), 0, max);
            dstPix->g = Clamp(int(srcPix->g * sG), 0, max);
            dstPix->b = Clamp(int(srcPix->b * sB), 0, max);
            dstPix->a = Clamp(int(srcPix->a * sA), 0, max);
          }
          srcPix++;
        }
        else {
          dstPix->r = dstPix->g = dstPix->b = dstPix->a= 0;
        }
        dstPix++;
      }
    }
  }
};

// template to do the Alpha processing
template <class PIX, class MASK, int max, int isFloat>
class ProcessAlpha : public Processor {
public :
  ProcessAlpha( OfxImageEffectHandle  instance,
               float scale,
               void *srcV, OfxRectI srcRect, int srcBytesPerLine,
               void *dstV, OfxRectI dstRect, int dstBytesPerLine,
               void *maskV, OfxRectI maskRect, int maskBytesPerLine,
               OfxRectI  window)
    : Processor(instance,
                scale, scale, scale, scale,
                srcV,  srcRect,  srcBytesPerLine,
                dstV,  dstRect,  dstBytesPerLine,
                maskV,  maskRect, maskBytesPerLine,
                window)
  {
  }

  void doProcessing(OfxRectI procWindow)
  {
    PIX *src = (PIX *) srcV;
    PIX *dst = (PIX *) dstV;
    MASK *mask = (MASK *) maskV;

    for(int y = procWindow.y1; y < procWindow.y2; y++) {
      if(gEffectHost->abort(instance)) break;

      PIX *dstPix = pixelAddress(dst, dstRect, procWindow.x1, y, dstBytesPerLine);

      for(int x = procWindow.x1; x < procWindow.x2; x++) {
        
        PIX *srcPix = pixelAddress(src, srcRect, x, y, srcBytesPerLine);

        // do any pixel masking?
        float maskV = 1.0f;
        if(mask) {
          MASK *maskPix = pixelAddress(mask, maskRect, x, y, maskBytesPerLine);
          if(maskPix) {
            maskV = float(*maskPix)/float(max);
          }
        }

        // figure the scale values per component
        float theScale = 1.0 + (rScale - 1.0) * maskV;

        if(srcPix) {
          // switch will be compiled out
          if(isFloat) {
            *dstPix = *srcPix * theScale;
          }
          else {
            *dstPix = Clamp(int(*srcPix * theScale), 0, max);
          }
          srcPix++;
        }
        else {
          *dstPix = 0;
        }
        dstPix++;
      }
    }
  }
};

// the process code  that the host sees
static OfxStatus render( OfxImageEffectHandle  instance,
                         OfxPropertySetHandle inArgs,
                         OfxPropertySetHandle /*outArgs*/)
{
  // get the render window and the time from the inArgs
  OfxTime time;
  OfxRectI renderWindow;
  OfxStatus status = kOfxStatOK;

  gPropHost->propGetDouble(inArgs, kOfxPropTime, 0, &time);
  gPropHost->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, &renderWindow.x1);

  // retrieve any instance data associated with this effect
  MyInstanceData *myData = getMyInstanceData(instance);

  // property handles and members of each image
  // in reality, we would put this in a struct as the C++ support layer does
  OfxPropertySetHandle sourceImg = NULL, outputImg = NULL, maskImg = NULL;
  int srcRowBytes, srcBitDepth, dstRowBytes, dstBitDepth, maskRowBytes = 0, maskBitDepth;
  bool srcIsAlpha, dstIsAlpha, maskIsAlpha = false;
  OfxRectI dstRect, srcRect, maskRect = {0};
  void *src, *dst, *mask = NULL;

  try {
    // get the source image
    sourceImg = ofxuGetImage(myData->sourceClip, time, srcRowBytes, srcBitDepth, srcIsAlpha, srcRect, src);
    if(sourceImg == NULL) throw OfxuNoImageException();

    // get the output image
    outputImg = ofxuGetImage(myData->outputClip, time, dstRowBytes, dstBitDepth, dstIsAlpha, dstRect, dst);
    if(outputImg == NULL) throw OfxuNoImageException();

    if(myData->isGeneralEffect) {
      // is the mask connected?
      if(ofxuIsClipConnected(instance, "Mask")) {
        maskImg = ofxuGetImage(myData->maskClip, time, maskRowBytes, maskBitDepth, maskIsAlpha, maskRect, mask);

        if(maskImg != NULL) {                        
          // and see that it is a single component
          if(!maskIsAlpha || maskBitDepth != srcBitDepth) {
            throw OfxuStatusException(kOfxStatErrImageFormat);
          }  
        }
      }
    }

    // see if they have the same depths and bytes and all
    if(srcBitDepth != dstBitDepth || srcIsAlpha != dstIsAlpha) {
      throw OfxuStatusException(kOfxStatErrImageFormat);
    }

    // are we compenent scaling
    int scaleComponents;
    gParamHost->paramGetValueAtTime(myData->perComponentScaleParam, time, &scaleComponents);

    // get the scale parameters
    double scale, rScale = 1, gScale = 1, bScale = 1, aScale = 1;
    gParamHost->paramGetValueAtTime(myData->scaleParam, time, &scale);

    if(scaleComponents) {
      gParamHost->paramGetValueAtTime(myData->scaleRParam, time, &rScale);
      gParamHost->paramGetValueAtTime(myData->scaleGParam, time, &gScale);
      gParamHost->paramGetValueAtTime(myData->scaleBParam, time, &bScale);
      gParamHost->paramGetValueAtTime(myData->scaleAParam, time, &aScale);
    }
    rScale *= scale; gScale *= scale; bScale *= scale; aScale *= scale;
  
    // do the rendering
    if(!dstIsAlpha) {
      switch(dstBitDepth) {
      case 8 : {      
        ProcessRGBA<OfxRGBAColourB, unsigned char, 255, 0> fred(instance, rScale, gScale, bScale, aScale,
                                                                src, srcRect, srcRowBytes,
                                                                dst, dstRect, dstRowBytes,
                                                                mask, maskRect, maskRowBytes,
                                                                renderWindow);
        fred.process();                                          
      }
        break;

      case 16 : {
        ProcessRGBA<OfxRGBAColourS, unsigned short, 65535, 0> fred(instance, rScale, gScale, bScale, aScale,
                                                                   src, srcRect, srcRowBytes,
                                                                   dst, dstRect, dstRowBytes,
                                                                   mask, maskRect, maskRowBytes,
                                                                   renderWindow);
        fred.process();           
      }                          
        break;

      case 32 : {
        ProcessRGBA<OfxRGBAColourF, float, 1, 1> fred(instance, rScale, gScale, bScale, aScale,
                                                      src, srcRect, srcRowBytes,
                                                      dst, dstRect, dstRowBytes,
                                                      mask, maskRect, maskRowBytes,
                                                      renderWindow);
        fred.process();                                          
        break;
      }
      }
    }
    else {
      switch(dstBitDepth) {
      case 8 : {
        ProcessAlpha<unsigned char, unsigned char, 255, 0> fred(instance, scale, 
                                                                src, srcRect, srcRowBytes,
                                                                dst, dstRect, dstRowBytes,
                                                                mask, maskRect, maskRowBytes,
                                                                renderWindow);
        fred.process();                                                                                  
      }
        break;

      case 16 : {
        ProcessAlpha<unsigned short, unsigned short, 65535, 0> fred(instance, scale, 
                                                                    src, srcRect, srcRowBytes,
                                                                    dst, dstRect, dstRowBytes,
                                                                    mask, maskRect, maskRowBytes,
                                                                    renderWindow);
        fred.process();           
      }                          
        break;

      case 32 : {
        ProcessAlpha<float, float, 1, 1> fred(instance, scale, 
                                              src, srcRect, srcRowBytes,
                                              dst, dstRect, dstRowBytes,
                                              mask, maskRect, maskRowBytes,
                                              renderWindow);
        fred.process();           
      }                          
        break;
      }
    }
  }
  catch(OfxuNoImageException &ex) {
    // if we were interrupted, the failed fetch is fine, just return kOfxStatOK
    // otherwise, something wierd happened
    if(!gEffectHost->abort(instance)) {
      status = kOfxStatFailed;
    }
  }
  catch(OfxuStatusException &ex) {
    status = ex.status();
  }

  // release the data pointers
  if(maskImg)
    gEffectHost->clipReleaseImage(maskImg);
  if(sourceImg)
    gEffectHost->clipReleaseImage(sourceImg);
  if(outputImg)
    gEffectHost->clipReleaseImage(outputImg);
  
  return status;
}

// convience function to define scaling parameter
static void
defineScaleParam( OfxParamSetHandle effectParams,
                 const char *name,
                 const char *label,
                 const char *scriptName,
                 const char *hint,
                 const char *parent)
{
  OfxPropertySetHandle props;
  OfxStatus stat;
  stat = gParamHost->paramDefine(effectParams, kOfxParamTypeDouble, name, &props);
  if (stat != kOfxStatOK) {
    throw OfxuStatusException(stat);
  }
  // say we are a scaling parameter
  gPropHost->propSetString(props, kOfxParamPropDoubleType, 0, kOfxParamDoubleTypeScale);
  gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 1.0);
  gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.0);
  gPropHost->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.0);
  gPropHost->propSetDouble(props, kOfxParamPropDisplayMax, 0, 100.0);
  gPropHost->propSetString(props, kOfxParamPropHint, 0, hint);
  gPropHost->propSetString(props, kOfxParamPropScriptName, 0, scriptName);
  gPropHost->propSetString(props, kOfxPropLabel, 0, label);
  if(parent)
    gPropHost->propSetString(props, kOfxParamPropParent, 0, parent);
}

//  describe the plugin in context
static OfxStatus
describeInContext( OfxImageEffectHandle  effect,  OfxPropertySetHandle inArgs)
{
  // get the context from the inArgs handle
  char *context;
  gPropHost->propGetString(inArgs, kOfxImageEffectPropContext, 0, &context);
  bool isGeneralContext = strcmp(context, kOfxImageEffectContextGeneral) == 0;

  OfxPropertySetHandle props;
  // define the single output clip in both contexts
  gEffectHost->clipDefine(effect, kOfxImageEffectOutputClipName, &props);

  // set the component types we can handle on out output
  gPropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
  gPropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 1, kOfxImageComponentAlpha);

  // define the single source clip in both contexts
  gEffectHost->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &props);

  // set the component types we can handle on our main input
  gPropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
  gPropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 1, kOfxImageComponentAlpha);

  if(isGeneralContext) {
    // define a second input that is a mask, alpha only and is optional
    gEffectHost->clipDefine(effect, "Mask", &props);
    gPropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentAlpha);
    gPropHost->propSetInt(props, kOfxImageClipPropOptional, 0, 1);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // define the parameters for this context
  // fetch the parameter set from the effect
  OfxParamSetHandle paramSet;
  gEffectHost->getParamSet(effect, &paramSet);

  // overall scale param
  defineScaleParam(paramSet, "scale", "scale", "scale", "Scales all component in the image", 0);

  // boolean param to enable/disable per component scaling
  gParamHost->paramDefine(paramSet, kOfxParamTypeBoolean, "scaleComponents", &props);
  gPropHost->propSetInt(props, kOfxParamPropDefault, 0, 0);
  gPropHost->propSetString(props, kOfxParamPropHint, 0, "Enables scales on individual components");
  gPropHost->propSetString(props, kOfxParamPropScriptName, 0, "scaleComponents");
  gPropHost->propSetString(props, kOfxPropLabel, 0, "Scale Individual Components");
  
  // grouping parameter for the by component params
  gParamHost->paramDefine(paramSet, kOfxParamTypeGroup, "componentScales", &props);
  gPropHost->propSetString(props, kOfxParamPropHint, 0, "Scales on the individual component");
  gPropHost->propSetString(props, kOfxPropLabel, 0, "Components");

  // rgb and a scale params
  defineScaleParam(paramSet, "scaleR", "red", "scaleR", 
                   "Scales the red component of the image", "componentScales");
  defineScaleParam(paramSet, "scaleG", "green", "scaleG",
                   "Scales the green component of the image", "componentScales");
  defineScaleParam(paramSet, "scaleB", "blue", "scaleB", 
                   "Scales the blue component of the image", "componentScales");
  defineScaleParam(paramSet, "scaleA", "alpha", "scaleA", 
                   "Scales the alpha component of the image", "componentScales");

  
  // make a page of controls and add my parameters to it
  gParamHost->paramDefine(paramSet, kOfxParamTypePage, "Main", &props);
  gPropHost->propSetString(props, kOfxParamPropPageChild, 0, "scale");
  gPropHost->propSetString(props, kOfxParamPropPageChild, 1, "scaleComponents");
  gPropHost->propSetString(props, kOfxParamPropPageChild, 2, "scaleR");
  gPropHost->propSetString(props, kOfxParamPropPageChild, 3, "scaleG");
  gPropHost->propSetString(props, kOfxParamPropPageChild, 4, "scaleB");
  gPropHost->propSetString(props, kOfxParamPropPageChild, 5, "scaleA");


  return kOfxStatOK;
}

////////////////////////////////////////////////////////////////////////////////
// the plugin's description routine
static OfxStatus
describe(OfxImageEffectHandle  effect)
{
  // first fetch the host APIs, this cannot be done before this call
  OfxStatus stat;
  if((stat = ofxuFetchHostSuites()) != kOfxStatOK)
    return stat;

  // record a few host features
  gPropHost->propGetInt(gHost->host, kOfxImageEffectPropSupportsMultipleClipDepths, 0, &gHostSupportsMultipleBitDepths);

  // get the property handle for the plugin
  OfxPropertySetHandle effectProps;
  gEffectHost->getPropertySet(effect, &effectProps);

  // We can render both fields in a fielded images in one hit if there is no animation
  // So set the flag that allows us to do this
  gPropHost->propSetInt(effectProps, kOfxImageEffectPluginPropFieldRenderTwiceAlways, 0, 0);

  // say we can support multiple pixel depths and let the clip preferences action deal with it all.
  gPropHost->propSetInt(effectProps, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 1);
  
  // set the bit depths the plugin can handle
  gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthByte);
  gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthShort);
  gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 2, kOfxBitDepthFloat);

  // set some labels and the group it belongs to
  gPropHost->propSetString(effectProps, kOfxPropLabel, 0, "OFX Gain Example");
  gPropHost->propSetString(effectProps, kOfxImageEffectPluginPropGrouping, 0, "OFX Example");

  // define the contexts we can be used in
  gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
  gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedContexts, 1, kOfxImageEffectContextGeneral);
  
  return kOfxStatOK;
}

////////////////////////////////////////////////////////////////////////////////
// The main function
static OfxStatus
pluginMain(const char *action,  const void *handle, OfxPropertySetHandle inArgs,  OfxPropertySetHandle outArgs)
{
  try {
  // cast to appropriate type
  OfxImageEffectHandle effect = (OfxImageEffectHandle) handle;

  if(strcmp(action, kOfxActionDescribe) == 0) {
    return describe(effect);
  }
  else if(strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
    return describeInContext(effect, inArgs);
  }
  else if(strcmp(action, kOfxActionLoad) == 0) {
    return onLoad();
  }
  else if(strcmp(action, kOfxActionUnload) == 0) {
    return onUnLoad();
  }
  else if(strcmp(action, kOfxActionCreateInstance) == 0) {
    return createInstance(effect);
  } 
  else if(strcmp(action, kOfxActionDestroyInstance) == 0) {
    return destroyInstance(effect);
  } 
  else if(strcmp(action, kOfxImageEffectActionIsIdentity) == 0) {
    return isIdentity(effect, inArgs, outArgs);
  }    
  else if(strcmp(action, kOfxImageEffectActionRender) == 0) {
    return render(effect, inArgs, outArgs);
  }    
  else if(strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
    return getSpatialRoD(effect, inArgs, outArgs);
  }  
  else if(strcmp(action, kOfxImageEffectActionGetRegionsOfInterest) == 0) {
    return getSpatialRoI(effect, inArgs, outArgs);
  }  
  else if(strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) {
    return getClipPreferences(effect, inArgs, outArgs);
  }  
  else if(strcmp(action, kOfxActionInstanceChanged) == 0) {
    return instanceChanged(effect, inArgs, outArgs);
  }  
  else if(strcmp(action, kOfxImageEffectActionGetTimeDomain) == 0) {
    return getTemporalDomain(effect, inArgs, outArgs);
  }  
  } catch (std::bad_alloc) {
    // catch memory
    //std::cout << "OFX Plugin Memory error." << std::endl;
    return kOfxStatErrMemory;
  } catch ( const std::exception& e ) {
    // standard exceptions
    //std::cout << "OFX Plugin error: " << e.what() << std::endl;
    return kOfxStatErrUnknown;
  } catch (int err) {
    // ho hum, gone wrong somehow
    return err;
  } catch ( ... ) {
    // everything else
    //std::cout << "OFX Plugin error" << std::endl;
    return kOfxStatErrUnknown;
  }

  // other actions to take the default value
  return kOfxStatReplyDefault;
}

// function to set the host structure
static void
setHostFunc(OfxHost *hostStruct)
{
  gHost         = hostStruct;
}

////////////////////////////////////////////////////////////////////////////////
// the plugin struct 
static OfxPlugin basicPlugin = 
{       
  kOfxImageEffectPluginApi,
  1,
  "org.opencolorio.BasicGainPlugin",
  1,
  0,
  setHostFunc,
  pluginMain
};
   
// // the two mandated functions
// OfxGetPlugin(int nth)
// {
//   if(nth == 0)
//     return &basicPlugin;
//   return 0;
// }
 
// OfxGetNumberOfPlugins(void)
// {       
//   return 1;
// }

int OfxGetNumberOfPlugins(void)
{
  return 1;
}

// return the OfxPlugin struct for the nth plugin
OfxPlugin * OfxGetPlugin(int nth)
{
  if(nth == 0)
    return &basicPlugin;
  return 0;
}