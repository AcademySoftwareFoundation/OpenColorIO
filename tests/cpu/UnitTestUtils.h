// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

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
    OCIO_SHARED_PTR<CachedFile> cachedFile = tester.read(filestream, filePath);

    filestream.close();

    return DynamicPtrCast<LocalCachedFile>(cachedFile);
}

}
OCIO_NAMESPACE_EXIT


#endif // OCIO_UNIT_TEST

#endif // INCLUDED_OCIO_UNITTESTUTILS_H
