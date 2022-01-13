// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "GPUUnitTest.h"

#include "apputils/argparse.h"
#include "utils/StringUtils.h"

#include "oglapp.h"
#if __APPLE__
#include "metalapp.h"
#endif

namespace OCIO = OCIO_NAMESPACE;


namespace Shader
{
    // Default error threshold
    constexpr float defaultErrorThreshold = 1e-7f;

    // In some occasions, MAX_FLOAT will be "rounded" to infinity on some GPU renderers.
    // In order to avoid this issue, consider all number over/under a given threshold as
    // equal for testing purposes.
    constexpr float largeThreshold = std::numeric_limits<float>::max();

    enum LimitsDiff
    {
        NOT_APPLICABLE,
        INCONSISTANT,
        CONSISTANT
    };
    inline LimitsDiff ValidateInf(float x1, float x2)
    {
        if (fabs(x1) < largeThreshold && fabs(x2) < largeThreshold)
        {
            return NOT_APPLICABLE;
        }
        if (((x1 >= largeThreshold) && (x2 < largeThreshold)) ||
            ((x1 < largeThreshold) && (x2 >= largeThreshold)) ||
            ((x1 <= -largeThreshold) && (x2 > -largeThreshold)) ||
            ((x1 > -largeThreshold) && (x2 <= -largeThreshold)))
        {
            return INCONSISTANT;
        }
        return CONSISTANT;
    }

    inline LimitsDiff ValidateNan(float x1, float x2)
    {
        if (!std::isnan(x1))
        {
            if (!std::isnan(x2))
            {
                return NOT_APPLICABLE;
            }
            else
            {
                return INCONSISTANT;
            }
        }
        else
        {
            if (std::isnan(x2))
            {
                return CONSISTANT;
            }
            else
            {
                return INCONSISTANT;
            }
        }
    }

    inline bool AbsoluteDifference(float x1, float x2, float & diff)
    {
        const float thisDiff = fabs(x2 - x1);
        if (thisDiff > diff)
        {
            diff = thisDiff;
            return true;
        }
        return false;
    }

    inline bool RelativeDifference(float x1, float x2, float min_x1, float & diff)
    {
        const float absx1 = fabs(x1);
        const float div = std::max(absx1, min_x1);
        const float thisDiff = fabs(x1 - x2) / div;
        if (thisDiff > diff)
        {
            diff = thisDiff;
            return true;
        }
        return false;
    }

    // Return true if diff has been updated.
    inline bool ComputeDiff(float x1, float x2, bool rel, float min_x1, float & diff)
    {
        if (rel)
        {
            return RelativeDifference(x1, x2, min_x1, diff);
        }
        else
        {
            return AbsoluteDifference(x1, x2, diff);
        }

    }

}


OCIOGPUTest::OCIOGPUTest(const std::string & testgroup,
                         const std::string & testname,
                         OCIOTestFuncCallback test)
    :   m_group(testgroup)
    ,   m_name(testname)
    ,   m_function(test)
    ,   m_errorThreshold(Shader::defaultErrorThreshold)
{
}

void OCIOGPUTest::setProcessor(OCIO::TransformRcPtr transform)
{
    OCIO::ConfigRcPtr config = OCIO::Config::Create();
    config->setProcessorCacheFlags(OCIO::PROCESSOR_CACHE_OFF);
    setProcessor(config, transform);
}

void OCIOGPUTest::setProcessor(OCIO::ConstConfigRcPtr config,
                               OCIO::TransformRcPtr transform)
{
    setProcessor(config->getProcessor(transform));
}

void OCIOGPUTest::setProcessor(OCIO::ConstProcessorRcPtr processor)
{
    if (m_processor.get() != nullptr)
    {
        throw OCIO::Exception("GPU Unit test already exists");
    }

    m_processor = processor;
}

OCIO::GpuShaderDescRcPtr & OCIOGPUTest::getShaderDesc()
{
    if (!m_shaderDesc)
    {
        m_shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        m_shaderDesc->setLanguage(m_gpuShadingLanguage);
        m_shaderDesc->setPixelName("myPixel");
    }
    return m_shaderDesc;
}

