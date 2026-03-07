// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#pragma once
#include <string>
#include <OpenColorIO/OpenColorIO.h>

#include "graphicalapp.h"
#include "hlsl.h"

#include <dxgi1_6.h>

using Microsoft::WRL::ComPtr;

namespace OCIO_NAMESPACE
{

class DxApp : public GraphicalApp
{
public:
    DxApp() = delete;
    DxApp(const DxApp&) = delete;
    DxApp& operator=(const DxApp&) = delete;

    DxApp(const char* winTitle, int winWidth, int winHeight);
    ~DxApp();

    virtual void initImage(int imageWidth, int imageHeight,
        Components comp, const float* imageBuffer) override;

    virtual void updateImage(const float* imageBuffer) override;

    virtual void createBuffers() override;

    virtual void setShader(GpuShaderDescRcPtr& shaderDesc) override;

    virtual void reshape(int width, int height) override;

    virtual void redisplay() override;

    virtual void readImage(float* imageBuffer) override;

    virtual void printGraphicsInfo() const noexcept override;

private:
    void waitForPreviousFrame();

    static const UINT FrameCount = 2;

    int m_viewportWidth{ 0 };
    int m_viewportHeight{ 0 };

    int m_imageWidth{ 0 };
    int m_imageHeight{ 0 };
    Components m_comp{ COMPONENTS_RGBA };

    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize;
    UINT m_cbvSrvDescriptorSize;

    // Synchronization objects.
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    // Image texture and upload resources.
    ComPtr<ID3D12Resource> m_imageTexture;
    ComPtr<ID3D12Resource> m_imageUploadBuffer;
    ComPtr<ID3D12Resource> m_readbackBuffer;
    UINT m_readbackRowPitch{ 0 };

    // Off-screen float render target (R32G32B32A32_FLOAT) for OCIO rendering and readback.
    // The swap chain back buffers (UNORM) are required for windowing but unused in tests.
    ComPtr<ID3D12Resource> m_floatRenderTarget;
    ComPtr<ID3D12DescriptorHeap> m_floatRtvHeap;

    // HLSL shader builder
    HLSLBuilderRcPtr m_hlslBuilder;

    // Constant buffer for OCIO uniform variables (dynamic properties like exposure, curves).
    // Always allocated; root signature always includes a CBV at b0.
    ComPtr<ID3D12Resource> m_constantBuffer;
    UINT8* m_cbMappedData{ nullptr };
    UINT m_cbufferAlignedSize{ 0 };

    // Current shader description, retained so redisplay() can update uniform values.
    GpuShaderDescRcPtr m_currentShaderDesc;

    // Window handle and class name for cleanup.
    HWND m_hwnd{ nullptr };
    std::string m_windowClassName;
};

}

