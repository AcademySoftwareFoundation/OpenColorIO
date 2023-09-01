// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include "CPUInfo.h"
#include <string.h>

#if _WIN32
#include <limits.h>
#include <intrin.h>
typedef unsigned __int32  uint32_t;
typedef __int64  int64_t;
#else
#include <stdint.h>
#endif

namespace OCIO_NAMESPACE
{

#if !defined(__aarch64__) && OCIO_ARCH_X86 // Intel-based processor or Apple Rosetta x86_64.

namespace {

union CPUIDResult
{
    int i[4];
    char c[16];
    struct {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    } reg;
};

static inline int64_t xgetbv()
{
    int index = 0;
#if _MSC_VER
    return _xgetbv(index);
#else
    int eax = 0;
    int edx = 0;
    __asm__ volatile (".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c" (index));
    return  (int64_t)edx << 32 | (int64_t)eax;
#endif
}

static inline void cpuid(int index, int *data)
{
#if _MSC_VER
    __cpuid(data, index);
#else
    __asm__ volatile (
        "mov    %%rbx, %%rsi \n\t"
        "cpuid               \n\t"
        "xchg   %%rbx, %%rsi"
        : "=a" (data[0]), "=S" (data[1]), "=c" (data[2]), "=d" (data[3])
        : "0" (index), "2"(0));
#endif
}

} // anonymous namespace

CPUInfo::CPUInfo()
{
    flags = 0, family = 0, model = 0;
    memset(name, 0, sizeof(name));
    memset(vendor, 0, sizeof(vendor));

    CPUIDResult info;
    uint32_t max_std_level, max_ext_level;
    int64_t xcr = 0;

    cpuid(0, info.i);
    max_std_level = info.i[0];
    memcpy(vendor + 0, &info.i[1], 4);
    memcpy(vendor + 4, &info.i[3], 4);
    memcpy(vendor + 8, &info.i[2], 4);

    if (max_std_level >= 1)
    {
        cpuid(1, info.i);
        family = ((info.reg.eax >> 8) & 0xf) + ((info.reg.eax >> 20) & 0xff);
        model  = ((info.reg.eax >> 4) & 0xf) + ((info.reg.eax >> 12) & 0xf0);

        if (info.reg.edx & (1 << 26))
            flags |= X86_CPU_FLAG_SSE2;

        if (info.reg.ecx & 1)
            flags |= X86_CPU_FLAG_SSE3;

        if (info.reg.ecx & 0x00000200)
            flags |= X86_CPU_FLAG_SSSE3;

        if (info.reg.ecx & 0x00080000)
            flags |= X86_CPU_FLAG_SSE4;

        if (info.reg.ecx & 0x00100000)
            flags |= X86_CPU_FLAG_SSE42;

        /* Check OSXSAVE and AVX bits */
        if (info.reg.ecx & 0x18000000)
        {
            xcr = xgetbv();
            if(xcr & 0x6) {
                flags |= X86_CPU_FLAG_AVX;

                if(info.reg.ecx & 0x20000000) {
                    flags |= X86_CPU_FLAG_F16C;
                }
            }
        }
    }

    if (max_std_level >= 7)
    {
        cpuid(7, info.i);

        if ((flags & X86_CPU_FLAG_AVX) && (info.reg.ebx & 0x00000020))
            flags |= X86_CPU_FLAG_AVX2;

        /* OPMASK/ZMM state */
        if ((xcr & 0xe0) == 0xe0) {
            if ((flags & X86_CPU_FLAG_AVX2) && (info.reg.ebx & 0xd0030000))
                flags |= X86_CPU_FLAG_AVX512;
        }
    }

    cpuid(0x80000000, info.i);
    max_ext_level = info.i[0];

    if (max_ext_level >= 0x80000001)
    {
        cpuid(0x80000001, info.i);
        if (!strncmp(vendor, "AuthenticAMD", 12)) {

            /* Athlon64, some Opteron, and some Sempron processors */
            if (flags & X86_CPU_FLAG_SSE2 && !(info.reg.ecx & 0x00000040))
                flags |= X86_CPU_FLAG_SSE2_SLOW;

            /* Bulldozer and Jaguar based CPUs */
            if ((family == 0x15 || family == 0x16) && (flags & X86_CPU_FLAG_AVX))
                flags |= X86_CPU_FLAG_AVX_SLOW;

            /* Zen 3 and earlier have slow gather */
            if ((family <= 0x19) && (flags & X86_CPU_FLAG_AVX2))
                flags |= X86_CPU_FLAG_AVX2_SLOWGATHER;
        }
    }

    if (!strncmp(vendor, "GenuineIntel", 12))
    {
        if (family == 6 && (model == 9 || model == 13 || model == 14))
        {
            if (flags & X86_CPU_FLAG_SSE2)
                flags |= X86_CPU_FLAG_SSE2_SLOW;

            if (flags & X86_CPU_FLAG_SSE3)
                flags |= X86_CPU_FLAG_SSE3_SLOW;
        }

        /* Conroe has a slow shuffle unit */
        if ((flags & X86_CPU_FLAG_SSSE3) && !(flags & X86_CPU_FLAG_SSE4) && family == 6 && model < 23)
            flags |= X86_CPU_FLAG_SSSE3_SLOW;

        /* Haswell has slow gather */
        if ((flags & X86_CPU_FLAG_AVX2) && family == 6 && model < 70)
            flags |= X86_CPU_FLAG_AVX2_SLOWGATHER;
    }

    // get cpu brand string
    for(int index = 0; index < 3; index++)
    {
      cpuid(0x80000002 + index, (int *)(name + 16*index));
    }
}

CPUInfo& CPUInfo::instance()
{
    static CPUInfo singleton = CPUInfo();
    return singleton;
}
#elif defined(__aarch64__) // ARM Processor or Apple ARM.
CPUInfo::CPUInfo()
{
    flags = 0;
    memset(name, 0, sizeof(name));

    snprintf(name, sizeof(name), "%s", "ARM");

    // SSE2NEON library supports SSE, SSE2, SSE3, SSSE3, SSE4.1 and SSE4.2.
    // It does not support any AVX instructions.
    if (OCIO_USE_SSE2)
    {
        flags |= X86_CPU_FLAG_SSE2;
        flags |= X86_CPU_FLAG_SSE3;
        flags |= X86_CPU_FLAG_SSSE3;
        flags |= X86_CPU_FLAG_SSE4;
        flags |= X86_CPU_FLAG_SSE42;
    }
}

CPUInfo& CPUInfo::instance()
{
    static CPUInfo singleton = CPUInfo();
    return singleton;
}
#endif

} // namespace OCIO_NAMESPACE