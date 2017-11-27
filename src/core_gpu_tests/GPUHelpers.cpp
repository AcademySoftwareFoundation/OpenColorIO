/*
    Made by Autodesk Inc. under the terms of the OpenColorIO BSD 3 Clause License
*/

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "GPUHelpers.h"


#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS) || defined(_MSC_VER)
#undef WINDOWS
#define WINDOWS
#endif



#include <stdio.h>
#include <fstream>

#if !defined(WINDOWS)
#include <sstream>
#include <stdlib.h>
#include <time.h>
#endif


std::string createTempFile(const std::string& fileExt, const std::string& fileContent)
{
    std::string filename;

#ifdef WINDOWS

    char tmpFilename[L_tmpnam];
    if(tmpnam_s(tmpFilename))
    {
        throw OCIO::Exception("Could not create a temporary file");
    }

    filename = tmpFilename;
    filename += fileExt;
    
#else

    // Note: because of security issue, tmpnam could not be used

    std::stringstream ss;
    time_t t = time(NULL);
    ss << rand_r((unsigned int*)&t);
    std::string str = "/tmp/ocio";
    str += ss.str();
    str += fileExt;

    filename = str;

#endif

    std::ofstream ofs(filename.c_str(), std::ios_base::out);
    ofs << fileContent;
    ofs.close();

    return filename;
}
