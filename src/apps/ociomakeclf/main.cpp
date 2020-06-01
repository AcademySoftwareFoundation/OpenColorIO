// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cstdio>
#include <iostream>
#include <fstream>
#include <string.h>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "apputils/argparse.h"
#include "apputils/measure.h"
#include "utils/StringUtils.h"


// Array of non OpenColorIO arguments.
static std::vector<std::string> args;

// Fill 'args' array with OpenColorIO arguments.
static int parse_end_args(int argc, const char * argv[])
{
    while ( argc > 0)
    {
        args.push_back(argv[0]);
        argc--;
        argv++;
    }

    return 0;
}

void CreateOutputLutFile(const std::string & outLutFilepath, OCIO::ConstGroupTransformRcPtr transform)
{
    // Get the processor.

    // Create an empty config but with the latest version.
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->upgradeToLatestVersion();

    OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);

    // CLF file format does not support inverse 1D LUTs, optimize the processor
    // to replace inverse 1D LUTs by 'fast forward' 1D LUTs. 
    OCIO::ConstProcessorRcPtr optProcessor
        = processor->getOptimizedProcessor(OCIO::BIT_DEPTH_F32, 
                                           OCIO::BIT_DEPTH_F32,
                                           OCIO::OPTIMIZATION_LUT_INV_FAST);

    // Create the CLF file.

    std::ofstream outfs(outLutFilepath, std::ios::out | std::ios::trunc);
    if (outfs.good())
    {
        try
        {
            optProcessor->write("Academy/ASC Common LUT Format", outfs);
        }
        catch (OCIO::Exception)
        {
            outfs.close();
            remove(outLutFilepath.c_str());
            throw;
        }

        outfs.close();
    }
    else
    {
        std::ostringstream oss;
        oss << "Could not open the file '"
            << outLutFilepath
            << "'."
            << std::endl;
        throw OCIO::Exception(oss.str().c_str());
    }
}

