// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <iostream>
#include <sstream>
#include <utility>
#include <fstream>

#import <Metal/Metal.h>
#import <AppKit/AppKit.h>

#include <OpenGL/gl.h>
#include <GLUT/glut.h>

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
    ~GraphicsContext()
    {
        if(glContext)
            [glContext release];
        glContext = nil;
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
    m_glStateBound = false;
}

void MetalApp::initImage(int imgWidth, int imgHeight, Components comp, const float * image)
{
    std::vector<float> imageData;
    if(comp == Components::COMPONENTS_RGB)
    {
        RGB_to_RGBA(image, 3 * imgWidth * imgHeight, imageData);
        image = imageData.data();
    }
    
    setImageDimensions(imgWidth, imgHeight, comp);
    m_image = std::make_shared<MtlTexture>(m_context->metalDevice, imgWidth, imgHeight, image);
    m_outputImage = std::make_shared<MtlTexture>(m_context->metalDevice, m_context->glContext, imgWidth, imgHeight, image);
    m_glStateBound = false;
}

void MetalApp::updateImage(const float *image)
{
    std::vector<float> imageData;
    if(getImageComponents() == Components::COMPONENTS_RGB)
    {
        RGB_to_RGBA(image, 3 * m_image->getWidth() * m_image->getHeight(), imageData);
        image = imageData.data();
    }
    m_image->update(image);
}

void MetalApp::prepareAndBindOpenGLState()
{
    // A dummyShaderDesc is enough.
    // The builder will only be used to build GL program
    GpuShaderDescRcPtr dummyShaderDesc = GpuShaderDesc::CreateShaderDesc();
    m_oglBuilder = OpenGLBuilder::Create(dummyShaderDesc);
    
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
    
    m_glStateBound = true;
}

