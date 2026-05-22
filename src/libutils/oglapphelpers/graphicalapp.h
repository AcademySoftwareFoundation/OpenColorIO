// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#pragma once

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{
// Forward declaration of GraphicalApp.
class GraphicalApp;
typedef OCIO_SHARED_PTR<GraphicalApp> GraphicalAppRcPtr;

class GraphicalApp
{
public:
    virtual ~GraphicalApp() = default;
    enum Components
    {
        COMPONENTS_RGB = 0,
        COMPONENTS_RGBA
    };

    // Shader code will be printed when generated.
    void setShaderVerbose(bool print)
    {
        m_verboseShader = print;
    }

    bool isShaderVerbose() const { return m_verboseShader; }

    // When displaying the processed image in a window, enable Y-axis mirroring.
    void setYMirror()
    {
        m_yMirror = true;
    }

    bool isYMirror() const { return m_yMirror; }

    // Initialize the image.
    virtual void initImage(int imageWidth, int imageHeight,
        Components comp, const float* imageBuffer) = 0;

    // Update the image if it changes.
    virtual void updateImage(const float* imageBuffer) = 0;

    // Create frame and rendering buffers. Needed if readImage will be used.
    virtual void createBuffers() = 0;

    // Set the shader code.
    virtual void setShader(GpuShaderDescRcPtr& shaderDesc) = 0;

    // Update the size of the buffer of the viewport that will be used to process the image
    // (it does not modify the UI).  To be called at least one time. Use image size if we want to
    // read back the processed image.  To process another image with the same size or using a
    // different shader, reshape does not need to be called again. In case of an interactive
    // application it should be called by the glutReshapeFunc callback using the windows size.
    virtual void reshape(int width, int height) = 0;

    // Process the image.
    virtual void redisplay() = 0;

    // Read the image from the rendering buffer. It is not meant to be used by interactive
    // applications used to display the image.
    virtual void readImage(float* imageBuffer) = 0;

    // Helper to print graphics info.
    virtual void printGraphicsInfo() const noexcept = 0;

    // Factory: returns a platform-appropriate GraphicalApp (OGL or DX).
    static GraphicalAppRcPtr CreateApp(const char * winTitle, int winWidth, int winHeight);

private:
    // Will shader code be outputed when setShader is called.
    bool m_verboseShader{ false };
    // For interactive applications displaying the processed image.
    bool m_yMirror{ false };
};

} // namespace OCIO_NAMESPACE

