# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# General CMake utility macros.
#

macro(split_version_string version_var output_prefix)
    string(REPLACE "." ";" _version_var_list ${version_var})
    list(GET _version_var_list 0 ${output_prefix}_VERSION_MAJOR)
    list(GET _version_var_list 1 ${output_prefix}_VERSION_MINOR)
    list(GET _version_var_list 2 ${output_prefix}_VERSION_PATCH)
    unset(_version_var_list)
endmacro()
