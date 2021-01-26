// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include "ContextVariableUtils.h"
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
                            const EnvMap & map,
                            UsedEnvs & envs);
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
    EnvironmentMode m_envmode = ENV_ENVIRONMENT_LOAD_PREDEFINED;
    EnvMap m_envMap;

    mutable std::string m_cacheID;

    // Cache for resolved strings containing context variables.
    using ResolvedStringCache = std::map<std::string, std::pair<std::string, UsedEnvs>>;
    mutable ResolvedStringCache m_resultsStringCache;
    // Cache for resolved & expanded file paths containing context variables.
    mutable ResolvedStringCache m_resultsFilepathCache;
    mutable Mutex m_resultsCacheMutex;

    Impl() = default;
    ~Impl() = default;

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

            m_resultsStringCache   = rhs.m_resultsStringCache;
            m_resultsFilepathCache = rhs.m_resultsFilepathCache;

            m_cacheID = rhs.m_cacheID;
        }
        return *this;
    }

    // Resolve all context variables from an arbitrary string.
    const char * resolveStringVar(const char * string, ContextRcPtr & usedContextVars) const noexcept
    {
        if (!string || !*string)
        {
            return "";
        }

        ResolvedStringCache::const_iterator iter = m_resultsStringCache.find(string);
        if (iter != m_resultsStringCache.end())
        {
            if (usedContextVars)
            {
                // Collect the used context variables.
                for (const auto & var : iter->second.second)
                {
                    usedContextVars->setStringVar(var.first.c_str(), var.second.c_str());
                }
            }

            return iter->second.first.c_str();
        }

        // Search some context variables to replace.
        UsedEnvs envs;
        const std::string resolvedString = ResolveContextVariables(string, m_envMap, envs);
        m_resultsStringCache[string] = std::make_pair(resolvedString, envs);

        if (usedContextVars)
        {
            // Record all the used context variables.
            for (const auto & var : envs)
            {
                usedContextVars->setStringVar(var.first.c_str(), var.second.c_str());
            }
        }

        // Return the resolved string.
        return m_resultsStringCache[string].first.c_str();
    }

    void clearCaches()
    {
        m_resultsStringCache.clear();
        m_resultsFilepathCache.clear();
        m_cacheID.clear();     
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
    // TODO: Do nothing if the path is already present in the list of paths. The important aspect
    // is to preserve the cache content.

    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    getImpl()->m_searchPaths = StringUtils::Split(path ? path : "", ':');
    getImpl()->m_searchPath  = (path ? path : "");
    getImpl()->clearCaches();
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
    getImpl()->clearCaches();
}

void Context::addSearchPath(const char * path)
{
    // TODO: Do nothing if the path is already present in the list of paths. The important aspect
    // is to preserve the cache content.

    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    if (path && *path)
    {
        getImpl()->m_searchPaths.emplace_back(path ? path : "");
        getImpl()->clearCaches();

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
    getImpl()->clearCaches();
}

const char * Context::getWorkingDir() const
{
    return getImpl()->m_workingDir.c_str();
}

void Context::setEnvironmentMode(EnvironmentMode mode) noexcept
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    getImpl()->m_envmode = mode;

    getImpl()->clearCaches();
}

EnvironmentMode Context::getEnvironmentMode() const noexcept
{
    return getImpl()->m_envmode;
}

void Context::loadEnvironment() noexcept
{
    bool update = (getImpl()->m_envmode == ENV_ENVIRONMENT_LOAD_ALL) ? false : true;
    LoadEnvironment(getImpl()->m_envMap, update);

    AutoMutex lock(getImpl()->m_resultsCacheMutex);
    getImpl()->clearCaches();
}

void Context::setStringVar(const char * name, const char * value) noexcept
{
    if (!name || !*name)
    {
        return;
    }

    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    // Set the value if specified.
    if (value)
    {
        EnvMap::iterator iter = getImpl()->m_envMap.find(name);
        if (iter != getImpl()->m_envMap.end())
        {
            if (0 != strcmp(iter->second.c_str(), value))
            {
                iter->second = value;
            }
            else
            {
                // Do not flush the cache because nothing changed.
                return;
            }
        }
        else
        {
            getImpl()->m_envMap[name] = value;
        }
    }
    // If a null value is specified, erase it.
    else
    {
        EnvMap::const_iterator iter = getImpl()->m_envMap.find(name);
        if (iter != getImpl()->m_envMap.end())
        {
            getImpl()->m_envMap.erase(iter);
        }
    }

    getImpl()->clearCaches();
}

const char * Context::getStringVar(const char * name) const noexcept
{
    if (!name || !*name)
    {
        return "";
    }

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

const char * Context::getStringVarByIndex(int index) const
{
    if(index < 0 || index >= static_cast<int>(getImpl()->m_envMap.size()))
        return "";

    EnvMap::const_iterator iter = getImpl()->m_envMap.begin();
    for(int count = 0; count<index; ++count) ++iter;

    return iter->second.c_str();
}

void Context::addStringVars(const ConstContextRcPtr & ctx) noexcept
{
    for (const auto & iter : ctx->getImpl()->m_envMap)
    {
        setStringVar(iter.first.c_str(), iter.second.c_str());
    }
}

void Context::clearStringVars()
{
    getImpl()->m_envMap.clear();
}

const char * Context::resolveStringVar(const char * string) const  noexcept
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    ContextRcPtr usedContextVars;

    return getImpl()->resolveStringVar(string, usedContextVars);
}

