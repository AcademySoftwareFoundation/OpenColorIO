/*
Copyright (c) 2019 Autodesk Inc., et al.
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

#ifndef INCLUDED_OCIO_UNITTESTUTILS_H
#define INCLUDED_OCIO_UNITTESTUTILS_H

#ifdef OCIO_UNIT_TEST

#include <fstream>

#include <OpenColorIO/OpenColorIO.h>
#include "Op.h"
#include "pystring/pystring.h"

OCIO_NAMESPACE_ENTER
{

const char * getTestFilesDir();

// Special test function that copies the implementation of FileTransform
// in order to be able to access ops from a file path. fileOps will not be
// finalized and will thus contain NoOps including FileNoOps.
// context can be used to control working directory, search path etc.
void BuildOpsTest(OpRcPtrVec & fileOps,
                  const std::string & fileName,
                  ContextRcPtr & context,
                  TransformDirection dir);

// Create processor for a given file.
ConstProcessorRcPtr GetFileTransformProcessor(const std::string & fileName);

class CachedFile;

template <class LocalFileFormat, class LocalCachedFile>
OCIO_SHARED_PTR<LocalCachedFile> LoadTestFile(
    const std::string & fileName, std::ios_base::openmode mode)
{
    const std::string filePath(std::string(getTestFilesDir()) + "/"
                               + fileName);

    // Open the filePath
    std::ifstream filestream;
    filestream.open(filePath.c_str(), mode);

    if (!filestream.is_open())
    {
        throw Exception("Error opening test file.");
    }

    std::string root, extension, name;
    pystring::os::path::splitext(root, extension, filePath);

    // Read file
    LocalFileFormat tester;
    OCIO_SHARED_PTR<CachedFile> cachedFile = tester.Read(filestream, filePath);

    filestream.close();

    return DynamicPtrCast<LocalCachedFile>(cachedFile);
}

}
OCIO_NAMESPACE_EXIT


#endif // OCIO_UNIT_TEST

#endif // INCLUDED_OCIO_UNITTESTUTILS_H
