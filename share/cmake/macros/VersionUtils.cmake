# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# General CMake utility macros.
#

macro(split_version_string version_var output_prefix)
    string(REPLACE "." ";" _version_var_list ${version_var})
    list(LENGTH _version_var_list _version_var_list_length)

    if (_version_var_list_length GREATER_EQUAL 1)
        list(GET _version_var_list 0 ${output_prefix}_VERSION_MAJOR)
    endif()

    if (_version_var_list_length GREATER_EQUAL 2)
        list(GET _version_var_list 1 ${output_prefix}_VERSION_MINOR)
    endif()

    if (_version_var_list_length GREATER_EQUAL 3)
        list(GET _version_var_list 2 ${output_prefix}_VERSION_PATCH)
    endif()

    unset(_version_var_list)
endmacro()
