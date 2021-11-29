// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_METALAPP_H
#define INCLUDED_OCIO_METALAPP_H


#include <OpenColorIO/OpenColorIO.h>

#include "oglapp.h"

namespace OCIO_NAMESPACE
{

struct GraphicsContext;

class MetalBuilder;
typedef OCIO_SHARED_PTR<MetalBuilder> MetalBuilderRcPtr;

class MetalApp;
typedef OCIO_SHARED_PTR<MetalApp> MetalAppRcPtr;

class MtlTexture;
typedef OCIO_SHARED_PTR<MtlTexture> MtlTextureRcPtr;

class MetalApp : public ScreenApp
{
public:
    MetalApp() = delete;
    MetalApp(const MetalApp &) = delete;
    MetalApp & operator=(const MetalApp &) = delete;

    // Initialize the app with given window name & client rect size.
    MetalApp(const char * winTitle, int winWidth, int winHeight);

    virtual ~MetalApp();

    void initContext();
    
    // Initialize the image.
    void initImage(int imageWidth, int imageHeight,
                   Components comp, const float * imageBuffer) override;
    // Update the image if it changes.
    void updateImage(const float * imageBuffer) override;
    
    void readImage(float * image) override;

    // Set the shader code.
    void setShader(GpuShaderDescRcPtr & shaderDesc) override;

    // Read the image from the rendering buffer. It is not meant to be used by interactive
    // applications used to display the image.
    // void readImage(float * imageBuffer) override;

    // Helper to print GL info.
    //void printGLInfo() const noexcept override;

    // Return a pointer of either ScreenApp or HeadlessApp depending on the
    // OCIO_HEADLESS_ENABLED preprocessor.
    static MetalAppRcPtr CreateMetalGlApp(const char * winTitle, int winWidth, int winHeight);
    
protected:
    MtlTextureRcPtr m_image { nullptr };
    MtlTextureRcPtr m_outputImage { nullptr };
    std::unique_ptr<GraphicsContext> m_context { nullptr };

private:
    MetalBuilderRcPtr m_metalBuilder;
};

}

#endif // INCLUDED_OCIO_METALAPP_H

