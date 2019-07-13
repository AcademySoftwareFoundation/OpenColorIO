# Find an external package installed on this system, or set the variables
# needed to install it.
#
# Parameters:
#   package - Package name matching Find<Package>.cmake module
#   min_version - Minimum allowed (and default) version
#
# Variables always defined by this macro:
#   <PACKAGE>_FOUND - Whether package already exists on system
#   <PACKAGE>_INSTALL_EXT - Whether to proceed with ext install
#
# Variables defined when <PACKAGE>_INSTALL_EXT is TRUE and [min_version] is 
# provided:
#   <PACKAGE>_VERSION
#   <PACKAGE>_VERSION_U - <major>_<minor>_<patch>
#   <PACKAGE>_VERSION_MAJOR
#   <PACKAGE>_VERSION_MINOR
#   <PACKAGE>_VERSION_PATCH
#
# Variables defined when <PACKAGE>_FOUND is TRUE:
#   <PACKAGE>_VERSION
#   <PACKAGE>_INCLUDE_DIR
#   <PACKAGE>_LIBRARY
#
# Usage:
#   find_ext_package(package [min_version])
#   find_ext_package(IlmBase 2.3.0)
#
# Pre-defined variables that affect macro behavior:
#   OCIO_INSTALL_EXT - Determines install requirements
#   GET_<PACKAGE>_VERSION - Request a specific version when install is needed
#

macro(find_ext_package package)
    string(TOUPPER ${package} _PKG_UPPER)

    # Parse optional args
    set(_EXTRA_ARGS ${ARGN})
    foreach(_ARG _EXTRA_ARGS)
        if(_ARG MATCHES "^([0-9]+)\\.")
            set(${_PKG_UPPER}_MIN_VERSION ${_ARG})
        endif()
    endforeach()

    if(OCIO_INSTALL_EXT STREQUAL ALL)
        # Install is requested regardless of package's current presence on 
        # the system.
        set(${_PKG_UPPER}_INSTALL_EXT TRUE)
    else()
        # Forward only package and [version] to find_package. REQUIRED will be 
        # determined by OCIO_INSTALL_EXT since this is an ext/ package.
        set(_FIND_PKG_ARGS ${package})
        if(${package}_MIN_VERSION)
            list(APPEND _FIND_PKG_ARGS ${${package}_MIN_VERSION})
        endif()
        find_package(${_FIND_PKG_ARGS})

        # Unify output variable casing
        if(${package}_FOUND)
            set(${_PKG_UPPER}_FOUND ${${package}_FOUND})
        endif()
        if(${package}_VERSION)
            set(${_PKG_UPPER}_VERSION ${${package}_VERSION})
        endif()

        if(${_PKG_UPPER}_FOUND)
            # If version is set, make sure it's high enough
            if(${_PKG_UPPER}_VERSION AND ${_PKG_UPPER}_MIN_VERSION
                    AND NOT ${_PKG_UPPER}_VERSION VERSION_GREATER_EQUAL ${_PKG_UPPER}_MIN_VERSION)
                if(OCIO_INSTALL_EXT STREQUAL NONE)
                    message(FATAL_ERROR
                        "Required package ${package} ${${_PKG_UPPER}_VERSION} was found, "
                        "but is older than the minimum required version "
                        "${${_PKG_UPPER}_MIN_VERSION}!\n"
                        "Use -DOCIO_INSTALL_EXT=<MISSING|ALL> to install the minimum version at "
                        "build time (optionally requesting a newer version with "
                        "-DGET_${_PKG_UPPER}_VERSION=<version>)."
                    )
                else()
                    # OCIO_INSTALL_EXT is MISSING. Enable install of newer version.
                    set(${_PKG_UPPER}_INSTALL_EXT TRUE)
                endif()
            else()
                # Package is available. No need to install.
                set(${_PKG_UPPER}_INSTALL_EXT FALSE)

                # Unify C/C++ output variable naming, which can vary. These are
                # not defined for pckages requiring an install. The install code
                # should define them explicitely.
                if(${_PKG_UPPER}_LIBRARIES)
                    set(${_PKG_UPPER}_LIBRARY ${${_PKG_UPPER}_LIBRARIES})
                endif()
                if(${_PKG_UPPER}_INCLUDE_DIRS)
                    set(${_PKG_UPPER}_INCLUDE_DIR ${${_PKG_UPPER}_INCLUDE_DIRS})
                endif()
            endif()
        else()
            if(OCIO_INSTALL_EXT STREQUAL NONE)
                message(FATAL_ERROR
                    "Required package ${package} not found!\n"
                    "Use -DOCIO_INSTALL_EXT=<MISSING|ALL> to install it at build time."
                )
            else()
                # OCIO_INSTALL_EXT is MISSING. Enable install.
                set(${_PKG_UPPER}_INSTALL_EXT TRUE)
            endif()
        endif()
    endif()

    # Install is needed. 
    if(${_PKG_UPPER}_INSTALL_EXT AND ${_PKG_UPPER}_MIN_VERSION)
        set(${_PKG_UPPER}_VERSION ${${_PKG_UPPER}_MIN_VERSION})

        # Make sure any request for a specific version is high enough
        if(GET_${_PKG_UPPER}_VERSION)
            if(NOT GET_${_PKG_UPPER}_VERSION VERSION_GREATER_EQUAL ${_PKG_UPPER}_MIN_VERSION)
                message(WARNING 
                    "The requested ${package} version ${GET_${_PKG_UPPER}_VERSION} is older "
                    "than the minimum required version!\n"
                    "Version ${${_PKG_UPPER}_MIN_VERSION} will be installed instead."
                )
            else()
                set(${_PKG_UPPER}_VERSION ${GET_${_PKG_UPPER}_VERSION})
            endif()
        endif()

        # Set a variety of useful *_VERSION_* variables for outside consumption
        # <major>.<minor>.<patch> --> <major>_<minor>_<patch>
        string(REPLACE "." "_" ${_PKG_UPPER}_VERSION_U ${${_PKG_UPPER}_VERSION})

        # Split version into major, minor, and patch. Default to 0 for 
        # unspecified components.
        string(REPLACE "." ";" _PKG_VERSION_LIST ${${_PKG_UPPER}_VERSION})
        list(LENGTH _PKG_VERSION_LIST _PKG_VERSION_COMPS)
        list(GET _PKG_VERSION_LIST 0 ${_PKG_UPPER}_VERSION_MAJOR)
        if(_PKG_VERSION_COMPS GREATER 1)
            list(GET _PKG_VERSION_LIST 1 ${_PKG_UPPER}_VERSION_MINOR)
        else()
            set(${_PKG_UPPER}_VERSION_MINOR 0)
        endif()
        if(_PKG_VERSION_COMPS GREATER 2)
            list(GET _PKG_VERSION_LIST 2 ${_PKG_UPPER}_VERSION_PATCH)
        else()
            set(${_PKG_UPPER}_VERSION_PATCH 0)
        endif()
    endif()

endmacro()
