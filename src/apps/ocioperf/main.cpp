// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#if (OIIO_VERSION < 10100)
namespace OIIO = OIIO_NAMESPACE;
#endif

#include "apputils/argparse.h"
#include "apputils/measure.h"
#include "OpenEXR/half.h"
#include "oiiohelpers.h"
#include "utils/StringUtils.h"


namespace OCIO = OCIO_NAMESPACE;



// Load in memory an image from disk.
void LoadImage(const std::string & filepath,
               bool verbose,
               OIIO::ImageSpec & spec, // [out] Image specifications.
               OCIO::ImgBuffer & img)  // [out] In memory image buffer.
{
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
}

// Process the complete image in one shot.
void ProcessImage(Measure & m, OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
                  const OIIO::ImageSpec & spec, const OCIO::ImgBuffer & img)
{
    // Always process the same complete image.
    OCIO::ImgBuffer srcImg(img);
    OCIO::ImageDescRcPtr imgDesc = OCIO::CreateImageDesc(spec, srcImg);

    m.resume();

    // Apply the color transformation (in place).
    cpuProcessor->apply(*imgDesc);

    m.pause();
}

// Process the complete image line by line.
void ProcessLines(Measure & m, OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
                  const OIIO::ImageSpec & spec, const OCIO::ImgBuffer & img)
{
    // Always process the same complete image.
    OCIO::ImgBuffer srcImg(img);
    char * lineToProcess = reinterpret_cast<char *>(srcImg.getBuffer());

    for(int h=0; h<spec.height; ++h)
    {
        OCIO::PackedImageDesc imageDesc((void*)lineToProcess,
                                        spec.width,
                                        1, // Only one line.
                                        spec.nchannels,
                                        OCIO::GetBitDepth(spec),
                                        spec.channel_bytes(),
                                        spec.pixel_bytes(),
                                        spec.scanline_bytes());

        m.resume();

        // Apply the color transformation (in place).
        cpuProcessor->apply(imageDesc);

        // Find the next line.
        lineToProcess += spec.scanline_bytes();

        m.pause();
    }
}

// Process the complete image pixel per pixel.
void ProcessPixels(Measure & m, OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
                  const OIIO::ImageSpec & spec, const OCIO::ImgBuffer & img)
{
    // Always process the same complete image.
    OCIO::ImgBuffer buf(img);
    char * lineToProcess = reinterpret_cast<char *>(buf.getBuffer());

    m.resume();
    if(spec.nchannels==3)
    {
        for(int h=0; h<spec.height; ++h)
        {
            char * pixelToProcess = lineToProcess;
            for(int w=0; w<spec.width; ++w)
            {
                cpuProcessor->applyRGB(reinterpret_cast<float*>(pixelToProcess));

                // Find the next pixel.
                pixelToProcess += spec.pixel_bytes();
            }

            // Find the next line.
            lineToProcess += spec.scanline_bytes();
        }
    }
    else
    {
        for(int h=0; h<spec.height; ++h)
        {
            char * pixelToProcess = lineToProcess;
            for(int w=0; w<spec.width; ++w)
            {
                cpuProcessor->applyRGBA(reinterpret_cast<float*>(pixelToProcess));

                // Find the next pixel.
                pixelToProcess += spec.pixel_bytes();
            }

            // Find the next line.
            lineToProcess += spec.scanline_bytes();
        }
    }
    m.pause();
}

