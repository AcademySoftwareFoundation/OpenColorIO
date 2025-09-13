# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Install pybind11
#
# Variables defined by this module:
#   pybind11_FOUND          - Indicate whether the library was found or not
#   pybind11_INCLUDE_DIR    - Location of the header files
#   pybind11_VERSION        - Library's version
#
# Global targets defined by this module:
#   pybind11::module         
#


###############################################################################
### Create target ###

if(NOT TARGET pybind11::module)
    add_library(pybind11::module INTERFACE IMPORTED GLOBAL)
    set(_pybind11_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT pybind11_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${PROJECT_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${PROJECT_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(pybind11_FOUND TRUE)
    if(OCIO_pybind11_RECOMMENDED_VERSION)
        set(pybind11_VERSION ${OCIO_pybind11_RECOMMENDED_VERSION})
    else()
        set(pybind11_VERSION ${pybind11_FIND_VERSION})
    endif()
    set(pybind11_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    if(_pybind11_TARGET_CREATE)
        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${pybind11_INCLUDE_DIR})

        set(pybind11_CMAKE_ARGS
            ${pybind11_CMAKE_ARGS}
            # Required for CMake 4.0+ compatibility.
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
            -DCMAKE_INSTALL_DATADIR=${CMAKE_INSTALL_DATADIR}
            -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            # Using FindPython mode (PYBIND11_FINDPYTHON=ON) doesn't seem to
            # work when building on docker manylinux images where Development
            # component is not available but is hardcoded in pybind11 script.
            -DPYTHON_EXECUTABLE=${Python_EXECUTABLE}
            -DPYBIND11_INSTALL=ON
            -DPYBIND11_TEST=OFF
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(pybind11_CMAKE_ARGS
                ${pybind11_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            string(REPLACE ";" "$<SEMICOLON>" ESCAPED_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

            set(pybind11_CMAKE_ARGS
                ${pybind11_CMAKE_ARGS}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_OSX_ARCHITECTURES=${ESCAPED_CMAKE_OSX_ARCHITECTURES}
            )
        endif()

        if (ANDROID)
            set(pybind11_CMAKE_ARGS
                ${pybind11_CMAKE_ARGS}
                -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL})
        endif()

        ExternalProject_Add(pybind11_install
            GIT_REPOSITORY "https://github.com/pybind/pybind11.git"
            GIT_TAG "v${pybind11_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/pybind11"
            BUILD_BYPRODUCTS ${pybind11_INCLUDE_DIR}
            CMAKE_ARGS ${pybind11_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
            BUILD_COMMAND ""
            INSTALL_COMMAND
                ${CMAKE_COMMAND} --build .
                                 --config ${CMAKE_BUILD_TYPE}
                                 --target install
                                 --parallel
        )

        add_dependencies(pybind11::module pybind11_install)
        if(OCIO_VERBOSE)
            message(STATUS "Installing pybind11: ${pybind11_INCLUDE_DIR} (version \"${pybind11_VERSION}\")")
        endif()
    endif()
endif()

###############################################################################
### Configure target ###

if(_pybind11_TARGET_CREATE)
    set_target_properties(pybind11::module PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${pybind11_INCLUDE_DIR}
    )

    # See pybind11Common.cmake for reasoning
    if (MSVC)
        set_target_properties(pybind11::module PROPERTIES
            INTERFACE_COMPILE_OPTIONS /bigobj
        )

    endif()

    mark_as_advanced(pybind11_INCLUDE_DIR pybind11_VERSION)
endif()
