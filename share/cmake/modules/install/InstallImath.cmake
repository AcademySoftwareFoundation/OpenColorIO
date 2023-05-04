# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Install Imath
#
# Variables defined by this module:
#   Imath_FOUND          - Indicate whether the library was found or not
#   Imath_LIBRARY        - Path to the library file
#   Imath_INCLUDE_DIR    - Location of the header files
#   Imath_VERSION        - Library's version
#
# Global targets defined by this module:
#   Imath::Imath
#   Imath::ImathConfig     
#


###############################################################################
### Create target

if (NOT TARGET Imath::Imath)
    add_library(Imath::Imath UNKNOWN IMPORTED GLOBAL)
    add_library(Imath::ImathConfig INTERFACE IMPORTED GLOBAL)
    set(_Imath_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT Imath_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${PROJECT_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${PROJECT_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(Imath_FOUND TRUE)
    if(OCIO_Imath_RECOMMENDED_VERSION)
        set(Imath_VERSION ${OCIO_Imath_RECOMMENDED_VERSION})
    else()
        set(Imath_VERSION ${Imath_FIND_VERSION})
    endif()
    set(Imath_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Set the expected library name
    if(BUILD_TYPE_DEBUG)
        set(_Imath_LIB_SUFFIX "_d")
    endif()

    include(VersionUtils)
    split_version_string(${Imath_VERSION} _Imath_ExternalProject)

    set(_Imath_LIB_VER "${_Imath_ExternalProject_VERSION_MAJOR}_${_Imath_ExternalProject_VERSION_MINOR}")

    set(Imath_LIBRARY
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}Imath-${_Imath_LIB_VER}${_Imath_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_Imath_TARGET_CREATE)
        if(MSVC)
            set(Imath_CXX_FLAGS "${Imath_CXX_FLAGS} /EHsc")
        endif()

        string(STRIP "${Imath_CXX_FLAGS}" Imath_CXX_FLAGS)
 
        set(Imath_CMAKE_ARGS
            ${Imath_CMAKE_ARGS}
            -DCMAKE_CXX_VISIBILITY_PRESET=${CMAKE_CXX_VISIBILITY_PRESET}
            -DCMAKE_VISIBILITY_INLINES_HIDDEN=${CMAKE_VISIBILITY_INLINES_HIDDEN}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${Imath_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
            -DCMAKE_INSTALL_DATADIR=${CMAKE_INSTALL_DATADIR}
            -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DBUILD_SHARED_LIBS=OFF
            -DBUILD_TESTING=OFF
            -DPYTHON=OFF
            -DDOCS=OFF
            -DIMATH_HALF_USE_LOOKUP_TABLE=OFF
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(Imath_CMAKE_ARGS
                ${Imath_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            string(REPLACE ";" "$<SEMICOLON>" ESCAPED_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

            set(Imath_CMAKE_ARGS
                ${Imath_CMAKE_ARGS}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_OSX_ARCHITECTURES=${ESCAPED_CMAKE_OSX_ARCHITECTURES}
            )
        endif()

        if (ANDROID)
            set(Imath_CMAKE_ARGS
                ${Imath_CMAKE_ARGS}
                -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${Imath_INCLUDE_DIR})

        ExternalProject_Add(imath_install
            GIT_REPOSITORY "https://github.com/AcademySoftwareFoundation/Imath.git"
            GIT_TAG "v${Imath_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/Imath"
            BUILD_BYPRODUCTS ${Imath_LIBRARY}
            CMAKE_ARGS ${Imath_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
            BUILD_COMMAND ""
            INSTALL_COMMAND
                ${CMAKE_COMMAND} --build .
                                 --config ${CMAKE_BUILD_TYPE}
                                 --target install
                                 --parallel
        )

        add_dependencies(Imath::Imath imath_install)

        if(OCIO_VERBOSE)
            message(STATUS "Installing Imath: ${Imath_LIBRARY} (version \"${Imath_VERSION}\")")
        endif()
    endif()
endif()

###############################################################################
### Configure target ###

if(_Imath_TARGET_CREATE)
    file(MAKE_DIRECTORY ${Imath_INCLUDE_DIR}/Imath)

    set_target_properties(Imath::Imath PROPERTIES
        IMPORTED_LOCATION ${Imath_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES "${Imath_INCLUDE_DIR};${Imath_INCLUDE_DIR}/Imath"
    )
    set_target_properties(Imath::ImathConfig PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Imath_INCLUDE_DIR};${Imath_INCLUDE_DIR}/Imath"
    )

    mark_as_advanced(Imath_INCLUDE_DIR Imath_LIBRARY Imath_VERSION)
endif()