void OCIOGPUTest::retestSetup(size_t idx)
{
    if (idx < m_retests.size())
    {
        m_retests[idx]();
    }
}

UnitTests& GetUnitTests()
{
    static UnitTests ocio_gpu_unit_tests;
    return ocio_gpu_unit_tests;
}

AddTest::AddTest(OCIOGPUTestRcPtr test)
{
    GetUnitTests().push_back(test);
}


namespace
{
    constexpr unsigned g_winWidth   = 256;
    constexpr unsigned g_winHeight  = 256;
    constexpr unsigned g_components = 4;

    void AllocateImageTexture(OCIO::OglAppRcPtr & app)
    {
        const unsigned numEntries = g_winWidth * g_winHeight * g_components;
        OCIOGPUTest::CustomValues::Values image(numEntries, 0.0f);

        app->initImage(g_winWidth, g_winHeight, OCIO::OglApp::COMPONENTS_RGBA, &image[0]);
    }

    void SetTestValue(float * image, float val, unsigned numComponents)
    {
        for (unsigned component = 0; component < numComponents; ++component)
        {
            for (unsigned idx = 0; idx < numComponents; ++idx)
            {
                const float valSet = (idx == component) ? val : 0.0f;
                image[idx+component*numComponents] = valSet;
            }
        }
    }

    void UpdateImageTexture(OCIO::OglAppRcPtr & app, OCIOGPUTestRcPtr & test)
    {
        // Note: User-specified custom values are padded out
        // to the preferred size (g_winWidth x g_winHeight).

        const unsigned predefinedNumEntries = g_winWidth * g_winHeight * g_components;

        if (test->getCustomValues().m_inputValues.empty())
        {
            // It means to generate the input values.

            const bool testWideRange = test->getTestWideRange();

#if __APPLE__ && __aarch64__
            // The Apple M1 chip handles differently the Nan and Inf processing introducing
            // differences with CPU processing.
            const bool testNaN = false;
            const bool testInfinity = false;
#else
            const bool testNaN = test->getTestNaN();
            const bool testInfinity = test->getTestInfinity();
#endif

            const float min = testWideRange ? -1.0f : 0.0f;
            const float max = testWideRange ? +2.0f : 1.0f;
            const float range = max - min;

            OCIOGPUTest::CustomValues tmp;
            tmp.m_originalInputValueSize = predefinedNumEntries;
            tmp.m_inputValues = OCIOGPUTest::CustomValues::Values(predefinedNumEntries, min);

            unsigned idx = 0;
            unsigned numEntries = predefinedNumEntries;
            const unsigned numTests = g_components * g_components;
            if (testNaN)
            {
                const float qnan = std::numeric_limits<float>::quiet_NaN();
                SetTestValue(&tmp.m_inputValues[0], qnan, g_components);
                idx += numTests;
                numEntries -= numTests;
            }

            if (testInfinity)
            {
                const float posinf = std::numeric_limits<float>::infinity();
                SetTestValue(&tmp.m_inputValues[idx], posinf, g_components);
                idx += numTests;
                numEntries -= numTests;

                const float neginf = -std::numeric_limits<float>::infinity();
                SetTestValue(&tmp.m_inputValues[idx], neginf, g_components);
                idx += numTests;
                numEntries -= numTests;
            }

            // Compute the value step based on the remaining number of values.
            const float step = range / float(numEntries);

            for (; idx < predefinedNumEntries; ++idx)
            {
                tmp.m_inputValues[idx] = min + step * float(idx);
            }

            test->setCustomValues(tmp);
        }
        else
        {
            // It means to use the custom input values.

            const OCIOGPUTest::CustomValues::Values & existingInputValues
                = test->getCustomValues().m_inputValues;

            const size_t numInputValues = existingInputValues.size();
            if (0 != (numInputValues%g_components))
            {
                throw OCIO::Exception("Only the RGBA input values are supported");
            }

            test->getCustomValues().m_originalInputValueSize = numInputValues;

            if (numInputValues > predefinedNumEntries)
            {
                throw OCIO::Exception("Exceed the predefined texture maximum size");
            }
            else if (numInputValues < predefinedNumEntries)
            {
                OCIOGPUTest::CustomValues values;
                values.m_originalInputValueSize = existingInputValues.size();

                // Resize the buffer to fit the expected input image size.
                values.m_inputValues.resize(predefinedNumEntries, 0);

                for (size_t idx = 0; idx < numInputValues; ++idx)
                {
                    values.m_inputValues[idx] = existingInputValues[idx];
                }

                test->setCustomValues(values);
            }
        }

        const OCIOGPUTest::CustomValues & values = test->getCustomValues();

        if (predefinedNumEntries != values.m_inputValues.size())
        {
            throw OCIO::Exception("Missing some expected input values");
        }

        app->updateImage(&values.m_inputValues[0]);
    }

