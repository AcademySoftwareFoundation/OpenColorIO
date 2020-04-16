// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_PATHUTILS_H
#define INCLUDED_OCIO_PATHUTILS_H

#include <OpenColorIO/OpenColorIO.h>

#include <map>

namespace OCIO_NAMESPACE
{
// This is not currently included in pystring, but we need it
// So let's define it locally for now

std::string AbsPath(const std::string & path);

// The EnvMap is ordered by the length of the keys (long -> short). This
// is so that recursive string expansion will deal with similar prefixed
// keys as expected.
// ie. '$TEST_$TESTING_$TE' will expand in this order '2 1 3'
template <class T>
struct EnvMapKey : std::binary_function <T, T, bool>
{
    bool
    operator() (const T &x, const T &y) const
    {
        // If the lengths are unequal, sort by length
        if(x.length() != y.length())
        {
            return (x.length() > y.length());
        }
        // Otherwise, use the standard string sort comparison
        else
        {
            return (x<y);
        }
    }
};
typedef std::map< std::string, std::string, EnvMapKey< std::string > > EnvMap;

// Get map of current env key = value, or update the existing entries
void LoadEnvironment(EnvMap & map, bool update = false);

// Expand a string with $VAR, ${VAR} or %VAR% with the keys passed
// in the EnvMap.
std::string EnvExpand(const std::string & str, const EnvMap & map);

// Check if a file exists
bool FileExists(const std::string & filename);

// Get a fast hash for a file, without reading all the contents.
// Currently, this checks the mtime and the inode number.
std::string GetFastFileHash(const std::string & filename);

void ClearPathCaches();

int ParseColorSpaceFromString(const Config & config, const char * str);

} // namespace OCIO_NAMESPACE

#endif
