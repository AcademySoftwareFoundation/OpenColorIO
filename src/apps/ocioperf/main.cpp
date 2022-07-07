// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <OpenColorIO/OpenColorIO.h>

#include "apputils/argparse.h"
#include "utils/StringUtils.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <iostream>


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

// Process the complete image line by line.
void ProcessLines(CustomMeasure & m,
                  OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
                  const OCIO::PackedImageDesc & img)
{
    // Always process the same complete image.
    char * lineToProcess = reinterpret_cast<char *>(img.getData());

    m.resume();

    for(int h=0; h<img.getHeight(); ++h)
    {
        OCIO::PackedImageDesc imageDesc((void*)lineToProcess,
                                        img.getWidth(),
                                        1, // Only one line.
                                        img.getNumChannels(),
                                        img.getBitDepth(),
                                        OCIO::AutoStride,
                                        OCIO::AutoStride,
                                        OCIO::AutoStride);

        // Apply the color transformation (in place).
        cpuProcessor->apply(imageDesc);

        // Find the next line.
        lineToProcess += img.getYStrideBytes();
    }

    m.pause();
}

// Process the complete image pixel per pixel.
void ProcessPixels(CustomMeasure & m,
                   OCIO::ConstCPUProcessorRcPtr & cpuProcessor,
                   const OCIO::PackedImageDesc & img)
{
    // Always process the same complete image.
    char * lineToProcess = reinterpret_cast<char *>(img.getData());

    m.resume();

    for(int h=0; h<img.getHeight(); ++h)
    {
        char * pixelToProcess = lineToProcess;
        for(int w=0; w<img.getWidth(); ++w)
        {
            cpuProcessor->applyRGBA(reinterpret_cast<float*>(pixelToProcess));

            // Find the next pixel.
            pixelToProcess += img.getXStrideBytes();
        }

        // Find the next line.
        lineToProcess += img.getYStrideBytes();
    }

    m.pause();
}

