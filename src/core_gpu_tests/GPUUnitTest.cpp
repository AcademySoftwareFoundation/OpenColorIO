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


#include <algorithm>
#include <string>
#include <sstream>
#include <map>
#include <iomanip>

#include <stdlib.h>
#include <string.h>



OCIOGPUTest::OCIOGPUTest(const std::string& testgroup, const std::string& testname, OCIOTestFunc test) 
    : _group(testgroup), _name(testname), _function(test), _errorThreshold(1e-8f)
{
}

void OCIOGPUTest::setContext(OCIO_NAMESPACE::TransformRcPtr transform, float errorThreshold)
{ 
    OCIO_NAMESPACE::ConfigRcPtr config = OCIO_NAMESPACE::Config::Create();
    _processor = config->getProcessor(transform);
    _errorThreshold = errorThreshold;
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

    GLuint g_fragShader = 0;
    GLuint g_program = 0;

    std::vector<float> g_image;
    GLuint g_imageTexID;

    GLuint g_lut3dTexID;
    const int LUT3D_EDGE_SIZE = 32;
    std::vector<float> g_lut3d;
    std::string g_lut3dcacheid;
    std::string g_shadercacheid;

    void AllocateImageTexture()
    {
        const unsigned numEntries = g_winWidth * g_winHeight * g_components;
        g_image.resize(numEntries);

        glGenTextures(1, &g_imageTexID);

        glActiveTexture(GL_TEXTURE1);

        glBindTexture(GL_TEXTURE_2D, g_imageTexID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    void AllocateDefaultLut3D()
    {
        const unsigned num3Dentries = 3 * LUT3D_EDGE_SIZE * LUT3D_EDGE_SIZE * LUT3D_EDGE_SIZE;
        g_lut3d.resize(num3Dentries);
        
        glGenTextures(1, &g_lut3dTexID);
        
        glActiveTexture(GL_TEXTURE2);
        
        glBindTexture(GL_TEXTURE_3D, g_lut3dTexID);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
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
        glDeleteShader(g_fragShader);
        glDeleteProgram(g_program);
        glutDestroyWindow(g_win);
    }

    GLuint CompileShaderProgram(GLenum shaderType, const char *text)
    {
        GLuint shader;
        GLint stat;
        
        shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, (const GLchar **) &text, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &stat);
        
        if (!stat)
        {
            GLchar log[1000];
            GLsizei len;
            glGetShaderInfoLog(shader, 1000, &len, log);
            log[len] = '\0';

            std::string err("Shader compilation error: ");
            err += log;
            throw OCIO::Exception(err.c_str());
        }
        
        return shader;
    }

    GLuint LinkShaderProgram(GLuint fragShader)
    {
        if (!fragShader) return 0;
        
        GLuint program = glCreateProgram();
        
        if (fragShader)
            glAttachShader(program, fragShader);
        
        glLinkProgram(program);
        
        /* check link */
        {
            GLint stat;
            glGetProgramiv(program, GL_LINK_STATUS, &stat);
            if (!stat) {
                GLchar log[1000];
                GLsizei len;
                glGetProgramInfoLog(program, 1000, &len, log);
                log[len] = '\0';

                std::string err("Shader link error: ");
                err += log;
                throw OCIO::Exception(err.c_str());
            }
        }
        
        return program;
    }

    void UpdateImageTexture()
    {
        static const float min = -1.0f;
        static const float max = +2.0f;
        static const float range = max - min;

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


    // The main if the shader program hard-coded to accept the lut 3D smapler as input
    const char * g_fragShaderText = ""
    "\n"
    "uniform sampler2D tex1;\n"
    "uniform sampler3D tex2;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec4 col = texture2D(tex1, gl_TexCoord[0].st);\n"
    "    gl_FragColor = OCIODisplay(col, tex2);\n"
    "}\n";


    void UpdateOCIOGLState(OCIO::ConstProcessorRcPtr& processor)
    {
        // Step 1: Create a GPU Shader Description

        OCIO::GpuShaderDesc shaderDesc;
        shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLSL_1_0);
        shaderDesc.setFunctionName("OCIODisplay");
        shaderDesc.setLut3DEdgeLen(LUT3D_EDGE_SIZE);
        
        // Step 2: Compute the 3D LUT

        std::string lut3dCacheID = processor->getGpuLut3DCacheID(shaderDesc);
        if(lut3dCacheID != g_lut3dcacheid)
        {
            g_lut3dcacheid = lut3dCacheID;
            processor->getGpuLut3D(&g_lut3d[0], shaderDesc);
            
            glBindTexture(GL_TEXTURE_3D, g_lut3dTexID);

            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F_ARB,
                            LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE,
                            0, GL_RGB,GL_FLOAT, &g_lut3d[0]);
        }
        
        // Step 3: Compute the Shader

        std::string shaderCacheID = processor->getGpuShaderTextCacheID(shaderDesc);
        if(g_program == 0 || shaderCacheID != g_shadercacheid)
        {
            g_shadercacheid = shaderCacheID;
            
            std::ostringstream os;
            os << processor->getGpuShaderText(shaderDesc) << "\n";
            os << g_fragShaderText;

            if(g_fragShader) glDeleteShader(g_fragShader);
            g_fragShader = CompileShaderProgram(GL_FRAGMENT_SHADER, os.str().c_str());
            if(g_program) glDeleteProgram(g_program);
            g_program = LinkShaderProgram(g_fragShader);
        }
    }

    // Validate the GPU processing against the CPU one
    void ValidateImageTexture(OCIO::ConstProcessorRcPtr& processor, float epsilon)
    {
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
            if( (std::fabs(cppImage[4*idx+0]-gpuImage[4*idx+0]) > epsilon) ||
                (std::fabs(cppImage[4*idx+1]-gpuImage[4*idx+1]) > epsilon) ||
                (std::fabs(cppImage[4*idx+2]-gpuImage[4*idx+2]) > epsilon) ||
                (std::fabs(cppImage[4*idx+3]-gpuImage[4*idx+3]) > epsilon) )
            {
                std::stringstream err;
                err << std::setprecision(10)
                    << "Image[" << idx << "] form orig = {"
                    << g_image[4*idx+0] << ", " << g_image[4*idx+1] << ", "
                    << g_image[4*idx+2] << ", " << g_image[4*idx+3] << "}"
                    << " to cpu = {"
                    << cppImage[4*idx+0] << ", " << cppImage[4*idx+1] << ", "
                    << cppImage[4*idx+2] << ", " << cppImage[4*idx+3] << "}"
                    << " and gpu = {"
                    << gpuImage[4*idx+0] << ", " << gpuImage[4*idx+1] << ", "
                    << gpuImage[4*idx+2] << ", " << gpuImage[4*idx+3] << "}";

                err << "\twith epsilon=" << epsilon;
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

    // Step 2: Allocate the needed textures

    AllocateImageTexture();

    AllocateDefaultLut3D();

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
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_3D, g_lut3dTexID, 0);

    // attach a renderbuffer to depth attachment point
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rboId);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Step 4: Execute all the unit tests

    unsigned failures = 0;

    const UnitTests & tests = GetUnitTests();
    const size_t numTests = tests.size();
    for(size_t idx=0; idx<numTests; ++idx)
    {
        OCIOGPUTest* test = tests[idx];
        test->setup();

        const unsigned curr_failures = failures;
 
        // Set the rendering destination to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        
        // Clear buffer
        glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::cerr << "Test [" << test->group() << "] [" << test->name() << "] - ";

        try
        {
            // Update the image texture
            UpdateImageTexture();

            // Update the GPU shader program
            UpdateOCIOGLState(test->getProcessor());

            // Enable the shader program, and its textures
            glUseProgram(g_program);
            glUniform1i(glGetUniformLocation(g_program, "tex1"), 1);
            glUniform1i(glGetUniformLocation(g_program, "tex2"), 2);

            // Process the image texture into the rendering buffer
            Reshape();
            Redisplay();

            // Validate the processed image using the rendering buffer
            ValidateImageTexture(test->getProcessor(), test->getErrorThreshold());
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

        if(curr_failures==failures)
        {
            std::cerr << "PASSED" << std::endl;
        }

        glUseProgram(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    std::cerr << std::endl << failures << " tests failed" << std::endl << std::endl;
}
