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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

void
print_help(boost::program_options::options_description desc) {
    std::cout << "\n";
    std::cout << "ociocheck -i INPUT_CONFIG\n";
    std::cout << "\n";
    std::cout << desc << "\n";
    return;
}


const char * USAGE_TEXT = "\n"
"ociocheck <CONFIG>\n";

int main(int argc, char **argv)
{
    try
    {
        std::string inputconfig;
        std::string outputconfig;
        
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce this help message")
            ("inputconfig,i", po::value<std::string>(&inputconfig), "Input .ocio configuration file. If none is specified, the $OCIO env will be used.")
            ("outputconfig,o", po::value<std::string>(&outputconfig)->default_value(""), "Output .ocio configuration file")
            ;
        
        po::variables_map vm;
        po::positional_options_description posi;
        //posi.add("outputfile", -1);
        
        po::store(po::command_line_parser(argc, argv).options(desc).positional(posi).run(), vm);
        po::notify(vm);
        
        // print help
        if(vm.count("help"))
        {
            print_help(desc);
            return 1;
        }
        
        OCIO::ConstConfigRcPtr config;
        
        std::cout << std::endl;
        if(!inputconfig.empty())
        {
            std::cout << "Loading " << inputconfig << std::endl;
            config = OCIO::Config::CreateFromFile(inputconfig.c_str());
        }
        else if(getenv("OCIO"))
        {
            std::cout << "Loading $OCIO " << getenv("OCIO") << std::endl;
            config = OCIO::Config::CreateFromEnv();
        }
        else
        {
            std::cout << "ERROR: You must specify an input ocio configuration (either with -i or $OCIO).\n";
            print_help(desc);
            return 1;
        }
        
        std::cout << std::endl;
        std::cout << "** General **" << std::endl;
        try
        {
            config->sanityCheck();
            std::cout << "Passed Sanity Check." << std::endl;
        }
        catch(OCIO::Exception & exception)
        {
            std::cout << "Error: " << exception.what() << std::endl;
        }
        std::cout << "Search Path: " << config->getSearchPath() << std::endl;
        std::cout << "Working Dir: " << config->getWorkingDir() << std::endl;
        
        std::cout << std::endl;
        std::cout << "Default Display: " << config->getDefaultDisplay() << std::endl;
        std::cout << "Default View: " << config->getDefaultView(config->getDefaultDisplay()) << std::endl;
        
        
        
        std::cout << std::endl;
        std::cout << "** Roles **" << std::endl;
        
        for(int i=0; i<config->getNumRoles(); ++i)
        {
            const char * rolename = config->getRoleName(i);
            std::cout << rolename << " = ";
            OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(rolename);
            if(cs) std::cout << cs->getName() << std::endl;
            else std::cout << "NOT FOUND!" << std::endl;
        }
        
        
        
        std::cout << std::endl;
        std::cout << "** ColorSpaces **" << std::endl;
        OCIO::ConstColorSpaceRcPtr lin = config->getColorSpace(OCIO::ROLE_SCENE_LINEAR);
        if(!lin)
        {
            std::cout << "Error: scene_linear role must be defined." << std::endl;
        }
        else
        {
            for(int i=0; i<config->getNumColorSpaces(); ++i)
            {
                OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(config->getColorSpaceNameByIndex(i));
                
                bool convertsToLinear = true;
                std::string convertsToLinearErrorText;
                
                bool convertsFromLinear = true;
                std::string convertsFromLinearErrorText;
                
                try
                {
                    OCIO::ConstProcessorRcPtr p = config->getProcessor(cs, lin);
                }
                catch(OCIO::Exception & exception)
                {
                    convertsToLinear = false;
                    convertsToLinearErrorText = exception.what();
                }
                
                try
                {
                    OCIO::ConstProcessorRcPtr p = config->getProcessor(lin, cs);
                }
                catch(OCIO::Exception & exception)
                {
                    convertsFromLinear = false;
                    convertsFromLinearErrorText = exception.what();
                }
                
                if(convertsToLinear && convertsFromLinear)
                {
                    std::cout << cs->getName() << std::endl;
                }
                else if(!convertsToLinear && !convertsFromLinear)
                {
                    std::cout << cs->getName();
                    std::cout << " -- error" << std::endl;
                    std::cout << "\t" << convertsToLinearErrorText << std::endl;
                    std::cout << "\t" << convertsFromLinearErrorText << std::endl;
                }
                else if(convertsToLinear)
                {
                    std::cout << cs->getName();
                    std::cout << " -- input only" << std::endl;
                }
                else if(convertsFromLinear)
                {
                    std::cout << cs->getName();
                    std::cout << " -- output only" << std::endl;
                }
            }
        }
        
        
        if(!outputconfig.empty())
        {
            std::ofstream output;
            output.open(outputconfig.c_str());
            
            if(!output.is_open())
            {
                std::cout << "Error opening " << outputconfig << " for writing." << std::endl;
            }
            else
            {
                config->serialize(output);
                output.close();
                std::cout << "Wrote " << outputconfig << std::endl;
            }
        }
        
        std::cout << std::endl;
        std::cout << "Tests complete." << std::endl << std::endl;
    }
    catch(OCIO::Exception & exception)
    {
        std::cerr << "ERROR: " << exception.what() << std::endl;
        exit(1);
    } catch (std::exception& exception) {
        std::cerr << "ERROR: " << exception.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cerr << "Unknown error encountered." << std::endl;
        exit(1);
    }
    
    return 0;
}
