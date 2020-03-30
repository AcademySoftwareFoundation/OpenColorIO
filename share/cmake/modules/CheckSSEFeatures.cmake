# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

if (NOT CMAKE_SIZE_OF_VOID_P EQUAL 8)
    if (MSVC)
        set(CMAKE_REQUIRED_FLAGS "/arch:SSE2")
    else ()
        set(CMAKE_REQUIRED_FLAGS "-msse2")
    endif ()
endif (NOT CMAKE_SIZE_OF_VOID_P EQUAL 8)

check_cxx_source_compiles ("
    #include <emmintrin.h>
    int main ()
    {
        __m128d a, b;
        double vals[2] = {0};
        a = _mm_loadu_pd (vals);
        b = _mm_add_pd (a,a);
        _mm_storeu_pd (vals,b);
        return (0);
    }"
    HAVE_SSE2)

MARK_AS_ADVANCED (HAVE_SSE2)
