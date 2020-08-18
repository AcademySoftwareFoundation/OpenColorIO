# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install a Python package in PyPi
#
# Variables defined by this module:
#   <package>_FOUND
#
# Targets defined by this module:
#   <package> - custom pip target, if package can be installed
#
# Usage:
#   find_python_package(Jinja2 2.10.1)
#
# If the package is not installed in a standard path, add it to the PYTHONPATH 
# environment variable before running CMake. If it is not found and 
# OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, the package will be 
# installed via pip at build time.
#

find_package(Python QUIET COMPONENTS Interpreter)

macro(find_python_package package version)
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

    ###########################################################################
    ### Try to find package ###

    if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
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
            set(_PKG_INSTALL FALSE)

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
            if(OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
                set(_PKG_INSTALL FALSE)
                if(_PKG_REQUIRED)
                    message(FATAL_ERROR "${_FIND_ERR}")
                endif()
            endif()
            message(STATUS "${_FIND_ERR}")
        endif()
    endif()

    ###########################################################################
    ### Install package from PyPi ###

    if(_PKG_INSTALL)
        set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
        set(${package}_FOUND TRUE)

        if(_${package}_TARGET_CREATE)
            # Package install location
            if(WIN32)
                set(_SITE_PKGS_DIR "${_EXT_DIST_ROOT}/lib${LIB_SUFFIX}/site-packages")
                # --prefix value needs OS-native path separator
                string(REPLACE "/" "\\" _PIP_PREFIX ${_EXT_DIST_ROOT})
            else()
                set(_Python_VARIANT "${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
                set(_SITE_PKGS_DIR
                    "${_EXT_DIST_ROOT}/lib${LIB_SUFFIX}/python${_Python_VARIANT}/site-packages")
                set(_PIP_PREFIX "${_EXT_DIST_ROOT}")
            endif()

            # Configure install target
            add_custom_command(
                TARGET
                    ${package}
                COMMAND
                    "${Python_EXECUTABLE}" -m pip install  
                                           --disable-pip-version-check
                                           --prefix="${_PIP_PREFIX}"
                                           -I ${package}==${version}
                WORKING_DIRECTORY
                    "${CMAKE_BINARY_DIR}"
            )
            message(STATUS "Installing ${package}: ${_SITE_PKGS_DIR} (version ${version})")
        endif()
    endif()

endmacro()
