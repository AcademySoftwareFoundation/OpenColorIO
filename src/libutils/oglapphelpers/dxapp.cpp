// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <iostream>
#include <sstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "dxapp.h"
#include "dxutils.h"

#include <dxcapi.h>

namespace OCIO_NAMESPACE
{
    namespace
    {
        LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            switch (message)
            {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            }

            return DefWindowProc(hWnd, message, wParam, lParam);
        }

        void GetHardwareAdapter(
            IDXGIFactory1* pFactory,
            IDXGIAdapter1** ppAdapter,
            bool requestHighPerformanceAdapter = true)
        {
            *ppAdapter = nullptr;

            ComPtr<IDXGIAdapter1> adapter;

            ComPtr<IDXGIFactory6> factory6;
            if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
            {
                for (
                    UINT adapterIndex = 0;
                    SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                        adapterIndex,
                        requestHighPerformanceAdapter ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                        IID_PPV_ARGS(&adapter)));
                        ++adapterIndex)
                {
                    DXGI_ADAPTER_DESC1 desc;
                    adapter->GetDesc1(&desc);

                    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                    {
                        continue;
                    }

                    if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                    {
                        break;
                    }
                }
            }

            if (adapter.Get() == nullptr)
            {
                for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
                {
                    DXGI_ADAPTER_DESC1 desc;
                    adapter->GetDesc1(&desc);

                    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                    {
                        continue;
                    }

                    if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                    {
                        break;
                    }
                }
            }

            *ppAdapter = adapter.Detach();
        }
    }

DxApp::DxApp(const char* winTitle, int winWidth, int winHeight)
    : m_viewportWidth{ winWidth }
    , m_viewportHeight{ winHeight }
{
    // Initialize the window class.
    WNDCLASSEXA windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEXA);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = NULL;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = winTitle;
    RegisterClassExA(&windowClass);
    m_windowClassName = winTitle;

    RECT windowRect = { 0, 0, static_cast<LONG>(m_viewportWidth), static_cast<LONG>(m_viewportHeight) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    m_hwnd = CreateWindowA(
        windowClass.lpszClassName,
        winTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,        // We have no parent window.
        NULL,        // We aren't using menus.
        NULL,
        NULL);

    ShowWindow(m_hwnd, SW_RESTORE);

    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(factory.Get(), &hardwareAdapter);

    ThrowIfFailed(D3D12CreateDevice(
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0, // Standard minimum for D3D12, maximize compatibility
        IID_PPV_ARGS(&m_device)
    ));

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_viewportWidth;
    swapChainDesc.Height = m_viewportHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(m_commandList->Close());

    // Create fence
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 1;
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
    {
        throw Exception("DxApp: CreateEvent failed.");
    }
}

DxApp::~DxApp()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    waitForPreviousFrame();

    CloseHandle(m_fenceEvent);

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
    }
    if (!m_windowClassName.empty())
    {
        UnregisterClassA(m_windowClassName.c_str(), NULL);
    }
}

