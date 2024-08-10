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

namespace
{
    constexpr const char* TempDirMagicPrefix = "OCIOTestTemp_";
}

std::string CreateTemporaryDirectory(const std::string & name)
{
    std::string extended_name = TempDirMagicPrefix + name;

    int nError = 0;

#if defined(_WIN32)
    char cPath[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, cPath);
    if( len>MAX_PATH || len==0 )
    {
        throw Exception("Temp path could not be determined.");
    }
    
    static const std::string directory = pystring::os::path::join(cPath, extended_name);
    nError = _mkdir(directory.c_str());
#else 
    std::string sPath = "/tmp";
    const std::string directory = pystring::os::path::join(sPath, extended_name);
    nError = mkdir(directory.c_str(), 0777);
#endif

    if (nError != 0)
    {
        std::ostringstream error;
        error << "Could not create a temporary directory '" << directory << "'. Make sure that the directory do "
            "not already exist or sufficient permissions are set";
        throw Exception(error.str().c_str());
    }

    return directory;
}

void RemoveTemporaryDirectory(const std::string & sDirectoryPath)
{
    if(sDirectoryPath.empty())
    {
        throw Exception("removeDirectory() is called with empty path.");
    }

    // Sanity check: don't delete the folder if we haven't created it.
    if(std::string::npos == sDirectoryPath.find(TempDirMagicPrefix))
    {
        std::ostringstream error;
        error << "removeDirectory() tries to delete folder '"<< sDirectoryPath << "' which was not created by the unit tests.";
        throw Exception(error.str().c_str());
    }

#if defined(_WIN32)
    std::string search_pattern = sDirectoryPath + ("/*.*");
    std::string cur_path = sDirectoryPath + "/";

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) 
    {
        do 
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Ignore "." and ".." directory.
                if ( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
                {
                    RemoveTemporaryDirectory(cur_path + fd.cFileName);
                }
            }
            else 
            {
                DeleteFileA((cur_path + fd.cFileName).c_str());
            }
        } while (FindNextFileA(hFind, &fd));

        FindClose(hFind);
        RemoveDirectoryA(sDirectoryPath.c_str());
    }
#else
    struct dirent* entry = NULL;
    DIR* dir = NULL;
    dir = opendir(sDirectoryPath.c_str());
    while ((entry = readdir(dir)))
    {
        DIR* sub_dir = NULL;
        FILE* file = NULL;

        std::string absPath;
        // Ignore "." and ".." directory.
        if (!StringUtils::Compare(".", entry->d_name) &&
            !StringUtils::Compare("..", entry->d_name))
        {
            absPath = pystring::os::path::join(sDirectoryPath, entry->d_name);
            sub_dir = opendir(absPath.c_str());
            if (sub_dir)
            {
                closedir(sub_dir);
                RemoveTemporaryDirectory(absPath);
            }
            else
            {
                file = fopen(absPath.c_str(), "r");
                if (file)
                {
                    fclose(file);
                    remove(absPath.c_str());
                }
            }
        }
    }
    remove(sDirectoryPath.c_str());
    closedir(dir);
#endif

}

} // namespace OCIO_NAMESPACE
