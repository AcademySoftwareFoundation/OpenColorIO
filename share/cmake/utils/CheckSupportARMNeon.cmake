# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

set(_cmake_required_flags_old "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_FLAGS "-march=armv8-a+fp+simd+crypto+crc")

check_cxx_source_compiles ("
    #include <arm_neon.h>
    int main()
    {
        float32x4_t v = vdupq_n_f32(0);
        return 0;
    }"
    HAVE_NEON)

set(CMAKE_REQUIRED_FLAGS "${_cmake_required_flags_old}")
unset(_cmake_required_flags_old)

mark_as_advanced(HAVE_NEON)
