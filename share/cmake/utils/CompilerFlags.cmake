# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# Define the global compilation and link flags.

set(PLATFORM_COMPILE_OPTIONS "")
set(PLATFORM_LINK_OPTIONS "")

###############################################################################
# Verify SIMD compatibility

if(OCIO_USE_SIMD)
    if (OCIO_ARCH_X86)
        include(CheckSupportX86SIMD)
    endif()

    if (OCIO_USE_SSE2NEON AND COMPILER_SUPPORTS_ARM_NEON)
        include(CheckSupportSSEUsingSSE2NEON)
        if(COMPILER_SUPPORTS_SSE_WITH_SSE2NEON)
            if(WIN32 AND MSVC)
                # Enable the "new" preprocessor, to more closely match Clang/GCC, required for sse2neon
                set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};/Zc:preprocessor")
            endif()
        else()
            set(OCIO_USE_SSE2NEON OFF)
        endif()
    endif()
else()
    set(OCIO_USE_SSE2 OFF)
    set(OCIO_USE_SSE3 OFF)
    set(OCIO_USE_SSSE3 OFF)
    set(OCIO_USE_SSE4 OFF)
    set(OCIO_USE_SSE42 OFF)
    set(OCIO_USE_AVX OFF)
    set(OCIO_USE_AVX2 OFF)
    set(OCIO_USE_AVX512 OFF)
    set(OCIO_USE_F16C OFF)

    set(OCIO_USE_SSE2NEON OFF)
endif()

if (NOT COMPILER_SUPPORTS_SSE2 AND NOT COMPILER_SUPPORTS_SSE_WITH_SSE2NEON AND
    NOT COMPILER_SUPPORTS_SSE3 AND NOT COMPILER_SUPPORTS_SSSE3 AND
    NOT COMPILER_SUPPORTS_SSE4 AND NOT COMPILER_SUPPORTS_SSE42 AND
    NOT COMPILER_SUPPORTS_AVX AND NOT COMPILER_SUPPORTS_AVX2 AND NOT COMPILER_SUPPORTS_AVX512 AND
    NOT COMPILER_SUPPORTS_F16C)
    message(STATUS "Disabling SIMD optimizations, as the target doesn't support them")
    set(OCIO_USE_SIMD OFF)
endif()

###############################################################################
# Compile flags

if(USE_MSVC)

    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};/DUSE_MSVC")

    # /we4062 Enables warning in switch when an enumeration value is not explicitly handled.
    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};/EHsc;/DWIN32;/we4062")

	# seems no longer needed, there were c++ modernization on 0.7.0. /coz
#    if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 17)
#        # Inheriting from std::iterator is deprecated starting with C++17 and Yaml 0.6.3 does that.
#        set(PLATFORM_COMPILE_OPTIONS
#            "${PLATFORM_COMPILE_OPTIONS};/D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING"
#        )
#    endif()

    # Make MSVC compiler report correct __cplusplus version (otherwise reports 199711L)
    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};/Zc:__cplusplus")

    # Explicitely specify the default warning level i.e. /W3.
    # Note: Do not use /Wall (i.e. /W4) which adds 'informational' warnings.
    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};/W3")

    # Do enable C4701 (Potentially uninitialized local variable 'name' used), which is level 4.
    # This is because strtoX-based from_chars leave the value variable unmodified.
    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};/we4701")

    if(OCIO_WARNING_AS_ERROR)
        set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};/WX")
    endif()

    # Enable parallel compilation of source files
    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};/MP")

elseif(USE_CLANG)

    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};-DUSE_CLANG")

    # Use of 'register' specifier must be removed for C++17 support.
    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};-Wno-deprecated-register")
elseif(USE_GCC)

    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};-DUSE_GCC")

endif()


if(USE_GCC OR USE_CLANG)

    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};-Wall")

    # Add more warning detection.
    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};-Wextra")

    # -Wswitch-enum Enables warning in switch when an enumeration value is not explicitly handled.
    set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};-Wswitch-enum")

    if(OCIO_WARNING_AS_ERROR)
        set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};-Werror")
    endif()

    if(APPLE)
        # TODO: There are still some deprecated methods.
        set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};-Wno-deprecated-declarations")
    endif()

    if(OCIO_ENABLE_SANITIZER)
        set(PLATFORM_COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS};-fno-omit-frame-pointer;-fsanitize=address;-fsanitize=undefined")
        set(PLATFORM_LINK_OPTIONS "${PLATFORM_LINK_OPTIONS};-fsanitize=address;-fsanitize=undefined")
    endif()

endif()

# An advanced variable will not be displayed in any of the cmake GUIs

mark_as_advanced(PLATFORM_COMPILE_OPTIONS)
mark_as_advanced(PLATFORM_LINK_OPTIONS)


###############################################################################
# Define hidden symbol visibility by default for all targets.

include(VariableUtils)

set_unless_defined(CMAKE_C_VISIBILITY_PRESET hidden)
set_unless_defined(CMAKE_CXX_VISIBILITY_PRESET hidden)
set_unless_defined(CMAKE_VISIBILITY_INLINES_HIDDEN YES)


###############################################################################
# Define RPATH.

if (UNIX AND NOT CMAKE_SKIP_RPATH AND NOT CMAKE_INSTALL_RPATH)
    # With the 'usual' install path structure in mind, search from the bin directory
    # (i.e. a binary loading a dynamic library) and then from the current directory
    # (i.e. dynamic library loading another dynamic library).  
    if (APPLE)
        set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR};@loader_path")
    else()
        set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR};$ORIGIN")
    endif()
endif()
