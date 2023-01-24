# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate OpenImageIO
#
# Variables defined by this module:
#   OpenImageIO_FOUND       - If FALSE, do not try to link to OpenImageIO
#   OpenImageIO_LIBRARY     - OpenImageIO library to link to
#   OpenImageIO_LIB_DIR     - Libray directory
#   OpenImageIO_INCLUDE_DIR - Where to find OpenImageIO.h
#   OpenImageIO_VERSION     - The version of the library
#
# Targets defined by this module:
#   OpenImageIO::OpenImageIO       - The libOpenImageIO library
#   OpenImageIO::OpenImageIO_Util  - Utility classes (v2.3+)
#

if(NOT DEFINED OpenImageIO_ROOT)
    # Search for OpenImageIOConfig.cmake
    find_package(OpenImageIO ${OpenImageIO_FIND_VERSION} CONFIG QUIET)
endif()

if (OpenImageIO_FOUND)
    # IMPORTED_GLOBAL property must be set to TRUE since alisasing a non-global imported target
    # is not possible until CMake 3.18+.
    set_target_properties(OpenImageIO::OpenImageIO PROPERTIES IMPORTED_GLOBAL TRUE)

    # Set the ROOT directory by going up a directory from OpenImageIO_LIB_DIR.
    get_filename_component(_PARENT_DIR ${OpenImageIO_LIB_DIR} DIRECTORY)
    # find_library will prioritize OpenImageIO_ROOT before any other path.
    set(OpenImageIO_ROOT ${_PARENT_DIR})

    # Set OpenImageIO_LIBRARY since OpenImageIOConfig does not do it.
    find_library (OpenImageIO_LIBRARY
        NAMES
            OpenImageIO
        HINTS
            ${OpenImageIO_LIB_DIR}
        PATH_SUFFIXES
            lib64
            lib
            OpenImageIO/lib
    )

    # Find util library
    find_library (OpenImageIO_UTIL_LIBRARY
        NAMES
            OpenImageIO_Util
        HINTS
            ${OpenImageIO_LIB_DIR}
        PATH_SUFFIXES
            lib64
            lib
            OpenImageIO/lib
    )

