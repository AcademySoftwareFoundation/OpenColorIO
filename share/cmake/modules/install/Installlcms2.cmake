# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Install lcms2
#
# Variables defined by this module:
#   lcms2_FOUND          - Indicate whether the library was found or not
#   lcms2_LIBRARY        - Path to the library file
#   lcms2_INCLUDE_DIR    - Location of the header files
#   lcms2_VERSION        - Library's version
#
# Global targets defined by this module:
#   lcms2::lcms2  
#


###############################################################################
### Configure target ###

if(NOT TARGET lcms2::lcms2)
    add_library(lcms2::lcms2 UNKNOWN IMPORTED GLOBAL)
    set(_lcms2_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT lcms2_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)

    set(_EXT_DIST_ROOT "${PROJECT_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${PROJECT_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(lcms2_FOUND TRUE)
    if(OCIO_lcms2_RECOMMENDED_VERSION)
        set(lcms2_VERSION ${OCIO_lcms2_RECOMMENDED_VERSION})
    else()
        set(lcms2_VERSION ${lcms2_FIND_VERSION})
    endif()
    set(lcms2_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}/lcms2")
    set(lcms2_LIBRARY 
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}lcms2${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_lcms2_TARGET_CREATE)
        if(UNIX)
            set(lcms2_C_FLAGS "${lcms2_C_FLAGS} -fvisibility=hidden -fPIC")
        endif()

        if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
            set(lcms2_C_FLAGS "${lcms2_C_FLAGS} -Wno-aggressive-loop-optimizations")
        endif()

        string(STRIP "${lcms2_C_FLAGS}" lcms2_C_FLAGS)

        # NOTE: Depending of the compiler version lcm2 2.2 does not compile with C++17 so revert 
        # to C++11 because the library is only used by a cmd line tool.

        set(lcms2_CXX_STANDARD ${CMAKE_CXX_STANDARD})
        if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 17)
            set(lcms2_CXX_STANDARD 11)
        endif()

        set(lcms2_CMAKE_ARGS
            ${lcms2_CMAKE_ARGS}
            -DCMAKE_C_FLAGS=${lcms2_C_FLAGS}
            -DCMAKE_CXX_STANDARD=${lcms2_CXX_STANDARD}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
            -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DBUILD_SHARED_LIBS=OFF
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(lcms2_CMAKE_ARGS
                ${lcms2_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            string(REPLACE ";" "$<SEMICOLON>" ESCAPED_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

            set(lcms2_CMAKE_ARGS
                ${lcms2_CMAKE_ARGS}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_OSX_ARCHITECTURES=${ESCAPED_CMAKE_OSX_ARCHITECTURES}
            )
        endif()

        if (ANDROID)
            set(lcms2_CMAKE_ARGS
                ${lcms2_CMAKE_ARGS}
                -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${lcms2_INCLUDE_DIR})

        ExternalProject_Add(lcms2_install
            GIT_REPOSITORY "https://github.com/mm2/Little-CMS.git"
            GIT_TAG "lcms${lcms2_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/Little-CMS"
            BUILD_BYPRODUCTS ${lcms2_LIBRARY}
            CMAKE_ARGS ${lcms2_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
            PATCH_COMMAND
                ${CMAKE_COMMAND} -E copy
                "${PROJECT_SOURCE_DIR}/share/cmake/projects/Buildlcms2.cmake"
                "CMakeLists.txt"
            BUILD_COMMAND ""
            INSTALL_COMMAND
                ${CMAKE_COMMAND} --build .
                                 --config ${CMAKE_BUILD_TYPE}
                                 --target install
                                 --parallel
        )

        add_dependencies(lcms2::lcms2 lcms2_install)
        if(OCIO_VERBOSE)
            message(STATUS "Installing lcms2: ${lcms2_LIBRARY} (version \"${lcms2_VERSION}\")")
        endif()
    endif()
endif()

###############################################################################
### Configure target ###

if(_lcms2_TARGET_CREATE)
    set_target_properties(lcms2::lcms2 PROPERTIES
        IMPORTED_LOCATION ${lcms2_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${lcms2_INCLUDE_DIR}
    )

    mark_as_advanced(lcms2_INCLUDE_DIR lcms2_LIBRARY lcms2_VERSION)
endif()
