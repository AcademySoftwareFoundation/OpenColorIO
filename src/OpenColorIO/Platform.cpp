// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <codecvt>
#include <locale>
#include <random>
#include <sstream>
#include <sys/stat.h>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Platform.h"

#ifndef _WIN32
#include <strings.h>
#endif


namespace OCIO_NAMESPACE
{

const char * GetEnvVariable(const char * name)
{
    static std::string value;
    Platform::Getenv(name, value);
    return value.c_str();
}

void SetEnvVariable(const char * name, const char * value)
{
    Platform::Setenv(name, value ? value : "");
}

void UnsetEnvVariable(const char * name)
{
    Platform::Unsetenv(name);
}

bool IsEnvVariablePresent(const char * name)
{
    return Platform::isEnvPresent(name);
}

namespace Platform
{

bool Getenv(const char * name, std::string & value)
{
    if (!name || !*name)
    {
        return false;
    }

#if defined(_WIN32)
    // Define working strings, converting to UTF-16 if necessary
#ifdef UNICODE
    std::wstring name_str = Utf8ToUtf16(name);
    std::wstring value_str;
#else
    std::string name_str = name;
    std::string value_str;
#endif

    if(uint32_t size = GetEnvironmentVariable(name_str.c_str(), nullptr, 0))
    {
        value_str.resize(size);

        GetEnvironmentVariable(name_str.c_str(), &value_str[0], size);

        // GetEnvironmentVariable is designed for raw pointer strings and therefore requires that
        // the destination buffer be long enough to place a null terminator at the end of it. Since
        // we're using std::wstrings here, the null terminator is unnecessary (and causes false
        // negatives in unit tests since the extra character makes it "non-equal" to normally
        // defined std::wstrings). Therefore, we pop the last character off (the null terminator)
        // to ensure that the string conforms to expectations.
        value_str.pop_back();

        // Return value, converting to UTF-8 if necessary
#ifdef UNICODE
        value = Utf16ToUtf8(value_str);
#else
        value = value_str;
#endif
        return true;
    }
    else
    {
        value.clear();
        return false;
    }
#else
    const char * val = ::getenv(name);
    value = (val && *val) ? val : "";
    return val; // Returns true if the env. variable exists but empty.
#endif
}

void Setenv(const char * name, const std::string & value)
{
    if (!name || !*name)
    {
        return;
    }

    // Note that the Windows _putenv_s() removes the env. variable if the value is empty. But
    // the Linux ::setenv() sets the env. variable to empty if the value is empty i.e. it still 
    // exists. To avoid the ambiguity, use Unsetenv() when the env. variable removal if needed.

#ifdef _WIN32

#ifdef UNICODE
    _wputenv_s(Utf8ToUtf16(name).c_str(), Utf8ToUtf16(value).c_str());
#else
    _putenv_s(name, value.c_str());
#endif

#else
    ::setenv(name, value.c_str(), 1);
#endif
}

void Unsetenv(const char * name)
{
    if (!name || !*name)
    {
        return;
    }

#ifdef _WIN32

#ifdef UNICODE
    // Note that the Windows _putenv_s() removes the env. variable if the value is empty.
    _wputenv_s(Utf8ToUtf16(name).c_str(), L"");
#else
    _putenv_s(name, "");
#endif

#else
    ::unsetenv(name);
#endif
}

bool isEnvPresent(const char * name)
{
    if (!name || !*name)
    {
        return false;
    }

    std::string value;
    return Getenv(name, value);
}

int Strcasecmp(const char * str1, const char * str2)
{
    if (!str1 || !str2) 
    {
        throw Exception("String pointer for comparison must not be null.");
    }

#ifdef _WIN32
    return ::_stricmp(str1, str2);
#else
    return ::strcasecmp(str1, str2);
#endif
}

int Strncasecmp(const char * str1, const char * str2, size_t n)
{
    if (!str1 || !str2) 
    {
        throw Exception("String pointer for comparison must not be null.");
    }

#ifdef _WIN32
    return ::_strnicmp(str1, str2, n);
#else
    return ::strncasecmp(str1, str2, n);
#endif
}

void * AlignedMalloc(size_t size, size_t alignment)
{
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* memBlock = 0x0;
    if (!posix_memalign(&memBlock, alignment, size)) return memBlock;
    return 0x0;
#endif
}

void AlignedFree(void* memBlock)
{
#ifdef _WIN32
    _aligned_free(memBlock);
#else
    free(memBlock);
#endif
}

namespace
{

int GenerateRandomNumber()
{
    // Note: Read https://isocpp.org/files/papers/n3551.pdf for details.

    static std::mt19937 engine{};
    static std::uniform_int_distribution<> dist{};

    return dist(engine);
}

}

std::string CreateTempFilename(const std::string & filenameExt)
{
    std::string filename;

#ifdef _WIN32

    // Note: Because of security issue, tmpnam could not be used.

    // Note 2: MinGW doesn't define L_tmpnam_s, we need to patch it in.
    // https://www.mail-archive.com/mingw-w64-public@lists.sourceforge.net/msg17360.html
#if !defined(L_tmpnam_s)
#define L_tmpnam_s L_tmpnam
#endif

    char tmpFilename[L_tmpnam_s];
    if(tmpnam_s(tmpFilename))
    {
        throw Exception("Could not create a temporary file.");
    }

    // Note that when a file name is pre-pended with a backslash and no path information, such as \fname21, this 
    // indicates that the name is valid for the current working directory.
    filename = tmpFilename[0] == '\\' ? tmpFilename + 1 : tmpFilename;

#else

    // Linux flavors must have a /tmp directory.
    std::stringstream ss;
    ss << "/tmp/ocio_" << GenerateRandomNumber();

    filename = ss.str();

#endif

    filename += filenameExt;

    return filename;
}

std::ifstream CreateInputFileStream(const char * filename, std::ios_base::openmode mode)
{
#if defined(_WIN32) && defined(UNICODE)
    return std::ifstream(Utf8ToUtf16(filename).c_str(), mode);
#else
    return std::ifstream(filename, mode);
#endif
}

void OpenInputFileStream(std::ifstream & stream, const char * filename, std::ios_base::openmode mode)
{
#if defined(_WIN32) && defined(UNICODE)
    stream.open(Utf8ToUtf16(filename).c_str(), mode);
#else
    stream.open(filename, mode);
#endif
}

#if defined(_WIN32) && defined(UNICODE)
const std::wstring filenameToUTF(const std::string & filename)
{
    // Can not return by reference since it could return std::wstring() or std::string().
    return Utf8ToUtf16(filename);
}
#else
const std::string filenameToUTF(const std::string & filename)
{
    return filename;
}
#endif

std::wstring Utf8ToUtf16(const std::string & str)
{
    if (str.empty()) {
        return std::wstring();
    }

#ifdef _WIN32
    int sz = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], sz);
    return wstr;
#else
    throw Exception("Only supported by the Windows platform.");
#endif
}

