# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(CheckCXXSourceCompiles)

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

mark_as_advanced(HAVE_SSE2)
