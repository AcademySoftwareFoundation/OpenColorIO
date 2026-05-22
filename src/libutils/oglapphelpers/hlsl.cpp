// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <iostream>
#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "hlsl.h"
#include "dxutils.h"

namespace OCIO_NAMESPACE
{

namespace
{

// Upload a TEXTURE_1D LUT as a real D3D12 1D texture with RGBA32F format.
// RGBA32F is used instead of RGB32F because DXGI_FORMAT_R32G32B32_FLOAT has
// limited hardware support in D3D12 (optional for most resource types).
void AllocateTexture1D(ID3D12Device* device,
                       ID3D12GraphicsCommandList* commandList,
                       UINT width,
                       UINT numChannels,
                       const float* values,
                       Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
                       Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    if (!values)
    {
        throw Exception("Missing 1D LUT texture data");
    }

    // Create a 1D texture resource in default heap.
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    textureDesc.Width = width;
    textureDesc.Height = 1;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture)));

    // Create upload buffer with row-pitch alignment
    const UINT dstPixelSize = 4 * sizeof(float);  // RGBA32F
    const UINT rowPitch = AlignRowPitch(width, dstPixelSize);
    const UINT uploadBufferSize = rowPitch;  // Only 1 row

    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)));

    // Map and expand source data (numChannels floats/pixel) to RGBA32F (4 floats/pixel)
    BYTE* pData = nullptr;
    ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
    float* dst = reinterpret_cast<float*>(pData);
    for (UINT i = 0; i < width; ++i)
    {
        dst[i * 4 + 0] = (numChannels > 0) ? values[i * numChannels + 0] : 0.0f;
        dst[i * 4 + 1] = (numChannels > 1) ? values[i * numChannels + 1] : 0.0f;
        dst[i * 4 + 2] = (numChannels > 2) ? values[i * numChannels + 2] : 0.0f;
        dst[i * 4 + 3] = 1.0f;
    }
    uploadBuffer->Unmap(0, nullptr);

    // Copy from upload buffer to texture
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    footprint.Footprint.Width = width;
    footprint.Footprint.Height = 1;
    footprint.Footprint.Depth = 1;
    footprint.Footprint.RowPitch = rowPitch;

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = uploadBuffer.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = footprint;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = texture.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    // Transition to shader resource state
    auto barrier1D = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier1D);
}

// Upload a TEXTURE_2D (folded) 1D LUT as a 2D texture of width x height with RGBA32F format.
void AllocateTexture2D( ID3D12Device* device,
                        ID3D12GraphicsCommandList* commandList,
                        UINT width,
                        UINT height,
                        UINT numChannels,
                        const float* values,
                        Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
                        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    if (!values)
    {
        throw Exception("Missing 2D LUT texture data");
    }

    // Create a 2D texture resource in default heap.
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps2D(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProps2D,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture)));

    // Create upload buffer: each row is padded to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
    const UINT dstPixelSize = 4 * sizeof(float);  // RGBA32F
    const UINT rowPitch = AlignRowPitch(width, dstPixelSize);
    const UINT uploadBufferSize = rowPitch * height;

    CD3DX12_HEAP_PROPERTIES uploadHeapProps2D(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc2D = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProps2D,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc2D,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)));

    // Map and expand all rows: source is row-major (width * numChannels floats per row)
    BYTE* pData = nullptr;
    ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
    for (UINT y = 0; y < height; ++y)
    {
        const float* srcRow = values + y * width * numChannels;
        float* dstRow = reinterpret_cast<float*>(pData + y * rowPitch);
        for (UINT x = 0; x < width; ++x)
        {
            dstRow[x * 4 + 0] = (numChannels > 0) ? srcRow[x * numChannels + 0] : 0.0f;
            dstRow[x * 4 + 1] = (numChannels > 1) ? srcRow[x * numChannels + 1] : 0.0f;
            dstRow[x * 4 + 2] = (numChannels > 2) ? srcRow[x * numChannels + 2] : 0.0f;
            dstRow[x * 4 + 3] = 1.0f;
        }
    }
    uploadBuffer->Unmap(0, nullptr);

    // Copy from upload buffer to texture
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    footprint.Footprint.Width = width;
    footprint.Footprint.Height = height;
    footprint.Footprint.Depth = 1;
    footprint.Footprint.RowPitch = rowPitch;

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = uploadBuffer.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = footprint;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = texture.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    // Transition to shader resource state
    auto barrier2D = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier2D);
}

