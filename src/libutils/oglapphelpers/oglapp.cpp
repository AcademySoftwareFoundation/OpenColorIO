// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <iostream>
#include <sstream>
#include <utility>

#ifdef __APPLE__

#include <OpenGL/gl.h>
#include <GLUT/glut.h>

#elif _WIN32

#include <GL/glew.h>
#include <GL/glut.h>

#else

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>

#endif


#include <OpenColorIO/OpenColorIO.h>

#include "oglapp.h"


namespace OCIO_NAMESPACE
{

OglApp::OglApp(int winWidth, int winHeight)
    : m_viewportWidth(winWidth)
    , m_viewportHeight(winHeight)
{}

OglApp::~OglApp()
{
    m_oglBuilder.reset();
}

void OglApp::initImage(int imgWidth, int imgHeight, Components comp, const float * image)
{
    m_imageWidth = imgWidth;
    m_imageHeight = imgHeight;
    m_components = comp;
    if (m_imageHeight != 0)
    {
        m_imageAspect = (float)m_imageWidth / (float)m_imageHeight;
    }

    glGenTextures(1, &m_imageTexID);
    glActiveTexture(GL_TEXTURE0);
    updateImage(image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void OglApp::updateImage(const float * image)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_imageTexID);

    const GLenum format = m_components == COMPONENTS_RGB ? GL_RGB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, m_imageWidth, m_imageHeight, 0,
                 format, GL_FLOAT, &image[0]);
}

void OglApp::redisplay()
{
    // The window size (for the UI) may not equal the image size (size of the image being
    // processed).  The goal here is to use OpenGL to resize the image to have the largest
    // size possible that will fit in the window size without any cropping.  This may result
    // in either letter or pillar boxing of the displayed image. If you intend to read back
    // the image, the reshape method should be called to update the window size to match the
    // image size.

    float viewportAspect = 1.0f;
    if (m_viewportHeight != 0)
    {
        viewportAspect = (float)m_viewportWidth / (float)m_viewportHeight;
    }

    float pts[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // x0,y0,x1,y1
    if (viewportAspect >= m_imageAspect)
    {
        float imgWidthScreenSpace = m_imageAspect * (float)m_viewportHeight;
        pts[0] = (float)m_viewportWidth * 0.5f - (float)imgWidthScreenSpace * 0.5f;
        pts[2] = (float)m_viewportWidth * 0.5f + (float)imgWidthScreenSpace * 0.5f;
        pts[1] = 0.0f;
        pts[3] = (float)m_viewportHeight;
    }
    else
    {
        float imgHeightScreenSpace = (float)m_viewportWidth / m_imageAspect;
        pts[0] = 0.0f;
        pts[2] = (float)m_viewportWidth;
        pts[1] = (float)m_viewportHeight * 0.5f - imgHeightScreenSpace * 0.5f;
        pts[3] = (float)m_viewportHeight * 0.5f + imgHeightScreenSpace * 0.5f;
    }

    if (m_yMirror)
    {
        std::swap(pts[1], pts[3]);
    }

    // Update the uniform values in case one changed.
    if (m_oglBuilder)
    {
        m_oglBuilder->useAllUniforms();
    }

    glEnable(GL_TEXTURE_2D);
        glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColor3f(1, 1, 1);

        glPushMatrix();
            glBegin(GL_QUADS);
                glTexCoord2f(0.0f, 1.0f);
                glVertex2f(pts[0], pts[3]);

                glTexCoord2f(0.0f, 0.0f);
                glVertex2f(pts[0], pts[1]);

                glTexCoord2f(1.0f, 0.0f);
                glVertex2f(pts[2], pts[1]);

                glTexCoord2f(1.0f, 1.0f);
                glVertex2f(pts[2], pts[3]);
            glEnd();
        glPopMatrix();

    glDisable(GL_TEXTURE_2D);

}

void OglApp::reshape(int width, int height)
{
    m_viewportWidth = width;
    m_viewportHeight = height;

    glViewport(0, 0, m_viewportWidth, m_viewportHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, m_viewportWidth, 0.0, m_viewportHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void OglApp::createGLBuffers()
{
    // Create a framebuffer object, you need to delete them when program exits.
    GLuint fboId;

    glGenFramebuffers(1, &fboId);
    // Set the rendering destination to an FBO.
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);

    GLuint rboId;

    // Create a renderbuffer object to store depth info.
    glGenRenderbuffers(1, &rboId);
    glBindRenderbuffer(GL_RENDERBUFFER, rboId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA32F_ARB, m_imageWidth, m_imageHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Attach a texture to FBO color attachment point.
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_imageTexID, 0);

    // Attach a renderbuffer to depth attachment point.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rboId);
}

void OglApp::readImage(float * image)
{
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    const GLenum format = m_components == COMPONENTS_RGB ? GL_RGB : GL_RGBA;
    glReadPixels(0, 0, m_imageWidth, m_imageHeight, format, GL_FLOAT, (GLvoid*)&image[0]);
}

void OglApp::setShader(GpuShaderDescRcPtr & shaderDesc)
{
    // Create oglBuilder using the shaderDesc.
    m_oglBuilder = OpenGLBuilder::Create(shaderDesc);
    m_oglBuilder->setVerbose(m_printShader);

    // Allocate & upload all the LUTs in a dedicated GPU texture.
    // Note: The start index for the texture indices is 1 as one texture
    //       was already created for the input image.
    m_oglBuilder->allocateAllTextures(1);

    std::ostringstream main;
    main << std::endl
         << "uniform sampler2D img;" << std::endl
         << std::endl
         << "void main()" << std::endl
         << "{" << std::endl
         << "    vec4 col = texture2D(img, gl_TexCoord[0].st);" << std::endl
         << "    gl_FragColor = " << shaderDesc->getFunctionName() << "(col);" << std::endl
         << "}" << std::endl;

    // Build the fragment shader program.
    m_oglBuilder->buildProgram(main.str().c_str());

    // Enable the fragment shader program, and all needed resources.
    m_oglBuilder->useProgram();
    // The image texture.
    glUniform1i(glGetUniformLocation(m_oglBuilder->getProgramHandle(), "img"), 0);
    // The LUT textures.
    m_oglBuilder->useAllTextures();
    // Enable uniforms for dynamic properties.
    m_oglBuilder->useAllUniforms();
}

void OglApp::printGLInfo() const noexcept
{
    std::cout << std::endl
              << "GL Vendor:    " << glGetString(GL_VENDOR) << std::endl
              << "GL Renderer:  " << glGetString(GL_RENDERER) << std::endl
              << "GL Version:   " << glGetString(GL_VERSION) << std::endl
              << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}

void OglApp::setupCommon()
{
#ifndef __APPLE__
    glewInit();

    // TO DO: Find out why glewInit() != GLEW_OK

    if (!glewIsSupported("GL_VERSION_2_0"))
    {
        throw Exception("OpenGL 2.0 not supported.");
    }
#endif

    // Initialize the OpenGL engine.

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);           // 4-byte pixel alignment

#ifndef __APPLE__
    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);     //
    glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);   // avoid any kind of clamping
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE); //
#endif

    glEnable(GL_TEXTURE_2D);
}

