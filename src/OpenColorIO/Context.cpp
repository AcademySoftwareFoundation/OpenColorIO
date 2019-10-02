// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "HashUtils.h"
#include "Mutex.h"
#include "PathUtils.h"
#include "PrivateTypes.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{

namespace
{
    void GetAbsoluteSearchPaths(StringVec & searchpaths,
                                const StringVec & pathStrings,
                                const std::string & configRootDir,
                                const EnvMap & map);
}
    
    class Context::Impl
    {
    public:
        // New platform-agnostic search paths vector.
        StringVec searchPaths_;
        // Original concatenated string search paths (keeping it for now to
        // avoid changes to the original API).
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
            if(this!=&rhs)
            {
                AutoMutex lock1(resultsCacheMutex_);
                AutoMutex lock2(rhs.resultsCacheMutex_);
                
                searchPaths_ = rhs.searchPaths_;
                searchPath_ = rhs.searchPath_;
                workingDir_ = rhs.workingDir_;
                envMap_ = rhs.envMap_;
                
                resultsCache_ = rhs.resultsCache_;
                cacheID_ = rhs.cacheID_;
            }
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
            if (!getImpl()->searchPaths_.empty())
            {
                cacheid << "Search Path ";
                for (auto & path : getImpl()->searchPaths_)
                {
                    cacheid << path << " ";
                }
            }
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
        
        pystring::split(path, getImpl()->searchPaths_, ":");
        
        getImpl()->searchPath_ = path;
        getImpl()->resultsCache_.clear();
        getImpl()->cacheID_ = "";
    }
    
    const char * Context::getSearchPath() const
    {
        return getImpl()->searchPath_.c_str();
    }
    
    int Context::getNumSearchPaths() const
    {
        return (int)getImpl()->searchPaths_.size();
    }

    const char * Context::getSearchPath(int index) const
    {
        if (index < 0 || index >= (int)getImpl()->searchPaths_.size()) return "";
        return getImpl()->searchPaths_[index].c_str();
    }

    void Context::clearSearchPaths()
    {
        AutoMutex lock(getImpl()->resultsCacheMutex_);

        getImpl()->searchPath_ = "";
        getImpl()->searchPaths_.clear();
        getImpl()->resultsCache_.clear();
        getImpl()->cacheID_ = "";
    }

    void Context::addSearchPath(const char * path)
    {
        AutoMutex lock(getImpl()->resultsCacheMutex_);

        if (strlen(path) != 0)
        {
            getImpl()->searchPaths_.emplace_back(path);
            getImpl()->resultsCache_.clear();
            getImpl()->cacheID_ = "";

            if (getImpl()->searchPath_.size() != 0)
            {
                getImpl()->searchPath_ += ":";
            }
            getImpl()->searchPath_ += getImpl()->searchPaths_.back();
        }
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
                getImpl()->resultsCache_[filename] = pystring::os::path::normpath(expandedfullpath);
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
        StringVec searchpaths;
        GetAbsoluteSearchPaths(searchpaths,
                               getImpl()->searchPaths_,
                               getImpl()->workingDir_,
                               getImpl()->envMap_);
        
        // Loop over each path, and try to find the file
        std::ostringstream errortext;
        errortext << "The specified file reference ";
        errortext << " '" << filename << "' could not be located. ";
        errortext << "The following attempts were made: ";
        
        for (unsigned int i = 0; i < searchpaths.size(); ++i)
        {
            // Make an attempt to find the LUT in one of the search paths
            std::string fullpath = pystring::os::path::join(searchpaths[i], filename);
            std::string expandedfullpath = EnvExpand(fullpath, getImpl()->envMap_);
            if(FileExists(expandedfullpath))
            {
                getImpl()->resultsCache_[filename] = pystring::os::path::normpath(expandedfullpath);
                return getImpl()->resultsCache_[filename].c_str();
            }
            if(i!=0) errortext << " : ";
            errortext << expandedfullpath;
        }
        
        throw ExceptionMissingFile(errortext.str().c_str());
    }

    std::ostream& operator<< (std::ostream& os, const Context& context)
    {
        os << "<Context";
        os << " searchPath=[";
        const int numSP = context.getNumSearchPaths();
        for (int i = 0; i < numSP; ++i)
        {
            os << "\"" << context.getSearchPath(i) << "\"";
            if (i != numSP - 1)
            {
                os << ", ";
            }
        }
        os << "], workingDir=" << context.getWorkingDir();
        os << ", environmentMode=" << EnvironmentModeToString(context.getEnvironmentMode());
        os << ", environment=";
        for(int i=0; i<context.getNumStringVars(); ++i)
        {
            const char * key = context.getStringVarNameByIndex(i);
            os << "\n\t" << key << ": " << context.getStringVar(key);
        }
        os << ">";
        return os;
    }
    

namespace
{
    void GetAbsoluteSearchPaths(StringVec & searchpaths,
                                const StringVec & pathStrings,
                                const std::string & workingDir,
                                const EnvMap & map)
    {
        if(pathStrings.empty())
        {
            searchpaths.push_back(workingDir);
            return;
        }
        
        for (unsigned int i = 0; i < pathStrings.size(); ++i)
        {
            // Expand variables in case the expansion adds slashes
            std::string expanded = EnvExpand(pathStrings[i], map);

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

