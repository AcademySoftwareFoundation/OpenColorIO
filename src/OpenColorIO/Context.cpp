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
#include "utils/StringUtils.h"


namespace OCIO_NAMESPACE
{

namespace
{
void GetAbsoluteSearchPaths(StringUtils::StringVec & searchpaths,
                            const StringUtils::StringVec & pathStrings,
                            const std::string & configRootDir,
                            const EnvMap & map);
}

class Context::Impl
{
public:
    // New platform-agnostic search paths vector.
    StringUtils::StringVec m_searchPaths;
    // Original concatenated string search paths (keeping it for now to
    // avoid changes to the original API).
    std::string m_searchPath;
    std::string m_workingDir;
    EnvironmentMode m_envmode;
    EnvMap m_envMap;

    mutable std::string m_cacheID;
    mutable StringMap m_resultsCache;
    mutable Mutex m_resultsCacheMutex;

    Impl() :
        m_envmode(ENV_ENVIRONMENT_LOAD_PREDEFINED)
    {
    }

    ~Impl()
    {

    }

    Impl& operator= (const Impl & rhs)
    {
        if(this!=&rhs)
        {
            AutoMutex lock1(m_resultsCacheMutex);
            AutoMutex lock2(rhs.m_resultsCacheMutex);

            m_searchPaths = rhs.m_searchPaths;
            m_searchPath = rhs.m_searchPath;
            m_workingDir = rhs.m_workingDir;
            m_envMap = rhs.m_envMap;

            m_resultsCache = rhs.m_resultsCache;
            m_cacheID = rhs.m_cacheID;
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
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    if(getImpl()->m_cacheID.empty())
    {
        std::ostringstream cacheid;
        if (!getImpl()->m_searchPaths.empty())
        {
            cacheid << "Search Path ";
            for (auto & path : getImpl()->m_searchPaths)
            {
                cacheid << path << " ";
            }
        }
        cacheid << "Working Dir " << getImpl()->m_workingDir << " ";
        cacheid << "Environment Mode " << getImpl()->m_envmode << " ";

        for (EnvMap::const_iterator iter = getImpl()->m_envMap.begin(),
                end = getImpl()->m_envMap.end();
                iter != end; ++iter)
        {
            cacheid << iter->first << "=" << iter->second << " ";
        }

        std::string fullstr = cacheid.str();
        getImpl()->m_cacheID = CacheIDHash(fullstr.c_str(), (int)fullstr.size());
    }

    return getImpl()->m_cacheID.c_str();
}

void Context::setSearchPath(const char * path)
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    getImpl()->m_searchPaths = StringUtils::Split(path, ':');
    getImpl()->m_searchPath  = path;
    getImpl()->m_resultsCache.clear();
    getImpl()->m_cacheID = "";
}

const char * Context::getSearchPath() const
{
    return getImpl()->m_searchPath.c_str();
}

int Context::getNumSearchPaths() const
{
    return (int)getImpl()->m_searchPaths.size();
}

const char * Context::getSearchPath(int index) const
{
    if (index < 0 || index >= (int)getImpl()->m_searchPaths.size()) return "";
    return getImpl()->m_searchPaths[index].c_str();
}

void Context::clearSearchPaths()
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    getImpl()->m_searchPath = "";
    getImpl()->m_searchPaths.clear();
    getImpl()->m_resultsCache.clear();
    getImpl()->m_cacheID = "";
}

void Context::addSearchPath(const char * path)
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    if (strlen(path) != 0)
    {
        getImpl()->m_searchPaths.emplace_back(path);
        getImpl()->m_resultsCache.clear();
        getImpl()->m_cacheID = "";

        if (getImpl()->m_searchPath.size() != 0)
        {
            getImpl()->m_searchPath += ":";
        }
        getImpl()->m_searchPath += getImpl()->m_searchPaths.back();
    }
}

void Context::setWorkingDir(const char * dirname)
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    getImpl()->m_workingDir = dirname;
    getImpl()->m_resultsCache.clear();
    getImpl()->m_cacheID = "";
}

const char * Context::getWorkingDir() const
{
    return getImpl()->m_workingDir.c_str();
}

void Context::setEnvironmentMode(EnvironmentMode mode)
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    getImpl()->m_envmode = mode;

    getImpl()->m_resultsCache.clear();
    getImpl()->m_cacheID = "";
}

EnvironmentMode Context::getEnvironmentMode() const
{
    return getImpl()->m_envmode;
}