    void UpdateOCIOGLState(OCIO::OglAppRcPtr & app, OCIOGPUTestRcPtr & test)
    {
        app->setPrintShader(test->isVerbose());

        OCIO::ConstProcessorRcPtr & processor = test->getProcessor();
        OCIO::GpuShaderDescRcPtr & shaderDesc = test->getShaderDesc();
        
        OCIO::ConstGPUProcessorRcPtr gpu;
        if (test->isLegacyShader())
        {
            gpu = processor->getOptimizedLegacyGPUProcessor(OCIO::OPTIMIZATION_DEFAULT, 
                                                            test->getLegacyShaderLutEdge());
        }
        else
        {
            gpu = processor->getDefaultGPUProcessor();
        }

        // Collect the shader program information for a specific processor.
        gpu->extractGpuShaderInfo(shaderDesc);

        app->setShader(shaderDesc);
    }

    void DiffComponent(const std::vector<float> & cpuImage,
                       const std::vector<float> & gpuImage,
                       size_t idx, bool relativeTest, float expectMin,
                       float & diff, size_t & idxDiff,
                       size_t & idxInf, size_t & idxNan)
    {
        float cpuVal = cpuImage[idx];
        float gpuVal = gpuImage[idx];
        Shader::LimitsDiff infDiff = Shader::ValidateInf(cpuVal, gpuVal);
        if (infDiff == Shader::NOT_APPLICABLE)
        {
            Shader::LimitsDiff nanDiff = Shader::ValidateNan(cpuVal, gpuVal);
            if (nanDiff == Shader::NOT_APPLICABLE)
            {
                if (Shader::ComputeDiff(cpuVal, gpuVal, relativeTest, expectMin, diff))
                {
                    idxDiff = idx;
                }
            }
            else if (nanDiff == Shader::INCONSISTANT)
            {
                idxNan = idx;
            }
        }
        else if (infDiff == Shader::INCONSISTANT)
        {
            idxInf = idx;
        }
    }

    constexpr size_t invalidIndex = std::numeric_limits<size_t>::max();

