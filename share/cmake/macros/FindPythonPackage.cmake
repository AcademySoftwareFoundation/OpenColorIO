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

find_package(Python QUIET COMPONENTS Interpreter)

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
                "${Python_EXECUTABLE}" -c "import ${_PKG_LOWER}"
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
                    "${Python_EXECUTABLE}" -c 
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

    # Override PYTHONPATH prior to installing via pip. This allows other
    # pip installed packages to depend on each other, and makes the
    # PyOpenColorIO build available, prioritizes ext/ installed Python
    # dependencies, and propagates the system PYTHONPATH.
    if(WIN32)
        # Use Windows path separators since this is being passed through to cmd
        file(TO_NATIVE_PATH ${CMAKE_BINARY_DIR} WIN_BINARY_DIR)

        set(DLL_PATH "${WIN_BINARY_DIR}\\src\\OpenColorIO")
        if(MSVC_IDE)
            set(DLL_PATH "${DLL_PATH}\\${CMAKE_BUILD_TYPE}")
        endif()

        string(CONCAT PATH_SET
            "PATH="
            "${DLL_PATH}"
            "\\\\;"
            "%PATH%"
        )

        set(PYD_PATH "${WIN_BINARY_DIR}\\src\\bindings\\python")
        if(MSVC_IDE)
            set(PYD_PATH "${PYD_PATH}\\${CMAKE_BUILD_TYPE}")
        endif()

        string(CONCAT PYTHONPATH_SET
            "PYTHONPATH="
            "${PYD_PATH}"
            "\\\\;"
            "${WIN_BINARY_DIR}\\ext\\dist\\lib\\site-packages"
            "\\\\;"
            "%PYTHONPATH%"
        )
        # Mimic *nix:
        #   '> PYTHONPATH=XXX CMD'
        # on Windows with:
        #   '> set PYTHONPATH=XXX \n call CMD'
        # '\n' is here because '\\&' does not work.
        set(PYT_PRE_CMD set ${PATH_SET} "\n" set ${PYTHONPATH_SET} "\n" call)

    else()
        set(_Python_VARIANT "${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
        string(CONCAT PYT_PRE_CMD
            "PYTHONPATH="
            "${CMAKE_BINARY_DIR}/src/bindings/python"
            ":"
            "${CMAKE_BINARY_DIR}/ext/dist/lib${LIB_SUFFIX}/python${_Python_VARIANT}/site-packages"
            ":"
            "$ENV{PYTHONPATH}"
        )
    endif()

    if(_PKG_INSTALL)
        set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
        set(${_PKG_UPPER}_FOUND TRUE)

        if(_${_PKG_UPPER}_TARGET_CREATE)
            # Package install location
            if(WIN32)
                set(_SITE_PKGS_DIR "${_EXT_DIST_ROOT}/lib${LIB_SUFFIX}/site-packages")
                # On Windows platform, pip is in the Scripts sub-directory.
                set(_PYTHON_PIP "${PYTHON_ROOT}/Scripts/pip.exe")
            else()
                set(_Python_VARIANT "${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
                set(_SITE_PKGS_DIR
                    "${_EXT_DIST_ROOT}/lib${LIB_SUFFIX}/python${_Python_VARIANT}/site-packages")
                set(_Python_PIP "pip")
            endif()

            # Configure install target
            add_custom_command(
                TARGET
                    ${package}
                COMMAND
                    ${PYT_PRE_CMD} ${_Python_PIP} install --disable-pip-version-check
                                           --install-option="--prefix=${_EXT_DIST_ROOT}"
                                           -I ${package}==${version}
                WORKING_DIRECTORY
                    "${CMAKE_BINARY_DIR}"
            )

            message(STATUS "Installing ${package}: ${_SITE_PKGS_DIR} (version ${version})")
        endif()
    endif()

endmacro()
