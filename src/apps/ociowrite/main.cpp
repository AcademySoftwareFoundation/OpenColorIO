/*
Copyright (c) 2019 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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

#include "argparse.h"


int main(int argc, const char **argv)
{
    bool verbose = false;
    std::string inputColorSpace, outputColorSpace;
    std::string filepath;

    bool help = false;

    // What are the allowed writing output formats?
    std::ostringstream formats;
    formats << "Formats to write to: ";
    for (int i = 0; i<OCIO::Processor::getNumWriteFormats(); ++i)
    {
        if (i != 0) formats << ", ";
        formats << OCIO::Processor::getFormatNameByIndex(i);
        formats << " (." << OCIO::Processor::getFormatExtensionByIndex(i) << ")";
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
        const char * env = getenv("OCIO");
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
        for (int i = 0; i < OCIO::Processor::getNumWriteFormats(); ++i)
        {
            std::string formatExt(OCIO::Processor::getFormatExtensionByIndex(i));
            if (requestedExt == formatExt)
            {
                transformFileFormat = OCIO::Processor::getFormatNameByIndex(i);
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
        if (!inputColorSpace.empty() && !outputColorSpace.empty())
        {
            const char * env = getenv("OCIO");
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

            // Get the processor.
            processor = config->getProcessor(inputColorSpace.c_str(), outputColorSpace.c_str());

            std::ofstream outfs(filepath.c_str(), std::ios::out | std::ios::trunc);
            if (outfs)
            {
                processor->write(transformFileFormat.c_str(), outfs);
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
