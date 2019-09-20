// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_DISPLAY_H
#define INCLUDED_OCIO_DISPLAY_H

#include <OpenColorIO/OpenColorIO.h>

#include <string>
#include <vector>
#include <map>

#include "PrivateTypes.h"

OCIO_NAMESPACE_ENTER
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
    typedef std::map<std::string, ViewVec> DisplayMap;  // (display name : ViewVec)
    
    DisplayMap::iterator find_display(DisplayMap & displays, const std::string & display);
    
    DisplayMap::const_iterator find_display_const(const DisplayMap & displays, const std::string & display);
    
    int find_view(const ViewVec & vec, const std::string & name);
    
    void AddDisplay(DisplayMap & displays,
                    const std::string & display,
                    const std::string & view,
                    const std::string & colorspace,
                    const std::string & looks);
    
    void ComputeDisplays(StringVec & displayCache,
                         const DisplayMap & displays,
                         const StringVec & activeDisplays,
                         const StringVec & activeDisplaysEnvOverride);
    
}
OCIO_NAMESPACE_EXIT

#endif