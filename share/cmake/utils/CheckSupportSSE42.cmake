# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

set(_cmake_cxx_flags_orig "${CMAKE_CXX_FLAGS}")

if(APPLE AND ("${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm64;x86_64" 
          OR "${CMAKE_OSX_ARCHITECTURES}" MATCHES "x86_64;arm64"))
    set(__universal_build 1)
    set(_cmake_osx_architectures_orig "${CMAKE_OSX_ARCHITECTURES}")
endif()

# MSVC doesn't have flags
if(USE_GCC OR USE_CLANG)
    set(CMAKE_CXX_FLAGS "-w -msse4.2")
endif()

if (APPLE AND __universal_build)
    # Force the test to build under x86_64
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    # Apple has an automatic translation layer from SSE to ARM Neon.
endif()

set(SSE42_CODE "
    #include <nmmintrin.h>
    
    int main() 
    { 
        _mm_cmpgt_epi64(_mm_set_epi64x(5, 10), _mm_set_epi64x(8, 5)); 
        return 0; 
    }
")

file(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/sse42_test.cpp" "${SSE42_CODE}")

message(STATUS "Performing Test COMPILER_SUPPORTS_SSE42")
try_compile(COMPILER_SUPPORTS_SSE42
  "${CMAKE_BINARY_DIR}/CMakeTmp"
  "${CMAKE_BINARY_DIR}/CMakeTmp/sse42_test.cpp"
)

if(COMPILER_SUPPORTS_SSE42)
    message(STATUS "Performing Test COMPILER_SUPPORTS_SSE42 - Success")
else()
    message(STATUS "Performing Test COMPILER_SUPPORTS_SSE42 - Failed")
endif()

set(CMAKE_CXX_FLAGS "${_cmake_cxx_flags_orig}")
unset(_cmake_cxx_flags_orig)

if(__universal_build)
    set(CMAKE_OSX_ARCHITECTURES "${_cmake_osx_architectures_orig}")
    unset(_cmake_osx_architectures_orig)
    unset(__universal_build)
endif()