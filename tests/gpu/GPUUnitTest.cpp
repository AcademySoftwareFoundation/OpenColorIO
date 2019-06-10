/*
Copyright (c) 2019 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

#include "GPUUnitTest.h"


#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <GLUT/glut.h>
#elif _WIN32
#include <GL/glew.h>
#include <GL/glut.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif


#include <sstream>
#include <map>
#include <iomanip>
#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <algorithm>


#include "glsl.h"


namespace Shader
{
    // Default error threshold
    const float defaultErrorThreshold = 1e-7f;

    // In some occasions, MAX_FLOAT will be "rounded" to infinity on some GPU renderers.
    // In order to avoid this issue, consider all number over/under a given threshold as
    // equal for testing purposes.
    const float largeThreshold = std::numeric_limits<float>::max();

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
        float div = std::max(absx1, min_x1);
        const float thisDiff = fabs(x1 - x2) / div;
        if (thisDiff > diff)
        {
            diff = thisDiff;
            return true;
        }
        return false;
    }

    // Return ture if diff has been updated. 
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


OCIOGPUTest::OCIOGPUTest(const std::string& testgroup, 
                         const std::string& testname, 
                         OCIOTestFunc test) 
    :   m_group(testgroup)
    ,   m_name(testname)
    ,   m_function(test)
    ,   m_errorThreshold(Shader::defaultErrorThreshold)
{
}

OCIOGPUTest::~OCIOGPUTest()
{
}

void OCIOGPUTest::setContext(OCIO_NAMESPACE::TransformRcPtr transform, 
                             OCIO_NAMESPACE::GpuShaderDescRcPtr shaderDesc)
{
    OCIO_NAMESPACE::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    setContext(config, transform, shaderDesc);
}

void OCIOGPUTest::setContext(OCIO_NAMESPACE::ConstConfigRcPtr config,
                             OCIO_NAMESPACE::TransformRcPtr transform, 
                             OCIO_NAMESPACE::GpuShaderDescRcPtr shaderDesc)
{
    setContextProcessor(config->getProcessor(transform), shaderDesc);
}

void OCIOGPUTest::setContextProcessor(OCIO_NAMESPACE::ConstProcessorRcPtr processor,
                                      OCIO_NAMESPACE::GpuShaderDescRcPtr shaderDesc)
{
    if(m_processor.get()!=0x0)
    {
        throw OCIO_NAMESPACE::Exception("GPU Unit test already exists");
    }

    m_shaderDesc     = shaderDesc;
    m_processor      = processor;
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
    GLint g_win = 0;
    static constexpr unsigned g_winWidth   = 256;
    static constexpr unsigned g_winHeight  = 256;
    static constexpr unsigned g_components = 4;

    OCIO::OpenGLBuilderRcPtr g_oglBuilder;

    GLuint g_imageTexID;

    void AllocateImageTexture()
    {
        const unsigned numEntries = g_winWidth * g_winHeight * g_components;
        OCIOGPUTest::CustomValues::Values image(numEntries, 0.0f);

        glGenTextures(1, &g_imageTexID);

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, g_imageTexID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, g_winWidth, g_winHeight, 0,
                     GL_RGBA, GL_FLOAT, &image[0]);
    }

    void Reshape()
    {
        glViewport(0, 0, g_winWidth, g_winHeight);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, g_winWidth, 0.0, g_winHeight, -100.0, 100.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    void Redisplay(void)
    {
        glEnable(GL_TEXTURE_2D);
            glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glColor3f(1, 1, 1);       
            glPushMatrix();
                glBegin(GL_QUADS);
                    glTexCoord2f(0.0f, 1.0f);
                    glVertex2f(0.0f, (float)g_winHeight);
                    
                    glTexCoord2f(0.0f, 0.0f);
                    glVertex2f(0.0f, 0.0f);
                    
                    glTexCoord2f(1.0f, 0.0f);
                    glVertex2f((float)g_winWidth, 0.0f);
                    
                    glTexCoord2f(1.0f, 1.0f);
                    glVertex2f((float)g_winWidth, (float)g_winHeight);
                glEnd();
            glPopMatrix();   
        glDisable(GL_TEXTURE_2D);
        
        glutSwapBuffers();
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

    void UpdateImageTexture(OCIOGPUTestRcPtr test)
    {
        // Note: User-specified custom values are padded out 
        // to the preferred size (g_winWidth x g_winHeight).

        const unsigned predefinedNumEntries
            = g_winWidth * g_winHeight * g_components;

        if (test->getCustomValues().m_inputValues.empty())
        {
            // It means to generate the input values.

            const bool testWideRange = test->getTestWideRange();
            const bool testNaN = test->getTestNaN();
            const bool testInfinity = test->getTestInfinity();

            const float min = testWideRange ? -1.0f : 0.0f;
            const float max = testWideRange ? +2.0f : 1.0f;
            const float range = max - min;

            OCIOGPUTest::CustomValues tmp;
            tmp.m_originalInputValueSize = predefinedNumEntries;
            tmp.m_inputValues
                = OCIOGPUTest::CustomValues::Values(predefinedNumEntries, min);

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

        glBindTexture(GL_TEXTURE_2D, g_imageTexID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, g_winWidth, g_winHeight, 0,
                     GL_RGBA, GL_FLOAT, &values.m_inputValues[0]);
    }

    void UpdateOCIOGLState(OCIOGPUTestRcPtr test)
    {
        OCIO::ConstProcessorRcPtr & processor = test->getProcessor();
        OCIO::GpuShaderDescRcPtr & shaderDesc = test->getShaderDesc();

        // Step 1: Update the GPU shader description.
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);

        // Step 2: Collect the shader program information for a specific processor.   
        processor->extractGpuShaderInfo(shaderDesc);

        // Step 3: Create the OpenGL builder to prepare the GPU shader program.
        g_oglBuilder = OCIO::OpenGLBuilder::Create(shaderDesc);
        g_oglBuilder->setVerbose(test->isVerbose());

        // Step 4: Allocate & upload all the LUTs in a dedicated GPU texture.
        g_oglBuilder->allocateAllTextures(1);

        std::ostringstream main;
        main << std::endl
             << "uniform sampler2D img;" << std::endl
             << std::endl
             << "void main()" << std::endl
             << "{" << std::endl
             << "    vec4 col = texture2D(img, gl_TexCoord[0].st);" << std::endl
             << "    gl_FragColor = " << shaderDesc->getFunctionName() << "(col);" << std::endl
             << "}" << std::endl;

        // Step 5: Build the fragment shader program.
        g_oglBuilder->buildProgram(main.str().c_str());

        // Step 6: Enable the fragment shader program, and all needed resources.
        g_oglBuilder->useProgram();
        glUniform1i(glGetUniformLocation(g_oglBuilder->getProgramHandle(), "img"), 0);
        g_oglBuilder->useAllTextures();
        g_oglBuilder->useAllUniforms();
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

    static constexpr size_t invalidIndex = std::numeric_limits<size_t>::max();

    // Validate the GPU processing against the CPU one.
    void ValidateImageTexture(OCIOGPUTestRcPtr test)
    {
        OCIO::ConstProcessorRcPtr & processor = test->getProcessor();
        const float epsilon = test->getErrorThreshold();
        const float expectMinValue = test->getExpectedMinimalValue();

        // Compute the width & height to avoid testing the padded values.

        const size_t numPixels 
            = test->getCustomValues().m_originalInputValueSize / g_components;

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
        OCIO_NAMESPACE::PackedImageDesc desc(&cpuImage[0], (long)width, (long)height, g_components);
        processor->apply(desc);

        // Step 2: Grab the GPU output from the rendering buffer.

        OCIOGPUTest::CustomValues::Values gpuImage(g_winWidth*g_winHeight*g_components, 0.0f);
        glReadBuffer( GL_COLOR_ATTACHMENT0 );
        glReadPixels(0, 0, g_winWidth, g_winHeight, GL_RGBA, GL_FLOAT, (GLvoid*)&gpuImage[0]);

        glutSwapBuffers();

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
                size_t componentIdx = idxInf % 4;
                size_t pixelIdx = idxInf / 4;
                err << std::setprecision(10)
                    << "\nLarge number error: " << diff << " at pixel: " << pixelIdx
                    << " on component " << componentIdx
                    << ".\nscr = {"
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
                size_t componentIdx = idxNan % 4;
                size_t pixelIdx = idxNan / 4;
                err << std::setprecision(10)
                    << "\nNAN error: " << diff << " at pixel: " << pixelIdx
                    << " on component " << componentIdx
                    << ".\nscr = {"
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

int main(int, char **)
{
    int argc = 2;
    const char* argv[] = { "main", "-glDebug" };
    glutInit(&argc, const_cast<char**>(&argv[0]));
    
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(g_winWidth, g_winHeight);
    glutInitWindowPosition(0, 0);

    g_win = glutCreateWindow(argv[0]);
    
#ifndef __APPLE__
    glewInit();
    if (!glewIsSupported("GL_VERSION_2_0"))
    {
        std::cout << "OpenGL 2.0 not supported" << std::endl;
        exit(1);
    }
#endif

    // Step 1: Initilize the OpenGL engine.

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);           // 4-byte pixel alignment

#ifndef __APPLE__
    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);     //
    glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);   // avoid any kind of clamping
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE); //
#endif

    glEnable(GL_TEXTURE_2D);
    glClearColor(0, 0, 0, 0);                        // background color
    glClearStencil(0);                               // clear stencil buffer

    // Step 2: Allocate the texture that holds the image.

    AllocateImageTexture();

    // Step 3: Create the frame buffer and render buffer.

    GLuint fboId;

    // Create a framebuffer object, you need to delete them when program exits.
    glGenFramebuffers(1, &fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);


    GLuint rboId;

    // Create a renderbuffer object to store depth info.
    glGenRenderbuffers(1, &rboId);
    glBindRenderbuffer(GL_RENDERBUFFER, rboId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F_ARB, g_winWidth, g_winHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Attach a texture to FBO color attachement point.
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_imageTexID, 0);

    // Attach a renderbuffer to depth attachment point.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rboId);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Step 4: Execute all the unit tests.

    unsigned failures = 0;

    std::cerr << "\n OpenColorIO_Core_GPU_Unit_Tests\n\n";

    UnitTests & tests = GetUnitTests();
    const size_t numTests = tests.size();
    for(size_t idx=0; idx<numTests; ++idx)
    {
        const unsigned curr_failures = failures;
     
        OCIOGPUTestRcPtr test = tests[idx];

        bool enabledTest = true;
        try
        {
            test->setup();
            enabledTest = test->isEnabled();

            std::string name(test->group());
            name += " / " + test->name();

            std::cerr << "[" 
                      << std::right << std::setw(3)
                      << (idx+1) << "/" << numTests << "] ["
                      << std::left << std::setw(50)
                      << name << "] - ";

            if(test->isValid() && enabledTest)
            {
                // Set the rendering destination to FBO.
                glBindFramebuffer(GL_FRAMEBUFFER, fboId);
                
                // Initialize the texture with the RGBA values to be processed.
                UpdateImageTexture(test);

                // Update the GPU shader program.
                UpdateOCIOGLState(test);

                const size_t numRetest = test->getNumRetests();
                // Need to run once and for each retest.  
                for (size_t idx = 0; idx <= numRetest; ++idx)
                {
                    if (idx != 0) // Skip first run.
                    {
                        // Call the retest callback.
                        test->retestSetup(idx - 1);
                        // Update uniforms with dynamic properties.
                        g_oglBuilder->useAllUniforms();
                    }

                    // Clear buffer.
                    glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    // Process the image texture into the rendering buffer.
                    Reshape();
                    Redisplay();

                    // Compute the expected values using the CPU and compare
                    // against the GPU values.
                    ValidateImageTexture(test);
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
            std::cerr << "DISABLED" << std::endl;
        }
        else if(curr_failures==failures && test->isValid())
        {
            size_t idx = test->getMaxDiffIndex();
            size_t componentIdx = idx % 4;
            size_t pixelIdx = idx / 4;

            std::cerr << "PASSED - (MaxDiff: " << test->getMaxDiff()
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

        glUseProgram(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    std::cerr << std::endl << failures << " tests failed" << std::endl << std::endl;

    return failures;
}
