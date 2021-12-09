// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_MSL_H
#define INCLUDED_OCIO_MSL_H

#include <vector>

#include <OpenColorIO/OpenColorIO.h>
#import  <Metal/Metal.h>

namespace OCIO_NAMESPACE
{

class MetalBuilder;
typedef OCIO_SHARED_PTR<MetalBuilder> MetalBuilderRcPtr;

void RGB_to_RGBA(const float* lutValues, int valueCount, std::vector<float>& float4AdaptedLutValues);

// This is a reference implementation showing how to do the texture upload & allocation,
// and the program compilation for the GLSL shader language.

class MetalBuilder
{
    struct TextureId
    {
        std::string         m_textureName;
        id<MTLTexture>      m_texture;
        std::string         m_samplerName;
        id<MTLSamplerState> m_samplerState;
        MTLTextureType      m_type;

        TextureId(id<MTLTexture>      tex,
                  const std::string & textureName,
                  id<MTLSamplerState> samplerState,
                  const std::string & samplerName,
                  MTLTextureType type)
            :   m_textureName(textureName)
            ,   m_texture(tex)
            ,   m_samplerName(samplerName)
            ,   m_samplerState(samplerState)
            ,   m_type(type)
        {}
        
        ~TextureId()
        {
            m_texture = nil;
            m_samplerState = nil;
        }
        
        void release()
        {
            if(m_texture)
            {
                [m_texture      release];
            }
            m_texture      = nil;
            
            if(m_samplerState)
            {
                [m_samplerState      release];
            }
            m_samplerState = nil;
        }
    };

    typedef std::vector<TextureId> TextureIds;

    // Uniform are used for dynamic parameters.
    class Uniform
    {
    public:
        Uniform(const std::string & name, const GpuShaderDesc::UniformData & data);

    private:
        Uniform() = delete;
        std::string m_name;
        GpuShaderDesc::UniformData m_data;
    };
    typedef std::vector<Uniform> Uniforms;

public:
    // Create an MSL builder using the GPU shader information from a specific processor
    static MetalBuilderRcPtr Create(const GpuShaderDescRcPtr & gpuShader);

    ~MetalBuilder();

    inline void setVerbose(bool verbose) { m_verbose = verbose; }
    inline bool isVerbose() const { return m_verbose; }

    // Allocate & upload all the needed textures
    //  (i.e. the index is the first available index for any kind of textures).
    void allocateAllTextures(unsigned startIndex);

    // Update all uniforms.
    void setUniforms(id<MTLRenderCommandEncoder> renderCmdEncoder);

    bool     buildPipelineStateObject(const std::string & clientShaderProgram);
    void     applyColorCorrection(id<MTLTexture> inputTexturePtr, id<MTLTexture> outputTexturePtr, unsigned int outWidth, unsigned int outHeight);

    // Determine the maximum width value of a texture
    // depending of the graphic card and its driver.
    static unsigned GetTextureMaxWidth();
    
    id<MTLDevice> getMetalDevice() { return m_device; }

protected:
    MetalBuilder(const GpuShaderDescRcPtr & gpuShader);
    
    // Metal Capturing functions -- used for debugging
    void triggerProgrammaticCaptureScope();
    void stopProgrammaticCaptureScope();

    void deleteAllTextures();

    // Critical for declaring primitive data types like float2, float3, ...
    std::string getMSLHeader();

private:
    MetalBuilder();
    MetalBuilder(const MetalBuilder &) = delete;
    MetalBuilder& operator=(const MetalBuilder &) = delete;
    
    void fillUniformBufferData();

    const GpuShaderDescRcPtr m_shaderDesc; // Description of the fragment shader to create
    
    id<MTLDevice>              m_device;
    id<MTLCommandQueue>        m_cmdQueue;
    id<MTLLibrary>             m_library;
    id<MTLRenderPipelineState> m_PSO;
    
    unsigned m_startIndex;                 // Starting index for texture allocations
    TextureIds m_textureIds;               // Texture ids of all needed textures
    std::vector<uint8_t> m_uniformData;    // Uniform buffer Data
    std::string m_shaderCacheID;           // Current shader program key
    bool m_verbose;                        // Print shader code to std::cout for debugging purposes
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_MSL_H
