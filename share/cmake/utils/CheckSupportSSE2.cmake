# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

set(_cmake_required_flags_old "${CMAKE_REQUIRED_FLAGS}")
set(_cmake_required_libraries_old "${CMAKE_REQUIRED_LIBRARIES}")

if(NOT CMAKE_SIZE_OF_VOID_P EQUAL 8)
    # As CheckCXXCompilerFlag implicitly uses CMAKE_CXX_FLAGS some custom flags could trigger
    # unrelated warnings causing a detection failure. So, the code disables all warnings to focus
    # on the SSE2 detection.
    if(USE_MSVC)
        set(CMAKE_REQUIRED_FLAGS "/w /arch:SSE2")
    elseif(USE_GCC OR USE_CLANG)
        set(CMAKE_REQUIRED_FLAGS "-w -msse2")
    endif()
endif()

set(_SSE2_HEADER "#include <emmintrin.h>")
if (HAVE_NEON)
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -march=armv8-a+fp+simd+crypto+crc")
    set(CMAKE_REQUIRED_LIBRARIES sse2neon)
    set(_SSE2_HEADER "#include <sse2neon.h>")
endif()

set(_SSE2_TEST_SOURCE_CODE "
${_SSE2_HEADER}
int main ()
{
    __m128d a, b;
    double vals[2] = {0};
    a = _mm_loadu_pd (vals);
    b = _mm_add_pd (a,a);
    _mm_storeu_pd (vals,b);
    return (0);
}")

check_cxx_source_compiles ("${_SSE2_TEST_SOURCE_CODE}" HAVE_SSE2)

set(CMAKE_REQUIRED_FLAGS "${_cmake_required_flags_old}")
set(CMAKE_REQUIRED_LIBRARIES "${_cmake_required_libraries_old}")
unset(_cmake_required_flags_old)
unset(_cmake_required_libraries_old)

mark_as_advanced(HAVE_SSE2)
