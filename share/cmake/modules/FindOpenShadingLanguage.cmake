# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install OpenShadingLanguage (OSL)
#
# If the library is not installed in a standard path, you can use the OSL_ROOT 
# variable to tell CMake where to find it. 
#


if(NOT TARGET osl::osl)
    add_library(osl::osl INTERFACE IMPORTED GLOBAL)
    set(OSL_FOUND OFF)
endif()

###############################################################################
### Try to find package ###

if(NOT DEFINED OSL_ROOT)

    find_package(OSL ${OpenShadingLanguage_FIND_VERSION} CONFIG)
    
    set(OpenShadingLanguage_VERSION ${OSL_VERSION})

    # TODO: No variable to have the share directory?

    set(OSL_SHADERS_INCLUDE_DIR ${OSL_INCLUDE_DIR}/../share)

    # Variable used by the OSL unit tests. 
    set(OSL_SHADERS_DIR ${OSL_SHADERS_INCLUDE_DIR}/OSL/shaders)

    include (FindPackageHandleStandardArgs)
    find_package_handle_standard_args (OpenShadingLanguage
        FOUND_VAR     OpenShadingLanguage_FOUND
        REQUIRED_VARS OSL_INCLUDE_DIR OSL_LIB_DIR OpenShadingLanguage_VERSION
        VERSION_VAR   OpenShadingLanguage_VERSION
    )

    set(OSL_FOUND ${OpenShadingLanguage_FOUND})

else()

    set(OSL_INCLUDE_DIR ${OSL_ROOT}/include)

    set(OSL_VERSION_HEADER "${OSL_INCLUDE_DIR}/OSL/oslversion.h")

    if(EXISTS "${OSL_VERSION_HEADER}")

        # Try to figure out version number
        file (STRINGS "${OSL_VERSION_HEADER}" TMP REGEX "^#define OSL_LIBRARY_VERSION_MAJOR .*$")
        string (REGEX MATCHALL "[0-9]+" OSL_VERSION_MAJOR ${TMP})
        file (STRINGS "${OSL_VERSION_HEADER}" TMP REGEX "^#define OSL_LIBRARY_VERSION_MINOR .*$")
        string (REGEX MATCHALL "[0-9]+" OSL_VERSION_MINOR ${TMP})
        file (STRINGS "${OSL_VERSION_HEADER}" TMP REGEX "^#define OSL_LIBRARY_VERSION_PATCH .*$")
        string (REGEX MATCHALL "[0-9]+" OSL_VERSION_PATCH ${TMP})
        file (STRINGS "${OSL_VERSION_HEADER}" TMP REGEX "^#define OSL_LIBRARY_VERSION_TWEAK .*$")
        if (TMP)
            string (REGEX MATCHALL "[0-9]+" OSL_VERSION_TWEAK ${TMP})
        else ()
            set (OSL_VERSION_TWEAK 0)
        endif ()
        set (OpenShadingLanguage_VERSION "${OSL_VERSION_MAJOR}.${OSL_VERSION_MINOR}.${OSL_VERSION_PATCH}.${OSL_VERSION_TWEAK}")

        # Find the oslcomp library.
        find_library(oslcomp_LIBRARY
            NAMES
                liboslcomp oslcomp
            HINTS
                ${OSL_ROOT}
            PATH_SUFFIXES
                lib
        )

        add_library(OSL::oslcomp SHARED IMPORTED)
        set_target_properties(OSL::oslcomp PROPERTIES 
            IMPORTED_LOCATION ${oslcomp_LIBRARY}
        )

        # Find the oslexec library.
        find_library(oslexec_LIBRARY
            NAMES
                liboslexec oslexec
            HINTS
                ${OSL_ROOT}
            PATH_SUFFIXES
                lib
        )

        add_library(OSL::oslexec SHARED IMPORTED)
        set_target_properties(OSL::oslexec PROPERTIES
            IMPORTED_LOCATION ${oslexec_LIBRARY}
        )

        set(OSL_SHADERS_INCLUDE_DIR ${OSL_ROOT}/share)

        # Variable used by the OSL unit tests. 
        set(OSL_SHADERS_DIR ${OSL_SHADERS_INCLUDE_DIR}/OSL/shaders)

        if(EXISTS "${OSL_SHADERS_DIR}")

            # Variable used by the OSL unit tests.
            set(OSL_FOUND ON)

        endif()

    endif()

    include (FindPackageHandleStandardArgs)
    find_package_handle_standard_args (OpenShadingLanguage
        FOUND_VAR     OpenShadingLanguage_FOUND
        REQUIRED_VARS OSL_INCLUDE_DIR oslcomp_LIBRARY oslexec_LIBRARY OpenShadingLanguage_VERSION
        VERSION_VAR   OpenShadingLanguage_VERSION
    )

    set(OSL_FOUND ${OpenShadingLanguage_FOUND})

endif()

###############################################################################
### Check the C++ version ###

# TODO: Which version starts to impose C++14?

if(${CMAKE_CXX_STANDARD} LESS_EQUAL 11)
    set(OSL_FOUND OFF)
    message(WARNING "Need C++14 or higher to compile OpenShadingLanguage. Skipping build of the OSL unit tests")
endif()

###############################################################################
### Configure target ###

if(OSL_FOUND)

    if (NOT OSL_FIND_QUIETLY)
        message(STATUS "OpenShadingLanguage includes    = ${OSL_INCLUDE_DIR}")
        message(STATUS "OpenShadingLanguage shaders     = ${OSL_SHADERS_DIR}")
        message(STATUS "OpenShadingLanguage library dir = ${OSL_LIB_DIR}")
    endif ()

    list(APPEND LIB_INCLUDE_DIRS ${OSL_INCLUDE_DIR})
    list(APPEND LIB_INCLUDE_DIRS ${OSL_SHADERS_INCLUDE_DIR})

    target_include_directories(osl::osl INTERFACE "${LIB_INCLUDE_DIRS}")
    target_link_libraries(osl::osl INTERFACE OSL::oslcomp OSL::oslexec)

    if (${OpenShadingLanguage_VERSION} VERSION_GREATER_EQUAL "1.12" AND ${CMAKE_CXX_STANDARD} LESS_EQUAL 11)
        set(OSL_FOUND OFF)
        message(WARNING "Need C++14 or higher to compile OpenShadingLanguage. Skipping build the OSL unit tests")
    endif()

    mark_as_advanced(OSL_INCLUDE_DIR
        oslcomp_LIBRARY oslcomp_FOUND
        oslexec_LIBRARY oslexec_FOUND
    )

endif()
