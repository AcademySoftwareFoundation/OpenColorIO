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
                                const std::string & configRootDir);
}
    
    class Context::Impl
    {
    public:
        std::string searchPath_;
        std::string workingDir_;
        EnvMap envMap_;
        
        mutable std::string cacheID_;
        mutable StringMap resultsCache_;
        mutable Mutex resultsCacheMutex_;
        
        Impl()
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
    
    void Context::loadEnvironment()
    {
        LoadEnvironment(getImpl()->envMap_);
    }
    
    void Context::setStringVar(const char * name, const char * value)
    {
        if(!name) return;
        
        AutoMutex lock(getImpl()->resultsCacheMutex_);
        getImpl()->resultsCache_.clear();
        getImpl()->cacheID_ = "";
        
        if(value)
        {
            getImpl()->envMap_.insert(EnvMap::value_type(name, value));
        }
        else
        {
            StringMap::iterator iter = getImpl()->envMap_.find(name);
            if(iter != getImpl()->envMap_.end())
            {
                getImpl()->envMap_.erase(iter);
            }
        }
    }
    
    const char * Context::getStringVar(const char * name) const
    {
        if(!name) return "";
        
        StringMap::const_iterator iter = getImpl()->envMap_.find(name);
        if(iter != getImpl()->envMap_.end())
        {
            return iter->second.c_str();
        }
        
        return "";
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
        
        if(pystring::startswith(filename, "/"))
        {
            std::string expandedfullpath = EnvExpand(filename, getImpl()->envMap_);
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
        
        // TODO: Cache this prepped vector?
        std::vector<std::string> searchpaths;
        GetAbsoluteSearchPaths(searchpaths,
                               getImpl()->searchPath_,
                               getImpl()->workingDir_);
        
        if(searchpaths.empty())
        {
            std::ostringstream os;
            os << "Relative file references ";
            os << "(" << filename << ") are not allowed. ";
            os << "No search path has been specified. ";
            throw Exception(os.str().c_str());
        }
        
        // Loop over each path, and try to find the file
        std::ostringstream errortext;
        errortext << "The specified file reference ";
        errortext << " '" << filename << "' could not be located. ";
        errortext << "The following attempts were made: ";
        
        for (unsigned int i = 0; i < searchpaths.size(); ++i)
        {
            // find the file?
            std::string fullpath = path::join(searchpaths[i], filename);
            std::string expandedfullpath = EnvExpand(fullpath, getImpl()->envMap_);
            if(FileExists(expandedfullpath))
            {
                getImpl()->resultsCache_[filename] = expandedfullpath;
                return getImpl()->resultsCache_[filename].c_str();
            }
            if(i!=0) errortext << " : ";
            errortext << expandedfullpath;
        }
        
        throw Exception(errortext.str().c_str());
    }


namespace
{
    void GetAbsoluteSearchPaths(std::vector<std::string> & searchpaths,
                                const std::string & pathString,
                                const std::string & workingDir)
    {
        if(pathString == "")
        {
            searchpaths.clear();
            return;
        }
        
        pystring::split(pathString, searchpaths, ":");
        
        // loop over each path and try to find the file
        for (unsigned int i = 0; i < searchpaths.size(); ++i)
        {
            // resolve '::' empty entry
            if(searchpaths[i].size() == 0)
            {
                searchpaths[i] = workingDir;
            }
            // TODO: resolve '..'
            else if(pystring::startswith(searchpaths[i], ".."))
            {
                std::ostringstream os;
                os << "Search paths starting with '..' : ";
                os << searchpaths[i];
                os << " are currently unhandled.";
                throw Exception(os.str().c_str());
                /*
                std::vector<std::string> result;
                pystring::rsplit(profilecwd, result, "/", 1);
                if(result.size() == 2)
                    searchpaths[i] = result[0];
                */
            }
            // TODO: resolve '.'
            else if(pystring::startswith(searchpaths[i], "."))
            {
                std::ostringstream os;
                os << "Search paths starting with '.' : ";
                os << searchpaths[i];
                os << " are currently unhandled.";
                throw Exception(os.str().c_str());
                /*
                searchpaths[i] = pystring::strip(searchpaths[i], ".");
                if(pystring::endswith(profilecwd, "/"))
                    searchpaths[i] = profilecwd + searchpaths[i];
                else
                    searchpaths[i] = profilecwd + "/" + searchpaths[i];
                */
            }
            // resolve relative
            else if(!pystring::startswith(searchpaths[i], "/"))
            {
                searchpaths[i] = path::join(workingDir, searchpaths[i]);
            }
            
            // Remove trailing "/"
            searchpaths[i] = pystring::rstrip(searchpaths[i], "/");
        }
    }

}


}
OCIO_NAMESPACE_EXIT
