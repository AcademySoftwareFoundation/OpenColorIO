// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_OGLAPP_H
#define INCLUDED_OCIO_OGLAPP_H


#include <OpenColorIO/OpenColorIO.h>

#include "glsl.h"

namespace OCIO_NAMESPACE
{

// Here is some sample code to demonstrate how to use this in a simple app that wants to process
// using the GPU and OpenGL.  Processed image is expected to have the same size as the input image.
// For an interactive application, OglApp can be used, but other OGL code is required.
//
// See ociodisplay for an example of an interactive app that displays an image in the UI and
// ocioconvert and ociochecklut for examples of non-interactive apps that just process values with
// the GPU.

/*
// Create and initialize.  Unfortunately, even for non-GUI apps we need to create a window that
// will appear in the UI so here you specify the name of the window and its size.  For non-GUI
// apps you may want to use a small size (it does not need to match the size of the image being
// processed).
OglApp oglApp("Window Name", windowWidth, windowHeight);

float * imageBuffer = GetImageBuffer();
int imageWidth = GetImageWidth();
int imageHeight = GetImageHeight();
oglApp.initImage(imagewidth, imageheight, OglApp::COMPONENTS_RGB, imageBuffer);
oglApp.createGLBuffers();

// Set (or change) shader.
GpuShaderDescRcPtr shader = GpuShaderDesc::CreateShaderDesc();
processor->getDefaultGPUProcessor()->extractGpuShaderInfo(shader);

oglApp.setShader(shader);

// Process the image:
// - Call reshape to make the window size match the size of the image being processed.  (This will
//   not update the size of the window in the UI.).
oglApp.reshape(imageWidth, imageHeight);
// - Call redisplay to apply the shader.
oglApp.redisplay();

// Read the processed image.
std::vector<float> imageBufferOut(3 * imageWidth * imageWidth);
oglApp.readImage(imageBufferOut.data());

*/
class OglApp
{
public:
    OglApp() = delete;

    // Initialize the app with given window name & client rect size.
    OglApp(const char * winTitle, int winWidth, int winHeight);
    ~OglApp();

    // When displaying the processed image in a window this needs to be done.
    // In that case, when image is read, the result will be mirrored on Y.
    void setYMirror()
    {
        m_yMirror = true;
    }

    // Shader code will be printed when generated.
    void setPrintShader(bool print)
    {
        m_printShader = print;
    }

    enum Components
    {
        COMPONENTS_RGB = 0,
        COMPONENTS_RGBA
    };

    // Initialize the image.
    void initImage(int imageWidth, int imageHeight,
                   Components comp, const float * imageBuffer);
    // Update the image if it changes.
    void updateImage(const float * imageBuffer);

    // Create GL frame and rendering buffers. Needed if readImage will be used.
    void createGLBuffers();

    // Set the shader code.
    void setShader(GpuShaderDescRcPtr & shaderDesc);

    // Update the size of the buffer of the OpenGL viewport that will be used to process the image
    // (it does not modify the UI).  To be called at least one time. Use image size if we want to
    // read back the processed image.  To process another image with the same size or using a
    // different shader, reshape does not need to be called again. In case of an interactive
    // application it should be called by the glutReshapeFunc callback using the windows size.
    void reshape(int width, int height);

    // To be called after changing dynamic properties and before calling redisplay.
    void updateUniforms();

    // Process the image.
    void redisplay();

    // Read the image from the rendering buffer. It is not meant to be used by interactive
    // applications used to display the image.
    void readImage(float * imageBuffer);

    // Helper to print GL info.
    void printGLInfo() const noexcept;

private:

    // Window identifier returned by glutCreateWindow.
    int m_mainWin{ 0 };

    // Window or output image size (set using reshape).
    // When the app is used to process an image this should be equal to the image size so that
    // when processed image is read from the viewport it matches the size of the original image.
    // When an interactive app is just displaying an image, this should equal the viewport size
    // and the image will be scaled to fit so there is no cropping.
    int m_viewportWidth{ 0 };
    int m_viewportHeight{ 0 };

    // Keep track of the original image ratio.
    float m_imageAspect{ 1.0f };

    // For interactive application displaying the processed image, this needs to be true.
    bool m_yMirror{ false };

    // Will shader code be outputed when setShader is called.
    bool m_printShader{ false };

    // Image information.
    int m_imageWidth{ 0 };
    int m_imageHeight{ 0 };
    Components m_components{ COMPONENTS_RGBA };
    unsigned int m_imageTexID;

    OpenGLBuilderRcPtr m_oglBuilder;

};

typedef OCIO_SHARED_PTR<OglApp> OglAppRcPtr;

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_OGLAPP_H
