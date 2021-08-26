# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install OpenShadingLanguage (OSL)
#
# If the library is not installed in a standard path, you can use the OSL_ROOT 
# variable to tell CMake where to find it. 
#

# TODO: OSL: Use "find_package(OSL 1.11 CONFIG)" directly instead of this file!


if(NOT TARGET osl::osl)
    add_library(osl::osl UNKNOWN IMPORTED GLOBAL)
    set(OSL_FOUND OFF)
endif()

###############################################################################
### Try to find package ###

if(DEFINED OSL_ROOT)

    set(OSL_INCLUDE_DIR ${OSL_ROOT}/include)

    if(EXISTS "${OSL_INCLUDE_DIR}/OSL/oslversion.h")

        # Find the oslcomp library.
        find_library(oslcomp_LIBRARY
            NAMES
                liboslcomp oslcomp
            HINTS
                ${OSL_ROOT}
            PATH_SUFFIXES
                lib
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

        # Variable used by the OSL unit tests. 
        set(OSL_SHADERS_DIR ${OSL_ROOT}/share/OSL/shaders)

        if(EXISTS "${OSL_SHADERS_DIR}")

            # Variable used by the OSL unit tests.
            set(OSL_FOUND ON)

        endif()

    endif()

endif()

###############################################################################
### Configure target ###

if(OSL_FOUND)

    if (NOT OSL_FIND_QUIETLY)
        message(STATUS "OpenShadingLanguage includes        = ${OSL_INCLUDE_DIR}")
        message(STATUS "OpenShadingLanguage oslcomp library = ${oslcomp_LIBRARY}")
        message(STATUS "OpenShadingLanguage oslexec library = ${oslexec_LIBRARY}")
    endif ()

    set_target_properties(osl::osl PROPERTIES
        IMPORTED_LOCATION ${oslcomp_LIBRARY}
        IMPORTED_LOCATION ${oslexec_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${OSL_INCLUDE_DIR}
    )

    mark_as_advanced(OSL_INCLUDE_DIR
        oslcomp_LIBRARY oslcomp_FOUND
        oslexec_LIBRARY oslexec_FOUND
    )

endif()