void DxApp::initImage(int imageWidth, int imageHeight, Components comp, const float* imageBuffer)
{
    if (comp != COMPONENTS_RGBA)
    {
        throw Exception("DxApp: COMPONENTS_RGB is unused and not currently implemented.");
    }

    m_imageWidth = imageWidth;
    m_imageHeight = imageHeight;
    m_comp = comp;

    // Create CBV/SRV heap if not already created (room for image + LUT textures)
    if (!m_cbvSrvHeap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 16;  // Slot 0 for image, remaining for LUT textures
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));
    }

    // Create the image texture in default heap
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    textureDesc.Width = m_imageWidth;
    textureDesc.Height = m_imageHeight;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_imageTexture)));

    // Create the upload buffer with row-pitch aligned size
    const UINT pixelSize = 4 * sizeof(float);  // RGBA32F
    const UINT rowPitch = AlignRowPitch(m_imageWidth, pixelSize);
    const UINT64 uploadBufferSize = rowPitch * m_imageHeight;

    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_imageUploadBuffer)));

    // Perform the initial upload
    // Map the upload buffer and copy image data
    const UINT srcRowPitch = m_imageWidth * pixelSize;  // Source data is tightly packed
    BYTE* pData = nullptr;
    ThrowIfFailed(m_imageUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));

    for (int y = 0; y < m_imageHeight; ++y)
    {
        const BYTE* srcRow = reinterpret_cast<const BYTE*>(imageBuffer) + y * srcRowPitch;
        BYTE* dstRow = pData + y * rowPitch;
        memcpy(dstRow, srcRow, srcRowPitch);
    }

    m_imageUploadBuffer->Unmap(0, nullptr);

    // Record commands to copy from upload buffer to the image texture
    ThrowIfFailed(m_commandAllocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Offset = 0;
    footprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    footprint.Footprint.Width = m_imageWidth;
    footprint.Footprint.Height = m_imageHeight;
    footprint.Footprint.Depth = 1;
    footprint.Footprint.RowPitch = rowPitch;

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = m_imageUploadBuffer.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = footprint;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = m_imageTexture.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    m_commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    // Transition to PIXEL_SHADER_RESOURCE
    auto barrierInit = CD3DX12_RESOURCE_BARRIER::Transition(
        m_imageTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_commandList->ResourceBarrier(1, &barrierInit);

    ThrowIfFailed(m_commandList->Close());

    // Execute the command list
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Wait for GPU to finish
    waitForPreviousFrame();

    // Create SRV for the image texture in slot 0
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
    m_device->CreateShaderResourceView(m_imageTexture.Get(), &srvDesc, srvHandle);
}

void DxApp::updateImage(const float* imageBuffer)
{
    // Map the upload buffer and copy image data row-by-row with proper pitch alignment
    const UINT pixelSize = 4 * sizeof(float);  // RGBA32F
    const UINT srcRowPitch = m_imageWidth * pixelSize;
    const UINT dstRowPitch = AlignRowPitch(m_imageWidth, pixelSize);

    BYTE* pData = nullptr;
    ThrowIfFailed(m_imageUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));

    for (int y = 0; y < m_imageHeight; ++y)
    {
        const BYTE* srcRow = reinterpret_cast<const BYTE*>(imageBuffer) + y * srcRowPitch;
        BYTE* dstRow = pData + y * dstRowPitch;
        memcpy(dstRow, srcRow, srcRowPitch);
    }

    m_imageUploadBuffer->Unmap(0, nullptr);

    // Record commands to copy from upload buffer to the image texture
    ThrowIfFailed(m_commandAllocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    // Transition texture from PIXEL_SHADER_RESOURCE to COPY_DEST for the update
    auto barrierUpdate1 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_imageTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    m_commandList->ResourceBarrier(1, &barrierUpdate1);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Offset = 0;
    footprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    footprint.Footprint.Width = m_imageWidth;
    footprint.Footprint.Height = m_imageHeight;
    footprint.Footprint.Depth = 1;
    footprint.Footprint.RowPitch = dstRowPitch;

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = m_imageUploadBuffer.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = footprint;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = m_imageTexture.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    m_commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    // Transition to PIXEL_SHADER_RESOURCE
    auto barrierUpdate2 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_imageTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_commandList->ResourceBarrier(1, &barrierUpdate2);

    ThrowIfFailed(m_commandList->Close());

    // Execute the command list
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Wait for GPU to finish
    waitForPreviousFrame();
}

void DxApp::createBuffers()
{
    // Create readback buffer for copying GPU results back to CPU
    const UINT pixelSize = 4 * sizeof(float);  // RGBA32F
    m_readbackRowPitch = AlignRowPitch(m_viewportWidth, pixelSize);
    const UINT64 readbackBufferSize = m_readbackRowPitch * m_viewportHeight;

    CD3DX12_HEAP_PROPERTIES readbackHeapProps(D3D12_HEAP_TYPE_READBACK);
    CD3DX12_RESOURCE_DESC readbackBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(readbackBufferSize);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &readbackHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &readbackBufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_readbackBuffer)));
}

