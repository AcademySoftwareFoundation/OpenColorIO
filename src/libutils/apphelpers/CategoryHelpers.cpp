// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "CategoryHelpers.h"
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

Infos FindColorSpaceInfos(ConstConfigRcPtr config, const Categories & categories)
{
    ConstColorSpaceSetRcPtr allCSS = GetColorSpaces(config, categories);

    Infos allInfos;

    for (int idx = 0; idx < allCSS->getNumColorSpaces(); ++idx)
    {
        ConstColorSpaceRcPtr cs = allCSS->getColorSpaceByIndex(idx);
        allInfos.push_back(ColorSpaceInfo::Create(config, cs));
    }

    return allInfos;
}

Infos FindAllColorSpaceInfos(ConstConfigRcPtr config)
{
    Infos allInfos;

    for (int idx = 0; idx < config->getNumColorSpaces(); ++idx)
    {
        const char * csName = config->getColorSpaceNameByIndex(idx);
        ConstColorSpaceRcPtr cs = config->getColorSpace(csName);
        allInfos.push_back(ColorSpaceInfo::Create(config, cs));
    }

    return allInfos;
}

ConstColorSpaceInfoRcPtr GetRoleInfo(ConstConfigRcPtr config, const char * roleName)
{
    ConstColorSpaceRcPtr cs = config->getColorSpace(roleName);
    if (!cs)
    {
        return ConstColorSpaceInfoRcPtr();
    }

    std::stringstream uiName;
    uiName << roleName << " (" << cs->getName() << ")";

    return ColorSpaceInfo::Create(config, 
                                  roleName,
                                  uiName.str().c_str(), 
                                  nullptr, 
                                  nullptr);
}

Infos getColorSpaceInfosFromCategories(ConstConfigRcPtr config,
                                       const char * role,
                                       const char * categories)
{
    // Step 1 - If the role exists, use only that space.

    if (role && *role)
    {
        if (config->hasRole(role))
        {
            ConstColorSpaceRcPtr cs = config->getColorSpace(role);

            Infos colorSpaceNames;
            colorSpaceNames.push_back(ColorSpaceInfo::Create(config, cs));
            return colorSpaceNames;
        }
    }

    const Categories allCategories = ExtractCategories(categories);
    Infos colorSpaceNames;

    // Step 2 - Use the list of all active color spaces if allCategories is empty.

    if (allCategories.empty())
    {
        Infos tmp = FindAllColorSpaceInfos(config);
        colorSpaceNames.insert(colorSpaceNames.end(), tmp.begin(), tmp.end());
        return colorSpaceNames;
    }

    // Step 3 - Find all active color spaces having at least one category.

    Infos tmp = FindColorSpaceInfos(config, allCategories);
    if (!tmp.empty())
    {
        colorSpaceNames.insert(colorSpaceNames.end(), tmp.begin(), tmp.end());
    }
    else
    {
        // Note: No color spaces match the categories so use them all.
        Infos tmp = FindAllColorSpaceInfos(config);
        colorSpaceNames.insert(colorSpaceNames.end(), tmp.begin(), tmp.end());

        std::stringstream ss;
        ss << "Using all color spaces as none were found using the categories: [";
        ss << categories << "].";

        LogMessage(LOGGING_LEVEL_WARNING, ss.str().c_str());
    }

    return colorSpaceNames;
}

} // namespace OCIO_NAMESPACE
