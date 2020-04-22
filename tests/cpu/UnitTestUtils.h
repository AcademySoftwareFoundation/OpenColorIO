// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_UNITTESTUTILS_H
#define INCLUDED_OCIO_UNITTESTUTILS_H


#include <fstream>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "Op.h"
#include "pystring/pystring.h"

namespace OCIO_NAMESPACE
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

// Create a FileTransform.
FileTransformRcPtr CreateFileTransform(const std::string & fileName);

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

// Relative comparison: check if the difference between value and expected
// relative to (divided by) expected does not exceed the eps.  A minimum
// expected value is used to limit the scaling of the difference and
// avoid large relative differences for small numbers.
template<typename T>
inline bool EqualWithSafeRelError(T value,
                                  T expected,
                                  T eps,
                                  T minExpected)
{
    // If value and expected are infinity, return true.
    if (value == expected) return true;
    if (IsNan(value) && IsNan(expected)) return true;
    const float div = (expected > 0) ?
        ((expected < minExpected) ? minExpected : expected) :
        ((-expected < minExpected) ? minExpected : -expected);

    return (
        ((value > expected) ? value - expected : expected - value)
        / div) <= eps;
}


}
// namespace OCIO_NAMESPACE


#endif // INCLUDED_OCIO_UNITTESTUTILS_H
