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

#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "Display.h"
#include "ParseUtils.h"

OCIO_NAMESPACE_ENTER
{
    
    DisplayMap::iterator find_display(DisplayMap & displays, const std::string & display)
    {
        for(DisplayMap::iterator iter = displays.begin();
            iter != displays.end();
            ++iter)
        {
            if(StrEqualsCaseIgnore(display, iter->first)) return iter;
        }
        return displays.end();
    }
    
    DisplayMap::const_iterator find_display_const(const DisplayMap & displays, const std::string & display)
    {
        for(DisplayMap::const_iterator iter = displays.begin();
            iter != displays.end();
            ++iter)
        {
            if(StrEqualsCaseIgnore(display, iter->first)) return iter;
        }
        return displays.end();
    }
    
    int find_view(const ViewVec & vec, const std::string & name)
    {
        for(unsigned int i=0; i<vec.size(); ++i)
        {
            if(StrEqualsCaseIgnore(name, vec[i].name)) return i;
        }
        return -1;
    }
    
    void AddDisplay(DisplayMap & displays,
                    StringVec & displayNames,
                    const std::string & display,
                    const std::string & view,
                    const std::string & colorspace,
                    const std::string & looks)
    {
        DisplayMap::iterator iter = find_display(displays, display);
        if(iter == displays.end())
        {
            ViewVec views;
            views.push_back( View(view, colorspace, looks) );
            displays[display] = views;
            displayNames.push_back(display);
        }
        else
        {
            ViewVec & views = iter->second;
            int index = find_view(views, view);
            if(index<0)
            {
                views.push_back( View(view, colorspace, looks) );
            }
            else
            {
                views[index].colorspace = colorspace;
                views[index].looks = looks;
            }
        }
    }
    
    void ComputeDisplays(StringVec & displayCache,
                         const DisplayMap & displays,
                         const StringVec & activeDisplays,
                         const StringVec & activeDisplaysEnvOverride)
    {
        displayCache.clear();
        
        StringVec displayMasterList;
        for(DisplayMap::const_iterator iter = displays.begin();
            iter != displays.end();
            ++iter)
        {
            displayMasterList.push_back(iter->first);
        }
        
        // Apply the env override if it's not empty.
        if(!activeDisplaysEnvOverride.empty())
        {
            displayCache = IntersectStringVecsCaseIgnore(displayMasterList, activeDisplaysEnvOverride);
            if(!displayCache.empty()) return;
        }
        // Otherwise, aApply the active displays if it's not empty.
        else if(!activeDisplays.empty())
        {
            displayCache = IntersectStringVecsCaseIgnore(displayMasterList, activeDisplays);
            if(!displayCache.empty()) return;
        }
        
        displayCache = displayMasterList;
    }

}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(Display, Basic)
{
    
}

#endif // OCIO_UNIT_TEST
