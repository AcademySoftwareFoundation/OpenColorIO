# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# Checks for ARM NEON availability

include(CheckCXXSourceCompiles)

set(_cmake_osx_architectures_orig "${CMAKE_OSX_ARCHITECTURES}")

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

set(source_code "
    #include <arm_neon.h>
    int main()
    {
        float32x4_t v = vdupq_n_f32(0);
        return 0;
}")

check_cxx_source_compiles ("${source_code}" COMPILER_SUPPORTS_ARM_NEON)

set(CMAKE_OSX_ARCHITECTURES "${_cmake_osx_architectures_orig}")

unset(_cmake_osx_architectures_orig)
mark_as_advanced(COMPILER_SUPPORTS_ARM_NEON)
