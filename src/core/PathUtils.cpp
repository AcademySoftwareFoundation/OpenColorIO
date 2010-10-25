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

#include <OpenColorIO/OpenColorIO.h>

#include "PathUtils.h"
#include "pystring/pystring.h"

#include <sstream>
#include <iostream>
#include <fstream>

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
    
    EnvMap GetEnvMap()
    {
        EnvMap map;
        for (char **env = GetEnviron(); *env != NULL; ++env)
        {
            // split environment up into std::map[name] = value
            std::string env_str = (char*)*env;
            int pos = env_str.find_first_of('=');
            map.insert(
                EnvMap::value_type(env_str.substr(0, pos),
                env_str.substr(pos+1, env_str.length()))
            );
        }
        return map;
    }
    
    #define OCIO_ENVEXPAND_MAX 30
    
    void EnvExpand(std::string *str, EnvMap *map)
    {
        std::string orig = *str;
        int i = 0;
        for (EnvMap::const_iterator iter = map->begin();
             iter != map->end(); ++iter)
        {
            i++;
            *str = pystring::replace(*str,
                ("${"+iter->first+"}"), iter->second,
                OCIO_ENVEXPAND_MAX);
            *str = pystring::replace(*str,
                ("$"+iter->first), iter->second,
                OCIO_ENVEXPAND_MAX);
            *str = pystring::replace(*str,
                ("%"+iter->first+"%"), iter->second,
                OCIO_ENVEXPAND_MAX);
            if(i >=  OCIO_ENVEXPAND_MAX)
                return;
        }
        // recursively call till string doesn't expand anymore
        if(*str != orig)
            EnvExpand(str, map);
        return;
    }
    
    bool FileExists (std::string filename) {
        std::ifstream fin;
        fin.open (filename.c_str());
        if (fin.fail())
            return false;
        fin.close();
        return true;
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
    OCIO::EnvExpand(&foo, &env_map);
    BOOST_CHECK( foo == foo_result );
    
}

BOOST_AUTO_TEST_SUITE_END()

#endif // OCIO_BUILD_TESTS
