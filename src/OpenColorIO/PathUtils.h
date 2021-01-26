// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_PATHUTILS_H
#define INCLUDED_OCIO_PATHUTILS_H

#include <OpenColorIO/OpenColorIO.h>



namespace OCIO_NAMESPACE
{

// This is not currently included in pystring, but we need it
// So let's define it locally for now

std::string AbsPath(const std::string & path);

// Check if a file exists
bool FileExists(const std::string & filename);

// Get a fast hash for a file, without reading all the contents.
// Currently, this checks the mtime and the inode number.
std::string GetFastFileHash(const std::string & filename);

void ClearPathCaches();

// Works on active and inactive color spaces name and aliases.
int ParseColorSpaceFromString(const Config & config, const char * str);

} // namespace OCIO_NAMESPACE

#endif
