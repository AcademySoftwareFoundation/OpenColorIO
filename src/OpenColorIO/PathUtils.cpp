// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <iostream>
#include <map>
#include <sys/stat.h>

#include <OpenColorIO/OpenColorIO.h>

#include "Mutex.h"
#include "PathUtils.h"
#include "pystring/pystring.h"
#include "utils/StringUtils.h"

#if !defined(_WIN32)
#include <sys/param.h>
#include <unistd.h>
#else
#include <direct.h>
#define MAXPATHLEN 4096
#endif


namespace OCIO_NAMESPACE
{
namespace
{
// Here is the explanation of the stat() method:
// https://pubs.opengroup.org/onlinepubs/009695299/basedefs/sys/stat.h.html
// "The st_ino and st_dev fields taken together uniquely identify the file within the system."
//
// However there are limitations to the stat() support on some Windows file systems:
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/stat-functions?redirectedfrom=MSDN&view=vs-2019
// "The inode, and therefore st_ino, has no meaning in the FAT, HPFS, or NTFS file systems."

// That's the default hash method implementation to compute a hash key based on a file content.
std::string DefaultComputeHash(const std::string &filename)
{
    struct stat fileInfo;
    if (stat(filename.c_str(), &fileInfo) == 0)
    {
        // Treat the st_dev (i.e. device) + st_ino (i.e. inode) as a proxy for the contents.

        std::ostringstream fasthash;
        fasthash << fileInfo.st_dev << ":";
#ifdef _WIN32
        // TODO: The hard-linked files are then not correctly supported on Windows platforms.
        fasthash << std::hash<std::string>{}(filename);
#else
        fasthash << fileInfo.st_ino;
#endif
        return fasthash.str();
    }

    return "";
}

// The global variable holds the hash function to use.
// It could be changed using SetComputeHashFunction() to customize the implementation.
ComputeHashFunction g_hashFunction = DefaultComputeHash;

// We mutex both the main map and each item individually, so that
// the potentially slow stat calls dont block other lookups to already
// existing items. (The stat calls will block other lookups on the
// *same* file though).

struct FileHashResult
{
    Mutex mutex;
    std::string hash;
    bool ready { false };
};

typedef OCIO_SHARED_PTR<FileHashResult> FileHashResultPtr;
typedef std::map<std::string, FileHashResultPtr> FileCacheMap;

FileCacheMap g_fastFileHashCache;
Mutex g_fastFileHashCache_mutex;
}

void SetComputeHashFunction(ComputeHashFunction hashFunction)
{
    g_hashFunction = hashFunction;
}

void ResetComputeHashFunction()
{
    g_hashFunction = DefaultComputeHash;
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
            fileHashResultPtr = std::make_shared<FileHashResult>();
            g_fastFileHashCache[filename] = fileHashResultPtr;
        }
    }

    std::string hash;
    {
        AutoMutex lock(fileHashResultPtr->mutex);
        if(!fileHashResultPtr->ready)
        {
            // NB: OCIO does not attempt to detect if files have changed and caused the cache to
            // become stale.
            fileHashResultPtr->ready = true;
            fileHashResultPtr->hash = g_hashFunction(filename);
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
void AdjustRightmost(const std::string & name, int index, int & colorspacePos,
                     int & rightMostColorPos, std::string & rightMostColorspace,
                     int & rightMostColorSpaceIndex)
{
    // If we have found a match, move the pointer over to the right end
    // of the substring.  This will allow us to find the longest name
    // that matches the rightmost colorspace
    colorspacePos += (int)name.size();

    if ((colorspacePos > rightMostColorPos) ||
        ((colorspacePos == rightMostColorPos) && (name.size() > rightMostColorspace.size()))
        )
    {
        rightMostColorPos = colorspacePos;
        rightMostColorspace = name;
        rightMostColorSpaceIndex = index;
    }
}
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
        const std::string csname = StringUtils::Lower(
            config.getColorSpaceNameByIndex(SEARCH_REFERENCE_SPACE_ALL, COLORSPACE_ALL, i));

        // find right-most extension matched in filename
        int colorspacePos = (int)StringUtils::ReverseFind(fullstr, csname);
        if (colorspacePos >= 0)
        {
            AdjustRightmost(csname, i, colorspacePos,
                            rightMostColorPos, rightMostColorspace, rightMostColorSpaceIndex);
        }

        auto cs = config.getColorSpace(csname.c_str());
        const size_t numAliases = cs->getNumAliases();
        for (size_t j = 0; j < numAliases; ++j)
        {
            const std::string aliasname = StringUtils::Lower(cs->getAlias(j));
            int colorspacePos = (int)StringUtils::ReverseFind(fullstr, aliasname);
            if (colorspacePos >= 0)
            {
                AdjustRightmost(aliasname, i, colorspacePos,
                                rightMostColorPos, rightMostColorspace, rightMostColorSpaceIndex);
            }
        }
    }
    return rightMostColorSpaceIndex;
}

} // namespace OCIO_NAMESPACE

