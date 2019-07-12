# Find an external package installed on this system, or set the variables
# needed to install it.
#
# Parameters:
#   package - Package name matching Find<Package>.cmake module
#   min_version - Minimum allowed (and default) version
#
# Variables defined by this macro:
#   <PACKAGE>_FOUND - Whether package already exists on system
#   <PACKAGE>_INSTALL_EXT - Whether to proceed with ext install
#   <PACKAGE>_VERSION - <major>.<minor>.<patch>
#   <PACKAGE>_VERSION_U - <major>_<minor>_<patch>
#   <PACKAGE>_VERSION_MAJOR
#   <PACKAGE>_VERSION_MINOR
#   <PACKAGE>_VERSION_PATCH
#
# Variables defined ONLY if the package is already installed on the
# system (when <PACKAGE>_FOUND is TRUE):
#   <PACKAGE>_INCLUDE_DIRS
#   <PACKAGE>_LIBRARY
#
# Usage:
#   find_ext_package(IlmBase 2.3.0)
#
# Pre-defined variables that affect macro behavior:
#   OCIO_INSTALL_EXT - Determines install requirements
#   GET_<PACKAGE>_VERSION - Request a specific version when install is needed
#

macro(find_ext_package package min_version)
	string(TOUPPER ${package} _PACKAGE_UPPER)

    # Find package on system
	if(NOT OCIO_INSTALL_EXT STREQUAL ALL)
		find_package(${package} ${min_version})

        # Force variable consistency between find modules
		if(${_PACKAGE_UPPER}_FOUND OR ${package}_FOUND)
		    if(${package}_FOUND)
		        set(${_PACKAGE_UPPER}_FOUND ${${package}_FOUND})
            endif()
		    if(${_PACKAGE_UPPER}_LIBRARIES)
		        set(${_PACKAGE_UPPER}_LIBRARY ${${_PACKAGE_UPPER}_LIBRARIES})
            endif()
            if(${_PACKAGE_UPPER}_INCLUDE_DIR)
		        set(${_PACKAGE_UPPER}_INCLUDE_DIRS ${${_PACKAGE_UPPER}_INCLUDE_DIR})
		    endif()
        endif()
	else()
	    set(${_PACKAGE_UPPER}_FOUND FALSE)
	endif()

    # Provide a helpful failure message
	if(NOT ${_PACKAGE_UPPER}_FOUND AND OCIO_INSTALL_EXT STREQUAL NONE)
	    message(FATAL_ERROR
            "Dependent package ${package} not found! "
            "Use -DOCIO_INSTALL_EXT=<MISSING|ALL> to install it at build time."
        )
	endif()

    # Should the package be installed?
	if((NOT ${_PACKAGE_UPPER}_FOUND AND OCIO_INSTALL_EXT STREQUAL MISSING)
	        OR (OCIO_INSTALL_EXT STREQUAL ALL))
		set(${_PACKAGE_UPPER}_INSTALL_EXT TRUE)
    else()
        if(GET_${_PACKAGE_UPPER}_VERSION)
            message(WARNING "Minimum supported ${package} version is ${min_version}")
        endif()
		set(${_PACKAGE_UPPER}_INSTALL_EXT FALSE)
	endif()

    # Which version exists or needs to be installed?
    if(${_PACKAGE_UPPER}_INSTALL_EXT
            AND GET_${_PACKAGE_UPPER}_VERSION
            AND GET_${_PACKAGE_UPPER}_VERSION VERSION_GREATER_EQUAL ${min_version})
        set(${_PACKAGE_UPPER}_VERSION ${GET_${_PACKAGE_UPPER}_VERSION})
    else()
        set(${_PACKAGE_UPPER}_VERSION ${min_version})
    endif()

    # <major>.<minor>.<patch> --> <major>_<minor>_<patch>
    string(REPLACE "." "_" ${_PACKAGE_UPPER}_VERSION_U ${${_PACKAGE_UPPER}_VERSION})

    # Split version into major, minor, and patch
    string(REPLACE "." ";" _PACKAGE_VERSION_LIST ${${_PACKAGE_UPPER}_VERSION})
    list(LENGTH _PACKAGE_VERSION_LIST _PACKAGE_VERSION_COMPS)
    list(GET _PACKAGE_VERSION_LIST 0 ${_PACKAGE_UPPER}_VERSION_MAJOR)
    if(_PACKAGE_VERSION_COMPS GREATER 1)
        list(GET _PACKAGE_VERSION_LIST 1 ${_PACKAGE_UPPER}_VERSION_MINOR)
    else()
        set(${_PACKAGE_UPPER}_VERSION_MINOR 0)
    endif()
    if(_PACKAGE_VERSION_COMPS GREATER 2)
        list(GET _PACKAGE_VERSION_LIST 2 ${_PACKAGE_UPPER}_VERSION_PATCH)
    else()
        set(${_PACKAGE_UPPER}_VERSION_PATCH 0)
    endif()
endmacro()
