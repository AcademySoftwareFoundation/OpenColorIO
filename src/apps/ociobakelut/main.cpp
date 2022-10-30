// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "apputils/argparse.h"
#include "ocioicc.h"


static std::string outputfile;

static int parse_end_args(int argc, const char *argv[])
{
    if(argc>0)
    {
        outputfile = argv[0];
    }

    return 0;
}

OCIO::GroupTransformRcPtr parse_luts(int argc, const char *argv[]);

int main (int argc, const char* argv[])
{

    bool help = false;
    int cubesize = -1;
    int shapersize = -1; // cubsize^2
    std::string format;
    std::string inputconfig;
    std::string inputspace;
    std::string shaperspace;
    std::string looks;
    std::string outputspace;
    std::string display;
    std::string view;
    bool usestdout = false;
    bool verbose = false;

    int whitepointtemp = 6505;
    std::string displayicc;
    std::string description;
    std::string copyright = "No copyright. Use freely.";

    // What are the allowed baker output formats?
    std::ostringstream formats;
    formats << "the LUT format to bake: ";
    for(int i=0; i<OCIO::Baker::getNumFormats(); ++i)
    {
        if(i!=0) formats << ", ";
        formats << OCIO::Baker::getFormatNameByIndex(i);
        formats << " (." << OCIO::Baker::getFormatExtensionByIndex(i) << ")";
    }
    formats << ", icc (.icc)";

    std::string formatstr = formats.str();

    std::string dummystr;
    float dummyf1, dummyf2, dummyf3;

    ArgParse ap;
    ap.options("ociobakelut -- create a new LUT or ICC profile from an OCIO config or LUT file(s)\n\n"
               "usage:  ociobakelut [options] <OUTPUTFILE.LUT>\n\n"
               "example:  ociobakelut --inputspace lg10 --outputspace srgb8 --format flame lg_to_srgb.3dl\n"
               "example:  ociobakelut --lut filmlut.3dl --lut calibration.3dl --format flame display.3dl\n"
               "example:  ociobakelut --cccid 0 --lut cdlgrade.ccc --lut calibration.3dl --format flame graded_display.3dl\n"
               "example:  ociobakelut --lut look.3dl --offset 0.01 -0.02 0.03 --lut display.3dl --format flame display_with_look.3dl\n"
               "example:  ociobakelut --inputspace lg10 --outputspace srgb8 --format icc ~/Library/ColorSync/Profiles/test.icc\n"
               "example:  ociobakelut --inputspace lin --shaperspace lg10 --outputspace lg10 --format spi1d lintolog.spi1d\n"
               "example:  ociobakelut --inputspace lg10 --displayview sRGB Film --format spi3d display_view.spi3d\n"
               "example:  ociobakelut --lut filmlut.3dl --lut calibration.3dl --format icc ~/Library/ColorSync/Profiles/test.icc\n\n",
               "%*", parse_end_args, "",
               "<SEPARATOR>", "Using Existing OCIO Configurations",
               "<SEPARATOR>", "    (use either displayview or outputspace, but not both)",
               "--inputspace %s", &inputspace, "Input OCIO ColorSpace (or Role)",
               "--displayview %s %s", &display, &view, "Output OCIO Display and View",
               "--outputspace %s", &outputspace, "Output OCIO ColorSpace (or Role)",
               "--shaperspace %s", &shaperspace, "the OCIO ColorSpace or Role, for the shaper",
               "--looks %s", &looks, "the OCIO looks to apply",
               "--iconfig %s", &inputconfig, "Input .ocio configuration file (default: $OCIO)\n",
               "<SEPARATOR>", "Config-Free LUT Baking",
               "<SEPARATOR>", "    (all options can be specified multiple times, each is applied in order)",
               "--cccid %s", &dummystr, "Specify a CCCId for any following LUTs",
               "--lut %s", &dummystr, "Specify a LUT (forward direction)",
               "--invlut %s", &dummystr, "Specify a LUT (inverse direction)",
               "--slope %f %f %f", &dummyf1, &dummyf2, &dummyf3, "slope",
               "--offset %f %f %f", &dummyf1, &dummyf2, &dummyf3, "offset (float)",
               "--offset10 %f %f %f", &dummyf1, &dummyf2, &dummyf3, "offset (10-bit)",
               "--power %f %f %f", &dummyf1, &dummyf2, &dummyf3, "power",
               "--sat %f", &dummyf1, "saturation (ASC-CDL luma coefficients)\n",
               "<SEPARATOR>", "Baking Options",
               "--format %s", &format, formatstr.c_str(),
               "--shapersize %d", &shapersize, "size of the shaper (default: format specific)",
               "--cubesize %d", &cubesize, "size of the main LUT (3d or 1d) (default: format specific)",
               "--stdout", &usestdout, "Write to stdout (rather than file)",
               "--v", &verbose, "Verbose",
               "--help", &help, "Print help message\n",
               "<SEPARATOR>", "ICC Options",
               //"--cubesize %d", &cubesize, "size of the ICC CLUT cube (default: 32)",
               "--whitepoint %d", &whitepointtemp, "whitepoint for the profile (default: 6505)",
               "--displayicc %s", &displayicc , "an ICC profile which matches the OCIO profiles target display",
               "--description %s", &description , "a meaningful description, this will show up in UI like photoshop (defaults to \"filename.icc\")",
               "--copyright %s", &copyright , "a copyright field added in the file (default: \"No copyright. Use freely.\")\n",
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

    // If we're printing to stdout, disable verbose printouts
    if(usestdout)
    {
        verbose = false;
    }

    // Create the OCIO processor for the specified transform.
    OCIO::ConstConfigRcPtr config;

    OCIO::GroupTransformRcPtr groupTransform;

    try
    {
        groupTransform = parse_luts(argc, argv);
    }
    catch(const OCIO::Exception & e)
    {
        std::cerr << "\nERROR: " << e.what() << std::endl;
        std::cerr << "See --help for more info." << std::endl;
        return 1;
    }
    catch(...)
    {
        std::cerr << "\nERROR: An unknown error occurred in parse_luts" << std::endl;
        std::cerr << "See --help for more info." << std::endl;
        return 1;
    }

    if(!groupTransform)
    {
        std::cerr << "\nERROR: parse_luts returned null transform" << std::endl;
        std::cerr << "See --help for more info." << std::endl;
        return 1;
    }

    // If --luts have been specified, synthesize a new (temporary) configuration
    // with the transformation embedded in a colorspace.
    if(groupTransform->getNumTransforms() > 0)
    {
        if(!inputspace.empty())
        {
            std::cerr << "\nERROR: --inputspace is not allowed when using --lut\n\n";
            std::cerr << "See --help for more info." << std::endl;
            return 1;
        }
        if(!outputspace.empty())
        {
            std::cerr << "\nERROR: --outputspace is not allowed when using --lut\n\n";
            std::cerr << "See --help for more info." << std::endl;
            return 1;
        }
        if(!looks.empty())
        {
            std::cerr << "\nERROR: --looks is not allowed when using --lut\n\n";
            std::cerr << "See --help for more info." << std::endl;
            return 1;
        }
        if(!shaperspace.empty())
        {
            std::cerr << "\nERROR: --shaperspace is not allowed when using --lut\n\n";
            std::cerr << "See --help for more info." << std::endl;
            return 1;
        }
        if(!display.empty() || !view.empty())
        {
            std::cerr << "\nERROR: --displayview is not allowed when using --lut\n\n";
            std::cerr << "See --help for more info." << std::endl;
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

        outputColorSpace->setTransform(groupTransform,
            OCIO::COLORSPACE_DIR_FROM_REFERENCE);

        if(verbose)
        {
            std::cout << "[OpenColorIO DEBUG]: Specified Transform:";
            std::cout << *groupTransform;
            std::cout << "\n";
        }

        editableConfig->addColorSpace(outputColorSpace);
        config = editableConfig;
    }
    else
    {

        if(inputspace.empty())
        {
            std::cerr << "\nERROR: You must specify the --inputspace.\n\n";
            std::cerr << "See --help for more info." << std::endl;
            return 1;
        }

        if(outputspace.empty() && (display.empty() && view.empty()))
        {
            std::cerr << "\nERROR: You must specify either --outputspace or --displayview.\n\n";
            std::cerr << "See --help for more info." << std::endl;
            return 1;
        }

        if(display.empty() ^ view.empty())
        {
            std::cerr << "\nERROR: You must specify both display and view with --displayview.\n\n";
            std::cerr << "See --help for more info." << std::endl;
            return 1;
        }

        if(format.empty())
        {
            std::cerr << "\nERROR: You must specify the LUT format using --format.\n\n";
            std::cerr << "See --help for more info." << std::endl;
            return 1;
        }

        if(!inputconfig.empty())
        {
            if(!usestdout && verbose)
                std::cout << "[OpenColorIO INFO]: Loading " << inputconfig << std::endl;
            config = OCIO::Config::CreateFromFile(inputconfig.c_str());
        }
        else if(OCIO::GetEnvVariable("OCIO"))
        {
            if(!usestdout && verbose)
            {
                std::cout << "[OpenColorIO INFO]: Loading $OCIO " 
                          << OCIO::GetEnvVariable("OCIO") << std::endl;
            }
            config = OCIO::Config::CreateFromEnv();
        }
        else
        {
            std::cerr << "ERROR: You must specify an input OCIO configuration ";
            std::cerr << "(either with --iconfig or $OCIO).\n\n";
            ap.usage ();
            return 1;
        }
    }

    if(outputfile.empty() && !usestdout)
    {
        std::cerr << "\nERROR: You must specify the outputfile or --stdout.\n\n";
        std::cerr << "See --help for more info." << std::endl;
        return 1;
    }

    try
    {
        // This could be moved to OCIO core by adding baking support for ICC
        // file format, here at the expense of adding the dependency to lcms2.
        if(format == "icc")
        {
            if(description.empty())
            {
                description = outputfile;
                if(verbose)
                    std::cout << "[OpenColorIO INFO]: \"--description\" set to default value of filename.icc: " << outputfile << "" << std::endl;
            }

            if(usestdout)
            {
                std::cerr << "\nERROR: --stdout not supported when writing ICC profiles.\n\n";
                std::cerr << "See --help for more info." << std::endl;
                return 1;
            }

            if(outputfile.empty())
            {
                std::cerr << "ERROR: you need to specify a output ICC path\n";
                std::cerr << "See --help for more info." << std::endl;
                return 1;
            }

            if(!shaperspace.empty())
            {
                std::cerr << "WARNING: shaperspace is ignored when generating ICC profiles.\n";
            }

            if(cubesize<2) cubesize = 32; // default

            OCIO::ConstProcessorRcPtr processor;
            if (!display.empty() && !view.empty())
            {
                OCIO::DisplayViewTransformRcPtr transform = OCIO::DisplayViewTransform::Create();
                transform->setSrc(inputspace.c_str());
                transform->setDisplay(display.c_str());
                transform->setView(view.c_str());

                OCIO::LegacyViewingPipelineRcPtr vp = OCIO::LegacyViewingPipeline::Create();
                vp->setDisplayViewTransform(transform);
                vp->setLooksOverrideEnabled(!looks.empty());
                vp->setLooksOverride(looks.c_str());

                processor = vp->getProcessor(config);
            }
            else
            {
                OCIO::LookTransformRcPtr transform = OCIO::LookTransform::Create();
                transform->setLooks(!looks.empty() ? looks.c_str() : "");
                transform->setSrc(inputspace.c_str());
                transform->setDst(outputspace.c_str());
                processor = config->getProcessor(transform, OCIO::TRANSFORM_DIR_FORWARD);
            }

            OCIO::ConstCPUProcessorRcPtr cpuprocessor = processor->getOptimizedCPUProcessor(OCIO::OPTIMIZATION_LOSSLESS);

            SaveICCProfileToFile(outputfile,
                                 cpuprocessor,
                                 cubesize,
                                 whitepointtemp,
                                 displayicc,
                                 description,
                                 copyright,
                                 verbose);
        }
        else
        {

            OCIO::BakerRcPtr baker = OCIO::Baker::Create();

            // setup the baker for our LUT type
            baker->setConfig(config);
            baker->setFormat(format.c_str());
            baker->setInputSpace(inputspace.c_str());
            baker->setShaperSpace(shaperspace.c_str());
            baker->setLooks(looks.c_str());
            baker->setTargetSpace(outputspace.c_str());
            baker->setDisplayView(display.c_str(), view.c_str());
            if(shapersize!=-1) baker->setShaperSize(shapersize);
            if(cubesize!=-1) baker->setCubeSize(cubesize);

            // output LUT
            std::ostringstream output;

            if(!usestdout && verbose)
                std::cout << "[OpenColorIO INFO]: Baking '" << format << "' LUT" << std::endl;

            if(usestdout)
            {
                baker->bake(std::cout);
            }
            else
            {
                std::ofstream f(outputfile.c_str());
                if(f.fail())
                {
                    std::cerr << "ERROR: Non-writable file path " << outputfile << " specified." << std::endl;
                    return 1;
                }
                baker->bake(f);
                if(verbose)
                    std::cout << "[OpenColorIO INFO]: Wrote '" << outputfile << "'" << std::endl;
            }
        }
    }
    catch(OCIO::Exception & exception)
    {
        std::cerr << "OCIO Error: " << exception.what() << std::endl;
        std::cerr << "See --help for more info." << std::endl;
        return 1;
    }
    catch (std::exception& exception)
    {
        std::cerr << "Error: " << exception.what() << "\n";
        std::cerr << "See --help for more info." << std::endl;
        return 1;
    }
    catch(...)
    {
        std::cerr << "Unknown OCIO error encountered." << std::endl;
        std::cerr << "See --help for more info." << std::endl;
        return 1;
    }

    return 0;
}


// TODO: Replace this dirty argument parsing code with a clean version
// that leverages the same codepath for the standard arguments.  If
// the OIIO derived argparse does not suffice, options we may want to consider
// include simpleopt, tclap, ultraopt

// TODO: Use better input validation, instead of atof.
// If too few arguments are provides for scale (let's say only two) and
// the following argument is the start of another flag (let's say "--invlut")
// then atof() will likely try to convert "--invlut" to its double equivalent,
// resulting in an invalid (or at least undesired) scale value.

OCIO::GroupTransformRcPtr parse_luts(int argc, const char *argv[])
{
    OCIO::GroupTransformRcPtr groupTransform = OCIO::GroupTransform::Create();
    const char *lastCCCId = NULL; // Ugly to use this but using GroupTransform::getTransform()
                                  // returns a const object so we must set this
                                  // prior to using --lut for now.

    for(int i=0; i<argc; ++i)
    {
        std::string arg(argv[i]);

        if(arg == "--lut" || arg == "-lut")
        {
            if(i+1>=argc)
            {
                throw OCIO::Exception("Error parsing --lut. Invalid num args");
            }

            OCIO::FileTransformRcPtr t = OCIO::FileTransform::Create();
            t->setSrc(argv[i+1]);
            t->setInterpolation(OCIO::INTERP_BEST);
            if (lastCCCId)
            {
                t->setCCCId(lastCCCId);
            }
            groupTransform->appendTransform(t);

            i += 1;
        }
        else if(arg == "--cccid" || arg == "-cccid")
        {
            if(i+1>=argc)
            {
                throw OCIO::Exception("Error parsing --cccid. Invalid num args");
            }

            lastCCCId = argv[i+1];

            i += 1;
        }
        else if(arg == "--invlut" || arg == "-invlut")
        {
            if(i+1>=argc)
            {
                throw OCIO::Exception("Error parsing --invlut. Invalid num args");
            }

            OCIO::FileTransformRcPtr t = OCIO::FileTransform::Create();
            t->setSrc(argv[i+1]);
            t->setInterpolation(OCIO::INTERP_BEST);
            t->setDirection(OCIO::TRANSFORM_DIR_INVERSE);
            groupTransform->appendTransform(t);

            i += 1;
        }
        else if(arg == "--slope" || arg == "-slope")
        {
            if(i+3>=argc)
            {
                throw OCIO::Exception("Error parsing --slope. Invalid num args");
            }

            OCIO::CDLTransformRcPtr t = OCIO::CDLTransform::Create();

            double scale[3];
            scale[0] = atof(argv[i+1]);
            scale[1] = atof(argv[i+2]);
            scale[2] = atof(argv[i+3]);
            t->setSlope(scale);
            groupTransform->appendTransform(t);

            i += 3;
        }
        else if(arg == "--offset" || arg == "-offset")
        {
            if(i+3>=argc)
            {
                throw OCIO::Exception("Error parsing --offset. Invalid num args");
            }

            OCIO::CDLTransformRcPtr t = OCIO::CDLTransform::Create();

            double offset[3];
            offset[0] = atof(argv[i+1]);
            offset[1] = atof(argv[i+2]);
            offset[2] = atof(argv[i+3]);
            t->setOffset(offset);
            groupTransform->appendTransform(t);

            i += 3;
        }
        else if(arg == "--offset10" || arg == "-offset10")
        {
            if(i+3>=argc)
            {
                throw OCIO::Exception("Error parsing --offset10. Invalid num args");
            }

            OCIO::CDLTransformRcPtr t = OCIO::CDLTransform::Create();

            double offset[3];
            offset[0] = atof(argv[i+1]) / 1023.0;
            offset[1] = atof(argv[i+2]) / 1023.0;
            offset[2] = atof(argv[i+3]) / 1023.0;
            t->setOffset(offset);
            groupTransform->appendTransform(t);
            i += 3;
        }
        else if(arg == "--power" || arg == "-power")
        {
            if(i+3>=argc)
            {
                throw OCIO::Exception("Error parsing --power. Invalid num args");
            }

            OCIO::CDLTransformRcPtr t = OCIO::CDLTransform::Create();

            double power[3];
            power[0] = atof(argv[i+1]);
            power[1] = atof(argv[i+2]);
            power[2] = atof(argv[i+3]);
            t->setPower(power);
            groupTransform->appendTransform(t);

            i += 3;
        }
        else if(arg == "--sat" || arg == "-sat")
        {
            if(i+1>=argc)
            {
                throw OCIO::Exception("Error parsing --sat. Invalid num args");
            }

            OCIO::CDLTransformRcPtr t = OCIO::CDLTransform::Create();
            t->setSat(atof(argv[i+1]));
            groupTransform->appendTransform(t);

            i += 1;
        }
    }

    return groupTransform;
}
