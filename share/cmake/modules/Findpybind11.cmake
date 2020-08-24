# Locate or install pybind11
#
# Variables defined by this module:
#   pybind11_FOUND - If FALSE, do not try to link to pybind11
#   pybind11_INCLUDE_DIR - Where to find pybind11.h
#   pybind11_VERSION - The version of the library
#
# Targets defined by this module:
#   pybind11::module - IMPORTED target, if found
#
# If pybind11 is not installed in a standard path, you can use the 
# pybind11_ROOT variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, pybind11 will be 
# downloaded at build time.
#

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    if(NOT DEFINED pybind11_ROOT)
        # Search for pybind11Config.cmake
        find_package(pybind11 ${pybind11_FIND_VERSION} CONFIG QUIET)
    endif()

    if(NOT pybind11_FOUND)
        # Find include directory
        find_path(pybind11_INCLUDE_DIR
            NAMES
                pybind11/pybind11.h
            HINTS
                ${pybind11_ROOT}
            PATH_SUFFIXES
                include
                pybind11/include
        )

        # Version information can be extracted from this header
        set(_pybind11_COMMON_H "${pybind11_INCLUDE_DIR}/pybind11/detail/common.h")

        if(EXISTS "${_pybind11_COMMON_H}")
            file(STRINGS "${_pybind11_COMMON_H}" _pybind11_VER_SEARCH 
                REGEX "^[ \t]*#define[ \t]+PYBIND11_VERSION_(MAJOR|MINOR|PATCH)[ \t]+([0-9]+).*$")

            if(_pybind11_VER_SEARCH)
                string(REGEX REPLACE ".*MAJOR[ \t]+([0-9]+).*" 
                    "\\1" _pybind11_MAJOR "${_pybind11_VER_SEARCH}")
                string(REGEX REPLACE ".*MINOR[ \t]+([0-9]+).*" 
                    "\\1" _pybind11_MINOR "${_pybind11_VER_SEARCH}")
                string(REGEX REPLACE ".*PATCH[ \t]+([0-9]+).*" 
                    "\\1" _pybind11_PATCH "${_pybind11_VER_SEARCH}")

                set(pybind11_VERSION "${_pybind11_MAJOR}.${_pybind11_MINOR}.${_pybind11_PATCH}")
            endif()
        endif()
    endif()

    if(NOT pybind11_VERSION)
        # Check if pybind11 was installed as a Python package (i.e. with 
        # "pip install pybind11"). This requires a Python interpreter.
        find_package(Python QUIET COMPONENTS Interpreter)

        if(Python_Interpreter_FOUND)
            execute_process(
                COMMAND
                    "${Python_EXECUTABLE}" -c 
                    "print(__import__('pybind11').__version__)"
                RESULTS_VARIABLE
                    _pybind11_VER_RESULTS
                OUTPUT_VARIABLE
                    _pybind11_VER_OUTPUT
                ERROR_QUIET
            )
            if(_pybind11_VER_OUTPUT)
                # Strip \n from python output
                string(STRIP ${_pybind11_VER_OUTPUT} _pybind11_VER_OUTPUT)
            endif()

            if(_pybind11_VER_RESULTS EQUAL 0 AND "${_pybind11_VER_OUTPUT}" MATCHES "[.0-9]+")
                execute_process(
                    COMMAND
                        "${Python_EXECUTABLE}" -c 
                        "import os;\
                            import pybind11;\
                            print(os.path.join(os.path.dirname(pybind11.__file__), 'include'))"
                    RESULTS_VARIABLE
                        _pybind11_DIR_RESULTS
                    OUTPUT_VARIABLE
                        _pybind11_DIR_OUTPUT
                    ERROR_QUIET
                )
                if(_pybind11_DIR_OUTPUT)
                    # Strip \n from python output
                    string(STRIP ${_pybind11_DIR_OUTPUT} _pybind11_DIR_OUTPUT)
                endif()

                if(_pybind11_DIR_RESULTS EQUAL 0 AND EXISTS "${_pybind11_DIR_OUTPUT}")
                    set(pybind11_VERSION ${_pybind11_VER_OUTPUT})
                    set(pybind11_INCLUDE_DIR ${_pybind11_DIR_OUTPUT})
                    message(STATUS 
                        "Using pybind11 python package (version \"${pybind11_VERSION}\", "
                        "found in python \"${Python_VERSION}\")"
                    )
                endif()
            endif()
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(pybind11_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(pybind11
        REQUIRED_VARS 
            pybind11_INCLUDE_DIR
        VERSION_VAR
            pybind11_VERSION
    )
endif()

###############################################################################
### Create target ###

if(NOT TARGET pybind11::module)
    add_library(pybind11::module INTERFACE IMPORTED GLOBAL)
    set(_pybind11_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT pybind11_FOUND)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(pybind11_FOUND TRUE)
    set(pybind11_VERSION ${pybind11_FIND_VERSION})
    set(pybind11_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    if(_pybind11_TARGET_CREATE)
        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${pybind11_INCLUDE_DIR})

        set(pybind11_CMAKE_ARGS
            ${pybind11_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DPYBIND11_INSTALL=ON
            -DPYBIND11_TEST=OFF
        )

        ExternalProject_Add(pybind11_install
            GIT_REPOSITORY "https://github.com/pybind/pybind11.git"
            GIT_TAG "v${pybind11_FIND_VERSION}"
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/pybind11"
            BUILD_BYPRODUCTS ${pybind11_INCLUDE_DIR}
            CMAKE_ARGS ${pybind11_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(pybind11::module pybind11_install)
        message(STATUS "Installing pybind11: ${pybind11_INCLUDE_DIR} (version \"${pybind11_VERSION}\")")
    endif()
endif()

###############################################################################
### Configure target ###

if(_pybind11_TARGET_CREATE)
    set_target_properties(pybind11::module PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${pybind11_INCLUDE_DIR}
    )

    mark_as_advanced(pybind11_INCLUDE_DIR pybind11_VERSION)
endif()