void Context::loadEnvironment()
{
    bool update = (getImpl()->m_envmode == ENV_ENVIRONMENT_LOAD_ALL) ? false : true;
    LoadEnvironment(getImpl()->m_envMap, update);

    AutoMutex lock(getImpl()->m_resultsCacheMutex);
    getImpl()->m_resultsCache.clear();
    getImpl()->m_cacheID = "";
}

void Context::setStringVar(const char * name, const char * value)
{
    if(!name) return;

    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    // Set the value if specified
    if(value)
    {
        getImpl()->m_envMap[name] = value;
    }
    // If a null value is specified, erase it
    else
    {
        EnvMap::iterator iter = getImpl()->m_envMap.find(name);
        if(iter != getImpl()->m_envMap.end())
        {
            getImpl()->m_envMap.erase(iter);
        }
    }

    getImpl()->m_resultsCache.clear();
    getImpl()->m_cacheID = "";
}

const char * Context::getStringVar(const char * name) const
{
    if(!name) return "";

    EnvMap::const_iterator iter = getImpl()->m_envMap.find(name);
    if(iter != getImpl()->m_envMap.end())
    {
        return iter->second.c_str();
    }

    return "";
}

int Context::getNumStringVars() const
{
    return static_cast<int>(getImpl()->m_envMap.size());
}

const char * Context::getStringVarNameByIndex(int index) const
{
    if(index < 0 || index >= static_cast<int>(getImpl()->m_envMap.size()))
        return "";

    EnvMap::const_iterator iter = getImpl()->m_envMap.begin();
    for(int count = 0; count<index; ++count) ++iter;

    return iter->first.c_str();
}

void Context::clearStringVars()
{
    getImpl()->m_envMap.clear();
}

const char * Context::resolveStringVar(const char * val) const
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    if(!val || !*val)
    {
        return "";
    }

    StringMap::const_iterator iter = getImpl()->m_resultsCache.find(val);
    if(iter != getImpl()->m_resultsCache.end())
    {
        return iter->second.c_str();
    }

    std::string resolvedval = EnvExpand(val, getImpl()->m_envMap);

    getImpl()->m_resultsCache[val] = resolvedval;
    return getImpl()->m_resultsCache[val].c_str();
}

const char * Context::resolveFileLocation(const char * filename) const
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    if(!filename || !*filename)
    {
        return "";
    }

    StringMap::const_iterator iter = getImpl()->m_resultsCache.find(filename);
    if(iter != getImpl()->m_resultsCache.end())
    {
        return iter->second.c_str();
    }

    // Attempt to load an absolute file reference
    {
    std::string expandedfullpath = EnvExpand(filename, getImpl()->m_envMap);
    if(pystring::os::path::isabs(expandedfullpath))
    {
        if(FileExists(expandedfullpath))
        {
            getImpl()->m_resultsCache[filename] = pystring::os::path::normpath(expandedfullpath);
            return getImpl()->m_resultsCache[filename].c_str();
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
    StringUtils::StringVec searchpaths;
    GetAbsoluteSearchPaths(searchpaths,
                           getImpl()->m_searchPaths,
                           getImpl()->m_workingDir,
                           getImpl()->m_envMap);

    // Loop over each path, and try to find the file
    std::ostringstream errortext;
    errortext << "The specified file reference ";
    errortext << " '" << filename << "' could not be located. ";
    errortext << "The following attempts were made: ";

    for (unsigned int i = 0; i < searchpaths.size(); ++i)
    {
        // Make an attempt to find the LUT in one of the search paths
        std::string fullpath = pystring::os::path::join(searchpaths[i], filename);
        std::string expandedfullpath = EnvExpand(fullpath, getImpl()->m_envMap);
        if(FileExists(expandedfullpath))
        {
            getImpl()->m_resultsCache[filename] = pystring::os::path::normpath(expandedfullpath);
            return getImpl()->m_resultsCache[filename].c_str();
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
void GetAbsoluteSearchPaths(StringUtils::StringVec & searchpaths,
                            const StringUtils::StringVec & pathStrings,
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
        std::string dirname = StringUtils::RightTrim(StringUtils::Trim(expanded), '/');

        if (!pystring::os::path::isabs(dirname))
        {
            dirname = pystring::os::path::join(workingDir, dirname);
        }

        searchpaths.push_back(pystring::os::path::normpath(dirname));
    }
}
} // anon.

} // namespace OCIO_NAMESPACE

