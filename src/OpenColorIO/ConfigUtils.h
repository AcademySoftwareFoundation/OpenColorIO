// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_CONFIG_UTILS_H
#define INCLUDED_OCIO_CONFIG_UTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include <vector>

namespace OCIO_NAMESPACE
{

namespace ConfigUtils
{
    const char * getBuiltinLinearSpaces(int index);
    
    // Return whether the color space contains SRGB or not.
    bool containsSRGB(ConstColorSpaceRcPtr & cs);

    // Get color space where isData is false and it has neither a to_ref or from_ref transform.
    int getRefSpace(const Config & cfg);

    bool isIdentityTransform(const Config & srcConfig, GroupTransformRcPtr & tf, std::vector<float> & vals, float absTolerance);

    // Get reference to a sRGB transform.
    TransformRcPtr getTransformToSRGBSpace(const ConstConfigRcPtr & builtinConfig, 
                                           std::string refColorSpaceName);

    // Get reference space if the specified color space is a recognized linear space.
    int getReferenceSpaceFromLinearSpace(const Config & srcConfig,
                                         const ConstColorSpaceRcPtr & cs,
                                         const ConstConfigRcPtr & builtinConfig);

    // Get reference space if the specified color space is an sRGB texture space.
    int getReferenceSpaceFromSRGBSpace(const Config & config, 
                                       const ConstColorSpaceRcPtr cs,
                                       const ConstConfigRcPtr & builtinConfig);

    // Identify the interchange space of the source config and the default built-in config.                  
    void identifyInterchangeSpace(int & srcInterchange, Config & eSrcConfig);
    // Identify the interchange space the default built-in config.
    void identifyBuiltinInterchangeSpace(int & builtinInterchangeIndex, Config & eSrcConfig);

    // Find the name of the color space in the source config that is the same as 
    // a color space in the default built-in config.
    int identifyBuiltinColorSpace(const char * builtinColorSpaceName, Config & eSrcConfig);
}

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CONFIG_UTILS_H
