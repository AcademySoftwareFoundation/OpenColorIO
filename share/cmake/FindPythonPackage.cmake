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

macro(find_python_package)
    cmake_parse_arguments(_FIND "REQUIRED" "" "" ${ARGN})
    list(LENGTH ${ARGN} ARGN_COUNT)
    if(ARGN_COUNT GREATER 0)
        list(GET ${ARGN} 0 package)
    endif()
    if(ARGN_COUNT GREATER 1)
        list(GET ${ARGN} 1 version)
    endif()

    message(STATUS "${_FIND_PKG}")
    string(TOUPPER ${_FIND_PKG} _FIND_PKG_UPPER)
    string(TOLOWER ${_FIND_PKG} _FIND_PKG_LOWER)

    add_custom_target(${_FIND_PKG})
    set(_${_FIND_PKG_UPPER}_INSTALL TRUE)

    ###########################################################################
    ### Try to find package ###

    if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
        execute_process(
            COMMAND
                "${PYTHON_EXECUTABLE}" -c "import ${_FIND_PKG_LOWER}"
            RESULT_VARIABLE
                _PKG_IMPORT_RESULT
        )

        if(_PKG_IMPORT_RESULT EQUAL 0)
            set(${_FIND_PKG_UPPER}_FOUND TRUE)
            set(_${_FIND_PKG_UPPER}_INSTALL FALSE)
        else()
            set(${_FIND_PKG_UPPER}_FOUND FALSE)
            set(_FIND_ERR "Could NOT find ${_FIND_PKG}: import ${_FIND_PKG_LOWER} failed")
            if(OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
                set(_${_FIND_PKG_UPPER}_INSTALL FALSE)
                if(_FIND_REQUIRED)
                    message(FATAL_ERROR "${_FIND_ERR}")
                endif()
            endif()
            message(WARNING "${_FIND_ERR}")
        endif()
    endif()

    ###########################################################################
    ### Install package from PyPi ###

    if(${_FIND_PKG_UPPER}_FOUND)
        set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
        set(${_FIND_PKG_UPPER}_FOUND TRUE)

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
                ${_FIND_PKG}
            COMMAND
                pip install --quiet 
                            --disable-pip-version-check
                            --install-option="--prefix=${_EXT_DIST_ROOT}"
                            -I ${_FIND_PKG}==${_FIND_VERSION}
            WORKING_DIRECTORY
                "${CMAKE_BINARY_DIR}"
        )

        message(STATUS "Installing ${_FIND_PKG}: ${_SITE_PKGS_DIR} (version ${_FIND_VERSION})")
    endif()

endmacro()