int main(int argc, const char ** argv)
{
    bool help = false, verbose = false, measure = false, listCSCColorSpaces = false;
    std::string cscColorSpace;

    ArgParse ap;
    ap.options("ociomakeclf -- Convert a LUT into CLF format and optionally add conversions from/to ACES2065-1 to make it an LMT.\n"
               "               If the csc argument is used, the CLF will contain the transforms:\n"
               "               [ACES2065-1 to CSC space] [the LUT] [CSC space to ACES2065-1].\n\n"
               "usage: ociomakeclf inLutFilepath outLutFilepath --csc cscColorSpace\n"
               "  or   ociomakeclf inLutFilepath outLutFilepath\n"
               "  or   ociomakeclf --list\n",
               "%*", parse_end_args, "",
               "<SEPARATOR>", "Options:",
               "--help",      &help,               "Print help message",
               "--verbose",   &verbose,            "Display general information",
               "--measure",   &measure,            "Measure (in ms) the CLF write",
               "--list",      &listCSCColorSpaces, "List of the supported CSC color spaces",
               "--csc %s",    &cscColorSpace,      "The color space that the input LUT expects and produces",
               nullptr);

    if (ap.parse(argc, argv) < 0)
    {
        std::cerr << std::endl << ap.geterror() << std::endl << std::endl;
        ap.usage();
        return 1;
    }

    if (help)
    {
        ap.usage();
        return 0;
    }

    // The LMT must accept and produce ACES2065-1 so look for all built-in transforms that produce 
    // that (based on the naming conventions).
    static constexpr char BuiltinSuffix[] = "_to_ACES2065-1";

    if (listCSCColorSpaces)
    {
        OCIO::ConstBuiltinTransformRegistryRcPtr registry = OCIO::BuiltinTransformRegistry::Get();

        std::cout << "The list of supported color spaces converting to ACES2065-1, is:";
        for (size_t idx = 0; idx < registry->getNumBuiltins(); ++idx)
        {
            std::string cscName = registry->getBuiltinStyle(idx);
            if (StringUtils::EndsWith(cscName, BuiltinSuffix))
            {
                cscName.resize(cscName.size() - strlen(BuiltinSuffix));
                std::cout << std::endl << "\t" << cscName;
            }
        }
        std::cout << std::endl << std::endl;

        return 0;
    }

    if (args.size() != 2)
    {
        std::cerr << "ERROR: Expecting 2 arguments, found " << args.size() << "." << std::endl;
        ap.usage();
        return 1;
    }
    
    const std::string inLutFilepath   = args[0].c_str();
    const std::string outLutFilepath  = args[1].c_str();

    const std::string originalCSC = cscColorSpace;

    if (!cscColorSpace.empty())
    {
        cscColorSpace += BuiltinSuffix;
    
        OCIO::ConstBuiltinTransformRegistryRcPtr registry = OCIO::BuiltinTransformRegistry::Get();

        bool cscFound = false;
        for (size_t idx = 0; idx < registry->getNumBuiltins() && !cscFound; ++idx)
        {
            if (StringUtils::Compare(cscColorSpace.c_str(), registry->getBuiltinStyle(idx)))
            {
                cscFound = true;

                // Save the builtin transform name with the right cases.
                cscColorSpace = registry->getBuiltinStyle(idx);
            }
        }

        if (!cscFound)
        {
            std::cerr << "ERROR: The LUT color space name '"
                      << originalCSC
                      << "' is not supported."
                      << std::endl;
            return 1;
        }
    }

    if (outLutFilepath.empty())
    {
        std::cerr << "ERROR: The output file path is missing." << std::endl;
        return 1;
    }
    else
    {
        const std::string filepath = StringUtils::Lower(outLutFilepath);
        if (!StringUtils::EndsWith(filepath, ".clf"))
        {
            std::cerr << "ERROR: The output LUT file path '"
                      << outLutFilepath
                      << "' must have a .clf extension."
                      << std::endl;
            return 1;
        }
    }

    if (verbose)
    {
        std::cout << "OCIO Version: " << OCIO::GetVersion() << std::endl;
    }

    try
    {
        if (verbose)
        {
            std::cout << "Building the transformation." << std::endl;
        }

        OCIO::GroupTransformRcPtr grp = OCIO::GroupTransform::Create();
        grp->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

        if (!cscColorSpace.empty())
        {
            std::string description;
            description += "ACES LMT transform built from a look LUT expecting color space: ";
            description += originalCSC;

            grp->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, description.c_str());
        }

        std::string description;
        description += "Original LUT name: ";
        description += inLutFilepath;
        grp->getFormatMetadata().addChildElement(OCIO::METADATA_DESCRIPTION, description.c_str());

        if (!cscColorSpace.empty())
        {
            // TODO: It should overwrite existing input and output descriptors if any.
            grp->getFormatMetadata().addChildElement(OCIO::METADATA_INPUT_DESCRIPTOR,  "ACES2065-1");
            grp->getFormatMetadata().addChildElement(OCIO::METADATA_OUTPUT_DESCRIPTOR, "ACES2065-1");
        }

        if (!cscColorSpace.empty())
        {
            // Create the color transformation from ACES2065-1 to the CSC color space.
            OCIO::BuiltinTransformRcPtr inBuiltin = OCIO::BuiltinTransform::Create();
            inBuiltin->setStyle(cscColorSpace.c_str());
            inBuiltin->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
            grp->appendTransform(inBuiltin);
        }

        // Create the file transform for the input LUT file.
        OCIO::FileTransformRcPtr file = OCIO::FileTransform::Create();
        file->setSrc(inLutFilepath.c_str());
        file->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
        file->setInterpolation(OCIO::INTERP_BEST);
        grp->appendTransform(file);

        if (!cscColorSpace.empty())
        {
            // Create the color transformation from the CSC color space to ACES2065-1.
            OCIO::BuiltinTransformRcPtr outBuiltin = OCIO::BuiltinTransform::Create();
            outBuiltin->setStyle(cscColorSpace.c_str());
            outBuiltin->setDirection(OCIO::TRANSFORM_DIR_FORWARD);
            grp->appendTransform(outBuiltin);
        }

        static constexpr char Msg[] = "Creating the CLF lut file";

        if (verbose && !measure)
        {
            std::cout << Msg << "." << std::endl;
        }

        if (measure)
        {
            Measure m(Msg);
            m.resume();

            // Create the CLF file.
            CreateOutputLutFile(outLutFilepath, grp);
        }
        else
        {
            // Create the CLF file.
            CreateOutputLutFile(outLutFilepath, grp);
        }
    }
    catch (OCIO::Exception & exception)
    {
        std::cerr << "ERROR: " << exception.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "ERROR: Unknown error encountered." << std::endl;
        return 1;
    }

    return 0; // Success.
}