// Upload a 3D LUT as a 3D texture with RGBA32F format.
void AllocateTexture3D(ID3D12Device* device,
                       ID3D12GraphicsCommandList* commandList,
                       UINT edgelen,
                       const float* values,
                       Microsoft::WRL::ComPtr<ID3D12Resource>& texture,
                       Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    if (!values)
    {
        throw Exception("Missing 3D LUT texture data");
    }

    // Create 3D texture in default heap
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    textureDesc.Width = edgelen;
    textureDesc.Height = edgelen;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = edgelen;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps3D(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &defaultHeapProps3D,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&texture)));

    // Create upload buffer with row-pitch alignment
    // Source data is always RGB (3 floats/voxel) for OCIO 3D LUTs.
    const UINT srcNumChannels = 3;  // OCIO 3D LUTs always provide RGB data
    const UINT dstPixelSize = 4 * sizeof(float);  // RGBA32F
    const UINT rowPitch = AlignRowPitch(edgelen, dstPixelSize);
    const UINT slicePitch = rowPitch * edgelen;
    const UINT uploadBufferSize = slicePitch * edgelen;

    CD3DX12_HEAP_PROPERTIES uploadHeapProps3D(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc3D = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProps3D,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc3D,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)));

    // Map RGB source data and expand to RGBA32F
    BYTE* pData = nullptr;
    ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));

    for (UINT z = 0; z < edgelen; ++z)
    {
        for (UINT y = 0; y < edgelen; ++y)
        {
            const float* srcRow = values + (z * edgelen * edgelen + y * edgelen) * srcNumChannels;
            float* dstRow = reinterpret_cast<float*>(pData + z * slicePitch + y * rowPitch);
            for (UINT x = 0; x < edgelen; ++x)
            {
                dstRow[x * 4 + 0] = srcRow[x * srcNumChannels + 0];  // R
                dstRow[x * 4 + 1] = srcRow[x * srcNumChannels + 1];  // G
                dstRow[x * 4 + 2] = srcRow[x * srcNumChannels + 2];  // B
                dstRow[x * 4 + 3] = 1.0f;                            // A
            }
        }
    }

    uploadBuffer->Unmap(0, nullptr);

    // Copy from upload buffer to texture
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    footprint.Footprint.Width = edgelen;
    footprint.Footprint.Height = edgelen;
    footprint.Footprint.Depth = edgelen;
    footprint.Footprint.RowPitch = rowPitch;

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = uploadBuffer.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = footprint;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = texture.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    // Transition to shader resource state
    auto barrier3D = CD3DX12_RESOURCE_BARRIER::Transition(
        texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier3D);
}

} // anonymous namespace


//////////////////////////////////////////////////////////

HLSLBuilderRcPtr HLSLBuilder::Create(const GpuShaderDescRcPtr& shaderDesc,
                                     ID3D12Device* device,
                                     ID3D12GraphicsCommandList* commandList,
                                     ID3D12DescriptorHeap* cbvSrvHeap)
{
    if (!shaderDesc)
    {
        throw Exception("HLSLBuilder: shaderDesc is null");
    }
    if (!device)
    {
        throw Exception("HLSLBuilder: device is null");
    }
    if (!commandList)
    {
        throw Exception("HLSLBuilder: commandList is null");
    }
    if (!cbvSrvHeap)
    {
        throw Exception("HLSLBuilder: cbvSrvHeap is null");
    }

    HLSLBuilderRcPtr builder(new HLSLBuilder(shaderDesc, device, commandList, cbvSrvHeap));
    builder->allocateAllTextures();

    // Close the command list; the caller executes and fence-waits.
    ThrowIfFailed(commandList->Close());

    return builder;
}

