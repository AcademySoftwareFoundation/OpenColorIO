// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "apputils/argparse.h"

#ifdef OCIO_GPU_ENABLED
#include "oglapp.h"
#endif // OCIO_GPU_ENABLED

namespace
{

class ProcessorWrapper
{
public:
    ProcessorWrapper() = delete;
    ProcessorWrapper(const ProcessorWrapper &) = delete;
    ProcessorWrapper & operator=(const ProcessorWrapper &) = delete;

    explicit ProcessorWrapper(bool verbose)
        : m_verbose(verbose)
    {
    }

    ~ProcessorWrapper()
    {
#ifdef OCIO_GPU_ENABLED
        if (m_oglApp)
        {
            m_oglApp.reset();
        }
#endif // OCIO_GPU_ENABLED
    }

    void setCPU(OCIO::ConstCPUProcessorRcPtr cpu)
    {
        m_cpu = cpu;
    }

#ifdef OCIO_GPU_ENABLED
    void setGPU(OCIO::ConstGPUProcessorRcPtr gpu)
    {
        m_gpu = gpu;
        if (!m_oglApp)
        {
            m_oglApp = OCIO::OglApp::CreateOglApp("ociochecklut", 256, 20);

            if (m_verbose)
            {
                m_oglApp->printGLInfo();
            }
        }

        m_oglApp->setPrintShader(m_verbose);
        float image[4]{ 0.f, 0.f, 0.f, 0.f };
        m_oglApp->initImage(1, 1, OCIO::OglApp::COMPONENTS_RGBA, image);
        m_oglApp->createGLBuffers();
        OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_2);
        m_gpu->extractGpuShaderInfo(shaderDesc);
        m_oglApp->setShader(shaderDesc);
    }
#else
    void setGPU(OCIO::ConstGPUProcessorRcPtr)
    {
        m_verbose = false; // Avoid a warning.
    }
#endif // OCIO_GPU_ENABLED

    void apply(std::vector<float> & pixel)
    {
        if (m_cpu)
        {
            m_cpu->applyRGBA(pixel.data());
        }
        else
        {
            applyGPU(pixel);
        }
    }

private:

#ifdef OCIO_GPU_ENABLED
    void applyGPU(std::vector<float> & pixel)
    {
        m_oglApp->updateImage(pixel.data());
        m_oglApp->reshape(1, 1);
        m_oglApp->redisplay();
        m_oglApp->readImage(pixel.data());
    }
    OCIO::OglAppRcPtr m_oglApp;
#else
    void applyGPU(std::vector<float> &)
    {
    }
#endif // OCIO_GPU_ENABLED

    OCIO::ConstCPUProcessorRcPtr m_cpu;
    OCIO::ConstGPUProcessorRcPtr m_gpu;

    bool m_verbose;
};

void CustomLoggingFunction(const char * message)
{
    std::cout << message;
}

static std::string inputfile;
static int parsed = 0;
static std::vector<float> input;

static int parse_end_args(int /* argc */, const char *argv[])
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
"Otherwise, if no RGB value is provided, a list of the operators in the LUT is printed.\n"
"Use -v to print warnings while parsing the LUT.\n";

}