int main(int argc, const char **argv)
{
    bool help = false;
    bool verbose = false;
    signed int testType = -1;
    std::string transformFile;
    std::string inColorSpace, outColorSpace, display, view;
    std::string inBitDepthStr("f32"), outBitDepthStr("f32");
    unsigned iterations = 50;
    bool nocache = false, nooptim = false;

    bool useColorspaces = false;
    bool useDisplayview = false;
    bool useInvertview  = false;

    ArgParse ap;
    ap.options("ocioperf -- apply and measure a color transformation processing\n\n"
               "usage: ocioperf [options] --transform /path/to/file.clf\n\n",
               "--h",                       &help,              
                                            "Display the help and exit",
               "--help",                    &help,              
                                            "Display the help and exit",
               "--verbose",                 &verbose,           
                                            "Display some general information",
               "--test %d",                 &testType,          
                                            "Define the type of processing to measure: "\
                                            "0 means on the complete image (the default), 1 is line-by-line, "\
                                            "2 is pixel-per-pixel and -1 performs all the test types",
               "--transform %s",            &transformFile, 
                                            "Provide the transform file to apply on the image",
               "--colorspaces %s %s",       &inColorSpace, &outColorSpace,
                                            "Provide the input and output color spaces to apply on the image",
               "--view %s %s %s",           &inColorSpace, &display, &view,
                                            "Provide the input color space and (display, view) pair to apply on the image",
               "--displayview %s %s %s",    &inColorSpace, &display, &view,
                                            "(Deprecated) Provide the input and (display, view) pair to apply on the image",
               "--invertview %s %s %s",     &display, &view, &outColorSpace,
                                            "Provide the (display, view) pair and output color space to apply on the image",
               "--iter %d",                 &iterations, "Provide the number of iterations on the processing. Default is 50",
               "--bitdepths %s %s",         &inBitDepthStr, &outBitDepthStr,
                                            "Provide input and output bit-depths (i.e. ui16, f32). Default is f32",
               "--nocache",                 &nocache, 
                                            "Bypass all caches. Default is false",
               "--nooptim",                 &nooptim, 
                                            "Disable the processor optimizations. Default is false",
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
            OCIO::ConfigRcPtr config  = OCIO::Config::CreateRaw()->createEditableCopy();
            config->setProcessorCacheFlags(nocache ? OCIO::PROCESSOR_CACHE_OFF 
                                                   : OCIO::PROCESSOR_CACHE_DEFAULT);

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
        // Checking for an input colorspace or input (display, view) pair.
        else if (!inColorSpace.empty() || (!display.empty() && !view.empty()))
        {
            if (verbose)
            {
                const char * env = OCIO::GetEnvVariable("OCIO");
                if (env && *env)
                {
                    std::cout << std::endl;
                    const std::string inputStr = !inColorSpace.empty() ?  inColorSpace : "(" + display + ", " + view + ")";
                    const std::string outputStr = !outColorSpace.empty() ?  outColorSpace : "(" + display + ", " + view + ")";
                    std::cout << "Processing from '" 
                              << inputStr << "' to '"
                              << outputStr << "'" << std::endl;
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
                    m.resume();
                    config->getCacheID();
                    m.pause();
                }
            }

            {
                CustomMeasure m("Create the context identifier:\t\t", iterations);
                for (unsigned iter = 0; iter < iterations; ++iter)
                {
                    m.resume();
                    config->getCurrentContext()->getCacheID();
                    m.pause();
                }
            }

            {
                // --colorspaces
                useColorspaces = !inColorSpace.empty() && !outColorSpace.empty();
                // --view
                useDisplayview = !inColorSpace.empty() && !display.empty() && !view.empty();
                // --invertview
                useInvertview = !display.empty() && !view.empty() && !outColorSpace.empty();

                // Errors validation
                std::string msg; 
                if ((useColorspaces && !useDisplayview && !useInvertview) ||
                     (useDisplayview && !useColorspaces && !useInvertview) ||
                     (useInvertview && !useColorspaces && !useDisplayview))
                {
                    if (useColorspaces || useInvertview)
                    {
                        msg = "Create the colorspaces processor:\t";
                    }
                    else if (useDisplayview)
                    {
                        msg = "Create the (display, view) processor:\t";
                    }
                } 
                else
                {
                    static constexpr char err[]
                        {"Any combinations of --colorspaces, --view or --invertview is invalid."};

                    throw OCIO::Exception(err);
                }

                CustomMeasure m(msg.c_str(), iterations);
                for (unsigned iter = 0; iter < iterations; ++iter)
                {
                    if (nocache)
                    {
                        // Flush all the global internal caches.
                        OCIO::ClearAllCaches();
                    }

                    // Processing colorspaces option 
                    if (useColorspaces)
                    {
                        m.resume();
                        processor = config->getProcessor(inColorSpace.c_str(), outColorSpace.c_str());
                        m.pause();
                    }
                    // Processing view option
                    else if (useDisplayview)
                    {
                        OCIO::ConstMatrixTransformRcPtr noChannelView;

                        m.resume();
                        processor = OCIO::DisplayViewHelpers::GetProcessor(config,
                                                                           inColorSpace.c_str(),
                                                                           display.c_str(),
                                                                           view.c_str(),
                                                                           noChannelView,
                                                                           OCIO::TRANSFORM_DIR_FORWARD);
                        m.pause();
                    }
                    // Processing invertview option
                    else if (useInvertview)
                    {
                        OCIO::ConstMatrixTransformRcPtr noChannelView;

                        m.resume();
                        processor = OCIO::DisplayViewHelpers::GetProcessor(config,
                                                                           outColorSpace.c_str(),
                                                                           display.c_str(),
                                                                           view.c_str(),
                                                                           noChannelView,
                                                                           OCIO::TRANSFORM_DIR_INVERSE);
                        m.pause();
                    }
                }
            }
        }
        else
        {
            throw OCIO::Exception("Missing color transformation description.");
        }

        const OCIO::OptimizationFlags optimFlags
            = nooptim ? OCIO::OPTIMIZATION_NONE : OCIO::OPTIMIZATION_DEFAULT;

        auto GetBitDepthFromString = [](const std::string & str) -> OCIO::BitDepth 
        {
            OCIO::BitDepth bd = OCIO::BIT_DEPTH_F32;
    
            if (str == "f32")
            {
                bd = OCIO::BIT_DEPTH_F32;
            }
            else if (str == "ui16")
            {
                bd = OCIO::BIT_DEPTH_UINT16;
            }
            else
            {
                std::string err("Unsupported bit-depth: ");
                err += str;
                throw OCIO::Exception(err.c_str());
            }

            return bd;
        };

        const OCIO::BitDepth inBitDepth  = GetBitDepthFromString(inBitDepthStr);
        const OCIO::BitDepth outBitDepth = GetBitDepthFromString(outBitDepthStr);

        // Get the optimized processor.
        OCIO::ConstProcessorRcPtr optProcessor;

        {
            CustomMeasure m("Create the optimized processor:\t\t", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                m.resume();
                optProcessor = processor->getOptimizedProcessor(inBitDepth,
                                                                outBitDepth,
                                                                optimFlags);
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
                gpuProcessor = optProcessor->getOptimizedGPUProcessor(optimFlags);
                m.pause();
            }
        }

        // Get the GPU Shader.
        OCIO::GpuShaderDescRcPtr shaderDesc;

        {
            CustomMeasure m("Create the GPU shader:\t\t\t", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
                shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_2);

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
                                                                      optimFlags);
                m.pause();
            }
        }

        std::cout << std::endl << std::endl;
        std::cout << "Image processing statistics:" << std::endl << std::endl;

        // Create an arbitrary 4K RGBA image.

        static constexpr size_t width  = 3840;
        static constexpr size_t height = 2160;
        static constexpr size_t numChannels = 4;

        static constexpr size_t maxElts = width * height;

        std::vector<float> img_f32_ref;
        std::vector<uint16_t> img_ui16_ref;

        // Generate a synthetic image by emulating a LUT3D identity algorithm that steps through
        // many different colors.  Need to avoid a constant image, simple gradients, or anything
        // that would result in more cache hits than a typical image.  Also, want to step through a 
        // wide range of colors, including outside [0,1], in case some algorithms are faster or
        // slower for certain colors.

        static constexpr size_t length   = 201;
        static constexpr float stepValue = 1.0f / ((float)length - 1.0f);

        if (inBitDepth == OCIO::BIT_DEPTH_F32)
        {
            static constexpr float min   = -1.0f;
            static constexpr float max   =  2.0f;
            static constexpr float range = max - min;

            img_f32_ref.resize(maxElts * numChannels);

            // Retrofit value in the range.
            auto adjustValue = [](float val) -> float
            {
                return val * range + min;
            };

            for (size_t idx = 0; idx < maxElts; ++idx)
            {
                img_f32_ref[numChannels * idx + 0] = adjustValue( ((idx / length / length) % length) * stepValue );
                img_f32_ref[numChannels * idx + 1] = adjustValue( ((idx / length) % length) * stepValue );
                img_f32_ref[numChannels * idx + 2] = adjustValue( (idx % length) * stepValue );

                img_f32_ref[numChannels * idx + 3] = adjustValue( float(idx) / maxElts );
            }
        }
        else // request an integer image
        {
            img_ui16_ref.resize(maxElts * numChannels);

            // Retrofit value in the range.
            auto adjustValue = [](float val) -> uint16_t
            {
                return static_cast<uint16_t>(val * 65535);
            };

            for (size_t idx = 0; idx < maxElts; ++idx)
            {
                img_ui16_ref[numChannels * idx + 0] = adjustValue( ((idx / length / length) % length) * stepValue );
                img_ui16_ref[numChannels * idx + 1] = adjustValue( ((idx / length) % length) * stepValue );
                img_ui16_ref[numChannels * idx + 2] = adjustValue( (idx % length) * stepValue );

                img_ui16_ref[numChannels * idx + 3] = adjustValue( float(idx) / maxElts );
            }
        }

        if(testType==0 || testType==-1)
        {
            // Process the complete image (in place).

            if (inBitDepth == outBitDepth)
            {
                CustomMeasure m("Process the complete image (in place):\t\t\t\t", iterations);

                for(unsigned iter=0; iter<iterations; ++iter)
                {
                    std::vector<float>    inImg_f32  = img_f32_ref;
                    std::vector<uint16_t> inImg_ui16 = img_ui16_ref;

                    OCIO::PackedImageDesc imgDesc(inBitDepth == OCIO::BIT_DEPTH_F32
                                                    ? (void*)&inImg_f32[0] : (void*)&inImg_ui16[0], 
                                                  width, 
                                                  height,
                                                  numChannels,
                                                  inBitDepth,
                                                  OCIO::AutoStride,
                                                  OCIO::AutoStride,
                                                  OCIO::AutoStride);

                    // Apply the color transformation.
                    m.resume();
                    cpuProcessor->apply(imgDesc);
                    m.pause();
                }
            }

            // Process the complete image with input and output buffers.

            {
                std::vector<float>    inImg_f32 = img_f32_ref;
                std::vector<uint16_t> inImg_ui16 = img_ui16_ref;

                OCIO::PackedImageDesc inImgDesc(inBitDepth == OCIO::BIT_DEPTH_F32
                                                    ? (void*)&inImg_f32[0] : (void*)&inImg_ui16[0], 
                                                width, 
                                                height,
                                                numChannels,
                                                inBitDepth,
                                                OCIO::AutoStride,
                                                OCIO::AutoStride,
                                                OCIO::AutoStride);

                std::vector<float> outImg_f32 = img_f32_ref;
                outImg_f32.resize(outBitDepth == OCIO::BIT_DEPTH_F32 ? width * height * numChannels : 0);

                std::vector<uint16_t> outImg_ui16 = img_ui16_ref;
                outImg_ui16.resize(outBitDepth == OCIO::BIT_DEPTH_UINT16 ? width * height * numChannels : 0);

                OCIO::PackedImageDesc outImgDesc(outBitDepth == OCIO::BIT_DEPTH_F32 
                                                    ? (void*)&outImg_f32[0] : (void*)&outImg_ui16[0], 
                                                 width, 
                                                 height,
                                                 numChannels,
                                                 outBitDepth,
                                                 OCIO::AutoStride,
                                                 OCIO::AutoStride,
                                                 OCIO::AutoStride);

                // Use a custom cpu processor as input and output bit depths could be different.
                auto cpu = optProcessor->getOptimizedCPUProcessor(inBitDepth,
                                                                  outBitDepth,
                                                                  optimFlags);

                CustomMeasure m("Process the complete image (two buffers):\t\t\t", iterations);

                for(unsigned iter=0; iter<iterations; ++iter)
                {
                    // Apply the color transformation.
                    m.resume();
                    cpu->apply(inImgDesc, outImgDesc);
                    m.pause();
                }
            }
        }

        if ((testType == 1 || testType == -1) && inBitDepth == outBitDepth)
        {
            // Process line by line.

            std::vector<float>    inImg_f32  = img_f32_ref;
            std::vector<uint16_t> inImg_ui16 = img_ui16_ref;

            OCIO::PackedImageDesc inImgDesc(inBitDepth == OCIO::BIT_DEPTH_F32
                                                ? (void*)&inImg_f32[0] : (void*)&inImg_ui16[0],
                                            width, 
                                            height,
                                            numChannels,
                                            inBitDepth,
                                            OCIO::AutoStride,
                                            OCIO::AutoStride,
                                            OCIO::AutoStride);

            CustomMeasure m("Process the complete image (in place) but line by line:\t\t", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                ProcessLines(m, cpuProcessor, inImgDesc);
            }
        }

        if ((testType == 2 || testType == -1) && inBitDepth == outBitDepth && inBitDepth == OCIO::BIT_DEPTH_F32)
        {
            // Process pixel per pixel.

            std::vector<float> inImg_f32 = img_f32_ref;

            OCIO::PackedImageDesc inImgDesc((void*)&inImg_f32[0], 
                                            width, 
                                            height,
                                            numChannels);

            CustomMeasure m("Process the complete image (in place) but pixel per pixel:\t", iterations);

            for(unsigned iter=0; iter<iterations; ++iter)
            {
                ProcessPixels(m, cpuProcessor, inImgDesc);
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