void DxApp::setShader(GpuShaderDescRcPtr& shaderDesc)
{
    // Reset command list for LUT texture uploads
    ThrowIfFailed(m_commandAllocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    // Store shader desc so redisplay() can update uniform values each frame.
    m_currentShaderDesc = shaderDesc;

    // Create HLSLBuilder to allocate all LUT textures.
    // Each texture is placed at the descriptor heap slot matching its HLSL register
    // (derived from shaderDesc->get3DTextureShaderBindingIndex / getTextureShaderBindingIndex).
    m_hlslBuilder = HLSLBuilder::Create(shaderDesc, m_device.Get(), m_commandList.Get(),
                                        m_cbvSrvHeap.Get());

    // Execute command list to upload LUT textures and wait for completion
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    waitForPreviousFrame();

    // Create constant buffer for OCIO uniform variables (dynamic properties).
    // D3D12 constant buffers must be 256-byte aligned.  We always create one so the
    // root signature can unconditionally declare a CBV at b0.
    {
        m_constantBuffer.Reset();
        m_cbMappedData = nullptr;

        const UINT rawSize = static_cast<UINT>(shaderDesc->getUniformBufferSize());
        m_cbufferAlignedSize = ((rawSize + 255u) & ~255u);
        if (m_cbufferAlignedSize == 0)
            m_cbufferAlignedSize = 256u;

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(m_cbufferAlignedSize);
        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_constantBuffer)));

        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(m_constantBuffer->Map(0, &readRange,
                                            reinterpret_cast<void**>(&m_cbMappedData)));
        memset(m_cbMappedData, 0, m_cbufferAlignedSize);
    }

    // Get the OCIO HLSL shader text
    std::string ocioShader = m_hlslBuilder->getShaderText();

    // Build the full HLSL shader source with a full-screen triangle vertex shader
    // and pixel shader that applies OCIO color transform
    std::ostringstream hlslSource;

    // Add the OCIO shader functions first
    hlslSource << ocioShader << "\n\n";

    // Declare the image texture and its sampler.
    // OCIO's generated shader always starts LUT texture bindings at t1+ (textureBindingStart=1),
    // so t0/s0 are always free for the input image.
    hlslSource << "Texture2D img : register(t0);\n";
    hlslSource << "SamplerState linearSampler : register(s0);\n\n";

    // Vertex shader: full-screen triangle using SV_VertexID (no vertex buffer needed)
    hlslSource << "struct VSOutput {\n";
    hlslSource << "    float4 position : SV_Position;\n";
    hlslSource << "    float2 texcoord : TEXCOORD0;\n";
    hlslSource << "};\n\n";

    hlslSource << "VSOutput VSMain(uint vertexID : SV_VertexID) {\n";
    hlslSource << "    VSOutput output;\n";
    hlslSource << "    // Full-screen triangle: vertices at (-1,-1), (3,-1), (-1,3)\n";
    hlslSource << "    float2 texcoord = float2((vertexID << 1) & 2, vertexID & 2);\n";
    hlslSource << "    output.position = float4(texcoord * float2(2, -2) + float2(-1, 1), 0, 1);\n";
    hlslSource << "    output.texcoord = texcoord;\n";
    hlslSource << "    return output;\n";
    hlslSource << "}\n\n";

    // Pixel shader: sample image and apply OCIO color transform
    hlslSource << "float4 PSMain(VSOutput input) : SV_Target {\n";
    hlslSource << "    float4 col = img.Sample(linearSampler, input.texcoord);\n";
    hlslSource << "    return " << shaderDesc->getFunctionName() << "(col);\n";
    hlslSource << "}\n";

    std::string fullShader = hlslSource.str();

    if (isShaderVerbose())
    {
        std::cout << std::endl;
        std::cout << "GPU Shader Program:" << std::endl;
        std::cout << std::endl;
        std::cout << fullShader << std::endl;
        std::cout << std::endl;
    }

    // Compile shaders with DXC (DirectX Shader Compiler) for SM 6.0
    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> dxcCompiler;
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler)));

    // Create a source blob from the shader string
    ComPtr<IDxcBlobEncoding> sourceBlob;
    ThrowIfFailed(dxcUtils->CreateBlobFromPinned(
        fullShader.c_str(), static_cast<UINT32>(fullShader.size()),
        DXC_CP_UTF8, &sourceBlob));

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr      = sourceBlob->GetBufferPointer();
    sourceBuffer.Size     = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_ACP;

    // Compile vertex shader (vs_6_0)
    LPCWSTR vsArgs[] = { L"-T", L"vs_6_0", L"-E", L"VSMain" };
    ComPtr<IDxcResult> vsResult;
    ThrowIfFailed(dxcCompiler->Compile(&sourceBuffer, vsArgs, _countof(vsArgs),
                                        nullptr, IID_PPV_ARGS(&vsResult)));
    HRESULT vsHr;
    vsResult->GetStatus(&vsHr);
    if (FAILED(vsHr))
    {
        ComPtr<IDxcBlobUtf8> errors;
        vsResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        std::ostringstream oss;
        oss << "Vertex shader compilation failed (" << HrToString(vsHr) << ")";
        if (errors && errors->GetStringLength())
            oss << ":\n" << errors->GetStringPointer();
        std::cerr << oss.str() << std::endl;
        throw Exception(oss.str().c_str());
    }
    ComPtr<IDxcBlob> vertexShaderBlob;
    ThrowIfFailed(vsResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&vertexShaderBlob), nullptr));

    // Compile pixel shader (ps_6_0).
    LPCWSTR psArgs[] = { L"-T", L"ps_6_0", L"-E", L"PSMain" };
    ComPtr<IDxcResult> psResult;
    ThrowIfFailed(dxcCompiler->Compile(&sourceBuffer, psArgs, _countof(psArgs),
                                        nullptr, IID_PPV_ARGS(&psResult)));
    HRESULT psHr;
    psResult->GetStatus(&psHr);
    if (FAILED(psHr))
    {
        ComPtr<IDxcBlobUtf8> errors;
        psResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        std::ostringstream oss;
        oss << "Pixel shader compilation failed (" << HrToString(psHr) << ")";
        if (errors && errors->GetStringLength())
            oss << ":\n" << errors->GetStringPointer();
        std::cerr << oss.str() << std::endl;
        throw Exception(oss.str().c_str());
    }
    ComPtr<IDxcBlob> pixelShaderBlob;
    ThrowIfFailed(psResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pixelShaderBlob), nullptr));

    // Build root signature with a descriptor table for all SRVs and a static sampler.
    // Total SRVs = 1 (image at t0) + total LUT count (at t1..tN).
    // We use the total texture count from shaderDesc to cover all possible binding slots,
    // since LUT heap slots are derived from binding indices and may not be sequential.
    const UINT numLUTs = shaderDesc->getNumTextures() + shaderDesc->getNum3DTextures();
    UINT totalSRVs = 1 + numLUTs;

    CD3DX12_DESCRIPTOR_RANGE1 srvRange;
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, totalSRVs, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

    // Root parameter 0: descriptor table for all SRVs (image + LUTs).
    // Root parameter 1: root CBV at b0 for OCIO uniform variables.
    CD3DX12_ROOT_PARAMETER1 rootParameters[2];
    rootParameters[0].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[1].InitAsConstantBufferView(0, 0,
        D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE,
        D3D12_SHADER_VISIBILITY_PIXEL);

    // Build static samplers: s0 for the input image, s1..sN for each OCIO LUT texture.
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers(1 + numLUTs);
    for (UINT i = 0; i <= numLUTs; ++i)
    {
        staticSamplers[i] = {};
        staticSamplers[i].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSamplers[i].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[i].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[i].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSamplers[i].MipLODBias = 0;
        staticSamplers[i].MaxAnisotropy = 0;
        staticSamplers[i].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSamplers[i].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        staticSamplers[i].MinLOD = 0.0f;
        staticSamplers[i].MaxLOD = D3D12_FLOAT32_MAX;
        staticSamplers[i].ShaderRegister = i;  // s0 = image, s1..sN = OCIO LUT samplers
        staticSamplers[i].RegisterSpace = 0;
        staticSamplers[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
                                static_cast<UINT>(staticSamplers.size()), staticSamplers.data(),
                                D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> sigErrorBlob;
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1,
                                                        &signature, &sigErrorBlob);
    if (FAILED(hr))
    {
        if (sigErrorBlob)
        {
            std::cerr << "Root signature serialization error:\n"
                      << static_cast<const char*>(sigErrorBlob->GetBufferPointer()) << std::endl;
        }
        ThrowIfFailed(hr);
    }

    // Release previous root signature if it exists
    m_rootSignature.Reset();

    hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(),
                                        signature->GetBufferSize(),
                                        IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
    {
        throw Exception(("CreateRootSignature failed: " + HrToString(hr)).c_str());
    }

    // Create the pipeline state object (PSO)
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0 };  // No vertex input layout (using SV_VertexID)
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
    psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    psoDesc.SampleDesc.Count = 1;

    // Release previous pipeline state if it exists
    m_pipelineState.Reset();

    hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr))
    {
        std::cerr << "CreateGraphicsPipelineState failed (" << HrToString(hr) << ")\n"
                  << "Full HLSL shader:\n" << fullShader << std::endl;
        throw Exception(("CreateGraphicsPipelineState failed: " + HrToString(hr)).c_str());
    }
}