    // Validate the GPU processing against the CPU one.
    void ValidateImageTexture(OCIO::OglAppRcPtr & app, OCIOGPUTestRcPtr & test)
    {
        // Each retest is rebuilding a cpu proc.
        OCIO::ConstCPUProcessorRcPtr processor = test->getProcessor()->getDefaultCPUProcessor();

        const float epsilon = test->getErrorThreshold();
        const float expectMinValue = test->getExpectedMinimalValue();

        // Compute the width & height to avoid testing the padded values.

        const size_t numPixels = test->getCustomValues().m_originalInputValueSize / g_components;

        size_t width, height = 0;
        if(numPixels<=g_winWidth)
        {
            width  = numPixels;
            height = 1;
        }
        else
        {
            width  = g_winWidth;
            height = numPixels/g_winWidth;
            if((numPixels%g_winWidth)>0) height += 1;
        }

        if(width==0 || width>g_winWidth || height==0 || height>g_winHeight)
        {
            throw OCIO::Exception("Mismatch with the expected image size");
        }

        // Step 1: Compute the output using the CPU engine.

        OCIOGPUTest::CustomValues::Values cpuImage = test->getCustomValues().m_inputValues;
        OCIO::PackedImageDesc desc(&cpuImage[0], (long)width, (long)height, g_components);
        processor->apply(desc);

        // Step 2: Grab the GPU output from the rendering buffer.

        OCIOGPUTest::CustomValues::Values gpuImage(g_winWidth*g_winHeight*g_components, 0.0f);
        app->readImage(&gpuImage[0]);

        // Step 3: Compare the two results.

        const OCIOGPUTest::CustomValues::Values & image = test->getCustomValues().m_inputValues;
        float diff = 0.0f;
        size_t idxDiff = invalidIndex;
        size_t idxNan = invalidIndex;
        size_t idxInf = invalidIndex;
        const bool relativeTest = test->getRelativeComparison();
        for(size_t idx=0; idx<(width*height); ++idx)
        {
            DiffComponent(cpuImage, gpuImage, 4 * idx + 0, relativeTest, expectMinValue,
                          diff, idxDiff, idxInf, idxNan);
            DiffComponent(cpuImage, gpuImage, 4 * idx + 1, relativeTest, expectMinValue,
                          diff, idxDiff, idxInf, idxNan);
            DiffComponent(cpuImage, gpuImage, 4 * idx + 2, relativeTest, expectMinValue,
                          diff, idxDiff, idxInf, idxNan);
            DiffComponent(cpuImage, gpuImage, 4 * idx + 3, relativeTest, expectMinValue,
                          diff, idxDiff, idxInf, idxNan);
        }

        size_t componentIdx = idxDiff % 4;
        size_t pixelIdx = idxDiff / 4;
        if (diff > epsilon || idxInf != invalidIndex || idxNan != invalidIndex)
        {
            std::stringstream err;
            err << std::setprecision(10)
                << "\nMaximum error: " << diff << " at pixel: " << pixelIdx
                << " on component " << componentIdx;
            if (diff > epsilon)
            {
                err << std::setprecision(10)
                    << " larger than epsilon.\nscr = {"
                    << image[4 * pixelIdx + 0] << ", " << image[4 * pixelIdx + 1] << ", "
                    << image[4 * pixelIdx + 2] << ", " << image[4 * pixelIdx + 3] << "}"
                    << "\ncpu = {"
                    << cpuImage[4 * pixelIdx + 0] << ", " << cpuImage[4 * pixelIdx + 1] << ", "
                    << cpuImage[4 * pixelIdx + 2] << ", " << cpuImage[4 * pixelIdx + 3] << "}"
                    << "\ngpu = {"
                    << gpuImage[4 * pixelIdx + 0] << ", " << gpuImage[4 * pixelIdx + 1] << ", "
                    << gpuImage[4 * pixelIdx + 2] << ", " << gpuImage[4 * pixelIdx + 3] << "}\n"
                    << (test->getRelativeComparison() ? "relative " : "absolute ")
                    << "tolerance="
                    << epsilon;
            }
            if (idxInf != invalidIndex)
            {
                componentIdx = idxInf % 4;
                pixelIdx = idxInf / 4;
                err << std::setprecision(10)
                    << "\nLarge number error: " << diff << " at pixel: " << pixelIdx
                    << " on component " << componentIdx
                    << ".\nsrc = {"
                    << image[4 * pixelIdx + 0] << ", " << image[4 * pixelIdx + 1] << ", "
                    << image[4 * pixelIdx + 2] << ", " << image[4 * pixelIdx + 3] << "}"
                    << "\ncpu = {"
                    << cpuImage[4 * pixelIdx + 0] << ", " << cpuImage[4 * pixelIdx + 1] << ", "
                    << cpuImage[4 * pixelIdx + 2] << ", " << cpuImage[4 * pixelIdx + 3] << "}"
                    << "\ngpu = {"
                    << gpuImage[4 * pixelIdx + 0] << ", " << gpuImage[4 * pixelIdx + 1] << ", "
                    << gpuImage[4 * pixelIdx + 2] << ", " << gpuImage[4 * pixelIdx + 3] << "}\n";
            }
            if (idxNan != invalidIndex)
            {
                componentIdx = idxNan % 4;
                pixelIdx = idxNan / 4;
                err << std::setprecision(10)
                    << "\nNAN error: " << diff << " at pixel: " << pixelIdx
                    << " on component " << componentIdx
                    << ".\nsrc = {"
                    << image[4 * pixelIdx + 0] << ", " << image[4 * pixelIdx + 1] << ", "
                    << image[4 * pixelIdx + 2] << ", " << image[4 * pixelIdx + 3] << "}"
                    << "\ncpu = {"
                    << cpuImage[4 * pixelIdx + 0] << ", " << cpuImage[4 * pixelIdx + 1] << ", "
                    << cpuImage[4 * pixelIdx + 2] << ", " << cpuImage[4 * pixelIdx + 3] << "}"
                    << "\ngpu = {"
                    << gpuImage[4 * pixelIdx + 0] << ", " << gpuImage[4 * pixelIdx + 1] << ", "
                    << gpuImage[4 * pixelIdx + 2] << ", " << gpuImage[4 * pixelIdx + 3] << "}\n";
            }
            throw OCIO::Exception(err.str().c_str());
        }
        else
        {
            test->updateMaxDiff(diff, idxDiff);
        }
    }
};

