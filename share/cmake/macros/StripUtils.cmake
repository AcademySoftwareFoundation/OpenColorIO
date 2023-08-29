# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# This function is based on Pybind11's pybind11_strip.

macro(ocio_strip_binary target_name)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" _uppercase_CMAKE_BUILD_TYPE)
    if(NOT MSVC AND NOT "${_uppercase_CMAKE_BUILD_TYPE}" MATCHES DEBUG|RELWITHDEBINFO)
        if(CMAKE_STRIP)
            if(APPLE)
                set(_strip_opt -x)
            endif()

            add_custom_command(
                TARGET ${target_name}
                POST_BUILD
                COMMAND ${CMAKE_STRIP} ${_strip_opt} $<TARGET_FILE:${target_name}>
            )
            unset(_strip_opt)
        endif()
    endif()
    unset(_uppercase_CMAKE_BUILD_TYPE)
endmacro()
