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

#include <map>
#include <string>
#include <iostream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"
#include "Mutex.h"
#include "PathUtils.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{

namespace
{
    typedef std::map< std::string, std::string> StringMap;
    
    void GetAbsoluteSearchPaths(std::vector<std::string> & searchpaths,
                                const std::string & pathString,
                                const std::string & configRootDir,
                                const EnvMap & map);
}
    
    class Context::Impl
    {
    public:
        std::string searchPath_;
        std::string workingDir_;
        EnvironmentMode envmode_;
        EnvMap envMap_;
        
        mutable std::string cacheID_;
        mutable StringMap resultsCache_;
        mutable Mutex resultsCacheMutex_;
        
        Impl() :
            envmode_(ENV_ENVIRONMENT_LOAD_PREDEFINED)
        {
        }
        
        ~Impl()
        {
        
        }
        
        Impl& operator= (const Impl & rhs)
        {
            AutoMutex lock1(resultsCacheMutex_);
            AutoMutex lock2(rhs.resultsCacheMutex_);
            
            searchPath_ = rhs.searchPath_;
            workingDir_ = rhs.workingDir_;
            envMap_ = rhs.envMap_;
            
            resultsCache_ = rhs.resultsCache_;
            cacheID_ = rhs.cacheID_;
            
            return *this;
        }
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    ContextRcPtr Context::Create()
    {
        return ContextRcPtr(new Context(), &deleter);
    }
    
    void Context::deleter(Context* c)
    {
        delete c;
    }
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    Context::Context()
    : m_impl(new Context::Impl)
    {
    }
    
    Context::~Context()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    ContextRcPtr Context::createEditableCopy() const
    {
        ContextRcPtr context = Context::Create();
        *context->m_impl = *getImpl();
        return context;
    }
    
    const char * Context::getCacheID() const
    {
        AutoMutex lock(getImpl()->resultsCacheMutex_);
        
        if(getImpl()->cacheID_.empty())
        {
            std::ostringstream cacheid;
            cacheid << "Search Path " << getImpl()->searchPath_ << " ";
            cacheid << "Working Dir " << getImpl()->workingDir_ << " ";
            cacheid << "Environment Mode " << getImpl()->envmode_ << " ";
            
            for (EnvMap::const_iterator iter = getImpl()->envMap_.begin(),
                 end = getImpl()->envMap_.end();
                 iter != end; ++iter)
            {
                cacheid << iter->first << "=" << iter->second << " ";
            }
            
            std::string fullstr = cacheid.str();
            getImpl()->cacheID_ = CacheIDHash(fullstr.c_str(), (int)fullstr.size());
        }
        
        return getImpl()->cacheID_.c_str();
    }
    
    void Context::setSearchPath(const char * path)
    {
        AutoMutex lock(getImpl()->resultsCacheMutex_);
        
        getImpl()->searchPath_ = path;
        getImpl()->resultsCache_.clear();
        getImpl()->cacheID_ = "";
    }
    
    const char * Context::getSearchPath() const
    {
        return getImpl()->searchPath_.c_str();
    }
    
    void Context::setWorkingDir(const char * dirname)
    {
        AutoMutex lock(getImpl()->resultsCacheMutex_);
        
        getImpl()->workingDir_ = dirname;
        getImpl()->resultsCache_.clear();
        getImpl()->cacheID_ = "";
    }
    
    const char * Context::getWorkingDir() const
    {
        return getImpl()->workingDir_.c_str();
    }
    
    void Context::setEnvironmentMode(EnvironmentMode mode)
    {
        AutoMutex lock(getImpl()->resultsCacheMutex_);
        
        getImpl()->envmode_ = mode;
        
        getImpl()->resultsCache_.clear();
        getImpl()->cacheID_ = "";
    }
    
    EnvironmentMode Context::getEnvironmentMode() const
    {
        return getImpl()->envmode_;
    }
    
    void Context::loadEnvironment()
    {
        bool update = (getImpl()->envmode_ == ENV_ENVIRONMENT_LOAD_ALL) ? false : true;
        LoadEnvironment(getImpl()->envMap_, update);
        
        AutoMutex lock(getImpl()->resultsCacheMutex_);
        getImpl()->resultsCache_.clear();
        getImpl()->cacheID_ = "";
    }
    
    void Context::setStringVar(const char * name, const char * value)
    {
        if(!name) return;
        
        AutoMutex lock(getImpl()->resultsCacheMutex_);
        
        // Set the value if specified
        if(value)
        {
            getImpl()->envMap_[name] = value;
        }
        // If a null value is specified, erase it
        else
        {
            EnvMap::iterator iter = getImpl()->envMap_.find(name);
            if(iter != getImpl()->envMap_.end())
            {
                getImpl()->envMap_.erase(iter);
            }
        }
        
        getImpl()->resultsCache_.clear();
        getImpl()->cacheID_ = "";
    }
    
    const char * Context::getStringVar(const char * name) const
    {
        if(!name) return "";
        
        EnvMap::const_iterator iter = getImpl()->envMap_.find(name);
        if(iter != getImpl()->envMap_.end())
        {
            return iter->second.c_str();
        }
        
        return "";
    }
    
    int Context::getNumStringVars() const
    {
        return static_cast<int>(getImpl()->envMap_.size());
    }
    
    const char * Context::getStringVarNameByIndex(int index) const
    {
        if(index < 0 || index >= static_cast<int>(getImpl()->envMap_.size()))
            return "";
        
        EnvMap::const_iterator iter = getImpl()->envMap_.begin();
        for(int count = 0; count<index; ++count) ++iter;
        
        return iter->first.c_str();
    }
    
    void Context::clearStringVars()
    {
        getImpl()->envMap_.clear();
    }
    
    const char * Context::resolveStringVar(const char * val) const
    {
        AutoMutex lock(getImpl()->resultsCacheMutex_);
        
        if(!val || !*val)
        {
            return "";
        }
        
        StringMap::const_iterator iter = getImpl()->resultsCache_.find(val);
        if(iter != getImpl()->resultsCache_.end())
        {
            return iter->second.c_str();
        }
        
        std::string resolvedval = EnvExpand(val, getImpl()->envMap_);
        
        getImpl()->resultsCache_[val] = resolvedval;
        return getImpl()->resultsCache_[val].c_str();
    }
    
    
    
    const char * Context::resolveFileLocation(const char * filename) const
    {
        AutoMutex lock(getImpl()->resultsCacheMutex_);
        
        if(!filename || !*filename)
        {
            return "";
        }
        
        StringMap::const_iterator iter = getImpl()->resultsCache_.find(filename);
        if(iter != getImpl()->resultsCache_.end())
        {
            return iter->second.c_str();
        }
        
        // Attempt to load an absolute file reference
        {
        std::string expandedfullpath = EnvExpand(filename, getImpl()->envMap_);
        if(pystring::os::path::isabs(expandedfullpath))
        {
            if(FileExists(expandedfullpath))
            {
                getImpl()->resultsCache_[filename] = expandedfullpath;
                return getImpl()->resultsCache_[filename].c_str();
            }
            std::ostringstream errortext;
            errortext << "The specified absolute file reference ";
            errortext << "'" << expandedfullpath << "' could not be located. ";
            throw Exception(errortext.str().c_str());
        }
        }
        
        // Load a relative file reference
        // Prep the search path vector
        // TODO: Cache this prepped vector?
        std::vector<std::string> searchpaths;
        GetAbsoluteSearchPaths(searchpaths,
                               getImpl()->searchPath_,
                               getImpl()->workingDir_,
                               getImpl()->envMap_);
        
        // Loop over each path, and try to find the file
        std::ostringstream errortext;
        errortext << "The specified file reference ";
        errortext << " '" << filename << "' could not be located. ";
        errortext << "The following attempts were made: ";
        
        for (unsigned int i = 0; i < searchpaths.size(); ++i)
        {
            // Make an attempt to find the lut in one of the search paths
            std::string fullpath = pystring::os::path::join(searchpaths[i], filename);
            std::string expandedfullpath = EnvExpand(fullpath, getImpl()->envMap_);
            if(FileExists(expandedfullpath))
            {
                getImpl()->resultsCache_[filename] = expandedfullpath;
                return getImpl()->resultsCache_[filename].c_str();
            }
            if(i!=0) errortext << " : ";
            errortext << expandedfullpath;
        }
        
        throw ExceptionMissingFile(errortext.str().c_str());
    }

    std::ostream& operator<< (std::ostream& os, const Context& context)
    {
        os << "Context:\n";
        for(int i=0; i<context.getNumStringVars(); ++i)
        {
            const char * key = context.getStringVarNameByIndex(i);
            os << key << "=" << context.getStringVar(key) << "\n";
        }
        return os;
    }
    

namespace
{
    void GetAbsoluteSearchPaths(std::vector<std::string> & searchpaths,
                                const std::string & pathString,
                                const std::string & workingDir,
                                const EnvMap & map)
    {
        if(pathString.empty())
        {
            searchpaths.push_back(workingDir);
            return;
        }
        
        std::vector<std::string> parts;
        pystring::split(pathString, parts, ":");
        
        for (unsigned int i = 0; i < parts.size(); ++i)
        {
            // Expand variables incase the expansion adds slashes
            std::string expanded = EnvExpand(parts[i], map);

            // Remove trailing "/", and spaces
            std::string dirname = pystring::rstrip(pystring::strip(expanded), "/");
            
            if(!pystring::os::path::isabs(dirname))
            {
                dirname = pystring::os::path::join(workingDir, dirname);
            }
            
            searchpaths.push_back(pystring::os::path::normpath(dirname));
        }
    }
}


}
OCIO_NAMESPACE_EXIT

///////////////////////////////////////////////////////////////////////////////

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"

#ifdef OCIO_SOURCE_DIR

#define _STR(x) #x
#define STR(x) _STR(x)

OIIO_ADD_TEST(Context, ABSPath)
{
    
    OCIO::ContextRcPtr con = OCIO::Context::Create();
    con->setSearchPath(STR(OCIO_SOURCE_DIR));
    
    con->setStringVar("non_abs", "src/core/Context.cpp");
    con->setStringVar("is_abs", STR(OCIO_SOURCE_DIR) "/src/core/Context.cpp");
    
    OIIO_CHECK_NO_THOW(con->resolveFileLocation("${non_abs}"));
    OIIO_CHECK_ASSERT(strcmp(con->resolveFileLocation("${non_abs}"),
        STR(OCIO_SOURCE_DIR) "/src/core/Context.cpp") == 0);
    
    OIIO_CHECK_NO_THOW(con->resolveFileLocation("${is_abs}"));
    OIIO_CHECK_ASSERT(strcmp(con->resolveFileLocation("${is_abs}"),
        STR(OCIO_SOURCE_DIR) "/src/core/Context.cpp") == 0);
    
}

#endif // OCIO_BINARY_DIR

#endif // OCIO_UNIT_TEST
