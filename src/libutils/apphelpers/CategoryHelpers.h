// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CATEGORY_HELPERS_H
#define INCLUDED_OCIO_CATEGORY_HELPERS_H

#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "ColorSpaceHelpers.h"


namespace OCIO_NAMESPACE
{

using Categories = std::vector<std::string>;
// Returns the list of categories.
Categories ExtractCategories(const char * categories);

using ColorSpaceNames = std::vector<std::string>;
// Return all the active color space names having at least one of the categories.
ColorSpaceNames FindColorSpaceNames(ConstConfigRcPtr config,
                                    const Categories & categories);
// Return all the active color space names.
ColorSpaceNames FindAllColorSpaceNames(ConstConfigRcPtr config, bool includeNamedTransforms);

using Infos = std::vector<ConstColorSpaceInfoRcPtr>;
// Return information on all the active color spaces having at least one of the categories.
Infos FindColorSpaceInfos(ConstConfigRcPtr config,
                          const Categories & categories,
                          bool includeNamedTransforms);
// Return information on all the active color spaces.
Infos FindAllColorSpaceInfos(ConstConfigRcPtr config, bool includeNamedTransforms);

// Return information for a role (result will be empty if the role doesn't exist).
ConstColorSpaceInfoRcPtr GetRoleInfo(ConstConfigRcPtr config, const char * roleName);


// Return information useful for building color space menus using the following heuristics.  If the
// role is non-empty and exists, just return that space.  If the categories are non-empty, return
// all color spaces that have at least one of the categories.  Otherwise if categories are empty or
// none of them match any color spaces, return all the color spaces. Set includeNamedTransforms to
// true to also search named transforms.
Infos getColorSpaceInfosFromCategories(ConstConfigRcPtr config,
                                       const char * role,        // Could be null or empty.
                                       const char * categories,  // Could be null or empty.
                                       bool includeNamedTransforms);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CATEGORY_HELPERS_H
