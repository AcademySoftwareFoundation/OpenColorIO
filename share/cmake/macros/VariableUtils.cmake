# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# General CMake utility macros.
#

macro(set_unless_defined var_name var_default)
    if(NOT DEFINED ${var_name})
        set(${var_name} ${var_default})
    else()
        message(STATUS "Variable explicitly defined: ${var_name} = ${${var_name}}")
    endif()
endmacro()