void DxApp::reshape(int width, int height)
{
    // Skip if nothing changed and resources are already initialized
    if (m_viewportWidth == width && m_viewportHeight == height && m_floatRenderTarget)
        return;

    // Wait for any in-flight GPU work before resizing resources
    waitForPreviousFrame();

    m_viewportWidth = width;
    m_viewportHeight = height;

    // Release swap chain render target references before ResizeBuffers
    for (UINT n = 0; n < FrameCount; n++)
        m_renderTargets[n].Reset();

    // Resize swap chain back buffers to the new dimensions
    ThrowIfFailed(m_swapChain->ResizeBuffers(
        FrameCount,
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        DXGI_FORMAT_R8G8B8A8_UNORM,
        0));

    // Recreate RTVs for the resized swap chain back buffers
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

    // Create (or recreate) the off-screen R32G32B32A32_FLOAT render target for OCIO rendering.
    // This avoids 8-bit UNORM quantization errors when comparing GPU vs CPU results.
    m_floatRenderTarget.Reset();

    D3D12_RESOURCE_DESC floatRtDesc = {};
    floatRtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    floatRtDesc.Width = static_cast<UINT>(width);
    floatRtDesc.Height = static_cast<UINT>(height);
    floatRtDesc.DepthOrArraySize = 1;
    floatRtDesc.MipLevels = 1;
    floatRtDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    floatRtDesc.SampleDesc.Count = 1;
    floatRtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE floatClearValue = {};
    floatClearValue.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &floatRtDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &floatClearValue,
        IID_PPV_ARGS(&m_floatRenderTarget)));

    // Create a single-slot RTV heap for the float render target (created once)
    if (!m_floatRtvHeap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC floatRtvHeapDesc = {};
        floatRtvHeapDesc.NumDescriptors = 1;
        floatRtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        floatRtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&floatRtvHeapDesc, IID_PPV_ARGS(&m_floatRtvHeap)));
    }

    // Create the float RTV
    D3D12_RENDER_TARGET_VIEW_DESC floatRtvDesc = {};
    floatRtvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    floatRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    m_device->CreateRenderTargetView(m_floatRenderTarget.Get(), &floatRtvDesc,
        m_floatRtvHeap->GetCPUDescriptorHandleForHeapStart());

    // Recreate readback buffer with updated dimensions (if already allocated by createBuffers)
    if (m_readbackBuffer)
    {
        m_readbackBuffer.Reset();
        const UINT pixelSize = 4 * sizeof(float);  // R32G32B32A32_FLOAT = 16 bytes per pixel
        m_readbackRowPitch = AlignRowPitch(static_cast<UINT>(width), pixelSize);
        const UINT64 readbackBufferSize = m_readbackRowPitch * static_cast<UINT>(height);

        CD3DX12_HEAP_PROPERTIES readbackHeapProps(D3D12_HEAP_TYPE_READBACK);
        CD3DX12_RESOURCE_DESC readbackBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(readbackBufferSize);
        ThrowIfFailed(m_device->CreateCommittedResource(
            &readbackHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &readbackBufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_readbackBuffer)));
    }
}

