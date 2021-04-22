# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# Define the global compilation and link flags.

set(PLATFORM_COMPILE_FLAGS "")

if(USE_MSVC)

    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /DUSE_MSVC")

    # /wd4275 Exceptions derive from std::runtime_error but STL classes are not exportable.
    # /we4062 Enables warning in switch when an enumeration value is not explicitly handled.
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /EHsc /wd4275 /DWIN32 /we4062")

    if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 17)
        # Inheriting from std::iterator is deprecated starting with C++17 and Yaml 0.6.3 does that.
        set(PLATFORM_COMPILE_FLAGS 
            "${PLATFORM_COMPILE_FLAGS} /D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING"
        )
    endif()

    if(OCIO_WARNING_AS_ERROR)
        set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /WX")
    endif()

elseif(USE_CLANG)

    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -DUSE_CLANG")

    # Use of 'register' specifier must be removed for C++17 support.
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wno-deprecated-register")

elseif(USE_GCC)

    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -DUSE_GCC")

endif()


if(USE_GCC OR USE_CLANG)

    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wall")

    # Add more warning detection.
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wextra")

    # -Wswitch-enum Enables warning in switch when an enumeration value is not explicitly handled.
    set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wswitch-enum")

    if(OCIO_WARNING_AS_ERROR)
        set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Werror")
    endif()

    if(OCIO_INLINES_HIDDEN)
        set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -fvisibility-inlines-hidden")
    endif()

    if(APPLE)
        # Mute the deprecated warning for some GLUT methods.
        set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wno-deprecated-declarations")
    endif()

endif()

# An advanced variable will not be displayed in any of the cmake GUIs

mark_as_advanced(PLATFORM_COMPILE_FLAGS)


###############################################################################
# Define if SSE2 can be used.

include(CheckSupportSSE2)

if(NOT HAVE_SSE2)
    message(STATUS "Disabling SSE optimizations, as the target doesn't support them")
    set(OCIO_USE_SSE OFF)
endif(NOT HAVE_SSE2)


###############################################################################
# Define RPATH.

if (UNIX AND NOT CMAKE_SKIP_RPATH)
    # With the 'usual' install path structure in mind, search from the bin directory
    # (i.e. a binary loading a dynamic library) and then from the current directory
    # (i.e. dynamic library loading another dynamic library).  
    if (APPLE)
        set(CMAKE_INSTALL_RPATH "@loader_path/../lib;@loader_path")
    else()
        set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN")
    endif()
endif()
