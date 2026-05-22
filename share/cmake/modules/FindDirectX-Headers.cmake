# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate DirectX-Headers (header-only, Windows only)
#
# Variables defined by this module:
#   DirectX-Headers_FOUND        - Indicate whether the package was found or not
#   DirectX-Headers_INCLUDE_DIR  - Location of the header files
#   DirectX-Headers_VERSION      - Package version
#
# Global targets defined by this module:
#   Microsoft::DirectX-Headers
#
# DirectX-Headers can be supplied by the caller through any of the standard
# CMake mechanisms:
# -- Set -DDirectX-Headers_DIR to the directory containing directx-headers-config.cmake
# -- Set -DDirectX-Headers_ROOT to the install prefix (with include/directx/ underneath)
# -- Set -DDirectX-Headers_INCLUDE_DIR to the directory containing directx/d3d12.h
#
# When OCIO_INSTALL_EXT_PACKAGES is not ALL, this module first tries to locate
# an existing install via the upstream CMake config, then falls back to a
# manual header search. If still not found and OCIO_INSTALL_EXT_PACKAGES is
# MISSING (the default), OCIO's ocio_install_dependency() pathway will invoke
# InstallDirectX-Headers.cmake to build it via FetchContent.
#

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Prefer the upstream CMake config (installed as lower-case).
    find_package(directx-headers ${DirectX-Headers_FIND_VERSION} CONFIG QUIET)

    if(directx-headers_FOUND)
        set(DirectX-Headers_FOUND TRUE)
        if(directx-headers_VERSION)
            set(DirectX-Headers_VERSION ${directx-headers_VERSION})
        endif()
    else()
        # Fall back to locating the public header directly (e.g. when the
        # headers were installed without the CMake config, or are provided
        # by a vendored copy).
        find_path(DirectX-Headers_INCLUDE_DIR
            NAMES
                directx/d3d12.h
            HINTS
                ${DirectX-Headers_ROOT}
            PATH_SUFFIXES
                include
        )
    endif()

    # If OCIO can install the package itself, demote REQUIRED so a missing
    # dependency here does not abort configuration before the install step.
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(DirectX-Headers_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(DirectX-Headers
        REQUIRED_VARS
            DirectX-Headers_INCLUDE_DIR
        VERSION_VAR
            DirectX-Headers_VERSION
    )
endif()

###############################################################################
### Create target (only needed for the manual-header-search fallback; the
### upstream CMake config already defines Microsoft::DirectX-Headers).

if(DirectX-Headers_FOUND AND NOT TARGET Microsoft::DirectX-Headers AND DirectX-Headers_INCLUDE_DIR)
    add_library(Microsoft::DirectX-Headers INTERFACE IMPORTED GLOBAL)
    set_target_properties(Microsoft::DirectX-Headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${DirectX-Headers_INCLUDE_DIR}"
    )

    mark_as_advanced(DirectX-Headers_INCLUDE_DIR DirectX-Headers_VERSION)
endif()
