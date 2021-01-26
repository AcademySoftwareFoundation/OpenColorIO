# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate a Python package
#
# Variables defined by this module:
#   <package>_FOUND
#
# Usage:
#   find_python_package(Jinja2 2.10.1)
#
# If the package is not installed in a standard path, add it to the PYTHONPATH 
# environment variable before running CMake.
#

find_package(Python QUIET COMPONENTS Interpreter)

macro(find_python_package package)
    # Some Python packages have "-" in the PyPi name, but are replaced with "_" 
    # in the actual package name.
    string(REPLACE "-" "_" _PKG_IMPORT ${package})

    # Parse options
    foreach(_ARG ${ARGN})
        if(_ARG STREQUAL "REQUIRED")
            set(_PKG_REQUIRED TRUE)
        endif()
        if(_PREV_ARG STREQUAL "REQUIRES")
            set(_PKG_REQUIRES ${_ARG})
        endif()
        set(_PREV_ARG ${_ARG})
    endforeach()
    
    if(NOT TARGET ${package})
        add_custom_target(${package})
        if(_PKG_REQUIRES)
            add_dependencies(${package} ${_PKG_REQUIRES})
            unset(_PKG_REQUIRES)
        endif()
        set(_${package}_TARGET_CREATE TRUE)
    endif()

    set(_PKG_INSTALL TRUE)

    # Try importing Python package
    execute_process(
        COMMAND
            "${Python_EXECUTABLE}" -c "import ${_PKG_IMPORT}"
        RESULT_VARIABLE
            _PKG_IMPORT_RESULT
        OUTPUT_QUIET ERROR_QUIET
    )

    if(_PKG_IMPORT_RESULT EQUAL 0)
        set(${package}_FOUND TRUE)

        # Get the package's location
        execute_process(
            COMMAND
                "${Python_EXECUTABLE}" -c 
                "import ${_PKG_IMPORT}, os; print(os.path.dirname(${_PKG_IMPORT}.__file__))"
            OUTPUT_VARIABLE
                _PKG_DIR
            ERROR_QUIET
        )
        string(STRIP "${_PKG_DIR}" _PKG_DIR)
        message(STATUS "Found ${package}: ${_PKG_DIR}")

    else()
        set(${package}_FOUND FALSE)
        set(_FIND_ERR "Could NOT find ${package} (import ${_PKG_IMPORT} failed)")

        if(_PKG_REQUIRED)
            message(FATAL_ERROR "${_FIND_ERR}")
        endif()
        message(STATUS "${_FIND_ERR}")
    endif()

endmacro()
