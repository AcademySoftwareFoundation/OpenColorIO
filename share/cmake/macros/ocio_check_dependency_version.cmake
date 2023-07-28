# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

###################################################################################################
# ocio_check_dependency_version try to find the specified dependency and validate the version.
#
# Note that a function is used here to scoped-in any variables set by find_package. We do not want
# those variables to be propagated to the caller of the function.
#
# Argument:
#   dep_name is the name of the dependency (package). Please note that dep_name is case sensitive.
#
###################################################################################################

function (ocio_check_dependency_version dep_name output)
    cmake_parse_arguments(
        # prefix - Must be different than the one used in ocio_handle_dependency.cmake.
        ocio_cdv
        # options
        ""
        # one value keywords
        "MIN_VERSION"
        # multi value keywords
        ""
        # args
        ${ARGN})
        
    if (dep_name)
        find_package(${dep_name} ${ocio_cdv_UNPARSED_ARGUMENTS})
        if (ocio_cdv_MIN_VERSION AND ${dep_name}_VERSION)
            if (${${dep_name}_VERSION} VERSION_GREATER_EQUAL ocio_cdv_MIN_VERSION)
                set(${output} TRUE PARENT_SCOPE)
            else()
                set(${output} FALSE PARENT_SCOPE)
            endif()
        endif()
    endif()
endfunction()