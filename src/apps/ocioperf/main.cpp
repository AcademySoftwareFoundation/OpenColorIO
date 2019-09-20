// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <chrono>

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#if (OIIO_VERSION < 10100)
namespace OIIO = OIIO_NAMESPACE;
#endif

#include "OpenEXR/half.h"
#include "oiiohelpers.h"


#include "argparse.h"


class Measure
{
public:
    Measure() = delete;
    Measure(const Measure &) = delete;

    explicit Measure(const char * explanation, unsigned iterations)
        :   m_explanations(explanation)
        ,   m_iterations(iterations)
    {
        m_start = std::chrono::high_resolution_clock::now();
    }

    ~Measure()
    {
        std::chrono::high_resolution_clock::time_point end
            = std::chrono::high_resolution_clock::now();

        std::chrono::duration<float, std::milli> duration = end - m_start;

        std::cout << std::endl;
        std::cout << m_explanations << std::endl;
        std::cout << "  CPU processing took: " 
                  << (duration.count()/float(m_iterations))
                  <<  " ms" << std::endl;
    }
private:
    const std::string m_explanations;
    const unsigned m_iterations;
    const OIIO::ImageSpec m_imageSpec;

    std::chrono::high_resolution_clock::time_point m_start;
};


int main(int argc, const char **argv)
{
    bool verbose = false;
    signed int testType = 0;
    std::string transformFile;
    std::string inputColorSpace, outputColorSpace;
    std::string filepath;
    unsigned iterations = 10;

    bool help = false;

    ArgParse ap;
    ap.options("ocioperf -- apply and measure a color transformation processing\n\n"
               "usage: ocioperf [options] --image inputimage\n\n",
               "--h", &help, "Display the help and exit",
               "--v", &verbose, "Display some general information",
               "--test %d", &testType, "Define the type of processing to measure: "\
                                       "0 means on the complete image (the default), 1 is line-by-line, "\
                                       "2 is pixel-per-pixel and -1 performs all the test types",
               "--transform %s", &transformFile, "Provide the transform file to apply on the image",
               "--colorspaces %s %s", &inputColorSpace, &outputColorSpace, 
                                      "Provide the input and output color spaces to apply on the image",
               "--image %s", &filepath, "Provide the filepath of the image to process",
               "--iter %d", &iterations, "Provide the number of iterations on the processing. Default is 10",
               NULL);

    if(ap.parse (argc, argv) < 0) {
        std::cerr << ap.geterror() << std::endl;
        ap.usage();
        exit(1);
    }

    if(help)
    {
        ap.usage();
        exit(1);
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
            catch(...)
            {

                std::cerr << "Error loading the config file: '" << env << "'";
                exit(1);
            }
        }
    }

    OIIO::ImageSpec spec;
    OCIO::ImgBuffer img;
    int imgwidth = 0;
    int imgheight = 0;
    int components = 0;
    
    if(filepath.empty())
    {
        std::cerr << std::endl;
        std::cerr << "The image filepath is missing." << std::endl;
        exit(1);
    }

    // Load the image
    std::cout << std::endl;
    std::cout << "Loading " << filepath << std::endl;

    try
    {
#if OIIO_VERSION < 10903
        OIIO::ImageInput* f = OIIO::ImageInput::create(filepath);
#else
        auto f = OIIO::ImageInput::create(filepath);
#endif
        if(!f)
        {
            std::cerr << std::endl;
            std::cerr << "Could not create image input." << std::endl;
            exit(1);
        }

        f->open(filepath, spec);

        std::string error = f->geterror();
        if(!error.empty())
        {
            std::cerr << std::endl;
            std::cerr << "Error loading image " << error << std::endl;
            exit(1);
        }

        OCIO::PrintImageSpec(spec, verbose);
        
        imgwidth = spec.width;
        imgheight = spec.height;
        components = spec.nchannels;
        
        img.allocate(spec);

        const bool ok = f->read_image(spec.format, img.getBuffer());
        if(!ok)
        {
            std::cerr << std::endl;
            std::cerr << "Error reading \"" << filepath << "\" : " << f->geterror() << "\n";
            exit(1);
        }

#if OIIO_VERSION < 10903
        OIIO::ImageInput::destroy(f);
#endif       
    }
    catch(...)
    {
        std::cerr << "Error loading file.";
        exit(1);
    }

    // Process the image
    try
    {
        // Load the current config.

        OCIO::ConstProcessorRcPtr processor;
        if(!transformFile.empty())
        {
            OCIO::ConstConfigRcPtr config  = OCIO::Config::Create();

            std::cerr << std::endl;
            std::cout << "Processing using '" << transformFile << "'" << std::endl;

            // Get the transform.
            OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
            transform->setSrc(transformFile.c_str());

            // Get the processor
            processor = config->getProcessor(transform);
        }
        else if(!inputColorSpace.empty() && !outputColorSpace.empty())
        {
            if(verbose)
            {
                const char * env = getenv("OCIO");
                if(env && *env)
                {
                    std::cerr << std::endl;
                    std::cout << "Processing from '" << inputColorSpace << "' to '"
                              << outputColorSpace << "'" << std::endl;
                }
                else
                {
                    std::cerr << std::endl;
                    std::cerr << "Missing the ${OCIO} env. variable." << std::endl;
                    exit(1);
                }
            }

            OCIO::ConstConfigRcPtr config  = OCIO::Config::CreateFromEnv();

            // Get the processor
            processor = config->getProcessor(inputColorSpace.c_str(), outputColorSpace.c_str());
        }   
        else
        {
            std::cerr << std::endl;
            std::cerr << "Missing color transformation description." << std::endl;
            exit(1);
        }

        const OCIO::BitDepth bitDepth = OCIO::GetBitDepth(spec);

        // Get the CPU processor.
        OCIO::ConstCPUProcessorRcPtr cpuProcessor 
            = processor->getOptimizedCPUProcessor(bitDepth, bitDepth,
                                                  OCIO::OPTIMIZATION_DEFAULT,
                                                  OCIO::FINALIZATION_DEFAULT);

        if(testType==0 || testType==-1)
        {
            Measure m("Process the complete image:", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                // Process the complete image.
                OCIO::ImageDescRcPtr imgDesc = OCIO::CreateImageDesc(spec, img);

                // Apply the color transformation (in place).
                cpuProcessor->apply(*imgDesc);
            }
        }

        if(testType==1 || testType==-1)
        {
            char * p = reinterpret_cast<char *>(img.getBuffer());

            Measure m("Process the complete image but line by line:", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                // Process line by line.

                char * line = p;
                for(int h=0; h<imgheight; ++h)
                {
                    OCIO::PackedImageDesc imageDesc(line, imgwidth, 1, components,
                                                    spec.channel_bytes(),
                                                    spec.pixel_bytes(),
                                                    spec.scanline_bytes());

                    cpuProcessor->apply(imageDesc);
                    line += spec.scanline_bytes();
                }
            }
        }

        if(testType==2 || testType==-1)
        {
            char * p = reinterpret_cast<char *>(img.getBuffer());

            Measure m("Process the complete image but pixel per pixel:", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                // Process pixel per pixel.
                if(components==3)
                {
                    char * line = p;
                    for(int h=0; h<imgheight; ++h)
                    {
                        char * pxl = line;
                        for(int w=0; w<imgwidth; ++w)
                        {
                            cpuProcessor->applyRGB(pxl);
                            pxl += spec.pixel_bytes();
                        }
                        line += spec.scanline_bytes();
                    }
                }
                else
                {
                    char * line = p;
                    for(int h=0; h<imgheight; ++h)
                    {
                        char * pxl = line;
                        for(int w=0; w<imgwidth; ++w)
                        {
                            cpuProcessor->applyRGBA(pxl);
                            pxl += spec.pixel_bytes();
                        }
                        line += spec.scanline_bytes();
                    }
                }
            }
        }
    }
    catch(OCIO::Exception & exception)
    {
        std::cerr << "OCIO Error: " << exception.what() << std::endl;
        exit(1);
    }
    catch(...)
    {
        std::cerr << "Unknown OCIO error encountered." << std::endl;
        exit(1);
    }

    return 0;
}
