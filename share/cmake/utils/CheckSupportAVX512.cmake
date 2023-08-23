# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

set(_cmake_required_flags_orig "${CMAKE_REQUIRED_FLAGS}")

if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "/w /arch:AVX512")
elseif(USE_GCC OR USE_CLANG)
    set(CMAKE_REQUIRED_FLAGS "-w -mavx512f")
endif()

set(AVX512_CODE "
    #include <immintrin.h>

    int main() {
        __m512i vec = _mm512_set1_epi32(42);
        return 0;
    }
")

file(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/avx512_test.cpp" "${AVX512_CODE}")

message(STATUS "Performing Test COMPILER_SUPPORTS_AVX512")
try_compile(COMPILER_SUPPORTS_AVX512
  "${CMAKE_BINARY_DIR}/CMakeTmp"
  "${CMAKE_BINARY_DIR}/CMakeTmp/avx512_test.cpp"
)

if(COMPILER_SUPPORTS_AVX512)
  message(STATUS "Performing Test COMPILER_SUPPORTS_AVX512 - Success")
else()
    message(STATUS "Performing Test COMPILER_SUPPORTS_AVX512 - Failed")
endif()

set(CMAKE_REQUIRED_FLAGS "${_cmake_required_flags_orig}")
unset(_cmake_required_flags_orig)