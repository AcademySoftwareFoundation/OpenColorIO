# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

set(_cmake_required_flags_orig "${CMAKE_REQUIRED_FLAGS}")

if(APPLE AND "${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm64;x86_64" 
          OR "${CMAKE_OSX_ARCHITECTURES}" MATCHES "x86_64;arm64")
    set(__universal_build 1)
    set(_cmake_osx_architectures_orig "${CMAKE_OSX_ARCHITECTURES}")
endif()

if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "/w /arch:AVX")
elseif(USE_GCC OR USE_CLANG)
    set(CMAKE_REQUIRED_FLAGS "-w -mavx")
endif()

if (APPLE AND __universal_build)
    # Force the test to build under x86_64
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    # Apple has an automatic translation layer from SSE/AVX to ARM Neon.
endif()

set(AVX_CODE "
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
check_cxx_source_compiles("${AVX_CODE}" COMPILER_SUPPORTS_AVX)

set(CMAKE_REQUIRED_FLAGS "${_cmake_required_flags_orig}")
unset(_cmake_required_flags_orig)

if(__universal_build)
    set(_cmake_osx_architectures_orig "${CMAKE_OSX_ARCHITECTURES}")
    unset(_cmake_osx_architectures_orig)
    unset(__universal_build)
endif()