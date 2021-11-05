// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <stdio.h>
#include <fstream>

#if !defined(_WIN32)
#include <sstream>
#include <stdlib.h>
#include <time.h>
#endif

#include <OpenColorIO/OpenColorIO.h>

#include "GPUHelpers.h"

namespace OCIO = OCIO_NAMESPACE;


// TODO: Make OCIO::Platform::CreateTempFilename() public so it could be used here.

std::string createTempFile(const std::string& fileExt, const std::string& fileContent)
{
    // Note: because of security issue, tmpnam could not be used

    std::string filename;

#ifdef _WIN32

    char tmpFilename[L_tmpnam];
    if(tmpnam_s(tmpFilename))
    {
        throw OCIO::Exception("Could not create a temporary file");
    }

    // Note that when a file name is pre-pended with a backslash and no path information, such as \fname21, this 
    // indicates that the name is valid for the current working directory.
    filename = tmpFilename[0] == '\\' ? tmpFilename + 1 : tmpFilename;
    filename += fileExt;

#else

    std::stringstream ss;
    ss << "/tmp/ocio";
    ss << std::rand();
    ss << fileExt;

    filename = ss.str();

#endif

    std::ofstream ofs(filename.c_str(), std::ios_base::out);
    ofs << fileContent;
    ofs.close();

    return filename;
}
