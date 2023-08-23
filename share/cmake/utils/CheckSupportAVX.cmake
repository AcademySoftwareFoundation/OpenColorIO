# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(_cmake_required_flags_orig "${CMAKE_REQUIRED_FLAGS}")

if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "/w /arch:AVX")
elseif(USE_GCC OR USE_CLANG)
    set(CMAKE_REQUIRED_FLAGS "-w -mavx")
endif()

set(AVX_CODE "
    #include <iostream>
    #include <immintrin.h>

    int main() 
    {
        // Create two arrays of floats
        float a[8] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
        float b[8] = {2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0};

        _mm256_add_ps(_mm256_load_ps(a), _mm256_load_ps(b));
        return 0; 
    }
")

file(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/avx_test.cpp" "${AVX_CODE}")

message(STATUS "Performing Test COMPILER_SUPPORTS_AVX")
try_compile(COMPILER_SUPPORTS_AVX
  "${CMAKE_BINARY_DIR}/CMakeTmp"
  "${CMAKE_BINARY_DIR}/CMakeTmp/avx_test.cpp"
)

if(COMPILER_SUPPORTS_AVX)
  message(STATUS "Performing Test COMPILER_SUPPORTS_AVX - Success")
else()
    message(STATUS "Performing Test COMPILER_SUPPORTS_AVX - Failed")
endif()

set(CMAKE_REQUIRED_FLAGS "${_cmake_required_flags_orig}")
unset(_cmake_required_flags_orig)
