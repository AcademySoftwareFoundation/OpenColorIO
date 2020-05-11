// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <set>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "apputils/argparse.h"


const char * DESC_STRING = "\n\n"
"ociocheck is useful to validate that the specified OCIO configuration\n"
"is valid, and that all the color transforms are defined.\n"
"For example, it is possible that the configuration may reference\n"
"lookup tables that do not exist. ociocheck will find these cases.\n\n"
"ociocheck can also be used to clean up formatting on an existing profile\n"
"that has been manually edited, using the '-o' option.\n";

int main(int argc, const char **argv)
{
    bool help = false;
    int errorcount = 0;
    std::string inputconfig;
    std::string outputconfig;

    ArgParse ap;
    ap.options("ociocheck -- validate an OpenColorIO configuration\n\n"
               "usage:  ociocheck [options]\n",
               "--help", &help, "Print help message",
               "--iconfig %s", &inputconfig, "Input .ocio configuration file (default: $OCIO)",
               "--oconfig %s", &outputconfig, "Output .ocio file",
               NULL);

    if (ap.parse(argc, argv) < 0)
    {
        std::cout << ap.geterror() << std::endl;
        ap.usage();
        std::cout << DESC_STRING;
        return 1;
    }

    if (help)
    {
        ap.usage();
        std::cout << DESC_STRING;
        return 1;
    }

    try
    {
        OCIO::ConstConfigRcPtr config;

        std::cout << std::endl;
        std::cout << "OpenColorIO Library Version: " << OCIO::GetVersion() << std::endl;
        std::cout << "OpenColorIO Library VersionHex: " << OCIO::GetVersionHex() << std::endl;

        if(!inputconfig.empty())
        {
            std::cout << "Loading " << inputconfig << std::endl;
            config = OCIO::Config::CreateFromFile(inputconfig.c_str());
        }
        else if(OCIO::GetEnvVariable("OCIO"))
        {
            std::cout << "Loading $OCIO " << OCIO::GetEnvVariable("OCIO") << std::endl;
            config = OCIO::Config::CreateFromEnv();
        }
        else
        {
            std::cout << "ERROR: You must specify an input OCIO configuration ";
            std::cout << "(either with --iconfig or $OCIO).\n";
            ap.usage ();
            std::cout << DESC_STRING;
            return 1;
        }

        std::cout << std::endl;
        std::cout << "** General **" << std::endl;
        std::cout << "Search Path: " << config->getSearchPath() << std::endl;
        std::cout << "Working Dir: " << config->getWorkingDir() << std::endl;
        std::cout << std::endl;

        if (config->getNumDisplays() == 0)
        {
            std::cout << "Error: At least one (display, view) pair must be defined." << std::endl;
            errorcount += 1;
        }
        else
        {
            std::cout << "Default Display: " << config->getDefaultDisplay() << std::endl;
            std::cout << "Default View: " << config->getDefaultView(config->getDefaultDisplay()) << std::endl;
        }

        {
            std::cout << std::endl;
            std::cout << "** Roles **" << std::endl;

            std::set<std::string> usedroles;
            const char * allroles[] = { OCIO::ROLE_DEFAULT, OCIO::ROLE_SCENE_LINEAR,
                                        OCIO::ROLE_DATA, OCIO::ROLE_REFERENCE,
                                        OCIO::ROLE_COMPOSITING_LOG, OCIO::ROLE_COLOR_TIMING,
                                        OCIO::ROLE_COLOR_PICKING,
                                        OCIO::ROLE_TEXTURE_PAINT, OCIO::ROLE_MATTE_PAINT,
                                        NULL };
            int MAXROLES=256;
            for(int i=0;i<MAXROLES; ++i)
            {
                const char * role = allroles[i];
                if(!role) break;
                usedroles.insert(role);

                OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(role);
                if(cs)
                {
                    std::cout << cs->getName() << " (" << role << ")" << std::endl;
                }
                else
                {
                    std::cout << "ERROR: NOT DEFINED" << " (" << role << ")" << std::endl;
                    errorcount += 1;
                }
            }

            for(int i=0; i<config->getNumRoles(); ++i)
            {
                const char * role = config->getRoleName(i);
                if(usedroles.find(role) != usedroles.end()) continue;

                OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(role);
                if(cs)
                {
                    std::cout << cs->getName() << " (" << role << ": user)" << std::endl;
                }
                else
                {
                    std::cout << "ERROR: NOT DEFINED" << " (" << role << ")" << std::endl;
                    errorcount += 1;
                }

            }
        }

        std::cout << std::endl;
        std::cout << "** ColorSpaces **" << std::endl;
        OCIO::ConstColorSpaceRcPtr lin = config->getColorSpace(OCIO::ROLE_SCENE_LINEAR);
        if(!lin)
        {
            std::cout << "Error: scene_linear role must be defined." << std::endl;
            errorcount += 1;
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

                    errorcount += 1;
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

        std::cout << std::endl;
        std::cout << "** Looks **" << std::endl;
        if(config->getNumLooks()>0)
        {
            for(int i=0; i<config->getNumLooks(); ++i)
            {
                std::cout << config->getLookNameByIndex(i) << std::endl;

                OCIO::ConstLookRcPtr look = config->getLook(config->getLookNameByIndex(i)); 

                OCIO::ConstTransformRcPtr transform = look->getTransform();         

                if(transform)
                {
                    try
                    {
                        OCIO::ConstProcessorRcPtr process = config->getProcessor(transform);
                        std::cout << "src file found" << std::endl;
                    }
                    catch(OCIO::Exception & exception)
                    {
                        std::cerr << "ERROR: " << exception.what() << std::endl;
                        errorcount += 1;
                    }
                }

                OCIO::ConstTransformRcPtr invTransform = look->getInverseTransform();         

                if(invTransform)
                {
                    try
                    {
                        OCIO::ConstProcessorRcPtr process = config->getProcessor(invTransform);
                        std::cout << "src file found" << std::endl;
                    }
                    catch(OCIO::Exception & exception)
                    {
                        std::cerr << "ERROR: " << exception.what() << std::endl;
                        errorcount += 1;
                    }
                }

            }
        }
        else
        {
            std::cout << "no looks defined" << std::endl;
        }
        std::cout << std::endl;
        std::cout << "** Sanity Check **" << std::endl;

        try
        {
            config->sanityCheck();
            std::cout << "passed" << std::endl;
        }
        catch(OCIO::Exception & exception)
        {
            std::cout << "ERROR" << std::endl;
            errorcount += 1;
            std::cout << exception.what() << std::endl;
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
    }
    catch(OCIO::Exception & exception)
    {
        std::cout << "ERROR: " << exception.what() << std::endl;
        return 1;
    } catch (std::exception& exception) {
        std::cout << "ERROR: " << exception.what() << "\n";
        return 1;
    }
    catch(...)
    {
        std::cout << "Unknown error encountered." << std::endl;
        return 1;
    }

    std::cout << std::endl;
    if(errorcount == 0)
    {
        std::cout << "Tests complete." << std::endl << std::endl;
        return 0;
    }
    else
    {
        std::cout << errorcount << " tests failed." << std::endl << std::endl;
        return 1;
    }
}
