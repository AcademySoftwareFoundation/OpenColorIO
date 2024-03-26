// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <iostream>

#include "CPUInfo.h"

namespace OCIO = OCIO_NAMESPACE;

int main()
{
    const OCIO::CPUInfo& cpu = OCIO::CPUInfo::instance();

    std::cout << "name           : " << cpu.getName()        << std::endl;
    std::cout << "vendor         : " << cpu.getVendor()      << std::endl;
    std::cout << "hasSSE2        : " << cpu.hasSSE2()        << std::endl;
    std::cout << "SSE2Slow       : " << cpu.SSE2Slow()       << std::endl;
    std::cout << "hasSSE3        : " << cpu.hasSSE3()        << std::endl;
    std::cout << "SSE3Slow       : " << cpu.SSE3Slow()       << std::endl;
    std::cout << "hasSSSE3       : " << cpu.hasSSSE3()       << std::endl;
    std::cout << "SSSE3Slow      : " << cpu.SSSE3Slow()      << std::endl;
    std::cout << "hasSSE4        : " << cpu.hasSSE4()        << std::endl;
    std::cout << "hasSSE42       : " << cpu.hasSSE42()       << std::endl;
    std::cout << "hasAVX         : " << cpu.hasAVX()         << std::endl;
    std::cout << "AVXSlow        : " << cpu.AVXSlow()        << std::endl;
    std::cout << "hasAVX2        : " << cpu.hasAVX2()        << std::endl;
    std::cout << "AVX2SlowGather : " << cpu.AVX2SlowGather() << std::endl;
    std::cout << "hasAVX512      : " << cpu.hasAVX512()      << std::endl;
    std::cout << "hasF16C        : " << cpu.hasF16C()        << std::endl;

    return 0;
}