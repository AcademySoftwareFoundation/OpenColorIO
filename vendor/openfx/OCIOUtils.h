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
typedef std::map<std::string, std::string> ContextMap;

/* Constants */
static const std::string PARAM_NAME_PAGE_0 = "Controls";

/* Default plugin setup */
void baseDescribe(const std::string & name, OFX::ImageEffectDescriptor& desc);
void baseDescribeInContext(OFX::ImageEffectDescriptor& desc);

/* Get the current OCIO config */
OCIO::ConstConfigRcPtr getOCIOConfig();

/* Convert OFX bit-depth enum to OCIO bit-depth enum */
OCIO::BitDepth getOCIOBitDepth(OFX::BitDepthEnum ofxBitDepth);

/* Get number of bytes in a single pixel component at OCIO bit-depth */
int getChanStrideBytes(OCIO::BitDepth ocioBitDepth);

/* Build color space ChoiceParam from the current OCIO config */
void defineCsNameParam(OFX::ImageEffectDescriptor & desc,
                       OFX::PageParamDescriptor * page,
                       const std::string & name, 
                       const std::string & label, 
                       const std::string & hint,
                       OFX::GroupParamDescriptor * parent);

/* Build display ChoiceParam from the current OCIO config */
void defineDisplayParam(OFX::ImageEffectDescriptor & desc,
                        OFX::PageParamDescriptor * page,
                        const std::string & name, 
                        const std::string & label, 
                        const std::string & hint,
                        OFX::GroupParamDescriptor * parent);

/* Build view ChoiceParam from the default OCIO config display */
void defineViewParam(OFX::ImageEffectDescriptor & desc,
                     OFX::PageParamDescriptor * page,
                     const std::string & name, 
                     const std::string & label, 
                     const std::string & hint,
                     OFX::GroupParamDescriptor * parent);

/* Build simple BooleanParam, defaulting to false */
void defineBooleanParam(OFX::ImageEffectDescriptor & desc,
                        OFX::PageParamDescriptor * page,
                        const std::string & name, 
                        const std::string & label, 
                        const std::string & hint,
                        OFX::GroupParamDescriptor * parent,
                        bool defaultValue = false);

/* Build simple StringParam */
void defineStringParam(OFX::ImageEffectDescriptor & desc,
                       OFX::PageParamDescriptor * page,
                       const std::string & name, 
                       const std::string & label, 
                       const std::string & hint,
                       OFX::GroupParamDescriptor * parent,
                       bool isSecret = false,
                       std::string defaultValue = "",
                       OFX::StringTypeEnum stringType = OFX::eStringTypeSingleLine);

/* Build simple PushButtonParam */
void definePushButtonParam(OFX::ImageEffectDescriptor & desc,
                           OFX::PageParamDescriptor * page,
                           const std::string & name, 
                           const std::string & label, 
                           const std::string & hint,
                           OFX::GroupParamDescriptor * parent);

/* Build GroupParam with four context variable StringParam name/value pairs */
void defineContextParams(OFX::ImageEffectDescriptor & desc,
                         OFX::PageParamDescriptor * page);

/* Fetch StringParams defined by defineContextParams */
void fetchContextParams(OFX::ImageEffect & instance, ParamMap & params);

/* Update internal context_store param on context variable StringParam change */
void contextParamChanged(OFX::ImageEffect & instance, 
                         const std::string & paramName);

/* Create copy of the current OCIO context with additional or overridden context
   variables from StringParams defined by defineContextParams.
 */
OCIO::ContextRcPtr createOCIOContext(ParamMap & params);

/* Get current option string from a ChoiceParam */
std::string getChoiceParamOption(OFX::ChoiceParam * param);

/* Update internal *_store param on config ChoiceParam change */
void choiceParamChanged(OFX::ImageEffect & instance, 
                        const std::string & paramName);

/* Restore "missing" config ChoiceParam option from internal *_store param */
void restoreChoiceParamOption(OFX::ImageEffect & instance,
                              const std::string & paramName,
                              const std::string & pluginType);

/* Update view ChoiceParam options from current display ChoiceParam option */
void updateViewParamOptions(OFX::ChoiceParam * displayParam, 
                            OFX::ChoiceParam * viewParam);

#endif // INCLUDED_OFX_OCIOUTILS_H
