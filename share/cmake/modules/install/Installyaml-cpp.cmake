# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Install yaml-cpp
#
# Variables defined by this module:
#   yaml-cpp_FOUND          - Indicate whether the library was found or not
#   yaml-cpp_LIBRARY        - Path to the library file
#   yaml-cpp_INCLUDE_DIR    - Location of the header files
#   yaml-cpp_VERSION        - Library's version
#
# Global targets defined by this module:
#   yaml-cpp::yaml-cpp
#

###############################################################################
### Create target (if previous 'find_package' call hasn't) ###

if(NOT TARGET yaml-cpp::yaml-cpp)
    add_library(yaml-cpp::yaml-cpp UNKNOWN IMPORTED GLOBAL)
    set(_yaml-cpp_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT yaml-cpp_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${PROJECT_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${PROJECT_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(yaml-cpp_FOUND TRUE)
    if(OCIO_yaml-cpp_RECOMMENDED_VERSION)
        set(yaml-cpp_VERSION ${OCIO_yaml-cpp_RECOMMENDED_VERSION})
    else()
        set(yaml-cpp_VERSION ${yaml-cpp_FIND_VERSION})
    endif()
    set(yaml-cpp_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Starting from 0.7.0, this is included on all platforms, we could also
    # override CMAKE_DEBUG_POSTFIX to bypass it.
    if(BUILD_TYPE_DEBUG)
        string(APPEND _yaml-cpp_LIB_SUFFIX "d")
    endif()

    # Set the expected library name
    set(yaml-cpp_LIBRARY
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}yaml-cpp${_yaml-cpp_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_yaml-cpp_TARGET_CREATE)
        if(MSVC)
            set(yaml-cpp_CXX_FLAGS "${yaml-cpp_CXX_FLAGS} /EHsc")
        endif()

        if(UNIX)
            if(USE_CLANG)
                # Remove some global 'shadow' warnings.
                set(yaml-cpp_CXX_FLAGS "${yaml-cpp_CXX_FLAGS} -Wno-shadow")
            endif()
        endif()

        string(STRIP "${yaml-cpp_CXX_FLAGS}" yaml-cpp_CXX_FLAGS)

        set(yaml-cpp_CMAKE_ARGS
            ${yaml-cpp_CMAKE_ARGS}
            -DCMAKE_POLICY_DEFAULT_CMP0063=NEW
            # Required for CMake 4.0+ compatibility
            -DCMAKE_POLICY_VERSION_MINIMUM=3.5
            -DCMAKE_CXX_VISIBILITY_PRESET=${CMAKE_CXX_VISIBILITY_PRESET}
            -DCMAKE_VISIBILITY_INLINES_HIDDEN=${CMAKE_VISIBILITY_INLINES_HIDDEN}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${yaml-cpp_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
            -DCMAKE_INSTALL_DATADIR=${CMAKE_INSTALL_DATADIR}
            -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DBUILD_SHARED_LIBS=OFF
            -DYAML_BUILD_SHARED_LIBS=OFF
            -DYAML_CPP_BUILD_TESTS=OFF
            -DYAML_CPP_BUILD_TOOLS=OFF
            -DYAML_CPP_BUILD_CONTRIB=OFF
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(yaml-cpp_CMAKE_ARGS
                ${yaml-cpp_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            string(REPLACE ";" "$<SEMICOLON>" ESCAPED_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

            set(yaml-cpp_CMAKE_ARGS
                ${yaml-cpp_CMAKE_ARGS}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_OSX_ARCHITECTURES=${ESCAPED_CMAKE_OSX_ARCHITECTURES}
            )
        endif()


        if (ANDROID)
            set(yaml-cpp_CMAKE_ARGS
                ${yaml-cpp_CMAKE_ARGS}
                -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL})
        endif()

        # in v0.8.0 yaml switched from "yaml-cpp-vA.B.C" to "vA.B.C" format for tags.
        set(yaml-cpp_GIT_TAG "${yaml-cpp_VERSION}")

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${yaml-cpp_INCLUDE_DIR})

        ExternalProject_Add(yaml-cpp_install
            GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git"
            GIT_TAG ${yaml-cpp_GIT_TAG}
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/yaml-cpp"
            BUILD_BYPRODUCTS ${yaml-cpp_LIBRARY}
            CMAKE_ARGS ${yaml-cpp_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
            BUILD_COMMAND ""
            INSTALL_COMMAND
                ${CMAKE_COMMAND} --build .
                                 --config ${CMAKE_BUILD_TYPE}
                                 --target install
                                 --parallel
        )

        add_dependencies(yaml-cpp::yaml-cpp yaml-cpp_install)
        if(OCIO_VERBOSE)
            message(STATUS "Installing yaml-cpp: ${yaml-cpp_LIBRARY} (version \"${yaml-cpp_VERSION}\")")
        endif()
    endif()
endif()

###############################################################################
### Configure target ###

if(_yaml-cpp_TARGET_CREATE)
    set_target_properties(yaml-cpp::yaml-cpp PROPERTIES
        IMPORTED_LOCATION ${yaml-cpp_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${yaml-cpp_INCLUDE_DIR}
        # https://github.com/jbeder/yaml-cpp/issues/1339
        INTERFACE_COMPILE_DEFINITIONS YAML_CPP_STATIC_DEFINE
    )

    mark_as_advanced(yaml-cpp_INCLUDE_DIR yaml-cpp_LIBRARY yaml-cpp_VERSION)
endif()
