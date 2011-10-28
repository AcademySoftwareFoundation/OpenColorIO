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
#include <fstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "argparse.h"

static std::string outputfile;

static int
parse_end_args(int argc, const char *argv[])
{
    if(argc>0)
    {
        outputfile = argv[0];
    }
    
    return 0;
}

int main (int argc, const char* argv[])
{
    
    bool help = false;
    int cubesize = -1;
    int shapersize = -1; // cubsize^2
    std::string format;
    std::string inputconfig;
    std::string inputspace;
    std::string shaperspace;
    std::string outputspace;
    bool usestdout = false;
    
    // What are the allowed baker output formats?
    std::ostringstream formats;
    formats << "the lut format to bake: ";
    for(int i=0; i<OCIO::Baker::getNumFormats(); ++i)
    {
        if(i!=0) formats << ", ";
        formats << OCIO::Baker::getFormatNameByIndex(i);
        formats << " (." << OCIO::Baker::getFormatExtensionByIndex(i) << ")";
    }
    std::string formatstr = formats.str();
    
    std::vector<std::string> luts;
    
    ArgParse ap;
    ap.options("ociobakelut -- create a new LUT from an OCIO config or lut file(s)\n\n"
               "usage:  ociobakelut [options] <OUTPUTFILE.LUT>\n\n"
               "example:  ociobakelut --inputspace lg10 --outputspace srgb8 --format flame lg_to_srgb.3dl\n"
               "example:  ociobakelut --lut filmlut.3dl --lut calibration.3dl --format flame lg_to_srgb.3dl\n\n",
               "%*", parse_end_args, "",
               "<SEPARATOR>", "Using Existing OCIO Configurations",
               "--inputspace %s", &inputspace, "Input OCIO ColorSpace (or Role)",
               "--outputspace %s", &outputspace, "Output OCIO ColorSpace (or Role)",
               "--shaperspace %s", &shaperspace, "the OCIO ColorSpace or Role, for the shaper",
               "--iconfig %s", &inputconfig, "Input .ocio configuration file (default: $OCIO)\n",
               "<SEPARATOR>", "Config-Free LUT Baking",
               "--lut %L", &luts, "Specify lut(s) to apply, in the forward direction.\n",
               "<SEPARATOR>", "Output Options",
               "--format %s", &format, formatstr.c_str(),
               "--shapersize %d", &shapersize, "size of the shaper (default: format specific)",
               "--cubesize %d", &cubesize, "size of the cube (default: format specific)",
               "--stdout", &usestdout, "Write lut to stdout (rather than file)",
               "--help", &help, "Print help message",
               // TODO: add --metadata option
               NULL);
    
    if (ap.parse(argc, argv) < 0)
    {
        std::cout << ap.geterror() << std::endl;
        ap.usage();
        std::cout << "\n";
        return 1;
    }
    
    if (help || (argc == 1 ))
    {
        ap.usage();
        std::cout << "\n";
        return 1;
    }
    
    // Create the OCIO processor for the specified transform.
    OCIO::ConstConfigRcPtr config;
    
    
    // If --luts have been specified, synthesize a new (temporary) configuration
    // with the transformation embedded in a colorspace.
    if(!luts.empty())
    {
        if(!inputspace.empty())
        {
            ap.usage();
            std::cerr << "\nERROR: --inputspace is not allowed when using --lut\n\n";
            return 1;
        }
        if(!outputspace.empty())
        {
            ap.usage();
            std::cerr << "\nERROR: --outputspace is not allowed when using --lut\n\n";
            return 1;
        }
        if(!shaperspace.empty())
        {
            ap.usage();
            std::cerr << "\nERROR: --shaperspace is not allowed when using --lut\n\n";
            return 1;
        }
        
        OCIO::ConfigRcPtr editableConfig = OCIO::Config::Create();
        
        OCIO::ColorSpaceRcPtr inputColorSpace = OCIO::ColorSpace::Create();
        inputspace = "RawInput";
        inputColorSpace->setName(inputspace.c_str());
        editableConfig->addColorSpace(inputColorSpace);
        
        OCIO::ColorSpaceRcPtr outputColorSpace = OCIO::ColorSpace::Create();
        outputspace = "ProcessedOutput";
        outputColorSpace->setName(outputspace.c_str());
        
        OCIO::GroupTransformRcPtr groupTransform = 
            OCIO::GroupTransform::Create();
        
        for(unsigned int i=0; i<luts.size(); ++i)
        {
            OCIO::FileTransformRcPtr f = OCIO::FileTransform::Create();
            f->setSrc(luts[i].c_str());
            f->setInterpolation(OCIO::INTERP_LINEAR);
            groupTransform->push_back(f);
        }
        
        outputColorSpace->setTransform(groupTransform,
            OCIO::COLORSPACE_DIR_FROM_REFERENCE);
        
        editableConfig->addColorSpace(outputColorSpace);
        config = editableConfig;
    }
    else
    {
    
        if(inputspace.empty())
        {
            ap.usage();
            std::cerr << "\nERROR: You must specify the --inputspace.\n\n";
            return 1;
        }
        
        if(outputspace.empty())
        {
            ap.usage();
            std::cerr << "\nERROR: You must specify the --outputspace.\n\n";
            return 1;
        }
        
        if(format.empty())
        {
            ap.usage();
            std::cerr << "\nERROR: You must specify the lut format using --format.\n\n";
            return 1;
        }
        
        if(!inputconfig.empty())
        {
            if(!usestdout) std::cerr << "[OpenColorIO INFO]: Loading " << inputconfig << std::endl;
            config = OCIO::Config::CreateFromFile(inputconfig.c_str());
        }
        else if(getenv("OCIO"))
        {
            if(!usestdout) std::cout << "[OpenColorIO INFO]: Loading $OCIO " << getenv("OCIO") << std::endl;
            config = OCIO::Config::CreateFromEnv();
        }
        else
        {
            std::cout << "ERROR: You must specify an input ocio configuration ";
            std::cout << "(either with --iconfig or $OCIO).\n\n";
            ap.usage ();
            return 1;
        }
    }
    
    if(outputfile.empty() && !usestdout)
    {
        ap.usage();
        std::cerr << "\nERROR: You must specify the outputfile or --stdout.\n\n";
        return 1;
    }
        
    try
    {
        OCIO::BakerRcPtr baker = OCIO::Baker::Create();
        
        // setup the baker for our lut type
        baker->setConfig(config);
        baker->setFormat(format.c_str());
        baker->setInputSpace(inputspace.c_str());
        baker->setShaperSpace(shaperspace.c_str());
        baker->setTargetSpace(outputspace.c_str());
        if(shapersize!=-1) baker->setShaperSize(shapersize);
        if(cubesize!=-1) baker->setCubeSize(cubesize);
        
        // output lut
        std::ostringstream output;
        
        if(!usestdout) std::cout << "[OpenColorIO INFO]: Baking '" << format << "' lut" << std::endl;
        
        if(usestdout)
        {
            baker->bake(std::cout);
        }
        else
        {
            std::ofstream f(outputfile.c_str());
            baker->bake(f);
            std::cout << "[OpenColorIO INFO]: Wrote '" << outputfile << "'" << std::endl;
        }
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
