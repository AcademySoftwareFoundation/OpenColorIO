// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#if (OIIO_VERSION < 10100)
namespace OIIO = OIIO_NAMESPACE;
#endif

#include "apputils/argparse.h"
#include "OpenEXR/half.h"
#include "oiiohelpers.h"
#include "utils/StringUtils.h"


namespace OCIO = OCIO_NAMESPACE;

// Utility to measure time in ms.
class CustomMeasure
{
public:
    CustomMeasure() = delete;
    CustomMeasure(const CustomMeasure &) = delete;

    explicit CustomMeasure(const char * explanation)
        :   m_explanations(explanation)
        ,   m_iterations(1)
    {
        resume();
    }

    explicit CustomMeasure(const char * explanation, unsigned iterations)
        :   m_explanations(explanation)
        ,   m_iterations(iterations)
    {
    }

    ~CustomMeasure()
    {
        if(m_started)
        {
            pause();
        }

        if (m_iterations > 0)
        {
            std::ostringstream oss;
            oss.width(9);
            oss.precision(6);

            oss << m_explanations
                << "For " << m_iterations << " iterations, it took: ["
                << m_durations[0].count();

            if (m_iterations > 1)
            {
                oss << ", "
                    << (std::chrono::duration<float, std::milli>(m_duration - m_durations[0]).count()
                                / float(m_iterations-1))
                    << ", "
                    << (m_duration.count()/float(m_iterations));
            }

            oss << "] ms";

            std::cout << oss.str() << std::endl;
        }
    }

    void resume()
    {
        if(m_started)
        {
            throw OCIO::Exception("Measure already started.");
        }

        m_started = true;
        m_start = std::chrono::high_resolution_clock::now();
    }

    void pause()
    {
        std::chrono::high_resolution_clock::time_point end
           = std::chrono::high_resolution_clock::now();

        if(m_started)
        {
            std::chrono::duration<float, std::milli> duration = end - m_start;

            m_durations.push_back(duration);
            m_duration += duration;
        }
        else
        {
            throw OCIO::Exception("Measure already stopped.");
        }

        m_started = false;
    }

private:
    const std::string m_explanations;
    const unsigned m_iterations { 1 };

    bool m_started { false };
    std::chrono::high_resolution_clock::time_point m_start;

    std::chrono::duration<float, std::milli> m_duration { 0 };
    std::vector<std::chrono::duration<float, std::milli>> m_durations;
};

// Load in memory an image from disk.
void LoadImage(const std::string & filepath,
               bool verbose,
               OIIO::ImageSpec & spec, // [out] Image specifications.
               OCIO::ImgBuffer & img)  // [out] In memory image buffer.
{
    if(filepath.empty())
    {
        throw std::runtime_error("The image filepath is missing.");
    }

    // Load the image
    std::cout << std::endl;
    std::cout << "Loading " << filepath << std::endl;

#if OIIO_VERSION < 10903
    OIIO::ImageInput* f = OIIO::ImageInput::create(filepath);
#else
    auto f = OIIO::ImageInput::create(filepath);
#endif
    if(!f)
    {
        throw std::runtime_error("Could not create image input.");
    }

    f->open(filepath, spec);

    std::string error = f->geterror();
    if(!error.empty())
    {
        std::ostringstream oss;
        oss << "Error loading image " << error;
        throw std::runtime_error(oss.str().c_str());
    }

    OCIO::PrintImageSpec(spec, verbose);

    img.allocate(spec);

    std::cout << std::endl;
    CustomMeasure m("Load image: ");

    const bool ok = f->read_image(spec.format, img.getBuffer());
    if(!ok)
    {
        std::ostringstream oss;
        oss << "Error reading \"" << filepath << "\" : " << f->geterror();
        throw std::runtime_error(oss.str().c_str());
    }

#if OIIO_VERSION < 10903
    OIIO::ImageInput::destroy(f);
#endif
}

