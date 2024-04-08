// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <iostream>
#include <fstream>
#include <vector>

#include <pystring.h>

#include <OpenColorIO/OpenColorIO.h>
#include "utils/StringUtils.h"

namespace OCIO = OCIO_NAMESPACE;

#include "apputils/argparse.h"
#include "apputils/logGuard.h"

#if !defined(_WIN32)
#include <sys/param.h>
#include <unistd.h>
#else
#include <direct.h>
#define MAXPATHLEN 4096
#endif

std::string GetCwd()
{
#ifdef _WIN32
    char path[MAXPATHLEN];
    _getcwd(path, MAXPATHLEN);
    return path;
#else
    std::vector<char> current_dir;
#ifdef PATH_MAX
    current_dir.resize(PATH_MAX);
#else
    current_dir.resize(1024);
#endif
    while (::getcwd(&current_dir[0], current_dir.size()) == NULL && errno == ERANGE) {
        current_dir.resize(current_dir.size() + 1024);
    }

    return std::string(&current_dir[0]);
#endif
}

std::string AbsPath(const std::string & path)
{
    std::string p = path;
    if(!pystring::os::path::isabs(p)) p = pystring::os::path::join(GetCwd(), p);
    return pystring::os::path::normpath(p);
}

// Array of non OpenColorIO arguments.
static std::vector<std::string> args;

// Fill 'args' array with OpenColorIO arguments.
static int
parse_end_args(int argc, const char *argv[])
{
    while (argc>0)
    {
        args.push_back(argv[0]);
        argc--;
        argv++;
    }

    return 0;
}

int main(int argc, const char **argv)
{
    ArgParse ap;
    std::string errorMsg = "";

    // Options
    std::string baseConfigName;
    std::string inputConfigName;
    std::string mergeParameters;
    std::string outputFile;
    bool displayConfig = false;
    bool displayAllConfig = false;
    bool displayParams = false;
    bool validate = false;
    bool help = false;

    OCIO::ConstConfigRcPtr baseCfg;
    OCIO::ConstConfigRcPtr inputCfg;
    OCIO::ConstConfigMergingParametersRcPtr params;
    OCIO::ConstConfigMergerRcPtr merger;

    ap.options("ociomergeconfigs -- Merge two configs\n\n"
               "Usage:\n"
               "    ociomergeconfigs [options] -b config.ocio -i extra.ocio \n\n",
               "%*", parse_end_args, "",
               "<SEPARATOR>", "Options:",
               "--validate",     &validate,         "Validate the final merged config",
               "--merge %s",     &mergeParameters,  "Path to merge parameters file (.ociom)",
               "--out %s",       &outputFile,       "Filepath to save the merged config",
               "--show-last",    &displayConfig,    "Display the last merged config to screen",
               "--show-all",     &displayAllConfig, "Display ALL merged config to screen",
               "--show-params",  &displayParams,    "Display merger options (OCIOM)",
               "--help",    &help,                  "Display the help and exit",
               "-h",        &help,                  "Display the help and exit",
               NULL
               );

    if (ap.parse(argc, argv) < 0)
    {
        std::cerr << ap.geterror() << std::endl;
        exit(1);
    }

    if (help)
    {
        ap.usage();
        return 0;
    }

    // Load the options from the ociom file.
    // Otherwise, return object with default behaviors.
    try
    {
        merger = OCIO::ConfigMerger::CreateFromFile(mergeParameters.c_str());
    } 
    catch (OCIO::Exception & e)
    {
        std::cout << e.what() << std::endl;
    }

    try
    {
        OCIO::ConstConfigMergerRcPtr newMerger = OCIO::ConfigMergingHelpers::MergeConfigs(merger);
        if (validate)
        {
            try
            {
                LogGuard logGuard;
                newMerger->getMergedConfig()->validate();
            }
            catch(OCIO::Exception & exception)
            {
                std::cout << exception.what() << std::endl;
                exit(1);
            }
        }

        if (displayParams)
        {
            std::cout << "********************" << std::endl;
            std::cout << "Merger options" << std::endl;
            std::cout << "********************" << std::endl;
            std::ostringstream os;
            newMerger->serialize(os);
            std::cout << os.str() << std::endl;
            std::cout << std::endl;
        }

        // "Show-all" option take priority over the "show" option.
        if (displayAllConfig)
        {
            for (int i = 0; i < merger->getNumOfConfigMergingParameters(); i++)
            {
                std::cout << "*********************" << std::endl;
                std::cout << "Merged Config " << i << std::endl;
                std::cout << "*********************" << std::endl;
                std::ostringstream os;
                newMerger->getMergedConfig(i)->serialize(os);
                std::cout << os.str() << std::endl;
            }
        }

        if (displayConfig && !displayAllConfig)
        {
            std::cout << "********************" << std::endl;
            std::cout << "Last Merged Config" << std::endl;
            std::cout << "********************" << std::endl;
            std::ostringstream os;
            newMerger->getMergedConfig()->serialize(os);
            std::cout << os.str() << std::endl;
        }

        if (!outputFile.empty())
        {
            std::ofstream mergedCfg;
            mergedCfg.open(AbsPath(outputFile));

            std::ostringstream os;
            newMerger->getMergedConfig()->serialize(mergedCfg);
            mergedCfg.close();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what();
        exit(1);
    }
}