void DxApp::redisplay()
{
    ThrowIfFailed(m_commandAllocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

    UINT frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Transition swap chain back buffer to render target for clearing
    auto barrierPresentToRt = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrierPresentToRt);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
        m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, m_rtvDescriptorSize);

    // Clear the swap chain back buffer (used for display only)
    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // If pipeline state is ready, render the OCIO transform to the off-screen float render target.
    // The float RT preserves full precision for accurate GPU vs CPU comparison in tests.
    if (m_pipelineState && m_floatRenderTarget)
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE floatRtvHandle(
            m_floatRtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Clear the float render target
        m_commandList->ClearRenderTargetView(floatRtvHandle, clearColor, 0, nullptr);

        // Set descriptor heap for shader resources
        ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvSrvHeap.Get() };
        m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

        // Set the root signature and pipeline state
        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        m_commandList->SetPipelineState(m_pipelineState.Get());

        // Set the root descriptor table to the start of the SRV heap (image + LUTs)
        CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
        m_commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

        // Update and bind the constant buffer (root parameter 1, b0).
        // This fills in any OCIO uniform variables (dynamic properties like exposure, curves).
        if (m_constantBuffer && m_cbMappedData && m_currentShaderDesc)
        {
            memset(m_cbMappedData, 0, m_cbufferAlignedSize);

            const unsigned numUniforms = m_currentShaderDesc->getNumUniforms();
            for (unsigned i = 0; i < numUniforms; ++i)
            {
                GpuShaderDesc::UniformData data;
                m_currentShaderDesc->getUniform(i, data);
                UINT8* dst = m_cbMappedData + data.m_bufferOffset;

                switch (data.m_type)
                {
                case UNIFORM_DOUBLE:
                    if (data.m_getDouble)
                    {
                        const float val = static_cast<float>(data.m_getDouble());
                        memcpy(dst, &val, sizeof(float));
                    }
                    break;
                case UNIFORM_BOOL:
                    if (data.m_getBool)
                    {
                        const int val = data.m_getBool() ? 1 : 0;
                        memcpy(dst, &val, sizeof(int));
                    }
                    break;
                case UNIFORM_FLOAT3:
                    if (data.m_getFloat3)
                    {
                        const Float3& f3 = data.m_getFloat3();
                        memcpy(dst, f3.data(), 3 * sizeof(float));
                    }
                    break;
                case UNIFORM_VECTOR_FLOAT:
                    if (data.m_vectorFloat.m_getSize && data.m_vectorFloat.m_getVector)
                    {
                        const int sz = data.m_vectorFloat.m_getSize();
                        const float* vals = data.m_vectorFloat.m_getVector();
                        for (int j = 0; j < sz; ++j)
                            memcpy(dst + j * 16, &vals[j], sizeof(float));
                    }
                    break;
                case UNIFORM_VECTOR_INT:
                    if (data.m_vectorInt.m_getSize && data.m_vectorInt.m_getVector)
                    {
                        const int sz = data.m_vectorInt.m_getSize();
                        const int* vals = data.m_vectorInt.m_getVector();
                        for (int j = 0; j < sz; ++j)
                            memcpy(dst + j * 16, &vals[j], sizeof(int));
                    }
                    break;
                default:
                    break;
                }
            }

            m_commandList->SetGraphicsRootConstantBufferView(
                1, m_constantBuffer->GetGPUVirtualAddress());
        }

        // Set viewport and scissor rect
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_viewportWidth);
        viewport.Height = static_cast<float>(m_viewportHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect = { 0, 0, m_viewportWidth, m_viewportHeight };
        m_commandList->RSSetScissorRects(1, &scissorRect);

        // Render to the off-screen float render target
        m_commandList->OMSetRenderTargets(1, &floatRtvHandle, FALSE, nullptr);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->DrawInstanced(3, 1, 0, 0);
    }

    // Transition swap chain back buffer back to PRESENT state
    auto barrierRtToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        m_renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrierRtToPresent);

    ThrowIfFailed(m_commandList->Close());

    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    ThrowIfFailed(m_swapChain->Present(1, 0));

    waitForPreviousFrame();
}

