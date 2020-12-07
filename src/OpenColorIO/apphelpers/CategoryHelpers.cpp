// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "CategoryHelpers.h"
#include "ColorSpaceHelpers.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

namespace
{

ConstColorSpaceSetRcPtr GetColorSpaces(ConstConfigRcPtr config, const Categories & categories)
{
    ConstColorSpaceSetRcPtr all = ColorSpaceSet::Create();

    for (const auto & cat : categories)
    {
        ConstColorSpaceSetRcPtr css = config->getColorSpaces(cat.c_str());
        all = all || css;
    }

    return all;
}

template<typename T>
ColorSpaceNames GetNames(const T & list)
{
    ColorSpaceNames allNames;

    for (int idx = 0; idx < list->getNumColorSpaces(); ++idx)
    {
        allNames.push_back(list->getColorSpaceNameByIndex(idx));
    }

    return allNames;
}

} // anon.

Categories ExtractCategories(const char * categories)
{
    Categories tmp = StringUtils::Split(StringUtils::Lower(categories), ',');
    Categories all;

    for (const auto & val : tmp)
    {
        auto v = StringUtils::Trim(val);
        if (!v.empty())
        {
            all.push_back(v);
        }
    }

    return all;
}

ColorSpaceNames FindColorSpaceNames(ConstConfigRcPtr config, const Categories & categories)
{
    ConstColorSpaceSetRcPtr allCS = GetColorSpaces(config, categories);
    return GetNames(allCS);
}

ColorSpaceNames FindAllColorSpaceNames(ConstConfigRcPtr config)
{
    return GetNames(config);
}

Infos FindColorSpaceInfos(ConstConfigRcPtr config,
                          const Categories & categories,
                          bool includeNamedTransforms)
{
    ConstColorSpaceSetRcPtr allCSS = GetColorSpaces(config, categories);

    Infos allInfos;

    for (int idx = 0; idx < allCSS->getNumColorSpaces(); ++idx)
    {
        ConstColorSpaceRcPtr cs = allCSS->getColorSpaceByIndex(idx);
        allInfos.push_back(ColorSpaceInfo::Create(config, cs));
    }

    if (includeNamedTransforms)
    {
        for (const auto & cat : categories)
        {
            for (int idx = 0; idx < config->getNumNamedTransforms(); ++idx)
            {
                const char * ntName = config->getNamedTransformNameByIndex(idx);
                ConstNamedTransformRcPtr nt = config->getNamedTransform(ntName);
                if (nt->hasCategory(cat.c_str()))
                {
                    allInfos.push_back(ColorSpaceInfo::Create(config, nt));
                }
            }
        }
    }
    return allInfos;
}

Infos FindAllColorSpaceInfos(ConstConfigRcPtr config, bool includeNamedTransforms)
{
    Infos allInfos;

    for (int idx = 0; idx < config->getNumColorSpaces(); ++idx)
    {
        const char * csName = config->getColorSpaceNameByIndex(idx);
        ConstColorSpaceRcPtr cs = config->getColorSpace(csName);
        allInfos.push_back(ColorSpaceInfo::Create(config, cs));
    }

    if (includeNamedTransforms)
    {
        for (int idx = 0; idx < config->getNumNamedTransforms(); ++idx)
        {
            const char * ntName = config->getNamedTransformNameByIndex(idx);
            ConstNamedTransformRcPtr nt = config->getNamedTransform(ntName);
            allInfos.push_back(ColorSpaceInfo::Create(config, nt));
        }
    }
    return allInfos;
}

Infos getColorSpaceInfosFromCategories(ConstConfigRcPtr config,
                                       const char * role,
                                       const char * categories,
                                       ColorSpaceMenuHelper::IncludeTypeFlag includeFlag)
{
    // Step 1 - If the role exists, use only that space.

    if (role && *role)
    {
        if (config->hasRole(role))
        {
            Infos colorSpaceNames;
            colorSpaceNames.push_back(ColorSpaceInfo::CreateFromRole(config, role, nullptr));
            return colorSpaceNames;
        }
    }

    // Step 2 - Add active color spaces from categories.

    const Categories allCategories = ExtractCategories(categories);
    Infos colorSpaceNames;

    const bool includeNT = HasFlag(includeFlag, ColorSpaceMenuHelper::INCLUDE_NAMEDTRANSFORMS);

    if (allCategories.empty())
    {
        // Step 2a - Use the list of all active color spaces if allCategories is empty.

        Infos tmp = FindAllColorSpaceInfos(config, includeNT);
        colorSpaceNames.insert(colorSpaceNames.end(), tmp.begin(), tmp.end());
    }
    else
    {
        // Step 2b - Find all active color spaces having at least one category.

        Infos tmp = FindColorSpaceInfos(config, allCategories, includeNT);
        if (!tmp.empty())
        {
            colorSpaceNames.insert(colorSpaceNames.end(), tmp.begin(), tmp.end());
        }
        else
        {
            // Note: No color spaces match the categories so use them all.
            Infos tmp = FindAllColorSpaceInfos(config, includeNT);
            colorSpaceNames.insert(colorSpaceNames.end(), tmp.begin(), tmp.end());

            std::stringstream ss;
            ss << "Using all color spaces as none were found using the categories: [";
            ss << categories << "].";

            LogMessage(LOGGING_LEVEL_WARNING, ss.str().c_str());
        }
    }

    // Step 3 - Include roles if needed.

    if (HasFlag(includeFlag, ColorSpaceMenuHelper::INCLUDE_ROLES))
    {
        for (int idx = 0; idx < config->getNumRoles(); ++idx)
        {
            ConstColorSpaceInfoRcPtr info = ColorSpaceInfo::CreateFromRole(config,
                                                                           config->getRoleName(idx),
                                                                           "Roles");
            colorSpaceNames.push_back(info);
        }
    }

    return colorSpaceNames;
}

} // namespace OCIO_NAMESPACE
