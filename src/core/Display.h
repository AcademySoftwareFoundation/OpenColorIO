/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


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
                    StringVec & displayNames,
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
