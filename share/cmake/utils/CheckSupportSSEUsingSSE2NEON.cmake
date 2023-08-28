# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

set(_cmake_required_flags_orig "${CMAKE_REQUIRED_FLAGS}")
set(_cmake_required_includes_orig "${CMAKE_REQUIRED_INCLUDES}")
set(_cmake_osx_architectures_orig "${CMAKE_OSX_ARCHITECTURES}")

if(APPLE AND COMPILER_SUPPORTS_ARM_NEON)

    if("${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm64;x86_64" OR
       "${CMAKE_OSX_ARCHITECTURES}" MATCHES "x86_64;arm64")
        # universal build
        # Force the test to build under arm64
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    endif()

    set(SSE2NEON_CODE "
        #include <sse2neon.h>

        int main() 
        { 
            // SSE2
            __m128d a, b;
            double vals[2] = {0};
            a = _mm_loadu_pd (vals);
            b = _mm_add_pd (a,a);
            _mm_storeu_pd (vals,b);

            // SSE3
            _mm_addsub_ps(_mm_setzero_ps(), _mm_setzero_ps());

            // SSSE3
            _mm_shuffle_epi8(_mm_setzero_si128(), _mm_setzero_si128()); 

            // SSE4
            _mm_blend_epi16(_mm_setzero_si128(), _mm_setzero_si128(), 0);

            // SSE 4.2
            _mm_cmpgt_epi64(_mm_set_epi64x(5, 10), _mm_set_epi64x(8, 5));
            
            return (0);
        }
    ")

    file(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/sse2neon_test.cpp" "${SSE2NEON_CODE}")

    message(STATUS "Performing Test COMPILER_SUPPORTS_SSE_WITH_SSE2NEON")
    try_compile(COMPILER_SUPPORTS_SSE_WITH_SSE2NEON
      "${CMAKE_BINARY_DIR}/CMakeTmp"
      "${CMAKE_BINARY_DIR}/CMakeTmp/sse2neon_test.cpp"
      COMPILE_DEFINITIONS "-I${sse2neon_INCLUDE_DIR}"
    )

    if(COMPILER_SUPPORTS_SSE_WITH_SSE2NEON)
        message(STATUS "Performing Test COMPILER_SUPPORTS_SSE_WITH_SSE2NEON - Success")
    else()
        message(STATUS "Performing Test COMPILER_SUPPORTS_SSE_WITH_SSE2NEON - Failed")
    endif()
endif()

set(CMAKE_REQUIRED_FLAGS "${_cmake_required_flags_orig}")
set(CMAKE_REQUIRED_INCLUDES "${_cmake_required_includes_orig}")
set(CMAKE_OSX_ARCHITECTURES "${_cmake_osx_architectures_orig}")

unset(_cmake_required_flags_orig)
unset(_cmake_required_includes_orig)
unset(_cmake_osx_architectures_orig)