int main (int argc, const char* argv[])
{
    bool verbose       = false;
    bool help          = false;
    bool test          = false;
    bool invlut        = false;
    bool usegpu        = false;
    bool usegpuLegacy  = false;
    bool outputgpuInfo = false;
    bool stepInfo      = false;

    ArgParse ap;
    ap.options("ociochecklut -- check any LUT file and optionally convert a pixel\n\n"
               "usage:  ociochecklut <INPUTFILE> <R G B> or <R G B A>\n",
               "%*", parse_end_args, "",
               "<SEPARATOR>", "Options:",
               "-t", &test, "Test a set a predefined RGB values",
               "-v", &verbose, "Verbose",
               "-s", &stepInfo, "Print the output after each step in a multi - transform LUT",
               "--help", &help, "Print help message",
               "--inv", &invlut, "Apply LUT in inverse direction",
               "--gpu", &usegpu, "Use GPU instead of CPU",
               "--gpulegacy", &usegpuLegacy, "Use the legacy (i.e. baked) GPU color processing "
                                             "instead of the CPU one (--gpu is ignored)",
               "--gpuinfo", &outputgpuInfo, "Output the OCIO shader program",
               nullptr);

    if (ap.parse(argc, argv) < 0 || help || inputfile.empty())
    {
        std::cout << ap.geterror() << std::endl;
        ap.usage();
        std::cout << DESC_STRING << std::endl;
        if (help)
        {
            // What are the allowed formats?
            std::cout << "Formats supported:" << std::endl;
            const auto nbFormats = OCIO::FileTransform::GetNumFormats();
            for (int i = 0; i < nbFormats; ++i)
            {
                std::cout << OCIO::FileTransform::GetFormatNameByIndex(i);
                std::cout << " (." << OCIO::FileTransform::GetFormatExtensionByIndex(i) << ")";
                std::cout << std::endl;
            }
            return 0;
        }

        return 1;
    }

    if (verbose)
    {
        std::cout << std::endl;
        std::cout << "OCIO Version: " << OCIO::GetVersion() << std::endl;
    }

#ifndef OCIO_GPU_ENABLED
    if (usegpu || outputgpuInfo || usegpuLegacy)
    {
        std::cerr << "Compiled without OpenGL support, GPU options are not available.";
        std::cerr << std::endl;
        return 1;
    }
#endif // OCIO_GPU_ENABLED

    OCIO::SetLoggingLevel(OCIO::LOGGING_LEVEL_WARNING);

    // By default, the OCIO log goes to std::cerr, so we also print any log messages associated
    // with reading the transform.
    OCIO::SetLoggingFunction(&CustomLoggingFunction);

    const bool printops = input.empty() && !test;

    if (!inputfile.empty())
    {
        OCIO::ConfigRcPtr config = OCIO::Config::Create();

        // Create the OCIO processor for the specified transform.
        OCIO::FileTransformRcPtr t = OCIO::FileTransform::Create();
        t->setSrc(inputfile.c_str());
        t->setInterpolation(OCIO::INTERP_BEST);
        t->setDirection(invlut ? OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD);

        ProcessorWrapper proc(outputgpuInfo);
        try
        {
            auto processor = config->getProcessor(t);
            if (printops)
            {
                auto transform = processor->createGroupTransform();
                std::cout << "Transform operators: " << std::endl;
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
            if (usegpu || usegpuLegacy)
            {
                proc.setGPU(usegpuLegacy ? processor->getOptimizedLegacyGPUProcessor(OCIO::OPTIMIZATION_DEFAULT, 32)
                                         : processor->getDefaultGPUProcessor());
            }
            else
            {
                proc.setCPU(processor->getDefaultCPUProcessor());
            }
        }
        catch (const OCIO::Exception & exception)
        {
            std::cerr << "ERROR: " << exception.what() << std::endl;
            return 1;
        }
        catch (...)
        {
            std::cerr << "ERROR: Unknown error encountered while creating processor." << std::endl;
            return 1;
        }

        if (printops)
        {
            // Only displays the LUT file content.
            return 0;
        }

        // Validate the input values.

        size_t numInput = input.size();

        if (test && numInput > 0)
        {
            std::cerr << "ERROR: Expecting either RGB (or RGBA) pixel or predefined RGB values (i.e. -t)."
                      << std::endl;
            return 1;
        }            

        size_t comp = 3;
        if (numInput == 4)
        {
            comp = 4;
        }
        else if (numInput != 3 && !test)
        {
            std::cerr << "ERROR: Expecting either RGB or RGBA pixel."
                      << std::endl;
            return 1;
        }

        // Process the input values.

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

        if (verbose || stepInfo)
        {
            std::cout << std::endl;
        }

        while (validInput)
        {
            if (curPix < numInput)
            {
                std::vector<float> pixel = { input[curPix], input[curPix+1], input[curPix+2],
                                             comp == 3 ? 0.0f : input[curPix + 3] };

                if (stepInfo)
                {
                    // Process each step in a multi - transform LUT
                    try
                    {
                        // Create GroupTransform so that each can be processed one at a time. 
                        auto processor = config->getProcessor(t);
                        auto transform = processor->createGroupTransform();
                        std::vector<float> inputPixel = pixel;
                        std::vector<float> outputPixel = pixel;
                        const auto numTransforms = transform->getNumTransforms();
                        
                        std::cout << std::endl;

                        for (int i = 0; i < numTransforms; ++i)
                        {
                            auto transformStep = transform->getTransform(i);
                            auto processorStep = config->getProcessor(transformStep);

                            if (usegpu || usegpuLegacy)
                            {
                                proc.setGPU(usegpuLegacy ? processorStep->getOptimizedLegacyGPUProcessor(OCIO::OPTIMIZATION_DEFAULT, 32)
                                    : processorStep->getDefaultGPUProcessor());
                            }
                            else
                            {
                                proc.setCPU(processorStep->getDefaultCPUProcessor());
                            }
                            
                            // Process the pixel
                            proc.apply(outputPixel);

                            // Print the input/output pixel
                            std::vector<std::string> in;
                            ToString(in, inputPixel, 0, comp);

                            std::vector<std::string> out;
                            ToString(out, outputPixel, 0, comp);

                            std::cout << "\n" << *(transform->getTransform(i)) << std::endl;
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

                            inputPixel = outputPixel;         
                        }
                    }
                    catch (const OCIO::Exception& exception)
                    {
                        std::cerr << "ERROR: " << exception.what() << std::endl;
                        return 1;
                    }
                    catch (...)
                    {
                        std::cerr << "ERROR: Unknown error encountered while processing single step operator." << std::endl;
                        return 1;
                    }

                    curPix += comp;
                }
                else
                {
                    // Process in a single step
                    try
                    {
                        proc.apply(pixel);
                    }
                    catch (const OCIO::Exception& e)
                    {
                        std::cerr << "ERROR: Processing pixel: " << e.what() << std::endl;
                        return 1;
                    }
                    catch (...)
                    {
                        std::cerr << "ERROR: Unknown error encountered while processing pixel." << std::endl;
                        return 1;
                    }

                    // Print to string so that in & out values can be aligned if needed.

                    std::vector<std::string> out;
                    ToString(out, pixel, 0, comp);

                    std::cout << std::endl;

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
            }
            else if (test)
            {
                if (verbose)
                {
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
