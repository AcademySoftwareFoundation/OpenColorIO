# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

set(_cmake_required_flags_orig "${CMAKE_REQUIRED_FLAGS}")
set(_cmake_required_includes_orig "${CMAKE_REQUIRED_INCLUDES}")
set(_cmake_osx_architectures_orig "${CMAKE_OSX_ARCHITECTURES}")

set(_cmake_osx_architectures_old "${CMAKE_OSX_ARCHITECTURES}")

if(NOT CMAKE_SIZE_OF_VOID_P EQUAL 8)
    # As CheckCXXCompilerFlag implicitly uses CMAKE_CXX_FLAGS some custom flags could trigger
    # unrelated warnings causing a detection failure. So, the code disables all warnings to focus
    # on the SSE2 detection.
    if(USE_MSVC)
        set(CMAKE_REQUIRED_FLAGS "/w /arch:SSE2")
    elseif(USE_GCC OR USE_CLANG)
        set(CMAKE_REQUIRED_FLAGS "-w -msse2")
    endif()
endif()


macro(check_sse2_availability _check_sse2_header_ _check_output_var_name_)
    set(_SSE2_TEST_SOURCE_CODE "
    ${_check_sse2_header_}
    int main ()
    {
        __m128d a, b;
        double vals[2] = {0};
        a = _mm_loadu_pd (vals);
        b = _mm_add_pd (a,a);
        _mm_storeu_pd (vals,b);
        return (0);
    }")

    check_cxx_source_compiles ("${_SSE2_TEST_SOURCE_CODE}" ${_check_output_var_name_})
    mark_as_advanced(${_check_output_var_name_})
endmacro()

if(NOT HAVE_NEON)
    check_sse2_availability("#include <emmintrin.h>" HAVE_SSE2)
elseif(APPLE AND HAVE_NEON)
    # Test for both supported architectures
    # x86_64 and arm64
    set(ARCHITECTURES_LIST "arm64;x86_64")
    
    message(STATUS "Checking SSE2 support using SSE2NEON library for arm64 and x86_64 architectures")
    foreach (current_arch IN LISTS ARCHITECTURES_LIST)

        set (CMAKE_OSX_ARCHITECTURES "${current_arch}")
        
        if(current_arch STREQUAL arm64)
            set(CMAKE_REQUIRED_INCLUDES ${sse2neon_INCLUDE_DIR})
            set(_sse2_header_ "#include <sse2neon.h>")
            set(_output_var_name_ "HAVE_SSE2_WITH_SSE2NEON")
        elseif(current_arch STREQUAL x86_64)
            set(_sse2_header_ "#include <emmintrin.h>")
            set(_output_var_name_ "HAVE_SSE2")
        endif()

        check_sse2_availability("${_sse2_header_}" ${_output_var_name_})

    endforeach()
endif()

set(CMAKE_REQUIRED_FLAGS "${_cmake_required_flags_orig}")
set(CMAKE_REQUIRED_INCLUDES "${_cmake_required_includes_orig}")
set(CMAKE_OSX_ARCHITECTURES "${_cmake_osx_architectures_orig}")

unset(_cmake_required_flags_orig)
unset(_cmake_required_includes_orig)
unset(_cmake_osx_architectures_orig)