int main(int argc, const char **argv)
{
    bool verbose = false;
    signed int testType = 0;
    std::string transformFile;
    std::string inputColorSpace, outputColorSpace;
    std::string filepath;
    unsigned iterations = 10;
    std::string outBitDepthStr("auto");

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
               "--out %s", &outBitDepthStr, "Provide an output bit-depth (auto, ui16, f32)"\
                                            " where auto preserves the input bit-depth",
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
        const char * env = OCIO::GetEnvVariable("OCIO");
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
    LoadImage(filepath, verbose, spec, img);

    outBitDepthStr = StringUtils::Lower(outBitDepthStr);

    // Process the image.
    try
    {
        // Load the current config.

        OCIO::ConstProcessorRcPtr processor;
        if(!transformFile.empty())
        {
            OCIO::ConstConfigRcPtr config  = OCIO::Config::Create();

            std::cout << std::endl;
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
                const char * env = OCIO::GetEnvVariable("OCIO");
                if(env && *env)
                {
                    std::cerr << std::endl;
                    std::cout << "Processing from '" << inputColorSpace << "' to '"
                              << outputColorSpace << "'" << std::endl;
                }
                else
                {
                    throw OCIO::Exception("Missing the ${OCIO} env. variable.");
                }
            }

            OCIO::ConstConfigRcPtr config  = OCIO::Config::CreateFromEnv();

            // Get the processor
            processor = config->getProcessor(inputColorSpace.c_str(), outputColorSpace.c_str());
        }
        else
        {
            throw OCIO::Exception("Missing color transformation description.");
        }

        const OCIO::BitDepth inBitDepth  = OCIO::GetBitDepth(spec);
        OCIO::BitDepth outBitDepth = inBitDepth;
        if(outBitDepthStr=="f32")
        {
            outBitDepth= OCIO::BIT_DEPTH_F32;
        }
        else if(outBitDepthStr=="ui16")
        {
            outBitDepth= OCIO::BIT_DEPTH_UINT16;
        }
        else if(outBitDepthStr!="auto")
        {
            std::string err("Unsupported output bit-depth: ");
            err += outBitDepthStr;
            throw OCIO::Exception(err.c_str());
        }

        // Get the CPU processor.
        OCIO::ConstCPUProcessorRcPtr cpuProcessor
            = processor->getOptimizedCPUProcessor(inBitDepth, outBitDepth,
                                                  OCIO::OPTIMIZATION_DEFAULT);

        if(testType==0 || testType==-1)
        {
            // Process the complete image (in place).

            if(inBitDepth==outBitDepth)
            {
                Measure m("Process the complete image (in place):", iterations);

                for(unsigned iter=0; iter<iterations; ++iter)
                {
                    ProcessImage(m, cpuProcessor, spec, img);
                }
            }

            // Process the complete image with input and output buffers.

            Measure m("Process the complete image (two buffers):", iterations);

            OIIO::TypeDesc fmt = spec.format;
            if(inBitDepth!=outBitDepth)
            {
                if(outBitDepth==OCIO::BIT_DEPTH_F32)
                {
                    fmt = OIIO::TypeDesc::FLOAT;
                }
                else if(outBitDepth==OCIO::BIT_DEPTH_UINT16)
                {
                    fmt = OIIO::TypeDesc::UINT16;
                }
                else
                {
                    throw OCIO::Exception("Unsupported output bit-depth.");
                }
            }

            OIIO::ImageSpec dstSpec(spec.width, spec.height, spec.nchannels, fmt);
            OCIO::ImgBuffer dstImg(dstSpec);
            OCIO::ImageDescRcPtr dstImgDesc = OCIO::CreateImageDesc(dstSpec, dstImg);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                // Always process the same complete image.
                OCIO::ImgBuffer srcImg(img);
                OCIO::ImageDescRcPtr srcImgDesc = OCIO::CreateImageDesc(spec, srcImg);

                // Apply the color transformation.
                m.resume();
                cpuProcessor->apply(*srcImgDesc, *dstImgDesc);
                m.pause();
            }

        }

        if((testType==1 || testType==-1) && (inBitDepth==outBitDepth))
        {
            // Process line by line.

            Measure m("Process the complete image (in place) but line by line:", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                ProcessLines(m, cpuProcessor, spec, img);
            }
        }

        if((testType==2 || testType==-1) && inBitDepth==outBitDepth)
        {
            // Process pixel per pixel if the image buffer is packed RGBA 32-bit float.

            OCIO::PackedImageDesc imgDesc(img.getBuffer(),
                                          spec.width,
                                          spec.height,
                                          spec.nchannels,
                                          OCIO::GetBitDepth(spec),
                                          spec.channel_bytes(),
                                          spec.pixel_bytes(),
                                          spec.scanline_bytes());

            if(imgDesc.isRGBAPacked() && imgDesc.isFloat())
            {
                Measure m("Process the complete image (in place) but pixel per pixel:", iterations);

                for(unsigned iter=0; iter<iterations; ++iter)
                {
                    ProcessPixels(m, cpuProcessor, spec, img);
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
