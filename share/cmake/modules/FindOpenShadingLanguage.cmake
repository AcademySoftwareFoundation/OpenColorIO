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

    find_package(OSL ${OpenShaderLanguage_VERSION} CONFIG)

    # TODO: No variable to have the share directory?

    set(OSL_SHADERS_INCLUDE_DIR ${OSL_INCLUDE_DIR}/../share)

    # Variable used by the OSL unit tests. 
    set(OSL_SHADERS_DIR ${OSL_SHADERS_INCLUDE_DIR}/OSL/shaders)

else()

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

endif()

###############################################################################
### Check the C++ version ###

# TODO: Which version starts to impose C++14?

if(${CMAKE_CXX_STANDARD} LESS_EQUAL 11)
    set(OSL_FOUND OFF)
    message(WARNING "Need C++14 or higher to compile OpenShadingLanguage. Skipping build the OSL unit tests")
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

    mark_as_advanced(OSL_INCLUDE_DIR
        oslcomp_LIBRARY oslcomp_FOUND
        oslexec_LIBRARY oslexec_FOUND
    )

endif()
