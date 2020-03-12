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
ColorSpaceNames FindColorSpaceNames(ConstConfigRcPtr config, const Categories & categories);
// Return all the active color space names.
ColorSpaceNames FindAllColorSpaceNames(ConstConfigRcPtr config);

using Infos = std::vector<ConstColorSpaceInfoRcPtr>;
// Return information on all the active color spaces having at least one of the categories.
Infos FindColorSpaceInfos(ConstConfigRcPtr config, const Categories & categories);
// Return information on all the active color spaces.
Infos FindAllColorSpaceInfos(ConstConfigRcPtr config);

// Return information for a role which could be empty if not found.
ConstColorSpaceInfoRcPtr GetRoleInfo(ConstConfigRcPtr config, const char * roleName);


Infos getColorSpaceInfosFromCategories(ConstConfigRcPtr config,
                                       const char * role,        // Could be null or empty.
                                       const char * categories); // Could be null or empty.

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CATEGORY_HELPERS_H
