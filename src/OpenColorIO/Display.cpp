// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "Display.h"
#include "ParseUtils.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

DisplayMap::iterator FindDisplay(DisplayMap & displays, const std::string & name)
{
    return std::find_if(displays.begin(), displays.end(),
                        [name](DisplayPair & display)
                        {
                            return 0 == Platform::Strcasecmp(name.c_str(), display.first.c_str());
                        });
}

DisplayMap::const_iterator FindDisplay(const DisplayMap & displays, const std::string & name)
{
    return std::find_if(displays.begin(), displays.end(),
                        [name](const DisplayPair & display)
                        {
                            return 0 == Platform::Strcasecmp(name.c_str(), display.first.c_str());
                        });
}

ViewVec::const_iterator FindView(const ViewVec & vec, const std::string & name)
{
    return std::find_if(vec.begin(), vec.end(),
                        [name](const View & view)
                        {
                            return 0 == Platform::Strcasecmp(name.c_str(), view.m_name.c_str());
                        });
}

ViewVec::iterator FindView(ViewVec & vec, const std::string & name)
{
    return std::find_if(vec.begin(), vec.end(),
                        [name](View & view)
                        {
                            return 0 == Platform::Strcasecmp(name.c_str(), view.m_name.c_str());
                        });
}

void AddView(ViewVec & views, const char * name, const char * viewTransform,
             const char * displayColorSpace, const char * looks,
             const char * rule, const char * description)
{
    if (0 == Platform::Strcasecmp(displayColorSpace, OCIO_VIEW_USE_DISPLAY_NAME))
    {
        displayColorSpace = OCIO_VIEW_USE_DISPLAY_NAME;
    }
    auto view = FindView(views, name);
    if (view == views.end())
    {
        views.push_back(View(name, viewTransform, displayColorSpace, looks, rule, description));
    }
    else
    {
        (*view).m_viewTransform = viewTransform     ? viewTransform     : "";
        (*view).m_colorspace    = displayColorSpace ? displayColorSpace : "";
        (*view).m_looks         = looks             ? looks             : "";
        (*view).m_rule          = rule              ? rule              : "";
        (*view).m_description   = description       ? description       : "";
    }
}

void ComputeDisplays(StringUtils::StringVec & displayCache,
                     const DisplayMap & displays,
                     const StringUtils::StringVec & activeDisplays,
                     const StringUtils::StringVec & activeDisplaysEnvOverride)
{
    displayCache.clear();

    StringUtils::StringVec displayMasterList;
    for(const auto & display : displays)
    {
        displayMasterList.push_back(display.first);
    }

    // Apply the env override if it's not empty.
    if(!activeDisplaysEnvOverride.empty())
    {
        displayCache = IntersectStringVecsCaseIgnore(activeDisplaysEnvOverride, displayMasterList);
        if(!displayCache.empty()) return;
    }
    // Otherwise, apply the active displays if it's not empty.
    else if(!activeDisplays.empty())
    {
        displayCache = IntersectStringVecsCaseIgnore(activeDisplays, displayMasterList);
        if(!displayCache.empty()) return;
    }

    displayCache = displayMasterList;
}

} // namespace OCIO_NAMESPACE
