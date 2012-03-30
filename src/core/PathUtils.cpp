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
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <sys/stat.h>

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"
#include "PathUtils.h"
#include "Platform.h"
#include "pystring/pystring.h"

#if !defined(WINDOWS)
#include <sys/param.h>
#else
#include <direct.h>
#define MAXPATHLEN 4096
#endif

#if defined(__APPLE__) && !defined(__IPHONE__)
#include <crt_externs.h> // _NSGetEnviron()
#elif !defined(WINDOWS)
#include <unistd.h>
extern char **environ;
#endif

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        typedef std::map<std::string, std::string> StringMap;
        
        StringMap g_fastFileHashCache;
        Mutex g_fastFileHashCache_mutex;
        
        std::string ComputeHash(const std::string & filename)
        {
            struct stat results;
            if (stat(filename.c_str(), &results) == 0)
            {
                // Treat the mtime + inode as a proxy for the contents
                std::ostringstream fasthash;
                fasthash << results.st_ino << ":";
                fasthash << results.st_mtime;
                return fasthash.str();
            }
            
            return "";
        }
    }
    
    std::string GetFastFileHash(const std::string & filename)
    {
        AutoMutex lock(g_fastFileHashCache_mutex);
        
        StringMap::iterator iter = g_fastFileHashCache.find(filename);
        if(iter != g_fastFileHashCache.end())
        {
            return iter->second;
        }
        
        std::string hash = ComputeHash(filename);
        g_fastFileHashCache[filename] = hash;
        
        return hash;
    }
    
    bool FileExists(const std::string & filename)
    {
        std::string hash = GetFastFileHash(filename);
        return (!hash.empty());
    }
    
    void ClearPathCaches()
    {
        AutoMutex lock(g_fastFileHashCache_mutex);
        g_fastFileHashCache.clear();
    }
    
    namespace pystring
    {
    namespace os
    {
        std::string getcwd()
        {
#ifdef WINDOWS
            char path[MAXPATHLEN];
            _getcwd(path, MAXPATHLEN);
            return path;
#else
            char path[MAXPATHLEN];
            ::getcwd(path, MAXPATHLEN);
            return path;
#endif
        }
        
    namespace path
    {
        std::string abspath(const std::string & path)
        {
            std::string p = path;
            if(!isabs(p)) p = join(getcwd(), p);
            return normpath(p);
        }
    } // namespace path
    } // namespace os
    }
    
    namespace
    {
        inline char** GetEnviron()
        {
#if __IPHONE__
            // TODO: fix this
            return NULL;
#elif __APPLE__
            return (*_NSGetEnviron());
#else
            return environ;
#endif
        }
        
        const int MAX_PATH_LENGTH = 4096;
    }
    
    void LoadEnvironment(EnvMap & map)
    {
        for (char **env = GetEnviron(); *env != NULL; ++env)
        {
            // split environment up into std::map[name] = value
            std::string env_str = (char*)*env;
            int pos = static_cast<int>(env_str.find_first_of('='));
            map.insert(
                EnvMap::value_type(env_str.substr(0, pos),
                env_str.substr(pos+1, env_str.length()))
            );
        }
    }
    
    std::string EnvExpand(const std::string & str, const EnvMap & map)
    {
        // Early exit if no magic characters are found.
        if(pystring::find(str, "$") == -1 && 
           pystring::find(str, "%") == -1) return str;
        
        std::string orig = str;
        std::string newstr = str;
        
        // This walks through the envmap in key order,
        // from longest to shortest to handle envvars which are
        // substrings.
        // ie. '$TEST_$TESTING_$TE' will expand in this order '2 1 3'
        
        for (EnvMap::const_iterator iter = map.begin();
             iter != map.end(); ++iter)
        {
            newstr = pystring::replace(newstr,
                ("${"+iter->first+"}"), iter->second);
            newstr = pystring::replace(newstr,
                ("$"+iter->first), iter->second);
            newstr = pystring::replace(newstr,
                ("%"+iter->first+"%"), iter->second);
        }
        
        // recursively call till string doesn't expand anymore
        if(newstr != orig)
        {
            return EnvExpand(newstr, map);
        }
        
        return orig;
    }
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

OIIO_ADD_TEST(PathUtils, EnvExpand)
{
    // build env by hand for unit test
    OCIO::EnvMap env_map; // = OCIO::GetEnvMap();
   
    // add some fake env vars so the test runs
    env_map.insert(OCIO::EnvMap::value_type("TEST1", "foo.bar"));
    env_map.insert(OCIO::EnvMap::value_type("TEST1NG", "bar.foo"));
    env_map.insert(OCIO::EnvMap::value_type("FOO_foo.bar", "cheese"));
   
    //
    std::string foo = "/a/b/${TEST1}/${TEST1NG}/$TEST1/$TEST1NG/${FOO_${TEST1}}/";
    std::string foo_result = "/a/b/foo.bar/bar.foo/foo.bar/bar.foo/cheese/";
    std::string testresult = OCIO::EnvExpand(foo, env_map);
    OIIO_CHECK_ASSERT( testresult == foo_result );
}

#endif // OCIO_BUILD_TESTS
