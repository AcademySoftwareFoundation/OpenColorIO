# Locate or install Sphinx (Python documentation generator)
#
# Variables defined by this module:
#   SPHINX_FOUND
#   SPHINX_EXECUTABLE
#
# Targets defined by this module:
#   Sphinx - custom pip target, if package can be installed
#
# Usage:
#   find_package(Sphinx)
#
# If Sphinx is not installed in a standard path, add it to the SPHINX_DIRS 
# variable to tell CMake where to find it. If it is not found and 
# OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, Sphinx will be 
# installed via pip at build time.
#

find_package(PythonInterp 2.7 QUIET)

add_custom_target(Sphinx)
set(SPHINX_FOUND FALSE)
set(INSTALL_EXT_SPHINX TRUE)

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Find sphinx-build
    find_program(SPHINX_EXECUTABLE 
        NAMES 
            sphinx-build
        HINTS
            ${SPHINX_DIRS}
    )

    # Override REQUIRED, since it's determined by OCIO_INSTALL_EXT_PACKAGES 
    # instead.
    set(Sphinx_FIND_REQUIRED FALSE)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(SPHINX 
        REQUIRED_VARS 
            SPHINX_EXECUTABLE
    )

    if(SPHINX_FOUND)
        set(INSTALL_EXT_SPHINX FALSE)
    else()
        if(OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
            message(FATAL_ERROR
                "Required executable sphinx-build not found!\n"
                "Use -DOCIO_INSTALL_EXT_PACKAGES=<MISSING|ALL> to install it, "
                "or -DSPHINX_DIRS=<path> to hint at its location."
            )
        endif()
    endif()
endif()

###############################################################################
### Install package from PyPi ###

if(INSTALL_EXT_${_PKG_UPPER})
    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")

    # Set find_package standard args
    set(SPHINX_FOUND TRUE)
    set(SPHINX_EXECUTABLE "${_EXT_DIST_ROOT}/bin/sphinx-build")

    # Configure install target
    add_custom_command(
        TARGET
            Sphinx
        COMMAND
            pip install --quiet 
                        --disable-pip-version-check
                        --install-option="--prefix=${_EXT_DIST_ROOT}"
                        -I Sphinx==${Sphinx_FIND_VERSION}
        WORKING_DIRECTORY
            "${CMAKE_BINARY_DIR}"
    )

    message(STATUS "Installing Sphinx: ${SPHINX_EXECUTABLE} (version ${Sphinx_FIND_VERSION})")
endif()

mark_as_advanced(SPHINX_EXECUTABLE)