void DxApp::readImage(float* imageBuffer)
{
    ThrowIfFailed(m_commandAllocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    // Transition the float render target from RENDER_TARGET to COPY_SOURCE for readback
    auto barrierReadback1 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_floatRenderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_commandList->ResourceBarrier(1, &barrierReadback1);

    // Copy from float render target to readback buffer
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Offset = 0;
    footprint.Footprint.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    footprint.Footprint.Width = static_cast<UINT>(m_viewportWidth);
    footprint.Footprint.Height = static_cast<UINT>(m_viewportHeight);
    footprint.Footprint.Depth = 1;
    footprint.Footprint.RowPitch = m_readbackRowPitch;

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = m_floatRenderTarget.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = m_readbackBuffer.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dstLocation.PlacedFootprint = footprint;

    m_commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

    // Transition the float render target back to RENDER_TARGET for the next frame
    auto barrierReadback2 = CD3DX12_RESOURCE_BARRIER::Transition(
        m_floatRenderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrierReadback2);

    ThrowIfFailed(m_commandList->Close());

    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    waitForPreviousFrame();

    // Map the readback buffer and copy float data row-by-row to caller's buffer,
    // stripping D3D12 row-pitch alignment padding
    BYTE* pData = nullptr;
    ThrowIfFailed(m_readbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData)));

    const UINT dstRowPitch = static_cast<UINT>(m_viewportWidth) * 4 * sizeof(float);
    for (int y = 0; y < m_viewportHeight; ++y)
    {
        const BYTE* srcRow = pData + y * m_readbackRowPitch;
        BYTE* dstRow = reinterpret_cast<BYTE*>(imageBuffer) + y * dstRowPitch;
        memcpy(dstRow, srcRow, dstRowPitch);
    }

    m_readbackBuffer->Unmap(0, nullptr);
}

