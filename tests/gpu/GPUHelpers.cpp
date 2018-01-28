/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