const char * Context::resolveStringVar(const char * string, ContextRcPtr & usedContextVars) const noexcept
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    return getImpl()->resolveStringVar(string, usedContextVars);
}

const char * Context::resolveFileLocation(const char * filename) const
{
    ContextRcPtr usedContextVars;

    return resolveFileLocation(filename, usedContextVars);
}

// TODO: Currently usedContextVars includes (for non-absolute filenames) all context vars used in
// the search_path, regardless of whether they were needed to resolve the file location. So 
// usedContextVars may not be empty, even if no context vars are needed. Ideally, usedContextVars
// would only contain the vars needed for the specific filename.
const char * Context::resolveFileLocation(const char * filename, ContextRcPtr & usedContextVars) const
{
    AutoMutex lock(getImpl()->m_resultsCacheMutex);

    // Resolve the context variables and collect the used context variables related to the filename
    // only i.e. not including the ones (directly or indirectly) from the search_paths.
    const std::string resolvedFilename = getImpl()->resolveStringVar(filename, usedContextVars);

    // Search for existing resolved filepath.
    Impl::ResolvedStringCache::const_iterator iter
        = getImpl()->m_resultsFilepathCache.find(resolvedFilename);

    if (iter != getImpl()->m_resultsFilepathCache.end())
    {
        if (usedContextVars)
        {
            // Collect all the used context variables from the search_paths if any.
            const UsedEnvs envs = iter->second.second;
            for (const auto & var : envs)
            {
                usedContextVars->setStringVar(var.first.c_str(), var.second.c_str());
            }
        }

        return iter->second.first.c_str();
    }

    // If the file reference is absolute, check if the file exists (independent of the search paths).
    if(pystring::os::path::isabs(resolvedFilename))
    {
        if(FileExists(resolvedFilename))
        {
            // That's already an absolute path so no extra context variables are present.
            UsedEnvs envs;

            // Note that the filepath cache key is the 'resolvedFilename'.

            getImpl()->m_resultsFilepathCache[resolvedFilename] 
                = std::make_pair(pystring::os::path::normpath(resolvedFilename), envs);

            return getImpl()->m_resultsFilepathCache[resolvedFilename].first.c_str();
        }

        std::ostringstream errortext;
        errortext << "The specified absolute file reference ";
        errortext << "'" << resolvedFilename << "' could not be located.";
        throw ExceptionMissingFile(errortext.str().c_str());
    }

    // As that's a relative path search for the right root path using search path(s) or working path.

    // The search_paths could contain some context variables.
    UsedEnvs envs;

    // TODO: Used context variables from GetAbsoluteSearchPaths() are from all the search_paths 
    // of the config i.e. it does not mean that all of them are used to resolve a FileTransform
    // for example.

    // Load a relative file reference
    // Prep and resolve the search path vector
    // TODO: Cache this prepped vector?
    StringUtils::StringVec searchpaths;
    GetAbsoluteSearchPaths(searchpaths,
                           getImpl()->m_searchPaths,
                           getImpl()->m_workingDir,
                           getImpl()->m_envMap,
                           envs);

    // Loop over each path, and try to find the file
    std::ostringstream errortext;
    errortext << "The specified file reference ";
    errortext << "'" << filename << "' could not be located. ";
    errortext << "The following attempts were made: ";

    for (unsigned int i = 0; i < searchpaths.size(); ++i)
    {
        // Make an attempt to find the LUT in one of the search paths.
        const std::string resolvedfullpath = pystring::os::path::join(searchpaths[i], resolvedFilename);

        if (!ContainsContextVariables(resolvedfullpath) && FileExists(resolvedfullpath))
        {
            // Collect all the used context variables.
            if (usedContextVars)
            {
                for (const auto & iter : envs)
                {
                    usedContextVars->setStringVar(iter.first.c_str(), iter.second.c_str());
                }
            }

            // Add to the cache.
            getImpl()->m_resultsFilepathCache[resolvedFilename]
                = std::make_pair(pystring::os::path::normpath(resolvedfullpath), envs);

            return getImpl()->m_resultsFilepathCache[resolvedFilename].first.c_str();
        }

        if(i!=0) errortext << " : ";
        errortext << "'" << resolvedfullpath << "'";
    }
    errortext << ".";

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
        os << "\n    " << key << ": " << context.getStringVar(key);
    }
    os << ">";
    return os;
}

namespace
{
void GetAbsoluteSearchPaths(StringUtils::StringVec & searchpaths,
                            const StringUtils::StringVec & pathStrings,
                            const std::string & workingDir,
                            const EnvMap & map,
                            UsedEnvs & envs)
{
    if(pathStrings.empty())
    {
        searchpaths.push_back(workingDir);
        return;
    }

    // TODO: Improve the algorithm to collect context variables per search_path.

    for (unsigned int i = 0; i < pathStrings.size(); ++i)
    {
        // Resolve variables in case the expansion adds slashes
        const std::string resolved = ResolveContextVariables(pathStrings[i], map, envs);

        // Remove trailing "/", and spaces
        std::string dirname = StringUtils::RightTrim(StringUtils::Trim(resolved), '/');

        if (!pystring::os::path::isabs(dirname))
        {
            dirname = pystring::os::path::join(workingDir, dirname);
        }

        searchpaths.push_back(pystring::os::path::normpath(dirname));
    }
}
} // anon.

} // namespace OCIO_NAMESPACE

