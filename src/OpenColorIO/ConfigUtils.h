// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_COLORSPACE_UTILS_H
#define INCLUDED_OCIO_COLORSPACE_UTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include <vector>

namespace OCIO_NAMESPACE
{
    // Return whether the color space contains SRGB or not.
    bool containsSRGB(ConstColorSpaceRcPtr & cs);

    // Get color space where isData is false and it has neither a to_ref or from_ref transform.
    std::string getRefSpace(const Config & cfg);

    // Get processor to a sRGB transform.
    ConstProcessorRcPtr getRefToSRGBTransform(const ConstConfigRcPtr & builtinConfig, 
                                              std::string refColorSpaceName);

    // Get reference space if the specified color space is a recognized linear space.
    std::string getReferenceSpaceFromLinearSpace(const Config & srcConfig,
                                                 const ConstColorSpaceRcPtr & cs,
                                                 const ConstConfigRcPtr & builtinConfig,
                                                 std::vector<std::string> & builtinLinearSpaces);

    // Get reference space if the specified color space is an sRGB texture space.
    std::string getReferenceSpaceFromSRGBSpace(const Config & config, 
                                               const ConstColorSpaceRcPtr cs,
                                               const ConstConfigRcPtr & builtinConfig,
                                               std::vector<std::string> & builtinLinearSpaces);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_BAKING_UTILS_H
