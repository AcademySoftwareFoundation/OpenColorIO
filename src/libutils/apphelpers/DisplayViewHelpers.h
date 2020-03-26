// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_DISPLAYVIEW_HELPERS_H
#define INCLUDED_OCIO_DISPLAYVIEW_HELPERS_H


#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

namespace DisplayViewHelpers
{

// Get the processor from the working color space to (display, view) pair (forward)
// or (display, view) pair to working (inverse). The working color space name could be 
// a role name, a color space name or a UI color space name.
ConstProcessorRcPtr GetProcessor(const ConstConfigRcPtr & config,
                                 const char * workingName,
                                 const char * displayName,
                                 const char * viewName,
                                 TransformDirection direction);

// Add a new (display, view) pair and the new color space to a configuration instance.
// The input to the userTransform must be in the specified connectionColorSpace.
void AddDisplayView(ConfigRcPtr & config,
                    const char * displayName,
                    const char * viewName,
                    const char * lookDefinition,  // Could be empty or null.
                    const ColorSpaceInfo & colorSpaceInfo,
                    FileTransformRcPtr & userTransform,
                    const char * categories,      // Could be empty or null.
                    const char * connectionColorSpaceName);

// Remove a (display, view) pair including the associated color space (only if not used).
// Note that the view is always removed but the display is only removed if empty.
void RemoveDisplayView(ConfigRcPtr & config, const char * displayName, const char * viewName);

} // DisplayViewHelpers


}  // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_DISPLAYVIEW_HELPERS_H
