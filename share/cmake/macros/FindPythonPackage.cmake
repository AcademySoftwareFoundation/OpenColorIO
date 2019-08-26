# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install a Python package in PyPi
#
# Variables defined by this module:
#   <PACKAGE>_FOUND
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

find_package(PythonInterp 2.7 QUIET)

macro(find_python_package package version)
    string(TOUPPER ${package} _PKG_UPPER)
    string(TOLOWER ${package} _PKG_LOWER)

    # Parse options
    foreach(_ARG ${ARGN})
        if(_ARG STREQUAL "REQUIRED")
            set(_PKG_REQUIRED TRUE)
        endif()
    endforeach()

    if(NOT TARGET ${package})
        add_custom_target(${package})
        set(_${_PKG_UPPER}_TARGET_CREATE TRUE)
    endif()

    set(_PKG_INSTALL TRUE)

    ###########################################################################
    ### Try to find package ###

    if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
        # Try importing Python package
        execute_process(
            COMMAND
                "${PYTHON_EXECUTABLE}" -c "import ${_PKG_LOWER}"
            RESULT_VARIABLE
                _PKG_IMPORT_RESULT
            OUTPUT_QUIET ERROR_QUIET
        )

        if(_PKG_IMPORT_RESULT EQUAL 0)
            set(${_PKG_UPPER}_FOUND TRUE)
            set(_PKG_INSTALL FALSE)

            # Get the package's location
            execute_process(
                COMMAND
                    "${PYTHON_EXECUTABLE}" -c 
                    "import ${_PKG_LOWER}, os; print(os.path.dirname(${_PKG_LOWER}.__file__))"
                OUTPUT_VARIABLE
                    _PKG_DIR
                ERROR_QUIET
            )
            string(STRIP "${_PKG_DIR}" _PKG_DIR)
            message(STATUS "Found ${package}: ${_PKG_DIR}")

        else()
            set(${_PKG_UPPER}_FOUND FALSE)
            set(_FIND_ERR "Could NOT find ${package}: import ${_PKG_LOWER} failed")
            if(OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
                set(_PKG_INSTALL FALSE)
                if(_PKG_REQUIRED)
                    message(FATAL_ERROR "${_FIND_ERR}")
                endif()
            endif()
            message(WARNING "${_FIND_ERR}")
        endif()
    endif()

    ###########################################################################
    ### Install package from PyPi ###

    if(_PKG_INSTALL)
        set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
        set(${_PKG_UPPER}_FOUND TRUE)

        if(_${_PKG_UPPER}_TARGET_CREATE)
            # Package install location
            if(WIN32)
                set(_SITE_PKGS_DIR "${_EXT_DIST_ROOT}/lib${LIB_SUFFIX}/site-packages")
                # On Windows platform, pip is in the Scripts sub-directory.
                get_filename_component(PYTHON_ROOT "${PYTHON_EXECUTABLE}" DIRECTORY)
                set(_PYTHON_PIP "${PYTHON_ROOT}/Scripts/pip.exe")
            else()
                set(_PYTHON_VARIANT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
                set(_SITE_PKGS_DIR
                    "${_EXT_DIST_ROOT}/lib${LIB_SUFFIX}/python${_PYTHON_VARIANT}/site-packages")
                set(_PYTHON_PIP "pip")
            endif()

            # Configure install target
            add_custom_command(
                TARGET
                    ${package}
                COMMAND
                    ${_PYTHON_PIP} install --disable-pip-version-check
                                           --install-option="--prefix=${_EXT_DIST_ROOT}"
                                           -I ${package}==${version}
                WORKING_DIRECTORY
                    "${CMAKE_BINARY_DIR}"
            )

            message(STATUS "Installing ${package}: ${_SITE_PKGS_DIR} (version ${version})")
        endif()
    endif()

endmacro()