else()
    # Search for pkg config file.
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_OpenImageIO QUIET "OpenImageIO>=${OpenImageIO_FIND_VERSION}")

    # Find include directory
    # Since CMake 3.12, find_path search through <pkg>_ROOT before everything else.
    find_path(OpenImageIO_INCLUDE_DIR
        NAMES
            oiioversion.h
        HINTS
            ${PC_OpenImageIO_INCLUDE_DIRS}
        PATH_SUFFIXES
            include/OpenImageIO
            OpenImageIO
            include
            OpenImageIO/include
    )

    # Set OpenImageIO_INCLUDES since OpenImageIOConfig does it.
    set(OpenImageIO_INCLUDES ${OpenImageIO_INCLUDE_DIR})

    # Find library
    # Since CMake 3.12, find_library search through <pkg>_ROOT before everything else.
    find_library (OpenImageIO_LIBRARY
        NAMES
            OpenImageIO
        HINTS
            ${PC_OpenImageIO_LIBRARY_DIRS}
        PATH_SUFFIXES
            lib64
            lib
            OpenImageIO/lib
    )

    # Find util library
    find_library (OpenImageIO_UTIL_LIBRARY
        NAMES
            OpenImageIO_Util
        HINTS
            ${PC_OpenImageIO_LIBRARY_DIRS}
        PATH_SUFFIXES
            lib64
            lib
            OpenImageIO/lib
    )

    # Set OpenImageIO_LIB_DIR since OpenImageIOConfig does it.
    get_filename_component(OpenImageIO_LIB_DIR ${OpenImageIO_LIBRARY} DIRECTORY)

    if (OpenImageIO_INCLUDE_DIR)
        # Try to figure out version number
        list(GET OpenImageIO_INCLUDE_DIR 0 _OpenImageIO_INCLUDE_DIR)
        if(EXISTS "${_OpenImageIO_INCLUDE_DIR}/oiioversion.h")
            set(OIIO_VERSION_HEADER "${_OpenImageIO_INCLUDE_DIR}/oiioversion.h")
        elseif(EXISTS "${_OpenImageIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h")
            set(OIIO_VERSION_HEADER "${_OpenImageIO_INCLUDE_DIR}/OpenImageIO/oiioversion.h")
        endif()

        if (EXISTS "${OIIO_VERSION_HEADER}")
            file (STRINGS "${OIIO_VERSION_HEADER}" TMP REGEX "^#define OIIO_VERSION_MAJOR .*$")
            string (REGEX MATCHALL "[0-9]+" OpenImageIO_VERSION_MAJOR ${TMP})
            file (STRINGS "${OIIO_VERSION_HEADER}" TMP REGEX "^#define OIIO_VERSION_MINOR .*$")
            string (REGEX MATCHALL "[0-9]+" OpenImageIO_VERSION_MINOR ${TMP})
            file (STRINGS "${OIIO_VERSION_HEADER}" TMP REGEX "^#define OIIO_VERSION_PATCH .*$")
            string (REGEX MATCHALL "[0-9]+" OpenImageIO_VERSION_PATCH ${TMP})
            file (STRINGS "${OIIO_VERSION_HEADER}" TMP REGEX "^#define OIIO_VERSION_TWEAK .*$")
            if (TMP)
                string (REGEX MATCHALL "[0-9]+" OpenImageIO_VERSION_TWEAK ${TMP})
            else ()
                set (OpenImageIO_VERSION_TWEAK 0)
            endif ()
            set (OpenImageIO_VERSION "${OpenImageIO_VERSION_MAJOR}.${OpenImageIO_VERSION_MINOR}.${OpenImageIO_VERSION_PATCH}.${OpenImageIO_VERSION_TWEAK}")
        endif ()

        set (OIIO_IMATH_HEADER "${OpenImageIO_INCLUDE_DIR}/OpenImageIO/Imath.h")
        if (EXISTS "${OIIO_IMATH_HEADER}")
            file (STRINGS "${OIIO_IMATH_HEADER}" TMP REGEX "^#define OIIO_USING_IMATH .*$")
            string (REGEX MATCHALL "[0-9]" OIIO_IMATH_VERSION ${TMP})
            if (OIIO_IMATH_VERSION LESS 3)
                message(STATUS "Skipping OpenImageIO built against OpenEXR 2, please use version 3 or greater.")
                return ()
            endif ()
        endif ()
    elseif(PC_OpenImageIO_FOUND)
        set(OpenImageIO_VERSION "${PC_OpenImageIO_VERSION}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenImageIO
    REQUIRED_VARS 
        OpenImageIO_LIB_DIR
        OpenImageIO_INCLUDE_DIR
        OpenImageIO_LIBRARY
    VERSION_VAR
        OpenImageIO_VERSION
)

###############################################################################
### Create target

if (OpenImageIO_FOUND AND NOT TARGET OpenImageIO::OpenImageIO)
    add_library(OpenImageIO::OpenImageIO UNKNOWN IMPORTED GLOBAL)
    
    # Configure target

    set_target_properties(OpenImageIO::OpenImageIO PROPERTIES
        IMPORTED_LOCATION "${OpenImageIO_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OpenImageIO_INCLUDE_DIR}")

    set_property(TARGET OpenImageIO::OpenImageIO APPEND PROPERTY
        IMPORTED_LOCATION "${OpenImageIO_LIBRARY}")

    # Starting with OIIO v2.3, some utility classes are now only declared in OpenImageIO_Util
    # (and not in both libraries like in older versions).
    if (${OpenImageIO_VERSION} VERSION_GREATER_EQUAL "2.3" AND NOT TARGET OpenImageIO::OpenImageIO_Util)
        add_library(OpenImageIO::OpenImageIO_Util UNKNOWN IMPORTED)
        set_target_properties(OpenImageIO::OpenImageIO_Util PROPERTIES
            IMPORTED_LOCATION "${OpenImageIO_UTIL_LIBRARY}")
        target_link_libraries(OpenImageIO::OpenImageIO INTERFACE OpenImageIO::OpenImageIO_Util)
    endif ()

    # Starting with OIIO v2.3, OIIO needs to compile at least in C++14.
    if (${OpenImageIO_VERSION} VERSION_GREATER_EQUAL "2.3" AND ${CMAKE_CXX_STANDARD} LESS_EQUAL 11)
        set(OpenImageIO_FOUND OFF)
        message(WARNING "Need C++14 or higher to compile with OpenImageIO ${OpenImageIO_VERSION}.")
    endif ()
    
    mark_as_advanced(OpenImageIO_INCLUDE_DIR OpenImageIO_LIBRARY OpenImageIO_VERSION)
endif()