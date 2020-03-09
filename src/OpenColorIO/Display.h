// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_DISPLAY_H
#define INCLUDED_OCIO_DISPLAY_H


#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

// Displays
struct View
{
    std::string name;
    std::string colorspace;
    std::string looks;

    View() { }

    View(const std::string & name_,
         const std::string & colorspace_,
         const std::string & looksList_) :
         name(name_),
         colorspace(colorspace_),
         looks(looksList_)
    { }
};

typedef std::vector<View> ViewVec;

// In 0.6, the Yaml lib changed their implementation of a Yaml::Map from a C++ map 
// to a std::vector< std::pair<> >.   We made the same change here so that the Display list 
// can remain in config order but we left the "Map" in the name since it refers to a Yaml::Map.
typedef std::vector<std::pair<std::string, ViewVec>> DisplayMap;  // Pair is (display name : ViewVec)

DisplayMap::iterator find_display(DisplayMap & displays, const std::string & display);

DisplayMap::const_iterator find_display_const(const DisplayMap & displays, const std::string & display);

int find_view(const ViewVec & vec, const std::string & name);

void AddDisplay(DisplayMap & displays,
                const std::string & display,
                const std::string & view,
                const std::string & colorspace,
                const std::string & looks);

void ComputeDisplays(StringUtils::StringVec & displayCache,
                     const DisplayMap & displays,
                     const StringUtils::StringVec & activeDisplays,
                     const StringUtils::StringVec & activeDisplaysEnvOverride);

} // namespace OCIO_NAMESPACE

#endif