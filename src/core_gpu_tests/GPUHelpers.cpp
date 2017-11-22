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


#include <fstream>

#ifdef WINDOWS
#include <stdio.h>
#else
#include <stdlib.h>
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
    
    std::ofstream ofs(filename, std::ios_base::out);
    ofs << fileContent;
    ofs.close();

#else

    char tmpFilename[] = "/tmp/ocioTmpFile-XXXXXX";
    f = mkstemp(tmpFilename);

    if(f==-1)
    {
        throw OCIO::Exception(strerror(errno));
    }

    if(-1==write(f, fileContent.c_str(), fileContent.size())
    {
        throw OCIO::Exception(strerror(errno));
    }

    close(f);

    filename = tmpFilename;

#endif

    return filename;
}