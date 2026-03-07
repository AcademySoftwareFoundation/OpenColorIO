// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_HLSL_H
#define INCLUDED_OCIO_HLSL_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <directx/d3dx12.h>

#include <vector>
#include <string>

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

class HLSLBuilder;
typedef OCIO_SHARED_PTR<HLSLBuilder> HLSLBuilderRcPtr;

// This is a DirectX implementation showing how to do texture upload & allocation
// for HLSL shaders, mirroring the role of OpenGLBuilder.

class HLSLBuilder
{
public:
    // Create an HLSL builder using GPU shader information from a specific processor.
    // Allocates D3D12 textures for all LUTs using binding indices from the shaderDesc.
    // All GPU uploads are fence-synchronized before returning.
    static HLSLBuilderRcPtr Create(const GpuShaderDescRcPtr& shaderDesc,
                                   ID3D12Device* device,
                                   ID3D12GraphicsCommandList* commandList,
                                   ID3D12DescriptorHeap* cbvSrvHeap);

    ~HLSLBuilder();

    inline void setVerbose(bool verbose) { m_verbose = verbose; }
    inline bool isVerbose() const { return m_verbose; }

    // Get the OCIO-generated HLSL shader text.
    std::string getShaderText() const;

    // Get the number of SRVs (texture slots) allocated for LUTs.
    // This does not include the image texture at slot 0.
    UINT getNumSRVs() const;

protected:
    HLSLBuilder(const GpuShaderDescRcPtr& shaderDesc,
                ID3D12Device* device,
                ID3D12GraphicsCommandList* commandList,
                ID3D12DescriptorHeap* cbvSrvHeap);

    void allocateAllTextures();
    void deleteAllTextures();

private:
    HLSLBuilder() = delete;
    HLSLBuilder(const HLSLBuilder&) = delete;
    HLSLBuilder& operator=(const HLSLBuilder&) = delete;

    struct TextureResource
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
        std::string m_textureName;
        std::string m_samplerName;

        TextureResource(const std::string& textureName,
                       const std::string& samplerName)
            : m_textureName(textureName)
            , m_samplerName(samplerName)
        {}
    };

    typedef std::vector<TextureResource> TextureResources;

    const GpuShaderDescRcPtr m_shaderDesc;    // Description of the shader
    ID3D12Device* m_device;                   // D3D12 device (not owned)
    ID3D12GraphicsCommandList* m_commandList; // Command list for uploads (not owned)
    ID3D12DescriptorHeap* m_cbvSrvHeap;       // Descriptor heap for SRVs (not owned)
    UINT m_srvDescriptorSize;                 // Size of one SRV descriptor
    TextureResources m_textures;              // All allocated texture resources
    bool m_verbose;                           // Print shader text for debugging
};

} // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_HLSL_H
