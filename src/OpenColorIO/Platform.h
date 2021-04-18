// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_PLATFORM_H
#define INCLUDED_OCIO_PLATFORM_H

// Platform-specific includes.

#if defined(_WIN32)

#define NOMINMAX 1

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Many Win32 API functions are split into two versions: ANSI-only and Unicode, marked with an A or
// W suffix respectively. Calling a documented function will actually resolve to one of these
// variants depending on whether the calling code has declared that it's Unicode-compatible or not
// (by defining UNICODE). Example excerpt from Windows header:
//
// #ifdef UNICODE
// #define SetWindowText  SetWindowTextW
// #else
// #define SetWindowText  SetWindowTextA
// #endif
//
// By defining UNICODE, we ensure that any documented function will resolve to its Unicode version.
#define UNICODE

#include <windows.h>

#endif // _WIN32


#include <fstream>
#include <string>


// Missing functions on Windows.
#ifdef _WIN32

#define sscanf sscanf_s

#endif // _WIN32


namespace OCIO_NAMESPACE
{

// TODO: Add proper endian detection using architecture / compiler mojo
//       In the meantime, hardcode to x86
#define OCIO_LITTLE_ENDIAN 1  // This is correct on x86

namespace Platform
{

// Return true if the environment variable exists.
bool Getenv(const char * name, std::string & value);

// Set a new value to a new or existing environment variable.
void Setenv(const char * name, const std::string & value);

// Remove the environment variable.
void Unsetenv(const char * name);

// Only test the presence of the envvar i.e the value does not matter.
bool isEnvPresent(const char * name);

// Case insensitive string comparison
int Strcasecmp(const char * str1, const char * str2);

// Case insensitive string comparison for the nth first characters only
int Strncasecmp(const char * str1, const char * str2, size_t n);

// Allocates memory on a specified alignment boundary. Must use
// AlignedFree to free the memory block.
// An exception is thrown if an allocation error occurs.
void* AlignedMalloc(size_t size, size_t alignment);

// Frees a block of memory that was allocated with AlignedMalloc.
void AlignedFree(void * memBlock);

// Create a temporary filename where filenameExt could be empty.
// Note: Temporary files should be at some point deleted by the OS (depending of the OS
//       and various platform specific settings). To be safe, add some code to remove
//       the file if created.
std::string CreateTempFilename(const std::string & filenameExt);

// Create an input file stream (std::ifstream) using a UTF-8 filename on any platform.
std::ifstream CreateInputFileStream(const char * filename, std::ios_base::openmode mode);

// Open an input file stream (std::ifstream) using a UTF-8 filename on any platform.
void OpenInputFileStream(std::ifstream & stream, const char * filename, std::ios_base::openmode mode);

// Create a unique hash of a file provided as a UTF-8 filename on any platform.
std::string CreateFileContentHash(const std::string &filename);

// Convert UTF-8 string to UTF-16LE.
std::wstring Utf8ToUtf16(std::string str);

// Convert UTF-16LE string to UTF-8.
std::string Utf16ToUtf8(std::wstring str);

}

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_PLATFORM_H
