// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_UNITTESTUTILS_H
#define INCLUDED_OCIO_UNITTESTUTILS_H


#include <fstream>

#ifdef __has_include
# if __has_include(<version>)
#   include <version>
# endif
#endif

#include <pystring.h>

#include <OpenColorIO/OpenColorIO.h>

#include "MathUtils.h"
#include "Op.h"
#include "Platform.h"
#include "CPUInfoConfig.h"

namespace OCIO_NAMESPACE
{

const std::string & GetTestFilesDir();

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
    const std::string filePath(GetTestFilesDir() + "/" + fileName);

    // Open the filePath
    std::ifstream filestream;
    Platform::OpenInputFileStream(filestream, filePath.c_str(), mode);

    if (!filestream.is_open())
    {
        throw Exception("Error opening test file.");
    }

    std::string root, extension, name;
    pystring::os::path::splitext(root, extension, filePath);

    // Read file
    LocalFileFormat tester;
    OCIO_SHARED_PTR<CachedFile> cachedFile = tester.read(filestream, filePath, INTERP_DEFAULT);

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
    const T div = (expected > 0) ?
        ((expected < minExpected) ? minExpected : expected) :
        ((-expected < minExpected) ? minExpected : -expected);

    return (
        ((value > expected) ? value - expected : expected - value)
        / div) <= eps;
}

// C++20 introduces new strongly typed, UTF-8 based, char8_t and u8string types
// which are not implicitly convertible to char and std::string respectively.
// Here we simply choose to ignore these new types for unit tests while the
// core API is not explicitly compatible with such types.
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1423r2.html
#if defined(__cpp_lib_char8_t)
#   define U8(x) reinterpret_cast<const char *>(u8##x)
#else
#   define U8(x) u8##x
#endif

struct EnvironmentVariableGuard
{
    EnvironmentVariableGuard(const std::string & name, const std::string & value) : m_name(name)
    {
        if (!name.empty())
        {
            Platform::Setenv(name.c_str(), value);
        }
    }

    EnvironmentVariableGuard(const std::string & name) : m_name(name)
    {
    }

    ~EnvironmentVariableGuard()
    {
        if (!m_name.empty())
        {
            Platform::Unsetenv(m_name.c_str());
        }
    }

    const std::string m_name;
};

/**
 * \brief Create a Temporary Directory
 * 
 * \param name Name of the directory
 * \return Full path to the directory
 */
std::string CreateTemporaryDirectory(const std::string & name);

/**
 * \brief Remove the directory specified in the path.
 * 
 * \param directoryPath Path to the directory
 */
void RemoveTemporaryDirectory(const std::string & directoryPath);

}
// namespace OCIO_NAMESPACE


#endif // INCLUDED_OCIO_UNITTESTUTILS_H
