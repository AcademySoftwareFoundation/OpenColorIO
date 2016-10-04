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


#ifndef INCLUDED_OCIO_PATHUTILS_H
#define INCLUDED_OCIO_PATHUTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include <map>

OCIO_NAMESPACE_ENTER
{
    namespace pystring
    {
    namespace os
    {
    namespace path
    {
        // This is not currently included in pystring, but we need it
        // So let's define it locally for now
        
        std::string abspath(const std::string & path);
    }
    }
    }
    
    // The EnvMap is ordered by the length of the keys (long -> short). This
    // is so that recursive string expansion will deal with similar prefixed
    // keys as expected.
    // ie. '$TEST_$TESTING_$TE' will expand in this order '2 1 3'
    template <class T>
    struct EnvMapKey : std::binary_function <T, T, bool>
    {
        bool
        operator() (const T &x, const T &y) const
        {
            // If the lengths are unequal, sort by length
            if(x.length() != y.length())
            {
                return (x.length() > y.length());
            }
            // Otherwise, use the standard string sort comparison
            else
            {
                return (x<y);
            }
        }
    };
    typedef std::map< std::string, std::string, EnvMapKey< std::string > > EnvMap;
    
    // Get map of current env key = value, or update the existing entries
    void LoadEnvironment(EnvMap & map, bool update = false);
    
    // Expand a string with $VAR, ${VAR} or %VAR% with the keys passed
    // in the EnvMap.
    std::string EnvExpand(const std::string & str, const EnvMap & map);
    
    // Check if a file exists
    bool FileExists(const std::string & filename);
    
    // Get a fast hash for a file, without reading all the contents.
    // Currently, this checks the mtime and the inode number.
    std::string GetFastFileHash(const std::string & filename);
    
    void ClearPathCaches();
}
OCIO_NAMESPACE_EXIT

#endif
