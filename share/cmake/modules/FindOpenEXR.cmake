# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate OpenEXR.
#
# Variables defined by this module:
#   OpenEXR_FOUND          - Indicate whether the library was found or not
#   OpenEXR_LIBRARY        - Path to the library file
#   OpenEXR_INCLUDE_DIR    - Location of the header files
#   OpenEXR_VERSION        - Library's version
#
# Imported targets defined by this module, if found:
#   OpenEXR::Iex
#   OpenEXR::IexConfig
#   OpenEXR::IlmThread
#   OpenEXR::IlmThreadConfig
#   OpenEXR::OpenEXR
#   OpenEXR::OpenEXRConfig
#   OpenEXR::OpenEXRCore
#   OpenEXR::OpenEXRUtil
#
# By default, the dynamic libraries will be found.
#
# If the library is not installed in a standard path, you can do the following the help
# the find module:
#
# If the package provides a configuration file, use -DOpenEXR_DIR=<path to folder>.
# If it doesn't provide it, try -DOpenEXR_ROOT=<path to folder with lib and includes>.
# Alternatively, try -DOpenEXR_LIBRARY=<path to lib file> and -DOpenEXR_INCLUDE_DIR=<path to folder>.
#

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_OpenEXR_REQUIRED_VARS OpenEXR_LIBRARY)
    set(_OpenEXR_LIB_VER "${OpenEXR_FIND_VERSION_MAJOR}_${OpenEXR_FIND_VERSION_MINOR}")

    if(NOT DEFINED OpenEXR_ROOT)
        # Search for OpenEXRConfig.cmake
        find_package(OpenEXR ${OpenEXR_FIND_VERSION} CONFIG QUIET)
    endif()

    if(OpenEXR_FOUND)
        get_target_property(OpenEXR_LIBRARY OpenEXR::OpenEXR LOCATION)
        get_target_property(OpenEXR_INCLUDE_DIR OpenEXR::OpenEXR INTERFACE_INCLUDE_DIRECTORIES)
        
        # IMPORTED_GLOBAL property must be set to TRUE since alisasing a non-global imported target
        # is not possible until CMake 3.18+.
        set_target_properties(OpenEXR::OpenEXR PROPERTIES IMPORTED_GLOBAL TRUE)
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(OpenEXR_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(OpenEXR
        REQUIRED_VARS
            ${_OpenEXR_REQUIRED_VARS}
        VERSION_VAR
            OpenEXR_VERSION
    )
endif()
