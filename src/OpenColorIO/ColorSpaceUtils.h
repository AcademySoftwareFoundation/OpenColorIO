// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_COLORSPACE_UTILS_H
#define INCLUDED_OCIO_COLORSPACE_UTILS_H

#include <vector>
#include <string>

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{
std::string getRefSpace(ConstConfigRcPtr & cfg);

bool containsSRGB(ConstColorSpaceRcPtr & cs);


bool rgbWithinAbsTolerance(std::vector<float> & rgb0, 
                           std::vector<float> & rgb1, 
                           float scale0, 
                           float tolerance);

GroupTransformRcPtr getRefToSRGBTransform(ConstConfigRcPtr & builtinConfig, 
                                          std::string refColorSpaceName);

ConstGroupTransformRcPtr combineGroupTransforms(ConstTransformRcPtr tf1, ConstTransformRcPtr tf2);

bool isIdentityTransform(ConstConfigRcPtr & config, 
                         ConstGroupTransformRcPtr & gt, 
                         std::vector<std::vector<float>> & vals);

std::string checkForLinearColorSpace(ConstConfigRcPtr & config, 
                                     ConstColorSpaceRcPtr & cs,
                                     ConstConfigRcPtr & builtinConfig,
                                     std::vector<std::string> & builtinLinearSpaces);

std::string checkForSRGBTextureColorSpace(ConstConfigRcPtr & config, 
                                          ConstColorSpaceRcPtr cs,
                                          ConstConfigRcPtr & builtinConfig,
                                          std::vector<std::string> & builtinLinearSpaces);
}

#endif