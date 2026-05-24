// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#pragma once

#include <string>

#include <OpenColorIO/OpenColorIO.h>

#include <directx/d3dx12.h>

namespace OCIO_NAMESPACE
{

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw Exception(HrToString(hr).c_str());
    }
}

// Align a row pitch to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT (256 bytes).
inline UINT AlignRowPitch(UINT width, UINT pixelSize)
{
    return (width * pixelSize + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)
           & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
}

} // namespace OCIO_NAMESPACE
