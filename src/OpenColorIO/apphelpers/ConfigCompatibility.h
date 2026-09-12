// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CONFIG_COMPATIBILITY_H
#define INCLUDED_OCIO_CONFIG_COMPATIBILITY_H

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO_NAMESPACE
{

namespace ConfigCompatibilityHelpers
{

// Returns true if the config has at least one display color space and all of them have an
// interop ID and an encoding set.
bool AllDisplayColorSpacesHaveAttributes(const ConstConfigRcPtr & config);

// Returns true if the config has at least one (display, view) pair using a view transform, and
// all views reference a non-empty view transform, except that a view with no view transform is
// allowed if the color space it refers to has isData set to true.
bool AllViewsHaveViewTransform(const ConstConfigRcPtr & config);

bool CheckHDRDisplaySupport26(const ConstConfigRcPtr & config);

} // namespace ConfigCompatibilityHelpers

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CONFIG_COMPATIBILITY_H
