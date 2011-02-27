/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "argparse.h"

static int
parse_end_args(int /*argc*/, const char /**argv[]*/)
{
    return 0;
}

int main (int argc, const char* argv[])
{
    
    bool help = false;
    bool verbose = true;
    int cubesize = 32;
    int shapersize = 1024; // cubsize^2
    std::string format;
    std::string inputconfig;
    std::string inputspace;
    std::string shaperspace;
    std::string outputspace;
    
    ArgParse ap;
    ap.options("ociobakelut -- bake out a lut from an OpenColorIO config\n\n"
               "usage:  ociobakelut [options]\n\n"
               "example:  ociobakelut --iconfig foo.ocio --inputspace lnf --shaperspace jplog --outputspace sRGB --format houdini\n\n",
               "%*", parse_end_args, "",
               "--help", &help, "Print help message",
               "--format %s", &format, "the lut format to bake",
               "--inputspace %s", &inputspace, "the OCIO ColorSpace or Role, for the input (required)",
               "--shaperspace %s", &shaperspace, "the OCIO ColorSpace or Role, for the shaper",
               "--outputspace %s", &outputspace, "the OCIO ColorSpace or Role, for the output (required)",
               "--shapersize %d", &shapersize, "size of the shaper (default: 1024 = cubsize^2)",
               "--cubesize %d", &cubesize, "size of the cube (default: 32)",
               // TODO: add --stdout option
               // TODO: add --file option
               // TODO: add --metadata option
               "--iconfig %s", &inputconfig, "Input .ocio configuration file (default: $OCIO)",
               NULL);
    
    if (ap.parse(argc, argv) < 0)
    {
        std::cout << ap.geterror() << std::endl;
        ap.usage();
        return 1;
    }
    
    if (help)
    {
        ap.usage();
        return 1;
    }
    
    if(inputspace.empty())
    {
        std::cout << "need to specify a --inputspace of the source that the lut will be applied to\n";
        ap.usage();
        return 1;
    }
    
    if(outputspace.empty())
    {
        std::cout << "need to specify a --outputspace of the display the lut is for\n";
        ap.usage();
        return 1;
    }
    
    if(format.empty())
    {
        std::cout << "need to specify a --format of the lut to bake\n";
        ap.usage();
        return 1;
    }
    
    try
    {
        // Create the OCIO processor for the specified transform.
        OCIO::ConstConfigRcPtr config;
        
        if(!inputconfig.empty())
        {
            if(!verbose) std::cout << "[OpenColorIO INFO]: Loading " << inputconfig << std::endl;
            config = OCIO::Config::CreateFromFile(inputconfig.c_str());
        }
        else if(getenv("OCIO"))
        {
            if(!verbose) std::cout << "[OpenColorIO INFO]: Loading $OCIO " << getenv("OCIO") << std::endl;
            config = OCIO::Config::CreateFromEnv();
        }
        else
        {
            std::cout << "ERROR: You must specify an input ocio configuration ";
            std::cout << "(either with --iconfig or $OCIO).\n";
            ap.usage ();
            return 1;
        }
        
        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        
        // setup the baker for our lut type
        baker->setConfig(config);
        baker->setFormat(format.c_str());
        baker->setInputSpace(inputspace.c_str());
        baker->setShaperSpace(shaperspace.c_str());
        baker->setTargetSpace(outputspace.c_str());
        baker->setShaperSize(shapersize);
        baker->setCubeSize(cubesize);
        
        // output lut
        std::ostringstream output;
        
        if(!verbose) std::cout << "[OpenColorIO INFO]: Baking '" << format << "' lut" << std::endl;
        baker->bake(output);
        
        // if stdout
        if(verbose)
        {
            std::cout << output.str();
        }
        else
        {
            // TODO: write to file here
        }
        
        if(!stdout) std::cout << "[OpenColorIO INFO]: done." << std::endl;
        
    }
    catch(OCIO::Exception & exception)
    {
        std::cerr << "OCIO Error: " << exception.what() << std::endl;
        return 1;
    }
    catch (std::exception& exception)
    {
        std::cerr << "Error: " << exception.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cerr << "Unknown OCIO error encountered." << std::endl;
        return 1;
    }
    
    return 0;
}