OglAppRcPtr OglApp::CreateOglApp(const char * winTitle, int winWidth, int winHeight)
{
#ifdef OCIO_HEADLESS_ENABLED
        return std::make_shared<HeadlessApp>(winTitle, winWidth, winHeight);
#else
        return std::make_shared<ScreenApp>(winTitle, winWidth, winHeight);
#endif
}

ScreenApp::ScreenApp(const char * winTitle, int winWidth, int winHeight):
    OglApp(winWidth, winHeight)
{
    int argc = 2;
    const char * argv[] = { winTitle, "-glDebug" };

    glutInit(&argc, const_cast<char**>(&argv[0]));

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(m_viewportWidth, m_viewportHeight);
    glutInitWindowPosition(0, 0);

    m_mainWin = glutCreateWindow(argv[0]);

    setupCommon();
}

ScreenApp::~ScreenApp()
{
    glutDestroyWindow(m_mainWin);
}

void ScreenApp::redisplay()
{
    OglApp::redisplay();
    glutSwapBuffers();
}

void ScreenApp::printGLInfo() const noexcept
{
    OglApp::printGLInfo();
}

#ifdef OCIO_HEADLESS_ENABLED

HeadlessApp::HeadlessApp(const char * winTitle, int bufWidth, int bufHeight)
    : OglApp(bufWidth, bufHeight)
    , m_pixBufferWidth(bufWidth)
    , m_pixBufferHeight(bufHeight)
{
    m_configAttribs =
    {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };

    m_pixBufferAttribs =
    {
        EGL_WIDTH, m_pixBufferWidth,
        EGL_HEIGHT, m_pixBufferHeight,
        EGL_NONE,
    };

    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(m_eglDisplay == EGL_NO_DISPLAY )
    {
        throw Exception("EGL could not be initialized.");
    }

    EGLint eglMajor, eglMinor;
    if(eglInitialize(m_eglDisplay, &eglMajor, &eglMinor) != EGL_TRUE)
    {
        throw Exception("EGL display connection couldn't be started.");
    }

    // Choose an appropriate configuration.
    EGLint numConfigs;
    if(eglChooseConfig(m_eglDisplay, &m_configAttribs[0], &m_eglConfig, 1, &numConfigs) != EGL_TRUE)
    {
        throw Exception("Failed to choose EGL configuration.");
    }

    m_eglSurface = eglCreatePbufferSurface(m_eglDisplay, m_eglConfig, &m_pixBufferAttribs[0]);
    eglBindAPI(EGL_OPENGL_API);

    // Create a context and make it current.
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, NULL);
    if(eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_eglContext) != EGL_TRUE)
    {
        throw Exception("Could not make EGL context current.");
    }

    setupCommon();
}

HeadlessApp::~HeadlessApp()
{
    eglTerminate(m_eglDisplay);
}

void HeadlessApp::printGLInfo() const noexcept
{
    OglApp::printGLInfo();
    printEGLInfo();
}

void HeadlessApp::printEGLInfo() const noexcept
{
    std::cout << std::endl
              << "EGL Vendor:   " << eglQueryString(m_eglDisplay, EGL_VENDOR) << std::endl
              << "EGL Version:  " << eglQueryString(m_eglDisplay, EGL_VERSION) << std::endl;
}

void HeadlessApp::redisplay()
{
    OglApp::redisplay();
    eglSwapBuffers(m_eglDisplay, m_eglSurface);
}

#endif

} // namespace OCIO_NAMESPACE
