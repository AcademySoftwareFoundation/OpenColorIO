// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_DISPLAY_H
#define INCLUDED_OCIO_DISPLAY_H


#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Platform.h"
#include "PrivateTypes.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

// View can be part of the list of views of a display or the list of shared views of a config.
struct View
{
    std::string m_name;
    std::string m_viewTransform; // Might be empty.
    std::string m_colorspace;
    std::string m_looks;         // Might be empty.
    std::string m_rule;          // Might be empty.
    std::string m_description;   // Might be empty.

    View() = default;

    View(const char * name,
         const char * viewTransform,
         const char * colorspace,
         const char * looks,
         const char * rule,
         const char * description)
        : m_name(name)
        , m_viewTransform(viewTransform ? viewTransform : "")
        , m_colorspace(colorspace ? colorspace : "")
        , m_looks(looks ? looks : "")
        , m_rule(rule ? rule : "")
        , m_description(description ? description : "")
    { }

    // Make sure that csname is not null.
    static bool UseDisplayName(const char * csname)
    {
        return csname && 0 == Platform::Strcasecmp(csname, OCIO_VIEW_USE_DISPLAY_NAME);
    }
    bool useDisplayNameForColorspace() const
    {
        return UseDisplayName(m_colorspace.c_str());
    }
};

typedef std::vector<View> ViewVec;

ViewVec::const_iterator FindView(const ViewVec & vec, const std::string & name);
ViewVec::iterator FindView(ViewVec & vec, const std::string & name);

void AddView(ViewVec & views, const char * name, const char * viewTransform,
             const char * displayColorSpace, const char * looks,
             const char * rule, const char * description);

// Display can be part of the list of displays (DisplayMap) of a config.
struct Display
{
    // Used to not save displays that originate by instantiating a virtual display.
    bool m_temporary = false;

    // List of views defined by the display.
    ViewVec m_views;
    // List of references to shared views defined be a config.
    StringUtils::StringVec m_sharedViews;
};

// In 0.6, the Yaml lib changed their implementation of a Yaml::Map from a C++ map 
// to a std::vector< std::pair<> >.   We made the same change here so that the Display list 
// can remain in config order but we left the "Map" in the name since it refers to a Yaml::Map.
typedef std::pair<std::string, Display> DisplayPair;
typedef std::vector<DisplayPair> DisplayMap;  // Pair is (display name : ViewVec)

DisplayMap::iterator FindDisplay(DisplayMap & displays, const std::string & display);
DisplayMap::const_iterator FindDisplay(const DisplayMap & displays, const std::string & display);

void ComputeDisplays(StringUtils::StringVec & displayCache,
                     const DisplayMap & displays,
                     const StringUtils::StringVec & activeDisplays,
                     const StringUtils::StringVec & activeDisplaysEnvOverride);

} // namespace OCIO_NAMESPACE

#endif