std::string Utf16ToUtf8(const std::wstring & wstr)
{
    if (wstr.empty()) {
        return std::string();
    }

#ifdef _WIN32
    int sz = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], sz, NULL, NULL);
    return str;
#else
    throw Exception("Only supported by the Windows platform.");
#endif
}

// Here is the explanation of the stat() method:
// https://pubs.opengroup.org/onlinepubs/009695299/basedefs/sys/stat.h.html
// "The st_ino and st_dev fields taken together uniquely identify the file within the system."
//
// However there are limitations to the stat() support on some Windows file systems:
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/stat-functions?redirectedfrom=MSDN&view=vs-2019
// "The inode, and therefore st_ino, has no meaning in the FAT, HPFS, or NTFS file systems."

// That's the default hash method implementation to compute a hash key based on a file content.
std::string CreateFileContentHash(const std::string &filename)
{
#if defined(_WIN32) && defined(UNICODE)
    struct _stat fileInfo;
    if (_wstat(Platform::Utf8ToUtf16(filename).c_str(), &fileInfo) == 0)
#else
    struct stat fileInfo;
    if (stat(filename.c_str(), &fileInfo) == 0)
#endif
    {
        // Treat the st_dev (i.e. device) + st_ino (i.e. inode) as a proxy for the contents.

        std::ostringstream fasthash;
        fasthash << fileInfo.st_dev << ":";
#ifdef _WIN32
        // TODO: The hard-linked files are then not correctly supported on Windows platforms.
        fasthash << std::hash<std::string>{}(filename);
#else
        fasthash << fileInfo.st_ino;
#endif
        return fasthash.str();
    }

    return "";
}

} // Platform

} // namespace OCIO_NAMESPACE

