# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Install expat
#
# Variables defined by this module:
#   expat_FOUND          - Indicate whether the library was found or not
#   expat_LIBRARY        - Path to the library file
#   expat_INCLUDE_DIR    - Location of the header files
#   expat_VERSION        - Library's version
#
# Global targets defined by this module:
#   expat::expat         
#


###############################################################################
### Create target

if(NOT TARGET expat::expat)
    add_library(expat::expat UNKNOWN IMPORTED GLOBAL)
    set(_expat_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT expat_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${PROJECT_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${PROJECT_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(expat_FOUND TRUE)
    if(OCIO_expat_RECOMMENDED_VERSION)
        set(expat_VERSION ${OCIO_expat_RECOMMENDED_VERSION})
    else()
        set(expat_VERSION ${expat_FIND_VERSION})
    endif()
    set(expat_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Set the expected library name
    if(WIN32)
        if(BUILD_TYPE_DEBUG)
            set(_expat_LIB_SUFFIX "d")
        endif()
        # Static Linking, Multi-threaded Dll naming (>=2.2.8):
        #   https://github.com/libexpat/libexpat/blob/R_2_2_8/expat/win32/README.txt
        set(_expat_LIB_SUFFIX "${_expat_LIB_SUFFIX}MD")
    endif()

    # Expat use a hardcoded lib prefix instead of CMAKE_STATIC_LIBRARY_PREFIX
    # https://github.com/libexpat/libexpat/blob/R_2_4_1/expat/CMakeLists.txt#L374
    set(_expat_LIB_PREFIX "lib")

    set(expat_LIBRARY
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${_expat_LIB_PREFIX}expat${_expat_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_expat_TARGET_CREATE)
        if(MSVC)
            set(EXPAT_C_FLAGS "${EXPAT_C_FLAGS} /EHsc")
            set(EXPAT_CXX_FLAGS "${EXPAT_CXX_FLAGS} /EHsc")
        endif()

        string(STRIP "${EXPAT_C_FLAGS}" EXPAT_C_FLAGS)
        string(STRIP "${EXPAT_CXX_FLAGS}" EXPAT_CXX_FLAGS)

        set(EXPAT_CMAKE_ARGS
            ${EXPAT_CMAKE_ARGS}
            -DCMAKE_POLICY_DEFAULT_CMP0063=NEW
            -DCMAKE_C_VISIBILITY_PRESET=${CMAKE_C_VISIBILITY_PRESET}
            -DCMAKE_CXX_VISIBILITY_PRESET=${CMAKE_CXX_VISIBILITY_PRESET}
            -DCMAKE_VISIBILITY_INLINES_HIDDEN=${CMAKE_VISIBILITY_INLINES_HIDDEN}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_C_FLAGS=${EXPAT_C_FLAGS}
            -DCMAKE_CXX_FLAGS=${EXPAT_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
            -DCMAKE_INSTALL_DATADIR=${CMAKE_INSTALL_DATADIR}
            -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DEXPAT_BUILD_DOCS=OFF
            -DEXPAT_BUILD_EXAMPLES=OFF
            -DEXPAT_BUILD_TESTS=OFF
            -DEXPAT_BUILD_TOOLS=OFF
            -DEXPAT_SHARED_LIBS=OFF
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(EXPAT_CMAKE_ARGS
                ${EXPAT_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            string(REPLACE ";" "$<SEMICOLON>" ESCAPED_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

            set(EXPAT_CMAKE_ARGS
                ${EXPAT_CMAKE_ARGS}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_OSX_ARCHITECTURES=${ESCAPED_CMAKE_OSX_ARCHITECTURES}
            )
        endif()

        if (ANDROID)
            set(EXPAT_CMAKE_ARGS
                ${EXPAT_CMAKE_ARGS}
                -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${expat_INCLUDE_DIR})

        string(REPLACE "." ";" VERSION_LIST ${expat_VERSION})
        list(GET VERSION_LIST 0 expat_VERSION_MAJOR)
        list(GET VERSION_LIST 1 expat_VERSION_MINOR)
        list(GET VERSION_LIST 2 expat_VERSION_PATCH)

        ExternalProject_Add(expat_install
            GIT_REPOSITORY "https://github.com/libexpat/libexpat.git"
            GIT_TAG "R_${expat_VERSION_MAJOR}_${expat_VERSION_MINOR}_${expat_VERSION_PATCH}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/libexpat"
            BUILD_BYPRODUCTS ${expat_LIBRARY}
            SOURCE_SUBDIR expat
            CMAKE_ARGS ${EXPAT_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
            BUILD_COMMAND ""
            INSTALL_COMMAND
                ${CMAKE_COMMAND} --build .
                                 --config ${CMAKE_BUILD_TYPE}
                                 --target install
                                 --parallel
        )

        add_dependencies(expat::expat expat_install)
        if(OCIO_VERBOSE)
            message(STATUS "Installing expat: ${expat_LIBRARY} (version \"${expat_VERSION}\")")
        endif()
    endif()
endif()

###############################################################################
### Configure target ###

if(_expat_TARGET_CREATE)
    set_target_properties(expat::expat PROPERTIES
        IMPORTED_LOCATION ${expat_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${expat_INCLUDE_DIR}
    )

    mark_as_advanced(expat_INCLUDE_DIR expat_LIBRARY expat_VERSION)
endif()
