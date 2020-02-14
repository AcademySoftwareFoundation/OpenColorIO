// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "argparse.h"

void CustomLoggingFunction(const char * message)
{
    std::cout << message;
}

static std::string inputfile;
static int parsed = 0;
static std::vector<float> input;

static int parse_end_args(int argc, const char *argv[])
{
    if (parsed == 0)
    {
        inputfile = argv[0];
    }
    else if (parsed > 0)
    {
        input.push_back(strtof(argv[0], nullptr));
    }
    parsed += 1;

    return 0;
}

std::string ToString(float a_value)
{
    std::ostringstream outs;
    outs.precision(7);
    outs << std::defaultfloat << a_value;
    return outs.str();
}

void PrintFirstComponent(const std::string & in, const std::string & out)
{
    std::cout.width(std::max(in.size(), out.size()));
    std::cout << in;
}

void PrintAlignedVec(const std::vector<std::string> & str, const std::vector<std::string> & strAlign,
                     size_t comp)
{
    PrintFirstComponent(str[0], strAlign[0]);
    std::cout << " ";
    PrintFirstComponent(str[1], strAlign[1]);
    std::cout << " ";
    PrintFirstComponent(str[2], strAlign[2]);
    if (comp == 4)
    {
        std::cout << " ";
        PrintFirstComponent(str[3], strAlign[3]);
    }

}

void ToString(std::vector<std::string> & str, const std::vector<float> & vec, size_t index,
              size_t comp)
{
    str.push_back(ToString(vec[index + 0]));
    str.push_back(ToString(vec[index + 1]));
    str.push_back(ToString(vec[index + 2]));
    if (comp == 4)
    {
        str.push_back(ToString(vec[index + 3]));
    }
}

const char * DESC_STRING = "\n"
"OCIOCHECKLUT loads any LUT type supported by OCIO and prints any errors\n"
"encountered.  Provide a normalized RGB or RGBA value to send that through\n"
"the LUT.  Alternatively use the -t option to evaluate a set of test values.\n"
"Use -v to print warnings while parsing the LUT.\n";

