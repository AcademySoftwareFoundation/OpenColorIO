# Locate or install pybind11
#
# Variables defined by this module:
#   PYBIND11_FOUND - If FALSE, do not try to link to pybind11
#   PYBIND11_INCLUDE_DIR - Where to find pybind11.h
#
# Targets defined by this module:
#   pybind11::pybind11 - IMPORTED target, if found
#
# If pybind11 is not installed in a standard path, you can use the 
# PYBIND11_DIRS variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, pybind11 will be 
# downloaded at build time.
#

if(NOT TARGET pybind11::pybind11)
    add_library(pybind11::pybind11 INTERFACE IMPORTED GLOBAL)
    set(_PYBIND11_TARGET_CREATE TRUE)
endif()

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_PYBIND11_SEARCH_DIRS
        ${PYBIND11_DIRS}
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw        # Fink
        /opt/local # DarwinPorts
        /opt/csw   # Blastwave
        /opt
    )

    # Find include directory
    find_path(PYBIND11_INCLUDE_DIR
        NAMES
            pybind11/pybind11.h
        HINTS
            ${_PYBIND11_SEARCH_DIRS}
        PATH_SUFFIXES
            include
            pybind11/include
    )

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(Pybind11_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Pybind11
        REQUIRED_VARS 
            PYBIND11_INCLUDE_DIR
    )
    set(PYBIND11_FOUND ${Pybind11_FOUND})
endif()

###############################################################################
### Install package from source ###

if(NOT PYBIND11_FOUND)
    include(ExternalProject)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(PYBIND11_FOUND TRUE)
    set(PYBIND11_VERSION ${Pybind11_FIND_VERSION})
    set(PYBIND11_INCLUDE_DIR "${_EXT_DIST_ROOT}/include")

    if(_PYBIND11_TARGET_CREATE)
        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${PYBIND11_INCLUDE_DIR})

        set(PYBIND11_CMAKE_ARGS
            ${PYBIND11_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DPYBIND11_INSTALL:BOOL=ON
            -DPYBIND11_TEST:BOOL=OFF
        )

        ExternalProject_Add(pybind11_install
            GIT_REPOSITORY "https://github.com/pybind/pybind11.git"
            GIT_TAG "v${Pybind11_FIND_VERSION}"
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/pybind11"
            BUILD_BYPRODUCTS ${PYBIND11_INCLUDE_DIR}
            CMAKE_ARGS ${PYBIND11_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(pybind11::pybind11 pybind11_install)
        message(STATUS "Installing Pybind11: ${PYBIND11_INCLUDE_DIR} (version ${PYBIND11_VERSION})")
    endif()
endif()

###############################################################################
### Configure target ###

if(_PYBIND11_TARGET_CREATE)
    set_target_properties(pybind11::pybind11 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${PYBIND11_INCLUDE_DIR}
    )

    mark_as_advanced(PYBIND11_INCLUDE_DIR PYBIND11_VERSION)
endif()
