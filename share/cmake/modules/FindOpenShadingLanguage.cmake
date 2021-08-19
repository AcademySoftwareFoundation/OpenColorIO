# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install OpenShadingLanguage (OSL)
#
# If the library is not installed in a standard path, you can use the OpenShadingLanguage_ROOT 
# variable to tell CMake where to find it. 
#

if(NOT TARGET osl::osl)
    add_library(osl::osl UNKNOWN IMPORTED GLOBAL)
    set(OSL_FOUND OFF)
endif()

###############################################################################
### Try to find package ###

if(DEFINED OpenShadingLanguage_ROOT)

    set(OpenShadingLanguage_INCLUDE_DIR ${OpenShadingLanguage_ROOT}/include)

    if(EXISTS "${OpenShadingLanguage_INCLUDE_DIR}/OSL/oslversion.h")

        # Find the oslcomp library.
        find_library(oslcomp_LIBRARY
            NAMES
                liboslcomp oslcomp
            PATHS
                ${OpenShadingLanguage_ROOT}
        )

        # Find the oslexec library.
        find_library(oslexec_LIBRARY
            NAMES
                liboslexec oslexec
            PATHS
                ${OpenShadingLanguage_ROOT}
        )

        # Variable used by the OSL unit tests. 
        set(OSL_SHADERS_DIR ${OpenShadingLanguage_ROOT}/share/OSL/shaders)

        if(EXISTS "${OSL_SHADERS_DIR}")

            # Variable used by the OSL unit tests.
            set(OSL_FOUND ON)

        endif()

    endif()

else()

    # TODO: OSL: Implement pkconfig support.

endif()

###############################################################################
### Configure target ###

if(OSL_FOUND)

    if (NOT OpenShadingLanguage_FIND_QUIETLY)
        message(STATUS "OpenShadingLanguage includes        = ${OpenShadingLanguage_INCLUDE_DIR}")
        message(STATUS "OpenShadingLanguage oslcomp library = ${oslcomp_LIBRARY}")
        message(STATUS "OpenShadingLanguage oslexec library = ${oslexec_LIBRARY}")
    endif ()

    set_target_properties(osl::osl PROPERTIES
        IMPORTED_LOCATION ${oslcomp_LIBRARY}
        IMPORTED_LOCATION ${oslexec_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${OpenShadingLanguage_INCLUDE_DIR}
    )

    mark_as_advanced(OpenShadingLanguage_INCLUDE_DIR
        oslcomp_LIBRARY oslcomp_FOUND
        oslexec_LIBRARY oslexec_FOUND
    )

endif()
