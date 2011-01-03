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

#include <fstream>
#include <iostream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"
#include "PathUtils.h"
#include "pystring/pystring.h"

#ifdef __APPLE__
#include <crt_externs.h> // _NSGetEnviron()
#else
#include <unistd.h>
extern char **environ;
#endif

OCIO_NAMESPACE_ENTER
{
    namespace
    {
        inline char** GetEnviron()
        {
#ifdef __APPLE__
            return (*_NSGetEnviron());
#else
            return environ;
#endif
        }
    }
    
    namespace path
    {
        // TODO: make these also work on windows
        // This attempts to match python's path.join, including
        // the relative absolute handling
        
        std::string join(const std::string & path1, const std::string & path2)
        {
            std::string pathtoken = "/";
            
            // Absolute paths should be treated as absolute
            if(pystring::startswith(path2, pathtoken))
                return path2;
            
            // Relative paths will be appended.
            if (pystring::endswith(path1, pathtoken))
                return path1 + path2;
            
            return path1 + pathtoken + path2;
        }
        
        // TODO: This doesnt return the same result for python in '/foo' case
        std::string dirname(const std::string & path)
        {
            std::vector<std::string> result;
            pystring::rsplit(path, result, "/", 1);
            if(result.size() == 2)
            {
                return result[0];
            }
            return "";
        }
    } // path namespace
    
    
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
    
    namespace
    {
        typedef std::map<std::string, bool> FileExistsMap;
        
        FileExistsMap g_fileExistsCache;
        Mutex g_fileExistsCache_mutex;
    }
    
    bool FileExists(const std::string & filename)
    {
        AutoMutex lock(g_fileExistsCache_mutex);
        
        FileExistsMap::iterator iter = g_fileExistsCache.find(filename);
        if(iter != g_fileExistsCache.end())
        {
            return iter->second;
        }
        
        bool exists = true;
        {
            std::ifstream fin;
            fin.open (filename.c_str());
            if (fin.fail()) exists = false;
            else fin.close();
        }
        
        g_fileExistsCache[filename] = exists;
        return exists;
    }
    
    std::string GetExtension(const std::string & str)
    {
        std::vector<std::string> parts;
        pystring::rsplit(str, parts, ".", 1);
        if(parts.size() == 2) return parts[1];
        return "";
    }
}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

BOOST_AUTO_TEST_SUITE( PathUtils_Unit_Tests )

BOOST_AUTO_TEST_CASE ( test_envexpand )
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
    BOOST_CHECK( testresult == foo_result );
    
}

BOOST_AUTO_TEST_SUITE_END()

#endif // OCIO_BUILD_TESTS
