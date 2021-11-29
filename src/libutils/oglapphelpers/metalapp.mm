// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <iostream>
#include <sstream>
#include <utility>
#include <fstream>

#import <Metal/Metal.h>
#import <AppKit/AppKit.h>

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

#include "metalapp.h"
#include "msl.h"
#include "glsl.h"
#include "mtltexture.h"

namespace OCIO_NAMESPACE
{

struct GraphicsContext
{
    GraphicsContext() = delete;
    GraphicsContext(NSOpenGLContext* glContext, id<MTLDevice> metalDevice)
    {
        this->glContext = glContext;
        this->metalDevice = metalDevice;
    }
    
    NSOpenGLContext* glContext;
    id<MTLDevice>    metalDevice;
};

MetalApp::MetalApp(const char * winTitle, int winWidth, int winHeight)
    : ScreenApp(winTitle, winWidth, winHeight)
{
    initContext();
}

void MetalApp::initContext()
{
    NSOpenGLContext* currentContext = [NSOpenGLContext currentContext];
    
    m_context = std::unique_ptr<GraphicsContext>(new GraphicsContext(currentContext, MTLCreateSystemDefaultDevice()));
}

MetalApp::~MetalApp()
{
    m_metalBuilder.reset();
}

void MetalApp::initImage(int imgWidth, int imgHeight, Components comp, const float * image)
{
    std::vector<float> imageData;
    if(comp == Components::COMPONENTS_RGB)
    {
        imageData = RGB_to_RGBA(image, 3 * imgWidth * imgHeight);
        image = imageData.data();
    }
    
    setImageDimensions(imgWidth, imgHeight, comp);
    m_image = std::make_shared<MtlTexture>(m_context->metalDevice, imgWidth, imgHeight, image);
    m_outputImage = std::make_shared<MtlTexture>(m_context->metalDevice, m_context->glContext, imgWidth, imgHeight, nullptr);
}

void MetalApp::updateImage(const float *image)
{
    std::vector<float> imageData;
    if(getImageComponents() == Components::COMPONENTS_RGB)
    {
        imageData = RGB_to_RGBA(image, 3 * m_image->getWidth() * m_image->getHeight());
        image = imageData.data();
    }
    m_image->update(image);
}

void MetalApp::setShader(GpuShaderDescRcPtr & shaderDesc)
{
    std::ostringstream main;
    
    m_metalBuilder = MetalBuilder::Create(shaderDesc);
    m_oglBuilder = OpenGLBuilder::Create(shaderDesc);
    
    m_metalBuilder->allocateAllTextures(1);
    
    if(shaderDesc->getLanguage() == GPU_LANGUAGE_MSL_2_0)
    {
        static const char* uniformDataInstanceName = "uniformData";
        std::ostringstream uniformParams;
        std::ostringstream params;
        
        main << "//Metal Shading Language version 2.3\n"
             << "#include <metal_stdlib>\n"
             << "#include <simd/simd.h>\n"
             << "using namespace metal;\n"
             << "\n"
             << "struct VertexOut\n"
             << "{\n"
             << "    float4 position [[ position ]];\n"
             << "    float2 texCoord0;\n"
             << "};\n";
        
        main << "vertex VertexOut ColorCorrectionVS(unsigned int vId [[ vertex_id ]])\n"
             << "{\n"
             << "    VertexOut vOut;\n"
             << "    switch(vId)\n"
             << "    {\n"
             << "    case 0:\n"
             << "        vOut.position  = float4(-1.0f, 3.0f, 1.0f, 1.0f);\n"
             << "        vOut.texCoord0 = float2(0.0f, -1.0f);\n"
             << "        break;\n"
             << "    case 1:\n"
             << "        vOut.position  = float4(3.0f, -1.0f, 1.0f, 1.0f);\n"
             << "        vOut.texCoord0 = float2(2.0f, 1.0f);\n"
             << "        break;\n"
             << "    case 2:\n"
             << "        vOut.position  = float4(-1.0f, -1.0f, 1.0f, 1.0f);\n"
             << "        vOut.texCoord0 = float2(0.0f, 1.0f);\n"
             << "        break;\n"
             << "    };\n"
             << "    return vOut;\n"
             << "}\n\n\n";
        
        main << shaderDesc->getShaderText();
        
        unsigned int uniformCount = shaderDesc->getNumUniforms();
        if(uniformCount > 0)
        {
            std::string separator = "";
            
            main << "struct UniformData\n"
                 << "{\n";
            for(unsigned int i = 0; i < shaderDesc->getNumUniforms(); ++i)
            {
                GpuShaderDesc::UniformData data;
                const char* uniformName = shaderDesc->getUniform(i, data);
                main << "    ";
                switch(data.m_type)
                {
                    case UNIFORM_DOUBLE:
                    case UNIFORM_BOOL:
                        main << "float "  << uniformName << ";\n";
                        break;
                        
                    case UNIFORM_FLOAT3:
                        main << "float3 "  << uniformName << ";\n";
                        break;
                        
                    case UNIFORM_VECTOR_FLOAT:
                        main << "float "  << uniformName << "[" << data.m_vectorFloat.m_getSize() << "];\n";
                        break;
                        
                    case UNIFORM_VECTOR_INT:
                        main << "int "  << uniformName << "[" << data.m_vectorInt.m_getSize() << "];\n";
                        break;
                        
                    case UNIFORM_UNKNOWN:
                        assert(false);
                        break;
                }
                
                uniformParams << separator << uniformDataInstanceName << "." << uniformName;
                separator = ", ";
            }
            main << "};\n";
        }
        
        main << "\n\n\n"
             <<"fragment float4 ColorCorrectionPS(VertexOut in [[stage_in]], texture2d<float> colorIn [[ texture(0) ]]\n";
        
        if(uniformCount > 0)
        {
            main << ",    constant UniformData& " << uniformDataInstanceName << " [[ buffer(0) ]]";
        }
        
        std::string separator = "";
        
        int tex_slot = 1;
        for(unsigned int i = 0; i < shaderDesc->getNum3DTextures(); ++i)
        {
            const char* textureName;
            const char* samplerName;
            unsigned int edgeLen;
            Interpolation interpolation;
            
            shaderDesc->get3DTexture(i, textureName, samplerName, edgeLen, interpolation);
            
            main << ",    texture3d<float> "
                 << textureName
                 << " [[ texture("
                 << std::to_string(tex_slot)
                 << ") ]]\n";
            
            main << ",    sampler "
                 << samplerName
                 << " [[ sampler("
                 << std::to_string(tex_slot++)
                 << ") ]]\n";
            
            params << separator
                   << textureName
                   << ", "
                   << samplerName;
            
            separator = ", ";
        }
        
        for(unsigned int i = 0; i < shaderDesc->getNumTextures(); ++i)
        {
            const char* textureName;
            const char* samplerName;
            unsigned int width, height;
            GpuShaderCreator::TextureType channel;
            Interpolation interpolation;
            
            shaderDesc->getTexture(i, textureName, samplerName, width, height, channel, interpolation);
            
            if(height > 1)
                main << ",    texture2d<float> ";
            else
                main << ",    texture1d<float> ";
            main << textureName
                 << " [[ texture("
                 << std::to_string(tex_slot)
                 << ") ]]\n";
            
            main << ",    sampler "
                 << samplerName
                 << " [[ sampler("
                 << std::to_string(tex_slot++)
                 << ") ]]\n";
            
            params << separator
                   << textureName
                   << ", "
                   << samplerName;
            
            separator = ", ";
        }
        
        if(uniformCount > 0)
            separator = ", ";
        
        main << ")"
                "{\n"
                "    constexpr sampler s = sampler(address::clamp_to_edge);\n"
                "    float4 inPixel = colorIn.sample(s, float2(in.texCoord0.x, in.texCoord0.y));\n"
                "    return "
             <<  shaderDesc->getFunctionName() << "(" << params.str() << uniformParams.str() << separator << "inPixel);\n"
             << "}\n";
        
        main << std::endl;
    }
    else
    {
        assert(false && "Metal renderer can only consume MSL shaders");
    }
    
    // Build the fragment shader program.
    if(m_metalBuilder->buildPipelineStateObject(main.str().c_str()))
    {
        m_metalBuilder->fillUniformBuffer();
        m_metalBuilder->applyColorCorrection(m_image->getMetalTextureHandle(),
                                             m_outputImage->getMetalTextureHandle(),
                                             m_outputImage->getWidth(),
                                             m_outputImage->getHeight());
    }
    
    
    {
        std::ostringstream main;
            
        main <<    std::endl
                << "uniform sampler2DRect img;" << std::endl
                << std::endl
                << "void main()" << std::endl
                << "{" << std::endl
                << "    gl_FragColor = texture2DRect(img, gl_TexCoord[0].st * "
                << "vec2(" << m_outputImage->getWidth() << ", " << m_outputImage->getHeight() << "));"
                << std::endl
                << "}" << std::endl;
        
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_RECTANGLE_EXT, m_outputImage->getGLHandle());
        
        // Build the fragment shader program.
        m_oglBuilder->buildProgram(main.str().c_str(), true);

        // Enable the fragment shader program, and all needed resources.
        m_oglBuilder->useProgram();
        
        // The image texture.
        glUniform1i(glGetUniformLocation(m_oglBuilder->getProgramHandle(), "img"), 0);
    }
}

MetalAppRcPtr MetalApp::CreateMetalGlApp(const char * winTitle, int winWidth, int winHeight)
{
    return std::make_shared<MetalApp>(winTitle, winWidth, winHeight);
}


} // namespace OCIO_NAMESPACE