HLSLBuilder::HLSLBuilder(const GpuShaderDescRcPtr& shaderDesc,
                         ID3D12Device* device,
                         ID3D12GraphicsCommandList* commandList,
                         ID3D12DescriptorHeap* cbvSrvHeap)
    : m_shaderDesc(shaderDesc)
    , m_device(device)
    , m_commandList(commandList)
    , m_cbvSrvHeap(cbvSrvHeap)
    , m_srvDescriptorSize(0)
    , m_verbose(false)
{
    m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

HLSLBuilder::~HLSLBuilder()
{
    deleteAllTextures();
}

void HLSLBuilder::allocateAllTextures()
{
    deleteAllTextures();

    // Process 3D LUTs
    const unsigned maxTexture3D = m_shaderDesc->getNum3DTextures();
    for (unsigned idx = 0; idx < maxTexture3D; ++idx)
    {
        // Get 3D texture information
        const char* textureName = nullptr;
        const char* samplerName = nullptr;
        unsigned edgelen = 0;
        Interpolation interpolation = INTERP_LINEAR;
        m_shaderDesc->get3DTexture(idx, textureName, samplerName, edgelen, interpolation);

        if (!textureName || !*textureName || !samplerName || !*samplerName || edgelen == 0)
        {
            throw Exception("HLSLBuilder: 3D texture data is corrupted");
        }

        const float* values = nullptr;
        m_shaderDesc->get3DTextureValues(idx, values);
        if (!values)
        {
            throw Exception("HLSLBuilder: 3D texture values are missing");
        }

        // Allocate the 3D texture
        TextureResource texRes(textureName, samplerName);
        AllocateTexture3D(m_device, m_commandList, edgelen, values, texRes.m_texture, texRes.m_uploadBuffer);

        // Create SRV at the slot matching the HLSL register assignment
        const UINT heapSlot = m_shaderDesc->get3DTextureShaderBindingIndex(idx);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MipLevels = 1;

        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
            m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(),
            heapSlot,
            m_srvDescriptorSize);

        m_device->CreateShaderResourceView(texRes.m_texture.Get(), &srvDesc, srvHandle);

        m_textures.push_back(texRes);
    }

    // Process 1D LUTs (stored as 2D textures with height=1)
    const unsigned maxTexture1D = m_shaderDesc->getNumTextures();
    for (unsigned idx = 0; idx < maxTexture1D; ++idx)
    {
        // Get 1D texture information
        const char* textureName = nullptr;
        const char* samplerName = nullptr;
        unsigned width = 0;
        unsigned height = 0;
        GpuShaderDesc::TextureType channel = GpuShaderDesc::TEXTURE_RGB_CHANNEL;
        Interpolation interpolation = INTERP_LINEAR;
        GpuShaderDesc::TextureDimensions dimensions = GpuShaderDesc::TEXTURE_1D;
        m_shaderDesc->getTexture(idx, textureName, samplerName, width, height, channel, dimensions, interpolation);

        if (!textureName || !*textureName || !samplerName || !*samplerName || width == 0)
        {
            throw Exception("HLSLBuilder: 1D texture data is corrupted");
        }

        const float* values = nullptr;
        m_shaderDesc->getTextureValues(idx, values);
        if (!values)
        {
            throw Exception("HLSLBuilder: 1D texture values are missing");
        }

        // Determine source channel count: RED=1, RGB=3
        const UINT numChannels = (channel == GpuShaderDesc::TEXTURE_RED_CHANNEL) ? 1u : 3u;
        TextureResource texRes(textureName, samplerName);

        // Create SRV at the slot matching the HLSL register assignment
        const UINT heapSlot = m_shaderDesc->getTextureShaderBindingIndex(idx);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

        if (dimensions == GpuShaderDesc::TEXTURE_1D)
        {
            AllocateTexture1D(m_device, m_commandList, width, numChannels, values,
                              texRes.m_texture, texRes.m_uploadBuffer);
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1D.MipLevels = 1;
        }
        else
        {
            AllocateTexture2D(m_device, m_commandList, width, height, numChannels, values,
                                 texRes.m_texture, texRes.m_uploadBuffer);
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
            m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(),
            heapSlot,
            m_srvDescriptorSize);

        m_device->CreateShaderResourceView(texRes.m_texture.Get(), &srvDesc, srvHandle);

        m_textures.push_back(texRes);
    }
}

void HLSLBuilder::deleteAllTextures()
{
    m_textures.clear();
}

std::string HLSLBuilder::getShaderText() const
{
    if (!m_shaderDesc)
    {
        return "";
    }

    const char* shaderText = m_shaderDesc->getShaderText();

    if (m_verbose && shaderText && *shaderText)
    {
        std::cout << "\nOCIO HLSL Shader:\n\n" << shaderText << std::endl;
    }

    return shaderText ? std::string(shaderText) : std::string();
}

UINT HLSLBuilder::getNumSRVs() const
{
    return static_cast<UINT>(m_textures.size());
}

} // namespace OCIO_NAMESPACE
