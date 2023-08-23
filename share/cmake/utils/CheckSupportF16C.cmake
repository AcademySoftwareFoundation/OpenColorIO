# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

set(_cmake_required_flags_orig "${CMAKE_REQUIRED_FLAGS}")

# MSVC doesn't have flags
if(USE_GCC OR USE_CLANG)
    set(CMAKE_REQUIRED_FLAGS "-w -mf16c")
endif()

set(F16C_CODE "
    #include <immintrin.h>

    int main() 
    { 
        _mm_cvtph_ps(_mm_set1_epi16(0x3C00)); 
        return 0; 
    }
")

file(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/sse42_test.cpp" "${F16C_CODE}")

message(STATUS "Performing Test COMPILER_SUPPORTS_F16C")
try_compile(COMPILER_SUPPORTS_F16C
  "${CMAKE_BINARY_DIR}/CMakeTmp"
  "${CMAKE_BINARY_DIR}/CMakeTmp/sse42_test.cpp"
)

if(COMPILER_SUPPORTS_F16C)
  message(STATUS "Performing Test COMPILER_SUPPORTS_F16C - Success")
else()
    message(STATUS "Performing Test COMPILER_SUPPORTS_F16C - Failed")
endif()

set(CMAKE_REQUIRED_FLAGS "${_cmake_required_flags_orig}")
unset(_cmake_required_flags_orig)