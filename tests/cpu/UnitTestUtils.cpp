// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "Logging.h"
#include "OpBuilders.h"
#include "UnitTestUtils.h"
#include "utils/StringUtils.h"

#ifndef _WIN32
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <direct.h>
#endif

namespace OCIO_NAMESPACE
{
#ifndef OCIO_UNIT_TEST_FILES_DIR
#error Expecting OCIO_UNIT_TEST_FILES_DIR to be defined for tests. Check relevant CMakeLists.txt
#endif

// For explanation, refer to https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html 
#define _STR(x) #x
#define STR(x) _STR(x)


static const std::string ocioTestFilesDir(STR(OCIO_UNIT_TEST_FILES_DIR));

const std::string & GetTestFilesDir()
{
    return ocioTestFilesDir;
}

// Create a FileTransform.
FileTransformRcPtr CreateFileTransform(const std::string & fileName)
{
    const std::string filePath(GetTestFilesDir() + "/" + fileName);

    // Create a FileTransform
    FileTransformRcPtr pFileTransform = FileTransform::Create();
    pFileTransform->setSrc(filePath.c_str());

    return pFileTransform;
}

void BuildOpsTest(OpRcPtrVec & fileOps,
                  const std::string & fileName,
                  ContextRcPtr & context,
                  TransformDirection dir)
{
    FileTransformRcPtr fileTransform = CreateFileTransform(fileName);

    // Create empty Config to use
    ConfigRcPtr config = Config::Create();
    BuildFileTransformOps(fileOps, *(config.get()), context,
                          *(fileTransform.get()), dir);
}


ConstProcessorRcPtr GetFileTransformProcessor(const std::string & fileName)
{
    FileTransformRcPtr fileTransform = CreateFileTransform(fileName);

    // Create empty Config to use.
    ConfigRcPtr config = Config::Create();
    // Get the processor corresponding to the transform.
    return config->getProcessor(fileTransform);
}

std::string CreateTemporaryDirectory(const std::string & name)
{
    int nError = 0;

#if defined(_WIN32)
    std::string sPath = GetEnvVariable("TEMP");
    static const std::string directory = pystring::os::path::join(sPath, name);
    nError = _mkdir(directory.c_str());
#else 
    std::string sPath = "/tmp";
    const std::string directory = pystring::os::path::join(sPath, name);
    nError = mkdir(directory.c_str(), 0777);
#endif

    if (nError != 0)
    {
        std::ostringstream error;
        error << "Could not create a temporary directory." << " Make sure that the directory do "
        << "not already exist or sufficient permissions are set";
        throw Exception(error.str().c_str());
    }

    return directory;
}

#if defined(_WIN32)
void removeDirectory(const wchar_t* directoryPath)
{
    std::wstring search_path = std::wstring(directoryPath) + Platform::filenameToUTF("/*.*");
    std::wstring s_p = std::wstring(directoryPath) + Platform::filenameToUTF("/");
    WIN32_FIND_DATA fd;
    HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) 
    {
        do 
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Ignore "." and ".." directory.
                if (wcscmp(fd.cFileName, Platform::filenameToUTF(".").c_str()) != 0 && 
                    wcscmp(fd.cFileName, Platform::filenameToUTF("..").c_str()) != 0)
                {
                    removeDirectory((wchar_t*)(s_p + fd.cFileName).c_str());
                }
            }
            else 
            {
                DeleteFile((s_p + fd.cFileName).c_str());
            }
        } while (::FindNextFile(hFind, &fd));

        ::FindClose(hFind);
        _wrmdir(directoryPath);
    }
}
#else
void removeDirectory(const char * directoryPath)
{
    struct dirent *entry = NULL;
    DIR *dir = NULL;
    dir = opendir(directoryPath);
    while((entry = readdir(dir)))
    {   
        DIR *sub_dir = NULL;
        FILE *file = NULL;

        std::string absPath;
        // Ignore "." and ".." directory.
        if (!StringUtils::Compare(".", entry->d_name) &&
            !StringUtils::Compare("..", entry->d_name))
        {
            absPath = pystring::os::path::join(directoryPath, entry->d_name);
            sub_dir = opendir(absPath.c_str());
            if(sub_dir)
            {   
                closedir(sub_dir);
                removeDirectory(absPath.c_str());
            }   
            else 
            {   
                file = fopen(absPath.c_str(), "r");
                if(file)
                {   
                    fclose(file);
                    remove(absPath.c_str());
                }   
            }   
        }
    }
    remove(directoryPath);
    closedir(dir);
}
#endif

void RemoveTemporaryDirectory(const std::string & directoryPath)
{
#if defined(_WIN32)
    removeDirectory(Platform::filenameToUTF(directoryPath).c_str());
#else
    removeDirectory(directoryPath.c_str());
#endif
}

} // namespace OCIO_NAMESPACE
