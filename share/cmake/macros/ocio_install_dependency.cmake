# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

###################################################################################################
# ocio_install_dependency installs a dependency by calling the corresponding Install module.
# e.g. Install<dep_name>.cmake
#
# Argument:
#   dep_name is the name of the dependency (package). Please note that dep_name is case sensitive.
#
# Options (one value):
#   VERSION                     - Version to install.
###################################################################################################
macro (ocio_install_dependency dep_name)
    cmake_parse_arguments(
        # prefix - Must be different than the one used in ocio_handle_dependency.cmake.
        ocio_id
        # options
        ""
        # one value keywords
        "VERSION;PROMOTE_TARGET"
        # multi value keywords
        ""
        # args
        ${ARGN})


    if(NOT ${dep_name}_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
        set(OCIO_${dep_name}_RECOMMENDED_VERSION ${ocio_id_VERSION})
        include(Install${dep_name})
        set(_${dep_name}_built_by_ocio TRUE)
    endif()

    if(${dep_name}_FOUND)
        if(${dep_name}_FOUND AND ocio_id_PROMOTE_TARGET)
            foreach (_target_to_be_promoted_ ${ocio_id_PROMOTE_TARGET})
                set_target_properties(${_target_to_be_promoted_} PROPERTIES IMPORTED_GLOBAL TRUE)
            endforeach()
        endif()

        if (${_${dep_name}_built_by_ocio})
           message(STATUS "${ColorSuccess}Installed ${dep_name} (version \"${ocio_id_VERSION}\")${ColorReset}")
        endif()
    endif()
    unset(_${dep_name}_built_by_ocio)
endmacro()