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

#ifdef _WIN32
    std::wstring name_u16 = Utf8ToUtf16(name);
    if(uint32_t size = GetEnvironmentVariable(name_u16.c_str(), nullptr, 0))
    {
        std::wstring value_u16(size, 0);
        GetEnvironmentVariable(name_u16.c_str(), &value_u16[0], size);

        // GetEnvironmentVariable is designed for raw pointer strings and therefore requires that
        // the destination buffer be long enough to place a null terminator at the end of it. Since
        // we're using std::wstrings here, the null terminator is unnecessary (and causes false
        // negatives in unit tests since the extra character makes it "non-equal" to normally
        // defined std::wstrings). Therefore, we pop the last character off (the null terminator)
        // to ensure that the string conforms to expectations.
        value_u16.pop_back();

        value = Utf16ToUtf8(value_u16);
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
    _wputenv_s(Utf8ToUtf16(name).c_str(), Utf8ToUtf16(value).c_str());
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
    // Note that the Windows _putenv_s() removes the env. variable if the value is empty.
    _wputenv_s(Utf8ToUtf16(name).c_str(), L"");
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
#ifdef _WIN32
    return ::_stricmp(str1, str2);
#else
    return ::strcasecmp(str1, str2);
#endif
}

int Strncasecmp(const char * str1, const char * str2, size_t n)
{
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

    char tmpFilename[L_tmpnam_s];
    if(tmpnam_s(tmpFilename))
    {
        throw Exception("Could not create a temporary file.");
    }

    filename = tmpFilename;

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
#ifdef _WIN32
    return std::ifstream(Utf8ToUtf16(filename).c_str(), mode);
#else
    return std::ifstream(filename, mode);
#endif
}

void OpenInputFileStream(std::ifstream & stream, const char * filename, std::ios_base::openmode mode)
{
#ifdef _WIN32
    stream.open(Utf8ToUtf16(filename).c_str(), mode);
#else
    stream.open(filename, mode);
#endif
}

std::wstring Utf8ToUtf16(std::string str)
{
#ifdef _WIN32
    int sz = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], sz);
    return wstr;
#else
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::codecvt_mode::little_endian>, wchar_t>{}.from_bytes(str);
#endif
}

std::string Utf16ToUtf8(std::wstring wstr)
{
#ifdef _WIN32
    int sz = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], sz, NULL, NULL);
    return str;
#else
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::codecvt_mode::little_endian>, wchar_t>{}.to_bytes(wstr);
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
#ifdef _WIN32
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

