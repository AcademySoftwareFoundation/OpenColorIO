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
    set(CMAKE_CXX_FLAGS "-w -mssse3")
endif()

if (APPLE AND __universal_build)
    # Force the test to build under x86_64
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    # Apple has an automatic translation layer from SSE to ARM Neon.
endif()

set(SSSE3_CODE "
    #include <tmmintrin.h>

    int main() 
    { 
        _mm_shuffle_epi8(_mm_setzero_si128(), _mm_setzero_si128()); 
        return 0; 
    }
")

file(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/ssse3_test.cpp" "${SSSE3_CODE}")

message(STATUS "Performing Test COMPILER_SUPPORTS_SSSE3")
try_compile(COMPILER_SUPPORTS_SSSE3
  "${CMAKE_BINARY_DIR}/CMakeTmp"
  "${CMAKE_BINARY_DIR}/CMakeTmp/ssse3_test.cpp"
)

if(COMPILER_SUPPORTS_SSSE3)
    message(STATUS "Performing Test COMPILER_SUPPORTS_SSSE3 - Success")
else()
    message(STATUS "Performing Test COMPILER_SUPPORTS_SSSE3 - Failed")
endif()

set(CMAKE_CXX_FLAGS "${_cmake_cxx_flags_orig}")
unset(_cmake_cxx_flags_orig)

if(__universal_build)
    set(CMAKE_OSX_ARCHITECTURES "${_cmake_osx_architectures_orig}")
    unset(_cmake_osx_architectures_orig)
    unset(__universal_build)
endif()