void DxApp::printGraphicsInfo() const noexcept
{
    try
    {
        // Query IDXGIDevice from the D3D12 device
        ComPtr<IDXGIDevice> dxgiDevice;
        if (FAILED(m_device.As(&dxgiDevice)))
        {
            return;  // Silently ignore failure
        }

        // Get the adapter from the DXGI device
        ComPtr<IDXGIAdapter> adapter;
        if (FAILED(dxgiDevice->GetAdapter(&adapter)))
        {
            return;  // Silently ignore failure
        }

        // Get adapter description
        DXGI_ADAPTER_DESC desc;
        if (FAILED(adapter->GetDesc(&desc)))
        {
            return;  // Silently ignore failure
        }

        // Convert wide string to narrow string for Description
        char narrowDesc[128];
        WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, narrowDesc, sizeof(narrowDesc), nullptr, nullptr);

        // Print adapter name and dedicated video memory
        std::cout << "Adapter: " << narrowDesc << std::endl;
        std::cout << "Dedicated Video Memory: " << (desc.DedicatedVideoMemory / (1024 * 1024)) << " MB" << std::endl;
    }
    catch (...)
    {
        // Silently ignore any exceptions
    }
}

void DxApp::waitForPreviousFrame()
{
    // Signal and increment the fence value.
    const UINT64 fence = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}

} // namespace OCIO_NAMESPACE
