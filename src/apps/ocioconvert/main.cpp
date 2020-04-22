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

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#if (OIIO_VERSION < 10100)
namespace OIIO = OIIO_NAMESPACE;
#endif

#include "apputils/argparse.h"

#ifdef OCIO_GPU_ENABLED
#include "oglapp.h"
#endif // OCIO_GPU_ENABLED

#include "oiiohelpers.h"
#include "OpenEXR/half.h"


// Array of non OpenColorIO arguments.
static std::vector<std::string> args;


// Fill 'args' array with OpenColorIO arguments.
static int
parse_end_args(int argc, const char *argv[])
{
  while(argc>0)
  {
    args.push_back(argv[0]);
    argc--;
    argv++;
  }

  return 0;
}

bool ParseNameValuePair(std::string& name, std::string& value,
                        const std::string& input);

bool StringToFloat(float * fval, const char * str);

bool StringToInt(int * ival, const char * str);

bool StringToVector(std::vector<int> * ivector, const char * str);

int main(int argc, const char **argv)
{
    ArgParse ap;

    std::vector<std::string> floatAttrs;
    std::vector<std::string> intAttrs;
    std::vector<std::string> stringAttrs;
    std::string keepChannels;
    bool croptofull = false;
    bool usegpu = false;
    bool usegpuLegacy = false;
    bool outputgpuInfo = false;
    bool verbose = false;
    bool useLut = false;
    bool useDisplayView = false;

    ap.options("ocioconvert -- apply colorspace transform to an image \n\n"
               "usage: ocioconvert [options]  inputimage inputcolorspace outputimage outputcolorspace\n"
               "   or: ocioconvert [options] --lut lutfile inputimage outputimage\n"
               "   or: ocioconvert [options] --view inputimage inputcolorspace outputimage displayname viewname\n\n",
               "%*", parse_end_args, "",
               "<SEPARATOR>", "Options:",
               "--lut",       &useLut,         "Convert using a LUT rather than a config file",
               "--view",      &useDisplayView, "Convert to a (display,view) pair rather than to "
                                               "an output color space",
               "--gpu",       &usegpu,         "Use GPU color processing instead of CPU (CPU is the default)",
               "--gpulegacy", &usegpuLegacy,   "Use the legacy (i.e. baked) GPU color processing "
                                               "instead of the CPU one (--gpu is ignored)",
               "--gpuinfo",  &outputgpuInfo,   "Output the OCIO shader program",
               "--v",        &verbose,         "Display general information",
               "<SEPARATOR>", "\nOpenImageIO options:",
               "--float-attribute %L",  &floatAttrs,   "\"name=float\" pair defining OIIO float attribute "
                                                       "for outputimage",
               "--int-attribute %L",    &intAttrs,     "\"name=int\" pair defining OIIO int attribute "
                                                       "for outputimage",
               "--string-attribute %L", &stringAttrs,  "\"name=string\" pair defining OIIO string attribute "
                                                       "for outputimage",
               "--croptofull",          &croptofull,   "Crop or pad to make pixel data region match the "
                                                       "\"full\" region",
               "--ch %s",               &keepChannels, "Select channels (e.g., \"2,3,4\")",
               NULL
               );
    if (ap.parse (argc, argv) < 0)
    {
        std::cerr << ap.geterror() << std::endl;
        ap.usage ();
        exit(1);
    }

#ifndef OCIO_GPU_ENABLED
    if (usegpu || outputgpuInfo || usegpuLegacy)
    {
        std::cerr << "Compiled without OpenGL support, GPU options are not available.";
        std::cerr << std::endl;
        exit(1);
    }
#endif // OCIO_GPU_ENABLED

    const char * inputimage       = nullptr;
    const char * inputcolorspace  = nullptr;
    const char * outputimage      = nullptr;
    const char * outputcolorspace = nullptr;
    const char * lutFile          = nullptr;
    const char * display          = nullptr;
    const char * view             = nullptr;

    if (!useLut && !useDisplayView)
    {
        if (args.size() != 4)
        {
            std::cerr << "ERROR: Expecting 4 arguments, found " << args.size() << std::endl;
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
                      << args.size() << std::endl;
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
                      << args.size() << std::endl;
            ap.usage();
            exit(1);
        }
        inputimage      = args[0].c_str();
        inputcolorspace = args[1].c_str();
        outputimage     = args[2].c_str();
        display         = args[3].c_str();
        view            = args[4].c_str();
    }

    if(verbose)
    {
        std::cout << std::endl;
        std::cout << "OIIO Version: " << OIIO_VERSION_STRING << std::endl;
        std::cout << "OCIO Version: " << OCIO::GetVersion() << std::endl;
        const char * env = getenv("OCIO");
        if(env && *env)
        {
            try
            {
                std::cout << std::endl;
                std::cout << "OCIO Configuration: '" << env << "'" << std::endl;
                OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
                std::cout << "OCIO search_path:    " << config->getSearchPath() << std::endl;
            }
            catch (const OCIO::Exception & e)
            {
                std::cout << "ERROR loading config file: " << e.what() << std::endl;
                exit(1);
            }
            catch(...)
            {

                std::cerr << "ERROR loading the config file: '" << env << "'";
                exit(1);
            }
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

    OIIO::ImageSpec spec;
    OCIO::ImgBuffer img;
    int imgwidth = 0;
    int imgheight = 0;
    int components = 0;

    // Load the image.
    std::cout << std::endl;
    std::cout << "Loading " << inputimage << std::endl;
    try
    {
#if OIIO_VERSION < 10903
        OIIO::ImageInput* f = OIIO::ImageInput::create(inputimage);
#else
        auto f = OIIO::ImageInput::create(inputimage);
#endif
        if(!f)
        {
            std::cerr << "ERROR: Could not create image input." << std::endl;
            exit(1);
        }

        f->open(inputimage, spec);

        std::string error = f->geterror();
        if(!error.empty())
        {
            std::cerr << "ERROR: Could not load image: " << error << std::endl;
            exit(1);
        }

        OCIO::PrintImageSpec(spec, verbose);

        imgwidth = spec.width;
        imgheight = spec.height;
        components = spec.nchannels;

        if (usegpu || usegpuLegacy)
        {
            spec.format = OIIO::TypeDesc::FLOAT;
            img.allocate(spec);

            const bool ok = f->read_image(spec.format, img.getBuffer());
            if(!ok)
            {
                std::cerr << "ERROR: Reading \"" << inputimage << "\" failed with: "
                          << f->geterror() << std::endl;
                exit(1);
            }

            if(croptofull)
            {
                std::cerr << "ERROR: Crop disabled in GPU mode" << std::endl;
                exit(1);
            }
        }
        else
        {
            img.allocate(spec);

            const bool ok = f->read_image(spec.format, img.getBuffer());
            if(!ok)
            {
                std::cerr << "ERROR: Reading \"" << inputimage << "\" failed with: "
                          << f->geterror() << std::endl;
                exit(1);
            }
        }

#if OIIO_VERSION < 10903
        OIIO::ImageInput::destroy(f);
#endif

        std::vector<int> kchannels;
        //parse --ch argument.
        if (keepChannels != "" && !StringToVector(&kchannels, keepChannels.c_str()))
        {
            std::cerr << "ERROR: --ch: '" << keepChannels
                      << "' should be comma-seperated integers" << std::endl;
            exit(1);
        }

        //if kchannels not specified, then keep all channels.
        if (kchannels.size() == 0)
        {
            kchannels.resize(components);
            for (int channel=0; channel < components; channel++)
            {
                kchannels[channel] = channel;
            }
        }

        if (croptofull)
        {
            imgwidth = spec.full_width;
            imgheight = spec.full_height;

            std::cout << "cropping to " << imgwidth
                      << "x" << imgheight << std::endl;
        }

        if (croptofull || (int)kchannels.size() < spec.nchannels)
        {
            // Redefine the spec so it matches the new bounding box.
            OIIO::ImageSpec croppedSpec = spec;

            croppedSpec.x = 0;
            croppedSpec.y = 0;
            croppedSpec.height    = imgheight;
            croppedSpec.width     = imgwidth;
            croppedSpec.nchannels = (int)(kchannels.size());

            OCIO::ImgBuffer croppedImg(croppedSpec);

            void * croppedBuf = croppedImg.getBuffer();
            void * imgBuf     = img.getBuffer();

            // crop down bounding box and ditch all but n channels.
            // img is a flattened 3 dimensional matrix heightxwidthxchannels.
            // fill croppedimg with only the needed pixels.
            for (int y=0 ; y < spec.height ; y++)
            {
                for (int x=0 ; x < spec.width; x++)
                {
                    for (int k=0; k < (int)kchannels.size(); k++)
                    {
                        int channel = kchannels[k];
                        int current_pixel_y = y + spec.y;
                        int current_pixel_x = x + spec.x;

                        if (current_pixel_y >= 0 &&
                            current_pixel_x >= 0 &&
                            current_pixel_y < imgheight &&
                            current_pixel_x < imgwidth)
                        {
                            const size_t imgIdx = (y * spec.width * components) 
                                                    + (x * components) + channel;

                            const size_t cropIdx = (current_pixel_y * imgwidth * kchannels.size())
                                                    + (current_pixel_x * kchannels.size())
                                                    + channel;

                            if(spec.format==OIIO::TypeDesc::FLOAT)
                            {
                                ((float*)croppedBuf)[cropIdx] = ((float*)imgBuf)[imgIdx];
                            }
                            else if(spec.format==OIIO::TypeDesc::HALF)
                            {
                                ((half*)croppedBuf)[cropIdx] = ((half*)imgBuf)[imgIdx];
                            }
                            else if(spec.format==OIIO::TypeDesc::UINT16)
                            {
                                ((uint16_t*)croppedBuf)[cropIdx] = ((uint16_t*)imgBuf)[imgIdx];
                            }
                            else if(spec.format==OIIO::TypeDesc::UINT8)
                            {
                                ((uint8_t*)croppedBuf)[cropIdx] = ((uint8_t*)imgBuf)[imgIdx];
                            }
                            else
                            {
                                std::cerr << "ERROR: Unsupported image type: " 
                                          << spec.format << std::endl;
                                exit(1);
                            }
                        }
                    }
                }
            }

            components = (int)(kchannels.size());

            img = std::move(croppedImg);
        }
    }
    catch(...)
    {
        std::cerr << "ERROR: Loading file failed" << std::endl;
        exit(1);
    }

#ifdef OCIO_GPU_ENABLED
    // Initialize GPU.

    OCIO::OglAppRcPtr oglApp;
    if (usegpu || usegpuLegacy)
    {
        OCIO::OglApp::Components comp = OCIO::OglApp::COMPONENTS_RGBA;
        if (components == 4)
        {
            comp = OCIO::OglApp::COMPONENTS_RGBA;
        }
        else if (components == 3)
        {
            comp = OCIO::OglApp::COMPONENTS_RGB;
        }
        else
        {
            std::cerr << "Cannot convert image with " << components << " components." << std::endl;
            exit(1);
        }

        try
        {
            oglApp = std::make_shared<OCIO::OglApp>("ocioconvert", 256, 20);
        }
        catch (const OCIO::Exception & e)
        {
            std::cerr << std::endl << e.what() << std::endl;
            exit(1);
        }

        if (outputgpuInfo)
        {
            oglApp->printGLInfo();
        }

        oglApp->initImage(imgwidth, imgheight, comp, (float *)img.getBuffer());
        
        oglApp->createGLBuffers();
    }
#endif // OCIO_GPU_ENABLED

    // Process the image.
    try
    {
        // Load the current config.
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();

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
                OCIO::DisplayTransformRcPtr t = OCIO::DisplayTransform::Create();
                t->setInputColorSpaceName(inputcolorspace);
                t->setDisplay(display);
                t->setView(view);
                processor = config->getProcessor(t);
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
            std::cout << "ERROR: Creating processor unknown failure" << std::endl;
            exit(1);
        }

#ifdef OCIO_GPU_ENABLED
        if (usegpu || usegpuLegacy)
        {
            // Get the GPU shader program from the processor and set oglApp to use it.
            OCIO::GpuShaderDescRcPtr shaderDesc;
            if (usegpuLegacy)
            {
                shaderDesc = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(32);
            }
            else
            {
                shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
            }
            shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);
            processor->getDefaultGPUProcessor()->extractGpuShaderInfo(shaderDesc);
            oglApp->setShader(shaderDesc);

            oglApp->reshape(imgwidth, imgheight);
            oglApp->redisplay();
            oglApp->readImage((float *)img.getBuffer());
        }
        else
#endif // OCIO_GPU_ENABLED
        {
            const OCIO::BitDepth bitDepth = OCIO::GetBitDepth(spec);

            OCIO::ConstCPUProcessorRcPtr cpuProcessor 
                = processor->getOptimizedCPUProcessor(bitDepth, bitDepth,
                                                      OCIO::OPTIMIZATION_DEFAULT);

            const std::chrono::high_resolution_clock::time_point start
                = std::chrono::high_resolution_clock::now();

            OCIO::ImageDescRcPtr imgDesc = OCIO::CreateImageDesc(spec, img);
            cpuProcessor->apply(*imgDesc);

            if(verbose)
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
    catch(OCIO::Exception & exception)
    {
        std::cerr << "ERROR: OCIO failed with: " << exception.what() << std::endl;
        exit(1);
    }
    catch(...)
    {
        std::cerr << "ERROR: Unknown error processing the image" << std::endl;
        exit(1);
    }

    //
    // set the provided OpenImageIO attributes.
    //
    bool parseerror = false;
    for(unsigned int i=0; i<floatAttrs.size(); ++i)
    {
        std::string name, value;
        float fval = 0.0f;

        if(!ParseNameValuePair(name, value, floatAttrs[i]) ||
           !StringToFloat(&fval,value.c_str()))
        {
            std::cerr << "ERROR: Attribute string '" << floatAttrs[i]
                      << "' should be in the form name=floatvalue" << std::endl;
            parseerror = true;
            continue;
        }

        spec.attribute(name, fval);
    }

    for(unsigned int i=0; i<intAttrs.size(); ++i)
    {
        std::string name, value;
        int ival = 0;
        if(!ParseNameValuePair(name, value, intAttrs[i]) ||
           !StringToInt(&ival,value.c_str()))
        {
            std::cerr << "ERROR: Attribute string '" << intAttrs[i]
                      << "' should be in the form name=intvalue" << std::endl;
            parseerror = true;
            continue;
        }

        spec.attribute(name, ival);
    }

    for(unsigned int i=0; i<stringAttrs.size(); ++i)
    {
        std::string name, value;
        if(!ParseNameValuePair(name, value, stringAttrs[i]))
        {
            std::cerr << "ERROR: Attribute string '" << stringAttrs[i]
                      << "' should be in the form name=value" << std::endl;
            parseerror = true;
            continue;
        }

        spec.attribute(name, value);
    }

    if(parseerror)
    {
        exit(1);
    }

    // Write out the result.
    try
    {
#if OIIO_VERSION < 10903
        OIIO::ImageOutput* f = OIIO::ImageOutput::create(outputimage);
#else
        auto f = OIIO::ImageOutput::create(outputimage);
#endif
        if(!f)
        {
            std::cerr << "ERROR: Could not create output input" << std::endl;
            exit(1);
        }

        f->open(outputimage, spec);

        if(!f->write_image(spec.format, img.getBuffer()))
        {
            std::cerr << "ERROR: Writing \"" << outputimage << "\" failed with: "
                      << f->geterror() << std::endl;
            exit(1);
        }

        f->close();
#if OIIO_VERSION < 10903
        OIIO::ImageOutput::destroy(f);
#endif
    }
    catch(...)
    {
        std::cerr << "ERROR: Writing file \"" << outputimage << "\"" << std::endl;
        exit(1);
    }

    std::cout << std::endl;
    std::cout << "Wrote " << outputimage << std::endl;

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
    if(pos==std::string::npos) return false;

    name = input.substr(0,pos);
    value = input.substr(pos+1);
    return true;
}

// return true on success.
bool StringToFloat(float * fval, const char * str)
{
    if(!str) return false;

    std::istringstream inputStringstream(str);
    float x;
    if(!(inputStringstream >> x))
    {
        return false;
    }

    if(fval) *fval = x;
    return true;
}

bool StringToInt(int * ival, const char * str)
{
    if(!str) return false;

    std::istringstream inputStringstream(str);
    int x;
    if(!(inputStringstream >> x))
    {
        return false;
    }

    if(ival) *ival = x;
    return true;
}

bool StringToVector(std::vector<int> * ivector, const char * str)
{
    std::stringstream ss(str);
    int i;
    while (ss >> i)
    {
        ivector->push_back(i);
        if (ss.peek() == ',')
        {
          ss.ignore();
        }
    }
    return ivector->size() != 0;
}




