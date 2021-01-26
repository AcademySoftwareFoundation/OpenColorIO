// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "apputils/argparse.h"


int main(int argc, const char **argv)
{
    bool verbose = false;
    std::string inputColorSpace, outputColorSpace, display, view;
    std::string filepath;

    bool help = false;

    // What are the allowed writing output formats?
    std::ostringstream formats;
    formats << "Formats to write to: ";
    for (int i = 0; i<OCIO::GroupTransform::GetNumWriteFormats(); ++i)
    {
        if (i != 0) formats << ", ";
        formats << OCIO::GroupTransform::GetFormatNameByIndex(i);
        formats << " (." << OCIO::GroupTransform::GetFormatExtensionByIndex(i) << ")";
    }

    std::string pathDesc = "Transform file path. Format is implied by extension. ";
    pathDesc += formats.str();

    ArgParse ap;
    ap.options("ociowrite -- write a color transformation to a file\n\n"
               "usage: ociowrite [options] --file outputfile\n\n",
               "--h", &help, "Display the help and exit",
               "--v", &verbose, "Display some general information",
               "--colorspaces %s %s", &inputColorSpace, &outputColorSpace, 
                                      "Provide the input and output color spaces",
               "--displayview %s %s %s", &inputColorSpace, &display, &view,
                                      "Provide the input and (display, view) pair",
               "--file %s", &filepath, pathDesc.c_str(),
               NULL);

    if (argc <= 1 || ap.parse(argc, argv) < 0)
    {
        std::cerr << ap.geterror() << std::endl;
        ap.usage();
        exit(1);
    }

    if (help)
    {
        ap.usage();
        exit(1);
    }

    if (verbose)
    {
        std::cout << std::endl;
        std::cout << "OCIO Version: " << OCIO::GetVersion() << std::endl;
        const char * env = OCIO::GetEnvVariable("OCIO");
        if (env && *env)
        {
            try
            {
                std::cout << std::endl;
                std::cout << "OCIO Configuration: '" << env << "'" << std::endl;
                OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
                std::cout << "OCIO search_path:    " << config->getSearchPath() << std::endl;
            }
            catch(...)
            {
                std::cerr << "Error loading the config file: '" << env << "'";
                exit(1);
            }
        }
    }

    if (filepath.empty())
    {
        std::cerr << std::endl;
        std::cerr << "The output transform filepath is missing." << std::endl;
        exit(1);
    }

    std::string transformFileFormat;
    const char * ext = strrchr(filepath.c_str(), '.');
    if (ext)
    {
        std::string requestedExt(ext+1);
        std::transform(requestedExt.begin(), requestedExt.end(), requestedExt.begin(), ::tolower);
        for (int i = 0; i < OCIO::GroupTransform::GetNumWriteFormats(); ++i)
        {
            std::string formatExt(OCIO::GroupTransform::GetFormatExtensionByIndex(i));
            if (requestedExt == formatExt)
            {
                transformFileFormat = OCIO::GroupTransform::GetFormatNameByIndex(i);
                break;
            }
        }
    }

    if (transformFileFormat.empty())
    {
        std::cerr << std::endl;
        std::cerr << "Could not find a valid format from the extension of: '";
        std::cerr << filepath << "'. " << formats.str() << std::endl;
        exit(1);
    }
    else if (verbose)
    {
        std::cout << std::endl;
        std::cout << "File format being used: " << transformFileFormat << std::endl;
    }

    std::cout << std::endl;

    // Process transform.
    try
    {
        // Load the current config.

        OCIO::ConstProcessorRcPtr processor;
        if (!inputColorSpace.empty())
        {
            const char * env = OCIO::GetEnvVariable("OCIO");
            if(env && *env)
            {
                if (verbose)
                {
                    std::cout << std::endl;
                    std::cout << "Processing from '" << inputColorSpace << "' to '"
                              << outputColorSpace << "'" << std::endl;
                }
            }
            else
            {
                std::cerr << std::endl;
                std::cerr << "Missing the ${OCIO} env. variable." << std::endl;
                exit(1);
            }

            OCIO::ConstConfigRcPtr config  = OCIO::Config::CreateFromEnv();

            if (verbose)
            {
                std::cout << std::endl;
                std::cout << "Config: " << config->getDescription()
                          << " - version: " << config->getMajorVersion();
                const auto minor = config->getMinorVersion();
                if (minor)
                {
                    std::cout << "." << minor;
                }
                std::cout << std::endl;
            }

            if (!outputColorSpace.empty())
            {
                if (!display.empty() || !view.empty())
                {
                    std::cerr << std::endl;
                    std::cerr << "Both --colorspaces and --displayview may not be used at the same time.";
                    exit(1);
                }

                processor = config->getProcessor(inputColorSpace.c_str(), outputColorSpace.c_str());
            }
            else if (!display.empty() && !view.empty())
            {
                processor = config->getProcessor(inputColorSpace.c_str(), 
                                                 display.c_str(),
                                                 view.c_str(),
                                                 OCIO::TRANSFORM_DIR_FORWARD);
            }
            else
            {
                std::cerr << std::endl;
                std::cerr << "Missing color spaces for --displayview." << std::endl;
                exit(1);
            }

            std::ofstream outfs(filepath.c_str(), std::ios::out | std::ios::trunc);
            if (outfs)
            {
                const auto group = processor->createGroupTransform();
                group->write(config, transformFileFormat.c_str(), outfs);
                outfs.close();
            }
            else
            {
                std::cerr << std::endl;
                std::cerr << "Could not open file: " << filepath << std::endl;
                exit(1);
            }
        }
        else
        {
            std::cerr << std::endl;
            std::cerr << "Source and destination color space must be specified." << std::endl;
            exit(1);
        }
    }
    catch(OCIO::Exception & exception)
    {
        std::cerr << "OCIO Error: " << exception.what() << std::endl;
        exit(1);
    }
    catch(...)
    {
        std::cerr << "Unknown OCIO error encountered." << std::endl;
        exit(1);
    }

    return 0;
}
