// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "apputils/argparse.h"

#ifdef OCIO_GPU_ENABLED
#include "oglapp.h"
#if __APPLE__
#include "metalapp.h"
#endif
#endif // OCIO_GPU_ENABLED

#include "imageio.h"

// Array of non OpenColorIO arguments.
static std::vector<std::string> args;

// Fill 'args' array with OpenColorIO arguments.
static int parse_end_args(int argc, const char *argv[])
{
    while (argc>0)
    {
        args.push_back(argv[0]);
        argc--;
        argv++;
    }

    return 0;
}

bool ParseNameValuePair(std::string& name, std::string& value, const std::string& input);

bool StringToFloat(float * fval, const char * str);

bool StringToInt(int * ival, const char * str);

int main(int argc, const char **argv)
{
    ArgParse ap;

    std::vector<std::string> floatAttrs;
    std::vector<std::string> intAttrs;
    std::vector<std::string> stringAttrs;

    std::string outputDepth;
    std::string inputconfig;

    bool usegpu                 = false;
#if __APPLE__
    bool useMetal = false;
#endif
    bool usegpuLegacy           = false;
    bool outputgpuInfo          = false;
    bool verbose                = false;
    bool help                   = false;
    bool useLut                 = false;
    bool useDisplayView         = false;
    bool useInvertView          = false;
    bool useNamedTransform      = false;
    bool useInvNamedTransform   = false;

    ap.options("ocioconvert -- apply colorspace transform to an image \n\n"
               "usage: ocioconvert [options] inputimage inputcolorspace outputimage outputcolorspace\n"
               "   or: ocioconvert [options] --lut lutfile inputimage outputimage\n"
               "   or: ocioconvert [options] --view inputimage inputcolorspace outputimage displayname viewname\n"
               "   or: ocioconvert [options] --invertview inputimage displayname viewname outputimage outputcolorspace\n"
               "   or: ocioconvert [options] --namedtransform transformname inputimage outputimage\n"
               "   or: ocioconvert [options] --invnamedtransform transformname inputimage outputimage\n\n",
               "%*", parse_end_args, "",
               "<SEPARATOR>", "Options:",
               "--lut",                 &useLut,                "Convert using a LUT rather than a config file",
               "--view",                &useDisplayView,        "Convert to a (display,view) pair rather than to "
                                                                "an output color space",
               "--invertview",          &useInvertView,         "Convert from a (display,view) pair rather than "
                                                                "from a color space",
               "--namedtransform",      &useNamedTransform,     "Convert using a named transform in the forward direction",
               "--invnamedtransform",   &useInvNamedTransform,  "Convert using a named transform in the inverse direction",
               "--gpu",                 &usegpu,                "Use GPU color processing instead of CPU (CPU is the default)",
#if __APPLE__
               "--metal",               &useMetal,              "Use Metal",
#endif
               "--gpulegacy",           &usegpuLegacy,          "Use the legacy (i.e. baked) GPU color processing "
                                                                "instead of the CPU one (--gpu is ignored)",
               "--gpuinfo",             &outputgpuInfo,         "Output the OCIO shader program",
               "--h",                   &help,                  "Display the help and exit",
               "--help",                &help,                  "Display the help and exit",
               "-v" ,                   &verbose,               "Display general information",
              "--iconfig %s",           &inputconfig,           "Input .ocio configuration file (default: $OCIO)",
               "<SEPARATOR>", "\nOpenImageIO or OpenEXR options:",
               "--bitdepth %s",         &outputDepth,  "Output image bitdepth",
               "--float-attribute %L",  &floatAttrs,   "\"name=float\" pair defining OIIO float attribute "
                                                       "for outputimage",
               "--int-attribute %L",    &intAttrs,     "\"name=int\" pair defining an int attribute "
                                                       "for outputimage",
               "--string-attribute %L", &stringAttrs,  "\"name=string\" pair defining a string attribute "
                                                       "for outputimage",
               NULL
               );

    if (ap.parse (argc, argv) < 0)
    {
        std::cerr << ap.geterror() << std::endl;
        ap.usage ();
        exit(1);
    }

    if (help)
    {
        ap.usage();
        return 0;
    }

#ifndef OCIO_GPU_ENABLED
    if (usegpu || outputgpuInfo || usegpuLegacy)
    {
        std::cerr << "Compiled without OpenGL support, GPU options are not available.";
        std::cerr << std::endl;
        exit(1);
    }
#endif // OCIO_GPU_ENABLED

    OCIO::BitDepth userOutputBitDepth = OCIO::BIT_DEPTH_UNKNOWN;
    if (!outputDepth.empty())
    {
        if (outputDepth == "uint8")
        {
            userOutputBitDepth = OCIO::BIT_DEPTH_UINT8;
        }
        else if (outputDepth == "uint16")
        {
            userOutputBitDepth = OCIO::BIT_DEPTH_UINT16;
        }
        else if (outputDepth == "half")
        {
            userOutputBitDepth = OCIO::BIT_DEPTH_F16;
        }
        else if (outputDepth == "float")
        {
            userOutputBitDepth = OCIO::BIT_DEPTH_F32;
        }
        else
        {
            throw OCIO::Exception("Unsupported output bitdepth, must be uint8, uint16, half or float.");
        }
    }

    const char * inputimage         = nullptr;
    const char * inputcolorspace    = nullptr;
    const char * outputimage        = nullptr;
    const char * outputcolorspace   = nullptr;
    const char * lutFile            = nullptr;
    const char * display            = nullptr;
    const char * view               = nullptr;
    const char * namedtransform     = nullptr;
 
    if (!useLut && !useDisplayView && !useInvertView && !useNamedTransform && !useInvNamedTransform)
    {
        if (args.size() != 4)
        {
            std::cerr << "ERROR: Expecting 4 arguments, found " 
                      << args.size() << "." << std::endl;
            ap.usage();
            exit(1);
        }
        inputimage       = args[0].c_str();
        inputcolorspace  = args[1].c_str();
        outputimage      = args[2].c_str();
        outputcolorspace = args[3].c_str();
    }
    else if (useLut && useDisplayView)
    {
        std::cerr << "ERROR: Options lut & view can't be used at the same time." << std::endl;
        ap.usage();
        exit(1);
    }
    else if (useLut)
    {
        if (args.size() != 3)
        {
            std::cerr << "ERROR: Expecting 3 arguments for --lut option, found "
                      << args.size() << "." << std::endl;
            ap.usage();
            exit(1);
        }
        lutFile     = args[0].c_str();
        inputimage  = args[1].c_str();
        outputimage = args[2].c_str();
    }
    else if (useDisplayView)
    {
        if (args.size() != 5)
        {
            std::cerr << "ERROR: Expecting 5 arguments for --view option, found "
                      << args.size() << "." << std::endl;
            ap.usage();
            exit(1);
        }
        inputimage      = args[0].c_str();
        inputcolorspace = args[1].c_str();
        outputimage     = args[2].c_str();
        display         = args[3].c_str();
        view            = args[4].c_str();
    }
    else if (useDisplayView && useInvertView)
    {
        std::cerr << "ERROR: Options view & invertview can't be used at the same time." << std::endl;
        ap.usage();
        exit(1);
    }
    else if (useInvertView) 
    {
        if (args.size() != 5)
        {
            std::cerr << "ERROR: Expecting 5 arguments for --invertview option, found "
                      << args.size() << "." << std::endl;
            ap.usage();
            exit(1);
        }
        inputimage          = args[0].c_str();
        display             = args[1].c_str();
        view                = args[2].c_str();
        outputimage         = args[3].c_str();
        outputcolorspace    = args[4].c_str();
    }
    else if (useNamedTransform)
    {
        if (useLut || useDisplayView || useInvertView || useInvNamedTransform)
        {
            std::cerr << "ERROR: Option namedtransform can't be used with lut, view, invertview, \
                or invnamedtransform at the same time." << std::endl;
            ap.usage();
            exit(1);
        }
        
        if (args.size() != 3)
        {
            std::cerr << "ERROR: Expecting 3 arguments for --namedtransform option, found "
                << args.size() << "." << std::endl;
            ap.usage();
            exit(1);
        }
        
        namedtransform  = args[0].c_str();
        inputimage      = args[1].c_str();
        outputimage     = args[2].c_str();
    }
    else if (useInvNamedTransform)
    {
        if (useLut || useDisplayView || useInvertView || useNamedTransform)
        {
            std::cerr << "ERROR: Option invnamedtransform can't be used with lut, view, invertview, \
                or namedtransform at the same time." << std::endl;
            ap.usage();
            exit(1);
        }

        if (args.size() != 3)
        {
            std::cerr << "ERROR: Expecting 3 arguments for --invnamedtransform option, found "
                << args.size() << "." << std::endl;
            ap.usage();
            exit(1);
        }

        namedtransform  = args[0].c_str();
        inputimage      = args[1].c_str();
        outputimage     = args[2].c_str();
    }

    // Load the current config.
    OCIO::ConstConfigRcPtr config;
    
    try
    {
        if (useLut)
        {
            config = OCIO::Config::CreateRaw();
        }
        else if (!inputconfig.empty() )
        {
            config = OCIO::Config::CreateFromFile(inputconfig.c_str());
        }
        else
        {
            const char * env = OCIO::GetEnvVariable("OCIO");
            if (env && *env)
                inputconfig = env;
            config = OCIO::GetCurrentConfig();
        }
    }
    catch (const OCIO::Exception & e)
    {
        std::cout << "ERROR loading config file: " << e.what() << std::endl;
        exit(1);
    }
    catch (...)
    {

        std::cerr << "ERROR loading config file: '" << inputconfig << "'" << std::endl;
        exit(1);
    }

    if (verbose)
    {
        std::cout << std::endl;
        std::cout << OCIO::ImageIO::GetVersion() << std::endl;
        std::cout << "OCIO Version: " << OCIO::GetVersion() << std::endl;
        if (!useLut)
        {
            std::cout << std::endl;
            std::cout << "OCIO Config. file:    '" << inputconfig << "'" << std::endl;
            std::cout << "OCIO Config. version: " << config->getMajorVersion() << "." 
                                                  << config->getMinorVersion() << std::endl;
            std::cout << "OCIO search_path:     " << config->getSearchPath() << std::endl;
        }
    }

    if (usegpuLegacy)
    {
        std::cout << std::endl;
        std::cout << "Using legacy OCIO v1 GPU color processing." << std::endl;
    }
    else if (usegpu)
    {
        std::cout << std::endl;
        std::cout << "Using GPU color processing." << std::endl;
    }

    OCIO::ImageIO imgInput;
    OCIO::ImageIO imgOutputCPU;
    // Default is to perform in-place conversion.
    OCIO::ImageIO *imgOutput = &imgInput;

    // Load the image.
    std::cout << std::endl;
    std::cout << "Loading " << inputimage << std::endl;
    try
    {
        if (usegpu || usegpuLegacy)
        {
            imgInput.read(inputimage, OCIO::BIT_DEPTH_F32);
        }
        else
        {
            imgInput.read(inputimage);
        }

        std::cout << imgInput.getImageDescStr() << std::endl;
    }
    catch (const std::exception & e)
    {
        std::cerr << "ERROR: Loading file failed: " << e.what() << std::endl;
        exit(1);
    }
    catch (...)
    {
        std::cerr << "ERROR: Loading file failed." << std::endl;
        exit(1);
    }

#ifdef OCIO_GPU_ENABLED
    // Initialize GPU.
    OCIO::OglAppRcPtr oglApp;

    if (usegpu || usegpuLegacy)
    {
        OCIO::OglApp::Components comp = OCIO::OglApp::COMPONENTS_RGBA;
        if (imgInput.getNumChannels() == 4)
        {
            comp = OCIO::OglApp::COMPONENTS_RGBA;
        }
        else if (imgInput.getNumChannels() == 3)
        {
            comp = OCIO::OglApp::COMPONENTS_RGB;
        }
        else
        {
            std::cerr << "Cannot convert image with " << imgInput.getNumChannels()
                      << " components." << std::endl;
            exit(1);
        }

        try
        {
        #if __APPLE__
            if (useMetal)
            {
                oglApp = std::make_shared<OCIO::MetalApp>("ocioconvert", 256, 20);
            }
            else
        #endif
            {
                oglApp = OCIO::OglApp::CreateOglApp("ocioconvert", 256, 20);
            }
        }
        catch (const OCIO::Exception & e)
        {
            std::cerr << std::endl << e.what() << std::endl;
            exit(1);
        }

        if (verbose)
        {
            oglApp->printGLInfo();
        }

        oglApp->setPrintShader(outputgpuInfo);

        oglApp->initImage(imgInput.getWidth(), imgInput.getHeight(), comp, (float *)imgInput.getData());
        
        oglApp->createGLBuffers();
    }
#endif // OCIO_GPU_ENABLED

    // Process the image.
    try
    {
        // Get the processor.
        OCIO::ConstProcessorRcPtr processor;

        try
        {
            if (useLut)
            {
                // Create the OCIO processor for the specified transform.
                OCIO::FileTransformRcPtr t = OCIO::FileTransform::Create();
                t->setSrc(lutFile);
                t->setInterpolation(OCIO::INTERP_BEST);
    
                processor = config->getProcessor(t);
            }
            else if (useDisplayView)
            {
                OCIO::DisplayViewTransformRcPtr t = OCIO::DisplayViewTransform::Create();
                t->setSrc(inputcolorspace);
                t->setDisplay(display);
                t->setView(view);
                processor = config->getProcessor(t);
            }
            else if (useInvertView)
            {
                OCIO::DisplayViewTransformRcPtr t = OCIO::DisplayViewTransform::Create();
                t->setSrc(outputcolorspace);
                t->setDisplay(display);
                t->setView(view);
                processor = config->getProcessor(t, OCIO::TRANSFORM_DIR_INVERSE);
            }
            else if (useNamedTransform)
            {
                auto nt = config->getNamedTransform(namedtransform);

                if (nt)
                {
                    processor = config->getProcessor(nt, OCIO::TRANSFORM_DIR_FORWARD);
                }
                else
                {
                   std::cout << "ERROR: Could not get NamedTransform " << namedtransform << std::endl;
                   exit(1);
                }                
            }
            else if (useInvNamedTransform)
            {
                auto nt = config->getNamedTransform(namedtransform);

                if (nt)
                {
                    processor = config->getProcessor(nt, OCIO::TRANSFORM_DIR_INVERSE);
                }
                else
                {
                    std::cout << "ERROR: Could not get NamedTransform " << namedtransform << std::endl;
                    exit(1);
                }
            }
            else
            {
                processor = config->getProcessor(inputcolorspace, outputcolorspace);
            }
        }
        catch (const OCIO::Exception & e)
        {
            std::cout << "ERROR: OCIO failed with: " << e.what() << std::endl;
            exit(1);
        }
        catch (...)
        {
            std::cout << "ERROR: Creating processor unknown failure." << std::endl;
            exit(1);
        }

#ifdef OCIO_GPU_ENABLED
        if (usegpu || usegpuLegacy)
        {
            // Get the GPU shader program from the processor and set oglApp to use it.
            OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
            shaderDesc->setLanguage(
                #if __APPLE__
                        useMetal ? OCIO::GPU_LANGUAGE_MSL_2_0 :
                #endif
                        OCIO::GPU_LANGUAGE_GLSL_1_2);

            OCIO::ConstGPUProcessorRcPtr gpu
                = usegpuLegacy ? processor->getOptimizedLegacyGPUProcessor(OCIO::OPTIMIZATION_DEFAULT, 32)
                               : processor->getDefaultGPUProcessor();
            gpu->extractGpuShaderInfo(shaderDesc);

            oglApp->setShader(shaderDesc);
            oglApp->reshape(imgInput.getWidth(), imgInput.getHeight());
            oglApp->redisplay();
            oglApp->readImage((float *)imgInput.getData());
        }
        else
#endif // OCIO_GPU_ENABLED
        {
            /*
                Set the bit-depth of the output buffer.

                Whereas the GPU processor always work on float data, the CPU processor
                can be optimised for a specific input and output bit-depth.

                The converted image may require more bits than the source image.
                For example, converting a log image to linear requires at least a half-float
                output format. For most cases, half-float strikes a good balance between
                precision and storage space. But if the input depth would lose precision
                when converted to half-float, use float for the output depth instead.

                Note that when using OpenImageIO, the actual output bit-depth may be overrided
                if the file format doesn't support it. OCIO is not trying to analyze the filename
                to emulate OpenImageIO's decision making process.
            */
            const OCIO::BitDepth inputBitDepth = imgInput.getBitDepth();
            OCIO::BitDepth outputBitDepth;

            if (userOutputBitDepth != OCIO::BIT_DEPTH_UNKNOWN)
            {
                outputBitDepth = userOutputBitDepth;
            }
            else
            {
                if (inputBitDepth == OCIO::BIT_DEPTH_UINT16 || inputBitDepth == OCIO::BIT_DEPTH_F32)
                {
                    outputBitDepth = OCIO::BIT_DEPTH_F32;
                }
                else if (inputBitDepth == OCIO::BIT_DEPTH_UINT8 || inputBitDepth == OCIO::BIT_DEPTH_F16)
                {
                    outputBitDepth = OCIO::BIT_DEPTH_F16;
                }
                else
                {
                    throw OCIO::Exception("Unsupported input bitdepth, must be uint8, uint16, half or float.");
                }
            }

            OCIO::ConstCPUProcessorRcPtr cpuProcessor
                = processor->getOptimizedCPUProcessor(inputBitDepth,
                                                      outputBitDepth,
                                                      OCIO::OPTIMIZATION_DEFAULT);

            const bool useOutputBuffer = inputBitDepth != outputBitDepth;

            if (useOutputBuffer)
            {
                imgOutputCPU.init(imgInput, outputBitDepth);
                imgOutput = &imgOutputCPU;
            }

            const std::chrono::high_resolution_clock::time_point start
                = std::chrono::high_resolution_clock::now();

            if (useOutputBuffer)
            {
                OCIO::ImageDescRcPtr srcImgDesc = imgInput.getImageDesc();
                OCIO::ImageDescRcPtr dstImgDesc = imgOutputCPU.getImageDesc();
                cpuProcessor->apply(*srcImgDesc, *dstImgDesc);
            }
            else
            {
                OCIO::ImageDescRcPtr imgDesc = imgInput.getImageDesc();
                cpuProcessor->apply(*imgDesc);
            }

            if (verbose)
            {
                const std::chrono::high_resolution_clock::time_point end
                    = std::chrono::high_resolution_clock::now();

                std::chrono::duration<float, std::milli> duration = end - start;

                std::cout << std::endl;
                std::cout << "CPU processing took: " 
                          << duration.count()
                          <<  " ms" << std::endl;
            }
        }
    }
    catch (const OCIO::Exception & exception)
    {
        std::cerr << "ERROR: OCIO failed with: " << exception.what() << std::endl;
        exit(1);
    }
    catch (...)
    {
        std::cerr << "ERROR: Unknown error processing the image." << std::endl;
        exit(1);
    }

    // Set the provided image attributes.
    bool parseError = false;
    for (unsigned int i=0; i<floatAttrs.size(); ++i)
    {
        std::string name, value;
        float fval = 0.0f;

        if (!ParseNameValuePair(name, value, floatAttrs[i]) ||
           !StringToFloat(&fval,value.c_str()))
        {
            std::cerr << "ERROR: Attribute string '" << floatAttrs[i]
                      << "' should be in the form name=floatvalue." << std::endl;
            parseError = true;
            continue;
        }

        imgOutput->attribute(name, fval);
    }

    for (unsigned int i=0; i<intAttrs.size(); ++i)
    {
        std::string name, value;
        int ival = 0;
        if (!ParseNameValuePair(name, value, intAttrs[i]) ||
           !StringToInt(&ival,value.c_str()))
        {
            std::cerr << "ERROR: Attribute string '" << intAttrs[i]
                      << "' should be in the form name=intvalue." << std::endl;
            parseError = true;
            continue;
        }

        imgOutput->attribute(name, ival);
    }

    for (unsigned int i=0; i<stringAttrs.size(); ++i)
    {
        std::string name, value;
        if (!ParseNameValuePair(name, value, stringAttrs[i]))
        {
            std::cerr << "ERROR: Attribute string '" << stringAttrs[i]
                      << "' should be in the form name=value." << std::endl;
            parseError = true;
            continue;
        }

        imgOutput->attribute(name, value);
    }

    if (parseError)
    {
        exit(1);
    }

    // Write out the result.
    try
    {
        if (useDisplayView)
        {
            outputcolorspace = config->getDisplayViewColorSpaceName(display, view);
        }

        if (outputcolorspace)
        {
            imgOutput->attribute("oiio:ColorSpace", outputcolorspace);
        }

        imgOutput->write(outputimage, userOutputBitDepth);
    }
    catch (...)
    {
        std::cerr << "ERROR: Writing file \"" << outputimage << "\"." << std::endl;
        exit(1);
    }

    std::cout << "Wrote " << outputimage << std::endl;
    std::cout << imgOutput->getImageDescStr() << std::endl;

    return 0;
}


// Parse name=value parts.
// return true on success.

bool ParseNameValuePair(std::string& name,
                        std::string& value,
                        const std::string& input)
{
    // split string into name=value.
    size_t pos = input.find('=');
    if (pos==std::string::npos)
    {
        return false;
    }

    name = input.substr(0,pos);
    value = input.substr(pos+1);
    return true;
}

// return true on success.
bool StringToFloat(float * fval, const char * str)
{
    if (!str)
    {
        return false;
    }

    std::istringstream inputStringstream(str);
    float x;
    if (!(inputStringstream >> x))
    {
        return false;
    }

    if (fval)
    {
        *fval = x;
    }
    return true;
}

bool StringToInt(int * ival, const char * str)
{
    if (!str)
    {
        return false;
    }

    std::istringstream inputStringstream(str);
    int x;
    if (!(inputStringstream >> x))
    {
        return false;
    }

    if (ival)
    {
        *ival = x;
    }
    return true;
}
