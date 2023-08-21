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
check_cxx_source_compiles("${F16C_CODE}" COMPILER_SUPPORTS_F16C)

set(CMAKE_REQUIRED_FLAGS "${_cmake_required_flags_orig}")
unset(_cmake_required_flags_orig)