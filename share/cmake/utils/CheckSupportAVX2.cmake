# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

set(_cmake_cxx_flags_orig "${CMAKE_CXX_FLAGS}")

if(APPLE AND ("${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm64;x86_64" 
          OR "${CMAKE_OSX_ARCHITECTURES}" MATCHES "x86_64;arm64"))
    set(__universal_build 1)
    set(_cmake_osx_architectures_orig "${CMAKE_OSX_ARCHITECTURES}")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "/w /arch:AVX2")
elseif(USE_GCC OR USE_CLANG)
    set(CMAKE_CXX_FLAGS "-w -mavx2 -mfma -mf16c")
endif()

if (APPLE AND __universal_build)
    # Force the test to build under x86_64
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    # Apple has an automatic translation layer from SSE to ARM Neon.
endif()

set(AVX2_CODE "
    #include <immintrin.h>

    int main() 
    {
        __m256i a = _mm256_set_epi32(1, 2, 3, 4, 5, 6, 7, 8);
        __m256i b = _mm256_set_epi32(8, 7, 6, 5, 4, 3, 2, 1);
        __m256i result = _mm256_add_epi32(a, b);

        __m256 result_f16c = _mm256_cvtph_ps(_mm_set_epi16(1, 2, 3, 4, 5, 6, 7, 8));

        __m256 result_fma = _mm256_fmadd_ps(
            _mm256_set_ps(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0),
            _mm256_set_ps(8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0),
            _mm256_set_ps(1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0)
        );

        return 0;
    }
")

file(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/avx2_test.cpp" "${AVX2_CODE}")

message(STATUS "Performing Test COMPILER_SUPPORTS_AVX2")
try_compile(COMPILER_SUPPORTS_AVX2
  "${CMAKE_BINARY_DIR}/CMakeTmp"
  "${CMAKE_BINARY_DIR}/CMakeTmp/avx2_test.cpp"
)

if(COMPILER_SUPPORTS_AVX2)
    message(STATUS "Performing Test COMPILER_SUPPORTS_AVX2 - Success")
else()
    message(STATUS "Performing Test COMPILER_SUPPORTS_AVX2 - Failed")
endif()

set(CMAKE_CXX_FLAGS "${_cmake_cxx_flags_orig}")
unset(_cmake_cxx_flags_orig)

if(__universal_build)
    set(CMAKE_OSX_ARCHITECTURES "${_cmake_osx_architectures_orig}")
    unset(_cmake_osx_architectures_orig)
    unset(__universal_build)
endif()