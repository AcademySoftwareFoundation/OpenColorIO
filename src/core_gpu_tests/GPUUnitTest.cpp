/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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
#include <GL/glext.h>
#include <GL/glut.h>
#endif


#include <sstream>
#include <map>
#include <iomanip>
#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <math.h>


#include "glsl.h"


#if defined __APPLE__
    #define F_ISNAN(a) __inline_isnanf(a)
#elif defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS) || defined(_MSC_VER)
    #define F_ISNAN(a) (_isnan(a)!=0)
#else
    #define F_ISNAN(a) __isnanf(a)
#endif



namespace Shader
{
    // Default error threshold
    const float defaultErrorThreshold = 1e-7f;

    // In some occasions, MAX_FLOAT will be "rounded" to infinity on some GPU renderers.
    // In order to avoid this issue, consider all number over/under a given threshold as
    // equal for testing purposes.
    const float largeThreshold = std::numeric_limits<float>::max();

    // Code copied from core/MathUtils.h
    inline bool EqualWithAbsError(float x1, float x2, float e)
    {
        return ((x1 > x2) ? x1 - x2 : x2 - x1) <= e;
    }

    // Relative comparison: check if the difference between value and expected
    // relative to (divided by) expected does not exceed the eps.  A minimum
    // expected value is used to limit the scaling of the difference and
    // avoid large relative differences for small numbers.
    inline bool EqualWithSafeRelError(float value,
                                      float expected,
                                      float eps,
                                      float minExpected)
    {
        const float div = (expected > 0.0f) ?
            ((expected < minExpected) ? minExpected : expected) :
            ((-expected < minExpected) ? minExpected : -expected);

        return (
            ((value > expected) ? value - expected : expected - value)
            / div) <= eps;
    }
    // end of copy from core/MathUtils.h

    // Compute the absolute equality of two floats
    // a is the first float to compare
    // b is the first float to compare
    // epsilon is the maximum expected epsilon
    bool AbsoluteCompare(float a, float b, float epsilon)
    {
        if ( ( (a >=  largeThreshold) && (b >=  largeThreshold) ) ||
             ( (a <= -largeThreshold) && (b <= -largeThreshold) ) ||
             ( F_ISNAN(a) && F_ISNAN(b) ) )
        {
            return true;
        }

        return EqualWithAbsError(a, b, epsilon);
    }

    // Compute the relative equality of two floats
    // a is the first float to compare
    // b is the first float to compare
    // epsilon is the maximum expected epsilon
    // expectedMinValue is the minimum expected value
    bool RelativeCompare(float a, float b, float epsilon, float expectedMinValue)
    {
        if ( ( (a >=  largeThreshold) && (b >=  largeThreshold) ) ||
             ( (a <= -largeThreshold) && (b <= -largeThreshold) ) ||
             ( F_ISNAN(a) && F_ISNAN(b) ) )
        {
          return true;
        }

        return EqualWithSafeRelError(a, b, epsilon, expectedMinValue);
    }
}


OCIOGPUTest::OCIOGPUTest(const std::string& testgroup, 
                         const std::string& testname, 
                         OCIOTestFunc test) 
    :   m_group(testgroup)
    ,   m_name(testname)
    ,   m_function(test)
    ,   m_errorThreshold(Shader::defaultErrorThreshold)
    ,   m_useWideRange(true)
    ,   m_performRelativeComparison(false)
    ,   m_expectedMinimalValue(1e-6f)
    ,   m_verbose(false)
{
}

void OCIOGPUTest::setContext(OCIO_NAMESPACE::TransformRcPtr transform, 
                             OCIO_NAMESPACE::GpuShaderDescRcPtr shaderDesc)
{
    if(m_processor.get()!=0x0)
    {
        throw OCIO_NAMESPACE::Exception("GPU Unit test already exists");
    }

    OCIO_NAMESPACE::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();

    m_shaderDesc     = shaderDesc;
    m_processor      = config->getProcessor(transform);
}

void OCIOGPUTest::setContext(OCIO_NAMESPACE::ConstProcessorRcPtr processor, 
                             OCIO_NAMESPACE::GpuShaderDescRcPtr shaderDesc)
{
    if(m_processor.get()!=0x0)
    {
        throw OCIO_NAMESPACE::Exception("GPU Unit test already exists");
    }

    m_shaderDesc     = shaderDesc;
    m_processor      = processor;
}


