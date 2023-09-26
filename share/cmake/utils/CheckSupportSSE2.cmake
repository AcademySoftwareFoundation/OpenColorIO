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
    # x86_64 always has SSE2
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        # Simulate the same message we would get by using check_cxx_source_compiles. 
        message(STATUS "x86_64 always support SSE2 - COMPILER_SUPPORTS_SSE2 - Success")
        # By setting the variable to 1, tuhe check_cxx_source_compiles will be skipped automatically.
        set(COMPILER_SUPPORTS_SSE2 1)
    else()
        check_cxx_compiler_flag("/arch:SSE2" COMPILER_SUPPORTS_SSE2)
    endif()
elseif(USE_GCC OR USE_CLANG)
    set(CMAKE_CXX_FLAGS "-w -msse2")
endif()

if (APPLE AND __universal_build)
    # Force the test to build under x86_64
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
    # Apple has an automatic translation layer from SSE to ARM Neon.
endif()

set(SSE2_CODE "
    #include <emmintrin.h>

    int main() 
    { 
        __m128d a, b;
        double vals[2] = {0};
        a = _mm_loadu_pd (vals);
        b = _mm_add_pd (a,a);
        _mm_storeu_pd (vals,b);
        return (0);
    }
")

file(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/sse2_test.cpp" "${SSE2_CODE}")

message(STATUS "Performing Test COMPILER_SUPPORTS_SSE2")
try_compile(COMPILER_SUPPORTS_SSE2
  "${CMAKE_BINARY_DIR}/CMakeTmp"
  "${CMAKE_BINARY_DIR}/CMakeTmp/sse2_test.cpp"
)

if(COMPILER_SUPPORTS_SSE2)
    message(STATUS "Performing Test COMPILER_SUPPORTS_SSE2 - Success")
else()
    message(STATUS "Performing Test COMPILER_SUPPORTS_SSE2 - Failed")
endif()

set(CMAKE_CXX_FLAGS "${_cmake_cxx_flags_orig}")
unset(_cmake_cxx_flags_orig)

if(__universal_build)
    set(CMAKE_OSX_ARCHITECTURES "${_cmake_osx_architectures_orig}")
    unset(_cmake_osx_architectures_orig)
    unset(__universal_build)
endif()