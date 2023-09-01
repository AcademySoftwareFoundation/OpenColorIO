// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef CPUInfo_H
#define CPUInfo_H

#include <OpenColorIO/OpenColorIO.h>
#include "CPUInfoConfig.h"

namespace OCIO_NAMESPACE
{

#define X86_CPU_FLAG_SSE2             (1 << 0) // SSE2 functions
#define X86_CPU_FLAG_SSE2_SLOW        (1 << 1) // SSE2 supported, but usually not faster than regular MMX/SSE (e.g. Core1)

#define X86_CPU_FLAG_SSE3             (1 << 2) // Prescott SSE3 functions
#define X86_CPU_FLAG_SSE3_SLOW        (1 << 3) // SSE3 supported, but usually not faster than regular MMX/SSE (e.g. Core1)

#define X86_CPU_FLAG_SSSE3            (1 << 4) // Conroe SSSE3 functions
#define X86_CPU_FLAG_SSSE3_SLOW       (1 << 5) // SSSE3 supported, but usually not faster than SSE2

#define X86_CPU_FLAG_SSE4             (1 << 6) // Penryn SSE4.1 functions
#define X86_CPU_FLAG_SSE42            (1 << 7) // Nehalem SSE4.2 functions

#define X86_CPU_FLAG_AVX              (1 << 8) // AVX functions: requires OS support even if YMM registers aren't used
#define X86_CPU_FLAG_AVX_SLOW         (1 << 9) // AVX supported, but slow when using YMM registers (e.g. Bulldozer)

#define X86_CPU_FLAG_AVX2            (1 << 10) // AVX2 functions: requires OS support even if YMM registers aren't used
#define X86_CPU_FLAG_AVX2_SLOWGATHER (1 << 11) // CPU has slow gathers.

#define X86_CPU_FLAG_AVX512          (1 << 12) // AVX-512 functions: requires OS support even if YMM/ZMM registers aren't used

#define X86_CPU_FLAG_F16C            (1 << 13) // CPU Has FP16C half float, AVX2 should always have this??

#define x86_check_flags(cpuext) \
    (OCIO_USE_ ## cpuext && ((flags) & X86_CPU_FLAG_ ## cpuext))

#if !defined(__aarch64__) && OCIO_ARCH_X86 // Intel-based processor or Apple Rosetta x86_64.

struct CPUInfo
{
    unsigned int flags;
    int family;
    int model;
    char name[65];
    char vendor[13];

    CPUInfo();

    static CPUInfo& instance();

    const char *getName() const { return name;}
    const char *getVendor() const { return vendor; }

    bool hasSSE2() const { return x86_check_flags(SSE2); }
    bool SSE2Slow() const { return (OCIO_USE_SSE2 && (flags & X86_CPU_FLAG_SSE2_SLOW)); }

    bool hasSSE3() const { return x86_check_flags(SSE3); }
    bool SSE3Slow() const { return (OCIO_USE_SSE3 && (flags & X86_CPU_FLAG_SSE3_SLOW)); }

    bool hasSSSE3() const { return x86_check_flags(SSSE3); }
    bool SSSE3Slow() const { return (OCIO_USE_SSSE3 && (flags & X86_CPU_FLAG_SSSE3_SLOW)); }

    bool hasSSE4() const { return x86_check_flags(SSE4); }
    bool hasSSE42() const { return x86_check_flags(SSE42); }

    bool hasAVX() const { return x86_check_flags(AVX); }
    bool AVXSlow() const { return (OCIO_USE_AVX && (flags & X86_CPU_FLAG_AVX_SLOW)); }

    bool hasAVX2() const { return x86_check_flags(AVX2); }
    bool AVX2SlowGather() const { return (OCIO_USE_AVX2 && (flags & X86_CPU_FLAG_AVX2_SLOWGATHER)); }

    bool hasAVX512() const { return x86_check_flags(AVX512); }

    bool hasF16C() const { return x86_check_flags(F16C); }

};

#undef x86_check_flags

#elif defined(__aarch64__) // ARM Processor or Apple ARM.

#define check_flags(cpuext) \
    (OCIO_USE_ ## cpuext && ((flags) & X86_CPU_FLAG_ ## cpuext))

struct CPUInfo
{
    unsigned int flags;
    char name[65];

    CPUInfo();

    static CPUInfo& instance();

    bool hasSSE2() const { return x86_check_flags(SSE2); }
    bool SSE2Slow() const { return false; }

    bool hasSSE3() const { return x86_check_flags(SSE3); }
    bool SSE3Slow() const { return false; }

    bool hasSSSE3() const { return x86_check_flags(SSSE3); }
    bool SSSE3Slow() const {  return false; }

    bool hasSSE4() const { return x86_check_flags(SSE4); }
    bool hasSSE42() const { return false; }

    // Apple M1 does not support AVX SIMD instructions through Rosetta.
    // SSE2NEON library does not supports AVX SIMD instructions.
    bool hasAVX() const { return false; }
    bool AVXSlow() const { return false; }
    bool hasAVX2() const { return false; }
    bool AVX2SlowGather() const { return false; }
    bool hasAVX512() const { return false; }
    bool hasF16C() const { return false; }

};

#undef x86_check_flags

#endif

} // namespace OCIO_NAMESPACE

#endif // CPUInfo_H