UnitTests& GetUnitTests()
{
    static UnitTests ocio_gpu_unit_tests;
    return ocio_gpu_unit_tests;
}

AddTest::AddTest(OCIOGPUTest* test)
{
    GetUnitTests().push_back(test);
}


namespace
{
    GLint g_win = 0;
    const unsigned g_winWidth   = 256;
    const unsigned g_winHeight  = 256;
    const unsigned g_components = 4;

    OpenGLBuilderRcPtr g_oglBuilder;

    std::vector<float> g_image;
    GLuint g_imageTexID;

    void AllocateImageTexture()
    {
        const unsigned numEntries = g_winWidth * g_winHeight * g_components;
        g_image.resize(numEntries);
        memset(&g_image[0], 0, numEntries * sizeof(float));

        glGenTextures(1, &g_imageTexID);

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, g_imageTexID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, g_winWidth, g_winHeight, 0,
                     GL_RGBA, GL_FLOAT, &g_image[0]);
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

    void CleanUp(void)
    {
        g_oglBuilder.reset();
        glutDestroyWindow(g_win);
    }

    void UpdateImageTexture(bool useWideRange)
    {
        const float min = useWideRange ? -1.0f : 0.0f;
        const float max = useWideRange ? +2.0f : 1.0f;
        const float range = max - min;

        const unsigned numEntries = g_winWidth * g_winHeight * g_components;
        const float step = range / numEntries;

        for(unsigned idx=0; idx<numEntries; ++idx)
        {
            g_image[idx] = min + step * float(idx);
        }

        glBindTexture(GL_TEXTURE_2D, g_imageTexID);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, g_winWidth, g_winHeight, 0,
            GL_RGBA, GL_FLOAT, &g_image[0]);
    }

    void UpdateOCIOGLState(OCIOGPUTest * test)
    {
        OCIO::ConstProcessorRcPtr & processor = test->getProcessor();
        OCIO::GpuShaderDescRcPtr & shaderDesc = test->getShaderDesc();

        // Step 1: Create the legacy GPU shader description
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_3);

        // Step 2: Collect the shader program information for a specific processor    
        processor->extractGpuShaderInfo(shaderDesc);

        // Step 3: Use the helper OpenGL builder
        g_oglBuilder = OpenGLBuilder::Create(shaderDesc);
        g_oglBuilder->setVerbose(test->isVerbose());

        // Step 4: Allocate & upload all the LUTs
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

        // Step 5: Build the fragment shader program
        g_oglBuilder->buildProgram(main.str().c_str());

        // Step 6: Enable the fragment shader program, and all needed textures
        g_oglBuilder->useProgram();
        glUniform1i(glGetUniformLocation(g_oglBuilder->getProgramHandle(), "img"), 0);
        g_oglBuilder->useAllTextures();
    }

    // Validate the GPU processing against the CPU one
    void ValidateImageTexture(OCIOGPUTest * test)
    {
        OCIO::ConstProcessorRcPtr & processor = test->getProcessor();
        const float epsilon = test->getErrorThreshold();
        const float expectMinValue = test->getExpectedMinimalValue();

        // Step 1: Compute the output using the CPU engine

        std::vector<float> cppImage = g_image;    
        OCIO_NAMESPACE::PackedImageDesc desc(&cppImage[0], g_winWidth, g_winHeight, g_components);
        processor->apply(desc);

        // Step 2: Grab the GPU output from the rendering buffer

        std::vector<float> gpuImage = g_image;
        glReadBuffer( GL_COLOR_ATTACHMENT0 );
        glReadPixels(0, 0, g_winWidth, g_winHeight, GL_RGBA, GL_FLOAT, (GLvoid*)&gpuImage[0]);

        // Step 3: Compare the two results

        for(size_t idx=0; idx<(g_winWidth * g_winHeight); ++idx)
        {
            const bool isFaulty 
                = test->getRelativeComparison()
                    ? (!Shader::RelativeCompare(cppImage[4*idx+0], gpuImage[4*idx+0], epsilon, expectMinValue) ||
                       !Shader::RelativeCompare(cppImage[4*idx+1], gpuImage[4*idx+1], epsilon, expectMinValue) ||
                       !Shader::RelativeCompare(cppImage[4*idx+2], gpuImage[4*idx+2], epsilon, expectMinValue) ||
                       !Shader::RelativeCompare(cppImage[4*idx+3], gpuImage[4*idx+3], epsilon, expectMinValue))
                    : (!Shader::AbsoluteCompare(cppImage[4*idx+0], gpuImage[4*idx+0], epsilon) ||
                       !Shader::AbsoluteCompare(cppImage[4*idx+1], gpuImage[4*idx+1], epsilon) ||
                       !Shader::AbsoluteCompare(cppImage[4*idx+2], gpuImage[4*idx+2], epsilon) ||
                       !Shader::AbsoluteCompare(cppImage[4*idx+3], gpuImage[4*idx+3], epsilon));
            if(isFaulty)
            {
                std::stringstream err;
                err << std::setprecision(10)
                    << "\n\tfrom orig[" << idx << "] = {" 
                    << g_image[4*idx+0] << ", " << g_image[4*idx+1] << ", "
                    << g_image[4*idx+2] << ", " << g_image[4*idx+3] << "}\n"
                    << "\tto  cpu = {"
                    << cppImage[4*idx+0] << ", " << cppImage[4*idx+1] << ", "
                    << cppImage[4*idx+2] << ", " << cppImage[4*idx+3] << "}\n"
                    << "\tand gpu = {" 
                    << gpuImage[4*idx+0] << ", " << gpuImage[4*idx+1] << ", "
                    << gpuImage[4*idx+2] << ", " << gpuImage[4*idx+3] << "}\n"
                    << "\twith epsilon=" 
                    << epsilon;
                throw OCIO::Exception(err.str().c_str());
            }
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

    // Step 1: Initilize the OpenGL engine

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);           // 4-byte pixel alignment

#ifndef __APPLE__
    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);     //
    glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);   // avoid any kind of clamping
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE); //
#endif

    glEnable(GL_TEXTURE_2D);
    glClearColor(0, 0, 0, 0);                        // background color
    glClearStencil(0);                               // clear stencil buffer

    // Step 2: Allocate the texture that holds the image

    AllocateImageTexture();

    // Step 3: Create the frame buffer and render buffer

    GLuint fboId;

    // create a framebuffer object, you need to delete them when program exits.
    glGenFramebuffers(1, &fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);


    GLuint rboId;

    // create a renderbuffer object to store depth info
    glGenRenderbuffers(1, &rboId);
    glBindRenderbuffer(GL_RENDERBUFFER, rboId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F_ARB, g_winWidth, g_winHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // attach a texture to FBO color attachement point
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_imageTexID, 0);

    // attach a renderbuffer to depth attachment point
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rboId);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Step 4: Execute all the unit tests

    unsigned failures = 0;

    std::cerr << "\n OpenColorIO_Core_GPU_Unit_Tests\n\n";

    const UnitTests & tests = GetUnitTests();
    const size_t numTests = tests.size();
    for(size_t idx=0; idx<numTests; ++idx)
    {
        const unsigned curr_failures = failures;
     
        OCIOGPUTest* test = tests[idx];

        try
        {
            test->setup();

            std::string name(test->group());
            name += " / " + test->name();

            std::cerr << "[" 
                      << std::right << std::setw(3)
                      << (idx+1) << "/" << numTests << "] ["
                      << std::left << std::setw(50)
                      << name << "] - ";

            if(test->isValid())
            {
                // Set the rendering destination to FBO
                glBindFramebuffer(GL_FRAMEBUFFER, fboId);
                
                // Clear buffer
                glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Update the image texture
                UpdateImageTexture(test->getWideRange());

                // Update the GPU shader program
                UpdateOCIOGLState(test);

                // Process the image texture into the rendering buffer
                Reshape();
                Redisplay();

                // Validate the processed image using the rendering buffer
                ValidateImageTexture(test);
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

        if(curr_failures==failures && test->isValid())
        {
            std::cerr << "PASSED" << std::endl;
        }
        else if(!test->isValid())
        {
            ++failures;
            std::cerr << "FAILED - Invalid test" << std::endl;
        }

        glUseProgram(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    std::cerr << std::endl << failures << " tests failed" << std::endl << std::endl;

    return failures;
}