void MetalApp::setShader(GpuShaderDescRcPtr & shaderDesc)
{
    std::ostringstream main;
    
    m_metalBuilder = MetalBuilder::Create(shaderDesc);
    m_metalBuilder->allocateAllTextures(1);
    
    if(shaderDesc->getLanguage() == GPU_LANGUAGE_MSL_2_0)
    {
        std::ostringstream uniformParams;
        std::ostringstream params;
        
        static constexpr char shaderheader[] = { R"(//Metal Shading Language version 2.0
#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

struct VertexOut
{
    float4 position [[ position ]];
    float2 texCoord0;
};
        
vertex VertexOut ColorCorrectionVS(unsigned int vId [[ vertex_id ]])
{
    VertexOut vOut;
    switch(vId)
    {
    case 0:
        vOut.position  = float4(-1.0f, 3.0f, 1.0f, 1.0f);
        vOut.texCoord0 = float2(0.0f, -1.0f);
        break;
    case 1:
        vOut.position  = float4(3.0f, -1.0f, 1.0f, 1.0f);
        vOut.texCoord0 = float2(2.0f, 1.0f);
        break;
    case 2:
        vOut.position  = float4(-1.0f, -1.0f, 1.0f, 1.0f);
        vOut.texCoord0 = float2(0.0f, 1.0f);
        break;
    };
    return vOut;
})" };
        
        main << shaderheader;
        main << shaderDesc->getShaderText();
    
        const std::string uniformDataStructName = "UniformData";
        const std::string uniformDataInstanceName = "uniformData";
        
        auto uniformLenVariableName = [](std::string varName) -> std::string
        {
            return varName + "_count";
        };
        
        auto uniformDataAccess = [&uniformDataInstanceName](std::string memberName) -> std::string
        {
            return uniformDataInstanceName + "." + memberName;
        };
    
        std::ostringstream uniformData;
        std::ostringstream uniformBufferBindings;
        
        const std::string tab = "    ";
        std::string separator = "";
        unsigned int uniformCount = shaderDesc->getNumUniforms();
        unsigned int uniformIdx = 1;
        for(unsigned int i = 0; i < uniformCount; ++i)
        {
            GpuShaderDesc::UniformData data;
            const char* uniformName = shaderDesc->getUniform(i, data);
            std::string spaceSpecifier = ",    constant ";
            switch(data.m_type)
            {
                case UNIFORM_DOUBLE:
                    uniformData << tab << "float "  << uniformName << ";\n";
                    uniformParams << separator << uniformDataAccess(uniformName);
                    break;
                    
                case UNIFORM_BOOL:
                    uniformData << tab << "int "  << uniformName << ";\n";
                    uniformParams << separator << uniformDataAccess(uniformName);
                    break;
                    
                case UNIFORM_FLOAT3:
                    uniformData << tab << "float3 "  << uniformName << ";\n";
                    uniformParams << separator << uniformDataAccess(uniformName);
                    break;
                    
                case UNIFORM_VECTOR_FLOAT:
                    uniformBufferBindings << spaceSpecifier << "float* "  << uniformName << "[[ buffer(" << std::to_string(uniformIdx++)  << ") ]]\n";
                    uniformData << tab << "int "  << uniformLenVariableName(uniformName) << ";\n";
                    uniformParams << separator << uniformName << ", " << uniformDataAccess(uniformLenVariableName(uniformName));
                    break;
                    
                case UNIFORM_VECTOR_INT:
                    uniformBufferBindings << spaceSpecifier << "int* "  << uniformName << "[[ buffer(" << std::to_string(uniformIdx++)  << ") ]]\n";
                    uniformData << tab << "int "  << uniformLenVariableName(uniformName) << ";\n";
                    uniformParams << separator << uniformName << ", " << uniformDataAccess(uniformLenVariableName(uniformName));
                    break;
                    
                case UNIFORM_UNKNOWN:
                    throw Exception("Unknown Uniform type.");
                    break;
            }
            
            separator = ", ";
        }
        
        bool needsUniformData = false;
        if(uniformData.str().size() > 0)
        {
            main << "\nstruct " << uniformDataStructName << "\n"
                 << "{\n"
                 << uniformData.str()
                << "};\n";
            
            needsUniformData = true;
        }
        
        main << "\n\n\n"
             <<"fragment float4 ColorCorrectionPS(VertexOut in [[stage_in]], texture2d<float> colorIn [[ texture(0) ]]\n";
        
        if(needsUniformData)
        {
            main << ",    constant " << uniformDataStructName << "& " << uniformDataInstanceName
                 << " [[ buffer(0) ]]\n";
        }
        
        if(uniformBufferBindings.str().size() > 0)
        {
            main << uniformBufferBindings.str();
        }
        
        separator = "";
        
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
            GpuShaderCreator::TextureDimensions dimensions;
            Interpolation interpolation;
            
            shaderDesc->getTexture(i, textureName, samplerName, width, height, channel, dimensions, interpolation);
            
            if(dimensions == GpuShaderDesc::TEXTURE_2D)
            {
                main << ",    texture2d<float> ";
            }
            else
            {
                main << ",    texture1d<float> ";
            }
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
        
        if(uniformParams.str().size() > 0 || params.str().size() > 0)
        {
            separator = ", ";
        }
        
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
        throw Exception("Metal renderer can only consume MSL shaders");
    }
    
    if(printShader())
    {
        std::cout << std::endl;
        std::cout << "GPU Shader Program:" << std::endl;
        std::cout << std::endl;
        std::cout << main.str() << std::endl;
        std::cout << std::endl;
    }
    
    // Build the fragment shader program.
    if(m_metalBuilder->buildPipelineStateObject(main.str().c_str()))
    {
        m_metalBuilder->applyColorCorrection(m_image->getMetalTextureHandle(),
                                             m_outputImage->getMetalTextureHandle(),
                                             m_outputImage->getWidth(),
                                             m_outputImage->getHeight());
    }
}

void MetalApp::redisplay()
{
    if(m_metalBuilder)
    {
        m_metalBuilder->applyColorCorrection(m_image->getMetalTextureHandle(),
                                            m_outputImage->getMetalTextureHandle(),
                                            m_outputImage->getWidth(),
                                            m_outputImage->getHeight());
    }
    
    if(!m_glStateBound)
    {
        prepareAndBindOpenGLState();
    }
    
    ScreenApp::redisplay();
}

MetalAppRcPtr MetalApp::CreateMetalGlApp(const char * winTitle, int winWidth, int winHeight)
{
    return std::make_shared<MetalApp>(winTitle, winWidth, winHeight);
}


} // namespace OCIO_NAMESPACE
