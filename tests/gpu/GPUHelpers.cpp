// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <stdio.h>
#include <fstream>

#if !defined(_WIN32)
#include <sstream>
#include <cstdlib>
#include <time.h>
#include <cerrno>
#endif

#include <OpenColorIO/OpenColorIO.h>

#include "GPUHelpers.h"

namespace OCIO = OCIO_NAMESPACE;


// TODO: Make OCIO::Platform::CreateTempFile() public so it could be used here.

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

    filename = tmpFilename[0] == '\\' ? tmpFilename + 1 : tmpFilename;
    filename += fileExt;

#else

    filename = "/tmp/ocio_XXXXXX" + fileExt;
    
    if(mkstemps(filename.data(), fileExt.size()) == -1)
    {
        throw std::system_error(errno, std::system_category(), "Could not create a temporary file.");
    }

#endif

    std::ofstream ofs(filename.c_str(), std::ios_base::out);
    ofs << fileContent;
    ofs.close();

    return filename;
}
