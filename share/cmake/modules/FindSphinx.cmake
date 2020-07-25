# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install Sphinx (Python documentation generator)
#
# Variables defined by this module:
#   Sphinx_FOUND
#   Sphinx_EXECUTABLE (CACHE)
#
# Targets defined by this module:
#   Sphinx - custom pip target, if package can be installed
#
# Usage:
#   find_package(Sphinx)
#
# If Sphinx is not installed in a standard path, add it to the Sphinx_ROOT 
# variable to tell CMake where to find it. If it is not found and 
# OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, Sphinx will be 
# installed via pip at build time.
#

find_package(Python QUIET COMPONENTS Interpreter)

if(NOT TARGET Sphinx)
    add_custom_target(Sphinx)
    set(_Sphinx_TARGET_CREATE TRUE)
endif()

if(Python_Interpreter_FOUND AND WIN32)
    get_filename_component(_Python_ROOT "${Python_EXECUTABLE}" DIRECTORY)
    set(_Python_SCRIPTS_DIR "${_Python_ROOT}/Scripts")
endif()

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Find sphinx-build
    find_program(Sphinx_EXECUTABLE 
        NAMES 
            sphinx-build
        HINTS
            ${Sphinx_ROOT}
            ${_Python_SCRIPTS_DIR}
    )

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(Sphinx_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Sphinx
        REQUIRED_VARS 
            Sphinx_EXECUTABLE
    )
    set(Sphinx_FOUND ${Sphinx_FOUND})
endif()

###############################################################################
### Install package from PyPi ###

if(NOT Sphinx_FOUND)
    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")

    # Set find_package standard args
    set(Sphinx_FOUND TRUE)
    if(WIN32)
        set(Sphinx_EXECUTABLE "${_EXT_DIST_ROOT}/Scripts/sphinx-build")
        # On Windows platform, pip is in the Scripts sub-directory.
        set(_Python_PIP "${_Python_SCRIPTS_DIR}/pip.exe")
    else()
        set(Sphinx_EXECUTABLE "${_EXT_DIST_ROOT}/bin/sphinx-build")
        set(_Python_PIP "pip")
    endif()

    # Configure install target
    if(_Sphinx_TARGET_CREATE)
        add_custom_command(
            TARGET
                Sphinx
            COMMAND
                ${_Python_PIP} install --disable-pip-version-check
                                       --install-option="--prefix=${_EXT_DIST_ROOT}"
                                       -I Sphinx==${Sphinx_FIND_VERSION}
            WORKING_DIRECTORY
                "${CMAKE_BINARY_DIR}"
        )

        message(STATUS "Installing Sphinx: ${Sphinx_EXECUTABLE} (version \"${Sphinx_FIND_VERSION}\")")
    endif()
endif()

mark_as_advanced(Sphinx_EXECUTABLE)
