# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# General CMake utility macros.
#

macro(package_root_message package)
    if(NOT "${${package}_FOUND}")
        message(STATUS "Use \"${package}_ROOT\" to specify an install location")
    endif()
endmacro()
