// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdlib>
#include <errno.h>
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
#include "utils/StringUtils.h"

#if !defined(_WIN32)
#include <sys/param.h>
#else
#include <direct.h>
#define MAXPATHLEN 4096
#endif

#if defined(__APPLE__) && !defined(__IPHONE__)
#include <crt_externs.h> // _NSGetEnviron()
#include <unistd.h>
#elif !defined(_WIN32)
#include <unistd.h>
extern char **environ;
#endif

namespace OCIO_NAMESPACE
{
namespace
{
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

// We mutex both the main map and each item individually, so that
// the potentially slow stat calls dont block other lookups to already
// existing items. (The stat calls will block other lookups on the
// *same* file though).

struct FileHashResult
{
    Mutex mutex;
    std::string hash;
    bool ready;

    FileHashResult():
        ready(false)
    {}
};

typedef OCIO_SHARED_PTR<FileHashResult> FileHashResultPtr;
typedef std::map<std::string, FileHashResultPtr> FileCacheMap;

FileCacheMap g_fastFileHashCache;
Mutex g_fastFileHashCache_mutex;
}

std::string GetFastFileHash(const std::string & filename)
{
    FileHashResultPtr fileHashResultPtr;
    {
        AutoMutex lock(g_fastFileHashCache_mutex);
        FileCacheMap::iterator iter = g_fastFileHashCache.find(filename);
        if(iter != g_fastFileHashCache.end())
        {
            fileHashResultPtr = iter->second;
        }
        else
        {
            fileHashResultPtr = FileHashResultPtr(new FileHashResult);
            g_fastFileHashCache[filename] = fileHashResultPtr;
        }
    }

    std::string hash;
    {
        AutoMutex lock(fileHashResultPtr->mutex);
        if(!fileHashResultPtr->ready)
        {
            fileHashResultPtr->ready = true;
            fileHashResultPtr->hash = ComputeHash(filename);
        }

        hash = fileHashResultPtr->hash;
    }

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

namespace
{
std::string GetCwd()
{
#ifdef _WIN32
    char path[MAXPATHLEN];
    _getcwd(path, MAXPATHLEN);
    return path;
#else
    std::vector<char> current_dir;
#ifdef PATH_MAX
    current_dir.resize(PATH_MAX);
#else
    current_dir.resize(1024);
#endif
    while (::getcwd(&current_dir[0], current_dir.size()) == NULL && errno == ERANGE) {
        current_dir.resize(current_dir.size() + 1024);
    }

    return std::string(&current_dir[0]);
#endif
}
}

std::string AbsPath(const std::string & path)
{
    std::string p = path;
    if(!pystring::os::path::isabs(p)) p = pystring::os::path::join(GetCwd(), p);
    return pystring::os::path::normpath(p);
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
}

void LoadEnvironment(EnvMap & map, bool update)
{
    for (char **env = GetEnviron(); *env != NULL; ++env)
    {

        // split environment up into std::map[name] = value
        std::string env_str = (char*)*env;
        int pos = static_cast<int>(env_str.find_first_of('='));
        std::string name = env_str.substr(0, pos);
        std::string value = env_str.substr(pos+1, env_str.length());

        if(update)
        {
            // update existing key:values that match
            EnvMap::iterator iter = map.find(name);
            if(iter != map.end()) iter->second = value;
        }
        else
        {
            map.insert(EnvMap::value_type(name, value));
        }
    }
}

std::string EnvExpand(const std::string & str, const EnvMap & map)
{
    // Early exit if no magic characters are found.
    if(StringUtils::Find(str, "$") == std::string::npos 
        && StringUtils::Find(str, "%") == std::string::npos)
    {
        return str;
    }

    std::string orig = str;
    std::string newstr = str;

    // This walks through the envmap in key order,
    // from longest to shortest to handle envvars which are
    // substrings.
    // ie. '$TEST_$TESTING_$TE' will expand in this order '2 1 3'

    for (EnvMap::const_iterator iter = map.begin();
            iter != map.end(); ++iter)
    {
        StringUtils::ReplaceInPlace(newstr, ("${"+iter->first+"}"), iter->second);
        StringUtils::ReplaceInPlace(newstr, ("$"+iter->first),      iter->second);
        StringUtils::ReplaceInPlace(newstr, ("%"+iter->first+"%"),  iter->second);
    }

    // recursively call till string doesn't expand anymore
    if(newstr != orig)
    {
        return EnvExpand(newstr, map);
    }

    return orig;
}

int ParseColorSpaceFromString(const Config & config, const char * str)
{
    if (!str) return -1;

    // Search the entire filePath, including directory name (if provided)
    // convert the filename to lowercase.
    const std::string fullstr = StringUtils::Lower(std::string(str));

    // See if it matches a LUT name.
    // This is the position of the RIGHT end of the colorspace substring,
    // not the left
    int rightMostColorPos = -1;
    std::string rightMostColorspace = "";
    int rightMostColorSpaceIndex = -1;

    // Find the right-most occcurance within the string for each colorspace.
    for (int i = 0; i < config.getNumColorSpaces(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL); ++i)
    {
        const std::string csname = StringUtils::Lower(config.getColorSpaceNameByIndex(i));

        // find right-most extension matched in filename
        int colorspacePos = (int)StringUtils::ReverseFind(fullstr, csname);
        if (colorspacePos < 0)
            continue;

        // If we have found a match, move the pointer over to the right end
        // of the substring.  This will allow us to find the longest name
        // that matches the rightmost colorspace
        colorspacePos += (int)csname.size();

        if ((colorspacePos > rightMostColorPos) ||
            ((colorspacePos == rightMostColorPos) && (csname.size() > rightMostColorspace.size()))
            )
        {
            rightMostColorPos = colorspacePos;
            rightMostColorspace = csname;
            rightMostColorSpaceIndex = i;
        }
    }
    return rightMostColorSpaceIndex;
}

} // namespace OCIO_NAMESPACE