int main(int argc, const char ** argv)
{

#if !defined(NDEBUG) && defined(_WIN32)
    // Disable the 'assert' dialog box in debug mode.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif

    bool printHelp = false;
    bool useMetalRenderer = false;
    bool verbose = false;
    bool stopOnFirstError = false;

    // Note that empty strings mean to run all the unit tests.
    std::string filter, utestGroupAllowed, utestNameAllowed;

    ArgParse ap;
    ap.options("\nCommand line arguments:\n",
               "--help",          &printHelp,        "Print help message",
               "--metal",         &useMetalRenderer, "Run the GPU unit test with Metal",
               "-v",              &verbose,          "Output the GPU shader program",
               "--stop_on_error", &stopOnFirstError, "Stop on the first error",
               "--run_only %s",   &filter,           "Run only some unit tests\n"
                                                     "\tex: --run_only ExponentOp/forward i.e. run only \"ExponentOp/forward\"\n"
                                                     "\tex: --run_only ExponentOp         i.e. run \"ExponentOp/*\"\n"
                                                     "\tex: --run_only /forward           i.e. run \"*/forward\"\n",
               nullptr);

    if (ap.parse(argc, argv) < 0)
    {
        std::cerr << ap.geterror() << std::endl;
        ap.usage();
        return 1;
    }

    if (printHelp)
    {
        ap.usage();
        return 1;
    }

    if (!filter.empty())
    {
        const std::vector<std::string> results = StringUtils::Split(filter, '/');
        if (!results.empty())
        {
            utestGroupAllowed = StringUtils::Lower(StringUtils::Trim(results[0]));
            if (results.size() >= 2)
            {
                utestNameAllowed = StringUtils::Lower(StringUtils::Trim(results[1]));

                if (results.size() >= 3)
                {
                    std::cerr << "Invalid value for the argument '--run_only'." << std::endl;
                    ap.usage();
                    return 1;
                }
            }
        }
    }

    // Step 1: Initialize the graphic library engines.
    OCIO::OglAppRcPtr app;
    
    try
    {
        if(useMetalRenderer)
        {
#if __APPLE__
            app = OCIO::MetalApp::CreateMetalGlApp("GPU tests - Metal", 10, 10);
#else
            std::cerr << std::endl << "'GPU tests - Metal' is not supported" << std::endl;
            return 1;
#endif
        }
        else
        {
            app = OCIO::OglApp::CreateOglApp("GPU tests", 10, 10);
        }
    }
    catch (const OCIO::Exception & e)
    {
        std::cerr << std::endl << e.what() << std::endl;
        return 1;
    }

    app->printGLInfo();

    // Step 2: Allocate the texture that holds the image.
    AllocateImageTexture(app);

    // Step 3: Create the frame buffer and render buffer.
    app->createGLBuffers();

    app->reshape(g_winWidth, g_winHeight);

    // Step 4: Execute all the unit tests.

    unsigned failures = 0;

    std::cout << "\n OpenColorIO_Core_GPU_Unit_Tests\n\n";

    UnitTests & tests = GetUnitTests();
    const size_t numTests = tests.size();
    for(size_t idx=0; idx<numTests; ++idx)
    {
        const unsigned curr_failures = failures;

        OCIOGPUTestRcPtr test = tests[idx];

        // Is that a unit test to run ?

        const std::string utestGroup = test->group();
        const std::string utestName  = test->name();

        bool utestAllowed = true;

        if (!utestGroupAllowed.empty() && StringUtils::Lower(utestGroup)!=utestGroupAllowed)
        {
            utestAllowed = false;
        }

        if (!utestNameAllowed.empty() && StringUtils::Lower(utestName)!=utestNameAllowed)
        {
            utestAllowed = false;
        }

        if (!utestAllowed)
        {
            continue;
        }

        // Prepare the unit test.

        test->setVerbose(verbose);
        test->setShadingLanguage(
#if __APPLE__
            useMetalRenderer ?
            OCIO::GPU_LANGUAGE_MSL_2_0 :
#endif
            OCIO::GPU_LANGUAGE_GLSL_1_2);

        bool enabledTest = true;
        try
        {
            test->setup();

            enabledTest = test->isEnabled();

            constexpr size_t maxCharToDisplay = 49;

            std::string name(test->group());
            name += " / " + test->name();

            if (name.size() > maxCharToDisplay)
            {
                name.resize(maxCharToDisplay);
            }

            std::cout << "["
                      << std::right << std::setw(3)
                      << (idx+1) << "/" << numTests << "] ["
                      << std::left << std::setw(maxCharToDisplay+1)
                      << name << "] - ";

            if(test->isValid() && enabledTest)
            {
                // Initialize the texture with the RGBA values to be processed.
                UpdateImageTexture(app, test);

                // Update the GPU shader program.
                UpdateOCIOGLState(app, test);

                const size_t numRetest = test->getNumRetests();
                // Need to run once and for each retest.
                for (size_t idxRetest = 0; idxRetest <= numRetest; ++idxRetest)
                {
                    if (idxRetest != 0) // Skip first run.
                    {
                        // Call the retest callback.
                        test->retestSetup(idxRetest - 1);
                    }

                    // Process the image texture into the rendering buffer.
                    app->redisplay();

                    // Compute the expected values using the CPU and compare
                    // against the GPU values.
                    ValidateImageTexture(app, test);
                }
            }
        }
        catch(OCIO::Exception & ex)
        {
            ++failures;
            std::cerr << "FAILED - " << ex.what() << std::endl;
        }
        catch(...)
        {
            ++failures;
            std::cerr << "FAILED - Unexpected error" << std::endl;
        }

        if (!enabledTest)
        {
            std::cout << "DISABLED" << std::endl;
        }
        else if(curr_failures==failures && test->isValid())
        {
            const size_t idxMaxDiff = test->getMaxDiffIndex();
            const size_t componentIdx = idxMaxDiff % 4;
            const size_t pixelIdx = idxMaxDiff / 4;

            std::cout << "PASSED - (MaxDiff: " << test->getMaxDiff()
                      << " at pix[" << pixelIdx
                      << "][" << componentIdx << "])" << std::endl;
        }
        else if(!test->isValid())
        {
            ++failures;
            std::cerr << "FAILED - Invalid test" << std::endl;
        }

        // Get rid of the test.
        tests[idx] = nullptr;
    }

    std::cout << std::endl << failures << " tests failed" << std::endl << std::endl;
    return failures;
}
