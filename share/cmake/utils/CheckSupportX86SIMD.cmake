# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# Check if compiler supports X86 SIMD extensions

if(MSVC)
    # x86_64 always has SSE2
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        set(COMPILER_SUPPORTS_SSE2 1)
    else()
        check_cxx_compiler_flag("/arch:SSE2" COMPILER_SUPPORTS_SSE2)
        set(OCIO_SSE2_ARGS "/arch:SSE2")
    endif()
    check_cxx_compiler_flag("/arch:AVX" COMPILER_SUPPORTS_AVX)
    check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_AVX2)
    check_cxx_compiler_flag("/arch:AVX512" COMPILER_SUPPORTS_AVX512)
    # MSVC doesn't have flags for these, if AVX available assume they are too
    set(COMPILER_SUPPORTS_SSE42 ${COMPILER_SUPPORTS_AVX})
    set(COMPILER_SUPPORTS_SSE4  ${COMPILER_SUPPORTS_AVX})
    set(COMPILER_SUPPORTS_SSSE3 ${COMPILER_SUPPORTS_AVX})
    set(COMPILER_SUPPORTS_SSE3  ${COMPILER_SUPPORTS_AVX})
    set(COMPILER_SUPPORTS_F16C  ${COMPILER_SUPPORTS_AVX})

    set(OCIO_AVX_ARGS "/arch:AVX")
    set(OCIO_AVX2_ARGS "/arch:AVX2")

else()
    check_cxx_compiler_flag("-msse2" COMPILER_SUPPORTS_SSE2)
    check_cxx_compiler_flag("-msse3" COMPILER_SUPPORTS_SSE3)
    check_cxx_compiler_flag("-mssse3" COMPILER_SUPPORTS_SSSE3)
    check_cxx_compiler_flag("-msse4" COMPILER_SUPPORTS_SSE4)
    check_cxx_compiler_flag("-msse4.2" COMPILER_SUPPORTS_SSE42)
    check_cxx_compiler_flag("-mavx" COMPILER_SUPPORTS_AVX)
    check_cxx_compiler_flag("-mavx2 -mfma -mf16c" CCOMPILER_SUPPORTS_AVX2)
    check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
    check_cxx_compiler_flag("-mf16c" COMPILER_SUPPORTS_F16C)

    set(OCIO_SSE2_ARGS "-msse2")
    set(OCIO_AVX_ARGS  "-mavx")
    set(OCIO_AVX2_ARGS "-mavx2" "-mfma")
endif()

if(${OCIO_USE_AVX512} AND NOT ${COMPILER_SUPPORTS_AVX512})
    message(STATUS "OCIO_USE_AVX512 requested but compiler does not support, disabling")
    set(OCIO_USE_AVX512 0)
endif()

if(${OCIO_USE_AVX2} AND NOT ${COMPILER_SUPPORTS_AVX2})
    message(STATUS "OCIO_USE_AVX2 requested but compiler does not support, disabling")
    set(OCIO_USE_AVX2 0)
endif()

if(${OCIO_USE_AVX} AND NOT ${COMPILER_SUPPORTS_AVX})
    message(STATUS "OCIO_USE_AVX requested but compiler does not support, disabling")
    set(OCIO_USE_AVX 0)
endif()

if(${OCIO_USE_SSE42} AND NOT ${COMPILER_SUPPORTS_SSE42})
    message(STATUS "OCIO_USE_SSE42 requested but compiler does not support, disabling")
    set(OCIO_USE_SSE42 0)
endif()

if(${OCIO_USE_SSE4} AND NOT ${COMPILER_SUPPORTS_SSE4})
    message(STATUS "OCIO_USE_SSE4 requested but compiler does not support, disabling")
    set(OCIO_USE_SSE4 0)
endif()

if(${OCIO_USE_SSSE3} AND NOT ${COMPILER_SUPPORTS_SSSE3})
    message(STATUS "OCIO_USE_SSSE3 requested but compiler does not support, disabling")
    set(OCIO_USE_SSSE3 0)
endif()

if(${OCIO_USE_SSE3} AND NOT ${COMPILER_SUPPORTS_SSE3})
    message(STATUS "OCIO_USE_SSE3 requested but compiler does not support, disabling")
    set(OCIO_USE_SSE3 0)
endif()

if(${OCIO_USE_SSE2} AND NOT ${COMPILER_SUPPORTS_SSE2})
    message(STATUS "OCIO_USE_SSE2 requested but compiler does not support, disabling")
    set(OCIO_USE_SSE2 0)
endif()

if(${OCIO_USE_F16C} AND NOT ${COMPILER_SUPPORTS_F16C})
    message(STATUS "OCIO_USE_F16C requested but compiler does not support, disabling")
    set(OCIO_USE_F16C 0)
endif()

if(${OCIO_USE_F16C})
    if(NOT MSVC)
        list(APPEND OCIO_SSE2_ARGS -mf16c)
        list(APPEND OCIO_AVX_ARGS -mf16c)
        list(APPEND OCIO_AVX2_ARGS -mf16c)
    endif()
endif()