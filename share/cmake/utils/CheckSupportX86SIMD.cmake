# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# Check if compiler supports X86 SIMD extensions

# These checks use try_compile instead of check_cxx_source_compiles because the latter was causing
# false positives on Apple ARM architectures.

include(CheckSupportAVX)
include(CheckSupportAVX2)
include(CheckSupportAVX512)

include(CheckSupportSSE42)
include(CheckSupportSSE4)
include(CheckSupportSSSE3)
include(CheckSupportSSE3)
include(CheckSupportSSE2)
include(CheckSupportF16C)

if(MSVC)
    if (COMPILER_SUPPORTS_SSE2)
        set(OCIO_SSE2_ARGS "/arch:SSE2")
    endif()

    if (COMPILER_SUPPORTS_AVX)
        set(OCIO_AVX_ARGS "/arch:AVX")
    endif()

    if (COMPILER_SUPPORTS_AVX2)
        set(OCIO_AVX2_ARGS "/arch:AVX2")
    endif()
else()
    if (COMPILER_SUPPORTS_SSE2)
        set(OCIO_SSE2_ARGS "-msse2")
    endif()
    
    if (COMPILER_SUPPORTS_AVX)
        set(OCIO_AVX_ARGS "-mavx")
    endif()

    if (COMPILER_SUPPORTS_AVX2)
        set(OCIO_AVX2_ARGS "-mavx2" "-mfma")
    endif()    
endif()

if(${OCIO_USE_AVX512} AND NOT ${COMPILER_SUPPORTS_AVX512})
    message(STATUS "OCIO_USE_AVX512 requested but compiler does not support, disabling")
    set(OCIO_USE_AVX512 OFF)
endif()

if(${OCIO_USE_AVX2} AND NOT ${COMPILER_SUPPORTS_AVX2})
    message(STATUS "OCIO_USE_AVX2 requested but compiler does not support, disabling")
    set(OCIO_USE_AVX2 OFF)
endif()

if(${OCIO_USE_AVX} AND NOT ${COMPILER_SUPPORTS_AVX})
    message(STATUS "OCIO_USE_AVX requested but compiler does not support, disabling")
    set(OCIO_USE_AVX OFF)
endif()

if(${OCIO_USE_SSE42} AND NOT ${COMPILER_SUPPORTS_SSE42})
    message(STATUS "OCIO_USE_SSE42 requested but compiler does not support, disabling")
    set(OCIO_USE_SSE42 OFF)
endif()

if(${OCIO_USE_SSE4} AND NOT ${COMPILER_SUPPORTS_SSE4})
    message(STATUS "OCIO_USE_SSE4 requested but compiler does not support, disabling")
    set(OCIO_USE_SSE4 OFF)
endif()

if(${OCIO_USE_SSSE3} AND NOT ${COMPILER_SUPPORTS_SSSE3})
    message(STATUS "OCIO_USE_SSSE3 requested but compiler does not support, disabling")
    set(OCIO_USE_SSSE3 OFF)
endif()

if(${OCIO_USE_SSE3} AND NOT ${COMPILER_SUPPORTS_SSE3})
    message(STATUS "OCIO_USE_SSE3 requested but compiler does not support, disabling")
    set(OCIO_USE_SSE3 OFF)
endif()

if(${OCIO_USE_SSE2} AND NOT ${COMPILER_SUPPORTS_SSE2})
    message(STATUS "OCIO_USE_SSE2 requested but compiler does not support, disabling")
    set(OCIO_USE_SSE2 OFF)
endif()

if(${OCIO_USE_F16C} AND NOT ${COMPILER_SUPPORTS_F16C})
    message(STATUS "OCIO_USE_F16C requested but compiler does not support, disabling")
    set(OCIO_USE_F16C OFF)
endif()

if(${OCIO_USE_F16C})
    if(NOT MSVC)
        list(APPEND OCIO_AVX_ARGS -mf16c)
        list(APPEND OCIO_AVX2_ARGS -mf16c)
    endif()
endif()