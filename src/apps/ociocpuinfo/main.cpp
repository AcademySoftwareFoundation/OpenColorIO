// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <iostream>

#include "OpenColorABI.h"

#include "CPUInfo.h"

namespace OCIO = OCIO_NAMESPACE;

int main()
{
    const OCIO::CPUInfo& cpu = OCIO::CPUInfo::instance();

    std::cout << "name           : " << cpu.getName()        << "\n";
    std::cout << "vendor         : " << cpu.getVendor()      << "\n";
    std::cout << "hasSSE2        : " << cpu.hasSSE2()        << "\n";
    std::cout << "SSE2Slow       : " << cpu.SSE2Slow()       << "\n";
    std::cout << "hasSSE3        : " << cpu.hasSSE3()        << "\n";
    std::cout << "SSE3Slow       : " << cpu.SSE3Slow()       << "\n";
    std::cout << "hasSSSE3       : " << cpu.hasSSSE3()       << "\n";
    std::cout << "SSSE3Slow      : " << cpu.SSSE3Slow()      << "\n";
    std::cout << "hasSSE4        : " << cpu.hasSSE4()        << "\n";
    std::cout << "hasSSE42       : " << cpu.hasSSE42()       << "\n";
    std::cout << "hasAVX         : " << cpu.hasAVX()         << "\n";
    std::cout << "AVXSlow        : " << cpu.AVXSlow()        << "\n";
    std::cout << "hasAVX2        : " << cpu.hasAVX2()        << "\n";
    std::cout << "AVX2SlowGather : " << cpu.AVX2SlowGather() << "\n";
    std::cout << "hasAVX512      : " << cpu.hasAVX512()      << "\n";
    std::cout << "hasF16C        : " << cpu.hasF16C()        << "\n";

    return 0;
}