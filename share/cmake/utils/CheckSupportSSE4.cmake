# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

set(_cmake_required_flags_orig "${CMAKE_REQUIRED_FLAGS}")

if(APPLE AND "${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm64;x86_64" 
          OR "${CMAKE_OSX_ARCHITECTURES}" MATCHES "x86_64;arm64")
    set(__universal_build 1)
    set(_cmake_osx_architectures_orig "${CMAKE_OSX_ARCHITECTURES}")
endif()

# MSVC doesn't have flags
if(USE_GCC OR USE_CLANG)
    set(CMAKE_REQUIRED_FLAGS "-w -msse4")
endif()

if (APPLE AND __universal_build)
    # Force the test to build under x86_64
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    # Apple has an automatic translation layer from SSE/AVX to ARM Neon.
endif()

set(SSE4_CODE "
    #include <smmintrin.h>

    int main() 
    { 
        _mm_blend_epi16(_mm_setzero_si128(), _mm_setzero_si128(), 0);
        return 0; 
    }
")
check_cxx_source_compiles("${SSE4_CODE}" COMPILER_SUPPORTS_SSE4)

set(CMAKE_REQUIRED_FLAGS "${_cmake_required_flags_orig}")
unset(_cmake_required_flags_orig)

if(__universal_build)
    set(_cmake_osx_architectures_orig "${CMAKE_OSX_ARCHITECTURES}")
    unset(_cmake_osx_architectures_orig)
    unset(__universal_build)
endif()