int main (int argc, const char* argv[])
{
    bool verbose = false;
    bool printops = false;
    bool help = false;
    bool test = false;

    ArgParse ap;
    ap.options("ociochecklut -- check any LUT file and optionally convert a pixel\n\n"
               "usage:  ociochecklut <INPUTFILE> <R G B> or <R G B A>\n\n",
               "%*", parse_end_args, "",
               "-t", &test, "Test a set a predefined RGB values\n",
               "-p", &printops, "Print transform operators\n",
               "-v", &verbose, "Verbose\n",
               "-help", &help, "Print help message\n",
               NULL);

    if (ap.parse(argc, argv) < 0 || help || inputfile.empty())
    {
        std::cout << ap.geterror() << std::endl;
        ap.usage();
        std::cout << DESC_STRING;
        if (help)
        {
            // What are the allowed formats?
            std::cout << std::endl << "Formats supported:" << std::endl;
            const auto nbFromats = OCIO::FileTransform::getNumFormats();
            for (int i = 0; i < nbFromats; ++i)
            {
                std::cout << OCIO::FileTransform::getFormatNameByIndex(i);
                std::cout << " (." << OCIO::FileTransform::getFormatExtensionByIndex(i) << ")";
                std::cout << std::endl;
            }
        }
        return 1;
    }

    auto numInput = input.size();
    size_t comp = 3;
    if (numInput != 0)
    {
        if (numInput == 4)
        {
            comp = 4;
        }
        else if (numInput != 3)
        {
            std::cout << "Expecting either RGB or RGBA pixel (or use --rgb or --rgba options)."
                      << std::endl;
            return 1;
        }
    }

    if (verbose)
    {
        std::cout << std::endl;
        std::cout << "OCIO Version: " << OCIO::GetVersion() << std::endl;
    }

    OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);

    // By default, the OCIO log goes to std::cerr, so we also print any log messages associated
    // with reading the transform.
    OCIO::SetLoggingFunction(&CustomLoggingFunction);

    if (!inputfile.empty())
    {
        OCIO::ConfigRcPtr config = OCIO::Config::Create();

        // Create the OCIO processor for the specified transform.
        OCIO::FileTransformRcPtr t = OCIO::FileTransform::Create();
        t->setSrc(inputfile.c_str());
        t->setInterpolation(OCIO::INTERP_BEST);

        OCIO::ConstCPUProcessorRcPtr cpuProcessor;
        try
        {
            auto processor = config->getProcessor(t);
            if (printops)
            {
                auto transform = processor->createGroupTransform();
                std::cout << std::endl << "Transform operators: " << std::endl;
                const auto numTransforms = transform->getNumTransforms();
                for (int i = 0; i < numTransforms; ++i)
                {
                    std::cout << "\t" << *(transform->getTransform(i)) << std::endl;
                }
                if (numTransforms == 0)
                {
                    std::cout << "No transform." << std::endl;
                }
            }

            cpuProcessor = processor->getDefaultCPUProcessor();
        }
        catch (const OCIO::Exception & e)
        {
            std::cout << std::endl << "ERROR creating processor: " << e.what() << std::endl;
            return 1;
        }
        catch (...)
        {
            std::cout << std::endl << "Unknown ERROR creating processor" << std::endl;
            return 1;
        }

        bool validInput = true;
        size_t curPix = 0;

        static const std::vector<float> input4test = {
              0.f,   0.f,   0.f,
              0.18f, 0.18f, 0.18f,
              0.5f,  0.5f,  0.5f,
              1.f,   1.f,   1.f,
              2.f,   2.f,   2.f,
            100.f, 100.f, 100.f,
              1.f,   0.f,   0.f,
              0.f,   1.f,   0.f,
              0.f,   0.f,   1.f };

        if (verbose)
        {
            std::cout << std::endl;
        }

        while (validInput)
        {
            if (curPix < numInput)
            {
                std::vector<float> pixel = { input[curPix], input[curPix+1], input[curPix+2],
                                             comp == 3 ? 0.0f : input[curPix + 3] };
                try
                {
                    cpuProcessor->applyRGBA(pixel.data());
                }
                catch (const OCIO::Exception & e)
                {
                    std::cout << "\nERROR processing pixel: " << e.what() << std::endl;
                    return 1;
                }
                catch (...)
                {
                    std::cout << "\nUnknown processing pixel" << std::endl;
                    return 1;
                }

                // Print to string so that in & out values can be aligned if needed.
                std::vector<std::string> out;
                ToString(out, pixel, 0, comp);

                if (verbose)
                {
                    std::vector<std::string> in;
                    ToString(in, input, curPix, comp);

                    std::cout << "Input  [R G B";
                    if (comp == 4)
                    {
                        std::cout << " A";
                    }
                    std::cout << "]: [";
                    PrintAlignedVec(in, out, comp);
                    std::cout << "]" << std::endl;

                    std::cout << "Output [R G B";
                    if (comp == 4)
                    {
                        std::cout << " A";
                    }
                    std::cout << "]: [";
                    PrintAlignedVec(out, in, comp);
                    std::cout << "]" << std::endl;
                }
                else
                {
                    std::cout << out[0] << " " << out[1] << " " << out[2];
                    if (comp == 4)
                    {
                        std::cout << " " << out[3];
                    }
                    std::cout << std::endl;
                }
                curPix += comp;
            }
            else if (test)
            {
                if (verbose)
                {
                    std::cout << std::endl;
                    std::cout << "Testing with predefined set of RGB pixels." << std::endl;
                }
                input = input4test;
                comp = 3;
                numInput = input4test.size();
                curPix = 0;
                test = false;
            }
            else
            {
                validInput = false;
            }
        }
    }

    return 0;
}