// Process the complete image in one shot.
void ProcessImage(CustomMeasure & m, OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
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
void ProcessLines(CustomMeasure & m, OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
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
void ProcessPixels(CustomMeasure & m, OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
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
    bool help = false;
    bool verbose = false;
    signed int testType = -1;
    std::string transformFile;
    std::string inputColorSpace, outputColorSpace, display, view;
    std::string filepath;
    unsigned iterations = 50;
    bool nocache = false;

    std::string outBitDepthStr("auto");

    ArgParse ap;
    ap.options("ocioperf -- apply and measure a color transformation processing\n\n"
               "usage: ocioperf [options] --image inputimage\n\n",
               "--help", &help, "Display the help and exit",
               "--verbose", &verbose, "Display some general information",
               "--test %d", &testType, "Define the type of processing to measure: "\
                                       "0 means on the complete image (the default), 1 is line-by-line, "\
                                       "2 is pixel-per-pixel and -1 performs all the test types",
               "--transform %s", &transformFile, "Provide the transform file to apply on the image",
               "--colorspaces %s %s", &inputColorSpace, &outputColorSpace,
                                      "Provide the input and output color spaces to apply on the image",
               "--displayview %s %s %s", &inputColorSpace, &display, &view,
                                      "Provide the input and (display, view) pair to apply on the image",
               "--image %s", &filepath, "Provide the filepath of the image to process",
               "--iter %d", &iterations, "Provide the number of iterations on the processing. Default is 10",
               "--out %s", &outBitDepthStr, "Provide an output bit-depth (auto, ui16, f32)"\
                                            " where auto preserves the input bit-depth",
               "--nocache", &nocache, "Bypass all caches",
               NULL);

    if (ap.parse (argc, argv) < 0)
    {
        std::cerr << ap.geterror() << std::endl;
        ap.usage();
        return 1;
    }

    if (help)
    {
        ap.usage();
        return 0;
    }

    if (verbose)
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
                std::cout << "OCIO Config. file:    '" << env << "'" << std::endl;
                OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
                std::cout << "OCIO Config. version: " << config->getMajorVersion() << "." 
                                                      << config->getMinorVersion() << std::endl;
                std::cout << "OCIO search_path:     " << config->getSearchPath() << std::endl;
            }
            catch(...)
            {

                std::cerr << "ERROR: Error loading the config file: '" << env << "'";
                return 1;
            }
        }
    }

    OIIO::ImageSpec spec;
    OCIO::ImgBuffer img;
    try
    {
        LoadImage(filepath, verbose, spec, img);
    }
    catch (std::exception & ex)
    {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "ERROR: Unknown error encountered." << std::endl;
        return 1;
    }

    outBitDepthStr = StringUtils::Lower(outBitDepthStr);

    if (!transformFile.empty())
    {
        std::cout << std::endl;
        std::cout << "Processing using '" << transformFile << "'" << std::endl << std::endl;
    }

    std::cout << std::endl << std::endl;
    std::cout << "Processing statistics:" << std::endl << std::endl;

    // Process the image.
    try
    {
        // Load the current config.

        OCIO::ConstProcessorRcPtr processor;
        if (!transformFile.empty())
        {
            OCIO::ConstConfigRcPtr config  = OCIO::Config::Create();

            // Get the transform.
            OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
            transform->setSrc(transformFile.c_str());

            {
                CustomMeasure m("Create the processor:\t\t\t", iterations);
                for (unsigned iter = 0; iter < iterations; ++iter)
                {
                    if (nocache)
                    {
                        OCIO::ClearAllCaches();
                    }

                    m.resume();
                    processor = config->getProcessor(transform, OCIO::TRANSFORM_DIR_FORWARD);
                    m.pause();
                }
            }
        }
        else if (!inputColorSpace.empty())
        {
            if (verbose)
            {
                const char * env = OCIO::GetEnvVariable("OCIO");
                if (env && *env)
                {
                    std::cout << std::endl;
                    std::cout << "Processing from '" << inputColorSpace << "' to '"
                              << outputColorSpace << "'" << std::endl << std::endl;
                }
                else
                {
                    throw OCIO::Exception("Missing the ${OCIO} env. variable.");
                }
            }

            OCIO::ConfigRcPtr config  = OCIO::Config::CreateFromEnv()->createEditableCopy();
            config->setProcessorCacheFlags(nocache ? OCIO::PROCESSOR_CACHE_OFF 
                                                   : OCIO::PROCESSOR_CACHE_DEFAULT);

            {
                CustomMeasure m("Create the config identifier:\t\t", iterations);
                for (unsigned iter = 0; iter < iterations; ++iter)
                {
                    if (nocache)
                    {
                        config = OCIO::Config::CreateFromEnv()->createEditableCopy();
                        config->setProcessorCacheFlags(nocache ? OCIO::PROCESSOR_CACHE_OFF 
                                                               : OCIO::PROCESSOR_CACHE_DEFAULT);
                    }

                    m.resume();
                    config->getCacheID();
                    m.pause();
                }
            }

            {
                CustomMeasure m("Create the context identifier:\t\t", iterations);
                for (unsigned iter = 0; iter < iterations; ++iter)
                {
                    if (nocache)
                    {
                        config = OCIO::Config::CreateFromEnv()->createEditableCopy();
                        config->setProcessorCacheFlags(nocache ? OCIO::PROCESSOR_CACHE_OFF 
                                                               : OCIO::PROCESSOR_CACHE_DEFAULT);
                    }

                    m.resume();
                    config->getCurrentContext()->getCacheID();
                    m.pause();
                }
            }

            {
                std::string msg;
                if (!outputColorSpace.empty())
                {
                    if (!display.empty() || !view.empty())
                    {
                        static constexpr char err[]
                            {"Both --colorspaces and --displayview may not be used at the same time."};

                        throw OCIO::Exception(err);
                    }

                    msg = "Create the processor:\t\t\t";
                }
                else
                {
                    msg = "Create the (display, view) processor:\t";
                }

                CustomMeasure m(msg.c_str(), iterations);
                for (unsigned iter = 0; iter < iterations; ++iter)
                {
                    if (nocache)
                    {
                        // Flush all the global internal caches.
                        OCIO::ClearAllCaches();
                    }

                    if (!outputColorSpace.empty())
                    {
                        m.resume();
                        processor = config->getProcessor(inputColorSpace.c_str(), outputColorSpace.c_str());
                        m.pause();
                    }
                    else
                    {
                        OCIO::ConstMatrixTransformRcPtr noChannelView;

                        m.resume();
                        processor = OCIO::DisplayViewHelpers::GetProcessor(config,
                                                                           inputColorSpace.c_str(),
                                                                           display.c_str(),
                                                                           view.c_str(),
                                                                           noChannelView,
                                                                           OCIO::TRANSFORM_DIR_FORWARD);
                        m.pause();
                    }
                }
            }
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

        // Get the optimized processor.
        OCIO::ConstProcessorRcPtr optProcessor;

        {
            CustomMeasure m("Create the optimized processor:\t\t", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                m.resume();
                optProcessor = processor->getOptimizedProcessor(inBitDepth,
                                                                outBitDepth,
                                                                OCIO::OPTIMIZATION_DEFAULT);
                m.pause();
            }
        }

        // Get the GPU processor.
        OCIO::ConstGPUProcessorRcPtr gpuProcessor;

        {
            CustomMeasure m("Create the GPU processor:\t\t", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                m.resume();
                gpuProcessor = optProcessor->getOptimizedGPUProcessor(OCIO::OPTIMIZATION_DEFAULT);
                m.pause();
            }
        }

        // Get the GPU Shader.
        OCIO::GpuShaderDescRcPtr shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_2);

        {
            CustomMeasure m("Create the GPU shader:\t\t\t", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                m.resume();
                gpuProcessor->extractGpuShaderInfo(shaderDesc);
                m.pause();
            }
        }

        // Get the CPU processor.
        OCIO::ConstCPUProcessorRcPtr cpuProcessor;

        {
            CustomMeasure m("Create the CPU processor:\t\t", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                m.resume();
                cpuProcessor = optProcessor->getOptimizedCPUProcessor(inBitDepth,
                                                                      outBitDepth,
                                                                      OCIO::OPTIMIZATION_DEFAULT);
                m.pause();
            }
        }

        std::cout << std::endl << std::endl;
        std::cout << "Image processing statistics:" << std::endl << std::endl;


        if(testType==0 || testType==-1)
        {
            // Process the complete image (in place).

            if(inBitDepth==outBitDepth)
            {
                CustomMeasure m("Process the complete image (in place):\t\t", iterations);

                for(unsigned iter=0; iter<iterations; ++iter)
                {
                    ProcessImage(m, cpuProcessor, spec, img);
                }
            }

            // Process the complete image with input and output buffers.

            CustomMeasure m("Process the complete image (two buffers):\t", iterations);

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

            CustomMeasure m("Process the complete image (in place) but line by line: ", iterations);

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
                CustomMeasure m("Process the complete image (in place) but pixel per pixel:", iterations);

                for(unsigned iter=0; iter<iterations; ++iter)
                {
                    ProcessPixels(m, cpuProcessor, spec, img);
                }
            }
        }

        std::cout << std::endl << std::endl;

    }
    catch (OCIO::Exception & ex)
    {
        std::cerr << "OCIO ERROR: " << ex.what() << std::endl;
        return 1;
    }
    catch (std::exception & ex)
    {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "ERROR: Unknown error encountered." << std::endl;
        return 1;
    }

    return 0;
}
