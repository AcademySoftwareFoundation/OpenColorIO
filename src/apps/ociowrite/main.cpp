// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "apputils/argparse.h"


int main(int argc, const char **argv)
{
    bool verbose = false;
    std::string inputColorSpace, outputColorSpace, display, view;
    std::string filepath;

    bool help = false;

    bool useColorspaces = false;
    bool useDisplayview = false;
    bool useInvertview  = false;

    // What are the allowed writing output formats?
    std::ostringstream formats;
    formats << "\n                            "
               "Formats to write to:\n                             ";
    for (int i = 0; i<OCIO::GroupTransform::GetNumWriteFormats(); ++i)
    {
        formats << OCIO::GroupTransform::GetFormatNameByIndex(i);
        formats << " (." << OCIO::GroupTransform::GetFormatExtensionByIndex(i) 
                << ")\n                             ";
    }

    std::string pathDesc = "Transform file path. Format is implied by extension. ";
    pathDesc += formats.str();

    ArgParse ap;
    ap.options("ociowrite -- write a color transformation to a file\n\n"
               "usage: ociowrite [options] --file outputfile\n\n",
               "--h",                       &help, 
                                            "Display the help and exit",
               "--help",                    &help, 
                                            "Display the help and exit",
               "--v",                       &verbose, 
                                            "Display some general information",
               "--colorspaces %s %s",       &inputColorSpace, &outputColorSpace,
                                            "Provide the input and output color spaces to apply on the image",
               "--view %s %s %s",           &inputColorSpace, &display, &view,
                                            "Provide the input color space and (display, view) pair to apply on the image",
               "--displayview %s %s %s",    &inputColorSpace, &display, &view,
                                            "(Deprecated) Provide the input color space and (display, view) pair to apply on the image",
               "--invertview %s %s %s",     &display, &view, &outputColorSpace,
                                            "Provide the (display, view) pair and output color space to apply on the image",
               "--file %s",                 &filepath, 
                                            pathDesc.c_str(),
               NULL);

    if (argc <= 1 || ap.parse(argc, argv) < 0)
    {
        std::cerr << ap.geterror() << "\n";
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
        std::cout << "\nOCIO Version: " << OCIO::GetVersion() << "\n";
        const char * env = OCIO::GetEnvVariable("OCIO");
        if (env && *env)
        {
            try
            {
                std::cout << "\nOCIO Configuration: '" << env << "'\n";
                OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
                std::cout << "OCIO search_path:    " << config->getSearchPath() << "\n";
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
        std::cerr << "\nThe output transform filepath is missing.\n";
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
        std::cerr << "\nCould not find a valid format from the extension of: '";
        std::cerr << filepath << "'. " << formats.str() << "\n";
        exit(1);
    }
    else if (verbose)
    {
        std::cout << "\nFile format being used: " << transformFileFormat << "\n";
    }

    std::cout << "\n";

    // Process transform.
    try
    {
        // Load the current config.

        OCIO::ConstProcessorRcPtr processor;

        // Checking for an input colorspace or input (display, view) pair.
        if (!inputColorSpace.empty() || (!display.empty() && !view.empty()))
        {
            const char * env = OCIO::GetEnvVariable("OCIO");
            if(env && *env)
            {
                if (verbose)
                {
                    std::string inputStr = !inputColorSpace.empty() ?  inputColorSpace : "(" + display + ", " + view + ")";
                    std::string outputStr = !outputColorSpace.empty() ?  outputColorSpace : "(" + display + ", " + view + ")";
                    std::cout << "\nProcessing from '" << inputStr << "' to '" << outputStr << "'\n";
                }
            }
            else
            {
                std::cerr << "\nMissing the ${OCIO} env. variable.\n";
                exit(1);
            }

            OCIO::ConstConfigRcPtr config  = OCIO::Config::CreateFromEnv();

            if (verbose)
            {
                std::cout << "\nConfig: " << config->getDescription()
                          << " - version: " << config->getMajorVersion();
                const auto minor = config->getMinorVersion();
                if (minor)
                {
                    std::cout << "." << minor;
                }
                std::cout << "\n";
            }

            // --colorspaces
            useColorspaces = !inputColorSpace.empty() && !outputColorSpace.empty();
            // --view
            useDisplayview = !inputColorSpace.empty() && !display.empty() && !view.empty();
            // --invertview
            useInvertview = !display.empty() && !view.empty() && !outputColorSpace.empty();

            // Errors validation
            std::string msg; 
            if (!((useColorspaces && !useDisplayview && !useInvertview) ||
                    (useDisplayview && !useColorspaces && !useInvertview) ||
                    (useInvertview && !useColorspaces && !useDisplayview)))
            {
                std::cerr << "\nAny combinations of --colorspaces, --view or --invertview is invalid.\n";
                exit(1);
            } 

            // Processing colorspaces option         
            if (useColorspaces)
            {
                // colorspaces to colorspaces

                // validate output arguments
                if (!outputColorSpace.empty())
                {
                    processor = config->getProcessor(inputColorSpace.c_str(), outputColorSpace.c_str());
                }
                else
                {
                    std::cerr << "\nMissing output color spaces for --colorspaces.\n";
                    exit(1);
                }
            } 
            // Processing view option
            else if (useDisplayview)
            {
                // colorspaces to (display,view) pair

                // validate output arguments
                if (!display.empty() && !view.empty())
                {
                    processor = config->getProcessor(inputColorSpace.c_str(), 
                                                     display.c_str(),
                                                     view.c_str(),
                                                     OCIO::TRANSFORM_DIR_FORWARD);
                }
                else
                {
                    std::cerr << "\nMissing output (display,view) pair for --view.\n";
                    exit(1);
                }
            }
            // Processing invertview option
            else if (useInvertview)
            {
                // (display,view) pair to colorspaces

                // validate output arguments
                if (!outputColorSpace.empty())
                {
                    processor = config->getProcessor(outputColorSpace.c_str(), 
                                                     display.c_str(),
                                                     view.c_str(),
                                                     OCIO::TRANSFORM_DIR_INVERSE);
                }
                else
                {
                    std::cerr << "\nMissing output colorspaces for --invertview.\n";
                    exit(1);
                }
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
                std::cerr << "\nCould not open file: " << filepath << "\n";
                exit(1);
            }
        }
        else
        {
            std::cerr << "\nColorspaces or (display,view) pair must be specified as source.\n";
            exit(1);
        }
    }
    catch(OCIO::Exception & exception)
    {
        std::cerr << "OCIO Error: " << exception.what() << "\n";
        exit(1);
    }
    catch(...)
    {
        std::cerr << "Unknown OCIO error encountered.\n";
        exit(1);
    }

    return 0;
}
