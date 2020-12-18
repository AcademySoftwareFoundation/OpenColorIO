// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_CATEGORY_HELPERS_H
#define INCLUDED_OCIO_CATEGORY_HELPERS_H

#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "utils/StringUtils.h"

namespace OCIO_NAMESPACE
{

class ColorSpaceInfo;
typedef OCIO_SHARED_PTR<ColorSpaceInfo> ColorSpaceInfoRcPtr;
typedef OCIO_SHARED_PTR<const ColorSpaceInfo> ConstColorSpaceInfoRcPtr;

class ColorSpaceMenuParametersImpl;

// Split a comma-separated list of tokens into separate strings and make each string lower case.
StringUtils::StringVec ExtractItems(const char * strings);

using Categories = StringUtils::StringVec;
using Encodings = StringUtils::StringVec;

using ColorSpaceNames = StringUtils::StringVec;
// Return all the active color space names having at least one of the categories.
ColorSpaceNames FindColorSpaceNames(ConstConfigRcPtr config, const Categories & categories);

using Infos = std::vector<ConstColorSpaceInfoRcPtr>;
Infos FindColorSpaceInfos(ConstConfigRcPtr config,
                          const Categories & appCategories,
                          const Categories & userCategories,
                          bool includeColorSpaces,
                          bool includeNamedTransforms,
                          const Encodings & encodings,
                          SearchReferenceSpaceType colorSpaceType);

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_CATEGORY_HELPERS_H
