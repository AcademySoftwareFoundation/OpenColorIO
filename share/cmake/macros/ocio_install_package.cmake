# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# Install the specific dependency using OCIO's custom installXYZ module 
# where XYZ is the name of the dependency.
macro (ocio_install_package dep_name)
    cmake_parse_arguments(
        # prefix
        ocio_dep
        # options
        ""
        # one value keywords
        "VERSION"
        # multi value keywords
        ""
        # args
        ${ARGN})
    if(NOT ${dep_name}_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
        set(OCIO_${dep_name}_RECOMMENDED_VERSION ${ocio_dep_VERSION})
        include(Install${dep_name})
    endif()
endmacro()