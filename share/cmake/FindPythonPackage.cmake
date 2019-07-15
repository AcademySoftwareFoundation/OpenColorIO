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

    add_custom_target(${package})
    set(${_PKG_UPPER}_FOUND FALSE)
    set(INSTALL_EXT_${_PKG_UPPER} TRUE)

    ###########################################################################
    ### Try to find package ###

    if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
        execute_process(
            COMMAND
                "${PYTHON_EXECUTABLE}" -c "import ${_PKG_LOWER}"
            RESULT_VARIABLE
                _PKG_IMPORT_RESULT
        )

        if(_PKG_IMPORT_RESULT EQUAL 0)
            set(${_PKG_UPPER}_FOUND TRUE)
            set(INSTALL_EXT_${_PKG_UPPER} FALSE)
        else()
            if(OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
                message(FATAL_ERROR
                    "Required package ${package} not found!\n"
                    "Use -DOCIO_INSTALL_EXT_PACKAGES=<MISSING|ALL> to install it, "
                    "or add its location to PYTHONPATH so it can be found."
                )
            endif()
        endif()
    endif()

    ###########################################################################
    ### Install package from PyPi ###

    if(INSTALL_EXT_${_PKG_UPPER})
        set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
        set(${_PKG_UPPER}_FOUND TRUE)

        # Package install location
        if(WIN32)
            set(_SITE_PKGS_DIR "${_EXT_DIST_ROOT}/lib${LIB_SUFFIX}/site-packages")
        else()
            set(_PYTHON_VARIANT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
            set(_SITE_PKGS_DIR 
                "${_EXT_DIST_ROOT}/lib${LIB_SUFFIX}/python${_PYTHON_VARIANT}/site-packages")
        endif()

        # Configure install target
        add_custom_command(
            TARGET
                ${package}
            COMMAND
                pip install --quiet 
                            --disable-pip-version-check
                            --install-option="--prefix=${_EXT_DIST_ROOT}"
                            -I ${package}==${version}
            WORKING_DIRECTORY
                "${CMAKE_BINARY_DIR}"
        )

        message(STATUS "Installing ${package}: ${_SITE_PKGS_DIR} (version ${version})")
    endif()

endmacro()
