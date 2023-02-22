# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate pybind11
#
# Variables defined by this module:
#   pybind11_FOUND - If FALSE, do not try to link to pybind11
#   pybind11_INCLUDE_DIR - Where to find pybind11.h
#   pybind11_VERSION - The version of the library
#
# Global targets defined by this module:
#   pybind11::module
#
# Usually CMake will use the dynamic library rather than static, if both are present. 
#
# If the library is not installed in a typical location where CMake will find it, you may specify 
# the location using one of the following methods:
# -- Set -Dpybind11_DIR to point to the directory containing the CMake configuration file for the package.
# -- Set -Dpybind11_ROOT to point to the directory containing the lib and include directories.
# -- Set -Dpybind11_LIBRARY and -Dpybind11_INCLUDE_DIR to point to the lib and include directories.
#

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    if(NOT DEFINED pybind11_ROOT)
        # Search for pybind11Config.cmake
        find_package(pybind11 ${pybind11_FIND_VERSION} CONFIG QUIET)
    endif()

    if(NOT pybind11_FOUND)
        # Find include directory
        find_path(pybind11_INCLUDE_DIR
            NAMES
                pybind11/pybind11.h
            HINTS
                ${pybind11_ROOT}
            PATH_SUFFIXES
                include
                pybind11/include
        )

        # Version information can be extracted from this header
        set(_pybind11_COMMON_H "${pybind11_INCLUDE_DIR}/pybind11/detail/common.h")

        if(EXISTS "${_pybind11_COMMON_H}")
            file(STRINGS "${_pybind11_COMMON_H}" _pybind11_VER_SEARCH 
                REGEX "^[ \t]*#define[ \t]+PYBIND11_VERSION_(MAJOR|MINOR|PATCH)[ \t]+([0-9]+).*$")

            if(_pybind11_VER_SEARCH)
                string(REGEX REPLACE ".*MAJOR[ \t]+([0-9]+).*" 
                    "\\1" _pybind11_MAJOR "${_pybind11_VER_SEARCH}")
                string(REGEX REPLACE ".*MINOR[ \t]+([0-9]+).*" 
                    "\\1" _pybind11_MINOR "${_pybind11_VER_SEARCH}")
                string(REGEX REPLACE ".*PATCH[ \t]+([0-9]+).*" 
                    "\\1" _pybind11_PATCH "${_pybind11_VER_SEARCH}")

                set(pybind11_VERSION "${_pybind11_MAJOR}.${_pybind11_MINOR}.${_pybind11_PATCH}")
            endif()
        endif()
    endif()

    if(NOT pybind11_VERSION)
        # Check if pybind11 was installed as a Python package (i.e. with 
        # "pip install pybind11"). This requires a Python interpreter.
        find_package(Python QUIET COMPONENTS Interpreter)

        if(Python_Interpreter_FOUND)
            execute_process(
                COMMAND
                    "${Python_EXECUTABLE}" -c 
                    "print(__import__('pybind11').__version__)"
                RESULTS_VARIABLE
                    _pybind11_VER_RESULTS
                OUTPUT_VARIABLE
                    _pybind11_VER_OUTPUT
                ERROR_QUIET
            )
            if(_pybind11_VER_OUTPUT)
                # Strip \n from python output
                string(STRIP ${_pybind11_VER_OUTPUT} _pybind11_VER_OUTPUT)
            endif()

            if(_pybind11_VER_RESULTS EQUAL 0 AND "${_pybind11_VER_OUTPUT}" MATCHES "[.0-9]+")
                execute_process(
                    COMMAND
                        "${Python_EXECUTABLE}" -c 
                        "import os;\
                            import pybind11;\
                            print(os.path.join(os.path.dirname(pybind11.__file__), 'include'))"
                    RESULTS_VARIABLE
                        _pybind11_DIR_RESULTS
                    OUTPUT_VARIABLE
                        _pybind11_DIR_OUTPUT
                    ERROR_QUIET
                )
                if(_pybind11_DIR_OUTPUT)
                    # Strip \n from python output
                    string(STRIP ${_pybind11_DIR_OUTPUT} _pybind11_DIR_OUTPUT)
                endif()

                if(_pybind11_DIR_RESULTS EQUAL 0 AND EXISTS "${_pybind11_DIR_OUTPUT}")
                    set(pybind11_VERSION ${_pybind11_VER_OUTPUT})
                    set(pybind11_INCLUDE_DIR ${_pybind11_DIR_OUTPUT})
                    message(STATUS 
                        "Using pybind11 python package (version \"${pybind11_VERSION}\", "
                        "found in python \"${Python_VERSION}\")"
                    )
                endif()
            endif()
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(pybind11_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(pybind11
        REQUIRED_VARS 
            pybind11_INCLUDE_DIR
        VERSION_VAR
            pybind11_VERSION
    )
endif()

###############################################################################
### Create target ###

if(pybind11_FOUND AND NOT TARGET pybind11::module)
    add_library(pybind11::module INTERFACE IMPORTED GLOBAL)
    set(_pybind11_TARGET_CREATE TRUE)
endif()

###############################################################################
### Configure target ###

if(_pybind11_TARGET_CREATE)
    set_target_properties(pybind11::module PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${pybind11_INCLUDE_DIR}
    )

    mark_as_advanced(pybind11_INCLUDE_DIR pybind11_VERSION)
endif()
