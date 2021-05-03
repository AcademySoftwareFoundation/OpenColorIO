// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OFX_OCIOUTILS_H
#define INCLUDED_OFX_OCIOUTILS_H

#include <string>
#include <map>

#include "ofxsImageEffect.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

typedef std::map<std::string, OFX::StringParam *> ParamMap;

/* Get the current OCIO config */
OCIO::ConstConfigRcPtr getOCIOConfig();

/* Convert OFX bit-depth enum to OCIO bit-depth enum */
OCIO::BitDepth getOCIOBitDepth(OFX::BitDepthEnum ofxBitDepth);

/* Get number of bytes in a single pixel component at OCIO bit-depth */
int getChanStrideBytes(OCIO::BitDepth ocioBitDepth);

/* Build color space ChoiceParam from the current OCIO config */
OFX::ChoiceParamDescriptor * defineCsNameParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent);

/* Build display ChoiceParam from the current OCIO config */
OFX::ChoiceParamDescriptor * defineDisplayParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent);

/* Build view ChoiceParam from the default OCIO config display */
OFX::ChoiceParamDescriptor * defineViewParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent);

/* Build simple BooleanParam, defaulting to false */
OFX::BooleanParamDescriptor * defineBooleanParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent,
    bool defaultValue = false);

/* Build simple StringParam */
OFX::StringParamDescriptor * defineStringParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent,
    std::string defaultValue = "",
    OFX::StringTypeEnum stringType = OFX::eStringTypeSingleLine);

OFX::PushButtonParamDescriptor * definePushButtonParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent);

/* Build GroupParam with four context variable StringParam name/value pairs */
void defineContextParams(OFX::ImageEffectDescriptor & desc,
                         OFX::PageParamDescriptor * page);

/* Fetch StringParams defined by defineContextParams */
void fetchContextParams(OFX::ImageEffect & instance, ParamMap & params);

/* Create copy of the current OCIO context with additional or overridden context
   variables from StringParams defined by defineContextParams.
 */
OCIO::ContextRcPtr createOCIOContext(ParamMap & params);

/* Get current option string from a ChoiceParam */
std::string getChoiceParamOption(OFX::ChoiceParam * param);

/* Update view ChoiceParam options from current display ChoiceParam option */
void updateViewParamOptions(OFX::ChoiceParam * displayParam, 
                            OFX::ChoiceParam * viewParam);

#endif // INCLUDED_OFX_OCIOUTILS_H
